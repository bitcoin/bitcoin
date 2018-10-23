#!/usr/bin/python
#

try:
    import gdb
except ImportError as e:
    raise ImportError("This script must be run in GDB: ", str(e))
import traceback
import sys
import os
sys.path.append(os.getcwd())
import common_helpers


class UsedSizeCommand (gdb.Command):
    """calc size of the memory used by the object"""

    def __init__ (self):
        super (UsedSizeCommand, self).__init__ ("usedsize", gdb.COMMAND_USER)

    @classmethod
    def assign_value(cls, obj_name, value):
        gdb.execute("set " + obj_name + " = " + str(value))

    @classmethod
    def get_type(cls, obj_name):
        return gdb.parse_and_eval(obj_name).type

    def invoke(self, arg, from_tty):
        try:
            args = gdb.string_to_argv(arg)
            obj = gdb.parse_and_eval(args[1])
            obj_type = obj.type
            print (args[1] + " is " + str(obj_type))
            size = common_helpers.get_instance_size(obj)
            UsedSizeCommand.assign_value(args[0], size)
            print (size)

        except Exception as e:
            print(traceback.format_exc())
            raise e

UsedSizeCommand()

