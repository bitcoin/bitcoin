# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_test(NAME util_test_runner
  COMMAND Python3::Interpreter ${PROJECT_BINARY_DIR}/test/util/test_runner.py
)
set_property(TEST util_test_runner PROPERTY
  ENVIRONMENT_MODIFICATION
    BITCOINUTIL=set:$<$<TARGET_EXISTS:bitcoin-util>:$<TARGET_FILE:bitcoin-util>>
    BITCOINTX=set:$<$<TARGET_EXISTS:bitcoin-tx>:$<TARGET_FILE:bitcoin-tx>>
)
set_tests_properties(util_test_runner PROPERTIES
  DISABLED $<NOT:$<AND:$<TARGET_EXISTS:Python3::Interpreter>,$<TARGET_EXISTS:bitcoin-util>,$<TARGET_EXISTS:bitcoin-tx>>>
)

add_test(NAME util_rpcauth_test
  COMMAND Python3::Interpreter ${PROJECT_BINARY_DIR}/test/util/rpcauth-test.py
)
set_tests_properties(util_rpcauth_test PROPERTIES
  DISABLED $<NOT:$<TARGET_EXISTS:Python3::Interpreter>>
)
