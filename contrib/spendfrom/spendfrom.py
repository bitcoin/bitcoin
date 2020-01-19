#!/usr/bin/env python3
#
# Use the raw transactions API to spend crowns received on particular addresses,
# and send any change back to that same address.
#
# Example usage:
#  spendfrom.py  # Lists available funds
#  spendfrom.py --from=ADDRESS --to=ADDRESS --amount=11.00
#
# Assumes it will talk to a crownd or Crown-Qt running
# on localhost.
#
# Depends on bitcoinrpc
#

from decimal import *
import getpass
import math
import os
import os.path
import platform
import sys
import time
from bitcoinrpc.authproxy import AuthServiceProxy, json, JSONRPCException

BASE_FEE=Decimal("0.001")
verbosity = 0

def check_json_precision():
    """Make sure json library being used does not lose precision converting BTC values"""
    n = Decimal("20000000.00000003")
    satoshis = int(json.loads(json.dumps(float(n)))*1.0e8)
    if satoshis != 2000000000000003:
        raise RuntimeError("JSON encode/decode loses precision")

def determine_db_dir():
    """Return the default location of the crown data directory"""
    if platform.system() == "Darwin":
        return os.path.expanduser("~/Library/Application Support/Crown/")
    elif platform.system() == "Windows":
        return os.path.join(os.environ['APPDATA'], "Crown")
    return os.path.expanduser("~/.crown")

def read_bitcoin_config(dbdir, conffile):
    """Read the crown config file from dbdir, returns dictionary of settings"""
    with open(os.path.join(dbdir, conffile)) as stream:
        config = dict(line.strip().split('=', 1) for line in stream if not line.startswith("#") and not len(line.strip()) == 0)
    return config

def connect_JSON(config):
    """Connect to a crown JSON-RPC server"""
    testnet = config.get('testnet', '0')
    testnet = (int(testnet) > 0)  # 0/1 in config file, convert to True/False
    if not 'rpcport' in config:
        config['rpcport'] = 19341 if testnet else 9341
    connect = "http://%s:%s@127.0.0.1:%s"%(config['rpcuser'], config['rpcpassword'], config['rpcport'])
    try:
        result = AuthServiceProxy(connect, timeout=600)
        # ServiceProxy is lazy-connect, so send an RPC command mostly to catch connection errors,
        # but also make sure the crownd we're talking to is/isn't testnet:
        if result.getmininginfo()['testnet'] != testnet:
            sys.stderr.write("RPC server at "+connect+" testnet setting mismatch\n")
            sys.exit(1)
        return result
    except:
        sys.stderr.write("Error connecting to RPC server at "+connect+"\n")
        sys.exit(1)

def unlock_wallet(crownd):
    info = crownd.getinfo()
    if 'unlocked_until' not in info:
        return True # wallet is not encrypted
    t = int(info['unlocked_until'])
    if t <= time.time():
        try:
            passphrase = getpass.getpass("Wallet is locked; enter passphrase: ")
            crownd.walletpassphrase(passphrase, 5)
        except:
            sys.stderr.write("Wrong passphrase\n")

    info = crownd.getinfo()
    return int(info['unlocked_until']) > time.time()

def list_available(crownd):
    address_summary = dict()

    address_to_account = dict()
    for info in crownd.listreceivedbyaddress(0):
        address_to_account[info["address"]] = info["account"]

    unspent = crownd.listunspent(0)
    for output in unspent:
        # listunspent doesn't give addresses, so:
        rawtx = crownd.getrawtransaction(output['txid'], 1)
        vout = rawtx["vout"][output['vout']]
        pk = vout["scriptPubKey"]

        # This code only deals with ordinary pay-to-crown-address
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

