#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class AssetTxRootsTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1']]

    def run_test(self):
        self.basic_audittxroot()
        self.basic_audittxroot1()

    def basic_audittxroot(self):
        self.nodes[0].syscoinsetethheaders([[709780,'709780','709779','a','a',0],[707780,'707780','707779','a','a',0],[707772,'707772','707771','a','a',0],[707776,'707776','707775','a','a',0],[707770,'707770','707769','a','a',0],[707778,'707778','707777','a','a',0],[707774,'707774','707773','a','a',0]])
        r = self.nodes[0].syscoinsetethstatus("synced", 709780)
        missing_blocks = r["missing_blocks"]
        # the - MAX_ETHEREUM_TX_ROOTS check to ensure you have at least that many roots stored from the tip
        assert_equal(missing_blocks[0]["from"] , 559780);
        assert_equal(missing_blocks[0]["to"] , 707769);

        assert_equal(missing_blocks[1]["from"] , 707771);
        assert_equal(missing_blocks[1]["to"] , 707771);
        assert_equal(missing_blocks[2]["from"] , 707773);
        assert_equal(missing_blocks[2]["to"] , 707773);
        assert_equal(missing_blocks[3]["from"] , 707775);
        assert_equal(missing_blocks[3]["to"] , 707775);
        assert_equal(missing_blocks[4]["from"] , 707777);
        assert_equal(missing_blocks[4]["to"] , 707777);
        assert_equal(missing_blocks[5]["from"] , 707779);
        assert_equal(missing_blocks[5]["to"] , 707779);
        assert_equal(missing_blocks[6]["from"] , 707781);
        assert_equal(missing_blocks[6]["to"] , 709779);
        self.nodes[0].syscoinsetethheaders([[707773,'707773','707772','a','a',0],[707775,'707775','707774','a','a',0],[707771,'707771','707770','a','a',0],[707777,'707777','707776','a','a',0],[707779,'707779','707778','a','a',0],[707781,'707781','707780','a','a',0]])
        r = self.nodes[0].syscoinsetethstatus("synced", 709780)
        missing_blocks = r["missing_blocks"]
        assert_equal(missing_blocks[0]["from"] , 559780);
        assert_equal(missing_blocks[0]["to"] , 707769);
        assert_equal(missing_blocks[1]["from"] , 707782);
        assert_equal(missing_blocks[1]["to"] , 709779);
        # now fork and check it revalidates chain
        # 707771 (should be 707772) -> 707773 and 707773 (should be 707774) -> 707775
        self.nodes[0].syscoinsetethheaders([[707773,'707773','707771','a','a',0],[707775,'707775','707773','a','a',0]])
        r = self.nodes[0].syscoinsetethstatus("synced", 709780)
        missing_blocks = r["missing_blocks"]
        # we should still have the missing ranges prior to the forks
        assert_equal(missing_blocks[0]["from"] , 559780);
        assert_equal(missing_blocks[0]["to"] , 707769);
        # 707773 is affected, -50 is 707723 and +50 is 707823
        assert_equal(missing_blocks[1]["from"] , 707723);
        assert_equal(missing_blocks[1]["to"] , 707823);
        assert_equal(missing_blocks[2]["from"] , 707725);
        assert_equal(missing_blocks[2]["to"] , 707825);
        # last missing range stays in the missing range list
        assert_equal(missing_blocks[3]["from"] , 707782);
        assert_equal(missing_blocks[3]["to"] , 709779);

    def basic_audittxroot1(self):
        roots = []
        nStartHeight = 700000
        nMissingRange1 = 700059
        nMissingRange2 = 800022
        nMissingRange3 = 814011
        nCount = 0
        for index in range(120000):
            i = nStartHeight + index
            if i == nMissingRange1 or i == nMissingRange2 or i == nMissingRange3:
                continue
            roots += [i,str(i),str(i-1),'a','a',0]
            if nCount > 0 and (nCount % 400) == 0:
                self.nodes[0].syscoinsetethheaders([roots])
                roots = []
                nCount = 0
                continue
            nCount += 1
        if len(roots) > 0:
            self.nodes[0].syscoinsetethheaders([roots])

        missing_blocks = self.nodes[0].syscoinsetethstatus("synced", 820000)
        assert_equal(missing_blocks[0]["from"] , 700059);
        assert_equal(missing_blocks[0]["to"] , 700059);
        assert_equal(missing_blocks[1]["from"] , 800022);
        assert_equal(missing_blocks[1]["to"] , 800022);
        assert_equal(missing_blocks[2]["from"] , 814011);
        assert_equal(missing_blocks[2]["to"] , 814011);
        self.nodes[0].syscoinsetethheaders([[814011,'814011','814010','a','a',0],[700059,'700059','700058','a','a',0],[800022,'800022','800021','a','a',0]])
        missing_blocks = self.nodes[0].syscoinsetethstatus("synced", 820000)
        assert_equal(missing_blocks, False)


if __name__ == '__main__':
    AssetTxRootsTest().main()
