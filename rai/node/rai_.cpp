#include "pyobject.hpp"
#include "rai_.hpp"


static rai::node *s_node;
void set_node(rai::node *_node)
{
   s_node = _node;
}

namespace python
{

static const char* last_error = "";
PyObject* set_last_error(const char *error)
{
   last_error = error;
   return py_new_none();
}

PyObject* set_last_error_return_false(const char *error)
{
   last_error = error;
   return py_new_bool(false);
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


class pyhistory_visitor : public rai::block_visitor
{
public:
   pyhistory_visitor (rai::node* _node, rai::transaction & transaction_a, PyDict& tree_a, rai::block_hash const & hash_a) :
   node(*_node),
   transaction (transaction_a),
   tree (tree_a),
   hash (hash_a)
   {
   }
   void send_block (rai::send_block const & block_a)
   {
      tree.add ("type", "send");
      auto account (block_a.hashables.destination.to_account ());
      tree.add ("account", account);
      auto amount (node.ledger.amount (transaction, hash).convert_to<string> ());
      tree.add ("amount", amount);
   }
   void receive_block (rai::receive_block const & block_a)
   {
      tree.add ("type", "receive");
      auto account (node.ledger.account (transaction, block_a.hashables.source).to_account ());
      tree.add ("account", account);
      auto amount (node.ledger.amount (transaction, hash).convert_to<string> ());
      tree.add ("amount", amount);
   }
   void open_block (rai::open_block const & block_a)
   {
      // Report opens as a receive
      tree.add ("type", "receive");
      if (block_a.hashables.source != rai::genesis_account)
      {
         tree.add ("account", node.ledger.account (transaction, block_a.hashables.source).to_account ());
         tree.add ("amount", node.ledger.amount (transaction, hash).convert_to<string> ());
      }
      else
      {
         tree.add ("account", rai::genesis_account.to_account ());
         tree.add ("amount", rai::genesis_amount.convert_to<string> ());
      }
   }
   void change_block (rai::change_block const &)
   {
      // Don't report change blocks
   }
   rai::transaction & transaction;
   PyDict& tree;
   rai::block_hash const & hash;
   rai::node& node;
};

string pyrai::wallet_create ()
{
   rai::keypair wallet_id;
   auto wallet (node.wallets.create (wallet_id.pub));
   return wallet_id.pub.to_string ();
}

PyObject* pyrai::account_balance (string& account_text)
{
   PyDict dict;
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      auto balance (node.balance_pending (account));
      string _balance = balance.first.convert_to<string> ();
      string _pending = balance.second.convert_to<string> ();
      dict.add("balance", _balance);
      dict.add("pending", _pending);
   }
   return dict.get();
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

#define error_response(a, b) return set_last_error(b);

#define error_response_false(a, b) return set_last_error_return_false(b);


PyObject* pyrai::accounts_pending (vector<string> accounts, uint64_t count, string threshold_text, bool source)
{
   PyDict result;
   rai::uint128_union threshold (0);

   auto error_threshold (threshold.decode_dec (threshold_text));
   if (error_threshold)
   {
      error_response (response, "Bad threshold number");
   }

   rai::transaction transaction (node.store.environment, nullptr, false);
   for (auto & account_text : accounts)
   {
      rai::uint256_union account;
      if (!account.decode_account (account_text))
      {
         PyDict dict;
         rai::account end (account.number () + 1);
         for (auto i (node.store.pending_begin (transaction, rai::pending_key (account, 0))), n (node.store.pending_begin (transaction, rai::pending_key (end, 0))); i != n && dict.size () < count; ++i)
         {
            rai::pending_key key (i->first);
            if (threshold.is_zero () && !source)
            {
               dict.add("hash", key.hash.to_string ());
            }
            else
            {
               rai::pending_info info (i->second);
               if (info.amount.number () >= threshold.number ())
               {
                  if (source)
                  {
                     PyDict subdict;
                     subdict.add ("amount", info.amount.number ().convert_to<std::string> ());
                     subdict.add ("source", info.source.to_account ());
                     dict.add (key.hash.to_string (), subdict.get());
                  }
                  else
                  {
                     dict.add (key.hash.to_string (), info.amount.number ().convert_to<std::string> ());
                  }
               }
            }
         }
         result.add (account.to_account (), dict.get());
      }
      else
      {
         error_response (response, "Bad account number");
      }
   }
   return result.get();
}

PyObject* pyrai::genesis_account()
{
   auto account = rai::genesis_account.to_account ();
   return py_new_string (account);
}

PyObject* pyrai::available_supply ()
{
   auto genesis_balance (node.balance (rai::genesis_account)); // Cold storage genesis
   auto landing_balance (node.balance (rai::account ("059F68AAB29DE0D3A27443625C7EA9CDDB6517A8B76FE37727EF6A4D76832AD5"))); // Active unavailable account
   auto faucet_balance (node.balance (rai::account ("8E319CE6F3025E5B2DF66DA7AB1467FE48F1679C13DD43BFDB29FA2E9FC40D3B"))); // Faucet account
   auto burned_balance ((node.balance_pending (rai::account (0))).second); // Burning 0 account
   auto available (rai::genesis_amount - genesis_balance - landing_balance - faucet_balance - burned_balance);
   auto _available = available.convert_to<std::string> ();
   return py_new_string(_available);
}

PyObject* pyrai::block (string hash_text)
{
   PyDict dict;
   rai::uint256_union hash;
   auto error (hash.decode_hex (hash_text));
   if (!error)
   {
      rai::transaction transaction (node.store.environment, nullptr, false);
      auto block (node.store.block_get (transaction, hash));
      if (block != nullptr)
      {
         std::string contents;
         block->serialize_json (contents);
         return py_new_string(contents);
      }
      else
      {
         error_response (response, "Block not found");
      }
   }
   else
   {
      error_response (response, "Bad hash number");
   }
}

PyObject* pyrai::blocks (vector<string> hashes)
{
   PyArray result;
   rai::transaction transaction (node.store.environment, nullptr, false);
   for (auto& hash_text : hashes)
   {
      rai::uint256_union hash;
      auto error (hash.decode_hex (hash_text));
      if (!error)
      {
         auto block (node.store.block_get (transaction, hash));
         if (block != nullptr)
         {
            PyDict dict;
            std::string contents;
            block->serialize_json (contents);
            dict.add (hash_text, contents);
            result.append (dict.get());
         }
         else
         {
            error_response (response, "Block not found");
         }
      }
      else
      {
         error_response (response, "Bad hash number");
      }
   }
   return result.get();
}

PyObject* pyrai::blocks_info (vector<string> hashes, bool pending, bool source)
{
   PyDict result;
   rai::transaction transaction (node.store.environment, nullptr, false);
   for (auto& hash_text : hashes)
   {
      rai::uint256_union hash;
      auto error (hash.decode_hex (hash_text));
      if (!error)
      {
         auto block (node.store.block_get (transaction, hash));
         if (block != nullptr)
         {
            PyDict dict;
            auto account (node.ledger.account (transaction, hash));
            dict.add ("block_account", account.to_account ());
            auto amount (node.ledger.amount (transaction, hash));
            dict.add ("amount", amount.convert_to<std::string> ());
            std::string contents;
            block->serialize_json (contents);
            dict.add ("contents", contents);
            if (pending)
            {
               auto block_l (dynamic_cast<rai::send_block *> (block.get ()));
               bool exists (false);
               if (block_l != nullptr)
               {
                  auto destination (block_l->hashables.destination);
                  exists = node.store.pending_exists (transaction, rai::pending_key (destination, hash));
               }
               dict.add ("pending", exists ? "1" : "0");
            }
            if (source)
            {
               rai::block_hash source_hash (block->source ());
               std::unique_ptr<rai::block> block_a (node.store.block_get (transaction, source_hash));
               if (block_a != nullptr)
               {
                  auto source_account (node.ledger.account (transaction, source_hash));
                  dict.add ("source_account", source_account.to_account ());
               }
               else
               {
                  dict.add ("source_account", "0");
               }
            }
            result.add (hash_text, dict.get());
         }
         else
         {
            error_response (response, "Block not found");
         }
      }
      else
      {
         error_response (response, "Bad hash number");
      }
   }
   return result.get();
}


PyObject* pyrai::block_account (string hash_text)
{
   rai::block_hash hash;
   if (!hash.decode_hex (hash_text))
   {
      rai::transaction transaction (node.store.environment, nullptr, false);
      if (node.store.block_exists (transaction, hash))
      {
         auto account (node.ledger.account (transaction, hash));
         return py_new_string(account.to_account ());
      }
      else
      {
         error_response (response, "Block not found");
      }
   }
   else
   {
      error_response (response, "Invalid block hash");
   }
}

PyObject* pyrai::block_count ()
{
   PyDict dict;
   rai::transaction transaction (node.store.environment, nullptr, false);
   dict.add ("count", to_string (node.store.block_count (transaction).sum ()));
   dict.add ("unchecked", to_string (node.store.unchecked_count (transaction)));
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


PyObject* pyrai::bootstrap (string address_text, string port_text)
{
   boost::system::error_code ec;
   auto address (boost::asio::ip::address_v6::from_string (address_text, ec));
   if (!ec)
   {
      uint16_t port;
      if (!rai::parse_port (port_text, port))
      {
         node.bootstrap_initiator.bootstrap (rai::endpoint (address, port));
         return py_new_bool(true);
      }
      else
      {
         error_response (response, "Invalid port");
      }
   }
   else
   {
      error_response (response, "Invalid address");
   }
}


PyObject* pyrai::bootstrap_any ()
{
   node.bootstrap_initiator.bootstrap ();
   return py_new_bool(true);
}


PyObject* pyrai::chain (string block_text, uint64_t count)
{
   PyArray arr;
   rai::block_hash block;
   if (!block.decode_hex (block_text))
   {
      rai::transaction transaction (node.store.environment, nullptr, false);
      while (!block.is_zero () && arr.size () < count)
      {
         auto block_l (node.store.block_get (transaction, block));
         if (block_l != nullptr)
         {
            arr.append(block.to_string ());
            block = block_l->previous ();
         }
         else
         {
            block.clear ();
         }
      }
      return arr.get();
   }
   else
   {
      error_response (response, "Invalid block hash");
   }
}

PyObject* pyrai::delegators (string account_text)
{
   rai::account account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      PyDict result;
      rai::transaction transaction (node.store.environment, nullptr, false);
      for (auto i (node.store.latest_begin (transaction)), n (node.store.latest_end ()); i != n; ++i)
      {
         rai::account_info info (i->second);
         auto block (node.store.block_get (transaction, info.rep_block));
         assert (block != nullptr);
         if (block->representative () == account)
         {
            std::string balance;
            rai::uint128_union (info.balance).encode_dec (balance);
            result.add (rai::account (i->first.uint256 ()).to_account (), balance);
         }
      }
      return result.get();
   }
   else
   {
      error_response (response, "Bad account number");
   }
}

PyObject* pyrai::delegators_count (string account_text)
{
   rai::account account;
   auto error (account.decode_account (account_text));
   if (!error)
   {
      uint64_t count (0);
      rai::transaction transaction (node.store.environment, nullptr, false);
      for (auto i (node.store.latest_begin (transaction)), n (node.store.latest_end ()); i != n; ++i)
      {
         rai::account_info info (i->second);
         auto block (node.store.block_get (transaction, info.rep_block));
         assert (block != nullptr);
         if (block->representative () == account)
         {
            ++count;
         }
      }
      return py_new_int(count);
   }
   else
   {
      set_last_error ("Bad account number");
      return py_new_int(0);
   }
}

PyObject* pyrai::deterministic_key (string seed_text, uint64_t index_a)
{
   rai::raw_key seed;
   auto error (seed.data.decode_hex (seed_text));
   if (!error)
   {
      PyDict dict;
      uint64_t index_a;
      rai::uint256_union index (index_a);
      rai::uint256_union prv;
      blake2b_state hash;
      blake2b_init (&hash, prv.bytes.size ());
      blake2b_update (&hash, seed.data.bytes.data (), seed.data.bytes.size ());
      blake2b_update (&hash, reinterpret_cast<uint8_t *> (&index.dwords[7]), sizeof (uint32_t));
      blake2b_final (&hash, prv.bytes.data (), prv.bytes.size ());
      boost::property_tree::ptree response_l;
      rai::uint256_union pub;
      ed25519_publickey (prv.bytes.data (), pub.bytes.data ());
      dict.add ("private", prv.to_string ());
      dict.add ("public", pub.to_string ());
      dict.add ("account", pub.to_account ());
      return dict.get();
   }
   else
   {
      error_response (response, "Bad seed");
   }
}

PyObject* pyrai::frontiers (string account_text, uint64_t count)
{
   rai::account start;
   if (!start.decode_account (account_text))
   {
      PyDict frontiers;
      rai::transaction transaction (node.store.environment, nullptr, false);
      for (auto i (node.store.latest_begin (transaction, start)), n (node.store.latest_end ()); i != n && frontiers.size () < count; ++i)
      {
         frontiers.add (rai::account (i->first.uint256 ()).to_account (), rai::account_info (i->second).head.to_string ());
      }
      return frontiers.get();
   }
   else
   {
      error_response (response, "Invalid starting account");
   }
}

PyObject* pyrai::frontier_count ()
{
   rai::transaction transaction (node.store.environment, nullptr, false);
   auto size (node.store.frontier_count (transaction));
   return py_new_int(size);
}

PyObject* pyrai::history (string hash_text, uint64_t count)
{
   rai::block_hash hash;
   if (!hash.decode_hex (hash_text))
   {
      PyArray result;
      rai::transaction transaction (node.store.environment, nullptr, false);
      auto block (node.store.block_get (transaction, hash));
      while (block != nullptr && count > 0)
      {
         PyDict entry;
         pyhistory_visitor visitor (&node, transaction, entry, hash);
         block->visit (visitor);
         if (0 != entry.size ())
         {
            entry.add ("hash", hash.to_string ());
         }
         result.append(entry.get());
         hash = block->previous ();
         block = node.store.block_get (transaction, hash);
         --count;
      }
      return result.get();
   }
   else
   {
      error_response (response, "Invalid block hash");
   }
}

PyObject* pyrai::mrai_from_raw (string amount_text)
{
   rai::uint128_union amount;
   if (!amount.decode_dec (amount_text))
   {
      auto result (amount.number () / rai::Mxrb_ratio);
      return py_new_string(result.convert_to<std::string> ());
   }
   else
   {
      set_last_error ("Bad amount number");
      return py_new_string("0");
   }
}

PyObject* pyrai::mrai_to_raw (string amount_text)
{
   rai::uint128_union amount;
   if (!amount.decode_dec (amount_text))
   {
      auto result (amount.number () * rai::Mxrb_ratio);
      if (result > amount.number ())
      {
         return py_new_string (result.convert_to<std::string> ());
      }
      else
      {
         error_response (response, "Amount too big");
      }
   }
   else
   {
      error_response (response, "Bad amount number");
   }
}

PyObject* pyrai::krai_from_raw (string amount_text)
{
   rai::uint128_union amount;
   if (!amount.decode_dec (amount_text))
   {
      auto result (amount.number () / rai::kxrb_ratio);
      return py_new_string(result.convert_to<std::string> ());
   }
   else
   {
      error_response (response, "Bad amount number");
   }
}

PyObject* pyrai::krai_to_raw (string amount_text)
{
   rai::uint128_union amount;
   if (!amount.decode_dec (amount_text))
   {
      auto result (amount.number () * rai::kxrb_ratio);
      if (result > amount.number ())
      {
         return py_new_string (result.convert_to<std::string> ());
      }
      else
      {
         error_response (response, "Amount too big");
      }
   }
   else
   {
      error_response (response, "Bad amount number");
   }
}

PyObject* pyrai::keepalive (string address_text, string port_text)
{
   uint16_t port;
   if (!rai::parse_port (port_text, port))
   {
      node.keepalive (address_text, port);
      return py_new_bool(true);
   }
   else
   {
      error_response_false (response, "Invalid port");
   }
}

PyObject* pyrai::key_create ()
{
   PyDict result;
   rai::keypair pair;
   result.add ("private", pair.prv.data.to_string ());
   result.add ("public", pair.pub.to_string ());
   result.add ("account", pair.pub.to_account ());
   return result.get();
}

PyObject* pyrai::key_expand (string key_text)
{
   rai::uint256_union prv;
   auto error (prv.decode_hex (key_text));
   if (!error)
   {
      PyDict result;
      rai::uint256_union pub;
      ed25519_publickey (prv.bytes.data (), pub.bytes.data ());
      result.add ("private", prv.to_string ());
      result.add ("public", pub.to_string ());
      result.add ("account", pub.to_account ());
      return result.get();
   }
   else
   {
      error_response (response, "Bad private key");
   }
}

PyObject* pyrai::ledger (string account_text, uint64_t count, bool sorting, bool representative, bool weight, bool pending)
{
   rai::account start (0);
//   uint64_t count (std::numeric_limits<uint64_t>::max ());
   auto error (start.decode_account (account_text));
   if (error)
   {
      error_response (response, "Invalid starting account");
   }
   PyDict result;
   rai::transaction transaction (node.store.environment, nullptr, false);
   if (!sorting) // Simple
   {
      for (auto i (node.store.latest_begin (transaction, start)), n (node.store.latest_end ()); i != n && result.size () < count; ++i)
      {
         PyDict dict;
         rai::account_info info (i->second);
         rai::account account (i->first.uint256 ());
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
            auto block (node.store.block_get (transaction, info.rep_block));
            assert (block != nullptr);
            dict.add ("representative", block->representative ().to_account ());
         }
         if (weight)
         {
            auto account_weight (node.ledger.weight (transaction, account));
            dict.add ("weight", account_weight.convert_to<std::string> ());
         }
         if (pending)
         {
            auto account_pending (node.ledger.account_pending (transaction, account));
            dict.add ("pending", account_pending.convert_to<std::string> ());
         }
         result.add (account.to_account (), dict.get());
      }
   }
   else // Sorting
   {
      std::vector<std::pair<rai::uint128_union, rai::account>> ledger_l;
      for (auto i (node.store.latest_begin (transaction, start)), n (node.store.latest_end ()); i != n; ++i)
      {
         rai::uint128_union balance (rai::account_info (i->second).balance);
         ledger_l.push_back (std::make_pair (balance, rai::account (i->first.uint256 ())));
      }
      std::sort (ledger_l.begin (), ledger_l.end ());
      std::reverse (ledger_l.begin (), ledger_l.end ());
      rai::account_info info;
      for (auto i (ledger_l.begin ()), n (ledger_l.end ()); i != n && result.size () < count; ++i)
      {
         PyDict dict;
         node.store.account_get (transaction, i->second, info);
         rai::account account (i->second);
         dict.add ("frontier", info.head.to_string ());
         dict.add ("open_block", info.open_block.to_string ());
         dict.add ("representative_block", info.rep_block.to_string ());
         std::string balance;
         (i->first).encode_dec (balance);
         dict.add ("balance", balance);
         dict.add ("modified_timestamp", std::to_string (info.modified));
         dict.add ("block_count", std::to_string (info.block_count));
         if (representative)
         {
            auto block (node.store.block_get (transaction, info.rep_block));
            assert (block != nullptr);
            dict.add ("representative", block->representative ().to_account ());
         }
         if (weight)
         {
            auto account_weight (node.ledger.weight (transaction, account));
            dict.add ("weight", account_weight.convert_to<std::string> ());
         }
         if (pending)
         {
            auto account_pending (node.ledger.account_pending (transaction, account));
            dict.add ("pending", account_pending.convert_to<std::string> ());
         }
         result.add (account.to_account (), dict.get());
      }
   }
   return result.get();
}


PyObject* pyrai::password_change (string wallet_text, string password_text)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::transaction transaction (node.store.environment, nullptr, true);
         auto error (existing->second->store.rekey (transaction, password_text));
         return py_new_bool(error ? 0 : 1);
      }
      else
      {
         error_response_false (response, "Wallet not found");
      }
   }
   else
   {
      error_response_false (response, "Bad wallet number");
   }
}

