#!/usr/bin/env python
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
import math
import os
import os.path
import platform
import sys
import time
from jsonrpc import ServiceProxy

BASE_FEE=Decimal("0.001")

def determine_db_dir():
    if platform.system() == "Darwin":
        return os.path.expanduser("~/Library/Application Support/Bitcoin/")
    elif platform.system() == "Windows":
        return os.path.join(os.environ['APPDATA'], "Bitcoin")
    return os.path.expanduser("~/.bitcoin")

def read_bitcoinconfig(dbdir):
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

def connect_JSON(datadir, testnet):
    conf = read_bitcoinconfig(datadir)
    if not 'rpcport' in conf:
        if 'testnet' in conf or testnet:
            conf['rpcport'] = 18332
        else:
            conf['rpcport'] = 8332

    connect = "http://%s:%s@127.0.0.1:%s"%(conf['rpcuser'], conf['rpcpassword'], conf['rpcport'])
    return ServiceProxy(connect)

def unlock_wallet(bitcoind):
    info = bitcoind.getinfo()
    if 'unlocked_until' not in info:
        return  # wallet is not encrypted
    t = int(info['unlocked_until'])
    if t <= time.time():
        print("wallet locked: passphrase?")
        passphrase = input()
        bitcoind.walletpassphrase(passphrase, 5)

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
        # outputs right now; anything exotic is ignored.
        if pk["type"] != "pubkeyhash":
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
        print("Error, only %f BTC available, need %f"%(total_available, needed));
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
        outputs[fromaddresses[-1]] = float(change_amount)

    rawtx = bitcoind.createrawtransaction(inputs, outputs)
    signed_rawtx = bitcoind.signrawtransaction(rawtx)
    if not signed_rawtx["complete"]:
        print("signrawtransaction failed")
        sys.exit(1)
    txdata = signed_rawtx["hex"]

    # This code could be a lot smarter, too:
    kb = len(signed_rawtx)/1000  # integer division rounds down
    if kb > 1 and fee < 0.001:
        print("Rejecting no-fee transaction, larger than 1000 bytes")
        print("Suggested fee: %f"%(BASE_FEE * kb))
        sys.exit(1)
    if amount < 0.01 and fee < 0.001:
        print("Rejecting no-fee, tiny-amount transaction")
        print("Suggested fee: %f"%(BASE_FEE * kb))
        sys.exit(1)

    return txdata

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

    bitcoind = connect_JSON(options.datadir, options.testnet)

    if options.amount is None:
        address_summary = list_available(bitcoind)
        for address,info in address_summary.iteritems():
            n_transactions = len(info['outputs'])
            if n_transactions > 1:
                print("%s %.4f %s (%d transactions)"%(address, info['total'], info['account'], n_transactions))
            else:
                print("%s %.4f %s"%(address, info['total'], info['account']))
    else:
        # Sanity check fee (paying more than 1% of amount):
        fee = Decimal(options.fee)
        amount = Decimal(options.amount)
        if fee > amount*Decimal("0.01"):
            print("Rejecting excessively large fee")
            sys.exit(1);
        unlock_wallet(bitcoind)
        txdata = create_tx(bitcoind, options.fromaddresses.split(","), options.to, amount, fee)
        if options.dry_run:
            print(txdata)
        else:
            txid = bitcoind.sendrawtransaction(txdata)
            print(txid)

if __name__ == '__main__':
    main()
