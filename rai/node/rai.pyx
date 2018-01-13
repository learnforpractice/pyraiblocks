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
        object account_balance (string& account_text)
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
        object send (string wallet_text, string source_text, string destination_text, string amount_text, uint64_t work)
        object accounts_pending (vector[string] accounts, uint64_t count, string threshold_text, bool source)
        object available_supply ();
        object block (string hash_text);
        object blocks (vector[string] hashes);
        object blocks_info (vector[string] hashes, bool pending, bool source)
        object block_account (string hash_text)
        object bootstrap (string address_text, string port_text)
        object bootstrap_any ()
        object chain (string block_text, uint64_t count)
        object delegators (string account_text)
        object delegators_count (string account_text)
        object deterministic_key (string seed_text, uint64_t index)
        object frontiers (string account_text, uint64_t count)
        object frontier_count ()
        object history (string hash_text, uint64_t count)
        object mrai_from_raw (string amount_text)
        object mrai_to_raw (string amount_text)
        object krai_from_raw (string amount_text)

        object keepalive (string address_text, string port_text)
        object key_create ()
        object key_expand (string key_text)

        object ledger (string account_text, uint64_t count, bool sorting, bool representative, bool weight, bool pending)

        object block_create (object dict)

        object payment_begin (string id_text)
        object payment_init (string id_text)
        object payment_end (string id_text, string account_text)
        object payment_wait (string account_text, string amount_text, uint64_t timeout)

        object process (string block_text);

        object receive (string wallet_text, string account_text, string hash_text, uint64_t work);
        object receive_minimum ();

        object receive_minimum_set (string amount_text)
        object representatives (uint64_t count, bool sorting)
        object wallet_representative (string wallet_text)
        object wallet_representative_set (string wallet_text, string representative_text)
        object wallet_republish (string wallet_text, uint64_t count)

        object search_pending (string wallet_text);

        object password_change (string wallet_text, string password_text)
        object password_enter (string wallet_text, string password_text)
        object password_valid (string wallet_text, bool wallet_locked)

        object genesis_account()
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

    ret = _rai.account_balance (account_text)
    return JsonStruct(ret)

def account_block_count (account):
    return _rai.account_block_count (account)

def account_info (account_text, representative=True, weight=True, pending=True):
    info = _rai.account_info (account_text, representative, weight, pending)
    return JsonStruct(info)

def account_create (wallet_text, generate_work=True):
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

def accounts_pending (vector[string] accounts, uint64_t count, string threshold_text, bool source):
    return _rai.accounts_pending (accounts, count, threshold_text, source)

def available_supply():
    return _rai.available_supply ()

def block (string hash_text):
    ret = _rai.block (hash_text);
    if ret:
        ret = json.loads(ret)
        return JsonStruct(ret)

def blocks (vector[string] hashes):
    return _rai.blocks (hashes);

def blocks_info (vector[string] hashes, pending = False, source = False):
    return _rai.blocks_info (hashes, pending, source)

def block_account (string hash_text):
    return _rai.block_account (hash_text)

def bootstrap (string address_text, string port_text):
    return _rai.bootstrap (address_text, port_text)

def bootstrap_any ():
    return _rai.bootstrap_any ()

def chain (string block_text, uint64_t count):
    return _rai.chain (block_text, count)

def delegators (string account_text):
    return _rai.delegators (account_text)

def delegators_count (string account_text):
    return _rai.delegators_count (account_text)

def deterministic_key (string seed_text, uint64_t index):
    return _rai.deterministic_key (seed_text, index)

def frontiers (string account_text, uint64_t count):
    return _rai.frontiers (account_text, count)

def frontier_count ():
    return _rai.frontier_count ()

def history (string hash_text, uint64_t count):
    ret = []
    result = _rai.history (hash_text, count)
    for r in result:
        ret.append(JsonStruct(r))
    return ret

def mrai_from_raw (string amount_text):
    return _rai.mrai_from_raw (amount_text)

def mrai_to_raw (string amount_text):
    return _rai.mrai_to_raw (amount_text)

def krai_from_raw (string amount_text):
    return _rai.krai_from_raw (amount_text)

def keepalive (string address_text, string port_text):
    return _rai.keepalive (address_text, port_text)

def key_create ():
    ret = _rai.key_create ()
    return JsonStruct(ret)

def key_expand (string key_text):
    ret = _rai.key_expand (key_text)
    if ret:
        return JsonStruct(ret)

def ledger (string account_text, uint64_t count, sorting = False, representative = False, weight = False, pending = False):
    return _rai.ledger (account_text, count, sorting, representative, weight, pending)

def block_create (d):
    ret = _rai.block_create (d)
    if ret:
        ret['block'] = JsonStruct(json.loads(ret['block']))
    return ret

def payment_begin (string id_text):
    return _rai.payment_begin(id_text)

def payment_init (string id_text):
    return _rai.payment_init (id_text)

def payment_end (string id_text, string account_text):
    return _rai.payment_end (id_text, account_text)

def payment_wait (string account_text, string amount_text, uint64_t timeout):
    return _rai.payment_wait (account_text, amount_text, timeout)

def process (string block_text):
    return _rai.process (block_text)

def receive (string wallet_text, string account_text, string hash_text, uint64_t work):
    return _rai.receive (wallet_text, account_text, hash_text, work)

def receive_minimum ():
    return _rai.receive_minimum ();

def receive_minimum_set (string amount_text):
    return _rai.receive_minimum_set (amount_text)

def representatives (uint64_t count, bool sorting):
    return _rai.representatives (count, sorting)

def wallet_representative (string wallet_text):
    return _rai.wallet_representative (wallet_text)

def wallet_representative_set (string wallet_text, string representative_text):
    return _rai.wallet_representative_set (wallet_text, representative_text)

def wallet_republish (string wallet_text, uint64_t count):
    return _rai.wallet_republish (wallet_text, count)

def search_pending (string wallet_text):
    return _rai.search_pending (wallet_text)

def password_change (string wallet_text, string password_text):
    return _rai.password_change (wallet_text, password_text)

def password_enter (string wallet_text, string password_text):
    return _rai.password_enter (wallet_text, password_text)

def password_valid (string wallet_text, wallet_locked = False):
    return _rai.password_valid (wallet_text, wallet_locked);

def send (string wallet_text, string source_text, string destination_text, string amount_text, uint64_t work):
    return _rai.send (wallet_text, source_text, destination_text, amount_text, work)

def genesis_account():
    return _rai.genesis_account()

def block_count_type ():
    return _rai.block_count_type ()


