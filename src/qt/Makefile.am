include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS =  $(INCLUDES) -I$(top_builddir)/src/obj \
  -I$(top_srcdir)/src/leveldb/include -I$(top_srcdir)/src \
  -I$(top_srcdir)/src/leveldb/helpers -I$(builddir)/src/qt $(BOOST_INCLUDES)
AM_LDFLAGS = $(PTHREAD_CFLAGS)
bin_PROGRAMS = bitcoin-qt
noinst_LIBRARIES = libbitcoinqt.a
SUBDIRS = $(BUILD_TEST_QT)
DIST_SUBDIRS = test

# bitcoin qt core #
QT_TS = locale/bitcoin_af_ZA.ts locale/bitcoin_ar.ts locale/bitcoin_bg.ts \
  locale/bitcoin_bs.ts locale/bitcoin_ca_ES.ts locale/bitcoin_ca.ts \
  locale/bitcoin_cs.ts locale/bitcoin_cy.ts locale/bitcoin_da.ts \
  locale/bitcoin_de.ts locale/bitcoin_el_GR.ts locale/bitcoin_en.ts \
  locale/bitcoin_eo.ts locale/bitcoin_es_CL.ts locale/bitcoin_es.ts \
  locale/bitcoin_et.ts locale/bitcoin_eu_ES.ts locale/bitcoin_fa_IR.ts \
  locale/bitcoin_fa.ts locale/bitcoin_fi.ts locale/bitcoin_fr_CA.ts \
  locale/bitcoin_fr.ts locale/bitcoin_gu_IN.ts locale/bitcoin_he.ts \
  locale/bitcoin_hi_IN.ts locale/bitcoin_hr.ts locale/bitcoin_hu.ts \
  locale/bitcoin_it.ts locale/bitcoin_ja.ts locale/bitcoin_la.ts \
  locale/bitcoin_lt.ts locale/bitcoin_lv_LV.ts locale/bitcoin_nb.ts \
  locale/bitcoin_nl.ts locale/bitcoin_pl.ts locale/bitcoin_pt_BR.ts \
  locale/bitcoin_pt_PT.ts locale/bitcoin_ro_RO.ts locale/bitcoin_ru.ts \
  locale/bitcoin_sk.ts locale/bitcoin_sr.ts locale/bitcoin_sv.ts \
  locale/bitcoin_th_TH.ts locale/bitcoin_tr.ts locale/bitcoin_uk.ts \
  locale/bitcoin_zh_CN.ts locale/bitcoin_zh_TW.ts

QT_QM = locale/bitcoin_af_ZA.qm locale/bitcoin_ar.qm locale/bitcoin_bg.qm \
  locale/bitcoin_bs.qm locale/bitcoin_ca_ES.qm locale/bitcoin_ca.qm \
  locale/bitcoin_cs.qm locale/bitcoin_cy.qm locale/bitcoin_da.qm \
  locale/bitcoin_de.qm locale/bitcoin_el_GR.qm locale/bitcoin_en.qm \
  locale/bitcoin_eo.qm locale/bitcoin_es_CL.qm locale/bitcoin_es.qm \
  locale/bitcoin_et.qm locale/bitcoin_eu_ES.qm locale/bitcoin_fa_IR.qm \
  locale/bitcoin_fa.qm locale/bitcoin_fi.qm locale/bitcoin_fr_CA.qm \
  locale/bitcoin_fr.qm locale/bitcoin_gu_IN.qm locale/bitcoin_he.qm \
  locale/bitcoin_hi_IN.qm locale/bitcoin_hr.qm locale/bitcoin_hu.qm \
  locale/bitcoin_it.qm locale/bitcoin_ja.qm locale/bitcoin_la.qm \
  locale/bitcoin_lt.qm locale/bitcoin_lv_LV.qm locale/bitcoin_nb.qm \
  locale/bitcoin_nl.qm locale/bitcoin_pl.qm locale/bitcoin_pt_BR.qm \
  locale/bitcoin_pt_PT.qm locale/bitcoin_ro_RO.qm locale/bitcoin_ru.qm \
  locale/bitcoin_sk.qm locale/bitcoin_sr.qm locale/bitcoin_sv.qm \
  locale/bitcoin_th_TH.qm locale/bitcoin_tr.qm locale/bitcoin_uk.qm \
  locale/bitcoin_zh_CN.qm locale/bitcoin_zh_TW.qm

QT_FORMS_UI = forms/aboutdialog.ui forms/addressbookpage.ui \
  forms/askpassphrasedialog.ui forms/editaddressdialog.ui \
  forms/optionsdialog.ui forms/overviewpage.ui \
  forms/qrcodedialog.ui forms/rpcconsole.ui forms/sendcoinsdialog.ui \
  forms/sendcoinsentry.ui forms/signverifymessagedialog.ui \
  forms/transactiondescdialog.ui

