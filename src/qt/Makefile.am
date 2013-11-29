include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src \
  -I$(top_builddir)/src/qt \
  -I$(top_builddir)/src/qt/forms \
  $(PROTOBUF_CFLAGS) \
  $(QR_CFLAGS)
bin_PROGRAMS = bitcoin-qt
noinst_LIBRARIES = libbitcoinqt.a
SUBDIRS = $(BUILD_TEST_QT)
DIST_SUBDIRS = test

# bitcoin qt core #
QT_TS = locale/bitcoin_ach.ts \
  locale/bitcoin_af_ZA.ts \
  locale/bitcoin_ar.ts \
  locale/bitcoin_be_BY.ts \
  locale/bitcoin_bg.ts \
  locale/bitcoin_bs.ts \
  locale/bitcoin_ca_ES.ts \
  locale/bitcoin_ca.ts \
  locale/bitcoin_cs.ts \
  locale/bitcoin_cy.ts \
  locale/bitcoin_da.ts \
  locale/bitcoin_de.ts \
  locale/bitcoin_de_AT.ts \
  locale/bitcoin_el_GR.ts \
  locale/bitcoin_en.ts \
  locale/bitcoin_eo.ts \
  locale/bitcoin_es_CL.ts \
  locale/bitcoin_es.ts \
  locale/bitcoin_et.ts \
  locale/bitcoin_eu_ES.ts \
  locale/bitcoin_fa_IR.ts \
  locale/bitcoin_fa.ts \
  locale/bitcoin_fi.ts \
  locale/bitcoin_fr_CA.ts \
  locale/bitcoin_fr.ts \
  locale/bitcoin_gl.ts \
  locale/bitcoin_gu_IN.ts \
  locale/bitcoin_he.ts \
  locale/bitcoin_hi_IN.ts \
  locale/bitcoin_hr.ts \
  locale/bitcoin_hu.ts \
  locale/bitcoin_id_ID.ts \
  locale/bitcoin_it.ts \
  locale/bitcoin_ja.ts \
  locale/bitcoin_kk_KZ.ts \
  locale/bitcoin_ko_KR.ts \
  locale/bitcoin_la.ts \
  locale/bitcoin_lt.ts \
  locale/bitcoin_lv_LV.ts \
  locale/bitcoin_ms_MY.ts \
  locale/bitcoin_nb.ts \
  locale/bitcoin_nl.ts \
  locale/bitcoin_pl.ts \
  locale/bitcoin_pt_BR.ts \
  locale/bitcoin_pt_PT.ts \
  locale/bitcoin_ro_RO.ts \
  locale/bitcoin_ru.ts \
  locale/bitcoin_sk.ts \
  locale/bitcoin_sl_SI.ts \
  locale/bitcoin_sq.ts \
  locale/bitcoin_sr.ts \
  locale/bitcoin_sv.ts \
  locale/bitcoin_th_TH.ts \
  locale/bitcoin_tr.ts \
  locale/bitcoin_uk.ts \
  locale/bitcoin_vi.ts \
  locale/bitcoin_vi_VN.ts \
  locale/bitcoin_zh_CN.ts \
  locale/bitcoin_zh_TW.ts

QT_FORMS_UI = forms/aboutdialog.ui forms/addressbookpage.ui \
  forms/askpassphrasedialog.ui \
  forms/coincontroldialog.ui \
  forms/editaddressdialog.ui \
  forms/intro.ui \
  forms/openuridialog.ui \
  forms/optionsdialog.ui forms/overviewpage.ui forms/receiverequestdialog.ui \
  forms/receivecoinsdialog.ui \
  forms/rpcconsole.ui forms/sendcoinsdialog.ui forms/sendcoinsentry.ui \
  forms/signverifymessagedialog.ui forms/transactiondescdialog.ui

