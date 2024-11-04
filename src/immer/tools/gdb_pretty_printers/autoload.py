import gdb.printing
import os
import sys
import inspect

filename = inspect.getframeinfo(inspect.currentframe()).filename
path = os.path.dirname(os.path.abspath(filename))
if not path in sys.path:
    sys.path.append(path)
from printers import immer_lookup_function

gdb.printing.register_pretty_printer(gdb.current_objfile(), immer_lookup_function)

print("immer gdb pretty-printers loaded")
