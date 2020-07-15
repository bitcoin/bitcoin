import unittest
from .messages import CTransaction as CTransaction, CTxOut as CTxOut, hash256 as hash256, ser_string as ser_string, ser_uint256 as ser_uint256, sha256 as sha256, uint256_from_str as uint256_from_str
from typing import Any, Dict

MAX_SCRIPT_ELEMENT_SIZE: int
OPCODE_NAMES: Dict[CScriptOp, str]

def hash160(s: Any): ...
def bn2vch(v: Any): ...

class CScriptOp(int):
    @staticmethod
    def encode_op_pushdata(d: Any): ...
    @staticmethod
    def encode_op_n(n: Any): ...
    def decode_op_n(self): ...
    def is_small_int(self): ...
    def __new__(cls, n: Any): ...

OP_0: Any
OP_FALSE = OP_0
OP_PUSHDATA1: Any
OP_PUSHDATA2: Any
OP_PUSHDATA4: Any
OP_1NEGATE: Any
OP_RESERVED: Any
OP_1: Any
OP_TRUE = OP_1
OP_2: Any
OP_3: Any
OP_4: Any
OP_5: Any
OP_6: Any
OP_7: Any
OP_8: Any
OP_9: Any
OP_10: Any
OP_11: Any
OP_12: Any
OP_13: Any
OP_14: Any
OP_15: Any
OP_16: Any
OP_NOP: Any
OP_VER: Any
OP_IF: Any
OP_NOTIF: Any
OP_VERIF: Any
OP_VERNOTIF: Any
OP_ELSE: Any
OP_ENDIF: Any
OP_VERIFY: Any
OP_RETURN: Any
OP_TOALTSTACK: Any
OP_FROMALTSTACK: Any
OP_2DROP: Any
OP_2DUP: Any
OP_3DUP: Any
OP_2OVER: Any
OP_2ROT: Any
OP_2SWAP: Any
OP_IFDUP: Any
OP_DEPTH: Any
OP_DROP: Any
OP_DUP: Any
OP_NIP: Any
OP_OVER: Any
OP_PICK: Any
OP_ROLL: Any
OP_ROT: Any
OP_SWAP: Any
OP_TUCK: Any
OP_CAT: Any
OP_SUBSTR: Any
OP_LEFT: Any
OP_RIGHT: Any
OP_SIZE: Any
OP_INVERT: Any
OP_AND: Any
OP_OR: Any
OP_XOR: Any
OP_EQUAL: Any
OP_EQUALVERIFY: Any
OP_RESERVED1: Any
OP_RESERVED2: Any
OP_1ADD: Any
OP_1SUB: Any
OP_2MUL: Any
OP_2DIV: Any
OP_NEGATE: Any
OP_ABS: Any
OP_NOT: Any
OP_0NOTEQUAL: Any
OP_ADD: Any
OP_SUB: Any
OP_MUL: Any
OP_DIV: Any
OP_MOD: Any
OP_LSHIFT: Any
OP_RSHIFT: Any
OP_BOOLAND: Any
OP_BOOLOR: Any
OP_NUMEQUAL: Any
OP_NUMEQUALVERIFY: Any
OP_NUMNOTEQUAL: Any
OP_LESSTHAN: Any
OP_GREATERTHAN: Any
OP_LESSTHANOREQUAL: Any
OP_GREATERTHANOREQUAL: Any
OP_MIN: Any
OP_MAX: Any
OP_WITHIN: Any
OP_RIPEMD160: Any
OP_SHA1: Any
OP_SHA256: Any
OP_HASH160: Any
OP_HASH256: Any
OP_CODESEPARATOR: Any
OP_CHECKSIG: Any
OP_CHECKSIGVERIFY: Any
OP_CHECKMULTISIG: Any
OP_CHECKMULTISIGVERIFY: Any
OP_NOP1: Any
OP_CHECKLOCKTIMEVERIFY: Any
OP_CHECKSEQUENCEVERIFY: Any
OP_NOP4: Any
OP_NOP5: Any
OP_NOP6: Any
OP_NOP7: Any
OP_NOP8: Any
OP_NOP9: Any
OP_NOP10: Any
OP_SMALLINTEGER: Any
OP_PUBKEYS: Any
OP_PUBKEYHASH: Any
OP_PUBKEY: Any
OP_INVALIDOPCODE: Any

class CScriptInvalidError(Exception): ...

class CScriptTruncatedPushDataError(CScriptInvalidError):
    data: Any = ...
    def __init__(self, msg: Any, data: Any) -> None: ...

class CScriptNum:
    value: Any = ...
    def __init__(self, d: int = ...) -> None: ...
    @staticmethod
    def encode(obj: Any): ...
    @staticmethod
    def decode(vch: Any): ...

class CScript(bytes):
    def __add__(self, other: Any) -> None: ...
    def join(self, iterable: Any) -> None: ...
    def __new__(cls, value: bytes = ...): ...
    def raw_iter(self) -> None: ...
    def __iter__(self) -> Any: ...
    def GetSigOpCount(self, fAccurate: Any): ...

SIGHASH_ALL: int
SIGHASH_NONE: int
SIGHASH_SINGLE: int
SIGHASH_ANYONECANPAY: int

def FindAndDelete(script: Any, sig: Any): ...
def LegacySignatureHash(script: Any, txTo: Any, inIdx: Any, hashtype: Any): ...
def SegwitV0SignatureHash(script: Any, txTo: Any, inIdx: Any, hashtype: Any, amount: Any): ...

class TestFrameworkScript(unittest.TestCase):
    def test_bn2vch(self) -> None: ...
    def test_cscriptnum_encoding(self) -> None: ...
