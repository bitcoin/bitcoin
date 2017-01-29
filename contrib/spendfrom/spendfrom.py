#!/usr/bin/env python
# Copyright (c) 2013 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Use the raw transactions API to spend bitcoins received on particular addresses,
# and send any change back to that same address.
#
# Example usage:
#  spendfrom.py  # Lists available funds
#  spendfrom.py --from=ADDRESS --to=ADDRESS --amount=11.00
#
# Assumes it will talk to a bitcoind or Bitcoin-Qt running
# on localhost.
#
# Depends on jsonrpc
#

from decimal import *
import getpass
import math
import os
import os.path
import platform
import sys
import time
from jsonrpc import ServiceProxy, json

BASE_FEE=Decimal("0.001")

def check_json_precision():
    """Make sure json library being used does not lose precision converting BTC values"""
    n = Decimal("20000000.00000003")
    satoshis = int(json.loads(json.dumps(float(n)))*1.0e8)
    if satoshis != 2000000000000003:
        raise RuntimeError("JSON encode/decode loses precision")

def determine_db_dir():
    """Return the default location of the bitcoin data directory"""
    if platform.system() == "Darwin":
        return os.path.expanduser("~/Library/Application Support/Bitcoin/")
    elif platform.system() == "Windows":
        return os.path.join(os.environ['APPDATA'], "Bitcoin")
    return os.path.expanduser("~/.bitcoin")

def read_bitcoin_config(dbdir):
    """Read the bitcoin.conf file from dbdir, returns dictionary of settings"""
    from ConfigParser import SafeConfigParser

    class FakeSecHead(object):
        def __init__(self, fp):
            self.fp = fp
            self.sechead = '[all]\n'
        def readline(self):
            if self.sechead:
                try: return self.sechead
                finally: self.sechead = None
            else:
                s = self.fp.readline()
                if s.find('#') != -1:
                    s = s[0:s.find('#')].strip() +"\n"
                return s

    config_parser = SafeConfigParser()
    config_parser.readfp(FakeSecHead(open(os.path.join(dbdir, "bitcoin.conf"))))
    return dict(config_parser.items("all"))

def connect_JSON(config):
    """Connect to a bitcoin JSON-RPC server"""
    testnet = config.get('testnet', '0')
    testnet = (int(testnet) > 0)  # 0/1 in config file, convert to True/False
    if not 'rpcport' in config:
        config['rpcport'] = 19332 if testnet else 9332
    connect = "http://%s:%s@127.0.0.1:%s"%(config['rpcuser'], config['rpcpassword'], config['rpcport'])
    try:
        result = ServiceProxy(connect)
        # ServiceProxy is lazy-connect, so send an RPC command mostly to catch connection errors,
        # but also make sure the bitcoind we're talking to is/isn't testnet:
        if result.getmininginfo()['testnet'] != testnet:
            sys.stderr.write("RPC server at "+connect+" testnet setting mismatch\n")
            sys.exit(1)
        return result
    except:
        sys.stderr.write("Error connecting to RPC server at "+connect+"\n")
        sys.exit(1)

def unlock_wallet(bitcoind):
    info = bitcoind.getinfo()
    if 'unlocked_until' not in info:
        return True # wallet is not encrypted
    t = int(info['unlocked_until'])
    if t <= time.time():
        try:
            passphrase = getpass.getpass("Wallet is locked; enter passphrase: ")
            bitcoind.walletpassphrase(passphrase, 5)
        except:
            sys.stderr.write("Wrong passphrase\n")

    info = bitcoind.getinfo()
    return int(info['unlocked_until']) > time.time()

def list_available(bitcoind):
    address_summary = dict()

    address_to_account = dict()
    for info in bitcoind.listreceivedbyaddress(0):
        address_to_account[info["address"]] = info["account"]

    unspent = bitcoind.listunspent(0)
    for output in unspent:
        # listunspent doesn't give addresses, so:
        rawtx = bitcoind.getrawtransaction(output['txid'], 1)
        vout = rawtx["vout"][output['vout']]
        pk = vout["scriptPubKey"]

        # This code only deals with ordinary pay-to-bitcoin-address
        # or pay-to-script-hash outputs right now; anything exotic is ignored.
        if pk["type"] != "pubkeyhash" and pk["type"] != "scripthash":
            continue
        
        address = pk["addresses"][0]
        if address in address_summary:
            address_summary[address]["total"] += vout["value"]
            address_summary[address]["outputs"].append(output)
        else:
            address_summary[address] = {
                "total" : vout["value"],
                "outputs" : [output],
                "account" : address_to_account.get(address, "")
                }

    return address_summary

def select_coins(needed, inputs):
    # Feel free to improve this, this is good enough for my simple needs:
    outputs = []
    have = Decimal("0.0")
    n = 0
    while have < needed and n < len(inputs):
        outputs.append({ "txid":inputs[n]["txid"], "vout":inputs[n]["vout"]})
        have += inputs[n]["amount"]
        n += 1
    return (outputs, have-needed)

