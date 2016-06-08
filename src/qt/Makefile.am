include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src \
  -I$(top_builddir)/src/qt \
  -I$(top_builddir)/src/qt/forms \
  $(PROTOBUF_CFLAGS) \
  $(QR_CFLAGS)
bin_PROGRAMS = crowncoin-qt
noinst_LIBRARIES = libcrowncoinqt.a
SUBDIRS = . $(BUILD_TEST_QT)
DIST_SUBDIRS = . test

# crowncoin qt core #
QT_TS = \
  locale/crowncoin_ach.ts \
  locale/crowncoin_af_ZA.ts \
  locale/crowncoin_ar.ts \
  locale/crowncoin_be_BY.ts \
  locale/crowncoin_bg.ts \
  locale/crowncoin_bs.ts \
  locale/crowncoin_ca_ES.ts \
  locale/crowncoin_ca.ts \
  locale/crowncoin_ca@valencia.ts \
  locale/crowncoin_cmn.ts \
  locale/crowncoin_cs.ts \
  locale/crowncoin_cy.ts \
  locale/crowncoin_da.ts \
  locale/crowncoin_de.ts \
  locale/crowncoin_el_GR.ts \
  locale/crowncoin_en.ts \
  locale/crowncoin_eo.ts \
  locale/crowncoin_es_CL.ts \
  locale/crowncoin_es_DO.ts \
  locale/crowncoin_es_MX.ts \
  locale/crowncoin_es.ts \
  locale/crowncoin_es_UY.ts \
  locale/crowncoin_et.ts \
  locale/crowncoin_eu_ES.ts \
  locale/crowncoin_fa_IR.ts \
  locale/crowncoin_fa.ts \
  locale/crowncoin_fi.ts \
  locale/crowncoin_fr_CA.ts \
  locale/crowncoin_fr.ts \
  locale/crowncoin_gl.ts \
  locale/crowncoin_gu_IN.ts \
  locale/crowncoin_he.ts \
  locale/crowncoin_hi_IN.ts \
  locale/crowncoin_hr.ts \
  locale/crowncoin_hu.ts \
  locale/crowncoin_id_ID.ts \
  locale/crowncoin_it.ts \
  locale/crowncoin_ja.ts \
  locale/crowncoin_ka.ts \
  locale/crowncoin_kk_KZ.ts \
  locale/crowncoin_ko_KR.ts \
  locale/crowncoin_ky.ts \
  locale/crowncoin_la.ts \
  locale/crowncoin_lt.ts \
  locale/crowncoin_lv_LV.ts \
  locale/crowncoin_mn.ts \
  locale/crowncoin_ms_MY.ts \
  locale/crowncoin_nb.ts \
  locale/crowncoin_nl.ts \
  locale/crowncoin_pam.ts \
  locale/crowncoin_pl.ts \
  locale/crowncoin_pt_BR.ts \
  locale/crowncoin_pt_PT.ts \
  locale/crowncoin_ro_RO.ts \
  locale/crowncoin_ru.ts \
  locale/crowncoin_sah.ts \
  locale/crowncoin_sk.ts \
  locale/crowncoin_sl_SI.ts \
  locale/crowncoin_sq.ts \
  locale/crowncoin_sr.ts \
  locale/crowncoin_sv.ts \
  locale/crowncoin_th_TH.ts \
  locale/crowncoin_tr.ts \
  locale/crowncoin_uk.ts \
  locale/crowncoin_ur_PK.ts \
  locale/crowncoin_uz@Cyrl.ts \
  locale/crowncoin_vi.ts \
  locale/crowncoin_vi_VN.ts \
  locale/crowncoin_zh_CN.ts \
  locale/crowncoin_zh_HK.ts \
  locale/crowncoin_zh_TW.ts

QT_FORMS_UI = \
  forms/aboutdialog.ui \
  forms/addressbookpage.ui \
  forms/askpassphrasedialog.ui \
  forms/coincontroldialog.ui \
  forms/editaddressdialog.ui \
  forms/helpmessagedialog.ui \
  forms/intro.ui \
  forms/openuridialog.ui \
  forms/optionsdialog.ui \
  forms/overviewpage.ui \
  forms/receivecoinsdialog.ui \
  forms/receiverequestdialog.ui \
  forms/rpcconsole.ui \
  forms/sendcoinsdialog.ui \
  forms/sendcoinsentry.ui \
  forms/signverifymessagedialog.ui \
  forms/transactiondescdialog.ui

