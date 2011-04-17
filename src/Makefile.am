
SUBDIRS = cryptopp

bin_PROGRAMS = bitcoind

bitcoind_SOURCES = util.cpp script.cpp db.cpp net.cpp irc.cpp \
			main.cpp rpc.cpp init.cpp
bitcoind_CXXFLAGS = -O2 -Wno-invalid-offsetof -Wformat -I$(DB_CXX_HEADER_PREFIX)
bitcoind_CPPFLAGS = -Icryptopp $(HOST_CPPFLAGS) \
	$(BOOST_CPPFLAGS) $(SSL_CPPFLAGS) $(CRYPTO_CPPFLAGS) $(GTHREAD2_CPPFLAGS)
bitcoind_LDADD = cryptopp/libcryptopp.la $(HOST_LIBS) \
   $(COMMON_LIBS) \
   $(BOOST_LIBS) \
   $(SSL_LIBS) \
   $(CRYPTO_LIBS) \
   $(GTHREAD2_LIBS) \
   $(UPNP_LIBS)
