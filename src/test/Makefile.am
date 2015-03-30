include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src

bin_PROGRAMS = test_dash

TESTS = test_dash

JSON_TEST_FILES = \
  data/script_valid.json \
  data/base58_keys_valid.json \
  data/sig_canonical.json \
  data/sig_noncanonical.json \
  data/base58_encode_decode.json \
  data/base58_keys_invalid.json \
  data/script_invalid.json \
  data/tx_invalid.json \
  data/tx_valid.json \
  data/sighash.json

RAW_TEST_FILES = data/alertTests.raw

BUILT_SOURCES = $(JSON_TEST_FILES:.json=.json.h) $(RAW_TEST_FILES:.raw=.raw.h)

# test_dash binary #
test_dash_CPPFLAGS = $(AM_CPPFLAGS) $(TESTDEFS)
test_dash_LDADD = $(LIBBITCOIN_SERVER) $(LIBBITCOIN_CLI) $(LIBBITCOIN_COMMON) $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
if ENABLE_WALLET
test_dash_LDADD += $(LIBBITCOIN_WALLET)
endif
test_dash_LDADD += $(BDB_LIBS)

test_dash_SOURCES = \
  alert_tests.cpp \
  allocator_tests.cpp \
  base32_tests.cpp \
  base58_tests.cpp \
  base64_tests.cpp \
  bignum_tests.cpp \
  bloom_tests.cpp \
  canonical_tests.cpp \
  checkblock_tests.cpp \
  Checkpoints_tests.cpp \
  compress_tests.cpp \
  DoS_tests.cpp \
  getarg_tests.cpp \
  key_tests.cpp \
  main_tests.cpp \
  miner_tests.cpp \
  mruset_tests.cpp \
  multisig_tests.cpp \
  netbase_tests.cpp \
  pmt_tests.cpp \
  rpc_tests.cpp \
  script_P2SH_tests.cpp \
  script_tests.cpp \
  serialize_tests.cpp \
  sigopcount_tests.cpp \
  test_dash.cpp \
  transaction_tests.cpp \
  uint256_tests.cpp \
  util_tests.cpp \
  scriptnum_tests.cpp \
  sighash_tests.cpp \
  $(JSON_TEST_FILES) $(RAW_TEST_FILES)

if ENABLE_WALLET
test_dash_SOURCES += \
   accounting_tests.cpp \
   wallet_tests.cpp \
   rpc_wallet_tests.cpp
endif

nodist_test_dash_SOURCES = $(BUILT_SOURCES)

CLEANFILES = *.gcda *.gcno $(BUILT_SOURCES)
