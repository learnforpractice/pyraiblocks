from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp cimport bool

import json

cdef extern from "rai_.hpp" namespace "python":
    int account_block_count_(string& account_text)
    string wallet_create_ ()
    int account_balance_ (string& account_text, string& balance, string& pending)
    object account_info_ (string& account_text, bool representative, bool weight, bool pending)
    object block_count_ ()
    object account_create_ (string& wallet_text, bool generate_work)
    object account_get_ (string& key_text)
    object account_history_ (string& account_text, int count)
    object account_list_ (string wallet_text)
    object account_move_ (string wallet_text, string source_text, vector[string] accounts_text);
    object account_key_ (string account_text);
    object account_remove_ (string wallet_text, string account_text)


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


def account_balance (_account_text):
    cdef string balance
    cdef string pending
    cdef string account_text
    
    account_text = _account_text
    if account_balance_ (account_text, balance, pending):
        return (int(balance), int(pending))
    return None

def account_block_count (account):
    return account_block_count_(account)

def account_info (account_text, representative=True, weight=True, pending=True):
    info = account_info_(account_text, representative, weight, pending)
    return JsonStruct(info)

def account_create (wallet_text, generate_work):
    ret = account_create_(wallet_text, generate_work)
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_get (string key_text):
    ret = account_get_(key_text)
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_history (account_text, count):
    ret = account_history_ (account_text, count)
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_list (string wallet_text):
    ret = account_list_(wallet_text)
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_move (string wallet_text, string source_text, vector[string] accounts_text):
    ret = account_move_ (wallet_text, source_text, accounts_text);
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_key (string account_text):
    ret = account_key_ (account_text);
    if isinstance(ret, Exception):
        raise ret
    return ret

def account_remove (string wallet_text, string account_text):
    ret = account_remove_ (wallet_text, account_text)
    if isinstance(ret, Exception):
        raise ret
    return ret

def wallet_create ():
    return wallet_create_()

def block_count ():
    return block_count_()

def sayHello():
    print('hello, world')

