#include "pyobject.hpp"
#include "rai_.hpp"


static rai::node *s_node;
void set_node(rai::node *_node)
{
   s_node = _node;
}

namespace python
{

static const char* last_error;
PyObject* set_last_error(const char *error)
{
   last_error = error;
   return py_new_none();
}

const char* get_last_error_()
{
   return last_error;
}

static pyrai* s_rai = NULL;

pyrai* get_pyrai()
{
   if (!s_rai)
   {
      s_rai = new pyrai(s_node);
   }
   return s_rai;
}

class history_visitor : public rai::block_visitor
{
public:
   history_visitor (rai::node* _node, rai::transaction & transaction_a, boost::property_tree::ptree & tree_a, rai::block_hash const & hash_a) :
   node(*_node),
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
      auto amount (node.ledger.amount (transaction, hash).convert_to<string> ());
      tree.put ("amount", amount);
   }
   void receive_block (rai::receive_block const & block_a)
   {
      tree.put ("type", "receive");
      auto account (node.ledger.account (transaction, block_a.hashables.source).to_account ());
      tree.put ("account", account);
      auto amount (node.ledger.amount (transaction, hash).convert_to<string> ());
      tree.put ("amount", amount);
   }
   void open_block (rai::open_block const & block_a)
   {
      // Report opens as a receive
      tree.put ("type", "receive");
      if (block_a.hashables.source != rai::genesis_account)
      {
         tree.put ("account", node.ledger.account (transaction, block_a.hashables.source).to_account ());
         tree.put ("amount", node.ledger.amount (transaction, hash).convert_to<string> ());
      }
      else
      {
         tree.put ("account", rai::genesis_account.to_account ());
         tree.put ("amount", rai::genesis_amount.convert_to<string> ());
      }
   }
   void change_block (rai::change_block const &)
   {
      // Don't report change blocks
   }
   rai::transaction & transaction;
   boost::property_tree::ptree & tree;
   rai::block_hash const & hash;
   rai::node& node;
};

string pyrai::wallet_create ()
{
   rai::keypair wallet_id;
   auto wallet (node.wallets.create (wallet_id.pub));
   return wallet_id.pub.to_string ();
}


PyObject* pyrai::block_count ()
{
   PyDict dict;
   rai::transaction transaction (node.store.environment, nullptr, false);
   dict.add ("count", to_string (node.store.block_count (transaction).sum ()));
   dict.add ("unchecked", to_string (node.store.unchecked_count (transaction)));
   return dict.get();
}

int pyrai::account_balance (string& account_text, string& _balance, string& _pending)
{
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      auto balance (node.balance_pending (account));
      _balance = balance.first.convert_to<string> ();
      _pending = balance.second.convert_to<string> ();
      return 1;
   }
   else
   {
      return 0;
   }
}
int pyrai::account_block_count (string& account_text)
{
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      rai::transaction transaction (node.store.environment, nullptr, false);
      rai::account_info info;
      if (!node.store.account_get (transaction, account, info))
      {
         return info.block_count;
      }
   }
   return 0;
}

PyObject* pyrai::account_info (string& account_text, bool representative, bool weight, bool pending)
{
   PyDict dict;
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (error)
   {
      return dict.get();
   }

   rai::transaction transaction (node.store.environment, nullptr, false);
   rai::account_info info;
   if (!node.store.account_get (transaction, account, info))
   {
      dict.add ("frontier", info.head.to_string ());
      dict.add ("open_block", info.open_block.to_string ());
      dict.add ("representative_block", info.rep_block.to_string ());
      string balance;
      rai::uint128_union (info.balance).encode_dec (balance);
      dict.add ("balance", balance);
      dict.add ("modified_timestamp", to_string (info.modified));
      dict.add ("block_count", to_string (info.block_count));
      if (representative)
      {
         auto block (node.store.block_get (transaction, info.rep_block));
         assert (block != nullptr);
         dict.add ("representative", block->representative ().to_account ());
      }
      if (weight)
      {
         auto account_weight (node.ledger.weight (transaction, account));
         dict.add ("weight", account_weight.convert_to<string> ());
      }
      if (pending)
      {
         auto account_pending (node.ledger.account_pending (transaction, account));
         dict.add ("pending", account_pending.convert_to<string> ());
      }
   }
   return dict.get();
}

