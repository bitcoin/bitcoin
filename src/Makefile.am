
SUBDIRS = cryptopp

bin_PROGRAMS = bitcoind

bitcoind_SOURCES = util.cpp script.cpp db.cpp net.cpp irc.cpp \
			main.cpp rpc.cpp init.cpp

bitcoind_LDADD = cryptopp/libcryptopp.la \
   -lpthread \
   -lboost_system \
   -lboost_filesystem \
   -lboost_program_options \
   -lboost_thread \
   -ldb_cxx \
   -lssl \
   -lcrypto \
   -lgthread-2.0 \
   -lz \
   -ldl