QT_MOC_CPP = moc_aboutdialog.cpp moc_addressbookpage.cpp \
  moc_addresstablemodel.cpp moc_askpassphrasedialog.cpp \
  moc_bitcoinaddressvalidator.cpp moc_bitcoinamountfield.cpp \
  moc_bitcoingui.cpp moc_bitcoinunits.cpp moc_clientmodel.cpp \
  moc_coincontroldialog.cpp \
  moc_coincontroltreewidget.cpp \
  moc_csvmodelwriter.cpp moc_editaddressdialog.cpp moc_guiutil.cpp \
  moc_intro.cpp moc_macdockiconhandler.cpp moc_macnotificationhandler.cpp \
  moc_monitoreddatamapper.cpp moc_notificator.cpp \
  moc_openuridialog.cpp \
  moc_optionsdialog.cpp \
  moc_optionsmodel.cpp moc_overviewpage.cpp moc_paymentserver.cpp \
  moc_receiverequestdialog.cpp moc_qvalidatedlineedit.cpp moc_qvaluecombobox.cpp \
  moc_receivecoinsdialog.cpp \
  moc_rpcconsole.cpp moc_sendcoinsdialog.cpp moc_sendcoinsentry.cpp \
  moc_signverifymessagedialog.cpp moc_splashscreen.cpp moc_trafficgraphwidget.cpp moc_transactiondesc.cpp \
  moc_transactiondescdialog.cpp moc_transactionfilterproxy.cpp \
  moc_transactiontablemodel.cpp moc_transactionview.cpp moc_walletframe.cpp \
  moc_walletmodel.cpp moc_walletview.cpp

BITCOIN_MM = macdockiconhandler.mm macnotificationhandler.mm

QT_MOC = intro.moc overviewpage.moc rpcconsole.moc

QT_QRC_CPP = qrc_bitcoin.cpp
QT_QRC = bitcoin.qrc

PROTOBUF_CC = paymentrequest.pb.cc
PROTOBUF_H = paymentrequest.pb.h
PROTOBUF_PROTO = paymentrequest.proto

BITCOIN_QT_H  = aboutdialog.h addressbookpage.h addresstablemodel.h \
  askpassphrasedialog.h bitcoinaddressvalidator.h bitcoinamountfield.h \
  bitcoingui.h bitcoinunits.h clientmodel.h coincontroldialog.h coincontroltreewidget.h csvmodelwriter.h \
  editaddressdialog.h guiconstants.h guiutil.h intro.h macdockiconhandler.h \
  macnotificationhandler.h monitoreddatamapper.h notificator.h \
  openuridialog.h \
  optionsdialog.h \
  optionsmodel.h overviewpage.h paymentrequestplus.h paymentserver.h \
  receivecoinsdialog.h \
  receiverequestdialog.h qvalidatedlineedit.h qvaluecombobox.h rpcconsole.h \
  sendcoinsdialog.h sendcoinsentry.h signverifymessagedialog.h splashscreen.h \
  trafficgraphwidget.h transactiondescdialog.h transactiondesc.h transactionfilterproxy.h \
  transactionrecord.h transactiontablemodel.h transactionview.h walletframe.h \
  walletmodel.h walletmodeltransaction.h walletview.h

RES_ICONS = res/icons/bitcoin.png res/icons/address-book.png \
  res/icons/quit.png res/icons/send.png res/icons/toolbar.png \
  res/icons/connect0_16.png res/icons/connect1_16.png \
  res/icons/connect2_16.png res/icons/connect3_16.png \
  res/icons/connect4_16.png res/icons/transaction0.png \
  res/icons/transaction2.png res/icons/clock1.png res/icons/clock2.png \
  res/icons/clock3.png res/icons/clock4.png res/icons/clock5.png \
  res/icons/configure.png res/icons/receive.png res/icons/editpaste.png \
  res/icons/editcopy.png res/icons/add.png res/icons/bitcoin_testnet.png \
  res/icons/toolbar_testnet.png res/icons/edit.png res/icons/history.png \
  res/icons/overview.png res/icons/export.png res/icons/synced.png \
  res/icons/remove.png res/icons/tx_mined.png res/icons/tx_input.png \
  res/icons/tx_output.png res/icons/tx_inout.png res/icons/lock_closed.png \
  res/icons/lock_open.png res/icons/key.png res/icons/filesave.png \
  res/icons/qrcode.png res/icons/debugwindow.png res/icons/bitcoin.ico \
  res/icons/bitcoin_testnet.ico

