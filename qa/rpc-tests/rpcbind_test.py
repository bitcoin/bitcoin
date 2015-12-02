#!/usr/bin/env python2
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Test for -rpcbind, as well as -rpcallowip and -rpcconnect

# TODO extend this test from the test framework (like all other tests)

import tempfile
import traceback

from test_framework.util import *
from test_framework.netutil import *

def run_bind_test(tmpdir, allow_ips, connect_to, addresses, expected):
    '''
    Start a node with requested rpcallowip and rpcbind parameters,
    then try to connect, and check if the set of bound addresses
    matches the expected set.
    '''
    expected = [(addr_to_hex(addr), port) for (addr, port) in expected]
    base_args = ['-disablewallet', '-nolisten']
    if allow_ips:
        base_args += ['-rpcallowip=' + x for x in allow_ips]
    binds = ['-rpcbind='+addr for addr in addresses]
    nodes = start_nodes(1, tmpdir, [base_args + binds], connect_to)
    try:
        pid = bitcoind_processes[0].pid
        assert_equal(set(get_bind_addrs(pid)), set(expected))
    finally:
        stop_nodes(nodes)
        wait_bitcoinds()

def run_allowip_test(tmpdir, allow_ips, rpchost, rpcport):
    '''
    Start a node with rpcwallow IP, and request getinfo
    at a non-localhost IP.
    '''
    base_args = ['-disablewallet', '-nolisten'] + ['-rpcallowip='+x for x in allow_ips]
    nodes = start_nodes(1, tmpdir, [base_args])
    try:
        # connect to node through non-loopback interface
        url = "http://rt:rt@%s:%d" % (rpchost, rpcport,)
        node = get_rpc_proxy(url, 1)
        node.getinfo()
    finally:
        node = None # make sure connection will be garbage collected and closed
        stop_nodes(nodes)
        wait_bitcoinds()


def run_test(tmpdir):
    assert(sys.platform == 'linux2') # due to OS-specific network stats queries, this test works only on Linux
    # find the first non-loopback interface for testing
    non_loopback_ip = None
    for name,ip in all_interfaces():
        if ip != '127.0.0.1':
            non_loopback_ip = ip
            break
    if non_loopback_ip is None:
        assert(not 'This test requires at least one non-loopback IPv4 interface')
    print("Using interface %s for testing" % non_loopback_ip)

    defaultport = rpc_port(0)

    # check default without rpcallowip (IPv4 and IPv6 localhost)
    run_bind_test(tmpdir, None, '127.0.0.1', [],
        [('127.0.0.1', defaultport), ('::1', defaultport)])
    # check default with rpcallowip (IPv6 any)
    run_bind_test(tmpdir, ['127.0.0.1'], '127.0.0.1', [],
        [('::0', defaultport)])
    # check only IPv4 localhost (explicit)
    run_bind_test(tmpdir, ['127.0.0.1'], '127.0.0.1', ['127.0.0.1'],
        [('127.0.0.1', defaultport)])
    # check only IPv4 localhost (explicit) with alternative port
    run_bind_test(tmpdir, ['127.0.0.1'], '127.0.0.1:32171', ['127.0.0.1:32171'],
        [('127.0.0.1', 32171)])
    # check only IPv4 localhost (explicit) with multiple alternative ports on same host
    run_bind_test(tmpdir, ['127.0.0.1'], '127.0.0.1:32171', ['127.0.0.1:32171', '127.0.0.1:32172'],
        [('127.0.0.1', 32171), ('127.0.0.1', 32172)])
    # check only IPv6 localhost (explicit)
    run_bind_test(tmpdir, ['[::1]'], '[::1]', ['[::1]'],
        [('::1', defaultport)])
    # check both IPv4 and IPv6 localhost (explicit)
    run_bind_test(tmpdir, ['127.0.0.1'], '127.0.0.1', ['127.0.0.1', '[::1]'],
        [('127.0.0.1', defaultport), ('::1', defaultport)])
    # check only non-loopback interface
    run_bind_test(tmpdir, [non_loopback_ip], non_loopback_ip, [non_loopback_ip],
        [(non_loopback_ip, defaultport)])

    # Check that with invalid rpcallowip, we are denied
    run_allowip_test(tmpdir, [non_loopback_ip], non_loopback_ip, defaultport)
    try:
        run_allowip_test(tmpdir, ['1.1.1.1'], non_loopback_ip, defaultport)
        assert(not 'Connection not denied by rpcallowip as expected')
    except ValueError:
        pass

def main():
    import optparse

    parser = optparse.OptionParser(usage="%prog [options]")
    parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true",
                      help="Leave bitcoinds and test.* datadir on exit or error")
    parser.add_option("--srcdir", dest="srcdir", default="../../src",
                      help="Source directory containing bitcoind/bitcoin-cli (default: %default%)")
    parser.add_option("--tmpdir", dest="tmpdir", default=tempfile.mkdtemp(prefix="test"),
                      help="Root directory for datadirs")
    (options, args) = parser.parse_args()

    os.environ['PATH'] = options.srcdir+":"+os.environ['PATH']

    check_json_precision()

    success = False
    nodes = []
    try:
        print("Initializing test directory "+options.tmpdir)
        if not os.path.isdir(options.tmpdir):
            os.makedirs(options.tmpdir)
        initialize_chain(options.tmpdir)

        run_test(options.tmpdir)

        success = True

    except AssertionError as e:
        print("Assertion failed: "+e.message)
    except Exception as e:
        print("Unexpected exception caught during testing: "+str(e))
        traceback.print_tb(sys.exc_info()[2])

    if not options.nocleanup:
        print("Cleaning up")
        wait_bitcoinds()
        shutil.rmtree(options.tmpdir)

    if success:
        print("Tests successful")
        sys.exit(0)
    else:
        print("Failed")
        sys.exit(1)

if __name__ == '__main__':
    main()
