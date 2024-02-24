#!/usr/bin/env python3
# Copyright (c) 2018-2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

try:
    import gdb
except ImportError as e:
    raise ImportError("This script must be run in GDB: ", str(e))
import traceback
import datetime
import sys
import os
import common_helpers
sys.path.append(os.getcwd())


class LogSizeCommand (gdb.Command):
    """calc size of the memory used by the object and write it to file"""

    def __init__ (self):
        super (LogSizeCommand, self).__init__ ("logsize", gdb.COMMAND_USER)

    def invoke(self, arg, _from_tty):
        try:
            args = gdb.string_to_argv(arg)
            obj = gdb.parse_and_eval(args[0])
            logfile = open(args[1], 'a', encoding="utf8")
            size = common_helpers.get_instance_size(obj)
            logfile.write("%s %s: %d\n" % (str(datetime.datetime.now()), args[0], size))
            logfile.close()
        except Exception as e:
            print(traceback.format_exc())
            raise e

LogSizeCommand()
