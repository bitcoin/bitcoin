# Sourced from https://gist.github.com/dwightguth/283afe96b60b3793f3c02036701457f8
# with light modifications.

import gdb.printing
import decimal
import traceback
import re

MAX = 1 << 64 - 1


class ArrayIter:
    def __init__(self, val):
        self.val_ptr = val.type.template_argument(0).pointer()
        self.v = val['impl_']
        self.size = self.v['size']
        self.i = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self.i == self.size:
            raise StopIteration
        ptr = self.v['ptr']
        data = ptr.dereference()['impl']['d']['buffer'].address.reinterpret_cast(self.val_ptr)
        result = ('[%d]' % self.i, data[self.i])
        self.i += 1
        return result


class Relaxed:
    def __init__(self, node, shift, relaxed, it):
        self.node = node
        self.shift = shift
        self.relaxed = relaxed
        self.it = it
        self.B = self.node.type.target().template_argument(2)
        self.BL = self.node.type.target().template_argument(3)

    def index(self, idx):
        offset = idx >> self.shift
        while self.relaxed.dereference()['d']['sizes'][offset] <= idx:
            offset += 1
        return offset

    def towards(self, idx):
        offset = self.index(idx)
        left_size = self.relaxed.dereference()['d']['sizes'][offset - 1] if offset else 0
        child = self.it.inner(self.node)[offset]
        is_leaf = self.shift == self.BL
        next_size = self.relaxed.dereference()['d']['sizes'][offset] - left_size
        next_idx = idx - left_size
        if is_leaf:
            return self.it.visit_leaf(LeafSub(child, next_size), next_idx)
        else:
            return self.it.visit_maybe_relaxed_sub(child, self.shift - self.B, next_size, next_idx)


class LeafSub:
    def __init__(self, node, count):
        self.node = node
        self.count_ = count
        self.BL = self.node.type.target().template_argument(3)
        self.MASK = (1 << self.BL) - 1

    def index(self, idx):
        return idx & self.MASK

    def count(self):
        return self.count_


class FullLeaf:
    def __init__(self, node):
        self.node = node
        self.BL = self.node.type.target().template_argument(3)
        self.BRANCHES = 1 << self.BL
        self.MASK = self.BRANCHES - 1

    def index(self, idx):
        return idx & self.MASK

    def count(self):
        return self.BRANCHES


class Leaf:
    def __init__(self, node, size):
        self.node = node
        self.size = size
        self.BL = self.node.type.target().template_argument(3)
        self.MASK = (1 << self.BL) - 1

    def index(self, idx):
        return idx & self.MASK

    def count(self):
        return self.index(self.size - 1) + 1


class RegularSub:
    def __init__(self, node, shift, size, it):
        self.node = node
        self.shift = shift
        self.size = size
        self.it = it
        self.B = self.node.type.target().template_argument(2)
        self.MASK = (1 << self.B) - 1

    def towards(self, idx):
        offset = self.index(idx)
        count = self.count()
        return self.it.towards_regular(self, idx, offset, count)

    def index(self, idx):
        return (idx >> self.shift) & self.MASK

    def count(self):
        return self.subindex(self.size - 1) + 1

    def subindex(self, idx):
        return idx >> self.shift


class Regular:
    def __init__(self, node, shift, size, it):
        self.node = node
        self.shift = shift
        self.size = size
        self.it = it
        self.B = self.node.type.target().template_argument(2)
        self.MASK = (1 << self.B) - 1

    def index(self, idx):
        return (idx >> self.shift) & self.MASK

    def count(self):
        return self.index(self.size - 1) + 1

    def towards(self, idx):
        offset = self.index(idx)
        count = self.count()
        return self.it.towards_regular(self, idx, offset, count)


class Full:
    def __init__(self, node, shift, it):
        self.node = node
        self.shift = shift
        self.it = it
        self.B = self.node.type.target().template_argument(2)
        self.BL = self.node.type.target().template_argument(3)
        self.MASK = (1 << self.B) - 1

    def index(self, idx):
        return (idx >> self.shift) & self.MASK

    def towards(self, idx):
        offset = self.index(idx)
        is_leaf = self.shift == self.BL
        child = self.it.inner(self.node)[offset]
        if is_leaf:
            return self.it.visit_leaf(FullLeaf(child), idx)
        else:
            return Full(child, self.shift - self.B, self.it).towards(idx)