BITCOIN_QT_CPP = aboutdialog.cpp addressbookpage.cpp \
  addresstablemodel.cpp askpassphrasedialog.cpp bitcoinaddressvalidator.cpp \
  bitcoinamountfield.cpp bitcoin.cpp bitcoingui.cpp \
  bitcoinunits.cpp clientmodel.cpp \
  coincontroldialog.cpp \
  coincontroltreewidget.cpp \
  csvmodelwriter.cpp editaddressdialog.cpp \
  guiutil.cpp intro.cpp monitoreddatamapper.cpp notificator.cpp \
  openuridialog.cpp \
  optionsdialog.cpp optionsmodel.cpp overviewpage.cpp paymentrequestplus.cpp \
  paymentserver.cpp qvalidatedlineedit.cpp qvaluecombobox.cpp \
  receivecoinsdialog.cpp receiverequestdialog.cpp \
  rpcconsole.cpp sendcoinsdialog.cpp sendcoinsentry.cpp \
  signverifymessagedialog.cpp splashscreen.cpp trafficgraphwidget.cpp transactiondesc.cpp \
  transactiondescdialog.cpp transactionfilterproxy.cpp transactionrecord.cpp \
  transactiontablemodel.cpp transactionview.cpp walletframe.cpp \
  walletmodel.cpp walletmodeltransaction.cpp walletview.cpp

RES_IMAGES = res/images/about.png res/images/splash.png \
  res/images/splash_testnet.png

RES_MOVIES = res/movies/update_spinner.mng

BITCOIN_RC = res/bitcoin-qt-res.rc

libbitcoinqt_a_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms $(QT_DBUS_INCLUDES)
libbitcoinqt_a_SOURCES = $(BITCOIN_QT_CPP) $(BITCOIN_QT_H) $(QT_FORMS_UI) \
  $(QT_QRC) $(QT_TS) $(PROTOBUF_PROTO) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES)

nodist_libbitcoinqt_a_SOURCES = $(QT_MOC_CPP) $(QT_MOC) $(PROTOBUF_CC) \
  $(PROTOBUF_H) $(QT_QRC_CPP)

BUILT_SOURCES = $(nodist_libbitcoinqt_a_SOURCES)

#Generating these with a half-written protobuf header leads to wacky results.
#This makes sure it's done.
$(QT_MOC): $(PROTOBUF_H)
$(QT_MOC_CPP): $(PROTOBUF_H)

if TARGET_DARWIN
  libbitcoinqt_a_SOURCES += $(BITCOIN_MM)
endif
if TARGET_WINDOWS
  libbitcoinqt_a_SOURCES += $(BITCOIN_RC)
endif
#

# bitcoin-qt binary #
bitcoin_qt_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms
bitcoin_qt_SOURCES = bitcoin.cpp
bitcoin_qt_LDADD = libbitcoinqt.a $(LIBBITCOIN_SERVER) $(LIBBITCOIN_WALLET) $(LIBBITCOIN_CLI) $(LIBBITCOIN_COMMON) $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS) $(QR_LIBS) $(PROTOBUF_LIBS) $(BDB_LIBS)

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
	@$(LUPDATE) $^ -locations relative -no-obsolete -ts locale/bitcoin_en.ts

$(QT_QRC_CPP): $(QT_QRC) $(QT_QM) $(QT_FORMS_H) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES) $(PROTOBUF_H)
	@cd $(abs_srcdir); test -f $(RCC) && $(RCC) -name bitcoin -o $(abs_builddir)/$@ $< || \
	  echo error: could not build $@
	$(SED) -i.bak -e '/^\*\*.*Created:/d' $@ && rm $@.bak
	$(SED) -i.bak -e '/^\*\*.*by:/d' $@ && rm $@.bak

CLEANFILES = $(BUILT_SOURCES) $(QT_QM) $(QT_FORMS_H) *.gcda *.gcno
