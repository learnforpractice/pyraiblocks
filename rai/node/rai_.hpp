/*
 * rai_.hpp
 *
 *  Created on: Jan 9, 2018
 *      Author: newworld
 */
#pragma once
#include <string>
#include <vector>
#include <Python.h>

using namespace std;


namespace rai
{

class node;

class pyrai
{
public:
   pyrai();
   virtual PyObject* account_balance (string& account_text);
   virtual int account_block_count (string& account_text);
   virtual PyObject* account_info (string& account_text, bool representative, bool weight, bool pending);
   virtual PyObject* account_create (string& wallet_text, bool generate_work);
   virtual PyObject* account_get (string& key_text);
   virtual PyObject* account_history (string& account_text, int count);
   virtual PyObject* account_list (string wallet_text);
   virtual PyObject* account_move (string wallet_text, string source_text, vector<string> accounts_text);
   virtual PyObject* account_key (string account_text);
   virtual PyObject* account_remove (string wallet_text, string account_text);
   virtual PyObject* account_representative (string account_text);
   virtual PyObject* account_representative_set (string wallet_text, string account_text, string representative_text, uint64_t work);
   virtual PyObject* account_weight (string account_text);
   virtual PyObject* accounts_balances (vector<string> accounts);
   virtual PyObject* accounts_create (string wallet_text, uint64_t count, bool generate_work);
   virtual PyObject* accounts_frontiers (vector<string> accounts);
   virtual PyObject* accounts_pending (vector<string> accounts, uint64_t count, string threshold_text, bool source);
   virtual PyObject* available_supply ();
   virtual PyObject* block (string hash_text);
   virtual PyObject* blocks (vector<string> hashes);
   virtual PyObject* blocks_info (vector<string> hashes, bool pending, bool source);
   virtual PyObject* block_account (string hash_text);
   virtual PyObject* bootstrap (string address_text, string port_text);
   virtual PyObject* bootstrap_any ();
   virtual PyObject* chain (string block_text, uint64_t count);
   virtual PyObject* delegators (string account_text);
   virtual PyObject* delegators_count (string account_text);
   virtual PyObject* deterministic_key (string seed_text, uint64_t index_a);
   virtual PyObject* frontiers (string account_text, uint64_t count);
   virtual PyObject* frontier_count ();
   virtual PyObject* history (string hash_text, uint64_t count);

   virtual PyObject* mrai_from_raw (string amount_text);
   virtual PyObject* mrai_to_raw (string amount_text);
   virtual PyObject* krai_from_raw (string amount_text);
   virtual PyObject* krai_to_raw (string amount_text);

   virtual PyObject* keepalive (string address_text, string port_text);
   virtual PyObject* key_create ();
   virtual PyObject* key_expand (string key_text);

   virtual PyObject* ledger (string account_text, uint64_t count, bool sorting, bool representative, bool weight, bool pending);

   virtual PyObject* block_create (PyObject *dict);
   virtual PyObject* payment_begin (string id_text);
   virtual PyObject* payment_init (string id_text);
   virtual PyObject* payment_end (string id_text, string account_text);
   virtual PyObject* payment_wait (string account_text, string amount_text, uint64_t timeout);

   virtual PyObject* process (string block_text);
   virtual PyObject* receive (string wallet_text, string account_text, string hash_text, uint64_t work);
   virtual PyObject* receive_minimum ();

   virtual PyObject* receive_minimum_set (string amount_text);
   virtual PyObject* representatives (uint64_t count, bool sorting);

   virtual PyObject* wallet_representative (string wallet_text);
   virtual PyObject* wallet_representative_set (string wallet_text, string representative_text);

   virtual PyObject* search_pending (string wallet_text);
   virtual PyObject* search_pending_all ();

   virtual PyObject* validate_account_number (string account_text);
   virtual PyObject* successors (string block_text, uint64_t count);
   virtual PyObject* version ();
   virtual PyObject* peers ();
   virtual PyObject* pending (string account_text, uint64_t count, string threshold_text, bool source);
   virtual PyObject* pending_exists (string hash_text);
   virtual PyObject* unchecked (uint64_t count);
   virtual PyObject* unchecked_clear ();
   virtual PyObject* unchecked_get (string hash_text);
   virtual PyObject* unchecked_keys (uint64_t count, string hash_text);

   virtual PyObject* wallet_add (string wallet_text, string key_text, bool generate_work);

   virtual PyObject* wallet_balance_total (string wallet_text);
   virtual PyObject* wallet_balances (string wallet_text, string threshold_text);
   virtual PyObject* wallet_change_seed (string seed_text, string wallet_text);
   virtual PyObject* wallet_contains (string account_text, string wallet_text);
   virtual PyObject* wallet_destroy (string wallet_text);

   virtual PyObject* wallet_export (string wallet_text);
   virtual PyObject* wallet_frontiers (string wallet_text);
   virtual PyObject* wallet_pending (string wallet_text, uint64_t count, string threshold_text, bool source);
   virtual PyObject* wallet_republish (string wallet_text, uint64_t count);
   virtual PyObject* wallet_work_get (string wallet_text, uint64_t work);

   virtual PyObject* password_change (string wallet_text, string password_text);
   virtual PyObject* password_enter (string wallet_text, string password_text);
   virtual PyObject* password_valid (string wallet_text, bool wallet_locked = false);

   virtual PyObject* work_cancel (string hash_text);
   virtual PyObject* work_generate (string hash_text);
   virtual PyObject* work_get (string wallet_text, string account_text);
   virtual PyObject* work_set (string wallet_text, string account_text, string work_text);
   virtual PyObject* work_peer_add (string address_text, string port_text);
   virtual PyObject* work_peers ();
   virtual PyObject* work_peers_clear ();
   virtual PyObject* work_validate (string hash_text, string work_text);

   virtual PyObject* genesis_account();
   virtual PyObject* send (string wallet_text, string source_text, string destination_text, string amount_text, uint64_t work);
   virtual PyObject* send_v2 (string wallet_text, string source_text, string destination_text, string action_text, uint64_t work);

   virtual PyObject* block_count_type ();

   virtual PyObject* vm_init(const char* dl_path);
   virtual PyObject* vm_exec(const char* code);

   virtual string wallet_create ();
   virtual PyObject* block_count ();
   virtual const char* get_last_error_();
   virtual void clear_last_error_();

   virtual int load_python(const char* dl_path = NULL, const char* python_code = NULL, int interactive_mode = 0);

   virtual PyObject* open_dl(const char* dl_path);


private:
   rai::node& node;
   void* lib_handle;
};

pyrai* get_pyrai();

}

typedef void (* fn_PyInit_rai)();
typedef void (* fn_PyInit_pyobject)();


typedef void (* fn_Py_InitializeEx)(int);
typedef void (* fn_PyEval_InitThreads)(void);
typedef int (* fn_PyRun_SimpleStringFlags)(const char *, PyCompilerFlags *);
typedef int (* fn_PyRun_InteractiveLoopFlags)(
    FILE *fp,
    const char *filename,       /* decoded from the filesystem encoding */
    PyCompilerFlags *flags);


#define _PyRun_InteractiveLoop(f, p) _PyRun_InteractiveLoopFlags(f, p, NULL)
#define _PyRun_SimpleString(s) _PyRun_SimpleStringFlags(s, NULL)

typedef struct python_api* (* fn_get_python_api) ();
typedef void (* fn_register_rai_class)(rai::pyrai *rai);