PyObject* pyrai::block_create (PyObject* dict)
{
   PyDict args = PyDict(dict);
/*
   string type, string wallet_text, string account_text, string representative_text,
            string destination_text, string source_text, string amount_text, uint64_t work, string key_text,
            string previous_text, string balance_text
*/
   std::string type;
   if (!args.get_value("type",type))
   {
      error_response (response, "type not defined!");
   }
   rai::uint256_union wallet (0);
   string value;
   if (args.get_value("wallet", value))
   {
      auto error (wallet.decode_hex (value));
      if (error)
      {
         error_response (response, "Bad wallet number");
      }
   }

   rai::uint256_union account (0);
   if (args.get_value("account", value))
   {
      auto error_account (account.decode_account (value));
      if (error_account)
      {
         error_response (response, "Bad account number");
      }
   }

   rai::uint256_union representative (0);
   if (args.get_value("representative", value))
   {
      auto error_representative (representative.decode_account (value));
      if (error_representative)
      {
         error_response (response, "Bad representative account");
      }
   }

   rai::uint256_union destination (0);
   if (args.get_value("destination", value)) {
      auto error_destination (destination.decode_account (value));
      if (error_destination)
      {
         error_response (response, "Bad destination account");
      }
   }

   rai::block_hash source (0);
   if (args.get_value("source", value))
   {
      auto error_source (source.decode_hex (value));
      if (error_source)
      {
         error_response (response, "Invalid source hash");
      }
   }

   rai::uint128_union amount (0);
   if (args.get_value("amount", value))
   {
      auto error_amount (amount.decode_dec (value));
      if (error_amount)
      {
         error_response (response, "Bad amount number");
      }
   }
   uint64_t work (0);
   if (args.get_value("work", value))
   {
      auto work_error (rai::from_string_hex (value, work));
      if (work_error)
      {
         error_response (response, "Bad work");
      }
   }
   rai::raw_key prv;
   prv.data.clear ();
   rai::uint256_union previous (0);
   rai::uint128_union balance (0);
   if (wallet != 0 && account != 0)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::transaction transaction (node.store.environment, nullptr, false);
         auto unlock_check (existing->second->store.valid_password (transaction));
         if (unlock_check)
         {
            auto account_check (existing->second->store.find (transaction, account));
            if (account_check != existing->second->store.end ())
            {
               existing->second->store.fetch (transaction, account, prv);
               previous = node.ledger.latest (transaction, account);
               balance = node.ledger.account_balance (transaction, account);
            }
            else
            {
               error_response (response, "Account not found in wallet");
            }
         }
         else
         {
            error_response (response, "Wallet is locked");
         }
      }
      else
      {
         error_response (response, "Wallet not found");
      }
   }
   if (args.get_value("key", value)) {
      auto error_key (prv.data.decode_hex (value));
      if (error_key)
      {
         error_response (response, "Bad private key");
      }
   }

   if (args.get_value("previous", value)) {
      auto error_previous (previous.decode_hex (value));
      if (error_previous)
      {
         error_response (response, "Invalid previous hash");
      }
   }

   if (args.get_value("balance", value)) {
      auto error_balance (balance.decode_dec (value));
      if (error_balance)
      {
         error_response (response, "Bad balance number");
      }
   }

   if (prv.data != 0)
   {
      rai::uint256_union pub;
      ed25519_publickey (prv.data.bytes.data (), pub.bytes.data ());
      if (type == "open")
      {
         if (representative != 0 && source != 0)
         {
            if (work == 0)
            {
               work = node.generate_work (pub);
            }
            PyDict dict;
            rai::open_block open (source, representative, pub, prv, pub, work);
            boost::property_tree::ptree response_l;
            dict.add ("hash", open.hash ().to_string ());
            std::string contents;
            open.serialize_json (contents);
            dict.add ("block", contents);
            return dict.get();
         }
         else
         {
            error_response (response, "Representative account and source hash required");
         }
      }
      else if (type == "receive")
      {
         if (source != 0 && previous != 0)
         {
            if (work == 0)
            {
               work = node.generate_work (previous);
            }
            PyDict dict;
            rai::receive_block receive (previous, source, prv, pub, work);
            boost::property_tree::ptree response_l;
            dict.add ("hash", receive.hash ().to_string ());
            std::string contents;
            receive.serialize_json (contents);
            dict.add ("block", contents);
            return dict.get();
         }
         else
         {
            error_response (response, "Previous hash and source hash required");
         }
      }
      else if (type == "change")
      {
         if (representative != 0 && previous != 0)
         {
            if (work == 0)
            {
               work = node.generate_work (previous);
            }
            PyDict dict;
            rai::change_block change (previous, representative, prv, pub, work);
            boost::property_tree::ptree response_l;
            dict.add ("hash", change.hash ().to_string ());
            std::string contents;
            change.serialize_json (contents);
            dict.add ("block", contents);
            return dict.get();
         }
         else
         {
            error_response (response, "Representative account and previous hash required");
         }
      }
      else if (type == "send")
      {
         if (destination != 0 && previous != 0 && balance != 0 && amount != 0)
         {
            if (balance.number () >= amount.number ())
            {
               if (work == 0)
               {
                  work = node.generate_work (previous);
               }
               PyDict dict;
               rai::send_block send (previous, destination, balance.number () - amount.number (), prv, pub, work);
               boost::property_tree::ptree response_l;
               dict.add ("hash", send.hash ().to_string ());
               std::string contents;
               send.serialize_json (contents);
               dict.add ("block", contents);
               return dict.get();
            }
            else
            {
               error_response (response, "Insufficient balance");
            }
         }
         else
         {
            error_response (response, "Destination account, previous hash, current balance and amount required");
         }
      }
      else
      {
         error_response (response, "Invalid block type");
      }
   }
   else
   {
      error_response (response, "Private key or local wallet and account required");
   }
}

