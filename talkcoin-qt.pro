#include(src/qt/material-widgets/material-widgets.pro)
#include(src/qt/authenticator/authenticator.pro)

TARGET = talkcoin-qt
TEMPLATE = app

QT       += core gui widgets network printsupport qml quick quickcontrols2

CONFIG += c++11 no_include_pwd thread object_parallel_to_source staticlib
VERSION = 0.18.0.4

INCLUDEPATH += $$PWD/src $$PWD/src/consensus $$PWD/src/interfaces $$PWD/src/crosschain $$PWD/src/authenticator $$PWD/src/qt/material-widgets
INCLUDEPATH += $$PWD/src/index $$PWD/src/leveldb $$PWD/src/bench $$PWD/src/leveldb/include $$PWD/src/leveldb/helpers/memenv $$PWD/src/secp256k1
INCLUDEPATH += $$PWD/src/univalue/include $$PWD/src/zmq $$PWD/src/rpc $$PWD/src/smessage $$PWD/src/qt $$PWD/src/qt/forms $$PWD/src/compat
INCLUDEPATH += $$PWD/src/crypto $$PWD/src/policy $$PWD/src/primitives $$PWD/src/script $$PWD/src/univalue $$PWD/src/wallet $$PWD/src/secp256k1/include

include (protobuf.pri)
PROTOS += src/qt/paymentrequest.proto \

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += ENABLE_MOMENTUM_HASH_ALGO ENABLE_PROOF_OF_STAKE ENABLE_SECURE_MESSAGING ENABLE_IBTP QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE
DEFINES += ENABLE_BIP70 USE_QRCODE ENABLE_WALLET ENABLE_ZMQ HAVE_CONSENSUS_LIB HAVE_CXX11 HAVE_DECL_DAEMON HAVE_SYS_TYPES_H HAVE_DECL_STRNLEN
DEFINES += HAVE_WORKING_BOOST_SLEEP_FOR PIE WANT_DENSE BOOST_SP_USE_STD_ATOMIC BOOST_AC_USE_STD_ATOMIC HAVE_BOOST_SYSTEM HAVE_BOOST_THREAD
DEFINES += HAVE_MEMORY_H USE_ASM HAVE_DECL_STRERROR_R HAVE_DLFCN_H HAVE_BYTESWAP_H HAVE_DECL_BSWAP_16 HAVE_DECL_BSWAP_32 HAVE_DECL_BSWAP_64
DEFINES += HAVE_FUNC_ATTRIBUTE_VISIBILITY HAVE_GETENTROPY_RAND HAVE_INTTYPES_H HAVE_MINIUPNPC_MINIUPNPC_H HAVE_MINIUPNPC_MINIWGET_H 
DEFINES += HAVE_MINIUPNPC_UPNPCOMMANDS_H HAVE_MINIUPNPC_UPNPERRORS_H HAVE_STDINT_H HAVE_STDIO_H HAVE_STDLIB_H
DEFINES += HAVE_STRERROR_R HAVE_STRINGS_H HAVE_STRING_H HAVE_SYS_ENDIAN_H HAVE_SYS_GETRANDOM HAVE_SYS_PRCTL_H HAVE_SYS_SELECT_H HAVE_SYS_STAT_H
DEFINES += HAVE_THREAD_LOCAL HAVE_UNISTD_H HAVE_VISIBILITY_ATTRIBUTE STDC_HEADERS HAVE_DLFCN_H HAVE_BOOST HAVE_BOOST_CHRONO HAVE_BOOST_FILESYSTEM 
DEFINES += FDELT_TYPE="long" USE_UPNP MINIUPNP_STATICLIB HAVE_BOOST_UNIT_TEST_FRAMEWORK
DEFINES += HAVE_BYTESWAP_H HAVE_DECL_BE16TOH HAVE_DECL_BE32TOH HAVE_DECL_BE64TOH HAVE_DECL_BSWAP_16 HAVE_DECL_BSWAP_32
DEFINES += HAVE_DECL_BSWAP_64 HAVE_DECL_HTOBE16  HAVE_DECL_HTOBE32 HAVE_DECL_HTOBE64 HAVE_DECL_HTOLE16 HAVE_DECL_HTOLE32
DEFINES += HAVE_DECL_HTOLE64 HAVE_DECL_LE16TOH HAVE_DECL_LE32TOH HAVE_DECL_LE64TOH
DEFINES += HAVE_PTHREAD HAVE_PTHREAD_PRIO_INHERIT HAVE_SYSTEM
#DEFINES += QT_QPA_PLATFORM_ANDROID QT_STATICPLUGIN
DEFINES += HAVE_DECL_FREEIFADDRS HAVE_DECL_GETIFADDRS
DEFINES += QGOOGLEAUTHENTICATOR_LIBRARY
DEFINES += COPYRIGHT_YEAR="2019"
DEFINES += COPYRIGHT_HOLDERS='\"Developers\"'
DEFINES += COPYRIGHT_HOLDERS_FINAL='\"Developers\"'
DEFINES += COPYRIGHT_HOLDERS_SUBSTITUTION='\"Talkcoin\"'
DEFINES += PACKAGE_NAME='\"Talkcoin\"'
DEFINES += CLIENT_VERSION_IS_RELEASE=1
DEFINES += CLIENT_VERSION_BUILD=0
DEFINES += CLIENT_VERSION_MAJOR=18
DEFINES += CLIENT_VERSION_MINOR=0
DEFINES += CLIENT_VERSION_REVISION=4


