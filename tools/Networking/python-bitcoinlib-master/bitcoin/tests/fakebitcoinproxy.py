"""
`FakeBitcoinProxy` allows for unit testing of code that normally uses bitcoin
RPC without requiring a running bitcoin node.

`FakeBitcoinProxy` has an interface similar to `bitcoin.rpc.Proxy`, but does not
connect to a local bitcoin RPC node. Hence, `FakeBitcoinProxy` is similar to a
mock for the RPC tests.

`FakeBitcoinProxy` does _not_ implement a full bitcoin RPC node. Instead, it
currently implements only a subset of the available RPC commands. Test setup is
responsible for populating a `FakeBitcoinProxy` object with reasonable mock
data.

:author: Bryan Bishop <kanzure@gmail.com>
"""

import random
import hashlib

from bitcoin.core import (
    # bytes to hex (see x)
    b2x,

    # convert hex string to bytes (see b2x)
    x,

    # convert little-endian hex string to bytes (see b2lx)
    lx,

    # convert bytes to little-endian hex string (see lx)
    b2lx,

    # number of satoshis per bitcoin
    COIN,

    # a type for a transaction that isn't finished building
    CMutableTransaction,
    CMutableTxIn,
    CMutableTxOut,
    COutPoint,
    CTxIn,
)

from bitcoin.wallet import (
    # bitcoin address initialized from base58-encoded string
    CBitcoinAddress,

    # base58-encoded secret key
    CBitcoinSecret,

    # has a nifty function from_pubkey
    P2PKHBitcoinAddress,
)

def make_address_from_passphrase(passphrase, compressed=True, as_str=True):
    """
    Create a Bitcoin address from a passphrase. The passphrase is hashed and
    then used as the secret bytes to construct the CBitcoinSecret.
    """
    if not isinstance(passphrase, bytes):
        passphrase = bytes(passphrase, "utf-8")
    passphrasehash = hashlib.sha256(passphrase).digest()
    private_key = CBitcoinSecret.from_secret_bytes(passphrasehash, compressed=compressed)
    address = P2PKHBitcoinAddress.from_pubkey(private_key.pub)
    if as_str:
        return str(address)
    else:
        return address

def make_txout(amount=None, address=None, counter=None):
    """
    Make a CTxOut object based on the parameters. Otherwise randomly generate a
    CTxOut to represent a transaction output.

    :param amount: amount in satoshis
    """
    passphrase_template = "correct horse battery staple txout {counter}"

    if not counter:
        counter = random.randrange(0, 2**50)

    if not address:
        passphrase = passphrase_template.format(counter=counter)
        address = make_address_from_passphrase(bytes(passphrase, "utf-8"))

    if not amount:
        maxsatoshis = (21 * 1000 * 1000) * (100 * 1000 * 1000) # 21 million BTC * 100 million satoshi per BTC
        amount = random.randrange(0, maxsatoshis) # between 0 satoshi and 21 million BTC

    txout = CMutableTxOut(amount, CBitcoinAddress(address).to_scriptPubKey())

    return txout

def make_blocks_from_blockhashes(blockhashes):
    """
    Create some block data suitable for FakeBitcoinProxy to consume during
    instantiation.
    """
    blocks = []

    for (height, blockhash) in enumerate(blockhashes):
        block = {"hash": blockhash, "height": height, "tx": []}
        if height != 0:
            block["previousblockhash"] = previousblockhash
        blocks.append(block)
        previousblockhash = blockhash

    return blocks

def make_rpc_batch_request_entry(rpc_name, params):
    """
    Construct an entry for the list of commands that will be passed as a batch
    (for `_batch`).
    """
    return {
        "id": "50",
        "version": "1.1",
        "method": rpc_name,
        "params": params,
    }

class FakeBitcoinProxyException(Exception):
    """
    Incorrect usage of fake proxy.
    """
    pass

