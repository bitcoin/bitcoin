#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
import sys
import argparse
import json

def perform_pre_checks():
    mock_result_path = os.path.join(os.getcwd(), "mock_result")
    if(os.path.isfile(mock_result_path)):
        f = open(mock_result_path, "r", encoding="utf8")
        mock_result = f.read()
        f.close()
        if mock_result[0]:
            sys.exit(int(mock_result[0]))

def enumerate(args):
  sys.stdout.write(json.dumps([{"fingerprint": "00000001"}, {"fingerprint": "00000002"}]))

def getdescriptors(args):
    xpub = "tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B"

    sys.stdout.write(json.dumps({
        "receive": [
            "pkh([00000001/44'/1'/" + args.account + "']" + xpub + "/0/*)#vt6w3l3j",
            "sh(wpkh([00000001/49'/1'/" + args.account + "']" + xpub + "/0/*))#r0grqw5x",
            "wpkh([00000001/84'/1'/" + args.account + "']" + xpub + "/0/*)#x30uthjs"
        ],
        "internal": [
            "pkh([00000001/44'/1'/" + args.account + "']" + xpub + "/1/*)#all0v2p2",
            "sh(wpkh([00000001/49'/1'/" + args.account + "']" + xpub + "/1/*))#kwx4c3pe",
            "wpkh([00000001/84'/1'/" + args.account + "']" + xpub + "/1/*)#h92akzzg"
        ]
    }))


parser = argparse.ArgumentParser(prog='./signer.py', description='External signer mock')
parser.add_argument('--fingerprint')
parser.add_argument('--testnet', action='store_true')
subparsers = parser.add_subparsers()

parser_enumerate = subparsers.add_parser('enumerate', help='list available signers')
parser_enumerate.set_defaults(func=enumerate)

parser_getdescriptors = subparsers.add_parser('getdescriptors')
parser_getdescriptors.set_defaults(func=getdescriptors)
parser_getdescriptors.add_argument('--account', metavar='account')

if len(sys.argv) == 1:
  args = parser.parse_args(['-h'])
  exit()

args = parser.parse_args()

perform_pre_checks()

args.func(args)