QT_MOC_CPP = \
  moc_addressbookpage.cpp \
  moc_addresstablemodel.cpp \
  moc_askpassphrasedialog.cpp \
  moc_crowncoinaddressvalidator.cpp \
  moc_crowncoinamountfield.cpp \
  moc_crowncoingui.cpp \
  moc_crowncoinunits.cpp \
  moc_clientmodel.cpp \
  moc_coincontroldialog.cpp \
  moc_coincontroltreewidget.cpp \
  moc_csvmodelwriter.cpp \
  moc_editaddressdialog.cpp \
  moc_guiutil.cpp \
  moc_intro.cpp \
  moc_macdockiconhandler.cpp \
  moc_macnotificationhandler.cpp \
  moc_monitoreddatamapper.cpp \
  moc_notificator.cpp \
  moc_openuridialog.cpp \
  moc_optionsdialog.cpp \
  moc_optionsmodel.cpp \
  moc_overviewpage.cpp \
  moc_paymentserver.cpp \
  moc_qvalidatedlineedit.cpp \
  moc_qvaluecombobox.cpp \
  moc_receivecoinsdialog.cpp \
  moc_receiverequestdialog.cpp \
  moc_recentrequeststablemodel.cpp \
  moc_rpcconsole.cpp \
  moc_sendcoinsdialog.cpp \
  moc_sendcoinsentry.cpp \
  moc_signverifymessagedialog.cpp \
  moc_splashscreen.cpp \
  moc_trafficgraphwidget.cpp \
  moc_transactiondesc.cpp \
  moc_transactiondescdialog.cpp \
  moc_transactionfilterproxy.cpp \
  moc_transactiontablemodel.cpp \
  moc_transactionview.cpp \
  moc_utilitydialog.cpp \
  moc_walletframe.cpp \
  moc_walletmodel.cpp \
  moc_walletview.cpp

CROWNCOIN_MM = \
  macdockiconhandler.mm \
  macnotificationhandler.mm

QT_MOC = \
  crowncoin.moc \
  intro.moc \
  overviewpage.moc \
  rpcconsole.moc

QT_QRC_CPP = qrc_crowncoin.cpp
QT_QRC = crowncoin.qrc

PROTOBUF_CC = paymentrequest.pb.cc
PROTOBUF_H = paymentrequest.pb.h
PROTOBUF_PROTO = paymentrequest.proto

CROWNCOIN_QT_H = \
  addressbookpage.h \
  addresstablemodel.h \
  askpassphrasedialog.h \
  crowncoinaddressvalidator.h \
  crowncoinamountfield.h \
  crowncoingui.h \
  crowncoinunits.h \
  clientmodel.h \
  coincontroldialog.h \
  coincontroltreewidget.h \
  csvmodelwriter.h \
  editaddressdialog.h \
  guiconstants.h \
  guiutil.h \
  intro.h \
  macdockiconhandler.h \
  macnotificationhandler.h \
  monitoreddatamapper.h \
  notificator.h \
  openuridialog.h \
  optionsdialog.h \
  optionsmodel.h \
  overviewpage.h \
  paymentrequestplus.h \
  paymentserver.h \
  qvalidatedlineedit.h \
  qvaluecombobox.h \
  receivecoinsdialog.h \
  receiverequestdialog.h \
  recentrequeststablemodel.h \
  rpcconsole.h \
  sendcoinsdialog.h \
  sendcoinsentry.h \
  signverifymessagedialog.h \
  splashscreen.h \
  trafficgraphwidget.h \
  transactiondesc.h \
  transactiondescdialog.h \
  transactionfilterproxy.h \
  transactionrecord.h \
  transactiontablemodel.h \
  transactionview.h \
  utilitydialog.h \
  walletframe.h \
  walletmodel.h \
  walletmodeltransaction.h \
  walletview.h \
  winshutdownmonitor.h

RES_ICONS = \
  res/icons/add.png \
  res/icons/address-book.png \
  res/icons/crowncoin.ico \
  res/icons/crowncoin.png \
  res/icons/crowncoin_testnet.ico \
  res/icons/crowncoin_testnet.png \
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
  res/icons/key.png \
  res/icons/lock_closed.png \
  res/icons/lock_open.png \
  res/icons/overview.png \
  res/icons/qrcode.png \
  res/icons/quit.png \
  res/icons/receive.png \
  res/icons/remove.png \
  res/icons/send.png \
  res/icons/synced.png \
  res/icons/toolbar.png \
  res/icons/toolbar_testnet.png \
  res/icons/transaction0.png \
  res/icons/transaction2.png \
  res/icons/transaction_conflicted.png \
  res/icons/tx_inout.png \
  res/icons/tx_input.png \
  res/icons/tx_output.png \
  res/icons/tx_mined.png

CROWNCOIN_QT_CPP = \
  crowncoin.cpp \
  crowncoinaddressvalidator.cpp \
  crowncoinamountfield.cpp \
  crowncoingui.cpp \
  crowncoinunits.cpp \
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
  splashscreen.cpp \
  trafficgraphwidget.cpp \
  utilitydialog.cpp \
  winshutdownmonitor.cpp