QT_MOC_CPP = moc_aboutdialog.cpp moc_addressbookpage.cpp \
  moc_addresstablemodel.cpp moc_askpassphrasedialog.cpp \
  moc_bitcoinaddressvalidator.cpp moc_bitcoinamountfield.cpp \
  moc_bitcoingui.cpp moc_bitcoinunits.cpp moc_clientmodel.cpp \
  moc_csvmodelwriter.cpp moc_editaddressdialog.cpp moc_guiutil.cpp \
  moc_monitoreddatamapper.cpp moc_notificator.cpp moc_optionsdialog.cpp \
  moc_optionsmodel.cpp moc_paymentserver.cpp moc_qrcodedialog.cpp \
  moc_qvalidatedlineedit.cpp moc_qvaluecombobox.cpp moc_sendcoinsdialog.cpp \
  moc_sendcoinsentry.cpp moc_signverifymessagedialog.cpp moc_splashscreen.cpp \
  moc_transactiondesc.cpp moc_transactiondescdialog.cpp \
  moc_transactionfilterproxy.cpp moc_transactiontablemodel.cpp \
  moc_transactionview.cpp moc_walletframe.cpp moc_walletmodel.cpp \
  moc_walletstack.cpp moc_walletview.cpp moc_rpcconsole.cpp \
  moc_overviewpage.cpp moc_macdockiconhandler.cpp \
  moc_macnotificationhandler.cpp

QT_MOC = overviewpage.moc rpcconsole.moc

QT_QRC_CPP = qrc_bitcoin.cpp
QT_QRC = bitcoin.qrc

QT_FORMS_H = forms/ui_aboutdialog.h forms/ui_askpassphrasedialog.h \
  forms/ui_optionsdialog.h forms/ui_sendcoinsentry.h \
  forms/ui_transactiondescdialog.h forms/ui_addressbookpage.h \
  forms/ui_editaddressdialog.h forms/ui_overviewpage.h \
  forms/ui_qrcodedialog.h forms/ui_rpcconsole.h forms/ui_sendcoinsdialog.h \
  forms/ui_signverifymessagedialog.h

BITCOIN_QT_H  = aboutdialog.h addressbookpage.h addresstablemodel.h \
  askpassphrasedialog.h bitcoinaddressvalidator.h bitcoinamountfield.h \
  bitcoingui.h bitcoinunits.h clientmodel.h csvmodelwriter.h \
  editaddressdialog.h guiconstants.h guiutil.h macdockiconhandler.h \
  macnotificationhandler.h monitoreddatamapper.h notificator.h optionsdialog.h \
  optionsmodel.h overviewpage.h paymentserver.h qrcodedialog.h \
  qvalidatedlineedit.h qvaluecombobox.h rpcconsole.h sendcoinsdialog.h \
  sendcoinsentry.h signverifymessagedialog.h splashscreen.h transactiondesc.h \
  transactiondescdialog.h transactionfilterproxy.h transactionrecord.h \
  transactiontablemodel.h transactionview.h walletframe.h walletmodel.h \
  walletstack.h walletview.h

libbitcoinqt_a_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms $(QT_DBUS_INCLUDES)
libbitcoinqt_a_SOURCES = aboutdialog.cpp addressbookpage.cpp \
  addresstablemodel.cpp askpassphrasedialog.cpp bitcoinaddressvalidator.cpp \
  bitcoinamountfield.cpp bitcoingui.cpp bitcoinstrings.cpp bitcoinunits.cpp \
  clientmodel.cpp csvmodelwriter.cpp editaddressdialog.cpp guiutil.cpp \
  monitoreddatamapper.cpp notificator.cpp optionsdialog.cpp optionsmodel.cpp \
  overviewpage.cpp paymentserver.cpp qrc_bitcoin.cpp qvalidatedlineedit.cpp \
  qvaluecombobox.cpp rpcconsole.cpp sendcoinsdialog.cpp sendcoinsentry.cpp \
  signverifymessagedialog.cpp splashscreen.cpp transactiondesc.cpp \
  transactiondescdialog.cpp transactionfilterproxy.cpp transactionrecord.cpp \
  transactiontablemodel.cpp transactionview.cpp walletframe.cpp \
  walletmodel.cpp walletstack.cpp walletview.cpp $(QT_FORMS_H) $(QT_QM) \
  $(QT_QRC_CPP) $(BITCOIN_QT_H) $(QT_FORMS_UI) $(QT_QRC) $(QT_TS)

nodist_libbitcoinqt_a_SOURCES = $(QT_MOC_CPP) $(QT_MOC)

BUILT_SOURCES = $(QT_FORMS_H) $(QT_MOC_CPP) $(QT_QRC_CPP) $(QT_QM) $(QT_MOC)

if TARGET_DARWIN
  libbitcoinqt_a_SOURCES += macdockiconhandler.mm macnotificationhandler.mm
endif
if USE_QRCODE
  libbitcoinqt_a_SOURCES += qrcodedialog.cpp
endif
#

# bitcoin-qt binary #
bitcoin_qt_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms
bitcoin_qt_SOURCES = bitcoin.cpp
bitcoin_qt_LDADD = libbitcoinqt.a $(LIBBITCOIN) $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS)
$(QT_QRC_CPP): $(QT_QRC) $(QT_QM)
	@test -f $(RCC) && $(RCC) -name bitcoin -o $@ $< || \
	  echo error: could not build $@

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = res

