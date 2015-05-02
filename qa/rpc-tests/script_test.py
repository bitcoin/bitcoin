#!/usr/bin/env python2
#
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

'''
Test notes:
This test uses the script_valid and script_invalid tests from the unittest
framework to do end-to-end testing where we compare that two nodes agree on
whether blocks containing a given test script are valid.

We generally ignore the script flags associated with each test (since we lack
the precision to test each script using those flags in this framework), but
for tests with SCRIPT_VERIFY_P2SH, we can use a block time after the BIP16 
switchover date to try to test with that flag enabled (and for tests without
that flag, we use a block time before the switchover date).

NOTE: This test is very slow and may take more than 40 minutes to run.
'''

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.comptool import TestInstance, TestManager
from test_framework.mininode import *
from test_framework.blocktools import *
from test_framework.script import *
import logging
import copy
import json

script_valid_file   = "../../src/test/data/script_valid.json"
script_invalid_file = "../../src/test/data/script_invalid.json"

# Pass in a set of json files to open. 
class ScriptTestFile(object):

    def __init__(self, files):
        self.files = files
        self.index = -1
        self.data = []

    def load_files(self):
        for f in self.files:
            self.data.extend(json.loads(open(os.path.dirname(os.path.abspath(__file__))+"/"+f).read()))

    # Skip over records that are not long enough to be tests
    def get_records(self):
        while (self.index < len(self.data)):
            if len(self.data[self.index]) >= 3:
                yield self.data[self.index]
            self.index += 1


# Helper for parsing the flags specified in the .json files
SCRIPT_VERIFY_NONE = 0
SCRIPT_VERIFY_P2SH = 1 
SCRIPT_VERIFY_STRICTENC = 1 << 1
SCRIPT_VERIFY_DERSIG = 1 << 2
SCRIPT_VERIFY_LOW_S = 1 << 3
SCRIPT_VERIFY_NULLDUMMY = 1 << 4
SCRIPT_VERIFY_SIGPUSHONLY = 1 << 5
SCRIPT_VERIFY_MINIMALDATA = 1 << 6
SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS = 1 << 7
SCRIPT_VERIFY_CLEANSTACK = 1 << 8

flag_map = { 
    "": SCRIPT_VERIFY_NONE,
    "NONE": SCRIPT_VERIFY_NONE, 
    "P2SH": SCRIPT_VERIFY_P2SH,
    "STRICTENC": SCRIPT_VERIFY_STRICTENC,
    "DERSIG": SCRIPT_VERIFY_DERSIG,
    "LOW_S": SCRIPT_VERIFY_LOW_S,
    "NULLDUMMY": SCRIPT_VERIFY_NULLDUMMY,
    "SIGPUSHONLY": SCRIPT_VERIFY_SIGPUSHONLY,
    "MINIMALDATA": SCRIPT_VERIFY_MINIMALDATA,
    "DISCOURAGE_UPGRADABLE_NOPS": SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS,
    "CLEANSTACK": SCRIPT_VERIFY_CLEANSTACK,
}

def ParseScriptFlags(flag_string):
    flags = 0
    for x in flag_string.split(","):
        if x in flag_map:
            flags |= flag_map[x]
        else:
            print "Error: unrecognized script flag: ", x
    return flags

'''
Given a string that is a scriptsig or scriptpubkey from the .json files above,
convert it to a CScript()
'''
# Replicates behavior from core_read.cpp
def ParseScript(json_script):
    script = json_script.split(" ")
    parsed_script = CScript()
    for x in script:
        if len(x) == 0:
            # Empty string, ignore.
            pass
        elif x.isdigit() or (len(x) >= 1 and x[0] == "-" and x[1:].isdigit()):
            # Number
            n = int(x, 0)
            if (n == -1) or (n >= 1 and n <= 16):
                parsed_script = CScript(bytes(parsed_script) + bytes(CScript([n])))
            else:
                parsed_script += CScriptNum(int(x, 0))
        elif x.startswith("0x"):
            # Raw hex data, inserted NOT pushed onto stack:
            for i in xrange(2, len(x), 2):
                parsed_script = CScript(bytes(parsed_script) + bytes(chr(int(x[i:i+2],16))))
        elif x.startswith("'") and x.endswith("'") and len(x) >= 2:
            # Single-quoted string, pushed as data.
            parsed_script += CScript([x[1:-1]])
        else:
            # opcode, e.g. OP_ADD or ADD:
            tryopname = "OP_" + x
            if tryopname in OPCODES_BY_NAME:
                parsed_script += CScriptOp(OPCODES_BY_NAME["OP_" + x])
            else:
                print "ParseScript: error parsing '%s'" % x
                return ""
    return parsed_script
            
