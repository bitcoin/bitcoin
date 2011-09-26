TEMPLATE = app
TARGET =
INCLUDEPATH += src src/json src/cryptopp src/qt
DEFINES += QT_GUI
# DEFINES += SSL
CONFIG += no_include_pwd

# for boost 1.37, add -mt to the boost libraries
LIBS += -lssl -lcrypto -ldb_cxx
unix:!macx:LIBS += -lboost_system -lboost_filesystem -lboost_program_options -lboost_thread
macx:LIBS += -lboost_system-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_thread-mt
macx:DEFINES += __WXMAC_OSX__ MSG_NOSIGNAL=0 BOOST_FILESYSTEM_VERSION=3
windows:LIBS += -lboost_system-mgw44-mt-1_43 -lboost_filesystem-mgw44-mt-1_43 -lboost_program_options-mgw44-mt-1_43 -lboost_thread-mgw44-mt-1_43 -lws2_32 -lgdi32
windows:DEFINES += __WXMSW__
windows:RC_FILE = src/qt/res/bitcoin-qt.rc

# use: qmake "USE_UPNP=1"
# miniupnpc (http://miniupnp.free.fr/files/) must be installed
count(USE_UPNP, 1) {
    message(Building with UPNP support)
    DEFINES += USE_UPNP=$$USE_UPNP
    LIBS += -lminiupnpc
}

count(USE_DBUS, 1) {
    message(Building with DBUS (Freedesktop notifications) support)
    DEFINES += QT_DBUS
    QT += dbus
}

# for extra security against potential buffer overflows
QMAKE_CXXFLAGS += -fstack-protector 
QMAKE_LFLAGS += -fstack-protector

# disable quite some warnings because bitcoin core "sins" a lot
QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall -Wno-invalid-offsetof -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-char-subscripts  -Wno-unused-value -Wno-sequence-point -Wno-parentheses -Wno-unknown-pragmas -Wno-switch

# Input
DEPENDPATH += src/qt src src/cryptopp src json/include
HEADERS += src/qt/bitcoingui.h \
    src/qt/transactiontablemodel.h \
    src/qt/addresstablemodel.h \
    src/qt/optionsdialog.h \
    src/qt/sendcoinsdialog.h \
    src/qt/addressbookpage.h \
    src/qt/aboutdialog.h \
    src/qt/editaddressdialog.h \
    src/qt/bitcoinaddressvalidator.h \
    src/base58.h \
    src/bignum.h \
    src/util.h \
    src/uint256.h \
    src/serialize.h \
    src/cryptopp/stdcpp.h \
    src/cryptopp/smartptr.h \
    src/cryptopp/simple.h \
    src/cryptopp/sha.h \
    src/cryptopp/secblock.h \
    src/cryptopp/pch.h \
    src/cryptopp/misc.h \
    src/cryptopp/iterhash.h \
    src/cryptopp/cryptlib.h \
    src/cryptopp/cpu.h \
    src/cryptopp/config.h \
    src/strlcpy.h \
    src/main.h \
    src/net.h \
    src/key.h \
    src/db.h \
    src/script.h \
    src/noui.h \
    src/init.h \
    src/headers.h \
    src/irc.h \
    src/json/json_spirit_writer_template.h \
    src/json/json_spirit_writer.h \
    src/json/json_spirit_value.h \
    src/json/json_spirit_utils.h \
    src/json/json_spirit_stream_reader.h \
    src/json/json_spirit_reader_template.h \
    src/json/json_spirit_reader.h \
    src/json/json_spirit_error_position.h \
    src/json/json_spirit.h \
    src/qt/clientmodel.h \
    src/qt/guiutil.h \
    src/qt/transactionrecord.h \
    src/qt/guiconstants.h \
    src/qt/optionsmodel.h \
    src/qt/monitoreddatamapper.h \
    src/qtui.h \
    src/qt/transactiondesc.h \
    src/qt/transactiondescdialog.h \
    src/qt/bitcoinamountfield.h \
    src/wallet.h \
    src/keystore.h \
    src/qt/transactionfilterproxy.h \
    src/qt/transactionview.h \
    src/qt/walletmodel.h \
    src/bitcoinrpc.h \
    src/qt/overviewpage.h \
    src/qt/csvmodelwriter.h \
    src/crypter.h \
    src/qt/sendcoinsentry.h \
    src/qt/qvalidatedlineedit.h \
    src/qt/bitcoinunits.h \
    src/qt/qvaluecombobox.h \
    src/qt/askpassphrasedialog.h \
    src/protocol.h \
    src/qt/notificator.h

SOURCES += src/qt/bitcoin.cpp src/qt/bitcoingui.cpp \
    src/qt/transactiontablemodel.cpp \
    src/qt/addresstablemodel.cpp \
    src/qt/optionsdialog.cpp \
    src/qt/sendcoinsdialog.cpp \
    src/qt/addressbookpage.cpp \
    src/qt/aboutdialog.cpp \
    src/qt/editaddressdialog.cpp \
    src/qt/bitcoinaddressvalidator.cpp \
    src/cryptopp/sha.cpp \
    src/cryptopp/cpu.cpp \
    src/util.cpp \
    src/script.cpp \
    src/main.cpp \
    src/init.cpp \
    src/net.cpp \
    src/irc.cpp \
    src/db.cpp \
    src/json/json_spirit_writer.cpp \
    src/json/json_spirit_value.cpp \
    src/json/json_spirit_reader.cpp \
    src/qt/clientmodel.cpp \
    src/qt/guiutil.cpp \
    src/qt/transactionrecord.cpp \
    src/qt/optionsmodel.cpp \
    src/qt/monitoreddatamapper.cpp \
    src/qt/transactiondesc.cpp \
    src/qt/transactiondescdialog.cpp \
    src/qt/bitcoinstrings.cpp \
    src/qt/bitcoinamountfield.cpp \
    src/wallet.cpp \
    src/keystore.cpp \
    src/qt/transactionfilterproxy.cpp \
    src/qt/transactionview.cpp \
    src/qt/walletmodel.cpp \
    src/bitcoinrpc.cpp \
    src/qt/overviewpage.cpp \
    src/qt/csvmodelwriter.cpp \
    src/crypter.cpp \
    src/qt/sendcoinsentry.cpp \
    src/qt/qvalidatedlineedit.cpp \
    src/qt/bitcoinunits.cpp \
    src/qt/qvaluecombobox.cpp \
    src/qt/askpassphrasedialog.cpp \
    src/protocol.cpp \
    src/qt/notificator.cpp

RESOURCES += \
    src/qt/bitcoin.qrc

FORMS += \
    src/qt/forms/sendcoinsdialog.ui \
    src/qt/forms/addressbookpage.ui \
    src/qt/forms/aboutdialog.ui \
    src/qt/forms/editaddressdialog.ui \
    src/qt/forms/transactiondescdialog.ui \
    src/qt/forms/overviewpage.ui \
    src/qt/forms/sendcoinsentry.ui \
    src/qt/forms/askpassphrasedialog.ui

CODECFORTR = UTF-8
# for lrelease/lupdate
TRANSLATIONS = src/qt/locale/bitcoin_nl.ts src/qt/locale/bitcoin_de.ts \
               src/qt/locale/bitcoin_ru.ts

OTHER_FILES += \
    README.rst

# For use with MacPorts
macx:INCLUDEPATH += /opt/local/include /opt/local/include/db48
macx:LIBS += -L/opt/local/lib -L/opt/local/lib/db48

# Additional Mac options
macx:ICON = src/qt/res/icons/bitcoin.icns
macx:TARGET = "Bitcoin Qt"
