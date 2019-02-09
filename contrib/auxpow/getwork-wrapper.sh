#!/usr/bin/env bash
# Copyright (c) 2018 Daniel Kraft
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Starts up the getwork-wrapper.py script, setting PYTHONPATH accordingly.
# This script must be run from the root source directory to work correctly.
#
# Example:
#   contrib/auxpow/getwork-wrapper.sh http://user:pass@localhost:port/ 1234

PYTHONPATH="test/functional/test_framework"
contrib/auxpow/getwork-wrapper.py --backend-url="$1" --port="$2"