QMAKE_CXXFLAGS += -Wstack-protector -fstack-protector-all  -Wall -Wextra -Wformat -Wvla -Wswitch -Wformat-security -Wthread-safety-analysis
QMAKE_CXXFLAGS += -Wrange-loop-analysis -Wredundant-decls -pthread
QMAKE_CXXFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -DHAVE_BUILD_INFO -D__STDC_FORMAT_MACROS -D_FILE_OFFSET_BITS=64 -g -O2
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-implicit-fallthrough -Wno-self-assign -Wno-unused-local-typedef -Wno-deprecated-register
#QMAKE_LFLAGS +=
QMAKE_CFLAGS += -pthread -D_REENTRANT
QMAKE_LFLAGS += "-z noexecstack -z relro -z now"

android:{
    QT += androidextras
    message("Android arch "$$ANDROID_TARGET_ARCH)
    DEFINES += OS_ANDROID
    #DEFINES += MOBILE_GUI

	contains(ANDROID_TARGET_ARCH,arm64-v8a) {
            DEFINES += LEVELDB_PLATFORM_POSIX LEVELDB_ATOMIC_PRESENT
            LIBS += -L $$PWD/depends/aarch64-linux-android/lib $$PWD/depends/aarch64-linux-android/lib/libssl.a $$PWD/depends/aarch64-linux-android/lib/libcrypto.a
            INCLUDEPATH += $$PWD/depends/aarch64-linux-android/include
            android:BOOST_LIB_SUFFIX = -mt-s-a64
	}
        contains(ANDROID_TARGET_ARCH,armeabi-v7a){
	    DEFINES += ENABLE_HWCRC32 LEVELDB_PLATFORM_POSIX LEVELDB_ATOMIC_PRESENT
	    QMAKE_CXXFLAGS += -fPIC
            LIBS += -L $$PWD/depends/armv7a-linux-android/lib $$PWD/depends/armv7a-linux-android/lib/libssl.a $$PWD/depends/armv7a-linux-android/lib/libcrypto.a
            INCLUDEPATH += $$PWD/depends/armv7a-linux-android/include
            android:BOOST_LIB_SUFFIX = -mt-s-a32
	}
	contains(ANDROID_TARGET_ARCH,x86){
	    DEFINES += ENABLE_HWCRC32 LEVELDB_PLATFORM_POSIX LEVELDB_ATOMIC_PRESENT
	    QMAKE_CXXFLAGS += -fPIC
            LIBS += -L $$PWD/depends/i686-linux-android/lib $$PWD/depends/i686-linux-android/lib/libssl.a $$PWD/depends/i686-linux-android/lib/libcrypto.a
            INCLUDEPATH += $$PWD/depends/i686-linux-android/include
            android:BOOST_LIB_SUFFIX = -mt-s-x32
	}
}