class TestBuilder(object):
    def create_credit_tx(self, scriptPubKey):
        # self.tx1 is a coinbase transaction, modeled after the one created by script_tests.cpp
        # This allows us to reuse signatures created in the unit test framework.
        self.tx1 = create_coinbase()                 # this has a bip34 scriptsig,
        self.tx1.vin[0].scriptSig = CScript([0, 0])  # but this matches the unit tests
        self.tx1.vout[0].nValue = 0
        self.tx1.vout[0].scriptPubKey = scriptPubKey
        self.tx1.rehash()
    def create_spend_tx(self, scriptSig):
        self.tx2 = create_transaction(self.tx1, 0, CScript(), 0)
        self.tx2.vin[0].scriptSig = scriptSig
        self.tx2.vout[0].scriptPubKey = CScript()
        self.tx2.rehash()
    def rehash(self):
        self.tx1.rehash()
        self.tx2.rehash()

# This test uses the (default) two nodes provided by ComparisonTestFramework,
# specified on the command line with --testbinary and --refbinary.
# See comptool.py
class ScriptTest(ComparisonTestFramework):

    def run_test(self):
        # Set up the comparison tool TestManager
        test = TestManager(self, self.options.tmpdir)
        test.add_all_connections(self.nodes)

        # Load scripts
        self.scripts = ScriptTestFile([script_valid_file, script_invalid_file])
        self.scripts.load_files()

        # Some variables we re-use between test instances (to build blocks)
        self.tip = None
        self.block_time = None

        NetworkThread().start()  # Start up network handling in another thread
        test.run()

    def generate_test_instance(self, pubkeystring, scriptsigstring):
        scriptpubkey = ParseScript(pubkeystring)
        scriptsig = ParseScript(scriptsigstring)

        test = TestInstance(sync_every_block=False)
        test_build = TestBuilder()
        test_build.create_credit_tx(scriptpubkey)
        test_build.create_spend_tx(scriptsig)
        test_build.rehash()

        block = create_block(self.tip, test_build.tx1, self.block_time)
        self.block_time += 1
        block.solve()
        self.tip = block.sha256
        test.blocks_and_transactions = [[block, True]]

        for i in xrange(100):
            block = create_block(self.tip, create_coinbase(), self.block_time)
            self.block_time += 1
            block.solve()
            self.tip = block.sha256
            test.blocks_and_transactions.append([block, True])

        block = create_block(self.tip, create_coinbase(), self.block_time)
        self.block_time += 1
        block.vtx.append(test_build.tx2)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        test.blocks_and_transactions.append([block, None])
        return test   

    # This generates the tests for TestManager.
    def get_tests(self):
        self.tip = int ("0x" + self.nodes[0].getbestblockhash() + "L", 0)
        self.block_time = 1333230000  # before the BIP16 switchover

        '''
        Create a new block with an anyone-can-spend coinbase
        '''
        block = create_block(self.tip, create_coinbase(), self.block_time)
        self.block_time += 1
        block.solve()
        self.tip = block.sha256
        yield TestInstance(objects=[[block, True]])

        '''
        Build out to 100 blocks total, maturing the coinbase.
        '''
        test = TestInstance(objects=[], sync_every_block=False, sync_every_tx=False)
        for i in xrange(100):
            b = create_block(self.tip, create_coinbase(), self.block_time)
            b.solve()
            test.blocks_and_transactions.append([b, True])
            self.tip = b.sha256
            self.block_time += 1
        yield test
 
        ''' Iterate through script tests. '''
        counter = 0
        for script_test in self.scripts.get_records():
            ''' Reset the blockchain to genesis block + 100 blocks. '''
            if self.nodes[0].getblockcount() > 101:
                self.nodes[0].invalidateblock(self.nodes[0].getblockhash(102))
                self.nodes[1].invalidateblock(self.nodes[1].getblockhash(102))

            self.tip = int ("0x" + self.nodes[0].getbestblockhash() + "L", 0)

            [scriptsig, scriptpubkey, flags] = script_test[0:3]
            flags = ParseScriptFlags(flags)

            # We can use block time to determine whether the nodes should be
            # enforcing BIP16.
            #
            # We intentionally let the block time grow by 1 each time.
            # This forces the block hashes to differ between tests, so that
            # a call to invalidateblock doesn't interfere with a later test.
            if (flags & SCRIPT_VERIFY_P2SH):
                self.block_time = 1333238400 + counter # Advance to enforcing BIP16
            else:
                self.block_time = 1333230000 + counter # Before the BIP16 switchover

            print "Script test: [%s]" % script_test

            yield self.generate_test_instance(scriptpubkey, scriptsig)
            counter += 1

if __name__ == '__main__':
    ScriptTest().main()
