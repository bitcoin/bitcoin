#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import argparse
import subprocess
import sys
import requests

DEFAULT_GLOBAL_FAUCET = 'https://signetfaucet.com/claim'
GLOBAL_FIRST_BLOCK_HASH = '00000086d6b2636cb2a392d45edc4ec544a10024d30141c9adf4bfd9de533b53'

parser = argparse.ArgumentParser(description='Script to get coins from a faucet.', epilog='You may need to start with double-dash (--) when providing bitcoin-cli arguments.')
parser.add_argument('-c', '--cmd', dest='cmd', default='bitcoin-cli', help='bitcoin-cli command to use')
parser.add_argument('-f', '--faucet', dest='faucet', default=DEFAULT_GLOBAL_FAUCET, help='URL of the faucet')
parser.add_argument('-a', '--addr', dest='addr', default='', help='Bitcoin address to which the faucet should send')
parser.add_argument('-p', '--password', dest='password', default='', help='Faucet password, if any')
parser.add_argument('bitcoin_cli_args', nargs='*', help='Arguments to pass on to bitcoin-cli (default: -signet)')

args = parser.parse_args()

if args.bitcoin_cli_args == []:
    args.bitcoin_cli_args = ['-signet']


def bitcoin_cli(rpc_command_and_params):
    argv = [args.cmd] + args.bitcoin_cli_args + rpc_command_and_params
    try:
        return subprocess.check_output(argv).strip().decode()
    except FileNotFoundError:
        print('The binary', args.cmd, 'could not be found.')
        exit()
    except subprocess.CalledProcessError:
        cmdline = ' '.join(argv)
        print(f'-----\nError while calling "{cmdline}" (see output above).')
        exit()


if args.faucet.lower() == DEFAULT_GLOBAL_FAUCET:
    # Get the hash of the block at height 1 of the currently active signet chain
    curr_signet_hash = bitcoin_cli(['getblockhash', '1'])
    if curr_signet_hash != GLOBAL_FIRST_BLOCK_HASH:
        print('The global faucet cannot be used with a custom Signet network. Please use the global signet or setup your custom faucet to use this functionality.\n')
        exit()

if args.addr == '':
    # get address for receiving coins
    args.addr = bitcoin_cli(['getnewaddress', 'faucet', 'bech32'])

data = {'address': args.addr, 'password': args.password}
try:
    res = requests.post(args.faucet, data=data)
except:
    print('Unexpected error when contacting faucet:', sys.exc_info()[0])
    exit()

# Display the output as per the returned status code
if res:
    # When the return code is in between 200 and 400 i.e. successful
    print(res.text)
elif res.status_code == 404:
    print('The specified faucet URL does not exist. Please check for any server issues/typo.')
elif res.status_code == 429:
    print('The script does not allow for repeated transactions as the global faucet is rate-limitied to 1 request/IP/day. You can access the faucet website to get more coins manually')
else:
    print(f'Returned Error Code {res.status_code}\n{res.text}\n')
    print('Please check the provided arguments for their validity and/or any possible typo.')