PyObject* pyrai::account_create (string& wallet_text, bool generate_work)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::account new_key (existing->second->deterministic_insert (generate_work));
         if (!new_key.is_zero ())
         {
            string _account = new_key.to_account ();
            return py_new_string(_account);
         }
         else
         {
            return set_last_error ("Wallet is locked");
         }
      }
      else
      {
         return set_last_error ("Wallet not found");
      }
   }
   else
   {
      return set_last_error ("Bad wallet number");
   }
   return py_new_none();
}

PyObject* pyrai::account_get (string& key_text)
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
      return set_last_error ("Bad public key");
   }
}

PyObject* pyrai::account_history (string& account_text, int count)
{
   PyArray arr;
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      boost::property_tree::ptree response_l;
      boost::property_tree::ptree history;
      rai::transaction transaction (node.store.environment, nullptr, false);
      auto hash (node.ledger.latest (transaction, account));
      auto block (node.store.block_get (transaction, hash));
      while (block != nullptr && count > 0)
      {
         PyDict dict;
         boost::property_tree::ptree entry;
         history_visitor visitor (&node, transaction, entry, hash);
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
         block = node.store.block_get (transaction, hash);
         --count;
      }
      return arr.get();

   }
   else
   {
      return set_last_error ("Bad account number");
   }
}

PyObject* pyrai::account_list (string wallet_text)
{
   PyArray arr;
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::transaction transaction (node.store.environment, nullptr, false);
         for (auto i (existing->second->store.begin (transaction)), j (existing->second->store.end ()); i != j; ++i)
         {
            string _account = rai::uint256_union (i->first.uint256 ()).to_account ();
            arr.append(_account);
         }
         return arr.get();
      }
      else
      {
         return set_last_error("Wallet not found");
      }
   }
   else
   {
      return set_last_error("Bad wallet number");
   }
}

PyObject* pyrai::account_move (string wallet_text, string source_text, vector<string> accounts_text)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         auto wallet (existing->second);
         rai::uint256_union source;
         auto error (source.decode_hex (source_text));
         if (!error)
         {
            auto existing (node.wallets.items.find (source));
            if (existing != node.wallets.items.end ())
            {
               auto source (existing->second);
               vector<rai::public_key> accounts;
               for (auto i (accounts_text.begin ()), n (accounts_text.end ()); i != n; ++i)
               {
                  rai::public_key account;
//                  printf("%s\n",(*i).c_str());
                  account.decode_hex (*i);
                  accounts.push_back (account);
               }
               rai::transaction transaction (node.store.environment, nullptr, true);
               auto error (wallet->store.move (transaction, source->store, accounts));
               return py_new_int(error ? 0 : 1);
            }
            else
            {
               return set_last_error("Source not found");
            }
         }
         else
         {
            return set_last_error("Bad source number");
         }
      }
      else
      {
         return set_last_error("Wallet not found");
      }
   }
   else
   {
      return set_last_error("Bad wallet number");
   }

}

PyObject* pyrai::account_key (string account_text)
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
      return set_last_error("Bad account number");
   }
}

PyObject* pyrai::account_remove (string wallet_text, string account_text)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         auto wallet (existing->second);
         rai::transaction transaction (node.store.environment, nullptr, true);
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
                  return set_last_error ("Account not found in wallet");
               }
            }
            else
            {
               return set_last_error ("Bad account number");
            }
         }
         else
         {
            return set_last_error ("Wallet locked");
         }
      }
      else
      {
         return set_last_error ("Wallet not found");
      }
   }
   else
   {
      return set_last_error ("Bad wallet number");
   }

}

PyObject* pyrai::account_representative (string account_text)
{
   rai::account account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      rai::transaction transaction (node.store.environment, nullptr, false);
      rai::account_info info;
      auto error (node.store.account_get (transaction, account, info));
      if (!error)
      {
         auto block (node.store.block_get (transaction, info.rep_block));
         assert (block != nullptr);
         auto _account = block->representative ().to_account ();
         return py_new_string (_account);
      }
      else
      {
         return set_last_error ("Account not found");
      }
   }
   else
   {
      return set_last_error ("Bad account number");
   }
}