class ListIter:
    def __init__(self, val):
        self.v = val['impl_']
        self.size = self.v['size']
        self.i = 0
        self.curr = (None, MAX, MAX)
        self.node_ptr_ptr = self.v['root'].type.pointer()
        self.B = self.v['root'].type.target().template_argument(2)
        self.BL = self.v['root'].type.target().template_argument(3)

    def __iter__(self):
        return self

    def __next__(self):
        if self.i == self.size:
            raise StopIteration
        if self.i < self.curr[1] or self.i >= self.curr[2]:
            self.curr = self.region()
        result = ('[%d]' % self.i, self.curr[0][self.i - self.curr[1]].cast(
            gdb.lookup_type(self.v.type.template_argument(0).name)))
        self.i += 1
        return result

    def region(self):
        tail_off = self.tail_offset()
        if self.i >= tail_off:
            return (self.leaf(self.v['tail']), tail_off, self.size)
        else:
            subs = self.visit_maybe_relaxed_sub(self.v['root'], self.v['shift'], tail_off, self.i)
            first = self.i - subs[1]
            end = first + subs[2]
            return (subs[0], first, end)

    def tail_offset(self):
        r = self.relaxed(self.v['root'])
        if r:
            return r.dereference()['d']['sizes'][r.dereference()['d']['count'] - 1]
        elif self.size:
            return (self.size - 1) & ~self.leaf_mask()
        else:
            return 0

    def relaxed(self, node):
        return node.dereference()['impl']['d']['data']['inner']['relaxed']

    def leaf(self, node):
        return node.dereference()['impl']['d']['data']['leaf']['buffer'].address

    def inner(self, node):
        return node.dereference()['impl']['d']['data']['inner']['buffer'].address.reinterpret_cast(
            self.node_ptr_ptr)

    def visit_maybe_relaxed_sub(self, node, shift, size, idx):
        relaxed = self.relaxed(node)
        if relaxed:
            return Relaxed(node, shift, relaxed, self).towards(idx)
        else:
            return RegularSub(node, shift, size, self).towards(idx)

    def visit_leaf(self, pos, idx):
        return (self.leaf(pos.node), pos.index(idx), pos.count())

    # pos = node, idx = full, offset = shifted & masked, count = shifted
    def towards_regular(self, pos, idx, offset, count):
        is_leaf = pos.shift == self.BL
        child = self.inner(pos.node)[offset]
        is_full = offset + 1 != count
        if is_full:
            if is_leaf:
                return self.visit_leaf(FullLeaf(child), idx)
            else:
                return Full(child, pos.shift - self.B, self).towards(idx)
        elif is_leaf:
            return self.visit_leaf(Leaf(child, pos.size), idx)
        else:
            return Regular(child, pos.shift - self.B, pos.size, self).towards(idx)

    def leaf_mask(self):
        return (1 << self.BL) - 1


def popcount(x):
    b = 0
    while x > 0:
        x &= x - 1
        b += 1
    return b


