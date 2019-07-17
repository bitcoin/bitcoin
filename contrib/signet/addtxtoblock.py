#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import sys
import os
import argparse

# dirty hack but makes CBlock etc available
sys.path.append('/'.join(os.getcwd().split('/')[:-2]) + '/test/functional')

from test_framework.messages import (
    CBlock,
    CTransaction,
    FromHex,
    ToHex,
)

from test_framework.blocktools import add_witness_commitment

def main():

    # Parse arguments and pass through unrecognised args
    parser = argparse.ArgumentParser(add_help=False,
                                        usage='%(prog)s [addtxtoblock options] [bitcoin block file] [tx file] [fee]',
                                        description=__doc__,
                                        epilog='''Help text and arguments:''',
                                        formatter_class=argparse.RawTextHelpFormatter)
    _, unknown_args = parser.parse_known_args()

    if len(unknown_args) != 3:
        print("Need three arguments (block file, tx file, and fee)")
        sys.exit(1)

    [blockfile, txfile, feestr] = unknown_args

    with open(blockfile, "r", encoding="utf8") as f:
        blockhex = f.read().strip()
    with open(txfile,    "r", encoding="utf8") as f:
        txhex    = f.read().strip()

    fee = int(feestr)

    block = CBlock()
    FromHex(block, blockhex)

    tx = CTransaction()
    FromHex(tx, txhex)

    block.vtx[0].vout[0].nValue += fee
    block.vtx.append(tx)
    add_witness_commitment(block)
    print(ToHex(block))

if __name__ == '__main__':
    main()
