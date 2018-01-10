#include <rai/node/rpc.hpp>

#include <boost/algorithm/string.hpp>

#include <rai/lib/interface.h>
#include <rai/node/node.hpp>

#include <ed25519-donna/ed25519.h>

#include "pyobject.hpp"

using namespace rai;
using namespace std;

static rai::node *s_node;

void set_node(rai::node *_node)
{
   s_node = _node;
}

bool decode_unsigned (std::string const & text, uint64_t & number)
{
   bool result;
   size_t end;
   try
   {
      number = std::stoull (text, &end);
      result = false;
   }
   catch (std::invalid_argument const &)
   {
      result = true;
   }
   catch (std::out_of_range const &)
   {
      result = true;
   }
   result = result || end != text.size ();
   return result;
}

namespace python
{

class history_visitor : public rai::block_visitor
{
public:
   history_visitor (rai::transaction & transaction_a, boost::property_tree::ptree & tree_a, rai::block_hash const & hash_a) :
   transaction (transaction_a),
   tree (tree_a),
   hash (hash_a)
   {
   }
   void send_block (rai::send_block const & block_a)
   {
      tree.put ("type", "send");
      auto account (block_a.hashables.destination.to_account ());
      tree.put ("account", account);
      auto amount (s_node->ledger.amount (transaction, hash).convert_to<std::string> ());
      tree.put ("amount", amount);
   }
   void receive_block (rai::receive_block const & block_a)
   {
      tree.put ("type", "receive");
      auto account (s_node->ledger.account (transaction, block_a.hashables.source).to_account ());
      tree.put ("account", account);
      auto amount (s_node->ledger.amount (transaction, hash).convert_to<std::string> ());
      tree.put ("amount", amount);
   }
   void open_block (rai::open_block const & block_a)
   {
      // Report opens as a receive
      tree.put ("type", "receive");
      if (block_a.hashables.source != rai::genesis_account)
      {
         tree.put ("account", s_node->ledger.account (transaction, block_a.hashables.source).to_account ());
         tree.put ("amount", s_node->ledger.amount (transaction, hash).convert_to<std::string> ());
      }
      else
      {
         tree.put ("account", rai::genesis_account.to_account ());
         tree.put ("amount", rai::genesis_amount.convert_to<std::string> ());
      }
   }
   void change_block (rai::change_block const &)
   {
      // Don't report change blocks
   }
   rai::transaction & transaction;
   boost::property_tree::ptree & tree;
   rai::block_hash const & hash;
};


int account_balance_ (string& account_text, string& _balance, string& _pending)
{
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      auto balance (s_node->balance_pending (account));
      _balance = balance.first.convert_to<std::string> ();
      _pending = balance.second.convert_to<std::string> ();
      return 1;
   }
   else
   {
      return 0;
   }
}



int account_block_count_ (std::string& account_text)
{
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      rai::transaction transaction (s_node->store.environment, nullptr, false);
      rai::account_info info;
      if (!s_node->store.account_get (transaction, account, info))
      {
         return info.block_count;
      }
   }
   return 0;
}

PyObject* account_info_ (std::string& account_text, bool representative, bool weight, bool pending)
{
   PyDict dict;
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (error)
   {
      return dict.get();
   }

   rai::transaction transaction (s_node->store.environment, nullptr, false);
   rai::account_info info;
   if (!s_node->store.account_get (transaction, account, info))
   {
      dict.add ("frontier", info.head.to_string ());
      dict.add ("open_block", info.open_block.to_string ());
      dict.add ("representative_block", info.rep_block.to_string ());
      std::string balance;
      rai::uint128_union (info.balance).encode_dec (balance);
      dict.add ("balance", balance);
      dict.add ("modified_timestamp", std::to_string (info.modified));
      dict.add ("block_count", std::to_string (info.block_count));
      if (representative)
      {
         auto block (s_node->store.block_get (transaction, info.rep_block));
         assert (block != nullptr);
         dict.add ("representative", block->representative ().to_account ());
      }
      if (weight)
      {
         auto account_weight (s_node->ledger.weight (transaction, account));
         dict.add ("weight", account_weight.convert_to<std::string> ());
      }
      if (pending)
      {
         auto account_pending (s_node->ledger.account_pending (transaction, account));
         dict.add ("pending", account_pending.convert_to<std::string> ());
      }
   }
   return dict.get();
}

PyObject* account_create_ (std::string& wallet_text, bool generate_work)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (s_node->wallets.items.find (wallet));
      if (existing != s_node->wallets.items.end ())
      {
         rai::account new_key (existing->second->deterministic_insert (generate_work));
         if (!new_key.is_zero ())
         {
            string _account = new_key.to_account ();
            return py_new_string(_account);
         }
         else
         {
            return py_new_exception ("Wallet is locked");
         }
      }
      else
      {
         return py_new_exception ("Wallet not found");
      }
   }
   else
   {
      return py_new_exception ("Bad wallet number");
   }
   return py_new_none();
}

