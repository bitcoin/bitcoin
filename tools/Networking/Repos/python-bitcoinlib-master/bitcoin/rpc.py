# Copyright (C) 2007 Jan-Klaas Kollhof
# Copyright (C) 2011-2018 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

"""Bitcoin Core RPC support

By default this uses the standard library ``json`` module. By monkey patching,
a different implementation can be used instead, at your own risk:

>>> import simplejson
>>> import bitcoin.rpc
>>> bitcoin.rpc.json = simplejson

(``simplejson`` is the externally maintained version of the same module and
thus better optimized but perhaps less stable.)
"""

from __future__ import absolute_import, division, print_function, unicode_literals
import ssl

try:
    import http.client as httplib
except ImportError:
    import httplib
import base64
import binascii
import decimal
import json
import os
import platform
import sys
try:
    import urllib.parse as urlparse
except ImportError:
    import urlparse

import bitcoin
from bitcoin.core import COIN, x, lx, b2lx, CBlock, CBlockHeader, CTransaction, COutPoint, CTxOut
from bitcoin.core.script import CScript
from bitcoin.wallet import CBitcoinAddress, CBitcoinSecret

DEFAULT_USER_AGENT = "AuthServiceProxy/0.1"

DEFAULT_HTTP_TIMEOUT = 30

# (un)hexlify to/from unicode, needed for Python3
unhexlify = binascii.unhexlify
hexlify = binascii.hexlify
if sys.version > '3':
    unhexlify = lambda h: binascii.unhexlify(h.encode('utf8'))
    hexlify = lambda b: binascii.hexlify(b).decode('utf8')


class JSONRPCError(Exception):
    """JSON-RPC protocol error base class

    Subclasses of this class also exist for specific types of errors; the set
    of all subclasses is by no means complete.
    """

    SUBCLS_BY_CODE = {}

    @classmethod
    def _register_subcls(cls, subcls):
        cls.SUBCLS_BY_CODE[subcls.RPC_ERROR_CODE] = subcls
        return subcls

    def __new__(cls, rpc_error):
        assert cls is JSONRPCError
        cls = JSONRPCError.SUBCLS_BY_CODE.get(rpc_error['code'], cls)

        self = Exception.__new__(cls)

        super(JSONRPCError, self).__init__(
            'msg: %r  code: %r' %
            (rpc_error['message'], rpc_error['code']))
        self.error = rpc_error

        return self

@JSONRPCError._register_subcls
class ForbiddenBySafeModeError(JSONRPCError):
    RPC_ERROR_CODE = -2

@JSONRPCError._register_subcls
class InvalidAddressOrKeyError(JSONRPCError):
    RPC_ERROR_CODE = -5

@JSONRPCError._register_subcls
class InvalidParameterError(JSONRPCError):
    RPC_ERROR_CODE = -8

@JSONRPCError._register_subcls
class VerifyError(JSONRPCError):
    RPC_ERROR_CODE = -25

@JSONRPCError._register_subcls
class VerifyRejectedError(JSONRPCError):
    RPC_ERROR_CODE = -26

@JSONRPCError._register_subcls
class VerifyAlreadyInChainError(JSONRPCError):
    RPC_ERROR_CODE = -27

@JSONRPCError._register_subcls
class InWarmupError(JSONRPCError):
    RPC_ERROR_CODE = -28