class FakeBitcoinProxy(object):
    """
    This is an alternative to using `bitcoin.rpc.Proxy` in tests. This class
    can store a number of blocks and transactions, which can then be retrieved
    by calling various "RPC" methods.
    """

    def __init__(self, blocks=None, transactions=None, getnewaddress_offset=None, getnewaddress_passphrase_template="getnewaddress passphrase template {}", num_fundrawtransaction_inputs=5):
        """
        :param getnewaddress_offset: a number to start using and incrementing
        in template used by getnewaddress.
        :type getnewaddress_offset: int
        :param int num_fundrawtransaction_inputs: number of inputs to create
        during fundrawtransaction.
        """
        self.blocks = blocks or {}
        self.transactions = transactions or {}

        if getnewaddress_offset == None:
            self._getnewaddress_offset = 0
        else:
            self._getnewaddress_offset = getnewaddress_offset

        self._getnewaddress_passphrase_template = getnewaddress_passphrase_template
        self._num_fundrawtransaction_inputs = num_fundrawtransaction_inputs
        self.populate_blocks_with_blockheights()

    def _call(self, rpc_method_name, *args, **kwargs):
        """
        This represents a "raw" RPC call, which has output that
        python-bitcoinlib does not parse.
        """
        method = getattr(self, rpc_method_name)
        return method(*args, **kwargs)

    def populate_blocks_with_blockheights(self):
        """
        Helper method to correctly apply "height" on all blocks.
        """
        for (height, block) in enumerate(self.blocks):
            block["height"] = height

    def getblock(self, blockhash, *args, **kwargs):
        """
        :param blockhash: hash of the block to retrieve data for
        :raise IndexError: invalid blockhash
        """

        # Note that the actual "getblock" bitcoind RPC call from
        # python-bitcoinlib returns a CBlock object, not a dictionary.

        if isinstance(blockhash, bytes):
            blockhash = b2lx(blockhash)

        for block in self.blocks:
            if block["hash"] == blockhash:
                return block

        raise IndexError("no block found for blockhash {}".format(blockhash))

    def getblockhash(self, blockheight):
        """
        Get block by blockheight.

        :type blockheight: int
        :rtype: dict
        """
        for block in self.blocks:
            if block["height"] == int(blockheight):
                return block["hash"]

    def getblockcount(self):
        """
        Return the total number of blocks. When there is only one block in the
        blockchain, this function will return zero.

        :rtype: int
        """
        return len(self.blocks) - 1

    def getrawtransaction(self, txid, *args, **kwargs):
        """
        Get parsed transaction.

        :type txid: bytes or str
        :rtype: dict
        """
        if isinstance(txid, bytes):
            txid = b2lx(txid)
        return self.transactions[txid]

    def getnewaddress(self):
        """
        Construct a new address based on a passphrase template. As more
        addresses are generated, the template value goes up.
        """
        passphrase = self._getnewaddress_passphrase_template.format(self._getnewaddress_offset)
        address = make_address_from_passphrase(bytes(passphrase, "utf-8"))
        self._getnewaddress_offset += 1
        return CBitcoinAddress(address)

    def importaddress(self, *args, **kwargs):
        """
        Completely unrealistic fake version of importaddress.
        """
        return True

    # This was implemented a long time ago and it's possible that this does not
    # match the current behavior of fundrawtransaction.
    def fundrawtransaction(self, given_transaction, *args, **kwargs):
        """
        Make up some inputs for the given transaction.
        """
        # just use any txid here
        vintxid = lx("99264749804159db1e342a0c8aa3279f6ef4031872051a1e52fb302e51061bef")

        if isinstance(given_transaction, str):
            given_bytes = x(given_transaction)
        elif isinstance(given_transaction, CMutableTransaction):
            given_bytes = given_transaction.serialize()
        else:
            raise FakeBitcoinProxyException("Wrong type passed to fundrawtransaction.")

        # this is also a clever way to not cause a side-effect in this function
        transaction = CMutableTransaction.deserialize(given_bytes)

        for vout_counter in range(0, self._num_fundrawtransaction_inputs):
            txin = CMutableTxIn(COutPoint(vintxid, vout_counter))
            transaction.vin.append(txin)

        # also allocate a single output (for change)
        txout = make_txout()
        transaction.vout.append(txout)

        transaction_hex = b2x(transaction.serialize())

        return {"hex": transaction_hex, "fee": 5000000}

    def signrawtransaction(self, given_transaction):
        """
        This method does not actually sign the transaction, but it does return
        a transaction based on the given transaction.
        """
        if isinstance(given_transaction, str):
            given_bytes = x(given_transaction)
        elif isinstance(given_transaction, CMutableTransaction):
            given_bytes = given_transaction.serialize()
        else:
            raise FakeBitcoinProxyException("Wrong type passed to signrawtransaction.")

        transaction = CMutableTransaction.deserialize(given_bytes)
        transaction_hex = b2x(transaction.serialize())
        return {"hex": transaction_hex}

    def sendrawtransaction(self, given_transaction):
        """
        Pretend to broadcast and relay the transaction. Return the txid of the
        given transaction.
        """
        if isinstance(given_transaction, str):
            given_bytes = x(given_transaction)
        elif isinstance(given_transaction, CMutableTransaction):
            given_bytes = given_transaction.serialize()
        else:
            raise FakeBitcoinProxyException("Wrong type passed to sendrawtransaction.")
        transaction = CMutableTransaction.deserialize(given_bytes)
        return b2lx(transaction.GetHash())

    def _batch(self, batch_request_entries):
        """
        Process a bunch of requests all at once. This mimics the _batch RPC
        feature found in python-bitcoinlib and bitcoind RPC.
        """
        necessary_keys = ["id", "version", "method", "params"]

        results = []

        for (idx, request) in enumerate(batch_request_entries):
            error = None
            result = None

            # assert presence of important details
            for necessary_key in necessary_keys:
                if not necessary_key in request.keys():
                    raise FakeBitcoinProxyException("Missing necessary key {} for _batch request number {}".format(necessary_key, idx))

            if isinstance(request["params"], list):
                method = getattr(self, request["method"])
                result = method(*request["params"])
            else:
                # matches error message received through python-bitcoinrpc
                error = {"message": "Params must be an array", "code": -32600}

            results.append({
                "error": error,
                "id": request["id"],
                "result": result,
            })

        return results