def create_tx(bitcoind, fromaddresses, toaddress, amount, fee):
    all_coins = list_available(bitcoind)

    total_available = Decimal("0.0")
    needed = amount+fee
    potential_inputs = []
    for addr in fromaddresses:
        if addr not in all_coins:
            continue
        potential_inputs.extend(all_coins[addr]["outputs"])
        total_available += all_coins[addr]["total"]

    if total_available < needed:
        sys.stderr.write("Error, only %f BTC available, need %f\n"%(total_available, needed));
        sys.exit(1)

    #
    # Note:
    # Python's json/jsonrpc modules have inconsistent support for Decimal numbers.
    # Instead of wrestling with getting json.dumps() (used by jsonrpc) to encode
    # Decimals, I'm casting amounts to float before sending them to bitcoind.
    #  
    outputs = { toaddress : float(amount) }
    (inputs, change_amount) = select_coins(needed, potential_inputs)
    if change_amount > BASE_FEE:  # don't bother with zero or tiny change
        change_address = fromaddresses[-1]
        if change_address in outputs:
            outputs[change_address] += float(change_amount)
        else:
            outputs[change_address] = float(change_amount)

    rawtx = bitcoind.createrawtransaction(inputs, outputs)
    signed_rawtx = bitcoind.signrawtransaction(rawtx)
    if not signed_rawtx["complete"]:
        sys.stderr.write("signrawtransaction failed\n")
        sys.exit(1)
    txdata = signed_rawtx["hex"]

    return txdata

def compute_amount_in(bitcoind, txinfo):
    result = Decimal("0.0")
    for vin in txinfo['vin']:
        in_info = bitcoind.getrawtransaction(vin['txid'], 1)
        vout = in_info['vout'][vin['vout']]
        result = result + vout['value']
    return result

def compute_amount_out(txinfo):
    result = Decimal("0.0")
    for vout in txinfo['vout']:
        result = result + vout['value']
    return result

def sanity_test_fee(bitcoind, txdata_hex, max_fee):
    class FeeError(RuntimeError):
        pass
    try:
        txinfo = bitcoind.decoderawtransaction(txdata_hex)
        total_in = compute_amount_in(bitcoind, txinfo)
        total_out = compute_amount_out(txinfo)
        if total_in-total_out > max_fee:
            raise FeeError("Rejecting transaction, unreasonable fee of "+str(total_in-total_out))

        tx_size = len(txdata_hex)/2
        kb = tx_size/1000  # integer division rounds down
        if kb > 1 and fee < BASE_FEE:
            raise FeeError("Rejecting no-fee transaction, larger than 1000 bytes")
        if total_in < 0.01 and fee < BASE_FEE:
            raise FeeError("Rejecting no-fee, tiny-amount transaction")
        # Exercise for the reader: compute transaction priority, and
        # warn if this is a very-low-priority transaction

    except FeeError as err:
        sys.stderr.write((str(err)+"\n"))
        sys.exit(1)

def main():
    import optparse

    parser = optparse.OptionParser(usage="%prog [options]")
    parser.add_option("--from", dest="fromaddresses", default=None,
                      help="addresses to get bitcoins from")
    parser.add_option("--to", dest="to", default=None,
                      help="address to get send bitcoins to")
    parser.add_option("--amount", dest="amount", default=None,
                      help="amount to send")
    parser.add_option("--fee", dest="fee", default="0.0",
                      help="fee to include")
    parser.add_option("--datadir", dest="datadir", default=determine_db_dir(),
                      help="location of bitcoin.conf file with RPC username/password (default: %default)")
    parser.add_option("--testnet", dest="testnet", default=False, action="store_true",
                      help="Use the test network")
    parser.add_option("--dry_run", dest="dry_run", default=False, action="store_true",
                      help="Don't broadcast the transaction, just create and print the transaction data")

    (options, args) = parser.parse_args()

    check_json_precision()
    config = read_bitcoin_config(options.datadir)
    if options.testnet: config['testnet'] = True
    bitcoind = connect_JSON(config)

    if options.amount is None:
        address_summary = list_available(bitcoind)
        for address,info in address_summary.iteritems():
            n_transactions = len(info['outputs'])
            if n_transactions > 1:
                print("%s %.8f %s (%d transactions)"%(address, info['total'], info['account'], n_transactions))
            else:
                print("%s %.8f %s"%(address, info['total'], info['account']))
    else:
        fee = Decimal(options.fee)
        amount = Decimal(options.amount)
        while unlock_wallet(bitcoind) == False:
            pass # Keep asking for passphrase until they get it right
        txdata = create_tx(bitcoind, options.fromaddresses.split(","), options.to, amount, fee)
        sanity_test_fee(bitcoind, txdata, amount*Decimal("0.01"))
        if options.dry_run:
            print(txdata)
        else:
            txid = bitcoind.sendrawtransaction(txdata)
            print(txid)

if __name__ == '__main__':
    main()