class BaseProxy(object):
    """Base JSON-RPC proxy class. Contains only private methods; do not use
    directly."""

    def __init__(self,
                 service_url=None,
                 service_port=None,
                 btc_conf_file=None,
                 timeout=DEFAULT_HTTP_TIMEOUT,
                 connection=None):

        # Create a dummy connection early on so if __init__() fails prior to
        # __conn being created __del__() can detect the condition and handle it
        # correctly.
        self.__conn = None
        authpair = None

        if service_url is None:
            # Figure out the path to the bitcoin.conf file
            if btc_conf_file is None:
                if platform.system() == 'Darwin':
                    btc_conf_file = os.path.expanduser('~/Library/Application Support/Bitcoin/')
                elif platform.system() == 'Windows':
                    btc_conf_file = os.path.join(os.environ['APPDATA'], 'Bitcoin')
                else:
                    btc_conf_file = os.path.expanduser('~/.bitcoin')
                btc_conf_file = os.path.join(btc_conf_file, 'bitcoin.conf')

            # Bitcoin Core accepts empty rpcuser, not specified in btc_conf_file
            conf = {'rpcuser': ""}

            # Extract contents of bitcoin.conf to build service_url
            try:
                with open(btc_conf_file, 'r') as fd:
                    for line in fd.readlines():
                        if '#' in line:
                            line = line[:line.index('#')]
                        if '=' not in line:
                            continue
                        k, v = line.split('=', 1)
                        conf[k.strip()] = v.strip()

            # Treat a missing bitcoin.conf as though it were empty
            except FileNotFoundError:
                pass

            if service_port is None:
                service_port = bitcoin.params.RPC_PORT
            conf['rpcport'] = int(conf.get('rpcport', service_port))
            conf['rpchost'] = conf.get('rpcconnect', 'localhost')

            service_url = ('%s://%s:%d' %
                ('http', conf['rpchost'], conf['rpcport']))

            cookie_dir = conf.get('datadir', os.path.dirname(btc_conf_file))
            if bitcoin.params.NAME != "mainnet":
                cookie_dir = os.path.join(cookie_dir, bitcoin.params.NAME)
            cookie_file = os.path.join(cookie_dir, ".cookie")
            try:
                with open(cookie_file, 'r') as fd:
                    authpair = fd.read()
            except IOError as err:
                if 'rpcpassword' in conf:
                    authpair = "%s:%s" % (conf['rpcuser'], conf['rpcpassword'])

                else:
                    raise ValueError('Cookie file unusable (%s) and rpcpassword not specified in the configuration file: %r' % (err, btc_conf_file))

        else:
            url = urlparse.urlparse(service_url)
            authpair = "%s:%s" % (url.username, url.password)

        self.__service_url = service_url
        self.__url = urlparse.urlparse(service_url)

        if self.__url.scheme not in ('http',):
            raise ValueError('Unsupported URL scheme %r' % self.__url.scheme)

        if self.__url.port is None:
            port = httplib.HTTP_PORT
        else:
            port = self.__url.port
        self.__id_count = 0

        if authpair is None:
            self.__auth_header = None
        else:
            authpair = authpair.encode('utf8')
            self.__auth_header = b"Basic " + base64.b64encode(authpair)

        if connection:
            self.__conn = connection
        else:
            self.__conn = httplib.HTTPConnection(self.__url.hostname, port=port,
                                                 timeout=timeout)

    def _call(self, service_name, *args):
        self.__id_count += 1

        postdata = json.dumps({'version': '1.1',
                               'method': service_name,
                               'params': args,
                               'id': self.__id_count})

        headers = {
            'Host': self.__url.hostname,
            'User-Agent': DEFAULT_USER_AGENT,
            'Content-type': 'application/json',
        }

        if self.__auth_header is not None:
            headers['Authorization'] = self.__auth_header

        self.__conn.request('POST', self.__url.path, postdata, headers)

        response = self._get_response()
        err = response.get('error')
        if err is not None:
            if isinstance(err, dict):
                raise JSONRPCError(
                    {'code': err.get('code', -345),
                     'message': err.get('message', 'error message not specified')})
            raise JSONRPCError({'code': -344, 'message': str(err)})
        elif 'result' not in response:
            raise JSONRPCError({
                'code': -343, 'message': 'missing JSON-RPC result'})
        else:
            return response['result']

    def _batch(self, rpc_call_list):
        postdata = json.dumps(list(rpc_call_list))

        headers = {
            'Host': self.__url.hostname,
            'User-Agent': DEFAULT_USER_AGENT,
            'Content-type': 'application/json',
        }

        if self.__auth_header is not None:
            headers['Authorization'] = self.__auth_header

        self.__conn.request('POST', self.__url.path, postdata, headers)
        return self._get_response()

    def _get_response(self):
        http_response = self.__conn.getresponse()
        if http_response is None:
            raise JSONRPCError({
                'code': -342, 'message': 'missing HTTP response from server'})

        rdata = http_response.read().decode('utf8')
        try:
            return json.loads(rdata, parse_float=decimal.Decimal)
        except Exception:
            raise JSONRPCError({
                'code': -342,
                'message': ('non-JSON HTTP response with \'%i %s\' from server: \'%.20s%s\''
                            % (http_response.status, http_response.reason,
                               rdata, '...' if len(rdata) > 20 else ''))})

    def close(self):
        if self.__conn is not None:
            self.__conn.close()

    def __del__(self):
        if self.__conn is not None:
            self.__conn.close()


