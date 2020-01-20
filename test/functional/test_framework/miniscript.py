#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Classes and methods to encode and decode miniscripts"""

from enum import Enum

from .key import ECPubKey
from .script import CScript, OP_ADD, OP_CHECKLOCKTIMEVERIFY, \
    OP_CHECKSEQUENCEVERIFY, OP_CHECKMULTISIG, OP_CHECKSIG, \
    OP_CHECKMULTISIGVERIFY, OP_CHECKSIGVERIFY, OP_DUP, OP_EQUAL, \
    OP_EQUALVERIFY, OP_HASH160, OP_HASH256, OP_RIPEMD160, OP_SHA256, \
    OP_SIZE, OP_VERIFY


class Property:
    """Miniscript expression property"""
    # "B": Base type
    # "V": Verify type
    # "K": Key type
    # "W": Wrapped type
    # "z": Zero-arg property
    # "o": One-arg property
    # "n": Nonzero arg property
    # "d": Dissatisfiable property
    # "u": Unit property
    # "e": Expression property
    # "f": Forced property
    # "s": Safe property
    # "m": Nonmalleable property
    # "x": Expensive verify
    types = "BVKW"
    props = "zonduefsmx"

    def __init__(self):
        for literal in self.types+self.props:
            setattr(self, literal, False)

    def from_string(self, property_str):
        """Construct property from string of valid property and types"""
        for char in property_str:
            assert(hasattr(self, char))
            setattr(self, char, True)
        assert self.is_valid()
        return self

    def to_string(self):
        """Generate string representation of property"""
        property_str = ""
        for char in self.types+self.props:
            if getattr(self, char):
                property_str += char
        return property_str

    def is_valid(self):
        # Can only be of a single type.
        num_types = 0
        for typ in self.types:
            if getattr(self, typ):
                num_types += 1
        assert num_types == 1

        # Check for conflicts in type & properties.
        return \
            (not self.z or not self.o) and \
            (not self.n or not self.z) and \
            (not self.V or not self.d) and \
            (not self.K or self.u) and \
            (not self.V or not self.u) and \
            (not self.e or not self.f) and \
            (not self.e or self.d) and \
            (not self.V or not self.e) and \
            (not self.d or not self.f) and \
            (not self.V or self.f) and \
            (not self.K or self.s) and \
            (not self.z or self.m)


class NodeType(Enum):
    JUST_0 = 0
    JUST_1 = 1
    PK = 2
    PK_H = 3
    OLDER = 4
    AFTER = 5
    SHA256 = 6
    HASH256 = 7
    RIPEMD160 = 8
    HASH160 = 9
    WRAP_A = 10
    WRAP_S = 11
    WRAP_C = 12
    WRAP_T = 13
    WRAP_D = 14
    WRAP_V = 15
    WRAP_J = 16
    WRAP_N = 17
    WRAP_U = 18
    WRAP_L = 19
    AND_V = 20
    AND_B = 21
    AND_N = 22
    OR_B = 23
    OR_C = 24
    OR_D = 25
    OR_I = 26
    ANDOR = 27
    THRESH = 28
    THRESH_M = 29


