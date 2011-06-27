SUBDIRS = cryptopp wx xpm json

AM_CXXFLAGS = -I$(top_srcdir) $(HOST_CXXFLAGS) $(DEBUG_CXXFLAGS)


bin_PROGRAMS = bitcoind

noinst_HEADERS = base58.h bignum.h db.h headers.h init.h irc.h key.h	\
		 main.h net.h noui.h rpc.h script.h serialize.h		\
		 strlcpy.h uint256.h util.h wallet.h keystore.h

if BUILD_WX

bin_PROGRAMS += bitcoin
# WX gui
bitcoin_SOURCES =  util.cpp script.cpp db.cpp net.cpp irc.cpp \
			main.cpp rpc.cpp init.cpp wallet.cpp keystore.cpp
bitcoin_CPPFLAGS = -Icryptopp $(HOST_CPPFLAGS) $(GUI_CPPFLAGS) -DGUI=1 -Iwx \
	$(BOOST_CPPFLAGS) $(SSL_CPPFLAGS) $(CRYPTO_CPPFLAGS) $(GTHREAD2_CPPFLAGS) 
bitcoin_LDADD = cryptopp/libcryptopp.a $(COMMON_LIBS) \
   $(BOOST_LIBS) \
   $(SSL_LIBS) \
   $(CRYPTO_LIBS) \
   $(GTHREAD2_LIBS) \
   $(UPNP_LIBS) \
   $(GUI_LIBS) \
   $(HOST_LIBS)

endif

# Deamon only, no gui
bitcoind_SOURCES = util.cpp script.cpp db.cpp net.cpp irc.cpp \
			main.cpp rpc.cpp init.cpp wallet.cpp keystore.cpp
bitcoind_CPPFLAGS = -Icryptopp $(HOST_CPPFLAGS) \
	$(BOOST_CPPFLAGS) $(SSL_CPPFLAGS) $(CRYPTO_CPPFLAGS) $(GTHREAD2_CPPFLAGS) 

bitcoind_LDADD = cryptopp/libcryptopp.a $(PROFILING_LIBS) \
   $(COMMON_LIBS) \
   $(BOOST_LIBS) \
   $(SSL_LIBS) \
   $(CRYPTO_LIBS) \
   $(GTHREAD2_LIBS) \
   $(UPNP_LIBS) \
   $(HOST_LIBS)

