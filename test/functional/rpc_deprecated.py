#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of RPC calls."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

class DeprecatedRpcTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[], ["-deprecatedrpc=validateaddress", "-deprecatedrpc=accounts"]]

    def run_test(self):
        # This test should be used to verify correct behaviour of deprecated
        # RPC methods with and without the -deprecatedrpc flags. For example:
        #
        # self.log.info("Make sure that -deprecatedrpc=createmultisig allows it to take addresses")
        # assert_raises_rpc_error(-5, "Invalid public key", self.nodes[0].createmultisig, 1, [self.nodes[0].getnewaddress()])
        # self.nodes[1].createmultisig(1, [self.nodes[1].getnewaddress()])

        self.log.info("Test validateaddress deprecation")
        SOME_ADDRESS = "yZNRHJXRPAiSMXd2knNE174gFqYKFbwVvB"  # This is just some random address to pass as a parameter to validateaddress
        dep_validate_address = self.nodes[0].validateaddress(SOME_ADDRESS)
        assert "ismine" not in dep_validate_address
        not_dep_val = self.nodes[1].validateaddress(SOME_ADDRESS)
        assert "ismine" in not_dep_val

        self.log.info("Test accounts deprecation")
        # The following account RPC methods are deprecated:
        # - getaccount
        # - getaccountaddress
        # - getaddressesbyaccount
        # - getreceivedbyaccount
        # - listaccouts
        # - listreceivedbyaccount
        # - move
        # - setaccount
        #
        # The following 'label' RPC methods are usable both with and without the
        # -deprecatedrpc=accounts switch enabled.
        # - getaddressesbylabel
        # - getreceivedbylabel
        # - listlabels
        # - listreceivedbylabel
        # - setlabel
        #
        address0 = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(101, address0)
        self.sync_all()
        address1 = self.nodes[1].getnewaddress()
        self.nodes[1].generatetoaddress(101, address1)

        self.log.info("- getaccount")
        assert_raises_rpc_error(-32, "getaccount is deprecated", self.nodes[0].getaccount, address0)
        self.nodes[1].getaccount(address1)

        self.log.info("- setaccount")
        assert_raises_rpc_error(-32, "setaccount is deprecated", self.nodes[0].setaccount, address0, "label0")
        self.nodes[1].setaccount(address1, "label1")

        self.log.info("- setlabel")
        self.nodes[0].setlabel(address0, "label0")
        self.nodes[1].setlabel(address1, "label1")

        self.log.info("- getaccountaddress")
        assert_raises_rpc_error(-32, "getaccountaddress is deprecated", self.nodes[0].getaccountaddress, "label0")
        self.nodes[1].getaccountaddress("label1")

        self.log.info("- getaddressesbyaccount")
        assert_raises_rpc_error(-32, "getaddressesbyaccount is deprecated", self.nodes[0].getaddressesbyaccount, "label0")
        self.nodes[1].getaddressesbyaccount("label1")

        self.log.info("- getaddressesbylabel")
        self.nodes[0].getaddressesbylabel("label0")
        self.nodes[1].getaddressesbylabel("label1")

        self.log.info("- getreceivedbyaccount")
        assert_raises_rpc_error(-32, "getreceivedbyaccount is deprecated", self.nodes[0].getreceivedbyaccount, "label0")
        self.nodes[1].getreceivedbyaccount("label1")

        self.log.info("- getreceivedbylabel")
        self.nodes[0].getreceivedbylabel("label0")
        self.nodes[1].getreceivedbylabel("label1")

        self.log.info("- listaccounts")
        assert_raises_rpc_error(-32, "listaccounts is deprecated", self.nodes[0].listaccounts)
        self.nodes[1].listaccounts()

        self.log.info("- listlabels")
        self.nodes[0].listlabels()
        self.nodes[1].listlabels()

        self.log.info("- listreceivedbyaccount")
        assert_raises_rpc_error(-32, "listreceivedbyaccount is deprecated", self.nodes[0].listreceivedbyaccount)
        self.nodes[1].listreceivedbyaccount()

        self.log.info("- listreceivedbylabel")
        self.nodes[0].listreceivedbylabel()
        self.nodes[1].listreceivedbylabel()

        self.log.info("- move")
        assert_raises_rpc_error(-32, "move is deprecated", self.nodes[0].move, "label0", "label0b", 10)
        self.nodes[1].move("label1", "label1b", 10)

if __name__ == '__main__':
    DeprecatedRpcTest().main()
