Regression tests
================

### [test_framework/authproxy.py](test_framework/authproxy.py)
Taken from the [python-bitcoinrpc repository](https://github.com/jgarzik/python-bitcoinrpc).

### [test_framework/test_framework.py](test_framework/test_framework.py)
Base class for new regression tests.

### [test_framework/util.py](test_framework/util.py)
Generally useful functions.

### [test_framework/mininode.py](test_framework/mininode.py)
Basic code to support p2p connectivity to a bitcoind.

### [test_framework/comptool.py](test_framework/comptool.py)
Framework for comparison-tool style, p2p tests.

### [test_framework/script.py](test_framework/script.py)
Utilities for manipulating transaction scripts (originally from python-bitcoinlib)

### [test_framework/blockstore.py](test_framework/blockstore.py)
Implements disk-backed block and tx storage.

### [test_framework/key.py](test_framework/key.py)
Wrapper around OpenSSL EC_Key (originally from python-bitcoinlib)

### [test_framework/bignum.py](test_framework/bignum.py)
Helpers for script.py

### [test_framework/blocktools.py](test_framework/blocktools.py)
Helper functions for creating blocks and transactions.

P2P test design notes
---------------------

## Mininode

* ```mininode.py``` contains all the definitions for objects that pass
over the network (```CBlock```, ```CTransaction```, etc, along with the network-level
wrappers for them, ```msg_block```, ```msg_tx```, etc).

* P2P tests have two threads.  One thread handles all network communication
with the bitcoind(s) being tested (using python's asyncore package); the other
implements the test logic.

* ```NodeConn``` is the class used to connect to a bitcoind.  If you implement
a callback class that derives from ```NodeConnCB``` and pass that to the
```NodeConn``` object, your code will receive the appropriate callbacks when
events of interest arrive.

* You can pass the same handler to multiple ```NodeConn```'s if you like, or pass
different ones to each -- whatever makes the most sense for your test.

* Call ```NetworkThread.start()``` after all ```NodeConn``` objects are created to
start the networking thread.  (Continue with the test logic in your existing
thread.)

* RPC calls are available in p2p tests.

* Can be used to write free-form tests, where specific p2p-protocol behavior
is tested.  Examples: ```p2p-accept-block.py```, ```p2p-compactblocks.py```.

## Comptool

* Testing framework for writing tests that compare the block/tx acceptance
behavior of a bitcoind against 1 or more other bitcoind instances, or against
known outcomes, or both.

* Set the ```num_nodes``` variable (defined in ```ComparisonTestFramework```) to start up
1 or more nodes.  If using 1 node, then ```--testbinary``` can be used as a command line
option to change the bitcoind binary used by the test.  If using 2 or more nodes,
then ```--refbinary``` can be optionally used to change the bitcoind that will be used
on nodes 2 and up.

* Implement a (generator) function called ```get_tests()``` which yields ```TestInstance```s.
Each ```TestInstance``` consists of:
  - a list of ```[object, outcome, hash]``` entries
    * ```object``` is a ```CBlock```, ```CTransaction```, or
    ```CBlockHeader```.  ```CBlock```'s and ```CTransaction```'s are tested for
    acceptance.  ```CBlockHeader```s can be used so that the test runner can deliver
    complete headers-chains when requested from the bitcoind, to allow writing
    tests where blocks can be delivered out of order but still processed by
    headers-first bitcoind's.
    * ```outcome``` is ```True```, ```False```, or ```None```.  If ```True```
    or ```False```, the tip is compared with the expected tip -- either the
    block passed in, or the hash specified as the optional 3rd entry.  If
    ```None``` is specified, then the test will compare all the bitcoind's
    being tested to see if they all agree on what the best tip is.
    * ```hash``` is the block hash of the tip to compare against. Optional to
    specify; if left out then the hash of the block passed in will be used as
    the expected tip.  This allows for specifying an expected tip while testing
    the handling of either invalid blocks or blocks delivered out of order,
    which complete a longer chain.
  - ```sync_every_block```: ```True/False```.  If ```False```, then all blocks
    are inv'ed together, and the test runner waits until the node receives the
    last one, and tests only the last block for tip acceptance using the
    outcome and specified tip.  If ```True```, then each block is tested in
    sequence and synced (this is slower when processing many blocks).
  - ```sync_every_transaction```: ```True/False```.  Analogous to
    ```sync_every_block```, except if the outcome on the last tx is "None",
    then the contents of the entire mempool are compared across all bitcoind
    connections.  If ```True``` or ```False```, then only the last tx's
    acceptance is tested against the given outcome.

* For examples of tests written in this framework, see
  ```invalidblockrequest.py``` and ```p2p-fullblocktest.py```.