class Node:
    """Miniscript expression class

    Provides methods to instantiate a miniscript node from a string descriptor
    or script. Node.sat, Node.dsat, Node.sat_ncan and Node.dsat_ncan return a
    list of tuples which encode the (dis)satisfying witness stack.
    """

    def __init__(self):
        self.desc = ''
        self.children = None
        self.t = None
        self._sat = None
        self._k = None
        self._pk = []
        self._pk_h = None

    @staticmethod
    def from_desc(string):
        """Construct miniscript node from string descriptor"""
        tag, child_exprs = Node._parse_string(string)
        k = None

        if tag == '0':
            return Node().construct_just_0()

        if tag == '1':
            return Node().construct_just_1()

        if tag == 'pk':
            key_b = bytes.fromhex(child_exprs[0])
            key_obj = ECPubKey()
            key_obj.set(key_b)
            return Node().construct_pk(key_obj)

        if tag == 'pk_h':
            keyhash_b = bytes.fromhex(child_exprs[0])
            return Node().construct_pk_h(keyhash_b)

        if tag == "older":
            n = int(child_exprs[0])
            return Node().construct_older(n)

        if tag == "after":
            time = int(child_exprs[0])
            return Node().construct_after(time)

        if tag in ["sha256", "hash256", "ripemd160", "hash160"]:
            hash_b = bytes.fromhex(child_exprs[0])
            return getattr(Node(), "construct_"+tag)(hash_b)

        if tag == 'thresh_m':
            k = int(child_exprs.pop(0))
            key_n = []
            for child_expr in child_exprs:
                key_obj = ECPubKey()
                key_obj.set(bytes.fromhex(child_expr))
                key_n.append(key_obj)
            return Node().construct_thresh_m(k, key_n)

        if tag == 'thresh':
            k = int(child_exprs.pop(0))
            return Node().construct_thresh(
                k, Node._parse_child_strings(child_exprs))

        # Standard node constructor forms:
        # construct_tag(node_x, node_y, ...):
        return getattr(Node(), "construct_"+tag)(
                *Node._parse_child_strings(child_exprs))

    @property
    def script(self):
        return CScript(Node._collapse_script(self._script))

    @staticmethod
    def from_script(c_script):
        """Construct miniscript node from script"""
        expr_list = []
        for op in c_script:
            # Encode 0, 20, 32 as int.
            # Other values are coerced to int types with Node._coerce_to_int()
            if op in [b'', b'\x14', b'\x20']:
                op_int = int.from_bytes(op, byteorder='big')
                expr_list.append(op_int)
            else:
                expr_list.append(op)

        # Decompose script:
        # OP_CHECKSIGVERIFY > OP_CHECKSIG + OP_VERIFY
        # OP_CHECKMULTISIGVERIFY > OP_CHECKMULTISIG + OP_VERIFY
        # OP_EQUALVERIFY > OP_EQUAL + OP_VERIFY
        expr_list = Node._decompose_script(expr_list)
        expr_list_len = len(expr_list)

        # Parse for terminal expressions.
        idx = 0
        while idx < expr_list_len:

            # Match against pk(key).
            if isinstance(expr_list[idx], bytes) and \
                    len(expr_list[idx]) == 33 and \
                    expr_list[idx][0] in [2, 3]:
                key = ECPubKey()
                key.set(expr_list[idx])
                expr_list[idx] = Node().construct_pk(key)

            # Match against just1.
            if expr_list[idx] == 1:
                expr_list[idx] = Node().construct_just_1()

            # Match against just0.
            if expr_list[idx] == 0:
                expr_list[idx] = Node().construct_just_0()

            # 2 element terminal expressions.
            if expr_list_len-idx >= 2:
                try:
                    # Timelock encoding n.
                    n_int = Node._coerce_to_int(expr_list[idx])
                    # Match against older.
                    if n_int >= 1 and \
                            expr_list[idx+1] == OP_CHECKSEQUENCEVERIFY:
                        node = Node().construct_older(n_int)
                        expr_list = expr_list[:idx]+[node]+expr_list[idx+2:]
                        expr_list_len -= 1
                    # Match against after.
                    elif n_int >= 1 and n_int <= 2**32 and \
                            expr_list[idx+1] == OP_CHECKLOCKTIMEVERIFY:
                        node = Node().construct_after(n_int)
                        expr_list = expr_list[:idx]+[node]+expr_list[idx+2:]
                        expr_list_len -= 1
                except Exception:
                    pass

            # 4 element terminal expressions.
            if expr_list_len-idx >= 5:

                # Match against pk_h(pkhash).
                if expr_list[idx:idx+2] == [OP_DUP, OP_HASH160] and \
                        isinstance(expr_list[idx+2], bytes) and \
                        len(expr_list[idx+2]) == 20 and \
                        expr_list[idx+3:idx+5] == [OP_EQUAL, OP_VERIFY]:
                    node = Node().construct_pk_h(expr_list[idx+2])
                    expr_list = expr_list[:idx]+[node]+expr_list[idx+5:]
                    # Reduce length of list after node construction.
                    expr_list_len -= 4

            # 6 element terminal expressions.
            if expr_list_len-idx >= 7:

                # Match against sha256.
                if expr_list[idx:idx+5] == [OP_SIZE, 32, OP_EQUAL,
                                            OP_VERIFY, OP_SHA256] and \
                        isinstance(expr_list[idx+5], bytes) and \
                        len(expr_list[idx+5]) == 32 and \
                        expr_list[idx+6] == OP_EQUAL:
                    node = Node().construct_sha256(expr_list[idx+5])
                    expr_list = expr_list[:idx]+[node]+expr_list[idx+7:]
                    expr_list_len -= 6

                # Match against hash256.
                if expr_list[idx:idx+5] == [OP_SIZE, 32, OP_EQUAL,
                                            OP_VERIFY, OP_HASH256] and \
                        isinstance(expr_list[idx+5], bytes) and \
                        len(expr_list[idx+5]) == 32 and \
                        expr_list[idx+6] == OP_EQUAL:
                    node = Node().construct_hash256(expr_list[idx+5])
                    expr_list = expr_list[:idx]+[node]+expr_list[idx+7:]
                    expr_list_len -= 6

                # Match against ripemd160.
                if expr_list[idx:idx+5] == [OP_SIZE, 32, OP_EQUAL,
                                            OP_VERIFY, OP_RIPEMD160] and \
                        isinstance(expr_list[idx+5], bytes) and \
                        len(expr_list[idx+5]) == 20 and \
                        expr_list[idx+6] == OP_EQUAL:
                    node = Node().construct_ripemd160(expr_list[idx+5])
                    expr_list = expr_list[:idx]+[node]+expr_list[idx+7:]
                    expr_list_len -= 6

                # Match against hash160.
                if expr_list[idx:idx+5] == [OP_SIZE, 32, OP_EQUAL,
                                            OP_VERIFY, OP_HASH160] and \
                        isinstance(expr_list[idx+5], bytes) and \
                        len(expr_list[idx+5]) == 20 and \
                        expr_list[idx+6] == OP_EQUAL:
                    node = Node().construct_hash160(expr_list[idx+5])
                    expr_list = expr_list[:idx]+[node]+expr_list[idx+7:]
                    expr_list_len -= 6

            # Increment index.
            idx += 1

        # Construct AST recursively.
        return Node._parse_expr_list(expr_list)

    @staticmethod
    def _parse_expr_list(expr_list):
        # Every recursive call must progress the AST construction,
        # until it is complete (single root node remains).
        expr_list_len = len(expr_list)

        # Root node reached.
        if expr_list_len == 1 and isinstance(expr_list[0], Node):
            return expr_list[0]

        # Step through each list index and match against templates.
        idx = expr_list_len - 1
        while idx >= 0:

            # Match against thresh_m.
            # Termainal expression, but k and n values can be Nodes (JUST_0/1)
            if expr_list_len-idx >= 5 and \
                    ((
                    isinstance(expr_list[idx], int) and
                    expr_list_len-idx-3 >= expr_list[idx] >= 1
                    ) or (
                    isinstance(expr_list[idx], Node) and
                    expr_list[idx].t == NodeType.JUST_1
                    )):
                k = Node._coerce_to_int(expr_list[idx])
                # Permissible values for n:
                # len(expr)-3 >= n >= 1
                for n in range(k, expr_list_len-2):
                    # Match ... <PK>*n ...
                    match, pk_m = True, []
                    for i in range(n):
                        if not (isinstance(expr_list[idx+1+i], Node) and
                                expr_list[idx+1+i].t == NodeType.PK):
                            match = False
                            break
                        else:
                            key = ECPubKey()
                            key.set(expr_list[idx+1+i]._pk[0])
                            pk_m.append(key)
                    # Match ... <m> <OP_CHECKMULTISIG>
                    if match is True:
                        m = Node._coerce_to_int(expr_list[idx+n+1])
                        if isinstance(m, int) and m == len(pk_m) and \
                                expr_list[idx+n+2] == OP_CHECKMULTISIG:
                            try:
                                node = Node().construct_thresh_m(k, pk_m)
                                expr_list = expr_list[:idx] + [node] + \
                                    expr_list[idx+n+3:]
                                return Node._parse_expr_list(expr_list)
                            except Exception:
                                pass

            # Right-to-left parsing.
            # Step one position left.
            idx -= 1

        # No match found.
        raise Exception("Malformed miniscript")

    def construct_just_1(self):
        self._construct(NodeType.JUST_1, Property().from_string("Bzufm"),
                        [],
                        [1], '1')
        return self

    def construct_just_0(self):
        self._construct(NodeType.JUST_0, Property().from_string("Bzudems"),
                        [],
                        [0], '0')
        return self

    def construct_pk(self, pubkey):
        assert isinstance(pubkey, ECPubKey) and pubkey.is_valid
        self._pk = [pubkey.get_bytes()]
        self._construct(NodeType.PK, Property().from_string("Konudems"),
                        [],
                        [pubkey.get_bytes()], 'pk('+self._pk[0].hex()+')'
                        )
        return self

    def construct_pk_h(self, pk_hash_digest):
        assert isinstance(pk_hash_digest, bytes) and len(pk_hash_digest) == 20
        self._pk_h = pk_hash_digest
        self._construct(NodeType.PK_H, Property().from_string("Knudems"),
                        [],
                        [OP_DUP, OP_HASH160, pk_hash_digest, OP_EQUALVERIFY],
                        'pk_h('+pk_hash_digest.hex()+')'
                        )
        return self

    def construct_older(self, delay):
        assert delay >= 1
        self._delay = delay
        self._construct(NodeType.OLDER, Property().from_string("Bzfm"),
                        [],
                        [delay, OP_CHECKSEQUENCEVERIFY],
                        'older('+str(delay)+')'
                        )
        return self

    def construct_after(self, time):
        assert time >= 1 and time < 2**31
        self._time = time
        self._construct(NodeType.AFTER, Property().from_string("Bzfm"),
                        [],
                        [time, OP_CHECKLOCKTIMEVERIFY],
                        'after('+str(time)+')'
                        )
        return self

    def construct_sha256(self, hash_digest):
        assert isinstance(hash_digest, bytes) and len(hash_digest) == 32
        self._sha256 = hash_digest
        self._construct(NodeType.SHA256, Property().from_string("Bonudm"),
                        [],
                        [OP_SIZE, 32, OP_EQUALVERIFY,
                         OP_SHA256, hash_digest, OP_EQUAL],
                        'sha256('+hash_digest.hex()+')'
                        )
        return self

    def construct_hash256(self, hash_digest):
        assert isinstance(hash_digest, bytes) and len(hash_digest) == 32
        self._hash256 = hash_digest
        self._construct(NodeType.HASH256, Property().from_string("Bonudm"),
                        [],
                        [OP_SIZE, 32, OP_EQUALVERIFY,
                         OP_HASH256, hash_digest, OP_EQUAL],
                        'hash256('+hash_digest.hex()+')'
                        )
        return self

    def construct_ripemd160(self, hash_digest):
        assert isinstance(hash_digest, bytes) and len(hash_digest) == 20
        self._ripemd160 = hash_digest
        self._construct(NodeType.RIPEMD160, Property().from_string("Bonudm"),
                        [],
                        [OP_SIZE, 32, OP_EQUALVERIFY,
                         OP_RIPEMD160, hash_digest, OP_EQUAL],
                        'ripemd160('+hash_digest.hex()+')'
                        )
        return self

    def construct_hash160(self, hash_digest):
        assert isinstance(hash_digest, bytes) and len(hash_digest) == 20
        self._hash160 = hash_digest
        self._construct(NodeType.HASH160, Property().from_string("Bonudm"),
                        [],
                        [OP_SIZE, 32, OP_EQUALVERIFY,
                         OP_HASH160, hash_digest, OP_EQUAL],
                        'hash160('+hash_digest.hex()+')'
                        )
        return self

    def construct_thresh_m(self, k, keys_n):
        self._k = k
        n = len(keys_n)
        assert n >= k >= 1
        prop_str = "Bnudems"
        self._pk_n = [key.get_bytes() for key in keys_n]
        desc = "thresh_m("+str(k)+","
        for idx, key_b in enumerate(self._pk_n):
            desc += key_b.hex()
            desc += "," if idx != (n-1) else ")"
        self._construct(NodeType.THRESH_M, Property().from_string(prop_str),
                        [],
                        [k, *self._pk_n, n, OP_CHECKMULTISIG], desc
                        )
        return self

    def _construct(self,
                   node_type, node_prop,
                   children,
                   script, desc):
        self.t = node_type
        self.p = node_prop
        self.children = children
        self._script = script
        self.desc = desc

    # Utility methods.
    @staticmethod
    def _coerce_to_int(expr):
        # Coerce expression to int when iterating through CScript expressions
        # after terminal expressions have been parsed.
        if isinstance(expr, bytes):
            op_int = int.from_bytes(expr, byteorder="big")
        elif isinstance(expr, Node) and expr.t == NodeType.JUST_0:
            op_int = 0
        elif isinstance(expr, Node) and expr.t == NodeType.JUST_1:
            op_int = 1
        else:
            op_int = expr
        return op_int

    @staticmethod
    def _parse_child_strings(child_exprs):
        child_nodes = []
        for string in child_exprs:
            child_nodes.append(Node.from_desc(string))
        return child_nodes

    @staticmethod
    def _parse_string(string):
        string = ''.join(string.split())
        tag = ''
        child_exprs = []
        depth = 0

        for idx, ch in enumerate(string):
            if (ch == '0' or ch == '1') and len(string) == 1:
                return ch, child_exprs
            if ch == ':' and depth == 0:
                # Discern between 1 or two wrappers.
                if idx == 1:
                    return string[0], [string[2:]]
                else:
                    return string[0], [string[1:]]
            if ch == '(':
                depth += 1
                if depth == 1:
                    tag = string[:idx]
                    prev_idx = idx
            if ch == ')':
                depth -= 1
                if depth == 0:
                    child_exprs.append(string[prev_idx+1:idx])
            if ch == ',' and depth == 1:
                child_exprs.append(string[prev_idx+1:idx])
                prev_idx = idx
        if depth == 0 and bool(tag) and bool(child_exprs):
            return tag, child_exprs
        else:
            raise Exception('Malformed miniscript string.')

    @staticmethod
    def _decompose_script(expr_list):
        idx = 0
        while idx < len(expr_list):
            if expr_list[idx] == OP_CHECKSIGVERIFY:
                expr_list = expr_list[:idx]+[OP_CHECKSIG, OP_VERIFY]+expr_list[
                    idx+1:]
            elif expr_list[idx] == OP_CHECKMULTISIGVERIFY:
                expr_list = expr_list[:idx]+[OP_CHECKMULTISIG, OP_VERIFY] + \
                    expr_list[idx+1:]
            elif expr_list[idx] == OP_EQUALVERIFY:
                expr_list = expr_list[:idx]+[OP_EQUAL, OP_VERIFY]+expr_list[
                    idx+1:]
            idx += 1
        return expr_list

    @staticmethod
    def _collapse_script(expr_list):
        idx = 0
        while idx < len(expr_list):
            if expr_list[idx:idx+1] == [OP_CHECKSIG, OP_VERIFY]:
                expr_list = expr_list[:idx]+[OP_CHECKSIGVERIFY]+expr_list[
                    idx+2:]
            elif expr_list[idx:idx+1] == [OP_CHECKMULTISIG, OP_VERIFY]:
                expr_list = expr_list[:idx]+[OP_CHECKMULTISIGVERIFY] + \
                    expr_list[idx+2:]
            elif expr_list[idx:idx+1] == [OP_EQUAL, OP_VERIFY]:
                expr_list = expr_list[:idx]+[OP_EQUALVERIFY]+expr_list[
                    idx+2:]
            idx += 1
        return expr_list
