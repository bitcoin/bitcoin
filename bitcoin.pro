TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += gui/include core/include cryptopp/include json/include
unix:LIBS += -lssl -lboost_system -lboost_filesystem -lboost_program_options -lboost_thread -ldb_cxx
macx:DEFINES += __WXMAC_OSX__ MSG_NOSIGNAL=0
# WINDOWS defines, -DSSL, look at build system

# Input
HEADERS += gui/include/bitcoingui.h \
    gui/include/transactiontablemodel.h \
    gui/include/addresstablemodel.h \
    gui/include/optionsdialog.h \
    gui/include/mainoptionspage.h \
    gui/include/sendcoinsdialog.h \
    gui/include/addressbookdialog.h \
    gui/include/aboutdialog.h \
    gui/include/editaddressdialog.h \
    gui/include/bitcoinaddressvalidator.h \
    core/include/base58.h \
    core/include/bignum.h \
    core/include/util.h \
    core/include/uint256.h \
    core/include/serialize.h \
    cryptopp/include/cryptopp/stdcpp.h \
    cryptopp/include/cryptopp/smartptr.h \
    cryptopp/include/cryptopp/simple.h \
    cryptopp/include/cryptopp/sha.h \
    cryptopp/include/cryptopp/secblock.h \
    cryptopp/include/cryptopp/pch.h \
    cryptopp/include/cryptopp/misc.h \
    cryptopp/include/cryptopp/iterhash.h \
    cryptopp/include/cryptopp/cryptlib.h \
    cryptopp/include/cryptopp/cpu.h \
    cryptopp/include/cryptopp/config.h \
    core/include/strlcpy.h \
    core/include/main.h \
    core/include/net.h \
    core/include/key.h \
    core/include/db.h \
    core/include/script.h \
    core/include/noui.h \
    core/include/init.h \
    core/include/headers.h \
    core/include/irc.h \
    json/include/json/json_spirit_writer_template.h \
    json/include/json/json_spirit_writer.h \
    json/include/json/json_spirit_value.h \
    json/include/json/json_spirit_utils.h \
    json/include/json/json_spirit_stream_reader.h \
    json/include/json/json_spirit_reader_template.h \
    json/include/json/json_spirit_reader.h \
    json/include/json/json_spirit_error_position.h \
    json/include/json/json_spirit.h \
    core/include/rpc.h \
    gui/src/clientmodel.h \
    gui/include/clientmodel.h \
    gui/include/guiutil.h \
    gui/include/transactionrecord.h \
    gui/include/guiconstants.h
SOURCES += gui/src/bitcoin.cpp gui/src/bitcoingui.cpp \
    gui/src/transactiontablemodel.cpp \
    gui/src/addresstablemodel.cpp \
    gui/src/optionsdialog.cpp \
    gui/src/mainoptionspage.cpp \
    gui/src/sendcoinsdialog.cpp \
    gui/src/addressbookdialog.cpp \
    gui/src/aboutdialog.cpp \
    gui/src/editaddressdialog.cpp \
    gui/src/bitcoinaddressvalidator.cpp \
    cryptopp/src/sha.cpp \
    cryptopp/src/cpu.cpp \
    core/src/util.cpp \
    core/src/script.cpp \
    core/src/main.cpp \
    core/src/init.cpp \
    core/src/rpc.cpp \
    core/src/net.cpp \
    core/src/irc.cpp \
    core/src/db.cpp \
    json/src/json_spirit_writer.cpp \
    json/src/json_spirit_value.cpp \
    json/src/json_spirit_reader.cpp \
    gui/src/clientmodel.cpp \
    gui/src/guiutil.cpp \
    gui/src/transactionrecord.cpp

RESOURCES += \
    gui/bitcoin.qrc

FORMS += \
    gui/forms/sendcoinsdialog.ui \
    gui/forms/addressbookdialog.ui \
    gui/forms/aboutdialog.ui \
    gui/forms/editaddressdialog.ui
