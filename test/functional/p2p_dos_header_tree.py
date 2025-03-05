#!/usr/bin/env python3
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that we reject low difficulty headers to prevent our block tree from filling up with useless bloat"""

from test_framework.messages import (
    CBlockHeader,
    from_hex,
)
from test_framework.p2p import (
    P2PInterface,
    msg_headers,
)
from test_framework.test_framework import BitcoinTestFramework

import os


class RejectLowDifficultyHeadersTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.chain = 'testnet3'  # Use testnet chain because it has an early checkpoint
        self.num_nodes = 2
        self.extra_args = [["-minimumchainwork=0x0", '-prune=550']] * self.num_nodes

    def add_options(self, parser):
        parser.add_argument(
            '--datafile',
            default='data/blockheader_testnet3.hex',
            help='Test data file (default: %(default)s)',
        )

    def run_test(self):
        self.log.info("Read headers data")
        self.headers_file_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), self.options.datafile)
        with open(self.headers_file_path, encoding='utf-8') as headers_data:
            h_lines = [l.strip() for l in headers_data.readlines()]

        # The headers data is taken from testnet3 for early blocks from genesis until the first checkpoint. There are
        # two headers with valid POW at height 1 and 2, forking off from genesis. They are indicated by the FORK_PREFIX.
        FORK_PREFIX = 'fork:'
        self.headers = [l for l in h_lines if not l.startswith(FORK_PREFIX)]
        self.headers_fork = [l[len(FORK_PREFIX):] for l in h_lines if l.startswith(FORK_PREFIX)]

        self.headers = [from_hex(CBlockHeader(), h) for h in self.headers]
        self.headers_fork = [from_hex(CBlockHeader(), h) for h in self.headers_fork]

        self.log.info("Feed all non-fork headers, including and up to the first checkpoint")
        peer_checkpoint = self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=0)
        peer_checkpoint.send_and_ping(msg_headers(self.headers))
        assert {
            'height': 546,
            'hash': '000000002a936ca763904c3c35fce2f3556c559c0214345d31b1bcebf76acb70',
            'branchlen': 546,
            'status': 'headers-only',
        } in self.nodes[0].getchaintips()

        self.log.info("Feed all fork headers (fails due to checkpoint)")
        with self.nodes[0].assert_debug_log(['bad-fork-prior-to-checkpoint']):
            peer_checkpoint.send_message(msg_headers(self.headers_fork))
            peer_checkpoint.wait_for_disconnect()

        self.log.info("Feed all fork headers (succeeds without checkpoint)")
        # On node 0 it succeeds because checkpoints are disabled
        self.restart_node(0, extra_args=['-nocheckpoints', "-minimumchainwork=0x0", '-prune=550'])
        peer_no_checkpoint = self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=0)
        peer_no_checkpoint.send_and_ping(msg_headers(self.headers_fork))
        assert {
            "height": 2,
            "hash": "00000000b0494bd6c3d5ff79c497cfce40831871cbf39b1bc28bd1dac817dc39",
            "branchlen": 2,
            "status": "headers-only",
        } in self.nodes[0].getchaintips()

        # On node 1 it succeeds because no checkpoint has been reached yet by a chain tip
        peer_before_checkpoint = self.nodes[1].add_outbound_p2p_connection(P2PInterface(), p2p_idx=1)
        peer_before_checkpoint.send_and_ping(msg_headers(self.headers_fork))
        assert {
            "height": 2,
            "hash": "00000000b0494bd6c3d5ff79c497cfce40831871cbf39b1bc28bd1dac817dc39",
            "branchlen": 2,
            "status": "headers-only",
        } in self.nodes[1].getchaintips()

        self.log.info("Feed checkpoint-violating block (succeeds up until the checkpoint mismatch, then fails)")
        block = CBlockHeader(self.headers_fork[-1])
        headers_fork2 = []
        for time_offset, nonce in (
            (3,0x023ec36d),(3,0x9e4f929b),(0,0x8bb04400),(3,0x31bbae20),
            (0,0xd53e5028),(2,0x7841f4e3),(5,0x7a8990a4),(2,0x489e28a1),
            (1,0x68bdb61a),(-1,0xf6d2a9fb),(5,0x487dcf25),(3,0x546da3d4),
            (4,0xc1f844d2),(3,0x0c086f6f),(0,0xc2321e55),(6,0xde1aa13c),
            (0,0x05227358),(4,0x5e021e88),(1,0xab2a18ae),(8,0x80c58047),
            (5,0xfaed66b9),(4,0x85e64899),(6,0x3a0de669),(1,0x5c519d58),
            (4,0x992f74d0),(4,0xf9eb3bef),(4,0x07673e35),(5,0x21b99d08),
            (3,0xef38d256),(2,0xa4f1dc1b),(1,0x2c8a4530),(0,0x2b5ea5b4),
            (6,0xf2284fc5),(0,0x1dfbd513),(0,0x0b17a886),(2,0x0bc3f397),
            (0,0x22cc4db0),(9,0x297d6621),(1,0xa6ae996b),(0,0x48749a3f),
            (0,0x989bf1a2),(6,0xa052d387),(1,0x9e2f6137),(3,0x6a0be10c),
            (1,0xef64e362),(4,0xce516de6),(0,0x2d077f41),(1,0x6188fb5d),
            (3,0xa72b0388),(2,0xd1065e3b),(0,0x73641cc7),(4,0xa667e807),
            (3,0x343f4941),(2,0xe62c5e97),(2,0x64ccbe3f),(2,0x891494ce),
            (2,0x1a1593ba),(1,0x7fb3165c),(3,0x9210c30b),(0,0x221e0eb6),
            (2,0x9e80023b),(5,0xb8981525),(4,0xa9eb35ed),(0,0x0cfe8f68),
            (4,0x31868c9d),(0,0xbc0fa60e),(5,0xcca64b21),(0,0x28a1f860),
            (2,0xbf629fb5),(2,0x4e0673ab),(0,0x8271fd15),(2,0x1d1ba6fd),
            (1,0xf8552431),(1,0x4a5847a3),(2,0x60e70a8b),(0,0x28e05674),
            (1,0x3a52baa0),(4,0x93cc4193),(1,0xb9e83452),(-1,0x02578099),
            (6,0xf1deecdb),(3,0xbfc713f0),(1,0xd42411c2),(3,0x2ff09457),
            (4,0xca26349a),(5,0x11e45b8a),(2,0x21861128),(0,0xc2f0cf4d),
            (4,0x8934b604),(0,0xcc822981),(4,0xef67ee2c),(2,0x940a8496),
            (2,0xd67ea903),(3,0xd6975b4c),(1,0x39d3775b),(3,0x33e2a45a),
            (0,0x43452dd5),(5,0xf0221dda),(2,0x24440716),(1,0x9c160e08),
            (4,0xd0e7252a),(1,0x447411d2),(3,0x958eaa10),(-3,0xb9c956ec),
            (1,0x5cd63c9c),(1,0x4af69deb),(3,0x90b93454),(2,0x17959b09),
            (3,0xb05154ec),(0,0x058f9e11),(1,0x227d9c1e),(4,0xacb7701f),
            (4,0x9542721f),(2,0x1e05aa42),(1,0x933d4365),(1,0xb5c3a67e),
            (0,0xac9cc6a6),(1,0x4a76055a),(2,0x7721afe9),(5,0xe12b457e),
            (1,0xaecdc119),(0,0xe8e4ca43),(1,0x6e42e836),(1,0x1d4dec49),
            (1,0xf8056d54),(1,0x584bccb8),(1,0x1932259b),(0,0xdf53ee37),
            (0,0x74e1f913),(1,0xc7d3a7e2),(4,0xef483c04),(0,0x60ad07f0),
            (3,0x89c790ea),(0,0xb7cadd7e),(2,0x38a82398),(1,0xfb93ea07),
            (0,0x200d8ecd),(0,0xf1339a41),(3,0x6af69ec7),(3,0xaffa785f),
            (2,0x67552c61),(3,0xec447684),(0,0xa28c3c1b),(0,0x1a3218f3),
            (3,0xc291da2c),(0,0x497d588f),(3,0x9fc4f865),(0,0x68070cc7),
            (4,0x9b7c0842),(3,0x3ea910ac),(0,0x5c793e79),(3,0x839fca4c),
            (1,0x16a801e7),(0,0x26ad46a3),(2,0x7709be6c),(2,0xc57d1b58),
            (4,0x9a4a49c1),(4,0xd59ee673),(7,0x8d1ded67),(1,0x9ddc3594),
            (0,0xf3af3d45),(3,0xbbb210be),(0,0xec262c80),(4,0x6731951b),
            (0,0xdc36282b),(2,0xae470ed2),(1,0x22c57186),(1,0xde307b71),
            (4,0x3d185e05),(-3,0x58d53609),(1,0xb25fc17d),(2,0x6c490b64),
            (5,0x03d05d9b),(2,0x3e84e2ce),(0,0x2c38303d),(1,0x487c092c),
            (1,0x23a3da6e),(3,0x2854a040),(1,0xa546be54),(1,0x50bfd84d),
            (1,0x6795f353),(0,0x82e77285),(1,0x24dd5aa3),(1,0x0563f0ea),
            (2,0xb2b9be8d),(1,0x54e70019),(0,0xb4a31d38),(0,0xfd3c816d),
            (4,0x84bfd3a7),(3,0x47ecc30d),(2,0x6e1caa0c),(1,0x47097b3e),
            (0,0x30316532),(0,0xca42b48d),(6,0xb5f05a80),(5,0xcf1f9c0f),
            (1,0xc7c9c0a0),(7,0xdce826fa),(2,0x92478133),(0,0xdeff31ab),
            (2,0xfeba362f),(2,0x143b78db),(0,0x48dfb442),(1,0x7b660255),
            (0,0x9b2d9613),(0,0xc81a0b46),(5,0xfa0257c2),(0,0xcb799eac),
            (3,0xe66885b5),(0,0xb7523b87),(3,0x80ec766e),(0,0xd3117846),
            (4,0x44d9d213),(3,0xf3540f37),(0,0x838a3b64),(4,0xd221d138),
            (3,0xa96e0947),(1,0xd09222da),(1,0x72071fec),(0,0x5424ae38),
            (4,0xbd5c3d49),(4,0x2d1be9ef),(3,0x31d8521d),(3,0x5f0ac290),
            (0,0x27710324),(3,0xc1cc6b58),(0,0xb1256275),(0,0x5473ced1),
            (1,0x35293898),(0,0x062a003f),(3,0xee177b2e),(4,0xbbb9235a),
            (1,0x183cebbe),(3,0xced4e55b),(2,0x8050c0fc),(7,0xe46e0207),
            (2,0xf23c4b96),(-2,0x2d7ec1f1),(1,0x90161c6d),(8,0x62d4b92f),
            (1,0x20086401),(2,0x2ce864a0),(4,0x317da57e),(2,0x2cc9250b),
            (0,0xf41ea979),(1,0xa50ca905),(0,0x4c0916bb),(6,0x45becd84),
            (3,0xdac6c807),(0,0xc88954d5),(2,0xf1ebf2b6),(2,0xa1e8f013),
            (1,0xb86af5b1),(1,0xc864d9f1),(4,0x5e91821b),(5,0x9b3ae958),
            (0,0x5c37debb),(4,0xf85e2180),(1,0x9de6eb5c),(1,0x51ec9650),
            (0,0xf3d31a0d),(0,0x127a4cad),(2,0x7698ee29),(1,0x63297ce6),
            (1,0xf9952a56),(0,0xb2bcdb59),(2,0xd904198f),(1,0xe0ceed57),
            (2,0xe883801f),(0,0x9c48eff1),(2,0x69bd1c00),(0,0xefb5c46c),
            (1,0x0ee6d904),(0,0xa01e4f15),(0,0xe50170fc),(3,0x1a057766),
            (2,0xffddd03e),(1,0x9842404d),(0,0xff3c892c),(3,0x468f2c36),
            (2,0xef3c2e4e),(0,0x72c0ea16),(4,0x815c722e),(1,0xe77dee42),
            (0,0xddd8b502),(5,0xf4d60169),(0,0x86b593af),(0,0x08cca2a5),
            (4,0x07652fb5),(2,0x19ead298),(0,0x94af6d52),(2,0x67955b19),
            (0,0xf579c469),(0,0x4b010360),(0,0x67bbba31),(5,0x2e47930e),
            (1,0xbc6872d8),(3,0x645621b8),(1,0xe2c5120b),(0,0xf3c50807),
            (1,0x6dcf4d07),(1,0xaeb4237d),(1,0x2a5f46a0),(2,0x4e8d3883),
            (0,0x48268869),(0,0x3bd2401a),(6,0x0bec238c),(0,0x262c5697),
            (1,0xbbf9572d),(2,0x37df6e6f),(3,0x2ce9e7f3),(2,0x9c0741b1),
            (0,0x8158c135),(2,0x007c8856),(1,0x6c59cc11),(1,0x2d77e50c),
            (0,0x43b85637),(1,0x37440748),(4,0x418b2692),(4,0x3bc8200e),
            (3,0x42e28d55),(1,0xadd84676),(0,0x2f58d608),(4,0xa252bf4e),
            (5,0x36365c0e),(3,0x22736e12),(2,0xac0e0e83),(1,0x30316f60),
            (4,0xedb2b98e),(1,0x46cd60ed),(3,0x8404d922),(2,0x0e230e16),
            (1,0xf2b50134),(-1,0x8e256fff),(0,0x80199c4d),(1,0x56e07a97),
            (1,0x2f9eed9a),(0,0xfb8a7055),(3,0xe3d02c6b),(3,0xd9b43b13),
            (1,0xe4faa52a),(2,0x270801f7),(0,0x5fa6a206),(4,0xa151f071),
            (3,0xd2848896),(0,0x3d5f905f),(5,0x499ae522),(1,0xc61d9839),
            (5,0xdd2f28b4),(0,0x7dc21c39),(0,0x57e1ab56),(0,0x42027d5a),
            (6,0x242a8813),(4,0x69fdcc57),(3,0xff7bd13a),(2,0x9a3b2839),
            (5,0x4af325ee),(2,0x58dea930),(2,0x202b57a4),(764,0xa39c30f0),
            (3,0x1ac08e2c),(7,0xa9263507),(1,0xb1c5602a),(0,0xf9716695),
            (2,0x8ce36f1c),(1,0xddf12292),(4,0xa22e830d),(1,0xb84c2e51),
            (2,0x3f6eb08c),(0,0x185ae643),(0,0x3b3ee340),(5,0x2f0bce41),
            (5,0x2c915d7c),(4,0xe0169c60),(5,0xc5b08211),(0,0x3f123b19),
            (2,0x443baa3f),(0,0x86e5bd6b),(1,0x728af7ae),(2,0x34cd4af0),
            (1,0x773b28fc),(4,0xb42fcf79),(2,0xcf4cddcd),(3,0x472c939d),
            (1,0xa2f66937),(3,0x9202dd5a),(13,0x87b4cbd0),(16,0x8d74ed44),
            (1,0xab0ec370),(0,0x4345bc72),(3,0x1663d4e8),(54,0x53304a20),
            (14,0xf3c7c0b5),(1,0xa755c4d5),(0,0xb4a4246d),(3,0xb55800f7),
            (14,0x80d85102),(0,0x98d4e5ac),(1,0x6f245269),(13,0x038c57c4),
            (0,0x86e5ac2e),(1,0x6c1963c6),(0,0x1adc14a1),(3,0x8f37ac31),
            (18,0x16fd4d32),(15,0xee5bbf16),(13,0xfba44c88),(13,0x4f6b4e47),
            (14,0xf9d7fe91),(1,0xe5e66769),(1,0x7ff9be17),(1,0xd53b2c20),
            (3,0xa48129e8),(1,0xca7692d9),(1,0x55067b69),(0,0xbae33993),
            (0,0x789a395c),(0,0xb2617974),(1,0xa49ae3ae),(0,0x52c934ba),
            (4,0x6ee4bde6),(0,0x083b4807),(4,0x382b6a26),(1,0x17f2e8d3),
            (4,0x70d17e03),(4,0x48bb565b),(2,0x437d655f),(2,0xece43c5f),
            (0,0xc06407f0),(4,0xabc4d27a),(4,0xf7162b9f),(5,0x97dc4313),
            (4,0x7c9578fa),(4,0xd38229e0),(4,0xc61a6e08),(0,0xc821347c),
            (5,0xbb37d0ab),(4,0x3e93a911),(5,0x3c03d85e),(4,0xb289102e),
            (5,0x5e42768e),(4,0x4c574f91),(0,0xc3531db9),(5,0x8ebbfe1b),
            (2,0x62de7520),(4,0x11def7c6),(0,0xd12a5839),(4,0xd60eb230),
            (0,0xfb6d0844),(2,0x701a5c56),(1,0x48998570),(4,0xcc786b3e),
            (1,0x8973807a),(4,0x77151b83),(3,0xb9b223a4),(4,0x7d213a8e),
            (0,0xe627ba6e),(1,0x2de15b60),(0,0x382c67c4),(6,0x43701b8e),
            (8,0x5f41b027),(1,0x09c5d515),(1,0x7fdaaf68),(4,0x8bf79290),
            (3,0x1a7c0271),(0,0x3562b3a1),(4,0x079aa703),(1,0x8fb1aace),
            (4,0xb02b8935),(4,0x939d1697),(0,0x7b2d7a80),(4,0x351c3288),
            (5,0x5f3dd388),(1,0xe865101a),(0,0x57871387),(0,0xd4fd6955),
            (4,0x86fe1cbb),(0,0x21296c24),(4,0x68f6d66d),(3,0x9ac3b10b),
            (8,0x659ebf23),(0,0x8e483006),(5,0x2821774f),(4,0x3b005030),
            (1,0x6bb6f09a),(3,0xb918ea6c),(5,0x259aca8a),(1,0x4bdaa577),
            (3,0x585e295b),(2,0xd7c04cca),(6,0x00a0f04d),(3,0x0cb7eda9),
            (5,0xb1d83641),(4,0xd1105e7d),(2,0xe8ad8382),(3,0x92df8405),
            (4,0xffb344bf),(0,0x94471661),(-3,0x0b9714a4),(6,0x87e08f18),
            (6,0xae594216),(5,0x51acdc5e),(4,0x83664747),(0,0x7b10c195),
            (2,0x1ef4426a),(5,0x1dbf29d0),(2,0xe9b85e89),(3,0x18611e1a),
            (0,0xa10966dd),(0,0xe54fad12),(5,0x03e6220d),(3,0x64fa4ada),
            (4,0x8facb4b4),(2,0xfc0dae5b),(4,0x61a98300),(0,0xd919ee25),
            (3,0x47d8f268),(1,0xd3d5250f),(1,0xa1b7d150),(1,0x39584b6a),
            (1,0xb730f059),(0,0x421a930a),(0,0xf4520724),(2,0xf51ac97e),
            (1,0x3a2e424f),(5,0x4488e824),(5,0x51f37f50),(0,0x2060682b),
            (4,0x668bcc70),(0,0x87bfad0d),(3,0x87a5e2be),(3,0xdb87b268),
            (0,0xbdc1bd9f),(0,0x9e97bae4),(0,0x5f8854cc),(1,0x6a54a576),
            (0,0x6abb7c6e),(4,0x3529e627),(0,0x2b17bf18),(6,0xe718bb6f),
            (0,0x2f8ca470),(5,0xabf106a3),(4,0x6f80543f),(0,0x7459d412),
        ):
            block.hashPrevBlock = block.sha256
            block.nTime += time_offset
            block.nNonce = nonce
            block.rehash()
            headers_fork2.append(CBlockHeader(block))

        with self.nodes[1].assert_debug_log(['checkpoint-mismatch']):
            peer_before_checkpoint.send_message(msg_headers(headers_fork2))
            peer_before_checkpoint.wait_for_disconnect()
        assert {
            "height": 545,
            "hash": "000000008ce04625549eb92726e39fe6de52cd44df861bbd477f9ada8bc30efc",
            "branchlen": 545,
            "status": "headers-only",
        } in self.nodes[1].getchaintips()

if __name__ == '__main__':
    RejectLowDifficultyHeadersTest(__file__).main()
