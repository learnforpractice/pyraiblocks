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

namespace python
{
   int account_block_count_(std::string& account_text);
   std::string wallet_create_ ();
   int account_balance_ (string& account_text, string& balance, string& pending);
   PyObject* account_info_ (std::string& account_text, bool representative, bool weight, bool pending);

   PyObject* block_count_ ();

   PyObject* account_create_ (string& wallet_text, bool generate_work);
   PyObject* account_get_ (string& key_text);
   PyObject* account_history_ (string& account_text, int count);
   PyObject* account_list_ (std::string wallet_text);
   PyObject* account_move_ (std::string wallet_text, std::string source_text, std::vector<std::string> accounts_text);
   PyObject* account_key_ (std::string account_text);
   PyObject* account_remove_ (std::string wallet_text, std::string account_text);

}