PyObject* pyrai::payment_begin (string id_text)
{
   rai::uint256_union id;
   if (!id.decode_hex (id_text))
   {
      auto existing (node.wallets.items.find (id));
      if (existing != node.wallets.items.end ())
      {
         rai::transaction transaction (node.store.environment, nullptr, true);
         std::shared_ptr<rai::wallet> wallet (existing->second);
         if (wallet->store.valid_password (transaction))
         {
            rai::account account (0);
            do
            {
               auto existing (wallet->free_accounts.begin ());
               if (existing != wallet->free_accounts.end ())
               {
                  account = *existing;
                  wallet->free_accounts.erase (existing);
                  if (wallet->store.find (transaction, account) == wallet->store.end ())
                  {
                     BOOST_LOG (node.log) << boost::str (boost::format ("Transaction wallet %1% externally modified listing account %1% as free but no longer exists") % id.to_string () % account.to_account ());
                     account.clear ();
                  }
                  else
                  {
                     if (!node.ledger.account_balance (transaction, account).is_zero ())
                     {
                        BOOST_LOG (node.log) << boost::str (boost::format ("Skipping account %1% for use as a transaction account since it's balance isn't zero") % account.to_account ());
                        account.clear ();
                     }
                  }
               }
               else
               {
                  account = wallet->deterministic_insert (transaction);
                  break;
               }
            } while (account.is_zero ());
            if (!account.is_zero ())
            {
               return py_new_string (account.to_account ());
            }
            else
            {
               error_response (response, "Unable to create transaction account");
            }
         }
         else
         {
            error_response (response, "Wallet locked");
         }
      }
      else
      {
         error_response (response, "Unable to find wallets");
      }
   }
   else
   {
      error_response (response, "Bad wallet number");
   }
}


