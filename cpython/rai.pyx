# cython: c_string_type=str, c_string_encoding=ascii

from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp cimport bool
import json
import os

cdef extern from *:
    ctypedef unsigned long long uint64_t

cdef extern from "rai/node/rai_.hpp" namespace "rai":
    cppclass pyrai:
        int account_block_count (string& account_text)
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
   
        object search_pending (string wallet_text);
        object search_pending_all ();
        object pending_exists (string hash_text)

        object validate_account_number (string account_text);
        object successors (string block_text, uint64_t count);

        object version ()
        object peers ()
        object pending (string account_text, uint64_t count, string threshold_text, bool source)
        object unchecked (uint64_t count)
        object unchecked_clear ()
        object unchecked_get (string hash_text)
        object unchecked_keys (uint64_t count, string hash_text)

        object wallet_add (string wallet_text, string key_text, bool generate_work);

        object wallet_balance_total (string wallet_text)
        object wallet_balances (string wallet_text, string threshold_text)
        object wallet_change_seed (string seed_text, string wallet_text)
        object wallet_contains (string account_text, string wallet_text)
        string wallet_create ()
        object wallet_destroy (string wallet_text)

        object wallet_export (string wallet_text);
        object wallet_frontiers (string wallet_text);
        object wallet_pending (string wallet_text, uint64_t count, string threshold_text, bool source)
        object wallet_republish (string wallet_text, uint64_t count)
        object wallet_work_get (string wallet_text, uint64_t work)

        object password_change (string wallet_text, string password_text)
        object password_enter (string wallet_text, string password_text)
        object password_valid (string wallet_text, bool wallet_locked)
        object work_cancel (string hash_text)
        object work_generate (string hash_text)
        object work_get (string wallet_text, string account_text)
        object work_set (string wallet_text, string account_text, string work_text)
        object work_peer_add (string address_text, string port_text)
        object work_peers ()
        object work_peers_clear ()
        object work_validate (string hash_text, string work_text)

        object send (string wallet_text, string source_text, string destination_text, string amount_text, uint64_t work)
        object send_v2 (string wallet_text, string source_text, string destination_text, string action_text, uint64_t work)

        object genesis_account()
        object block_count_type ()

        object vm_init(const char* dl_path)
        object vm_exec(const char* code)

        int load_python(const char* dl_path, const char* python_code, int interactive_mode)

        object open_dl(const char* dl_path);

        const char* get_last_error_()
        void clear_last_error_()

    pyrai* get_pyrai()


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

def block_count ():
    return _rai.block_count()

def get_last_error():
    return _rai.get_last_error_()

def clear_last_error():
    _rai.clear_last_error_();

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
    ret = _rai.accounts_balances (accounts)
    if ret:
        return JsonStruct(ret)

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

def receive (string wallet_text, string account_text, string hash_text, uint64_t work = 0):
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

def search_pending (string wallet_text):
    return _rai.search_pending (wallet_text)

def search_pending_all ():
    return _rai.search_pending_all ()

def validate_account_number (string account_text):
    return _rai.validate_account_number (account_text)

def successors (string block_text, uint64_t count):
    return _rai.successors (block_text, count)

def version ():
    return _rai.version ()

def peers ():
    return _rai.peers ();

def pending (string account_text, uint64_t count, string threshold_text, bool source):
    return _rai.pending (account_text, count, threshold_text, source)

def pending_exists (string hash_text):
    return _rai.pending_exists (hash_text)

def unchecked (uint64_t count):
    return _rai.unchecked (count)

def unchecked_clear ():
    return _rai.unchecked_clear ()

def unchecked_get (string hash_text):
    return _rai.unchecked_get (hash_text)

def unchecked_keys (uint64_t count, string hash_text):
    return _rai.unchecked_keys (count, hash_text)

def wallet_add (string wallet_text, string key_text, generate_work=True):
    return _rai.wallet_add (wallet_text, key_text, generate_work)