#LIBS += -lssl -lcrypto
LIBS += -lqrencode -lminiupnpc -levent -ldb_cxx$$BDB_LIB_SUFFIX -lprotobuf -lzmq -levent_pthreads -lleveldb -lmemenv -pthread
LIBS += -L$$PWD/src/leveldb
LIBS += -lboost_system$$BOOST_LIB_SUFFIX -lboost_filesystem$$BOOST_LIB_SUFFIX -lboost_thread$$BOOST_LIB_SUFFIX -lboost_chrono$$BOOST_LIB_SUFFIX -lboost_atomic$$BOOST_LIB_SUFFIX
LIBS += $$PWD/src/secp256k1/src/libsecp256k1_la-secp256k1.o

{
#   SECP256K1_LIB_PATH = $$PWD/src/secp256k1/.libs
#   LIBS += $$join(SECP256K1_LIB_PATH,,-L,) -lsecp256k1
}

UI_DIR = build

SOURCES += \
  src/addrdb.cpp \
  src/addrman.cpp \
  src/arith_uint256.cpp \
  src/auxiliaryblockrequest.cpp \
  src/axiom.cpp \
  src/banman.cpp \
  src/base58.cpp \
  src/bech32.cpp \
  src/bloom.cpp \
  src/blockencodings.cpp \
  src/blockfilter.cpp \
  src/chain.cpp \
  src/chainparams.cpp \
  src/chainparamsbase.cpp \
  src/checkpoints.cpp \
  src/clientversion.cpp \
  src/coins.cpp \
  src/compressor.cpp \
  src/consensus/tx_verify.cpp \
  src/consensus/merkle.cpp \
  src/consensus/tx_check.cpp \
  src/core_read.cpp \
  src/core_write.cpp \
  src/crosschain/interblockchain.cpp \
  src/crypto/aes.cpp \
  src/crypto/chacha20.cpp \
  src/crypto/hkdf_sha256_32.cpp \
  src/crypto/hmac_sha256.cpp \
  src/crypto/hmac_sha512.cpp \
  src/crypto/poly1305.cpp \
  src/crypto/ripemd160.cpp \
  src/crypto/sha1.cpp \
  src/crypto/sha256.cpp \
  src/crypto/sha512.cpp \
  src/crypto/siphash.cpp \
  src/crypto/sha256_sse41.cpp \
  src/crypto/sha256_avx2.cpp \
  src/crypto/sha256_shani.cpp \
  src/dbwrapper.cpp \
  src/flatfile.cpp \
  src/fs.cpp \
  src/groestl-hash.cpp \
  src/hash.cpp \
  src/httprpc.cpp \
  src/httpserver.cpp \
  src/init.cpp \
  src/index/base.cpp \
  src/index/blockfilterindex.cpp \
  src/index/txindex.cpp \
  src/interfaces/chain.cpp \
  src/interfaces/handler.cpp \
  src/interfaces/node.cpp \
  src/interfaces/wallet.cpp \
  src/key.cpp \
  src/key_io.cpp \
  src/logging.cpp \
  src/merkleblock.cpp \
  src/miner.cpp \
  src/momentum.cpp \
  src/net.cpp \
  src/netaddress.cpp \
  src/netbase.cpp \
  src/net_permissions.cpp \
  src/net_processing.cpp \
  src/node/coin.cpp \
  src/node/psbt.cpp \
  src/node/transaction.cpp \
  src/noui.cpp \
  src/outputtype.cpp \
  src/policy/feerate.cpp \
  src/policy/fees.cpp \
  src/policy/policy.cpp \
  src/policy/rbf.cpp \
  src/policy/settings.cpp \
  src/pos.cpp \
  src/pow.cpp \
  src/primitives/block.cpp \
  src/primitives/transaction.cpp \
  src/protocol.cpp \
  src/psbt.cpp \
  src/pubkey.cpp \
  src/random.cpp \
  src/rest.cpp \
  src/rpc/blockchain.cpp \
  src/rpc/client.cpp \
  src/rpc/mining.cpp \
  src/rpc/misc.cpp \
  src/rpc/net.cpp \
  src/rpc/rawtransaction.cpp \
  src/rpc/rawtransaction_util.cpp \
  src/rpc/request.cpp \
  src/rpc/util.cpp \
  src/rpc/server.cpp \
  src/rpc/smessage.cpp \
  src/scheduler.cpp \
  src/script/talkcoinconsensus.cpp \
  src/script/descriptor.cpp \
  src/script/interpreter.cpp \
  src/script/script.cpp \
  src/script/script_error.cpp \
  src/script/sigcache.cpp \
  src/script/sign.cpp \
  src/script/signingprovider.cpp \
  src/script/standard.cpp \
  src/shutdown.cpp \
  src/smessage/lz4.c \
  src/smessage/smessage.cpp \
  src/smessage/xxhash.c \
  src/sphlib/sph_shabal.cpp \
  src/sphlib/groestl.cpp \
  src/support/cleanse.cpp \
  src/support/lockedpool.cpp \
  src/sync.cpp \
  src/threadinterrupt.cpp \
  src/timedata.cpp \
  src/torcontrol.cpp \
  src/txdb.cpp \
  src/txmempool.cpp \
  src/ui_interface.cpp \
  src/uint256.cpp \
  src/util/bip32.cpp \
  src/util/bytevectorhash.cpp \
  src/util/error.cpp \
  src/util/fees.cpp \
  src/util/system.cpp \
  src/util/moneystr.cpp \
  src/util/rbf.cpp \
  src/util/threadnames.cpp \
  src/util/strencodings.cpp \
  src/util/time.cpp \
  src/util/url.cpp \
  src/util/validation.cpp \
  src/univalue/lib/univalue.cpp \
  src/univalue/lib/univalue_get.cpp \
  src/univalue/lib/univalue_read.cpp \
  src/univalue/lib/univalue_write.cpp \
  src/validation.cpp \
  src/validationinterface.cpp \
  src/versionbits.cpp \
  src/versionbitsinfo.cpp \
  src/wallet/init.cpp \
  src/wallet/coincontrol.cpp \
  src/wallet/crypter.cpp \
  src/wallet/db.cpp \
  src/wallet/feebumper.cpp \
  src/wallet/fees.cpp \
  src/wallet/ismine.cpp \
  src/wallet/load.cpp \
  src/wallet/psbtwallet.cpp \
  src/wallet/rpcdump.cpp \
  src/wallet/rpcwallet.cpp \
  src/wallet/wallet.cpp \
  src/wallet/walletdb.cpp \
  src/wallet/walletutil.cpp \
  src/wallet/coinselection.cpp \
  src/wallet/wallettool.cpp \
  src/warnings.cpp \
  src/zmq/zmqabstractnotifier.cpp \
  src/zmq/zmqnotificationinterface.cpp \
  src/zmq/zmqpublishnotifier.cpp \
  src/zmq/zmqrpc.cpp \
  src/qt/bantablemodel.cpp \
  src/qt/talkcoin.cpp \
  src/qt/talkcoinaddressvalidator.cpp \
  src/qt/talkcoinamountfield.cpp \
  src/qt/talkcoingui.cpp \
  src/qt/talkcoinmobilegui.cpp \
  src/qt/talkcoinunits.cpp \
  src/qt/clientmodel.cpp \
  src/qt/csvmodelwriter.cpp \
  src/qt/guiutil.cpp \
  src/qt/intro.cpp \
  src/qt/main.cpp \
  src/qt/modaloverlay.cpp \
  src/qt/networkstyle.cpp \
  src/qt/notificator.cpp \
  src/qt/optionsdialog.cpp \
  src/qt/optionsmodel.cpp \
  src/qt/peertablemodel.cpp \
  src/qt/platformstyle.cpp \
  src/qt/qrimagewidget.cpp \
  src/qt/qvalidatedlineedit.cpp \
  src/qt/qvaluecombobox.cpp \
  src/qt/rpcconsole.cpp \
  src/qt/splashscreen.cpp \
  src/qt/trafficgraphwidget.cpp \
  src/qt/utilitydialog.cpp \
  src/qt/messagemodel.cpp\
  src/qt/messagepage.cpp \
  src/qt/addressbookpage.cpp \
  src/qt/addresstablemodel.cpp \
  src/qt/askpassphrasedialog.cpp \
  src/qt/coincontroldialog.cpp \
  src/qt/coincontroltreewidget.cpp \
  src/qt/editaddressdialog.cpp \
  src/qt/openuridialog.cpp \
  src/qt/overviewpage.cpp \
  src/qt/paymentrequestplus.cpp \
  src/qt/paymentserver.cpp \
  src/qt/qvalidatedtextedit.cpp \
  src/qt/receivecoinsdialog.cpp \
  src/qt/receiverequestdialog.cpp \
  src/qt/recentrequeststablemodel.cpp \
  src/qt/sendcoinsdialog.cpp \
  src/qt/sendcoinsentry.cpp \
  src/qt/sendmessagesentry.cpp \
  src/qt/sendmessagesdialog.cpp \
  src/qt/signverifymessagedialog.cpp \
  src/qt/transactiondesc.cpp \
  src/qt/transactiondescdialog.cpp \
  src/qt/transactionfilterproxy.cpp \
  src/qt/transactionrecord.cpp \
  src/qt/transactiontablemodel.cpp \
  src/qt/transactionview.cpp \
  src/qt/walletcontroller.cpp \
  src/qt/walletframe.cpp \
  src/qt/walletmodel.cpp \
  src/qt/walletmodeltransaction.cpp \
  src/qt/walletview.cpp \
  src/leveldb/db/builder.cc \
  src/leveldb/db/c.cc \
  src/leveldb/db/dbformat.cc \
  src/leveldb/db/db_impl.cc \
  src/leveldb/db/db_iter.cc \
  src/leveldb/db/dumpfile.cc \
  src/leveldb/db/filename.cc \
  src/leveldb/db/log_reader.cc \
  src/leveldb/db/log_writer.cc \
  src/leveldb/db/memtable.cc \
  src/leveldb/db/repair.cc \
  src/leveldb/db/table_cache.cc \
  src/leveldb/db/version_edit.cc \
  src/leveldb/db/version_set.cc \
  src/leveldb/db/write_batch.cc \
  src/leveldb/table/block_builder.cc \
  src/leveldb/table/block.cc \
  src/leveldb/table/filter_block.cc \
  src/leveldb/table/format.cc \
  src/leveldb/table/iterator.cc \
  src/leveldb/table/merger.cc \
  src/leveldb/table/table_builder.cc \
  src/leveldb/table/table.cc \
  src/leveldb/table/two_level_iterator.cc \
  src/leveldb/util/arena.cc \
  src/leveldb/util/bloom.cc \
  src/leveldb/util/cache.cc \
  src/leveldb/util/coding.cc \
  src/leveldb/util/comparator.cc \
  src/leveldb/util/crc32c.cc \
  src/leveldb/util/env.cc \
  src/leveldb/util/env_posix.cc \
  src/leveldb/util/filter_policy.cc \
  src/leveldb/util/hash.cc \
  src/leveldb/util/histogram.cc \
  src/leveldb/util/logging.cc \
  src/leveldb/util/options.cc \
  src/leveldb/util/status.cc \
  src/leveldb/port/port_posix.cc \
  src/leveldb/helpers/memenv/memenv.cc \
  src/leveldb/port/port_posix_sse.cc \
  src/compat/glibc_sanity.cpp \
  src/compat/glibcxx_sanity.cpp \
  src/compat/strnlen.cpp \
  src/compat/glibc_compat.cpp