PyObject* pyrai::account_representative_set (string wallet_text, string account_text, string representative_text, uint64_t work)
{
   rai::uint256_union _wallet;
   auto error (_wallet.decode_hex (wallet_text));
   if (error)
   {
      return py_new_none();
   }
   auto existing (node.wallets.items.find (_wallet));
   if (existing == node.wallets.items.end ())
   {
      return py_new_none();
   }
   auto wallet (existing->second);
   rai::account account;

   if (account.decode_account (account_text))
   {
      return set_last_error ("Bad account number");
   }

   rai::account representative;
   if (representative.decode_account (representative_text))
   {
      return py_new_none();
   }
   if (work)
   {
      rai::transaction transaction (node.store.environment, nullptr, true);
      rai::account_info info;
      if (!node.store.account_get (transaction, account, info))
      {
         if (!rai::work_validate (info.head, work))
         {
            existing->second->store.work_put (transaction, account, work);
         }
         else
         {
            return set_last_error ("Invalid work");
         }
      }
      else
      {
         return set_last_error ("Account not found");
      }
   }
   promise<string> result;
   wallet->change_async (account, representative, [&result](shared_ptr<rai::block> block) {
      rai::block_hash hash (0);
      if (block != nullptr)
      {
         hash = block->hash ();
      }
      result.set_value(hash.to_string ());
   },
   work == 0);
   string _result = result.get_future ().get ();
   return py_new_string(_result);
}

PyObject* pyrai::account_weight (string account_text)
{
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      auto balance (node.weight (account));
      string _balance = balance.convert_to<std::string> ();
      return py_new_string(_balance);
   }
   else
   {
      return set_last_error ("Bad account number");
   }
}


PyObject* pyrai::accounts_balances (vector<string> accounts)
{
   PyDict result;
   for (auto & account_text : accounts)
   {
      rai::uint256_union account;
      auto error (account.decode_account (account_text));
      if (!error)
      {
         PyDict dict;
         auto balance (node.balance_pending (account));
         dict.add ("balance", balance.first.convert_to<std::string> ());
         dict.add ("pending", balance.second.convert_to<std::string> ());
         result.add (account.to_account (), dict.get());
      }
      else
      {
         return set_last_error("Bad account number");
      }
   }
   return result.get();
}


PyObject* pyrai::accounts_create (string wallet_text, uint64_t count, bool generate_work)
{
   PyArray result;
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      if (count != 0)
      {
         auto existing (node.wallets.items.find (wallet));
         if (existing != node.wallets.items.end ())
         {
            for (auto i (0); result.size () < count; ++i)
            {
               rai::account new_key (existing->second->deterministic_insert (generate_work));
               if (!new_key.is_zero ())
               {
                  result.append(new_key.to_account ());
               }
            }
         }
         else
         {
            return set_last_error ("Wallet not found");
         }
      }
      else
      {
         return set_last_error ("Invalid count limit");
      }
   }
   else
   {
      return set_last_error ("Bad wallet number");
   }
   return result.get();
}


PyObject* pyrai::accounts_frontiers (vector<string> accounts)
{
   PyDict dict;
   rai::transaction transaction (node.store.environment, nullptr, false);
   for (auto & account_text : accounts)
   {
      rai::uint256_union account;
      auto error (account.decode_account (account_text));
      if (!error)
      {
         auto latest (node.ledger.latest (transaction, account));
         if (!latest.is_zero ())
         {
            dict.add(account.to_account (), latest.to_string ());
         }
      }
      else
      {
         return set_last_error ("Bad account number");
      }
   }
   return dict.get();
}


PyObject* pyrai::block_count_type ()
{
   rai::transaction transaction (node.store.environment, nullptr, false);
   rai::block_counts count (node.store.block_count (transaction));
   PyDict dict;
   dict.add ("send", std::to_string (count.send));
   dict.add ("receive", std::to_string (count.receive));
   dict.add ("open", std::to_string (count.open));
   dict.add ("change", std::to_string (count.change));
   return dict.get();
}




}
