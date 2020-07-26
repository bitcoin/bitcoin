#!/usr/bin/env python3
#

try:
    import gdb
except ImportError as e:
    raise ImportError("This script must be run in GDB: ", str(e))
import sys
import os
import common_helpers
sys.path.append(os.getcwd())


def find_type(orig, name):
    typ = orig.strip_typedefs()
    while True:
        # Strip cv qualifiers
        search = '%s::%s' % (typ.unqualified(), name)
        try:
            return gdb.lookup_type(search)
        except RuntimeError:
            pass
        # type is not found,  try superclass search
        field = typ.fields()[0]
        if not field.is_base_class:
            raise ValueError("Cannot find type %s::%s" % (str(orig), name))
        typ = field.type


def get_value_from_aligned_membuf(buf, valtype):
    """Returns the value held in a __gnu_cxx::__aligned_membuf."""
    return buf['_M_storage'].address.cast(valtype.pointer()).dereference()


def get_value_from_node(node):
    valtype = node.type.template_argument(0)
    return get_value_from_aligned_membuf(node['_M_storage'], valtype)


class VectorObj:

    def __init__ (self, gobj):
        self.obj = gobj

    @classmethod
    def is_this_type(cls, obj_type):
        type_name = str(obj_type)
        if type_name.find("std::vector<") == 0:
            return True
        if type_name.find("std::__cxx11::vector<") == 0:
            return True
        return False

    def element_type(self):
        return self.obj.type.template_argument(0)

    def size(self):
        return int(self.obj['_M_impl']['_M_finish'] -
                   self.obj['_M_impl']['_M_start'])

    def get_used_size(self):
        if common_helpers.is_special_type(self.element_type()):
            size = self.obj.type.sizeof
            item = self.obj['_M_impl']['_M_start']
            finish = self.obj['_M_impl']['_M_finish']
            while item != finish:
                elem = item.dereference()
                obj = common_helpers.get_special_type_obj(elem)
                size += obj.get_used_size()
                item = item + 1
            return size
        return self.obj.type.sizeof + self.size() * self.element_type().sizeof


class ListObj:

    def __init__ (self, gobj):
        self.obj = gobj

    @classmethod
    def is_this_type(cls, obj_type):
        type_name = str(obj_type)
        if type_name.find("std::list<") == 0:
            return True
        if type_name.find("std::__cxx11::list<") == 0:
            return True
        return False

    def element_type(self):
        return self.obj.type.template_argument(0)

    def get_used_size(self):
        is_special = common_helpers.is_special_type(self.element_type())
        head = self.obj['_M_impl']['_M_node']
#        nodetype = find_type(self.obj.type, '_Node')
        nodetype = head.type
        nodetype = nodetype.strip_typedefs().pointer()
        current = head['_M_next']
        size = self.obj.type.sizeof
        while current != head.address:
            if is_special:
                elem = current.cast(nodetype).dereference()
                size += common_helpers.get_instance_size(elem)
            else:
                size += self.element_type().sizeof
            current = current['_M_next']

        return size


class PairObj:

    def __init__ (self, gobj):
        self.obj = gobj

    @classmethod
    def is_this_type(cls, obj_type):
        type_name = str(obj_type)
        if type_name.find("std::pair<") == 0:
            return True
        if type_name.find("std::__cxx11::pair<") == 0:
            return True
        return False

    def key_type(self):
        return self.obj.type.template_argument(0)

    def value_type(self):
        return self.obj.type.template_argument(1)

    def get_used_size(self):
        if not common_helpers.is_special_type(self.key_type()) and not common_helpers.is_special_type(self.value_type()):
            return self.key_type().sizeof + self.value_type().sizeof

        size = 0

        if common_helpers.is_special_type(self.key_type()):
            obj = common_helpers.get_special_type_obj(self.obj['first'])
            size += obj.get_used_size()
        else:
            size += self.key_type().sizeof

        if common_helpers.is_special_type(self.value_type()):
            obj = common_helpers.get_special_type_obj(self.obj['second'])
            size += obj.get_used_size()
        else:
            size += self.value_type().sizeof

        return size


class MapObj:

    def __init__ (self, gobj):
        self.obj = gobj
        self.obj_type = gobj.type
        rep_type = find_type(self.obj_type, "_Rep_type")
        self.node_type = find_type(rep_type, "_Link_type")
        self.node_type = self.node_type.strip_typedefs()

    @classmethod
    def is_this_type(cls, obj_type):
        type_name = str(obj_type)
        if type_name.find("std::map<") == 0:
            return True
        if type_name.find("std::__cxx11::map<") == 0:
            return True
        return False

    def key_type(self):
        return self.obj_type.template_argument(0).strip_typedefs()

    def value_type(self):
        return self.obj_type.template_argument(1).strip_typedefs()

    def size(self):
        res = int(self.obj['_M_t']['_M_impl']['_M_node_count'])
        return res

    def get_used_size(self):
        if not common_helpers.is_special_type(self.key_type()) and not common_helpers.is_special_type(self.value_type()):
            return self.obj_type.sizeof + self.size() * (self.key_type().sizeof + self.value_type().sizeof)
        if self.size() == 0:
            return self.obj_type.sizeof
        size = self.obj_type.sizeof
        row_node = self.obj['_M_t']['_M_impl']['_M_header']['_M_left']
        for i in range(self.size()):
            node_val = row_node.cast(self.node_type).dereference()
            pair = get_value_from_node(node_val)

            obj = common_helpers.get_special_type_obj(pair)
            size += obj.get_used_size()

            node = row_node
            if node.dereference()['_M_right']:
                node = node.dereference()['_M_right']
                while node.dereference()['_M_left']:
                    node = node.dereference()['_M_left']
            else:
                parent = node.dereference()['_M_parent']
                while node == parent.dereference()['_M_right']:
                    node = parent
                    parent = parent.dereference()['_M_parent']
                if node.dereference()['_M_right'] != parent:
                    node = parent
            row_node = node
        return size


class SetObj:

    def __init__ (self, gobj):
        self.obj = gobj
        self.obj_type = gobj.type
        rep_type = find_type(self.obj_type, "_Rep_type")
        self.node_type = find_type(rep_type, "_Link_type")
        self.node_type = self.node_type.strip_typedefs()

    @classmethod
    def is_this_type(cls, obj_type):
        type_name = str(obj_type)
        if type_name.find("std::set<") == 0:
            return True
        if type_name.find("std::__cxx11::set<") == 0:
            return True
        return False

    def element_type(self):
        return self.obj_type.template_argument(0)

    def size(self):
        res = int(self.obj['_M_t']['_M_impl']['_M_node_count'])
        return res

    def get_used_size(self):
        if not common_helpers.is_special_type(self.element_type()):
            return self.obj_type.sizeof + self.size() * self.element_type().sizeof
        if self.size() == 0:
            return self.obj_type.sizeof
        size = self.obj_type.sizeof
        row_node = self.obj['_M_t']['_M_impl']['_M_header']['_M_left']
        for i in range(self.size()):
            node_val = row_node.cast(self.node_type).dereference()
            val = get_value_from_node(node_val)

            obj = common_helpers.get_special_type_obj(val)
            size += obj.get_used_size()

            node = row_node
            if node.dereference()['_M_right']:
                node = node.dereference()['_M_right']
                while node.dereference()['_M_left']:
                    node = node.dereference()['_M_left']
            else:
                parent = node.dereference()['_M_parent']
                while node == parent.dereference()['_M_right']:
                    node = parent
                    parent = parent.dereference()['_M_parent']
                if node.dereference()['_M_right'] != parent:
                    node = parent
            row_node = node
        return size