HEADERS += \
  src/addrdb.h \
  src/addrman.h \
  src/amount.h \
  src/attributes.h \
  src/arith_uint256.h \
  src/auxiliaryblockrequest.h \
  src/axiom.h \
  src/banman.h \
  src/base58.h \
  src/bech32.h \
  src/bloom.h \
  src/blockencodings.h \
  src/blockfilter.h \
  src/chain.h \
  src/chainparams.h \
  src/chainparamsbase.h \
  src/chainparamsseeds.h \
  src/checkpoints.h \
  src/checkqueue.h \
  src/clientversion.h \
  src/coins.h \
  src/compat.h \
  src/compat/assumptions.h \
  src/compat/byteswap.h \
  src/compat/endian.h \
  src/compat/sanity.h \
  src/compressor.h \
  src/consensus/consensus.h \
  src/consensus/tx_check.h \
  src/consensus/tx_verify.h \
  src/consensus/merkle.h \
  src/consensus/params.h \
  src/consensus/validation.h \
  src/core_io.h \
  src/core_memusage.h \
  src/crosschain/interblockchain.h \
  src/crypto/aes.h \
  src/crypto/chacha20.h \
  src/crypto/common.h \
  src/crypto/hkdf_sha256_32.h \
  src/crypto/hmac_sha256.h \
  src/crypto/hmac_sha512.h \
  src/crypto/poly1305.h \
  src/crypto/ripemd160.h \
  src/crypto/sha1.h \
  src/crypto/sha256.h \
  src/crypto/sha512.h \
  src/crypto/siphash.h \
  src/cuckoocache.h \
  src/dbwrapper.h \
  src/flatfile.h \
  src/fs.h \
  src/groestl.h \
  src/hash.h \
  src/httprpc.h \
  src/httpserver.h \
  src/index/base.h \
  src/index/blockfilterindex.h \
  src/index/txindex.h \
  src/indirectmap.h \
  src/init.h \
  src/interfaces/chain.h \
  src/interfaces/handler.h \
  src/interfaces/node.h \
  src/interfaces/wallet.h \
  src/key.h \
  src/key_io.h \
  src/limitedmap.h \
  src/logging.h \
  src/memusage.h \
  src/merkleblock.h \
  src/miner.h \
  src/momentum.h \
  src/net.h \
  src/net_processing.h \
  src/netaddress.h \
  src/netbase.h \
  src/netmessagemaker.h \
  src/net_permissions.h \
  src/node/coin.h \
  src/node/psbt.h \
  src/node/transaction.h \
  src/noui.h \
  src/optional.h \
  src/outputtype.h \
  src/policy/feerate.h \
  src/policy/fees.h \
  src/policy/policy.h \
  src/policy/rbf.h \
  src/policy/settings.h \
  src/pos.h \
  src/pow.h \
  src/prevector.h \
  src/primitives/block.h \
  src/primitives/transaction.h \
  src/protocol.h \
  src/psbt.h \
  src/pubkey.h \
  src/random.h \
  src/reverse_iterator.h \
  src/reverselock.h \
  src/rpc/blockchain.h \
  src/rpc/client.h \
  src/rpc/mining.h \
  src/rpc/protocol.h \
  src/rpc/server.h \
  src/rpc/rawtransaction_util.h \
  src/rpc/register.h \
  src/rpc/request.h \
  src/rpc/util.h \
  src/scheduler.h \
  src/script/descriptor.h \
  src/script/interpreter.h \
  src/script/script.h \
  src/script/script_error.h \
  src/script/sigcache.h \
  src/script/sign.h \
  src/script/signingprovider.h \
  src/script/standard.h \
  src/semiOrderedMap.h \
  src/serialize.h \
  src/shutdown.h \
  src/smessage/lz4.h \
  src/smessage/smessage.h \
  src/smessage/xxhash.h \
  src/span.h \
  src/sphlib/sph_types.h \
  src/sphlib/sph_groestl.h \
  src/sphlib/sph_shabal.h \
  src/streams.h \
  src/support/allocators/secure.h \
  src/support/allocators/zeroafterfree.h \
  src/support/cleanse.h \
  src/support/events.h \
  src/support/lockedpool.h \
  src/sync.h \
  src/threadsafety.h \
  src/threadinterrupt.h \
  src/timedata.h \
  src/tinyformat.h \
  src/torcontrol.h \
  src/txdb.h \
  src/txmempool.h \
  src/uint256.h \
  src/ui_interface.h \
  src/undo.h \
  src/univalue/include/univalue.h \
  src/util/bip32.h \
  src/util/bytevectorhash.h \
  src/util/error.h \
  src/util/fees.h \
  src/util/system.h \
  src/util/memory.h \
  src/util/moneystr.h \
  src/util/rbf.h \
  src/util/strencodings.h \
  src/util/threadnames.h \
  src/util/time.h \
  src/util/translation.h \
  src/util/url.h \
  src/util/validation.h \
  src/validation.h \
  src/validationinterface.h \
  src/versionbits.h \
  src/versionbitsinfo.h \
  src/walletinitinterface.h \
  src/wallet/coincontrol.h \
  src/wallet/crypter.h \
  src/wallet/db.h \
  src/wallet/feebumper.h \
  src/wallet/fees.h \
  src/wallet/ismine.h \
  src/wallet/load.h \
  src/wallet/psbtwallet.h \
  src/wallet/rpcwallet.h \
  src/wallet/wallet.h \
  src/wallet/walletdb.h \
  src/wallet/wallettool.h \
  src/wallet/walletutil.h \
  src/wallet/coinselection.h \
  src/warnings.h \
  src/version.h \
  src/zmq/zmqabstractnotifier.h \
  src/zmq/zmqconfig.h\
  src/zmq/zmqnotificationinterface.h \
  src/zmq/zmqpublishnotifier.h \
  src/zmq/zmqrpc.h \
  src/qt/addressbookpage.h \
  src/qt/addresstablemodel.h \
  src/qt/askpassphrasedialog.h \
  src/qt/bantablemodel.h \
  src/qt/talkcoinaddressvalidator.h \
  src/qt/talkcoinamountfield.h \
  src/qt/talkcoin.h \
  src/qt/talkcoingui.h \
  src/qt/talkcoinmobilegui.h \
  src/qt/talkcoinunits.h \
  src/qt/clientmodel.h \
  src/qt/coincontroldialog.h \
  src/qt/coincontroltreewidget.h \
  src/qt/csvmodelwriter.h \
  src/qt/editaddressdialog.h \
  src/qt/guiconstants.h \
  src/qt/guiutil.h \
  src/qt/intro.h \
  src/qt/macdockiconhandler.h \
  src/qt/macnotificationhandler.h \
  src/qt/macos_appnap.h \
  src/qt/modaloverlay.h \
  src/qt/networkstyle.h \
  src/qt/notificator.h \
  src/qt/openuridialog.h \
  src/qt/optionsdialog.h \
  src/qt/optionsmodel.h \
  src/qt/overviewpage.h \
  src/qt/paymentrequestplus.h \
  src/qt/paymentserver.h \
  src/qt/peertablemodel.h \
  src/qt/platformstyle.h \
  src/qt/qrimagewidget.h \
  src/qt/qvalidatedlineedit.h \
  src/qt/qvalidatedtextedit.h \
  src/qt/qvaluecombobox.h \
  src/qt/receivecoinsdialog.h \
  src/qt/receiverequestdialog.h \
  src/qt/recentrequeststablemodel.h \
  src/qt/rpcconsole.h \
  src/qt/sendcoinsdialog.h \
  src/qt/sendcoinsentry.h \
  src/qt/signverifymessagedialog.h \
  src/qt/splashscreen.h \
  src/qt/trafficgraphwidget.h \
  src/qt/transactiondesc.h \
  src/qt/transactiondescdialog.h \
  src/qt/transactionfilterproxy.h \
  src/qt/transactionrecord.h \
  src/qt/transactiontablemodel.h \
  src/qt/transactionview.h \
  src/qt/utilitydialog.h \
  src/qt/walletcontroller.h \
  src/qt/walletframe.h \
  src/qt/walletmodel.h \
  src/qt/walletmodeltransaction.h \
  src/qt/walletview.h \
  src/qt/winshutdownmonitor.h \
  src/qt/sendmessagesentry.h \
  src/qt/sendmessagesdialog.h \
  src/qt/messagemodel.h \
  src/qt/messagepage.h \
  src/leveldb/port/atomic_pointer.h \
  src/leveldb/port/port_example.h \
  src/leveldb/port/port_posix.h \
  src/leveldb/port/win/stdint.h \
  src/leveldb/port/port.h \
  src/leveldb/port/port_win.h \
  src/leveldb/port/thread_annotations.h \
  src/leveldb/include/leveldb/db.h \
  src/leveldb/include/leveldb/options.h \
  src/leveldb/include/leveldb/comparator.h \
  src/leveldb/include/leveldb/filter_policy.h \
  src/leveldb/include/leveldb/slice.h \
  src/leveldb/include/leveldb/table_builder.h \
  src/leveldb/include/leveldb/env.h \
  src/leveldb/include/leveldb/c.h \
  src/leveldb/include/leveldb/iterator.h \
  src/leveldb/include/leveldb/cache.h \
  src/leveldb/include/leveldb/dumpfile.h \
  src/leveldb/include/leveldb/table.h \
  src/leveldb/include/leveldb/write_batch.h \
  src/leveldb/include/leveldb/status.h \
  src/leveldb/db/log_format.h \
  src/leveldb/db/memtable.h \
  src/leveldb/db/version_set.h \
  src/leveldb/db/write_batch_internal.h \
  src/leveldb/db/filename.h \
  src/leveldb/db/version_edit.h \
  src/leveldb/db/dbformat.h \
  src/leveldb/db/builder.h \
  src/leveldb/db/log_writer.h \
  src/leveldb/db/db_iter.h \
  src/leveldb/db/skiplist.h \
  src/leveldb/db/db_impl.h \
  src/leveldb/db/table_cache.h \
  src/leveldb/db/snapshot.h \
  src/leveldb/db/log_reader.h \
  src/leveldb/table/filter_block.h \
  src/leveldb/table/block_builder.h \
  src/leveldb/table/block.h \
  src/leveldb/table/two_level_iterator.h \
  src/leveldb/table/merger.h \
  src/leveldb/table/format.h \
  src/leveldb/table/iterator_wrapper.h \
  src/leveldb/util/crc32c.h \
  src/leveldb/util/env_posix_test_helper.h \
  src/leveldb/util/arena.h \
  src/leveldb/util/random.h \
  src/leveldb/util/posix_logger.h \
  src/leveldb/util/hash.h \
  src/leveldb/util/histogram.h \
  src/leveldb/util/coding.h \
  src/leveldb/util/testutil.h \
  src/leveldb/util/mutexlock.h \
  src/leveldb/util/logging.h \
  src/leveldb/util/testharness.h  \
  src/leveldb/helpers/memenv/memenv.h

