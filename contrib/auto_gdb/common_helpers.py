#!/usr/bin/python
#

try:
    import gdb
except ImportError as e:
    raise ImportError("This script must be run in GDB: ", str(e))
import sys
import os
sys.path.append(os.getcwd())
import stl_containers
import simple_class_obj

SIZE_OF_INT = 4
SIZE_OF_BOOL = 1
SIZE_OF_INT64 = 8
SIZE_OF_UINT256 = 32


def get_special_type_obj(gobj):
    obj_type = gobj.type.strip_typedefs()
    if stl_containers.VectorObj.is_this_type(obj_type):
        return stl_containers.VectorObj(gobj)
    if stl_containers.ListObj.is_this_type(obj_type):
        return stl_containers.ListObj(gobj)
    if stl_containers.PairObj.is_this_type(obj_type):
        return stl_containers.PairObj(gobj)
    if stl_containers.MapObj.is_this_type(obj_type):
        return stl_containers.MapObj(gobj)
    if stl_containers.SetObj.is_this_type(obj_type):
        return stl_containers.SetObj(gobj)
    if simple_class_obj.SimpleClassObj.is_this_type(obj_type):
        return simple_class_obj.SimpleClassObj(gobj)
    return False


def is_special_type(obj_type):
    if stl_containers.VectorObj.is_this_type(obj_type):
        return True
    if stl_containers.ListObj.is_this_type(obj_type):
        return True
    if stl_containers.PairObj.is_this_type(obj_type):
        return True
    if stl_containers.MapObj.is_this_type(obj_type):
        return True
    if stl_containers.SetObj.is_this_type(obj_type):
        return True
    if simple_class_obj.SimpleClassObj.is_this_type(obj_type):
        return True
    return False


def get_instance_size(gobj):
    obj = get_special_type_obj(gobj)
    if not obj:
        return gobj.type.sizeof
    return obj.get_used_size()
