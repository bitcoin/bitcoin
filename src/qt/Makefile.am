include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src \
  -I$(top_builddir)/src/qt \
  -I$(top_builddir)/src/qt/forms \
  $(PROTOBUF_CFLAGS) \
  $(QR_CFLAGS)
bin_PROGRAMS = credits-qt
noinst_LIBRARIES = libcreditsqt.a
SUBDIRS = . $(BUILD_TEST_QT)
DIST_SUBDIRS = . test

# bitcoin qt core #
QT_TS = \
  locale/bitcoin_en.ts

QT_FORMS_UI = \
  forms/aboutdialog.ui \
  forms/addressbookpage.ui \
  forms/bitcoin_addressbookpage.ui \
  forms/askpassphrasedialog.ui \
  forms/bitcoin_askpassphrasedialog.ui \
  forms/claimcoinsdialog.ui \
  forms/coincontroldialog.ui \
  forms/bitcoin_coincontroldialog.ui \
  forms/miner_coincontroldialog.ui \
  forms/editaddressdialog.ui \
  forms/bitcoin_editaddressdialog.ui \
  forms/helpmessagedialog.ui \
  forms/intro.ui \
  forms/minercoinsdialog.ui \
  forms/uneditablecoinsentry.ui \
  forms/openuridialog.ui \
  forms/optionsdialog.ui \
  forms/overviewpage.ui \
  forms/bitcoin_overviewpage.ui \
  forms/receivecoinsdialog.ui \
  forms/bitcoin_receivecoinsdialog.ui \
  forms/receiverequestdialog.ui \
  forms/bitcoin_receiverequestdialog.ui \
  forms/rpcconsole.ui \
  forms/bitcoin_rpcconsole.ui \
  forms/sendcoinsdialog.ui \
  forms/bitcoin_sendcoinsdialog.ui \
  forms/sendcoinsentry.ui \
  forms/bitcoin_sendcoinsentry.ui \
  forms/signverifymessagedialog.ui \
  forms/transactiondescdialog.ui \
  forms/bitcoin_transactiondescdialog.ui

QT_MOC_CPP = \
  moc_addressbookpage.cpp \
  moc_bitcoin_addressbookpage.cpp \
  moc_addresstablemodel.cpp \
  moc_bitcoin_addresstablemodel.cpp \
  moc_askpassphrasedialog.cpp \
  moc_bitcoin_askpassphrasedialog.cpp \
  moc_bitcoinaddressvalidator.cpp \
  moc_bitcoinamountfield.cpp \
  moc_bitcoingui.cpp \
  moc_bitcreditunits.cpp \
  moc_bitcoinunits.cpp \
  moc_claimcoinsdialog.cpp \
  moc_clientmodel.cpp \
  moc_coincontroldialog.cpp \
  moc_bitcoin_coincontroldialog.cpp \
  moc_miner_coincontroldialog.cpp \
  moc_coincontroltreewidget.cpp \
  moc_bitcoin_coincontroltreewidget.cpp \
  moc_miner_coincontroltreewidget.cpp \
  moc_uneditablecoinsentry.cpp \
  moc_csvmodelwriter.cpp \
  moc_editaddressdialog.cpp \
  moc_bitcoin_editaddressdialog.cpp \
  moc_guiutil.cpp \
  moc_intro.cpp \
  moc_macdockiconhandler.cpp \
  moc_macnotificationhandler.cpp \
  moc_minercoinsdialog.cpp \
  moc_minerdepositsview.cpp \
  moc_monitoreddatamapper.cpp \
  moc_notificator.cpp \
  moc_openuridialog.cpp \
  moc_optionsdialog.cpp \
  moc_optionsmodel.cpp \
  moc_overviewpage.cpp \
  moc_bitcoin_overviewpage.cpp \
  moc_peertablemodel.cpp \
  moc_paymentserver.cpp \
  moc_qvalidatedlineedit.cpp \
  moc_qvaluecombobox.cpp \
  moc_receivecoinsdialog.cpp \
  moc_bitcoin_receivecoinsdialog.cpp \
  moc_receiverequestdialog.cpp \
  moc_bitcoin_receiverequestdialog.cpp \
  moc_recentrequeststablemodel.cpp \
  moc_bitcoin_recentrequeststablemodel.cpp \
  moc_rpcconsole.cpp \
  moc_bitcoin_rpcconsole.cpp \
  moc_sendcoinsdialog.cpp \
  moc_bitcoin_sendcoinsdialog.cpp \
  moc_sendcoinsentry.cpp \
  moc_bitcoin_sendcoinsentry.cpp \
  moc_signverifymessagedialog.cpp \
  moc_splashscreen.cpp \
  moc_trafficgraphwidget.cpp \
  moc_transactiondesc.cpp \
  moc_bitcoin_transactiondesc.cpp \
  moc_transactiondescdialog.cpp \
  moc_bitcoin_transactiondescdialog.cpp \
  moc_transactionfilterproxy.cpp \
  moc_bitcoin_transactionfilterproxy.cpp \
  moc_transactiontablemodel.cpp \
  moc_bitcoin_transactiontablemodel.cpp \
  moc_transactionview.cpp \
  moc_bitcoin_transactionview.cpp \
  moc_utilitydialog.cpp \
  moc_walletframe.cpp \
  moc_walletmodel.cpp \
  moc_bitcoin_walletmodel.cpp \
  moc_walletview.cpp

