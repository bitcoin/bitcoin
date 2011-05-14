TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += gui/include lib/include cryptopp/include

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
    lib/include/base58.h \
    lib/include/bignum.h \
    lib/include/util.h \
    lib/include/uint256.h \
    lib/include/serialize.h \
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
    cryptopp/include/cryptopp/config.h
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
    cryptopp/src/cpu.cpp

RESOURCES += \
    gui/bitcoin.qrc

FORMS += \
    gui/forms/sendcoinsdialog.ui \
    gui/forms/addressbookdialog.ui \
    gui/forms/aboutdialog.ui \
    gui/forms/editaddressdialog.ui
