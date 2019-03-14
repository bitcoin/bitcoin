#!/usr/bin/env python3
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
This checks if all command line args are documented.
Return value is 0 to indicate no error.

Author: @MarcoFalke
'''

from subprocess import check_output
import re

FOLDER_GREP = 'src'
FOLDER_TEST = 'src/test/'
CMD_ROOT_DIR = '`git rev-parse --show-toplevel`/{}'.format(FOLDER_GREP)
CMD_GREP_ARGS = r"egrep -r -I '(map(Multi)?Args(\.count\(|\[)|Get(Bool)?Arg\()\"\-[^\"]+?\"' {} | grep -v '{}'".format(CMD_ROOT_DIR, FOLDER_TEST)
CMD_GREP_DOCS = r"egrep -r -I 'HelpMessageOpt\(\"\-[^\"=]+?(=|\")' {}".format(CMD_ROOT_DIR)
REGEX_ARG = re.compile(r'(?:map(?:Multi)?Args(?:\.count\(|\[)|Get(?:Bool)?Arg\()\"(\-[^\"]+?)\"')
REGEX_DOC = re.compile(r'HelpMessageOpt\(\"(\-[^\"=]+?)(?:=|\")')
# list unsupported, deprecated and duplicate args as they need no documentation
SET_DOC_OPTIONAL = set(['-rpcssl', '-benchmark', '-h', '-help', '-socks', '-tor', '-debugnet', '-whitelistalwaysrelay', '-blockminsize', '-sendfreetransactions'])

def main():
  used = check_output(CMD_GREP_ARGS, shell=True, universal_newlines=True)
  docd = check_output(CMD_GREP_DOCS, shell=True, universal_newlines=True)

  args_used = set(re.findall(REGEX_ARG,used))
  args_docd = set(re.findall(REGEX_DOC,docd)).union(SET_DOC_OPTIONAL)
  args_need_doc = args_used.difference(args_docd)
  args_unknown = args_docd.difference(args_used)

  print("Args used        : {}".format(len(args_used)))
  print("Args documented  : {}".format(len(args_docd)))
  print("Args undocumented: {}".format(len(args_need_doc)))
  print(args_need_doc)
  print("Args unknown     : {}".format(len(args_unknown)))
  print(args_unknown)

  exit(len(args_need_doc))

if __name__ == "__main__":
    main()