class RawProxy(BaseProxy):
    """Low-level proxy to a bitcoin JSON-RPC service

    Unlike ``Proxy``, no conversion is done besides parsing JSON. As far as
    Python is concerned, you can call any method; ``JSONRPCError`` will be
    raised if the server does not recognize it.
    """
    def __init__(self,
                 service_url=None,
                 service_port=None,
                 btc_conf_file=None,
                 timeout=DEFAULT_HTTP_TIMEOUT,
                 **kwargs):
        super(RawProxy, self).__init__(service_url=service_url,
                                       service_port=service_port,
                                       btc_conf_file=btc_conf_file,
                                       timeout=timeout,
                                       **kwargs)

    def __getattr__(self, name):
        if name.startswith('__') and name.endswith('__'):
            # Prevent RPC calls for non-existing python internal attribute
            # access. If someone tries to get an internal attribute
            # of RawProxy instance, and the instance does not have this
            # attribute, we do not want the bogus RPC call to happen.
            raise AttributeError

        # Create a callable to do the actual call
        f = lambda *args: self._call(name, *args)

        # Make debuggers show <function bitcoin.rpc.name> rather than <function
        # bitcoin.rpc.<lambda>>
        f.__name__ = name
        return f


class Proxy(BaseProxy):
    """Proxy to a bitcoin RPC service

    Unlike ``RawProxy``, data is passed as ``bitcoin.core`` objects or packed
    bytes, rather than JSON or hex strings. Not all methods are implemented
    yet; you can use ``call`` to access missing ones in a forward-compatible
    way. Assumes Bitcoin Core version >= v0.16.0; older versions mostly work,
    but there are a few incompatibilities.
    """

    def __init__(self,
                 service_url=None,
                 service_port=None,
                 btc_conf_file=None,
                 timeout=DEFAULT_HTTP_TIMEOUT,
                 **kwargs):
        """Create a proxy object

        If ``service_url`` is not specified, the username and password are read
        out of the file ``btc_conf_file``. If ``btc_conf_file`` is not
        specified, ``~/.bitcoin/bitcoin.conf`` or equivalent is used by
        default.  The default port is set according to the chain parameters in
        use: mainnet, testnet, or regtest.

        Usually no arguments to ``Proxy()`` are needed; the local bitcoind will
        be used.

        ``timeout`` - timeout in seconds before the HTTP interface times out
        """

        super(Proxy, self).__init__(service_url=service_url,
                                    service_port=service_port,
                                    btc_conf_file=btc_conf_file,
                                    timeout=timeout,
                                    **kwargs)

    def call(self, service_name, *args):
        """Call an RPC method by name and raw (JSON encodable) arguments"""
        return self._call(service_name, *args)

    def dumpprivkey(self, addr):
        """Return the private key matching an address
        """
        r = self._call('dumpprivkey', str(addr))

        return CBitcoinSecret(r)

    def fundrawtransaction(self, tx, include_watching=False):
        """Add inputs to a transaction until it has enough in value to meet its out value.

        include_watching - Also select inputs which are watch only

        Returns dict:

        {'tx':        Resulting tx,
         'fee':       Fee the resulting transaction pays,
         'changepos': Position of added change output, or -1,
        }
        """
        hextx = hexlify(tx.serialize())
        r = self._call('fundrawtransaction', hextx, include_watching)

        r['tx'] = CTransaction.deserialize(unhexlify(r['hex']))
        del r['hex']

        r['fee'] = int(r['fee'] * COIN)

        return r

    def generate(self, numblocks):
        """
        DEPRECATED (will be removed in bitcoin-core v0.19)
        
        Mine blocks immediately (before the RPC call returns)

        numblocks - How many blocks are generated immediately.

        Returns iterable of block hashes generated.
        """
        r = self._call('generate', numblocks)
        return (lx(blk_hash) for blk_hash in r)
    
    def generatetoaddress(self, numblocks, addr):
        """Mine blocks immediately (before the RPC call returns) and
        allocate block reward to passed address. Replaces deprecated 
        "generate(self,numblocks)" method.

        numblocks - How many blocks are generated immediately.
        addr     - Address to receive block reward (CBitcoinAddress instance)

        Returns iterable of block hashes generated.
        """
        r = self._call('generatetoaddress', numblocks, str(addr))
        return (lx(blk_hash) for blk_hash in r)

    def getaccountaddress(self, account=None):
        """Return the current Bitcoin address for receiving payments to this
        account."""
        r = self._call('getaccountaddress', account)
        return CBitcoinAddress(r)

    def getbalance(self, account='*', minconf=1, include_watchonly=False):
        """Get the balance

        account - The selected account. Defaults to "*" for entire wallet. It
        may be the default account using "".

        minconf - Only include transactions confirmed at least this many times.
        (default=1)

        include_watchonly - Also include balance in watch-only addresses (see 'importaddress')
        (default=False)
        """
        r = self._call('getbalance', account, minconf, include_watchonly)
        return int(r*COIN)

    def getbestblockhash(self):
        """Return hash of best (tip) block in longest block chain."""
        return lx(self._call('getbestblockhash'))

    def getblockheader(self, block_hash, verbose=False):
        """Get block header <block_hash>

        verbose - If true a dict is returned with the values returned by
                  getblockheader that are not in the block header itself
                  (height, nextblockhash, etc.)

        Raises IndexError if block_hash is not valid.
        """
        try:
            block_hash = b2lx(block_hash)
        except TypeError:
            raise TypeError('%s.getblockheader(): block_hash must be bytes; got %r instance' %
                    (self.__class__.__name__, block_hash.__class__))
        try:
            r = self._call('getblockheader', block_hash, verbose)
        except InvalidAddressOrKeyError as ex:
            raise IndexError('%s.getblockheader(): %s (%d)' %
                    (self.__class__.__name__, ex.error['message'], ex.error['code']))

        if verbose:
            nextblockhash = None
            if 'nextblockhash' in r:
                nextblockhash = lx(r['nextblockhash'])
            return {'confirmations':r['confirmations'],
                    'height':r['height'],
                    'mediantime':r['mediantime'],
                    'nextblockhash':nextblockhash,
                    'chainwork':x(r['chainwork'])}
        else:
            return CBlockHeader.deserialize(unhexlify(r))


    def getblock(self, block_hash):
        """Get block <block_hash>

        Raises IndexError if block_hash is not valid.
        """
        try:
            block_hash = b2lx(block_hash)
        except TypeError:
            raise TypeError('%s.getblock(): block_hash must be bytes; got %r instance' %
                    (self.__class__.__name__, block_hash.__class__))
        try:
            # With this change ( https://github.com/bitcoin/bitcoin/commit/96c850c20913b191cff9f66fedbb68812b1a41ea#diff-a0c8f511d90e83aa9b5857e819ced344 ),
            # bitcoin core's rpc takes 0/1/2 instead of true/false as the 2nd argument which specifies verbosity, since v0.15.0.
            # The change above is backward-compatible so far; the old "false" is taken as the new "0".
            r = self._call('getblock', block_hash, False)
        except InvalidAddressOrKeyError as ex:
            raise IndexError('%s.getblock(): %s (%d)' %
                    (self.__class__.__name__, ex.error['message'], ex.error['code']))
        return CBlock.deserialize(unhexlify(r))

    def getblockcount(self):
        """Return the number of blocks in the longest block chain"""
        return self._call('getblockcount')

    def getblockhash(self, height):
        """Return hash of block in best-block-chain at height.

        Raises IndexError if height is not valid.
        """
        try:
            return lx(self._call('getblockhash', height))
        except InvalidParameterError as ex:
            raise IndexError('%s.getblockhash(): %s (%d)' %
                    (self.__class__.__name__, ex.error['message'], ex.error['code']))

    def getinfo(self):
        """Return a JSON object containing various state info"""
        r = self._call('getinfo')
        if 'balance' in r:
            r['balance'] = int(r['balance'] * COIN)
        if 'paytxfee' in r:
            r['paytxfee'] = int(r['paytxfee'] * COIN)
        return r

    def getmininginfo(self):
        """Return a JSON object containing mining-related information"""
        return self._call('getmininginfo')

    def getnewaddress(self, account=None):
        """Return a new Bitcoin address for receiving payments.

        If account is not None, it is added to the address book so payments
        received with the address will be credited to account.
        """
        r = None
        if account is not None:
            r = self._call('getnewaddress', account)
        else:
            r = self._call('getnewaddress')

        return CBitcoinAddress(r)

    def getrawchangeaddress(self):
        """Returns a new Bitcoin address, for receiving change.

        This is for use with raw transactions, NOT normal use.
        """
        r = self._call('getrawchangeaddress')
        return CBitcoinAddress(r)

    def getrawmempool(self, verbose=False):
        """Return the mempool"""
        if verbose:
            return self._call('getrawmempool', verbose)

        else:
            r = self._call('getrawmempool')
            r = [lx(txid) for txid in r]
            return r

    def getrawtransaction(self, txid, verbose=False):
        """Return transaction with hash txid

        Raises IndexError if transaction not found.

        verbose - If true a dict is returned instead with additional
        information on the transaction.

        Note that if all txouts are spent and the transaction index is not
        enabled the transaction may not be available.
        """
        try:
            r = self._call('getrawtransaction', b2lx(txid), 1 if verbose else 0)
        except InvalidAddressOrKeyError as ex:
            raise IndexError('%s.getrawtransaction(): %s (%d)' %
                    (self.__class__.__name__, ex.error['message'], ex.error['code']))
        if verbose:
            r['tx'] = CTransaction.deserialize(unhexlify(r['hex']))
            del r['hex']
            del r['txid']
            del r['version']
            del r['locktime']
            del r['vin']
            del r['vout']
            r['blockhash'] = lx(r['blockhash']) if 'blockhash' in r else None
        else:
            r = CTransaction.deserialize(unhexlify(r))

        return r

    def getreceivedbyaddress(self, addr, minconf=1):
        """Return total amount received by given a (wallet) address

        Get the amount received by <address> in transactions with at least
        [minconf] confirmations.

        Works only for addresses in the local wallet; other addresses will
        always show zero.

        addr    - The address. (CBitcoinAddress instance)

        minconf - Only include transactions confirmed at least this many times.
        (default=1)
        """
        r = self._call('getreceivedbyaddress', str(addr), minconf)
        return int(r * COIN)

    def gettransaction(self, txid):
        """Get detailed information about in-wallet transaction txid

        Raises IndexError if transaction not found in the wallet.

        FIXME: Returned data types are not yet converted.
        """
        try:
            r = self._call('gettransaction', b2lx(txid))
        except InvalidAddressOrKeyError as ex:
            raise IndexError('%s.getrawtransaction(): %s (%d)' %
                    (self.__class__.__name__, ex.error['message'], ex.error['code']))
        return r

    def gettxout(self, outpoint, includemempool=True):
        """Return details about an unspent transaction output.

        Raises IndexError if outpoint is not found or was spent.

        includemempool - Include mempool txouts
        """
        r = self._call('gettxout', b2lx(outpoint.hash), outpoint.n, includemempool)

        if r is None:
            raise IndexError('%s.gettxout(): unspent txout %r not found' % (self.__class__.__name__, outpoint))

        r['txout'] = CTxOut(int(r['value'] * COIN),
                            CScript(unhexlify(r['scriptPubKey']['hex'])))
        del r['value']
        del r['scriptPubKey']
        r['bestblock'] = lx(r['bestblock'])
        return r

    def importaddress(self, addr, label='', rescan=True):
        """Adds an address or pubkey to wallet without the associated privkey."""
        addr = str(addr)

        r = self._call('importaddress', addr, label, rescan)
        return r

    def listunspent(self, minconf=0, maxconf=9999999, addrs=None):
        """Return unspent transaction outputs in wallet

        Outputs will have between minconf and maxconf (inclusive)
        confirmations, optionally filtered to only include txouts paid to
        addresses in addrs.
        """
        r = None
        if addrs is None:
            r = self._call('listunspent', minconf, maxconf)
        else:
            addrs = [str(addr) for addr in addrs]
            r = self._call('listunspent', minconf, maxconf, addrs)

        r2 = []
        for unspent in r:
            unspent['outpoint'] = COutPoint(lx(unspent['txid']), unspent['vout'])
            del unspent['txid']
            del unspent['vout']

            # address isn't always available as Bitcoin Core allows scripts w/o
            # an address type to be imported into the wallet, e.g. non-p2sh
            # segwit
            try:
                unspent['address'] = CBitcoinAddress(unspent['address'])
            except KeyError:
                pass
            unspent['scriptPubKey'] = CScript(unhexlify(unspent['scriptPubKey']))
            unspent['amount'] = int(unspent['amount'] * COIN)
            r2.append(unspent)
        return r2

    def lockunspent(self, unlock, outpoints):
        """Lock or unlock outpoints"""
        json_outpoints = [{'txid':b2lx(outpoint.hash), 'vout':outpoint.n}
                          for outpoint in outpoints]
        return self._call('lockunspent', unlock, json_outpoints)

    def sendrawtransaction(self, tx, allowhighfees=False):
        """Submit transaction to local node and network.

        allowhighfees - Allow even if fees are unreasonably high.
        """
        hextx = hexlify(tx.serialize())
        r = None
        if allowhighfees:
            r = self._call('sendrawtransaction', hextx, True)
        else:
            r = self._call('sendrawtransaction', hextx)
        return lx(r)

    def sendmany(self, fromaccount, payments, minconf=1, comment='', subtractfeefromamount=[]):
        """Send amount to given addresses.

        payments - dict with {address: amount}
        """
        json_payments = {str(addr):float(amount)/COIN
                         for addr, amount in payments.items()}
        r = self._call('sendmany', fromaccount, json_payments, minconf, comment, subtractfeefromamount)
        return lx(r)

    def sendtoaddress(self, addr, amount, comment='', commentto='', subtractfeefromamount=False):
        """Send amount to a given address"""
        addr = str(addr)
        amount = float(amount)/COIN
        r = self._call('sendtoaddress', addr, amount, comment, commentto, subtractfeefromamount)
        return lx(r)

    def signrawtransaction(self, tx, *args):
        """Sign inputs for transaction

        FIXME: implement options
        """
        hextx = hexlify(tx.serialize())
        r = self._call('signrawtransaction', hextx, *args)
        r['tx'] = CTransaction.deserialize(unhexlify(r['hex']))
        del r['hex']
        return r

    def signrawtransactionwithwallet(self, tx, *args):
        """Sign inputs for transaction
            bicoincore >= 0.17.x

        FIXME: implement options
        """
        hextx = hexlify(tx.serialize())
        r = self._call('signrawtransactionwithwallet', hextx, *args)
        r['tx'] = CTransaction.deserialize(unhexlify(r['hex']))
        del r['hex']
        return r

    def submitblock(self, block, params=None):
        """Submit a new block to the network.

        params is optional and is currently ignored by bitcoind. See
        https://en.bitcoin.it/wiki/BIP_0022 for full specification.
        """
        hexblock = hexlify(block.serialize())
        if params is not None:
            return self._call('submitblock', hexblock, params)
        else:
            return self._call('submitblock', hexblock)

    def validateaddress(self, address):
        """Return information about an address"""
        r = self._call('validateaddress', str(address))
        if r['isvalid']:
            r['address'] = CBitcoinAddress(r['address'])
        if 'pubkey' in r:
            r['pubkey'] = unhexlify(r['pubkey'])
        return r

    def unlockwallet(self, password, timeout=60):
        """Stores the wallet decryption key in memory for 'timeout' seconds.

        password - The wallet passphrase.

        timeout - The time to keep the decryption key in seconds.
        (default=60)
        """
        r = self._call('walletpassphrase', password, timeout)
        return r

    def _addnode(self, node, arg):
        r = self._call('addnode', node, arg)
        return r

    def addnode(self, node):
        return self._addnode(node, 'add')

    def addnodeonetry(self, node):
        return self._addnode(node, 'onetry')

    def removenode(self, node):
        return self._addnode(node, 'remove')

__all__ = (
    'JSONRPCError',
    'ForbiddenBySafeModeError',
    'InvalidAddressOrKeyError',
    'InvalidParameterError',
    'VerifyError',
    'VerifyRejectedError',
    'VerifyAlreadyInChainError',
    'InWarmupError',
    'RawProxy',
    'Proxy',
)