class ChampIter:
    def __init__(self, val):
        self.depth = 0
        self.count = 0
        v = val['impl_']['root']
        self.node_ptr_ptr = v.type.pointer()
        m = self.datamap(v)
        if m:
            self.cur = self.values(v)
            self.end = self.values(v) + popcount(m)
        else:
            self.cur = None
            self.end = None
        self.path = [v.address]
        self.B = v.type.target().template_argument(4)
        self.MAX_DEPTH = ((8 * 8) + self.B - 1) / 8
        self.ensure_valid()

    def __iter__(self):
        return self

    def __next__(self):
        if self.cur == None:
            raise StopIteration
        result = self.cur.dereference()
        self.cur += 1
        self.count += 1
        self.ensure_valid()
        return result

    def ensure_valid(self):
        while self.cur == self.end:
            while self.step_down():
                if self.cur != self.end:
                    return
            if not self.step_right():
                self.cur = None
                self.end = None
                return

    def step_down(self):
        if self.depth < self.MAX_DEPTH:
            parent = self.path[self.depth].dereference()
            if self.nodemap(parent):
                self.depth += 1
                self.path.append(self.children(parent))
                child = self.path[self.depth]
                if self.depth < self.MAX_DEPTH:
                    m = self.datamap(child)
                    if m:
                        self.cur = self.values(child)
                        self.end = self.cur + popcount(m)
                else:
                    self.cur = self.collisions(child)
                    self.end = self.cur = self.collision_count(child)
                return True
        return False

    def step_right(self):
        while self.depth > 0:
            parent = self.path[self.depth - 1].dereference()
            last = self.children(parent) + popcount(self.nodemap(parent))
            next_ = self.path[self.depth] + 1
            if next_ < last:
                self.path[self.depth] = next_
                child = self.path[self.depth].dereference()
                if self.depth < self.MAX_DEPTH:
                    m = self.datamap(child)
                    if m:
                        self.cur = self.values(child)
                        self.end = self.cur + popcount(m)
                else:
                    self.cur = self.collisions(child)
                    self.end = self.cur + self.collision_count(child)
                return True
            self.depth -= 1
            self.path.pop()
        return False

    def values(self, node):
        return node.dereference()['impl']['d']['data']['inner']['values'].dereference(
        )['d']['buffer'].address.cast(self.T_ptr)

    def children(self, node):
        return node.dereference()['impl']['d']['data']['inner']['buffer'].address.cast(
            self.node_ptr_ptr)

    def datamap(self, node):
        return node.dereference()['impl']['d']['data']['inner']['datamap']

    def nodemap(self, node):
        return node.dereference()['impl']['d']['data']['inner']['nodemap']

    def collision_count(self, node):
        return node.dereference()['impl']['d']['data']['collision']['count']

    def collisions(self, node):
        return node.dereference()['impl']['d']['data']['collision']['buffer'].address.cast(
            self.T_ptr)


class MapIter(ChampIter):
    def __init__(self, val):
        self.T_ptr = gdb.lookup_type("std::pair<" + val.type.template_argument(0).name + ", " +
                                     val.type.template_argument(1).name + ">").pointer()
        ChampIter.__init__(self, val)
        self.pair = None

    def __next__(self):
        if self.pair:
            result = ('[%d]' % self.count, self.pair['second'])
            self.pair = None
            return result
        self.pair = super().__next__()
        return ('[%d]' % self.count, self.pair['first'])


class SetIter(ChampIter):
    def __init__(self, val):
        self.T_ptr = gdb.lookup_type(val.type.template_argument(0).name).pointer()
        ChampIter.__init__(self, val)

    def __next__(self):
        return ('[%d]' % self.count, super().__next__())


def num_elements(num):
    return '1 element' if num == 1 else '%d elements' % num


class ArrayPrinter:
    "Prints an immer::array"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return 'immer::array with %s' % num_elements(self.val['impl_']['size'])

    def children(self):
        return ArrayIter(self.val)

    def display_hint(self):
        return 'array'


class MapPrinter:
    "Print an immer::map"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return 'immer::map with %s' % num_elements(self.val['impl_']['size'])

    def children(self):
        return MapIter(self.val)

    def display_hint(self):
        return 'map'


class SetPrinter:
    "Prints an immer::set"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return 'immer::set with %s' % num_elements(self.val['impl_']['size'])

    def children(self):
        return SetIter(self.val)


class TablePrinter:
    "Prints an immer::table"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return 'immer::table with %s' % num_elements(self.val['impl_']['size'])

    def children(self):
        return SetIter(self.val)


class ListPrinter:
    "Prints an immer::vector or immer::flex_vector"

    def __init__(self, val, typename):
        self.val = val
        self.typename = typename

    def to_string(self):
        return '%s of length %d' % (self.typename, int(self.val['impl_']['size']))

    def children(self):
        return ListIter(self.val)

    def display_hint(self):
        return 'array'


def immer_lookup_function(val):
    compiled_rx = re.compile('^([a-zA-Z0-9_:]+)(<.*>)?$')
    typename = gdb.types.get_basic_type(val.type).tag
    if not typename:
        return None
    match = compiled_rx.match(typename)
    if not match:
        return None

    basename = match.group(1)
    if basename == "immer::array":
        return ArrayPrinter(val)
    elif basename == "immer::map":
        return MapPrinter(val)
    elif basename == "immer::set":
        return SetPrinter(val)
    elif basename == "immer::table":
        return TablePrinter(val)
    elif basename == "immer::vector":
        return ListPrinter(val, "immer::vector")
    elif basename == "immer::flex_vector":
        return ListPrinter(val, "immer::flex_vector")
    return None
