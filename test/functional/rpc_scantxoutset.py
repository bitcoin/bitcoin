#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the scantxoutset rpc call."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, Decimal

import shutil
import os

class ScantxoutsetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(110)

        addr1 = self.nodes[0].getnewaddress("")
        pubk1 = self.nodes[0].getaddressinfo(addr1)['pubkey']
        addr2 = self.nodes[0].getnewaddress("")
        pubk2 = self.nodes[0].getaddressinfo(addr2)['pubkey']
        addr3 = self.nodes[0].getnewaddress("")
        pubk3 = self.nodes[0].getaddressinfo(addr3)['pubkey']
        self.nodes[0].sendtoaddress(addr1, 0.001)
        self.nodes[0].sendtoaddress(addr2, 0.002)
        self.nodes[0].sendtoaddress(addr3, 0.004)

        #send to child keys of tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK
        self.nodes[0].sendtoaddress("yR5yZLjevw5kX3UxGiQN1g96LXGJni2wSS", 0.008) # (m/0'/0'/0')
        self.nodes[0].sendtoaddress("yPcxzaQekxTjaJVSaZ58r22o37H8moWPK2", 0.016) # (m/0'/0'/1')
        self.nodes[0].sendtoaddress("yhv7iRHSx4SgyvCPmkm6Js8gTuJTtJH9ec", 0.032) # (m/0'/0'/1500')
        self.nodes[0].sendtoaddress("yWEdyyKVNbmaiXHkg3LVPqgoXpMA3S6Xt7", 0.064) # (m/0'/0'/0)
        self.nodes[0].sendtoaddress("yTGAdq8sSHJ1QrcqSUaMHs8RMj3Bqz3bkb", 0.128) # (m/0'/0'/1)
        self.nodes[0].sendtoaddress("yRTNkmjXjhatND4Dv3V4GwBzYJQ4o9ukQr", 0.256) # (m/0'/0'/1500)
        self.nodes[0].sendtoaddress("yPwUp9Vwmr4zE6rSuZg3TBxeyRerdRAbNd", 0.512) # (m/1/1/0')
        self.nodes[0].sendtoaddress("yLapNU3bG8E8JNGXRhZbRHHDifrqTucGcg", 1.024) # (m/1/1/1')
        self.nodes[0].sendtoaddress("yUhbAKf7AcTC5sPXb2dkABKm3FYENqdzv2", 2.048) # (m/1/1/1500')
        self.nodes[0].sendtoaddress("yZTyMdEJjZWJi6CwY6g3WurLESH3UsWrrM", 4.096) # (m/1/1/0)
        self.nodes[0].sendtoaddress("ydccVGNV2EcEouAxbbgdu8pi8gkdaqkiav", 8.192) # (m/1/1/1)
        self.nodes[0].sendtoaddress("yVCdQxPXJ3SrtTLv8FuLXDNaynz6kmjPNq", 16.384) # (m/1/1/1500)


        self.nodes[0].generate(1)

        self.log.info("Stop node, remove wallet, mine again some blocks...")
        self.stop_node(0)
        shutil.rmtree(os.path.join(self.nodes[0].datadir, "regtest", 'wallets'))
        self.start_node(0)
        self.nodes[0].generate(110)

        self.restart_node(0, ['-nowallet'])
        self.log.info("Test if we have found the non HD unspent outputs.")
        assert_equal(self.nodes[0].scantxoutset("start", [ "pkh(" + pubk1 + ")", "pkh(" + pubk2 + ")", "pkh(" + pubk3 + ")"])['total_amount'], Decimal("0.007"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(" + pubk1 + ")", "combo(" + pubk2 + ")", "combo(" + pubk3 + ")"])['total_amount'], Decimal("0.007"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "addr(" + addr1 + ")", "addr(" + addr2 + ")", "combo(" + pubk3 + ")"])['total_amount'], Decimal("0.007"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "addr(" + addr1 + ")", "addr(" + addr2 + ")", "addr(" + addr3 + ")"])['total_amount'], Decimal("0.007"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "addr(" + addr1 + ")", "addr(" + addr2 + ")", "pkh(" + pubk3 + ")"])['total_amount'], Decimal("0.007"))

        self.log.info("Test extended key derivation.")
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0h/0h)"])['total_amount'], Decimal("0.008"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0'/1h)"])['total_amount'], Decimal("0.016"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/0'/1500')"])['total_amount'], Decimal("0.032"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/0h/0)"])['total_amount'], Decimal("0.064"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0h/1)"])['total_amount'], Decimal("0.128"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/0'/1500)"])['total_amount'], Decimal("0.256"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0h/*h)", "range": 1499}])['total_amount'], Decimal("0.024"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0'/*h)", "range": 1500}])['total_amount'], Decimal("0.056"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/0'/*)", "range": 1499}])['total_amount'], Decimal("0.192"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0h/*)", "range": 1500}])['total_amount'], Decimal("0.448"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0')"])['total_amount'], Decimal("0.512"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/1')"])['total_amount'], Decimal("1.024"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/1500h)"])['total_amount'], Decimal("2.048"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0)"])['total_amount'], Decimal("4.096"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/1)"])['total_amount'], Decimal("8.192"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/1500)"])['total_amount'], Decimal("16.384"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/0)"])['total_amount'], Decimal("4.096"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/1)"])['total_amount'], Decimal("8.192"))
        assert_equal(self.nodes[0].scantxoutset("start", [ "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/1500)"])['total_amount'], Decimal("16.384"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*')", "range": 1499}])['total_amount'], Decimal("1.536"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*')", "range": 1500}])['total_amount'], Decimal("3.584"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)", "range": 1499}])['total_amount'], Decimal("12.288"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)", "range": 1500}])['total_amount'], Decimal("28.672"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/*)", "range": 1499}])['total_amount'], Decimal("12.288"))
        assert_equal(self.nodes[0].scantxoutset("start", [ {"desc": "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/*)", "range": 1500}])['total_amount'], Decimal("28.672"))

if __name__ == '__main__':
    ScantxoutsetTest().main()
