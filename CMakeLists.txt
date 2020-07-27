
if (NOT CMAKE_INSTALL_PREFIX OR
    CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../../Install")
endif()


file(GLOB_RECURSE BITCOIN_0_19_1DEV_SRCS ${CMAKE_CURRENT_SOURCE_DIR} *.c *.h *.cpp)

add_custom_target(bitcoin_0.19.1dev ALL
  DEPENDS ${CMAKE_INSTALL_PREFIX}/plugins/libBITCOIND_0.19.1DEV.so
  )

add_custom_command(OUTPUT ${CMAKE_INSTALL_PREFIX}/plugins/libBITCOIND_0.19.1DEV.so
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/bitcoind
  DEPENDS openssl_1.1.0h
  COMMAND g++ -std=c++11 -O0 -g3 -ftrapv -fstack-reuse=none -Wstack-protector -fstack-protector-all -Wall -Wextra -Wformat -Wvla -Wswitch -Wformat-security -Wredundant-decls -Wno-unused-parameter -Wno-implicit-fallthrough -fPIC -pthread -Wl,-z -Wl,relro -Wl,-z -Wl,now -fPIC -shared -rdynamic -Wl,-rpath=${CMAKE_INSTALL_PREFIX}/openssl_1.1.0h/lib -Wl,-rpath=${CMAKE_INSTALL_PREFIX}/bdb_4.8.30/lib -o libBITCOIND.so bitcoind-bitcoind.o -L${CMAKE_INSTALL_PREFIX}/openssl_1.1.0h/lib -L${CMAKE_INSTALL_PREFIX}/bdb_4.8.30/lib libbitcoin_server.a libbitcoin_wallet.a libbitcoin_server.a libbitcoin_common.a univalue/.libs/libunivalue.a libbitcoin_util.a libbitcoin_consensus.a crypto/libbitcoin_crypto_base.a crypto/libbitcoin_crypto_sse41.a crypto/libbitcoin_crypto_avx2.a crypto/libbitcoin_crypto_shani.a leveldb/libleveldb.a leveldb/libleveldb_sse42.a leveldb/libmemenv.a secp256k1/.libs/libsecp256k1.a -L/usr/lib/x86_64-linux-gnu -lboost_system -lboost_filesystem -lboost_thread -lpthread -lboost_chrono -ldb_cxx -lcrypto -levent_pthreads -levent -levent -pthread  -D__LIBTOOL_IS_A_FOOL__
  COMMAND ${CMAKE_COMMAND} -E copy libBITCOIND.so ${CMAKE_INSTALL_PREFIX}/plugins/libBITCOIND_0.19.1DEV.so
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/bitcoind
  DEPENDS bdb_4.8.30
  DEPENDS ${BITCOIN_0_19_1_SRCS}
  COMMAND ./autogen.sh
  COMMAND ./configure CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/bdb_4.8.30/include LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/bdb_4.8.30/lib --with-pic --disable-bench --disable-tests --enable-debug
  COMMAND make -j4
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
  
# Define targets that can be used in same build-tree
add_library(Bitcoin_0_19_1dev::Plugin SHARED IMPORTED GLOBAL)
add_dependencies(Bitcoin_0_19_1dev::Plugin bitcoin_0.19.1)
set_target_properties(Bitcoin_0_19_1dev::Plugin PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES "C"
  IMPORTED_LOCATION ${CMAKE_INSTALL_PREFIX}/plugins/libBITCOIND_0.19.1DEV.so)



