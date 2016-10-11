include_HEADERS += include/secp256k1_ecdh.h
noinst_HEADERS += src/modules/ecdh/main_impl.h
noinst_HEADERS += src/modules/ecdh/tests_impl.h
if USE_BENCHMARK
noinst_PROGRAMS += bench_ecdh
bench_ecdh_SOURCES = src/bench_ecdh.c
bench_ecdh_LDADD = libsecp256k1.la $(SECP_LIBS) $(COMMON_LIB)
endif
