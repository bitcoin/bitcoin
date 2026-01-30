#!/usr/bin/env python3
# Copyright (c) 2015-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
This checks if all command line args are documented.
Return value is 0 to indicate no error.
'''

from subprocess import check_output
import re

FOLDER_GREP = 'src'
FOLDER_TEST = 'src/test/'
REGEX_ARG = r'\b(?:GetArg|GetArgs|GetBoolArg|GetIntArg|GetPathArg|IsArgSet|get_net)\("(-[^"]+)"'
REGEX_DOC = r'AddArg\("(-[^"=]+?)(?:=|")'
CMD_ROOT_DIR = '$(git rev-parse --show-toplevel)/{}'.format(FOLDER_GREP)
CMD_GREP_ARGS = r"git grep --perl-regexp '{}' -- {} ':(exclude){}'".format(REGEX_ARG, CMD_ROOT_DIR, FOLDER_TEST)
CMD_GREP_WALLET_ARGS = r"git grep --function-context 'void WalletInit::AddWalletOptions' -- {} | grep AddArg".format(CMD_ROOT_DIR)
CMD_GREP_WALLET_HIDDEN_ARGS = r"git grep --function-context 'void DummyWalletInit::AddWalletOptions' -- {}".format(CMD_ROOT_DIR)
CMD_GREP_DOCS = r"git grep --perl-regexp '{}' {}".format(REGEX_DOC, CMD_ROOT_DIR)
# list unsupported, deprecated and duplicate args as they need no documentation
SET_DOC_OPTIONAL = set(['-h', '-?', '-dbcrashratio', '-forcecompactdb', '-ipcconnect', '-ipcfd'])


def lint_missing_argument_documentation():
    used = check_output(CMD_GREP_ARGS, shell=True, text=True).strip()
    docd = check_output(CMD_GREP_DOCS, shell=True, text=True).strip()

    args_used = set(re.findall(re.compile(REGEX_ARG), used))
    args_docd = set(re.findall(re.compile(REGEX_DOC), docd)).union(SET_DOC_OPTIONAL)
    args_need_doc = args_used.difference(args_docd)
    args_unknown = args_docd.difference(args_used)

    print("Args used        : {}".format(len(args_used)))
    print("Args documented  : {}".format(len(args_docd)))
    print("Args undocumented: {}".format(len(args_need_doc)))
    print(args_need_doc)
    print("Args unknown     : {}".format(len(args_unknown)))
    print(args_unknown)

    assert 0 == len(args_need_doc), "Please document the following arguments: {}".format(args_need_doc)


def lint_missing_hidden_wallet_args():
    wallet_args = check_output(CMD_GREP_WALLET_ARGS, shell=True, text=True).strip()
    wallet_hidden_args = check_output(CMD_GREP_WALLET_HIDDEN_ARGS, shell=True, text=True).strip()

    wallet_args = set(re.findall(re.compile(REGEX_DOC), wallet_args))
    wallet_hidden_args = set(re.findall(re.compile(r'    "([^"=]+)'), wallet_hidden_args))

    hidden_missing = wallet_args.difference(wallet_hidden_args)
    if hidden_missing:
        assert 0, "Please add {} to the hidden args in DummyWalletInit::AddWalletOptions".format(hidden_missing)


def main():
    lint_missing_argument_documentation()
    lint_missing_hidden_wallet_args()


if __name__ == "__main__":
    main()
