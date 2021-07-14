import os
import time

from pathlib import Path

from pypoptools.pypoptesting.framework.bin_util import assert_dir_accessible, get_open_port
from pypoptools.pypoptesting.framework.entities import *
from pypoptools.pypoptesting.framework.json_rpc import JsonRpcApi, JsonRpcException
from pypoptools.pypoptesting.framework.managers import ProcessManager
from pypoptools.pypoptesting.framework.node import Node
from pypoptools.pypoptesting.framework.sync_util import wait_until

PORT_MIN = 15000
PORT_MAX = 25000
BIND_TO = '127.0.0.1'


def _write_vbitcoin_conf(datadir, p2p_port, rpc_port, rpc_user, rpc_password):
    bitcoin_conf_file = Path(datadir, "vbitcoin.conf")
    with open(bitcoin_conf_file, 'w', encoding='utf8') as f:
        f.write("regtest=1\n")
        f.write("[{}]\n".format("regtest"))
        f.write("port={}\n".format(p2p_port))
        f.write("rpcport={}\n".format(rpc_port))
        f.write("rpcuser={}\n".format(rpc_user))
        f.write("rpcpassword={}\n".format(rpc_password))
        f.write("fallbackfee=0.0002\n")
        f.write("server=1\n")
        f.write("keypool=1\n")
        f.write("discover=0\n")
        f.write("dnsseed=0\n")
        f.write("listenonion=0\n")
        f.write("printtoconsole=0\n")
        f.write("upnp=0\n")
        f.write("shrinkdebugfile=0\n")
        f.write("popvbknetwork=regtest\n")
        f.write("popbtcnetwork=regtest\n")
        f.write("poplogverbosity=info\n")