def select_coins(needed, inputs, criteria):
    # criteria will be used to prioritise the coins selected for this txn.
    # Some alternative strategies are oldest|smallest|largest UTXO first.
    # Using smallest first will combine the largest number of UTXOs but may 
    # produce a transaction which is too large. The input set is an unordered
    # list looking something like
    # [{'vout': 1, 'address': 'tCRWTQhoMTQgyHZyrhZSnScJYuGXCHXPKdNwd', 'confirmations': 12086, 'spendable': True, 'amount': Decimal('0.92500000'), 'scriptPubKey': '76a9149e2b5779df2364dc833fd5167bc561ce65f3884b88ac', 'txid': 'ef4d44dc28b818eb09651f1e62f348aa66a81b6e2baf5242ba4f48ab68a78aca', 'account': 'XMN05'}, 
    #  {'vout': 1, 'address': 'tCRWTQhoMTQgyHZyrhZSnScJYuGXCHXPKdNwd', 'confirmations': 5906, 'spendable': True, 'amount': Decimal('0.92500000'), 'scriptPubKey': '76a9149e2b5779df2364dc833fd5167bc561ce65f3884b88ac', 'txid': 'c1142434fa1fb3484bd54b42fd654fe86a7c2de9ec62f049b868db1439b591ca', 'account': 'XMN05'}, 
    #  {'vout': 0, 'address': 'tCRWTQhoMTQgyHZyrhZSnScJYuGXCHXPKdNwd', 'confirmations': 1395, 'spendable': True, 'amount': Decimal('0.75000000'), 'scriptPubKey': '76a9149e2b5779df2364dc833fd5167bc561ce65f3884b88ac', 'txid': '74205c8acdfeffb57fba501676e7ae14fff4a6dc06843ad008eb841f7ae198ca', 'account': 'XMN05'}, 
    #  {'vout': 0, 'address': 'tCRWTQhoMTQgyHZyrhZSnScJYuGXCHXPKdNwd', 'confirmations': 9531, 'spendable': True, 'amount': Decimal('0.75000000'), 'scriptPubKey': '76a9149e2b5779df2364dc833fd5167bc561ce65f3884b88ac', 'txid': 'f5c19fe103d72e9257a54e3562a85a6d6309d8a0d419aee7228a2c69e02e9cca', 'account': 'XMN05'}
    #  ...
    outputs = []
    have = Decimal("0.0")
    n = 0
    if verbosity > 0: print("Selecting coins from the set of %d inputs"%len(inputs))
    if verbosity > 1: print(inputs)
    while have < needed and n < len(inputs) and n < 1660:
        outputs.append({ "txid":inputs[n]["txid"], "vout":inputs[n]["vout"]})
        have += inputs[n]["amount"]
        n += 1
    if verbosity > 0: print("Chose %d UTXOs with total value %f CRW requiring %f CRW change"%(n, have, have-needed)) 
    if verbosity > 2: print(outputs)   
    return (outputs, have-needed)

def create_tx(crownd, fromaddresses, toaddress, amount, fee, criteria, upto):
    all_coins = list_available(crownd)

    total_available = Decimal("0.0")
    needed = amount+fee
    potential_inputs = []
    for addr in fromaddresses:
        if addr not in all_coins:
            continue
        potential_inputs.extend(all_coins[addr]["outputs"])
        total_available += all_coins[addr]["total"]

    if total_available == 0:
        sys.stderr.write("Selected addresses are empty\n")
        sys.exit(1)
    elif total_available < needed:
        if upto:
            needed = total_available
            amount = total_available - fee
            print("Warning, only %f CRW available, sending up to %f CRW"%(total_available, amount))
        else:
            sys.stderr.write("Error, only %f CRW available, need %f CRW\n"%(total_available, needed))
            sys.exit(1)
        
    #
    # Note:
    # Python's json/jsonrpc modules have inconsistent support for Decimal numbers.
    # Instead of wrestling with getting json.dumps() (used by jsonrpc) to encode
    # Decimals, I'm casting amounts to float before sending them to crownd.
    #
    outputs = { toaddress : float(amount) }
    (inputs, change_amount) = select_coins(needed, potential_inputs, criteria)
    if change_amount < 0:         # hit the transaction UTXO limit
        amount += change_amount
        outputs[toaddress] = float(amount)
        if not upto:
            sys.stderr.write("Error, only %f CRW available, need %f\n"%(amount + fee, needed))
            sys.exit(1)
    elif change_amount > BASE_FEE:  # don't bother with zero or tiny change
        change_address = fromaddresses[-1]
        if change_address in outputs:
            outputs[change_address] += float(change_amount)
        else:
            outputs[change_address] = float(change_amount)

    rawtx = crownd.createrawtransaction(inputs, outputs)
    signed_rawtx = crownd.signrawtransaction(rawtx)
    if not signed_rawtx["complete"]:
        sys.stderr.write("signrawtransaction failed\n")
        sys.exit(1)
    txdata = signed_rawtx["hex"]

    return txdata

