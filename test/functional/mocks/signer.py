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

parser = argparse.ArgumentParser(prog='./signer.py', description='External signer mock')
subparsers = parser.add_subparsers()

if len(sys.argv) == 1:
  args = parser.parse_args(['-h'])
  exit()

args = parser.parse_args()

perform_pre_checks()

args.func(args)