BITCOIN_MM = \
  macdockiconhandler.mm \
  macnotificationhandler.mm

QT_MOC = \
  bitcoin.moc \
  intro.moc \
  overviewpage.moc \
  bitcoin_overviewpage.moc \
  rpcconsole.moc  \
  bitcoin_rpcconsole.moc

QT_QRC_CPP = qrc_bitcoin.cpp
QT_QRC = bitcoin.qrc

PROTOBUF_CC = paymentrequest.pb.cc
PROTOBUF_H = paymentrequest.pb.h
PROTOBUF_PROTO = paymentrequest.proto

BITCOIN_QT_H = \
  addressbookpage.h \
  bitcoin_addressbookpage.h \
  addresstablemodel.h \
  bitcoin_addresstablemodel.h \
  askpassphrasedialog.h \
  bitcoin_askpassphrasedialog.h \
  bitcoinaddressvalidator.h \
  bitcoinamountfield.h \
  bitcoingui.h \
  bitcreditunits.h \
  bitcoinunits.h \
  claimcoinsdialog.h \
  clientmodel.h \
  coincontroldialog.h \
  bitcoin_coincontroldialog.h \
  miner_coincontroldialog.h \
  coincontroltreewidget.h \
  bitcoin_coincontroltreewidget.h \
  miner_coincontroltreewidget.h \
  uneditablecoinsentry.h \
  csvmodelwriter.h \
  editaddressdialog.h \
  bitcoin_editaddressdialog.h \
  guiconstants.h \
  guiutil.h \
  intro.h \
  macdockiconhandler.h \
  macnotificationhandler.h \
  minercoinsdialog.h \
  minerdepositsview.h \
  monitoreddatamapper.h \
  notificator.h \
  openuridialog.h \
  optionsdialog.h \
  optionsmodel.h \
  overviewpage.h \
  bitcoin_overviewpage.h \
  paymentrequestplus.h \
  paymentserver.h \
  peertablemodel.h \
  qvalidatedlineedit.h \
  qvaluecombobox.h \
  receivecoinsdialog.h \
  bitcoin_receivecoinsdialog.h \
  receiverequestdialog.h \
  bitcoin_receiverequestdialog.h \
  recentrequeststablemodel.h \
  bitcoin_recentrequeststablemodel.h \
  rpcconsole.h \
  bitcoin_rpcconsole.h \
  sendcoinsdialog.h \
  bitcoin_sendcoinsdialog.h \
  sendcoinsentry.h \
  bitcoin_sendcoinsentry.h \
  signverifymessagedialog.h \
  splashscreen.h \
  trafficgraphwidget.h \
  transactiondesc.h \
  bitcoin_transactiondesc.h \
  transactiondescdialog.h \
  bitcoin_transactiondescdialog.h \
  transactionfilterproxy.h \
  bitcoin_transactionfilterproxy.h \
  transactionrecord.h \
  bitcoin_transactionrecord.h \
  transactiontablemodel.h \
  bitcoin_transactiontablemodel.h \
  transactionview.h \
  bitcoin_transactionview.h \
  utilitydialog.h \
  walletframe.h \
  walletmodel.h \
  bitcoin_walletmodel.h \
  walletmodeltransaction.h \
  bitcoin_walletmodeltransaction.h \
  walletview.h \
  winshutdownmonitor.h

