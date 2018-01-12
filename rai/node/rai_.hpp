/*
 * rai_.hpp
 *
 *  Created on: Jan 9, 2018
 *      Author: newworld
 */
#pragma once
#include <string>
#include <vector>

#include <rai/node/rpc.hpp>
#include <boost/algorithm/string.hpp>
#include <rai/lib/interface.h>
#include <rai/node/node.hpp>
#include <ed25519-donna/ed25519.h>

#include <Python.h>

using namespace std;
using namespace rai;

namespace python
{

class pyrai
{
public:
   pyrai(rai::node *_node):node(*_node)
   {
   }
   PyObject* account_balance (string& account_text);
   int account_block_count (string& account_text);
   PyObject* account_info (string& account_text, bool representative, bool weight, bool pending);
   PyObject* account_create (string& wallet_text, bool generate_work);
   PyObject* account_get (string& key_text);
   PyObject* account_history (string& account_text, int count);
   PyObject* account_list (string wallet_text);
   PyObject* account_move (string wallet_text, string source_text, vector<string> accounts_text);
   PyObject* account_key (string account_text);
   PyObject* account_remove (string wallet_text, string account_text);
   PyObject* account_representative (string account_text);
   PyObject* account_representative_set (string wallet_text);
   PyObject* account_representative_set (string wallet_text, string account_text, string representative_text, uint64_t work);
   PyObject* account_weight (string account_text);
   PyObject* accounts_balances (vector<string> accounts);
   PyObject* accounts_create (string wallet_text, uint64_t count, bool generate_work);
   PyObject* accounts_frontiers (vector<string> accounts);
   PyObject* accounts_pending (vector<string> accounts, uint64_t count, string threshold_text, bool source);
   PyObject* available_supply ();
   PyObject* block (string hash_text);
   PyObject* blocks (vector<string> hashes);
   PyObject* blocks_info (vector<string> hashes, bool pending, bool source);
   PyObject* block_account (string hash_text);
   PyObject* bootstrap (string address_text, string port_text);
   PyObject* bootstrap_any ();
   PyObject* chain (string block_text, uint64_t count);
   PyObject* delegators (string account_text);
   PyObject* delegators_count (string account_text);
   PyObject* deterministic_key (string seed_text, uint64_t index_a);
   PyObject* frontiers (string account_text, uint64_t count);
   PyObject* frontier_count ();

   PyObject* genesis_account();
   PyObject* send (string wallet_text, string source_text, string destination_text, string amount_text, uint64_t work);

   PyObject* block_count_type ();


   string wallet_create ();
   PyObject* block_count ();
private:
   rai::node& node;
};

pyrai* get_pyrai();
const char* get_last_error_();


}