def compute_amount_in(crownd, txinfo):
    result = Decimal("0.0")
    for vin in txinfo['vin']:
        in_info = crownd.getrawtransaction(vin['txid'], 1)
        vout = in_info['vout'][vin['vout']]
        result = result + vout['value']
    return result

def compute_amount_out(txinfo):
    result = Decimal("0.0")
    for vout in txinfo['vout']:
        result = result + vout['value']
    return result

def sanity_test_fee(crownd, txdata_hex, max_fee, fee):
    class FeeError(RuntimeError):
        pass
    try:
        txinfo = crownd.decoderawtransaction(txdata_hex)
        total_in = compute_amount_in(crownd, txinfo)
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
    global verbosity

    parser = optparse.OptionParser(usage="%prog [options]")
    parser.add_option("--from", dest="fromaddresses", default=None,
                      help="(comma separated list of) address(es) to get CRW from")
    parser.add_option("--to", dest="toaddress", default=None,
                      help="address to send CRW to. Specify 'new' for a new address in the local wallet")
    parser.add_option("--amount", dest="amount", default=None,
                      help="amount to send excluding fee")
    parser.add_option("--fee", dest="fee", default="0.025",
                      help="amount of fee to pay")
    parser.add_option("--config", dest="conffile", default="crown.conf",
                      help="name of crown config file (default: %default)")
    parser.add_option("--datadir", dest="datadir", default=determine_db_dir(),
                      help="location of crown config file with RPC username/password (default: %default)")
    parser.add_option("--select", type='choice', 
                      choices=['oldest', 'smallest', 'largest'],
                      default=None,
                      help="select the oldest|smallest|largest UTXOs first. Not yet operational")
    parser.add_option("--upto", dest="upto", default=False, action="store_true",
                      help="allow inexact send up to the specified amount")
    parser.add_option("--testnet", dest="testnet", default=False, action="store_true",
                      help="use the test network")
    parser.add_option("--dry_run", dest="dry_run", default=False, action="store_true",
                      help="don't broadcast the transaction, just create and print the transaction data")
    parser.add_option("-v", action="count", dest="verbosity", default=0,
                      help="increase the verbosity level. Use more than one for extra verbosity")
    
    (options, args) = parser.parse_args()

    verbosity = options.verbosity

    check_json_precision()
    config = read_bitcoin_config(options.datadir, options.conffile)
    if options.testnet: config['testnet'] = True
    crownd = connect_JSON(config)

    if options.amount is None:
        spendable_amount = 0
        utxo_count = 0
        address_summary = list_available(crownd)
        if address_summary.items():
            for address,info in address_summary.items():
                n_transactions = len(info['outputs'])
                if n_transactions > 1:
                    print("%s %.8f %s (%d UTXOs)"%(address, info['total'], info['account'], n_transactions))
                else:
                    print("%s %.8f %s"%(address, info['total'], info['account']))
                spendable_amount += info['total']
                utxo_count += n_transactions
            print('-------------------------------------------------------------------------------')
            print('Total spendable: %.8f CRW in %d UTXOs and %d addresses'%(spendable_amount, utxo_count, len(address_summary.items())))
        else:
            print("Nothing to spend!")

    else:
        if options.toaddress is None:
            print("You must specify a to address")
            sys.exit(1)
        if options.toaddress.lower() == 'new':
            options.toaddress = crownd.getnewaddress('')
            print("Sending to new address %s"%(options.toaddress))

        if not crownd.validateaddress(options.toaddress)['isvalid']:
            print("To address is invalid")
            sys.exit(1)
        else:    
            fee = Decimal(options.fee)
            amount = Decimal(options.amount)
            while unlock_wallet(crownd) == False:
                pass # Keep asking for passphrase until they get it right
            txdata = create_tx(crownd, options.fromaddresses.split(","), options.toaddress, amount, fee, options.select, options.upto)
            sanity_test_fee(crownd, txdata, amount*Decimal("0.01"), fee)
            txlen = len(txdata)/2
            if verbosity > 0: print("Transaction size is %d bytes"%(txlen))
            if options.dry_run:
                print("Raw transaction data: %s"%(txdata))
                if verbosity > 2: print("Decoded transaction: %s"%(crownd.decoderawtransaction(txdata)))
            elif txlen < 250000:
                txid = crownd.sendrawtransaction(txdata)
                print(txid)
            else:
                print("Transaction size is too large")

if __name__ == '__main__':
    main()