PyObject* pyrai::payment_init (string id_text)
{
   rai::uint256_union id;
   if (!id.decode_hex (id_text))
   {
      rai::transaction transaction (node.store.environment, nullptr, true);
      auto existing (node.wallets.items.find (id));
      if (existing != node.wallets.items.end ())
      {
         auto wallet (existing->second);
         if (wallet->store.valid_password (transaction))
         {
            wallet->init_free_accounts (transaction);
            return py_new_string("Ready");
         }
         else
         {
            return py_new_string("Transaction wallet locked");
         }
      }
      else
      {
         return py_new_string("Unable to find transaction wallet");
      }
   }
   else
   {
      return py_new_string("Bad transaction wallet number");
   }
}


PyObject* pyrai::payment_end (string id_text, string account_text)
{
   rai::uint256_union id;
   if (!id.decode_hex (id_text))
   {
      rai::transaction transaction (node.store.environment, nullptr, false);
      auto existing (node.wallets.items.find (id));
      if (existing != node.wallets.items.end ())
      {
         auto wallet (existing->second);
         rai::account account;
         if (!account.decode_account (account_text))
         {
            auto existing (wallet->store.find (transaction, account));
            if (existing != wallet->store.end ())
            {
               if (node.ledger.account_balance (transaction, account).is_zero ())
               {
                  wallet->free_accounts.insert (account);
                  return py_new_bool(true);
               }
               else
               {
                  error_response_false (response, "Account has non-zero balance");
               }
            }
            else
            {
               error_response_false (response, "Account not in wallet");
            }
         }
         else
         {
            error_response_false (response, "Invalid account number");
         }
      }
      else
      {
         error_response_false (response, "Unable to find wallet");
      }
   }
   else
   {
      error_response_false (response, "Bad wallet number");
   }
}

