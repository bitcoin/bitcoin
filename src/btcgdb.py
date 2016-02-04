import gdb
import gdb.types

VERSION = "0.11.2-1"

def dumpCBlockIndex(bindex, length, indent = 0):
    count = 0
    length = int(length)
    while bindex != 0 and count < length:
      count+=1
      obj = bindex.dereference()
      gdb.write(" "*indent + str(bindex) + ": height:" + str(obj["nHeight"]) + " tx:" + str(obj["nTx"]) + " status:" + "0x%x" % obj["nStatus"] + " (" + int2BlockStatus(obj["nStatus"]) + ")" + "\n")
      bindex = obj["pprev"]

class SpHelp(gdb.Command):
    def __init__(self,cmds):
        super(SpHelp, self).__init__("btc-help", gdb.COMMAND_DATA, gdb.COMPLETE_SYMBOL)
        self.cmds = cmds

    def help(self):
      return "btc-help: This help"

    def invoke(self, argument, from_tty):
        args = gdb.string_to_argv(argument)
        if len(args)>0:
          expr = args[0]
          arg = gdb.parse_and_eval(expr)
          # todo find this command and print help on only it
        for c in self.cmds:
          gdb.write(c.help() + "\n")
        
int2BlockStatusList = ( (gdb.parse_and_eval("BLOCK_VALID_HEADER") , "BLOCK_VALID_HEADER"),
(gdb.parse_and_eval("BLOCK_VALID_TREE") , "BLOCK_VALID_TREE "),
(gdb.parse_and_eval("BLOCK_VALID_CHAIN") , "BLOCK_VALID_CHAIN"),
(gdb.parse_and_eval("BLOCK_VALID_SCRIPTS") , "BLOCK_VALID_SCRIPTS"),
(gdb.parse_and_eval("BLOCK_HAVE_DATA") , "BLOCK_HAVE_DATA"),
(gdb.parse_and_eval("BLOCK_HAVE_UNDO") , "BLOCK_HAVE_UNDO"),
(gdb.parse_and_eval("BLOCK_EXCESSIVE") , "BLOCK_EXCESSIVE"),
(gdb.parse_and_eval("BLOCK_FAILED_VALID") , "BLOCK_FAILED_VALID"),
(gdb.parse_and_eval("BLOCK_FAILED_CHILD") , "BLOCK_FAILED_CHILD") )

def int2BlockStatus(x):
  ret = []
  for flag,val in int2BlockStatusList:
    if x&flag: ret.append(val)
  return " | ".join(ret)
 
  
class BuDumpCBlockIndex(gdb.Command):
    def __init__(self):
        super(BuDumpCBlockIndex, self).__init__("btc-dump-bidx", gdb.COMMAND_DATA, gdb.COMPLETE_SYMBOL)

    def help(self):
      return "btc-dump-bidx CBlockIndex* count: Dump the CBlockIndex chain count elements deep"
    def invoke(self, argument, from_tty):
        args = gdb.string_to_argv(argument)
        if len(args)!=2:
          gdb.write("args:\n  CBlockIndex*: pointer to the chain\n  number: how far to follow the chain\nexample: btc-dump-bidx pindex 10\n")
          return
        ptr = gdb.parse_and_eval(args[0])
        count = gdb.parse_and_eval(args[1])
        dumpCBlockIndex(ptr,count)
        
        

sd = BuDumpCBlockIndex()

SpHelp([sd])

gdb.write("Loaded Bitcoin Unlimited GDB extensions %s.\nRun 'btc-help' for command help\n" % VERSION)