if ENABLE_WALLET
CROWNCOIN_QT_CPP += \
  addressbookpage.cpp \
  addresstablemodel.cpp \
  askpassphrasedialog.cpp \
  coincontroldialog.cpp \
  coincontroltreewidget.cpp \
  editaddressdialog.cpp \
  openuridialog.cpp \
  overviewpage.cpp \
  paymentrequestplus.cpp \
  paymentserver.cpp \
  receivecoinsdialog.cpp \
  receiverequestdialog.cpp \
  recentrequeststablemodel.cpp \
  sendcoinsdialog.cpp \
  sendcoinsentry.cpp \
  signverifymessagedialog.cpp \
  transactiondesc.cpp \
  transactiondescdialog.cpp \
  transactionfilterproxy.cpp \
  transactionrecord.cpp \
  transactiontablemodel.cpp \
  transactionview.cpp \
  walletframe.cpp \
  walletmodel.cpp \
  walletmodeltransaction.cpp \
  walletview.cpp
endif

RES_IMAGES = \
  res/images/about.png \
  res/images/splash.png \
  res/images/splash_testnet.png

RES_MOVIES = $(wildcard res/movies/spinner-*.png)

CROWNCOIN_RC = res/crowncoin-qt-res.rc

libcrowncoinqt_a_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms $(QT_DBUS_INCLUDES)
libcrowncoinqt_a_SOURCES = $(CROWNCOIN_QT_CPP) $(CROWNCOIN_QT_H) $(QT_FORMS_UI) \
  $(QT_QRC) $(QT_TS) $(PROTOBUF_PROTO) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES)

nodist_libcrowncoinqt_a_SOURCES = $(QT_MOC_CPP) $(QT_MOC) $(PROTOBUF_CC) \
  $(PROTOBUF_H) $(QT_QRC_CPP)

BUILT_SOURCES = $(nodist_libcrowncoinqt_a_SOURCES)

#Generating these with a half-written protobuf header leads to wacky results.
#This makes sure it's done.
$(QT_MOC): $(PROTOBUF_H)
$(QT_MOC_CPP): $(PROTOBUF_H)

# crowncoin-qt binary #
crowncoin_qt_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms
crowncoin_qt_SOURCES = crowncoin.cpp
if TARGET_DARWIN
  crowncoin_qt_SOURCES += $(CROWNCOIN_MM)
endif
if TARGET_WINDOWS
  crowncoin_qt_SOURCES += $(CROWNCOIN_RC)
endif
crowncoin_qt_LDADD = libcrowncoinqt.a $(LIBCROWNCOIN_SERVER)
if ENABLE_WALLET
crowncoin_qt_LDADD += $(LIBCROWNCOIN_WALLET)
endif
crowncoin_qt_LDADD += $(LIBCROWNCOIN_CLI) $(LIBCROWNCOIN_COMMON) $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS) $(QR_LIBS) $(PROTOBUF_LIBS) $(BDB_LIBS)
crowncoin_qt_LDFLAGS = $(QT_LDFLAGS)

# forms/foo.h -> forms/ui_foo.h
QT_FORMS_H=$(join $(dir $(QT_FORMS_UI)),$(addprefix ui_, $(notdir $(QT_FORMS_UI:.ui=.h))))

#locale/foo.ts -> locale/foo.qm
QT_QM=$(QT_TS:.ts=.qm)

.PHONY: FORCE
.SECONDARY: $(QT_QM)

crowncoinstrings.cpp: FORCE
	$(MAKE) -C $(top_srcdir)/src qt/crowncoinstrings.cpp

translate: crowncoinstrings.cpp $(QT_FORMS_UI) $(QT_FORMS_UI) $(CROWNCOIN_QT_CPP) $(CROWNCOIN_QT_H) $(CROWNCOIN_MM)
	@test -n $(LUPDATE) || echo "lupdate is required for updating translations"
	@QT_SELECT=$(QT_SELECT) $(LUPDATE) $^ -locations relative -no-obsolete -ts locale/crowncoin_en.ts

$(QT_QRC_CPP): $(QT_QRC) $(QT_QM) $(QT_FORMS_H) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES) $(PROTOBUF_H)
	@cd $(abs_srcdir); test -f $(RCC) && QT_SELECT=$(QT_SELECT) $(RCC) -name crowncoin -o $(abs_builddir)/$@ $< || \
	  echo error: could not build $@
	$(SED) -e '/^\*\*.*Created:/d' $@ > $@.n && mv $@{.n,}
	$(SED) -e '/^\*\*.*by:/d' $@  > $@.n && mv $@{.n,}

CLEANFILES = $(BUILT_SOURCES) $(QT_QM) $(QT_FORMS_H) *.gcda *.gcno