PyObject* pyrai::payment_wait (string account_text, string amount_text, uint64_t timeout)
{
   rai::uint256_union account;
   if (!account.decode_account (account_text))
   {
      rai::uint128_union amount;
      if (!amount.decode_dec (amount_text))
      {
#if 0
            {
               auto observer (std::make_shared<rai::payment_observer> (response, rpc, account, amount));
               observer->start (timeout);
               std::lock_guard<std::mutex> lock (rpc.mutex);
               assert (rpc.payment_observers.find (account) == rpc.payment_observers.end ());
               rpc.payment_observers[account] = observer;
            }
            rpc.observer_action (account);
#endif
            return py_new_bool(true);
      }
      else
      {
         error_response_false (response, "Bad amount number");
      }
   }
   else
   {
      error_response_false (response, "Bad account number");
   }
}

PyObject* pyrai::process (string block_text)
{
   boost::property_tree::ptree block_l;
   std::stringstream block_stream (block_text);
   boost::property_tree::read_json (block_stream, block_l);
   auto block (rai::deserialize_block_json (block_l));
   if (block != nullptr)
   {
      if (!rai::work_validate (*block))
      {
         auto hash (block->hash ());
         node.block_arrival.add (hash);
         rai::process_return result;
         std::shared_ptr<rai::block> block_a (std::move (block));
         {
            rai::transaction transaction (node.store.environment, nullptr, true);
            result = node.block_processor.process_receive_one (transaction, block_a);
         }
         switch (result.code)
         {
            case rai::process_result::progress:
            {
               node.observers.blocks (block_a, result.account, result.amount);
               return py_new_string(hash.to_string ());
               break;
            }
            case rai::process_result::gap_previous:
            {
               error_response (response, "Gap previous block");
               break;
            }
            case rai::process_result::gap_source:
            {
               error_response (response, "Gap source block");
               break;
            }
            case rai::process_result::old:
            {
               error_response (response, "Old block");
               break;
            }
            case rai::process_result::bad_signature:
            {
               error_response (response, "Bad signature");
               break;
            }
            case rai::process_result::overspend:
            {
               error_response (response, "Overspend");
               break;
            }
            case rai::process_result::unreceivable:
            {
               error_response (response, "Unreceivable");
               break;
            }
            case rai::process_result::not_receive_from_send:
            {
               error_response (response, "Not receive from send");
               break;
            }
            case rai::process_result::fork:
            {
               error_response (response, "Fork");
               break;
            }
            case rai::process_result::account_mismatch:
            {
               error_response (response, "Account mismatch");
               break;
            }
            default:
            {
               error_response (response, "Error processing block");
               break;
            }
         }
      }
      else
      {
         error_response (response, "Block work is invalid");
      }
   }
   else
   {
      error_response (response, "Block is invalid");
   }
}