class VBitcoindNode(Node):
    def __init__(self, number: int, datadir: Path):
        self.number = number

        p2p_port = get_open_port(PORT_MIN, PORT_MAX, BIND_TO)
        self.p2p_address = "{}:{}".format(BIND_TO, p2p_port)

        rpc_port = get_open_port(PORT_MIN, PORT_MAX, BIND_TO)
        rpc_url = "http://{}:{}/".format(BIND_TO, rpc_port)
        rpc_user = 'testuser'
        rpc_password = 'testpassword'
        self.rpc = JsonRpcApi(rpc_url, user=rpc_user, password=rpc_password)

        vbitcoind_path = os.environ.get('VBITCOIND_PATH')
        if vbitcoind_path == None:
            raise Exception("VBITCOIND_PATH env var is not set. Set up the path to the vbitcoind binary to the VBITCOIND_PATH env var")

        exe = Path(Path.cwd(), vbitcoind_path)
        if not exe:
            raise Exception("VBitcoinNode: vbitcoind is not found in PATH")

        assert_dir_accessible(datadir)
        args = [
            exe,
            "-datadir=" + str(datadir),
            "-logtimemicros",
            "-logthreadnames",
            "-debug",
            "-debugexclude=libevent",
            "-debugexclude=leveldb",
            "-txindex",
            "-uacomment=testnode{}".format(number)
        ]
        self.manager = ProcessManager(args, datadir)

        _write_vbitcoin_conf(datadir, p2p_port, rpc_port, rpc_user, rpc_password)

    def start(self) -> None:
        self.manager.start()
        wait_until(lambda: self.is_rpc_available(), timeout=60)

    def is_rpc_available(self) -> bool:
        try:
            self.rpc.getblockcount()
            return True
        except Exception as e:
            return False

    def stop(self) -> None:
        self.manager.stop()

    def connect(self, node):
        self.rpc.addnode(node.p2p_address, 'onetry')
        # poll until version handshake complete to avoid race conditions
        # with transaction relaying
        wait_until(lambda: all(peer['version'] != 0 for peer in self.rpc.getpeerinfo()))

    def disconnect(self, node):
        node_num = node.number
        for peer_id in [peer['id'] for peer in self.rpc.getpeerinfo() if
                        "testnode{}".format(node_num) in peer['subver']]:
            try:
                self.rpc.disconnectnode(address='', nodeid=peer_id)
            except JsonRpcException as e:
                # If this node is disconnected between calculating the peer id
                # and issuing the disconnect, don't worry about it.
                # This avoids a race condition if we're mass-disconnecting peers.
                if e.error['code'] != -29:  # RPC_CLIENT_NODE_NOT_CONNECTED
                    raise

        # wait to disconnect
        wait_until(
            lambda: not any(["testnode{}".format(node_num) in peer['subver'] for peer in self.rpc.getpeerinfo()]),
            timeout=5)

    def getpeers(self) -> List[Peer]:
        s = self.rpc.getpeerinfo()
        return [
            Peer(
                id=peer['id'],
                banscore=peer['banscore']
            )
            for peer in s
        ]

    def getbalance(self) -> int:
        # convert to satoshi
        return int(self.rpc.getbalance() * 10**8)

    def getnewaddress(self) -> str:
        return self.rpc.getnewaddress()

    def getpayoutinfo(self, address: str = None) -> Hexstr:
        address = address or self.getnewaddress()
        s = self.rpc.validateaddress(address)
        return s['scriptPubKey']

    def generate(self, nblocks: int, address: str = None) -> None:
        address = address or self.getnewaddress()
        for i in range(nblocks):
            self.rpc.generatetoaddress(1, address)
            tip_hash = self.getbestblockhash()
            tip = self.rpc.getblock(tip_hash)
            tip_time = tip['time']
            current_time = int(time.time())
            if current_time < tip_time:
                time.sleep(tip_time - current_time)

    def getbestblockhash(self) -> Hexstr:
        return self.rpc.getbestblockhash()

    def getblock(self, hash: Hexstr) -> BlockWithPopData:
        s = self.rpc.getblock(hash)
        return BlockWithPopData(
            hash=s['hash'],
            height=s['height'],
            prevhash=s.get('previousblockhash', ''),
            confirmations=s['confirmations'],
            endorsedBy=s['pop']['state']['endorsedBy'],
            blockOfProofEndorsements=[],
            containingATVs=s['pop']['data']['atvs'],
            containingVTBs=s['pop']['data']['vtbs'],
            containingVBKs=s['pop']['data']['vbkblocks']
        )

    def getblockcount(self) -> int:
        return self.rpc.getblockcount()

    def getblockhash(self, height: int) -> Hexstr:
        return self.rpc.getblockhash(height)

    def getbtcbestblockhash(self) -> Hexstr:
        return self.rpc.getbtcbestblockhash()

    def getpopdatabyhash(self, hash: Hexstr) -> GetpopdataResponse:
        s = self.rpc.getpopdatabyhash(hash)
        return GetpopdataResponse(
            header=s['block_header'],
            authenticated_context=s['authenticated_context']['serialized'],
            last_known_vbk_block=s['last_known_veriblock_blocks'][-1],
            last_known_btc_block=s['last_known_bitcoin_blocks'][-1],
        )

    def getpopdatabyheight(self, height: int) -> GetpopdataResponse:
        s = self.rpc.getpopdatabyheight(height)
        return GetpopdataResponse(
            header=s['block_header'],
            authenticated_context=s['authenticated_context']['serialized'],
            last_known_vbk_block=s['last_known_veriblock_blocks'][-1],
            last_known_btc_block=s['last_known_bitcoin_blocks'][-1],
        )

    def getpopparams(self) -> PopParamsResponse:
        s = self.rpc.getpopparams()
        bootstrap = GenericBlock(
            hash=s['bootstrapBlock']['hash'],
            prevhash=s['bootstrapBlock']['previousBlock'],
            height=s['bootstrapBlock']['height']
        )

        vbkBootstrap = BlockAndNetwork(
            block=GenericBlock(
                hash=s['vbkBootstrapBlock']['hash'],
                prevhash=s['vbkBootstrapBlock'].get('previousBlock', ''),
                height=s['vbkBootstrapBlock']['height']
            ),
            network=s['vbkBootstrapBlock']['network']
        )

        btcBootstrap = BlockAndNetwork(
            block=GenericBlock(
                hash=s['btcBootstrapBlock']['hash'],
                prevhash=s['btcBootstrapBlock'].get('previousBlock', ''),
                height=s['btcBootstrapBlock']['height']
            ),
            network=s['btcBootstrapBlock']['network']
        )

        return PopParamsResponse(
            popActivationHeight=s['popActivationHeight'],
            popRewardPercentage=s['popRewardPercentage'],
            popRewardCoefficient=s['popRewardCoefficient'],
            popPayoutDelay=s['payoutParams']['popPayoutDelay'],
            bootstrapBlock=bootstrap,
            vbkBootstrap=vbkBootstrap,
            btcBootstrap=btcBootstrap,
            networkId=s['networkId'],
            maxVbkBlocksInAltBlock=s['maxVbkBlocksInAltBlock'],
            maxVTBsInAltBlock=s['maxVTBsInAltBlock'],
            maxATVsInAltBlock=s['maxATVsInAltBlock'],
            endorsementSettlementInterval=s['endorsementSettlementInterval'],
            finalityDelay=s['finalityDelay'],
            keystoneInterval=s['keystoneInterval'],
            maxAltchainFutureBlockTime=s['maxAltchainFutureBlockTime']
        )

    def getrawatv(self, atvid: Hexstr) -> AtvResponse:
        s = self.rpc.getrawatv(atvid, 1)
        r = AtvResponse()
        r.in_active_chain = s['in_active_chain']
        r.confirmations = s['confirmations']
        if r.confirmations > 0:
            # in block
            r.blockhash = s['blockhash']
            r.blockheight = s['blockheight']
            r.containingBlocks = s['containing_blocks']

        a = s['atv']
        tx = a['transaction']
        pd = tx['publicationData']
        bop = a['blockOfProof']
        r.atv = ATV(
            id=a['id'],
            tx=VbkTx(
                hash=tx['hash'],
                publicationData=PublicationData(**pd)
            ),
            blockOfProof=GenericBlock(
                hash=bop['hash'],
                prevhash=bop['previousBlock'],
                height=bop['height']
            )
        )

        return r

    def getrawpopmempool(self) -> RawPopMempoolResponse:
        s = self.rpc.getrawpopmempool()
        return RawPopMempoolResponse(**s)

    def getrawvbkblock(self, vbkblockid: Hexstr) -> VbkBlockResponse:
        s = self.rpc.getrawvbkblock(vbkblockid, 1)
        r = VbkBlockResponse()
        r.in_active_chain = s['in_active_chain']
        r.confirmations = s['confirmations']
        if r.confirmations > 0:
            # in block
            r.blockhash = s['blockhash']
            r.blockheight = s['blockheight']
            r.containingBlocks = s['containing_blocks']

        r.vbkblock = VbkBlock(**s['vbkblock'])
        return r

    def getrawvtb(self, vtbid: Hexstr) -> VtbResponse:
        s = self.rpc.getrawvtb(vtbid, 1)
        r = VtbResponse()
        r.in_active_chain = s['in_active_chain']
        r.confirmations = s['confirmations']
        if r.confirmations > 0:
            # in block
            r.blockhash = s['blockhash']
            r.blockheight = s['blockheight']
            r.containingBlocks = s['containing_blocks']

        v = s['vtb']
        tx = v['transaction']
        cb = v['containingBlock']
        r.vtb = VTB(
            id=v['id'],
            tx=VbkPopTx(
                hash=tx['hash'],
                publishedBlock=VbkBlock(**tx['publishedBlock']),
                blockOfProof=BtcBlock(**tx['blockOfProof']),
                blockOfProofContext=[BtcBlock(**x) for x in tx['blockOfProofContext']]
            ),
            containingBlock=GenericBlock(
                hash=cb['hash'],
                prevhash=cb['previousBlock'],
                height=cb['height']
            )
        )

        return r

    def getvbkbestblockhash(self) -> Hexstr:
        return self.rpc.getvbkbestblockhash()

    def submitpopatv(self, atv: Hexstr) -> SubmitPopResponse:
        s = self.rpc.submitpopatv(atv)
        return SubmitPopResponse(
            accepted=s['accepted'],
            code=s.get('code', ''),
            message=s.get('message', '')
        )

    def submitpopvbk(self, vbk: Hexstr) -> SubmitPopResponse:
        s = self.rpc.submitpopvbk(vbk)
        return SubmitPopResponse(
            accepted=s['accepted'],
            code=s.get('code', ''),
            message=s.get('message', '')
        )

    def submitpopvtb(self, vtb: Hexstr) -> SubmitPopResponse:
        s = self.rpc.submitpopvtb(vtb)
        return SubmitPopResponse(
            accepted=s['accepted'],
            code=s.get('code', ''),
            message=s.get('message', '')
        )

    def getrpcfunctions(self) -> RpcFunctions:
        return RpcFunctions(
            get_popdata_by_height = 'getpopdatabyheight',
            get_popdata_by_hash = 'getpopdatabyhash',
            submit_atv = 'submitpopatv',
            submit_vtb = 'submitpopvtb',
            submit_vbk = 'submitpopvbk',
            get_missing_btc_block_hashes = 'getmissingbtcblockhashes',
            extract_block_info = 'extractblockinfo',
            get_vbk_block = 'getvbkblock',
            get_btc_block = 'getbtcblock',
            get_vbk_best_block_hash = 'getvbkbestblockhash',
            get_btc_best_block_hash = 'getbtcbestblockhash',
            get_raw_atv = 'getrawatv',
            get_raw_vtb = 'getrawvtb',
        )