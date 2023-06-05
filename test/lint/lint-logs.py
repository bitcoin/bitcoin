#!/usr/bin/env python3
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check that all logs are terminated with '\n'
#
# Some logs are continued over multiple lines. They should be explicitly
# commented with /* Continued */

import re
import sys

from subprocess import check_output


def main():
    logs_list = check_output(["git", "grep", "--extended-regexp", r"(LogPrintLevel|LogPrintfCategory|LogPrintf?)\(", "--", "*.cpp"], text=True, encoding="utf8").splitlines()

    unterminated_logs = [line for line in logs_list if not re.search(r'(\\n"|/\* Continued \*/)', line)]

    if unterminated_logs != []:
        print("All calls to LogPrintf(), LogPrintfCategory(), LogPrint(), LogPrintLevel(), and WalletLogPrintf() should be terminated with \"\\n\".")
        print("")

        for line in unterminated_logs:
            print(line)

        sys.exit(1)


if __name__ == "__main__":
    main()