PyObject* pyrai::receive (string wallet_text, string account_text, string hash_text, uint64_t work)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::account account;
         auto error (account.decode_account (account_text));
         if (!error)
         {
            rai::transaction transaction (node.store.environment, nullptr, false);
            auto account_check (existing->second->store.find (transaction, account));
            if (account_check != existing->second->store.end ())
            {
               rai::uint256_union hash;
               auto error (hash.decode_hex (hash_text));
               if (!error)
               {
                  auto block (node.store.block_get (transaction, hash));
                  if (block != nullptr)
                  {
                     if (node.store.pending_exists (transaction, rai::pending_key (account, hash)))
                     {
                        if (work)
                        {
                           rai::account_info info;
                           rai::uint256_union head;
                           if (!node.store.account_get (transaction, account, info))
                           {
                              head = info.head;
                           }
                           else
                           {
                              head = account;
                           }
                           if (!rai::work_validate (head, work))
                           {
                              rai::transaction transaction_a (node.store.environment, nullptr, true);
                              existing->second->store.work_put (transaction_a, account, work);
                           }
                           else
                           {
                              error_response (response, "Invalid work");
                           }
                        }
                        promise<string> result;
                        existing->second->receive_async (std::move (block), account, rai::genesis_amount, [&result](std::shared_ptr<rai::block> block_a) {
                           rai::uint256_union hash_a (0);
                           if (block_a != nullptr)
                           {
                              hash_a = block_a->hash ();
                           }
                           result.set_value (hash_a.to_string ());
                        },
                        work == 0);
                        string _result = result.get_future ().get ();
                        return py_new_string (_result);
                     }
                     else
                     {
                        error_response (response, "Block is not available to receive");
                     }
                  }
                  else
                  {
                     error_response (response, "Block not found");
                  }
               }
               else
               {
                  error_response (response, "Bad block number");
               }
            }
            else
            {
               error_response (response, "Account not found in wallet");
            }
         }
         else
         {
            error_response (response, "Bad account number");
         }
      }
      else
      {
         error_response (response, "Wallet not found");
      }
   }
   else
   {
      error_response (response, "Bad wallet number");
   }
   return py_new_none();
}

