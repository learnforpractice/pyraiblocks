#include <dlfcn.h>

#include <rai/node/node.hpp>

#include "vm.hpp"

extern "C" {
//	mp_obj_t micropy_load(const char *mod_name, const char *data, size_t len);
	mp_obj_t micropy_call_0(mp_obj_t module_obj, const char *func);
	mp_obj_t micropy_call_1(mp_obj_t module_obj, const char* func, const char* arg0);
	mp_obj_t micropy_call_2(mp_obj_t module_obj, const char *func, uint64_t code, uint64_t type);
}
typedef int (*fn_get_account_balance)(const char* account, uint64_t* _balance, uint64_t* _pending);

struct rai_functions
{
	fn_get_account_balance get_account_balance;
};

typedef int (*fn_main_micropython)(int argc, char **argv);
typedef void* (*fn_execute_from_str)(const char *str);
typedef mp_obj_t (*fn_micropy_load)(const char *mod_name, const char *data, size_t len);
typedef void (*fn_register_rai_functions)(struct rai_functions* _functions);


static rai::node *g_node;


int get_account_balance(const char* _account, uint64_t* _balance, uint64_t* _pending)
{
	rai::uint256_union account;
	std::string account_text(_account);
	auto error (account.decode_account (account_text));
	if (error)
	{
		return 0;
	}
	auto balance = g_node->balance_pending (account);
	*_balance = (balance.first/rai::Mxrb_ratio).convert_to<uint64_t> ();
	*_pending = (balance.second/rai::Mxrb_ratio).convert_to<uint64_t> ();
	return 1;
}

static rai_functions g_rai_functions = {
		get_account_balance,
};

//"micropython/libmicropython.dylib"
void * vm_init_(const char* dl_path)
{
	void* lib_handle = dlopen(dl_path, RTLD_LOCAL|RTLD_LAZY);
	printf("lib_handle: %llu\n",(uint64_t)lib_handle);

	fn_register_rai_functions register_rai_functions = (fn_register_rai_functions)dlsym(lib_handle, "register_rai_functions");
	register_rai_functions(&g_rai_functions);

	fn_main_micropython main_micropython = (fn_main_micropython)dlsym(lib_handle, "main_micropython");
	main_micropython(0,0);


	return lib_handle;
}

rai::VM* rai::VM::_instance = 0;

rai::VM::VM(rai::node& _node) : node(_node), transaction(0)
{
	_instance = this;
	g_node = &_node;
}

rai::VM::~VM()
{
}

rai::VM* rai::VM::get_instance()
{
	return _instance;
}

void rai::VM::load(MDB_txn * transaction_a, rai::account const & account_a, std::string& module_name)
{
//	rai::transaction transaction_a (store.environment, nullptr, false);
	rai::mdb_val value;
	std::vector<uint8_t> code;
	auto error (node.store.account_get_code (transaction_a, account_a, code));
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

	void *lib_handle = vm_init_("micropython/libmicropython.dylib");

	fn_micropy_load micropy_load = (fn_micropy_load)dlsym(lib_handle, "micropy_load");
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