def wallet_balance_total (string wallet_text):
    return _rai.wallet_balance_total (wallet_text)

def wallet_balances (string wallet_text, string threshold_text):
    return _rai.wallet_balances (wallet_text, threshold_text)

def wallet_change_seed (string seed_text, string wallet_text):
    return _rai.wallet_change_seed (seed_text, wallet_text)

def wallet_contains (string account_text, string wallet_text):
    return _rai.wallet_contains (account_text, wallet_text)

def wallet_create ():
    return _rai.wallet_create()

def wallet_destroy (string wallet_text):
    return _rai.wallet_destroy (wallet_text)

def wallet_export (string wallet_text):
    return _rai.wallet_export (wallet_text)

def wallet_frontiers (string wallet_text):
    return _rai.wallet_frontiers (wallet_text)

def wallet_pending (string wallet_text, uint64_t count, string threshold_text, bool source):
    return _rai.wallet_pending (wallet_text, count, threshold_text, source)

def wallet_republish (string wallet_text, uint64_t count):
    return _rai.wallet_republish (wallet_text, count)

def wallet_pending (string wallet_text, uint64_t count, string threshold_text, bool source):
    return _rai.wallet_pending (wallet_text, count, threshold_text, source)

def wallet_work_get (string wallet_text, uint64_t work):
    return _rai.wallet_work_get (wallet_text, work)

def password_change (string wallet_text, string password_text):
    return _rai.password_change (wallet_text, password_text)

def password_enter (string wallet_text, string password_text):
    return _rai.password_enter (wallet_text, password_text)

def password_valid (string wallet_text, wallet_locked = False):
    return _rai.password_valid (wallet_text, wallet_locked);

def wallet_locked(string wallet_text):
    return password_valid(wallet_text, True)

def work_cancel (string hash_text):
    return _rai.work_cancel (hash_text)

def work_generate (string hash_text):
    return _rai.work_generate (hash_text)

def work_get (string wallet_text, string account_text):
    return _rai.work_get (wallet_text, account_text)

def work_set (string wallet_text, string account_text, string work_text):
    return _rai.work_set (wallet_text, account_text, work_text);

def work_peer_add (string address_text, string port_text):
    return _rai.work_peer_add (address_text, port_text)

def work_peer_add (string address_text, string port_text):
    return _rai.work_peer_add (address_text, port_text)

def work_peers ():
    return _rai.work_peers ()

def work_peers_clear ():
    return _rai.work_peers_clear ()

def work_validate (string hash_text, string work_text):
    return _rai.work_validate (hash_text, work_text)

def send (string wallet_text, string source_text, string destination_text, amount_text, uint64_t work = 0):
    amount_text = str(amount_text)
    return _rai.send (wallet_text, source_text, destination_text, amount_text, work)

def send_v2 (string wallet_text, string source_text, string destination_text, string action_text, uint64_t work = 0):
    return _rai.send_v2 (wallet_text, source_text, destination_text, action_text, work)

def genesis_account():
    return _rai.genesis_account()

def block_count_type ():
    ret = _rai.block_count_type ()
    return JsonStruct(ret)

dylib_index = 0
def start_py(const char* python_code):
    global dylib_index
    
    print('+++++python_code:', python_code)
    origin_dylib_path = "cpython/libcpython.dylib"
    dylib_index += 1
    dl_path = "cpython/libcpython{0}.dylib".format(dylib_index)
    if not os.path.exists(dl_path):
        with open(origin_dylib_path, 'rb') as f1:
            with open(dl_path, 'wb') as f2:
                f2.write(f1.read())
                f2.write(b'abc')
    _rai.load_python(dl_path, python_code, 0)

def vm_init(const char* dl_path):
    return _rai.vm_init(dl_path)

def vm_exec(const char* code):
    return _rai.vm_exec(code)

def dlopen(const char* dl_path):
    return _rai.open_dl(dl_path)