PyObject* pyrai::password_enter (string wallet_text, string password_text)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         auto error (existing->second->enter_password (password_text));
         return py_new_bool(error ? 0 : 1);
      }
      else
      {
         error_response_false (response, "Wallet not found");
      }
   }
   else
   {
      error_response_false (response, "Bad wallet number");
   }
}

PyObject* pyrai::receive_minimum ()
{
   return py_new_string (node.config.receive_minimum.to_string_dec ());
}

PyObject* pyrai::receive_minimum_set (string amount_text)
{
   rai::uint128_union amount;
   if (!amount.decode_dec (amount_text))
   {
      node.config.receive_minimum = amount;
      return py_new_bool (true);
   }
   else
   {
      error_response (response, "Bad amount number");
   }
}

PyObject* pyrai::representatives (uint64_t count, bool sorting)
{
   PyDict result;
   rai::transaction transaction (node.store.environment, nullptr, false);
   if (!sorting) // Simple
   {
      for (auto i (node.store.representation_begin (transaction)), n (node.store.representation_end ()); i != n && result.size () < count; ++i)
      {
         rai::account account (i->first.uint256 ());
         auto amount (node.store.representation_get (transaction, account));
         result.add (account.to_account (), amount.convert_to<std::string> ());
      }
   }
   else // Sorting
   {
      std::vector<std::pair<rai::uint128_union, std::string>> representation;
      for (auto i (node.store.representation_begin (transaction)), n (node.store.representation_end ()); i != n; ++i)
      {
         rai::account account (i->first.uint256 ());
         auto amount (node.store.representation_get (transaction, account));
         representation.push_back (std::make_pair (amount, account.to_account ()));
      }
      std::sort (representation.begin (), representation.end ());
      std::reverse (representation.begin (), representation.end ());
      for (auto i (representation.begin ()), n (representation.end ()); i != n && result.size () < count; ++i)
      {
         result.add (i->second, (i->first).number ().convert_to<std::string> ());
      }
   }
   return result.get();
}

PyObject* pyrai::wallet_representative (string wallet_text)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::transaction transaction (node.store.environment, nullptr, false);
         boost::property_tree::ptree response_l;
         return py_new_string (existing->second->store.representative (transaction).to_account ());
      }
      else
      {
         error_response (response, "Wallet not found");
      }
   }
   else
   {
      error_response (response, "Bad account number");
   }
}


