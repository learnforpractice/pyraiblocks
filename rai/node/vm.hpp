#ifndef _VM_HPP__
#define _VM_HPP__

#ifdef _cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <string.h>
#include <map>

#include <rai/node/utility.hpp>
#include <rai/secure.hpp>

extern "C" {
#include "py/objlist.h"
#include "py/objstringio.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/obj.h"
#include "py/compile.h"
}

namespace rai
{
	class node;
	class mp_py_module
	{
	public:
		mp_py_module(mp_obj_t obj, std::vector<uint8_t>& src)
		{
			this->obj = obj;
			this->src = src;
		}
		mp_obj_t obj;
		std::vector<uint8_t> src;
	};

	class VM
	{
	public:
		VM(rai::node& _node);
		~VM();		void load(MDB_txn * transaction_a, rai::account const & account_a, std::string& module_name);
		void call(rai::account const & account_a, std::string& module_name, std::string func, std::string args1, std::string arg2);
		static rai::VM* get_instance();

	private:
		rai::node& node;
		MDB_txn * transaction;
		std::map<rai::account, std::map<std::string, mp_py_module*>> py_modules;
		static rai::VM* _instance;
	};
}

#ifdef _cplusplus
}
#endif


#endif