FORMS += \
  src/qt/forms/addressbookpage.ui \
  src/qt/forms/askpassphrasedialog.ui \
  src/qt/forms/coincontroldialog.ui \
  src/qt/forms/editaddressdialog.ui \
  src/qt/forms/helpmessagedialog.ui \
  src/qt/forms/intro.ui \
  src/qt/forms/modaloverlay.ui \
  src/qt/forms/openuridialog.ui \
  src/qt/forms/optionsdialog.ui \
  src/qt/forms/overviewpage.ui \
  src/qt/forms/receivecoinsdialog.ui \
  src/qt/forms/receiverequestdialog.ui \
  src/qt/forms/debugwindow.ui \
  src/qt/forms/sendcoinsdialog.ui \
  src/qt/forms/sendcoinsentry.ui \
  src/qt/forms/signverifymessagedialog.ui \
  src/qt/forms/transactiondescdialog.ui \
  src/qt/forms/sendmessagesentry.ui \
  src/qt/forms/sendmessagesdialog.ui \
  src/qt/forms/messagepage.ui

RESOURCES += \
    src/qt/talkcoin.qrc \
    src/qt/talkcoin_locale.qrc

OTHER_FILES = \
	src/qt/res/qml/main.qml \
	src/qt/res/qml/WalletPane.qml \
	src/qt/res/qml/SendPane.qml \
	src/qt/res/qml/ReceivePane.qml \
	src/qt/res/qml/SettingsPane.qml \
	src/qt/res/qml/AboutPane.qml \
	src/qt/res/qml/ConsolePane.qml

TRANSLATIONS = $$files(src/qt/locale/talkcoin_*.ts)

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
isEmpty(QM_DIR):QM_DIR = $$PWD/src/qt/locale
# automatically build translations, so they can be included in resource file
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$QM_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

android {

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += talkcoin-qt