PyObject* pyrai::wallet_representative_set (string wallet_text, string representative_text)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::account representative;
         auto error (representative.decode_account (representative_text));
         if (!error)
         {
            rai::transaction transaction (node.store.environment, nullptr, true);
            existing->second->store.representative_set (transaction, representative);
            return py_new_bool (true);
         }
         else
         {
            error_response_false (response, "Invalid account number");
         }
      }
      else
      {
         error_response_false (response, "Wallet not found");
      }
   }
   else
   {
      error_response_false (response, "Bad account number");
   }
}

PyObject* pyrai::wallet_republish (string wallet_text, uint64_t count)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      PyArray result;
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::transaction transaction (node.store.environment, nullptr, false);
         for (auto i (existing->second->store.begin (transaction)), n (existing->second->store.end ()); i != n; ++i)
         {
            rai::account account (i->first.uint256 ());
            auto latest (node.ledger.latest (transaction, account));
            std::unique_ptr<rai::block> block;
            std::vector<rai::block_hash> hashes;
            while (!latest.is_zero () && hashes.size () < count)
            {
               hashes.push_back (latest);
               block = node.store.block_get (transaction, latest);
               latest = block->previous ();
            }
            std::reverse (hashes.begin (), hashes.end ());
            for (auto & hash : hashes)
            {
               block = node.store.block_get (transaction, hash);
               node.network.republish_block (transaction, std::move (block));
               result.append (hash.to_string ());
            }
         }
         return result.get ();
      }
      else
      {
         error_response (response, "Wallet not found");
      }
   }
   else
   {
      error_response (response, "Bad wallet number");
   }

}


PyObject* pyrai::search_pending (string wallet_text)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         auto error (existing->second->search_pending ());
         boost::property_tree::ptree response_l;
         return py_new_bool (!error);
      }
      else
      {
         error_response_false (response, "Wallet not found");
      }
   }
   return py_new_bool (false);
}

PyObject* pyrai::search_pending_all ()
{
   node.wallets.search_pending_all ();
   return py_new_bool (true);
}

PyObject* pyrai::validate_account_number (string account_text)
{
   rai::uint256_union account;
   auto error (account.decode_account (account_text));
   return py_new_bool (error ? 0 : 1);
}

PyObject* pyrai::successors (string block_text, uint64_t count)
{
   PyArray blocks;
   rai::block_hash block;
   rai::transaction transaction (node.store.environment, nullptr, false);
   while (!block.is_zero () && blocks.size () < count)
   {
      auto block_l (node.store.block_get (transaction, block));
      if (block_l != nullptr)
      {
         boost::property_tree::ptree entry;
         entry.put ("", block.to_string ());
         blocks.append (block.to_string ());
         block = node.store.block_successor (transaction, block);
      }
      else
      {
         block.clear ();
      }
   }
   return blocks.get();
}

PyObject* pyrai::password_valid (string wallet_text, bool wallet_locked)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::transaction transaction (node.store.environment, nullptr, false);
         auto valid (existing->second->store.valid_password (transaction));
         if (!wallet_locked)
         {
            return py_new_bool (valid ? 1 : 0);
         }
         else
         {
            return py_new_bool (valid ? 0 : 1);
         }
      }
      else
      {
         error_response_false (response, "Wallet not found");
      }
   }
   else
   {
      error_response_false (response, "Bad wallet number");
   }
}


PyObject* pyrai::send (string wallet_text, string source_text, string destination_text, string amount_text, uint64_t work)
{
   rai::uint256_union wallet;
   auto error (wallet.decode_hex (wallet_text));
   if (!error)
   {
      auto existing (node.wallets.items.find (wallet));
      if (existing != node.wallets.items.end ())
      {
         rai::account source;
         auto error (source.decode_account (source_text));
         if (!error)
         {
            rai::account destination;
            auto error (destination.decode_account (destination_text));
            if (!error)
            {
               rai::amount amount;
               auto error (amount.decode_dec (amount_text));
               if (!error)
               {
                  rai::uint128_t balance (0);
                  {
                     rai::transaction transaction (node.store.environment, nullptr, work != 0); // false if no "work" in request, true if work > 0
                     rai::account_info info;
                     if (!node.store.account_get (transaction, source, info))
                     {
                        balance = (info.balance).number ();
                     }
                     else
                     {
                        return set_last_error ("Account not found");
                     }
                     if (work)
                     {
                        if (!rai::work_validate (info.head, work))
                        {
                           existing->second->store.work_put (transaction, source, work);
                        }
                        else
                        {
                           return set_last_error ("Invalid work");
                        }
                     }
                  }
                  if (balance >= amount.number ())
                  {
                     promise<string> result;
                     existing->second->send_async (source, destination, amount.number (), [&result](std::shared_ptr<rai::block> block_a) {
                        rai::uint256_union hash (0);
                        if (block_a != nullptr)
                        {
                           hash = block_a->hash ();
                        }
                        result.set_value(hash.to_string ());
                     },
                     work == 0);
                     string _result = result.get_future ().get ();
                     return py_new_string(_result);
                  }
                  else
                  {
                     return set_last_error ("Insufficient balance");
                  }
               }
               else
               {
                  return set_last_error ("Bad amount format");
               }
            }
            else
            {
               return set_last_error ("Bad destination account");
            }
         }
         else
         {
            return set_last_error ("Bad source account");
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







}