PyObject* account_get_ (string& key_text)
{
   rai::uint256_union pub;
   auto error (pub.decode_hex (key_text));
   if (!error)
   {
      string _account = pub.to_account ();
      return py_new_string(_account);
   }
   else
   {
      return py_new_exception ("Bad public key");
   }
}

PyObject* account_history_ (string& account_text, int count)
{
   PyArray arr;
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      boost::property_tree::ptree response_l;
      boost::property_tree::ptree history;
      rai::transaction transaction (s_node->store.environment, nullptr, false);
      auto hash (s_node->ledger.latest (transaction, account));
      auto block (s_node->store.block_get (transaction, hash));
      while (block != nullptr && count > 0)
      {
         PyDict dict;
         boost::property_tree::ptree entry;
         history_visitor visitor (transaction, entry, hash);
         block->visit (visitor);
         if (!entry.empty ())
         {
            dict.add ("hash", hash.to_string ());
            for(auto it=entry.begin(); it!=entry.end(); it++)
            {
               string key = it->first;
               string value = it->second.data();
               dict.add(key, value);
            }
         }
         arr.append(dict.get());
         hash = block->previous ();
         block = s_node->store.block_get (transaction, hash);
         --count;
      }
      return arr.get();

   }
   else
   {
      return py_new_exception ("Bad account number");
   }
}

PyObject* account_list_ (std::string wallet_text)
{
   PyArray arr;
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (s_node->wallets.items.find (wallet));
      if (existing != s_node->wallets.items.end ())
      {
         rai::transaction transaction (s_node->store.environment, nullptr, false);
         for (auto i (existing->second->store.begin (transaction)), j (existing->second->store.end ()); i != j; ++i)
         {
            string _account = rai::uint256_union (i->first.uint256 ()).to_account ();
            arr.append(_account);
         }
         return arr.get();
      }
      else
      {
         return py_new_exception("Wallet not found");
      }
   }
   else
   {
      return py_new_exception("Bad wallet number");
   }
}

PyObject* account_move_ (std::string wallet_text, std::string source_text, std::vector<std::string> accounts_text)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (s_node->wallets.items.find (wallet));
      if (existing != s_node->wallets.items.end ())
      {
         auto wallet (existing->second);
         rai::uint256_union source;
         auto error (source.decode_hex (source_text));
         if (!error)
         {
            auto existing (s_node->wallets.items.find (source));
            if (existing != s_node->wallets.items.end ())
            {
               auto source (existing->second);
               std::vector<rai::public_key> accounts;
               for (auto i (accounts_text.begin ()), n (accounts_text.end ()); i != n; ++i)
               {
                  rai::public_key account;
//                  printf("%s\n",(*i).c_str());
                  account.decode_hex (*i);
                  accounts.push_back (account);
               }
               rai::transaction transaction (s_node->store.environment, nullptr, true);
               auto error (wallet->store.move (transaction, source->store, accounts));
               return py_new_int(error ? 0 : 1);
            }
            else
            {
               return py_new_exception("Source not found");
            }
         }
         else
         {
            return py_new_exception("Bad source number");
         }
      }
      else
      {
         return py_new_exception("Wallet not found");
      }
   }
   else
   {
      return py_new_exception("Bad wallet number");
   }

}

PyObject* account_key_ (std::string account_text)
{
   rai::account account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      auto _account = account.to_string ();
      return py_new_string(_account);
   }
   else
   {
      return py_new_exception("Bad account number");
   }
}

PyObject* account_remove_ (std::string wallet_text, std::string account_text)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (s_node->wallets.items.find (wallet));
      if (existing != s_node->wallets.items.end ())
      {
         auto wallet (existing->second);
         rai::transaction transaction (s_node->store.environment, nullptr, true);
         if (existing->second->store.valid_password (transaction))
         {
            rai::account account_id;
            auto error (account_id.decode_account (account_text));
            if (!error)
            {
               auto account (wallet->store.find (transaction, account_id));
               if (account != wallet->store.end ())
               {
                  wallet->store.erase (transaction, account_id);
                  return py_new_int(1);
               }
               else
               {
                  return py_new_exception ("Account not found in wallet");
               }
            }
            else
            {
               return py_new_exception ("Bad account number");
            }
         }
         else
         {
            return py_new_exception ("Wallet locked");
         }
      }
      else
      {
         return py_new_exception ("Wallet not found");
      }
   }
   else
   {
      return py_new_exception ("Bad wallet number");
   }

}

std::string wallet_create_ ()
{
   rai::keypair wallet_id;
   auto wallet (s_node->wallets.create (wallet_id.pub));
   return wallet_id.pub.to_string ();
}


PyObject* block_count_ ()
{
   PyDict dict;
   rai::transaction transaction (s_node->store.environment, nullptr, false);
   dict.add ("count", std::to_string (s_node->store.block_count (transaction).sum ()));
   dict.add ("unchecked", std::to_string (s_node->store.unchecked_count (transaction)));
   return dict.get();
}



}
