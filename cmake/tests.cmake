# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_test(NAME util_rpcauth_test
  COMMAND Python3::Interpreter ${PROJECT_BINARY_DIR}/test/util/rpcauth-test.py
)
set_tests_properties(util_rpcauth_test PROPERTIES
  DISABLED $<NOT:$<TARGET_EXISTS:Python3::Interpreter>>
)
