#include <dlfcn.h>

#include "vm.hpp"

extern "C" {
//	mp_obj_t micropy_load(const char *mod_name, const char *data, size_t len);
	mp_obj_t micropy_call_0(mp_obj_t module_obj, const char *func);
	mp_obj_t micropy_call_1(mp_obj_t module_obj, const char* func, const char* arg0);
	mp_obj_t micropy_call_2(mp_obj_t module_obj, const char *func, uint64_t code, uint64_t type);
}



rai::VM* rai::VM::_instance = 0;

rai::VM::VM(rai::block_store& _store) : store(_store), transaction(0)
{
	_instance = this;
};

rai::VM::~VM()
{
};

rai::VM* rai::VM::get_instance()
{
	return _instance;
}

typedef int (*fn_main_micropython)(int argc, char **argv);
typedef void* (*fn_execute_from_str)(const char *str);
typedef mp_obj_t (*fn_micropy_load)(const char *mod_name, const char *data, size_t len);


void rai::VM::load(MDB_txn * transaction_a, rai::account const & account_a, std::string& module_name)
{
//	rai::transaction transaction_a (store.environment, nullptr, false);
	rai::mdb_val value;
	std::vector<uint8_t> code;
	auto error (store.account_get_code (transaction_a, account_a, code));
	if (error)
	{
		return;
	}
	printf("code.data(): %s \n",std::string((char*)code.data(), code.size()).c_str());
	std::map<rai::account, std::map<std::string, mp_py_module*>>::iterator it = py_modules.find(account_a);
	if (it != py_modules.end())
	{
		std::map<std::string, mp_py_module*>::iterator it2 = it->second.find(module_name);
		if (it2 != it->second.end() && it2->second->src == code)
		{
			return;
		}
		else
		{
			//TODO: delete module
		}
	}

	void* lib_handle = dlopen("micropython/libmicropython.dylib", RTLD_LOCAL|RTLD_LAZY);
	fn_main_micropython main_micropython = (fn_main_micropython)dlsym(lib_handle, "main_micropython");
	fn_micropy_load micropy_load = (fn_micropy_load)dlsym(lib_handle, "micropy_load");
	main_micropython(0,0);


	mp_obj_t obj = micropy_load(module_name.c_str(), (const char*)code.data(), code.size());

	if (obj != 0)
	{
		if (it == py_modules.end())
		{
			std::map<std::string, mp_py_module*> module_map;
			mp_py_module *module = new mp_py_module(obj, code);
			module_map[module_name] = module;
			py_modules[account_a] = module_map;
		}
		else
		{
			std::map<std::string, mp_py_module*>::iterator it2 = it->second.find(module_name);
			if (it2 == it->second.end())
			{
				mp_py_module *module = new mp_py_module(obj, code);
				it->second[module_name] = module;
			}
			else
			{
				it->second[module_name]->src = code;
				it->second[module_name]->obj = obj;
			}
		}
	}
	else
	{
		//TODO: delete moudle in py_modules[module_name] first
//		py_modules[module_name] = 0;
	}
}

void rai::VM::call(rai::account const & account_a, std::string& module_name, std::string func, std::string args0, std::string arg1)
{
	nlr_buf_t nlr;
	std::map<rai::account, std::map<std::string, mp_py_module*>>::iterator it = py_modules.find(account_a);
	if (it == py_modules.end())
	{
		return;
	}

	if (nlr_push(&nlr) == 0) {
//		micropy_call_1(module_name, function_name, arg0, arg1);
		nlr_pop();
	} else {
		mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
		throw std::exception();
	}
}


