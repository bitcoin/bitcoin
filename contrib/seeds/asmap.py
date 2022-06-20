# Copyright (c) 2022 Pieter Wuille
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
This module provides the ASNEntry and ASMap classes.
"""

import copy
import ipaddress
import random
import unittest
from enum import Enum
from functools import total_ordering
from typing import Callable, Dict, Iterable, List, Optional, Tuple, Union, overload

def net_to_prefix(net: Union[ipaddress.IPv4Network,ipaddress.IPv6Network]) -> List[bool]:
    """
    Convert an IPv4 or IPv6 network to a prefix represented as a list of bits.

    IPv4 ranges are remapped to their IPv4-mapped IPv6 range (::ffff:0:0/96).
    """
    num_bits = net.prefixlen
    netrange = int.from_bytes(net.network_address.packed, 'big')

    # Map an IPv4 prefix into IPv6 space.
    if isinstance(net, ipaddress.IPv4Network):
        num_bits += 96
        netrange += 0xffff00000000

    # Strip unused bottom bits.
    assert (netrange & ((1 << (128 - num_bits)) - 1)) == 0
    return [((netrange >> (127 - i)) & 1) != 0 for i in range(num_bits)]

def prefix_to_net(prefix: List[bool]) -> Union[ipaddress.IPv4Network,ipaddress.IPv6Network]:
    """The reverse operation of net_to_prefix."""
    # Convert to number
    netrange = sum(b << (127 - i) for i, b in enumerate(prefix))
    num_bits = len(prefix)
    assert num_bits <= 128

    # Return IPv4 range if in ::ffff:0:0/96
    if num_bits >= 96 and (netrange >> 32) == 0xffff:
        return ipaddress.IPv4Network((netrange & 0xffffffff, num_bits - 96), True)

    # Return IPv6 range otherwise.
    return ipaddress.IPv6Network((netrange, num_bits), True)

# Shortcut for (prefix, ASN) entries.
ASNEntry = Tuple[List[bool], int]

# Shortcut for (prefix, old ASN, new ASN) entries.
ASNDiff = Tuple[List[bool], int, int]

class _VarLenCoder:
    """
    A class representing a custom variable-length binary encoder/decoder for
    integers. Each object represents a different coder, with different parameters
    minval and clsbits.

    The encoding is easiest to describe using an example. Let's say minval=100 and
    clsbits=[4,2,2,3]. In that case:
    - x in [100..115]: encoded as [0] + [4-bit BE encoding of (x-100)].
    - x in [116..119]: encoded as [1,0] + [2-bit BE encoding of (x-116)].
    - x in [120..123]: encoded as [1,1,0] + [2-bit BE encoding of (x-120)].
    - x in [124..131]: encoded as [1,1,1] + [3-bit BE encoding of (x-124)].

    In general, every number is encoded as:
    - First, k "1"-bits, where k is the class the number falls in (there is one class
      per element of clsbits).
    - Then, a "0"-bit, unless k is the highest class, in which case there is nothing.
    - Lastly, clsbits[k] bits encoding in big endian the position in its class that
      number falls into.
    - Every class k consists of 2^clsbits[k] consecutive integers. k=0 starts at minval,
      other classes start one past the last element of the class before it.
    """

    def __init__(self, minval: int, clsbits: List[int]):
        """Construct a new _VarLenCoder."""
        self._minval = minval
        self._clsbits = clsbits
        self._maxval = minval + sum(1 << b for b in clsbits) - 1

    def can_encode(self, val: int) -> bool:
        """Check whether value val is in the range this coder supports."""
        return self._minval <= val <= self._maxval

    def encode(self, val: int, ret: List[int]) -> None:
        """Append encoding of val onto integer list ret."""

        assert self._minval <= val <= self._maxval
        val -= self._minval
        bits = 0
        for k, bits in enumerate(self._clsbits):
            if val >> bits:
                # If the value will not fit in class k, subtract its range from v,
                # emit a "1" bit and continue with the next class.
                val -= 1 << bits
                ret.append(1)
            else:
                if k + 1 < len(self._clsbits):
                    # Unless we're in the last class, emit a "0" bit.
                    ret.append(0)
                break
        # And then encode v (now the position within the class) in big endian.
        ret.extend((val >> (bits - 1 - b)) & 1 for b in range(bits))

    def encode_size(self, val: int) -> int:
        """Compute how many bits are needed to encode val."""
        assert self._minval <= val <= self._maxval
        val -= self._minval
        ret = 0
        bits = 0
        for k, bits in enumerate(self._clsbits):
            if val >> bits:
                val -= 1 << bits
                ret += 1
            else:
                ret += k + 1 < len(self._clsbits)
                break
        return ret + bits

    def decode(self, stream, bitpos) -> Tuple[int,int]:
        """Decode a number starting at bitpos in stream, returning value and new bitpos."""
        val = self._minval
        bits = 0
        for k, bits in enumerate(self._clsbits):
            bit = 0
            if k + 1 < len(self._clsbits):
                bit = stream[bitpos]
                bitpos += 1
            if not bit:
                break
            val += 1 << bits
        for i in range(bits):
            bit = stream[bitpos]
            bitpos += 1
            val += bit << (bits - 1 - i)
        return val, bitpos

# Variable-length encoders used in the binary asmap format.
_CODER_INS = _VarLenCoder(0, [0, 0, 1])
_CODER_ASN = _VarLenCoder(1, list(range(15, 25)))
_CODER_MATCH = _VarLenCoder(2, list(range(1, 9)))
_CODER_JUMP = _VarLenCoder(17, list(range(5, 31)))

class _Instruction(Enum):
    """One instruction in the binary asmap format."""
    # A return instruction, encoded as [0], returns a constant ASN. It is followed by
    # an integer using the ASN encoding.
    RETURN = 0
    # A jump instruction, encoded as [1,0] inspects the next unused bit in the input
    # and either continues execution (if 0), or skips a specified number of bits (if 1).
    # It is followed by an integer, and then two subprograms. The integer uses jump encoding
    # and corresponds to the length of the first subprogram (so it can be skipped).
    JUMP = 1
    # A match instruction, encoded as [1,1,0] inspects 1 or more of the next unused bits
    # in the input with its argument. If they all match, execution continues. If they do
    # not, failure is returned. If a default instruction has been executed before, instead
    # of failure the default instruction's argument is returned. It is followed by an
    # integer in match encoding, and a subprogram. That value is at least 2 bits and at
    # most 9 bits. An n-bit value signifies matching (n-1) bits in the input with the lower
    # (n-1) bits in the match value.
    MATCH = 2
    # A default instruction, encoded as [1,1,1] sets the default variable to its argument,
    # and continues execution. It is followed by an integer in ASN encoding, and a subprogram.
    DEFAULT = 3
    # Not an actual instruction, but a way to encode the empty program that fails. In the
    # encoder, it is used more generally to represent the failure case inside MATCH instructions,
    # which may (if used inside the context of a DEFAULT instruction) actually correspond to
    # a successful return. In this usage, they're always converted to an actual MATCH or RETURN
    # before the top level is reached (see make_default below).
    END = 4

class _BinNode:
    """A class representing a (node of) the parsed binary asmap format."""

    @overload
    def __init__(self, ins: _Instruction): ...
    @overload
    def __init__(self, ins: _Instruction, arg1: int): ...
    @overload
    def __init__(self, ins: _Instruction, arg1: "_BinNode", arg2: "_BinNode"): ...
    @overload
    def __init__(self, ins: _Instruction, arg1: int, arg2: "_BinNode"): ...

    def __init__(self, ins: _Instruction, arg1=None, arg2=None):
        """
        Construct a new asmap node. Possibilities are:
        - _BinNode(_Instruction.RETURN, asn)
        - _BinNode(_Instruction.JUMP, node_0, node_1)
        - _BinNode(_Instruction.MATCH, val, node)
        - _BinNode(_Instruction.DEFAULT, asn, node)
        - _BinNode(_Instruction.END)
        """
        self.ins = ins
        self.arg1 = arg1
        self.arg2 = arg2
        if ins == _Instruction.RETURN:
            assert isinstance(arg1, int)
            assert arg2 is None
            self.size = _CODER_INS.encode_size(ins.value) + _CODER_ASN.encode_size(arg1)
        elif ins == _Instruction.JUMP:
            assert isinstance(arg1, _BinNode)
            assert isinstance(arg2, _BinNode)
            self.size = (_CODER_INS.encode_size(ins.value) + _CODER_JUMP.encode_size(arg1.size) +
                         arg1.size + arg2.size)
        elif ins == _Instruction.DEFAULT:
            assert isinstance(arg1, int)
            assert isinstance(arg2, _BinNode)
            self.size = _CODER_INS.encode_size(ins.value) + _CODER_ASN.encode_size(arg1) + arg2.size
        elif ins == _Instruction.MATCH:
            assert isinstance(arg1, int)
            assert isinstance(arg2, _BinNode)
            self.size = (_CODER_INS.encode_size(ins.value) + _CODER_MATCH.encode_size(arg1)
                         + arg2.size)
        elif ins == _Instruction.END:
            assert arg1 is None
            assert arg2 is None
            self.size = 0
        else:
            assert False

    @staticmethod
    def make_end() -> "_BinNode":
        """Constructor for a _BinNode with just an END instruction."""
        return _BinNode(_Instruction.END)

    @staticmethod
    def make_leaf(val: int) -> "_BinNode":
        """Constructor for a _BinNode of just a RETURN instruction."""
        assert val is not None and val > 0
        return _BinNode(_Instruction.RETURN, val)

    @staticmethod
    def make_branch(node0: "_BinNode", node1: "_BinNode") -> "_BinNode":
        """
        Construct a _BinNode corresponding to running either the node0 or node1 subprogram,
        based on the next input bit. It exploits shortcuts that are possible in the encoding,
        and uses either a JUMP, MATCH, or END instruction.
        """
        if node0.ins == _Instruction.END and node1.ins == _Instruction.END:
            return node0
        if node0.ins == _Instruction.END:
            if node1.ins == _Instruction.MATCH and node1.arg1 <= 0xFF:
                return _BinNode(node1.ins, node1.arg1 + (1 << node1.arg1.bit_length()), node1.arg2)
            return _BinNode(_Instruction.MATCH, 3, node1)
        if node1.ins == _Instruction.END:
            if node0.ins == _Instruction.MATCH and node0.arg1 <= 0xFF:
                return _BinNode(node0.ins, node0.arg1 + (1 << (node0.arg1.bit_length() - 1)),
                                node0.arg2)
            return _BinNode(_Instruction.MATCH, 2, node0)
        return _BinNode(_Instruction.JUMP, node0, node1)

    @staticmethod
    def make_default(val: int, sub: "_BinNode") -> "_BinNode":
        """
        Construct a _BinNode that corresponds to the specified subprogram, with the specified
        default value. It exploits shortcuts that are possible in the encoding, and will use
        either a DEFAULT or a RETURN instruction."""
        assert val is not None and val > 0
        if sub.ins == _Instruction.END:
            return _BinNode(_Instruction.RETURN, val)
        if sub.ins in (_Instruction.RETURN, _Instruction.DEFAULT):
            return sub
        return _BinNode(_Instruction.DEFAULT, val, sub)

@total_ordering
class ASMap:
    """
    A class whose objects represent a mapping from subnets to ASNs.

    Internally the mapping is stored as a binary trie, but can be converted
    from/to a list of ASNEntry objects, and from/to the binary asmap file format.

    In the trie representation, nodes are represented as bare lists for efficiency
    and ease of manipulation:
    - [0] means an unassigned subnet (no ASN mapping for it is present)
    - [int] means a subnet mapped entirely to the specified ASN.
    - [node,node] means a subnet whose lower half and upper half have different
    -             mappings, represented by new trie nodes.
    """

    def update(self, prefix: List[bool], asn: int) -> None:
        """Update this ASMap object to map prefix to the specified asn."""
        assert asn == 0 or _CODER_ASN.can_encode(asn)

        def recurse(node: List, offset: int) -> None:
            if offset == len(prefix):
                # Reached the end of prefix; overwrite this node.
                node.clear()
                node.append(asn)
                return
            if len(node) == 1:
                # Need to descend into a leaf node; split it up.
                oldasn = node[0]
                node.clear()
                node.append([oldasn])
                node.append([oldasn])
            # Descend into the node.
            recurse(node[prefix[offset]], offset + 1)
            # If the result is two identical leaf children, merge them.
            if len(node[0]) == 1 and len(node[1]) == 1 and node[0] == node[1]:
                oldasn = node[0][0]
                node.clear()
                node.append(oldasn)
        recurse(self._trie, 0)

    def update_multi(self, entries: List[Tuple[List[bool], int]]) -> None:
        """Apply multiple update operations, where longer prefixes take precedence."""
        entries.sort(key=lambda entry: len(entry[0]))
        for prefix, asn in entries:
            self.update(prefix, asn)

    def _set_trie(self, trie) -> None:
        """Set trie directly. Internal use only."""
        def recurse(node: List) -> None:
            if len(node) < 2:
                return
            recurse(node[0])
            recurse(node[1])
            if len(node[0]) == 2:
                return
            if node[0] == node[1]:
                if len(node[0]) == 0:
                    node.clear()
                else:
                    asn = node[0][0]
                    node.clear()
                    node.append(asn)
        recurse(trie)
        self._trie = trie

    def __init__(self, entries: Optional[Iterable[ASNEntry]] = None) -> None:
        """Construct an ASMap object from an optional list of entries."""
        self._trie = [0]
        if entries is not None:
            def entry_key(entry):
                """Sort function that places shorter prefixes first."""
                prefix, asn = entry
                return len(prefix), prefix, asn
            for prefix, asn in sorted(entries, key=entry_key):
                self.update(prefix, asn)

    def lookup(self, prefix: List[bool]) -> Optional[int]:
        """Look up a prefix. Returns ASN, or 0 if unassigned, or None if indeterminate."""
        node = self._trie
        for bit in prefix:
            if len(node) == 1:
                break
            node = node[bit]
        if len(node) == 1:
            return node[0]
        return None

    def _to_entries_flat(self, fill: bool = False) -> List[ASNEntry]:
        """Convert an ASMap object to a list of non-overlapping (prefix, asn) objects."""
        prefix : List[bool] = []

        def recurse(node: List) -> List[ASNEntry]:
            ret = []
            if len(node) == 1:
                if node[0] > 0:
                    ret = [(list(prefix), node[0])]
            elif len(node) == 2:
                prefix.append(False)
                ret = recurse(node[0])
                prefix[-1] = True
                ret += recurse(node[1])
                prefix.pop()
                if fill and len(ret) > 1:
                    asns = set(x[1] for x in ret)
                    if len(asns) == 1:
                        ret = [(list(prefix), list(asns)[0])]
            return ret
        return recurse(self._trie)

    def _to_entries_minimal(self, fill: bool = False) -> List[ASNEntry]:
        """Convert a trie to a minimal list of ASNEntry objects, exploiting overlap."""
        prefix : List[bool] = []

        def recurse(node: List) -> (Tuple[Dict[Optional[int], List[ASNEntry]], bool]):
            if len(node) == 1 and node[0] == 0:
                return {None if fill else 0: []}, True
            if len(node) == 1:
                return {node[0]: [], None: [(list(prefix), node[0])]}, False
            ret: Dict[Optional[int], List[ASNEntry]] = {}
            prefix.append(False)
            left, lhole = recurse(node[0])
            prefix[-1] = True
            right, rhole = recurse(node[1])
            prefix.pop()
            hole = not fill and (lhole or rhole)
            def candidate(ctx: Optional[int], res0: Optional[List[ASNEntry]],
                    res1: Optional[List[ASNEntry]]):
                if res0 is not None and res1 is not None:
                    if ctx not in ret or len(res0) + len(res1) < len(ret[ctx]):
                        ret[ctx] = res0 + res1
            for ctx in set(left) | set(right):
                candidate(ctx, left.get(ctx), right.get(ctx))
                candidate(ctx, left.get(None), right.get(ctx))
                candidate(ctx, left.get(ctx), right.get(None))
            if not hole:
                for ctx in list(ret):
                    if ctx is not None:
                        candidate(None, [(list(prefix), ctx)], ret[ctx])
            if None in ret:
                ret = {ctx:entries for ctx, entries in ret.items()
                       if ctx is None or len(entries) < len(ret[None])}
            if hole:
                ret = {ctx:entries for ctx, entries in ret.items() if ctx is None or ctx == 0}
            return ret, hole
        res, _ = recurse(self._trie)
        return res[0] if 0 in res else res[None]

    def __str__(self) -> str:
        """Convert this ASMap object to a string containing Python code constructing it."""
        return f"ASMap({self._trie})"

    def to_entries(self, overlapping: bool = True, fill: bool = False) -> List[ASNEntry]:
        """
        Convert the mappings in this ASMap object to a list of ASNEntry objects.

        Arguments:
            overlapping: Permit the subnets in the resulting ASNEntry to overlap.
                         Setting this can result in a shorter list.
            fill:        Permit the resulting ASNEntry objects to cover subnets that
                         are unassigned in this ASMap object. Setting this can
                         result in a shorter list.
        """
        if overlapping:
            return self._to_entries_minimal(fill)
        return self._to_entries_flat(fill)

    @staticmethod
    def from_random(num_leaves: int = 10, max_asn: int = 6,
                    unassigned_prob: float = 0.5) -> "ASMap":
        """
        Construct a random ASMap object, with specified:
         - Number of leaves in its trie (at least 1)
         - Maximum ASN value (at least 1)
         - Probability for leaf nodes to be unassigned

        The number of leaves in the resulting object may be less than what is
        requested. This method is mostly intended for testing.
        """
        assert num_leaves >= 1
        assert max_asn >= 1 or unassigned_prob == 1
        assert _CODER_ASN.can_encode(max_asn)
        assert 0.0 <= unassigned_prob <= 1.0
        trie: List = []
        leaves = [trie]
        ret = ASMap()
        for i in range(1, num_leaves):
            idx = random.randrange(i)
            leaf = leaves[idx]
            lastleaf = leaves.pop()
            if idx + 1 < i:
                leaves[idx] = lastleaf
            leaf.append([])
            leaf.append([])
            leaves.append(leaf[0])
            leaves.append(leaf[1])
        for leaf in leaves:
            if random.random() >= unassigned_prob:
                leaf.append(random.randrange(1, max_asn + 1))
            else:
                leaf.append(0)
        #pylint: disable=protected-access
        ret._set_trie(trie)
        return ret

    def _to_binnode(self, fill: bool = False) -> _BinNode:
        """Convert a trie to a _BinNode object."""
        def recurse(node: List) -> Tuple[Dict[Optional[int], _BinNode], bool]:
            if len(node) == 1 and node[0] == 0:
                return {(None if fill else 0): _BinNode.make_end()}, True
            if len(node) == 1:
                return {None: _BinNode.make_leaf(node[0]), node[0]: _BinNode.make_end()}, False
            ret: Dict[Optional[int], _BinNode] = {}
            left, lhole = recurse(node[0])
            right, rhole = recurse(node[1])
            hole = (lhole or rhole) and not fill

            def candidate(ctx: Optional[int], arg1, arg2, func: Callable):
                if arg1 is not None and arg2 is not None:
                    cand = func(arg1, arg2)
                    if ctx not in ret or cand.size < ret[ctx].size:
                        ret[ctx] = cand

            for ctx in set(left) | set(right):
                candidate(ctx, left.get(ctx), right.get(ctx), _BinNode.make_branch)
                candidate(ctx, left.get(None), right.get(ctx), _BinNode.make_branch)
                candidate(ctx, left.get(ctx), right.get(None), _BinNode.make_branch)
            if not hole:
                for ctx in set(ret) - set([None]):
                    candidate(None, ctx, ret[ctx], _BinNode.make_default)
            if None in ret:
                ret = {ctx:enc for ctx, enc in ret.items()
                       if ctx is None or enc.size < ret[None].size}
            if hole:
                ret = {ctx:enc for ctx, enc in ret.items() if ctx is None or ctx == 0}
            return ret, hole
        res, _ = recurse(self._trie)
        return res[0] if 0 in res else res[None]

    @staticmethod
    def _from_binnode(binnode: _BinNode) -> "ASMap":
        """Construct an ASMap object from a _BinNode. Internal use only."""
        def recurse(node: _BinNode, default: int) -> List:
            if node.ins == _Instruction.RETURN:
                return [node.arg1]
            if node.ins == _Instruction.JUMP:
                return [recurse(node.arg1, default), recurse(node.arg2, default)]
            if node.ins == _Instruction.MATCH:
                val = node.arg1
                sub = recurse(node.arg2, default)
                while val >= 2:
                    bit = val & 1
                    val >>= 1
                    if bit:
                        sub = [[default], sub]
                    else:
                        sub = [sub, [default]]
                return sub
            assert node.ins == _Instruction.DEFAULT
            return recurse(node.arg2, node.arg1)
        ret = ASMap()
        if binnode.ins != _Instruction.END:
            #pylint: disable=protected-access
            ret._set_trie(recurse(binnode, 0))
        return ret

    def to_binary(self, fill: bool = False) -> bytes:
        """
        Convert this ASMap object to binary.

        Argument:
            fill: permit the resulting binary encoder to contain mappers for
                  unassigned subnets in this ASMap object. Doing so may
                  reduce the size of the encoding.
        Returns:
            A bytes object with the encoding of this ASMap object.
        """
        bits: List[int] = []

        def recurse(node: _BinNode) -> None:
            _CODER_INS.encode(node.ins.value, bits)
            if node.ins == _Instruction.RETURN:
                _CODER_ASN.encode(node.arg1, bits)
            elif node.ins == _Instruction.JUMP:
                _CODER_JUMP.encode(node.arg1.size, bits)
                recurse(node.arg1)
                recurse(node.arg2)
            elif node.ins == _Instruction.DEFAULT:
                _CODER_ASN.encode(node.arg1, bits)
                recurse(node.arg2)
            else:
                assert node.ins == _Instruction.MATCH
                _CODER_MATCH.encode(node.arg1, bits)
                recurse(node.arg2)

        binnode = self._to_binnode(fill)
        if binnode.ins != _Instruction.END:
            recurse(binnode)

        val = 0
        nbits = 0
        ret = []
        for bit in bits:
            val += (bit << nbits)
            nbits += 1
            if nbits == 8:
                ret.append(val)
                val = 0
                nbits = 0
        if nbits:
            ret.append(val)
        return bytes(ret)

    @staticmethod
    def from_binary(bindata: bytes) -> Optional["ASMap"]:
        """Decode an ASMap object from the provided binary encoding."""

        bits: List[int] = []
        for byte in bindata:
            bits.extend((byte >> i) & 1 for i in range(8))

        def recurse(bitpos: int) -> Tuple[_BinNode, int]:
            insval, bitpos = _CODER_INS.decode(bits, bitpos)
            ins = _Instruction(insval)
            if ins == _Instruction.RETURN:
                asn, bitpos = _CODER_ASN.decode(bits, bitpos)
                return _BinNode(ins, asn), bitpos
            if ins == _Instruction.JUMP:
                jump, bitpos = _CODER_JUMP.decode(bits, bitpos)
                left, bitpos1 = recurse(bitpos)
                if bitpos1 != bitpos + jump:
                    raise ValueError("Inconsistent jump")
                right, bitpos = recurse(bitpos1)
                return _BinNode(ins, left, right), bitpos
            if ins == _Instruction.MATCH:
                match, bitpos = _CODER_MATCH.decode(bits, bitpos)
                sub, bitpos = recurse(bitpos)
                return _BinNode(ins, match, sub), bitpos
            assert ins == _Instruction.DEFAULT
            asn, bitpos = _CODER_ASN.decode(bits, bitpos)
            sub, bitpos = recurse(bitpos)
            return _BinNode(ins, asn, sub), bitpos

        if len(bits) == 0:
            binnode = _BinNode(_Instruction.END)
        else:
            try:
                binnode, bitpos = recurse(0)
            except (ValueError, IndexError):
                return None
            if bitpos < len(bits) - 7:
                return None
            if not all(bit == 0 for bit in bits[bitpos:]):
                return None

        return ASMap._from_binnode(binnode)

    def __lt__(self, other: "ASMap") -> bool:
        return self._trie < other._trie

    def __eq__(self, other: object) -> bool:
        if isinstance(other, ASMap):
            return self._trie == other._trie
        return False

    def extends(self, req: "ASMap") -> bool:
        """Determine whether this matches req for all subranges where req is assigned."""
        def recurse(actual: List, require: List) -> bool:
            if len(require) == 1 and require[0] == 0:
                return True
            if len(require) == 1:
                if len(actual) == 1:
                    return bool(require[0] == actual[0])
                return recurse(actual[0], require) and recurse(actual[1], require)
            if len(actual) == 2:
                return recurse(actual[0], require[0]) and recurse(actual[1], require[1])
            return recurse(actual, require[0]) and recurse(actual, require[1])
        assert isinstance(req, ASMap)
        #pylint: disable=protected-access
        return recurse(self._trie, req._trie)

    def diff(self, other: "ASMap") -> List[ASNDiff]:
        """Compute the diff from self to other."""
        prefix: List[bool] = []
        ret: List[ASNDiff] = []

        def recurse(old_node: List, new_node: List):
            if len(old_node) == 1 and len(new_node) == 1:
                if old_node[0] != new_node[0]:
                    ret.append((list(prefix), old_node[0], new_node[0]))
            else:
                old_left: List = old_node if len(old_node) == 1 else old_node[0]
                old_right: List = old_node if len(old_node) == 1 else old_node[1]
                new_left: List = new_node if len(new_node) == 1 else new_node[0]
                new_right: List = new_node if len(new_node) == 1 else new_node[1]
                prefix.append(False)
                recurse(old_left, new_left)
                prefix[-1] = True
                recurse(old_right, new_right)
                prefix.pop()
        assert isinstance(other, ASMap)
        #pylint: disable=protected-access
        recurse(self._trie, other._trie)
        return ret

    def __copy__(self) -> "ASMap":
        """Construct a copy of this ASMap object. Its state will not be shared."""
        ret = ASMap()
        #pylint: disable=protected-access
        ret._set_trie(copy.deepcopy(self._trie))
        return ret

    def __deepcopy__(self, _) -> "ASMap":
        # ASMap objects do not allow sharing of the _trie member, so we don't need the memoization.
        return self.__copy__()


class TestASMap(unittest.TestCase):
    """Unit tests for this module."""

    def test_ipv6_prefix_roundtrips(self) -> None:
        """Test that random IPv6 network ranges roundtrip through prefix encoding."""
        for _ in range(20):
            net_bits = random.getrandbits(128)
            for prefix_len in range(0, 129):
                masked_bits = (net_bits >> (128 - prefix_len)) << (128 - prefix_len)
                net = ipaddress.IPv6Network((masked_bits.to_bytes(16, 'big'), prefix_len))
                prefix = net_to_prefix(net)
                self.assertTrue(len(prefix) <= 128)
                net2 = prefix_to_net(prefix)
                self.assertEqual(net, net2)

    def test_ipv4_prefix_roundtrips(self) -> None:
        """Test that random IPv4 network ranges roundtrip through prefix encoding."""
        for _ in range(100):
            net_bits = random.getrandbits(32)
            for prefix_len in range(0, 33):
                masked_bits = (net_bits >> (32 - prefix_len)) << (32 - prefix_len)
                net = ipaddress.IPv4Network((masked_bits.to_bytes(4, 'big'), prefix_len))
                prefix = net_to_prefix(net)
                self.assertTrue(32 <= len(prefix) <= 128)
                net2 = prefix_to_net(prefix)
                self.assertEqual(net, net2)

    def test_asmap_roundtrips(self) -> None:
        """Test case that verifies random ASMap objects roundtrip to/from entries/binary."""
        # Iterate over the number of leaves the random test ASMap objects have.
        for leaves in range(1, 20):
            # Iterate over the number of bits in the AS numbers used.
            for asnbits in range(0, 24):
                # Iterate over the probability that leaves are unassigned.
                for pct in range(101):
                    # Construct a random ASMap object according to the above parameters.
                    asmap = ASMap.from_random(num_leaves=leaves, max_asn=1 + (1 << asnbits),
                                              unassigned_prob=0.01 * pct)
                    # Run tests for to_entries and construction from those entries, both
                    # for overlapping and non-overlapping ones.
                    for overlapping in [False, True]:
                        entries = asmap.to_entries(overlapping=overlapping, fill=False)
                        random.shuffle(entries)
                        asmap2 = ASMap(entries)
                        assert asmap2 is not None
                        self.assertEqual(asmap2, asmap)
                        entries = asmap.to_entries(overlapping=overlapping, fill=True)
                        random.shuffle(entries)
                        asmap2 = ASMap(entries)
                        assert asmap2 is not None
                        self.assertTrue(asmap2.extends(asmap))

                    # Run tests for to_binary and construction from binary.
                    enc = asmap.to_binary(fill=False)
                    asmap3 = ASMap.from_binary(enc)
                    assert asmap3 is not None
                    self.assertEqual(asmap3, asmap)
                    enc = asmap.to_binary(fill=True)
                    asmap3 = ASMap.from_binary(enc)
                    assert asmap3 is not None
                    self.assertTrue(asmap3.extends(asmap))

    def test_patching(self) -> None:
        """Test behavior of update, lookup, extends, and diff."""
        #pylint: disable=too-many-locals,too-many-nested-blocks
        # Iterate over the number of leaves the random test ASMap objects have.
        for leaves in range(1, 20):
            # Iterate over the number of bits in the AS numbers used.
            for asnbits in range(0, 10):
                # Iterate over the probability that leaves are unassigned.
                for pct in range(0, 101):
                    # Construct a random ASMap object according to the above parameters.
                    asmap = ASMap.from_random(num_leaves=leaves, max_asn=1 + (1 << asnbits),
                                              unassigned_prob=0.01 * pct)
                    # Make a copy of that asmap object to which patches will be applied.
                    # It starts off being equal to asmap.
                    patched = copy.copy(asmap)
                    # Keep a list of patches performed.
                    patches: List[ASNEntry] = []
                    # Initially there cannot be any difference.
                    self.assertEqual(asmap.diff(patched), [])
                    # Make 5 patches, each building on top of the previous ones.
                    for _ in range(0, 5):
                        # Construct a random path and new ASN to assign it to, apply it to patched,
                        # and remember it in patches.
                        pathlen = random.randrange(5)
                        path = [random.getrandbits(1) != 0 for _ in range(pathlen)]
                        newasn = random.randrange(1 + (1 << asnbits))
                        patched.update(path, newasn)
                        patches = [(path, newasn)] + patches

                        # Compute the diff, and whether asmap extends patched, and the other way
                        # around.
                        diff = asmap.diff(patched)
                        self.assertEqual(asmap == patched, len(diff) == 0)
                        extends = asmap.extends(patched)
                        back_extends = patched.extends(asmap)
                        # Determine whether those extends results are consistent with the diff
                        # result.
                        self.assertEqual(extends, all(d[2] == 0 for d in diff))
                        self.assertEqual(back_extends, all(d[1] == 0 for d in diff))
                        # For every diff found:
                        for path, old_asn, new_asn in diff:
                            # Verify asmap and patched actually differ there.
                            self.assertTrue(old_asn != new_asn)
                            self.assertEqual(asmap.lookup(path), old_asn)
                            self.assertEqual(patched.lookup(path), new_asn)
                            for _ in range(2):
                                # Extend the path far enough that it's smaller than any mapped
                                # range, and check the lookup holds there too.
                                spec_path = list(path)
                                while len(spec_path) < 32:
                                    spec_path.append(random.getrandbits(1) != 0)
                                self.assertEqual(asmap.lookup(spec_path), old_asn)
                                self.assertEqual(patched.lookup(spec_path), new_asn)
                                # Search through the list of performed patches to find the last one
                                # applying to the extended path (note that patches is in reverse
                                # order, so the first match should work).
                                found = False
                                for patch_path, patch_asn in patches:
                                    if spec_path[:len(patch_path)] == patch_path:
                                        # When found, it must match whatever the result was patched
                                        # to.
                                        self.assertEqual(new_asn, patch_asn)
                                        found = True
                                        break
                                # And such a patch must exist.
                                self.assertTrue(found)

if __name__ == '__main__':
    unittest.main()
