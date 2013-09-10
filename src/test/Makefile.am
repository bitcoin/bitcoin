include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS =  $(INCLUDES) -I$(top_builddir)/src/obj \
  -I$(top_srcdir)/src/leveldb/include -I$(top_srcdir)/src/leveldb/helpers \
  -I$(top_srcdir)/src $(BOOST_INCLUDES)

AM_LDFLAGS = $(PTHREAD_CFLAGS)

bin_PROGRAMS = test_bitcoin

TESTS = test_bitcoin

TEST_DATA_DIR=$(srcdir)/data

TEST_DATA_FILES= $(TEST_DATA_DIR)/script_valid.json \
  $(TEST_DATA_DIR)/base58_keys_valid.json $(TEST_DATA_DIR)/sig_canonical.json \
  $(TEST_DATA_DIR)/sig_noncanonical.json \
  $(TEST_DATA_DIR)/base58_encode_decode.json $(TEST_DATA_DIR)/alertTests \
  $(TEST_DATA_DIR)/base58_keys_invalid.json \
  $(TEST_DATA_DIR)/script_invalid.json $(TEST_DATA_DIR)/tx_invalid.json \
  $(TEST_DATA_DIR)/tx_valid.json

# test_bitcoin binary #
test_bitcoin_CPPFLAGS = $(AM_CPPFLAGS) $(TESTDEFS) \
  -DTEST_DATA_DIR=$(abs_srcdir)/data
test_bitcoin_LDADD = $(LIBBITCOIN) $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
test_bitcoin_SOURCES = accounting_tests.cpp alert_tests.cpp \
  allocator_tests.cpp base32_tests.cpp base58_tests.cpp base64_tests.cpp \
  bignum_tests.cpp bloom_tests.cpp canonical_tests.cpp checkblock_tests.cpp \
  Checkpoints_tests.cpp compress_tests.cpp DoS_tests.cpp getarg_tests.cpp \
  key_tests.cpp miner_tests.cpp mruset_tests.cpp multisig_tests.cpp \
  netbase_tests.cpp pmt_tests.cpp rpc_tests.cpp script_P2SH_tests.cpp \
  script_tests.cpp serialize_tests.cpp sigopcount_tests.cpp test_bitcoin.cpp \
  transaction_tests.cpp uint160_tests.cpp uint256_tests.cpp util_tests.cpp \
  wallet_tests.cpp $(TEST_DATA_FILES)

CLEANFILES = *.gcda *.gcno