RES_ICONS = \
  res/icons/add.png \
  res/icons/address-book.png \
  res/icons/bitcredit.ico \
  res/icons/bitcredit.png \
  res/icons/bitcredit_testnet.ico \
  res/icons/bitcredit_testnet.png \
  res/icons/claim.png \
  res/icons/clock1.png \
  res/icons/clock2.png \
  res/icons/clock3.png \
  res/icons/clock4.png \
  res/icons/clock5.png \
  res/icons/configure.png \
  res/icons/connect0_16.png \
  res/icons/connect1_16.png \
  res/icons/connect2_16.png \
  res/icons/connect3_16.png \
  res/icons/connect4_16.png \
  res/icons/debugwindow.png \
  res/icons/edit.png \
  res/icons/editcopy.png \
  res/icons/editpaste.png \
  res/icons/export.png \
  res/icons/filesave.png \
  res/icons/history.png \
  res/icons/bitcoin_history.png \
  res/icons/key.png \
  res/icons/lock_closed.png \
  res/icons/lock_open.png \
  res/icons/mining.png \
  res/icons/mining_deposits.png \
  res/icons/overview.png \
  res/icons/bitcoin_overview.png \
  res/icons/qrcode.png \
  res/icons/quit.png \
  res/icons/receive.png \
  res/icons/bitcoin_receive.png \
  res/icons/remove.png \
  res/icons/send.png \
  res/icons/bitcoin_send.png \
  res/icons/synced.png \
  res/icons/toolbar.png \
  res/icons/toolbar_testnet.png \
  res/icons/transaction0.png \
  res/icons/transaction2.png \
  res/icons/transaction_conflicted.png \
  res/icons/tx_inout.png \
  res/icons/bitcoin_tx_inout.png \
  res/icons/tx_input.png \
  res/icons/tx_output.png \
  res/icons/tx_deposit.png \
  res/icons/tx_mined.png

BITCOIN_QT_CPP = \
  bitcoin.cpp \
  bitcoinaddressvalidator.cpp \
  bitcoinamountfield.cpp \
  bitcoingui.cpp \
  bitcreditunits.cpp \
  bitcoinunits.cpp \
  clientmodel.cpp \
  csvmodelwriter.cpp \
  guiutil.cpp \
  intro.cpp \
  monitoreddatamapper.cpp \
  notificator.cpp \
  optionsdialog.cpp \
  optionsmodel.cpp \
  qvalidatedlineedit.cpp \
  qvaluecombobox.cpp \
  rpcconsole.cpp \
  bitcoin_rpcconsole.cpp \
  splashscreen.cpp \
  trafficgraphwidget.cpp \
  utilitydialog.cpp \
  winshutdownmonitor.cpp

if ENABLE_WALLET
BITCOIN_QT_CPP += \
  addressbookpage.cpp \
  bitcoin_addressbookpage.cpp \
  addresstablemodel.cpp \
  bitcoin_addresstablemodel.cpp \
  askpassphrasedialog.cpp \
  bitcoin_askpassphrasedialog.cpp \
  claimcoinsdialog.cpp \
  coincontroldialog.cpp \
  bitcoin_coincontroldialog.cpp \
  miner_coincontroldialog.cpp \
  coincontroltreewidget.cpp \
  bitcoin_coincontroltreewidget.cpp \
  miner_coincontroltreewidget.cpp \
  editaddressdialog.cpp \
  bitcoin_editaddressdialog.cpp \
  minercoinsdialog.cpp \
  uneditablecoinsentry.cpp \
  minerdepositsview.cpp \
  openuridialog.cpp \
  overviewpage.cpp \
  bitcoin_overviewpage.cpp \
  paymentrequestplus.cpp \
  paymentserver.cpp \
  peertablemodel.cpp \
  receivecoinsdialog.cpp \
  bitcoin_receivecoinsdialog.cpp \
  receiverequestdialog.cpp \
  bitcoin_receiverequestdialog.cpp \
  recentrequeststablemodel.cpp \
  bitcoin_recentrequeststablemodel.cpp \
  sendcoinsdialog.cpp \
  bitcoin_sendcoinsdialog.cpp \
  sendcoinsentry.cpp \
  bitcoin_sendcoinsentry.cpp \
  signverifymessagedialog.cpp \
  transactiondesc.cpp \
  bitcoin_transactiondesc.cpp \
  transactiondescdialog.cpp \
  bitcoin_transactiondescdialog.cpp \
  transactionfilterproxy.cpp \
  bitcoin_transactionfilterproxy.cpp \
  transactionrecord.cpp \
  bitcoin_transactionrecord.cpp \
  transactiontablemodel.cpp \
  bitcoin_transactiontablemodel.cpp \
  transactionview.cpp \
  bitcoin_transactionview.cpp \
  walletframe.cpp \
  walletmodel.cpp \
  bitcoin_walletmodel.cpp \
  walletmodeltransaction.cpp \
  bitcoin_walletmodeltransaction.cpp \
  walletview.cpp
