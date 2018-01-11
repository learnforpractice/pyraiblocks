from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp cimport bool

import json

cdef extern from *:
    ctypedef unsigned long long uint64_t

cdef extern from "rai_.hpp" namespace "python":
    cppclass pyrai:
        int account_block_count (string& account_text)
        string wallet_create ()
        int account_balance (string& account_text, string& balance, string& pending)
        object account_info (string& account_text, bool representative, bool weight, bool pending)
        object block_count ()
        object account_create (string& wallet_text, bool generate_work)
        object account_get (string& key_text)
        object account_history (string& account_text, int count)
        object account_list (string wallet_text)
        object account_move (string wallet_text, string source_text, vector[string] accounts_text)
        object account_key (string account_text);
        object account_remove (string wallet_text, string account_text)
        object account_representative (string account_text)
        object account_representative_set (string wallet_text, string account_text, string representative_text, uint64_t work)
        object account_weight (string account_text)
        object accounts_balances (vector[string] accounts)
        object accounts_create (string wallet_text, uint64_t count, bool generate_work)
        object accounts_frontiers (vector[string] accounts)

        object block_count_type ()

    pyrai* get_pyrai()
    const char* get_last_error_()
    
cdef pyrai* _rai = get_pyrai()

class JsonStruct(object):
    def __init__(self, js):
        if isinstance(js, bytes):
            js = js.decode('utf8')
            js = json.loads(js)
            if isinstance(js, str):
                js = json.loads(js)
        for key in js:
            value = js[key]
            if isinstance(value, dict):
                self.__dict__[key] = JsonStruct(value)
            elif isinstance(value, list):
                for i in range(len(value)):
                    v = value[i]
                    if isinstance(v, dict):
                        value[i] = JsonStruct(v)
                self.__dict__[key] = value
            else:
                self.__dict__[key] = value
    def __str__(self):
        return str(self.__dict__)
#        return json.dumps(self, default=lambda x: x.__dict__,sort_keys=False,indent=4, separators=(',', ': '))
    def __repr__(self):
        return json.dumps(self, default=lambda x: x.__dict__, sort_keys=False, indent=4, separators=(',', ': '))

def wallet_create ():
    return _rai.wallet_create()

def block_count ():
    return _rai.block_count()

def get_last_error():
    return get_last_error_()


def account_balance (string account_text):
    cdef string balance
    cdef string pending

    if _rai.account_balance (account_text, balance, pending):
        return (int(balance), int(pending))
    return None

def account_block_count (account):
    return _rai.account_block_count (account)

def account_info (account_text, representative=True, weight=True, pending=True):
    info = _rai.account_info (account_text, representative, weight, pending)
    return JsonStruct(info)

def account_create (wallet_text, generate_work):
    return _rai.account_create (wallet_text, generate_work)

def account_get (string key_text):
    return _rai.account_get (key_text)

def account_history (account_text, count):
    ret = _rai.account_history (account_text, count)
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_list (string wallet_text):
    ret = _rai.account_list (wallet_text)
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_move (string wallet_text, string source_text, vector[string] accounts_text):
    ret = _rai.account_move (wallet_text, source_text, accounts_text);
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_key (string account_text):
    ret = _rai.account_key (account_text);
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_remove (string wallet_text, string account_text):
    ret = _rai.account_remove (wallet_text, account_text)
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_representative (string account_text):
    return _rai.account_representative (account_text)

def account_representative_set (string wallet_text, string account_text, string representative_text, uint64_t work):
    return _rai.account_representative_set (wallet_text, account_text, representative_text, work)

def account_weight (string account_text):
    return  _rai.account_weight (account_text)

def accounts_balances (vector[string] accounts):
    return _rai.accounts_balances (accounts)

def accounts_create (string wallet_text, int count, generate_work = True):
    return _rai.accounts_create (wallet_text, count, generate_work)

def accounts_frontiers (vector[string] accounts):
    return _rai.accounts_frontiers (accounts)

def block_count_type ():
    return _rai.block_count_type ()


