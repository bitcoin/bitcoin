#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test p2p blocksonly"""

from test_framework.messages import msg_inv, msg_tx, CInv, CTransaction, FromHex
from test_framework.mininode import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class P2PBlocksOnly(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.extra_args = [["-blocksonly"]]

    def run_test(self):
        self.nodes[0].add_p2p_connection(P2PInterface())  # sends us a tx INV message
        self.nodes[0].add_p2p_connection(P2PInterface())  # sends us a TX message
        self.nodes[0].add_p2p_connection(P2PInterface())

        assert_equal(self.nodes[0].getnetworkinfo()['localrelay'], False)

        self.log.info('Create signed transaction')
        prevtx = self.nodes[0].getblock(self.nodes[0].getblockhash(1), 2)['tx'][0]
        rawtx = self.nodes[0].createrawtransaction(
            inputs=[{
                'txid': prevtx['txid'],
                'vout': 0
            }],
            outputs=[{
                self.nodes[0].get_deterministic_priv_key().address: 50 - 0.00125
            }],
        )
        sigtx = self.nodes[0].signrawtransactionwithkey(
            hexstring=rawtx,
            privkeys=[self.nodes[0].get_deterministic_priv_key().key],
            prevtxs=[{
                'txid': prevtx['txid'],
                'vout': 0,
                'scriptPubKey': prevtx['vout'][0]['scriptPubKey']['hex'],
            }],
        )['hex']
        tx = FromHex(CTransaction(), sigtx)

        self.log.info('Check that a TX inv from a blocks-only peer results in disconnect')
        with self.nodes[0].assert_debug_log(['in violation of protocol']):
            self.nodes[0].p2ps[0].send_message(msg_inv([CInv(1, int(tx.rehash(), 16))]))
            self.nodes[0].p2ps[0].wait_for_disconnect()

        self.log.info('Check that a TX message from a blocks-only peer results in disconnect')
        with self.nodes[0].assert_debug_log(['in violation of protocol']):
            self.nodes[0].p2ps[1].send_message(msg_tx(tx))
            self.nodes[0].p2ps[1].wait_for_disconnect()

        self.log.info('Check that txs from rpc are not rejected and relayed to other peers')
        assert_equal(self.nodes[0].getpeerinfo()[0]['relaytxes'], True)
        txid = self.nodes[0].testmempoolaccept([sigtx])[0]['txid']
        with self.nodes[0].assert_debug_log(['received getdata for: tx {} peer=2'.format(txid)]):
            self.nodes[0].sendrawtransaction(sigtx)
            self.nodes[0].p2ps[2].wait_for_tx(txid)
            assert_equal(self.nodes[0].getmempoolinfo()['size'], 1)

if __name__ == '__main__':
    P2PBlocksOnly().main()