endif

RES_IMAGES = \
  res/images/about.png \
  res/images/splash.png \
  res/images/splash_testnet.png

RES_MOVIES = $(wildcard res/movies/spinner-*.png)

BITCOIN_RC = res/credits-qt-res.rc

libcreditsqt_a_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms $(QT_DBUS_INCLUDES)
libcreditsqt_a_SOURCES = $(BITCOIN_QT_CPP) $(BITCOIN_QT_H) $(QT_FORMS_UI) \
  $(QT_QRC) $(QT_TS) $(PROTOBUF_PROTO) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES)

nodist_libcreditsqt_a_SOURCES = $(QT_MOC_CPP) $(QT_MOC) $(PROTOBUF_CC) \
  $(PROTOBUF_H) $(QT_QRC_CPP)

BUILT_SOURCES = $(nodist_libcreditsqt_a_SOURCES)

#Generating these with a half-written protobuf header leads to wacky results.
#This makes sure it's done.
$(QT_MOC): $(PROTOBUF_H)
$(QT_MOC_CPP): $(PROTOBUF_H)

# credits-qt binary #
credits_qt_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms
credits_qt_SOURCES = bitcoin.cpp
if TARGET_DARWIN
  credits_qt_SOURCES += $(BITCOIN_MM)
endif
if TARGET_WINDOWS
  credits_qt_SOURCES += $(BITCOIN_RC)
endif
credits_qt_LDADD = libcreditsqt.a $(LIBCREDITS_SERVER)
if ENABLE_WALLET
credits_qt_LDADD += $(LIBCREDITS_WALLET)
endif
credits_qt_LDADD += $(LIBCREDITS_CLI) $(LIBCREDITS_COMMON) $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS) $(QR_LIBS) $(PROTOBUF_LIBS) $(BDB_LIBS)
credits_qt_LDFLAGS = $(QT_LDFLAGS)

# forms/foo.h -> forms/ui_foo.h
QT_FORMS_H=$(join $(dir $(QT_FORMS_UI)),$(addprefix ui_, $(notdir $(QT_FORMS_UI:.ui=.h))))

#locale/foo.ts -> locale/foo.qm
QT_QM=$(QT_TS:.ts=.qm)

.PHONY: FORCE
.SECONDARY: $(QT_QM)

bitcoinstrings.cpp: FORCE
	$(MAKE) -C $(top_srcdir)/src qt/bitcoinstrings.cpp

translate: bitcoinstrings.cpp $(QT_FORMS_UI) $(QT_FORMS_UI) $(BITCOIN_QT_CPP) $(BITCOIN_QT_H) $(BITCOIN_MM)
	@test -n $(LUPDATE) || echo "lupdate is required for updating translations"
	@QT_SELECT=$(QT_SELECT) $(LUPDATE) $^ -locations relative -no-obsolete -ts locale/bitcoin_en.ts

$(QT_QRC_CPP): $(QT_QRC) $(QT_QM) $(QT_FORMS_H) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES) $(PROTOBUF_H)
	@cd $(abs_srcdir); test -f $(RCC) && QT_SELECT=$(QT_SELECT) $(RCC) -name bitcoin -o $(abs_builddir)/$@ $< || \
	  echo error: could not build $@
	$(SED) -e '/^\*\*.*Created:/d' $@ > $@.n && mv $@{.n,}
	$(SED) -e '/^\*\*.*by:/d' $@  > $@.n && mv $@{.n,}

CLEANFILES = $(BUILT_SOURCES) $(QT_QM) $(QT_FORMS_H) *.gcda *.gcno
