ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS =  $(INCLUDES) -I$(top_srcdir)/obj -I$(top_srcdir)/leveldb/include \
               -I$(top_srcdir)/leveldb/helpers -I$(builddir)/qt $(BOOST_INCLUDES)
AM_LDFLAGS = $(PTHREAD_CFLAGS)
bin_PROGRAMS = bitcoind $(BUILD_TEST_PROGRAMS)
EXTRA_PROGRAMS = test_bitcoin test_bitcoin-qt
noinst_LIBRARIES = libbitcoind.a
TESTS = $(BUILD_TEST_PROGRAMS)

TEST_DATA_DIR=$(top_srcdir)/test/data

TEST_DATA_FILES= ./test/data/script_valid.json \
./test/data/base58_keys_valid.json ./test/data/sig_canonical.json \
./test/data/sig_noncanonical.json ./test/data/base58_encode_decode.json \
./test/data/alertTests ./test/data/base58_keys_invalid.json \
./test/data/script_invalid.json ./test/data/tx_invalid.json \
./test/data/tx_valid.json

# bitcoin core #
BITCOIN_CORE_H = addrman.h alert.h allocators.h base58.h bignum.h \
bitcoinrpc.h bloom.h checkpoints.h checkqueue.h clientversion.h compat.h \
crypter.h db.h hash.h init.h key.h keystore.h leveldb.h limitedmap.h main.h \
mruset.h netbase.h net.h protocol.h script.h serialize.h sync.h \
threadsafety.h txdb.h ui_interface.h uint256.h util.h version.h walletdb.h \
wallet.h json/json_spirit.h json/json_spirit_error_position.h \
json/json_spirit_reader.h json/json_spirit_reader_template.h \
json/json_spirit_stream_reader.h json/json_spirit_utils.h \
json/json_spirit_value.h json/json_spirit_writer.h \
json/json_spirit_writer_template.h

libbitcoind_a_SOURCES = alert.cpp version.cpp checkpoints.cpp netbase.cpp \
addrman.cpp crypter.cpp key.cpp db.cpp init.cpp keystore.cpp main.cpp net.cpp \
protocol.cpp bitcoinrpc.cpp rpcdump.cpp rpcnet.cpp rpcmining.cpp \
rpcwallet.cpp rpcblockchain.cpp rpcrawtransaction.cpp script.cpp sync.cpp \
util.cpp wallet.cpp walletdb.cpp hash.cpp bloom.cpp noui.cpp leveldb.cpp \
txdb.cpp $(BITCOIN_CORE_H)
nodist_libbitcoind_a_SOURCES = obj/build.h
#


# bitcoind binary #
bitcoind_LDADD = libbitcoind.a leveldb/libleveldb.a leveldb/libmemenv.a \
                 $(BOOST_LIBS)
bitcoind_SOURCES = bitcoind.cpp
#


# test_bitcoin binary #
test_bitcoin_CPPFLAGS = $(AM_CPPFLAGS) $(TESTDEFS) \
                        -DTEST_DATA_DIR=$(TEST_DATA_DIR)
test_bitcoin_LDADD = $(bitcoind_LDADD) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
test_bitcoin_SOURCES = test/accounting_tests.cpp test/alert_tests.cpp \
test/allocator_tests.cpp test/base32_tests.cpp test/base58_tests.cpp \
test/base64_tests.cpp test/bignum_tests.cpp test/bloom_tests.cpp \
test/canonical_tests.cpp test/checkblock_tests.cpp test/Checkpoints_tests.cpp \
test/compress_tests.cpp test/DoS_tests.cpp test/getarg_tests.cpp \
test/key_tests.cpp test/miner_tests.cpp test/mruset_tests.cpp \
test/multisig_tests.cpp test/netbase_tests.cpp test/pmt_tests.cpp \
test/rpc_tests.cpp test/script_P2SH_tests.cpp test/script_tests.cpp \
test/serialize_tests.cpp test/sigopcount_tests.cpp test/test_bitcoin.cpp \
test/transaction_tests.cpp test/uint160_tests.cpp test/uint256_tests.cpp \
test/util_tests.cpp test/wallet_tests.cpp $(TEST_DATA_FILES)

# bitcoin qt core #
QT_TS = qt/locale/bitcoin_af_ZA.ts qt/locale/bitcoin_ar.ts \
qt/locale/bitcoin_bg.ts qt/locale/bitcoin_bs.ts qt/locale/bitcoin_ca_ES.ts \
qt/locale/bitcoin_ca.ts qt/locale/bitcoin_cs.ts qt/locale/bitcoin_cy.ts \
qt/locale/bitcoin_da.ts qt/locale/bitcoin_de.ts qt/locale/bitcoin_el_GR.ts \
qt/locale/bitcoin_en.ts qt/locale/bitcoin_eo.ts qt/locale/bitcoin_es_CL.ts \
qt/locale/bitcoin_es.ts qt/locale/bitcoin_et.ts qt/locale/bitcoin_eu_ES.ts \
qt/locale/bitcoin_fa_IR.ts qt/locale/bitcoin_fa.ts qt/locale/bitcoin_fi.ts \
qt/locale/bitcoin_fr_CA.ts qt/locale/bitcoin_fr.ts qt/locale/bitcoin_gu_IN.ts \
qt/locale/bitcoin_he.ts qt/locale/bitcoin_hi_IN.ts qt/locale/bitcoin_hr.ts \
qt/locale/bitcoin_hu.ts qt/locale/bitcoin_it.ts qt/locale/bitcoin_ja.ts \
qt/locale/bitcoin_la.ts qt/locale/bitcoin_lt.ts qt/locale/bitcoin_lv_LV.ts \
qt/locale/bitcoin_nb.ts qt/locale/bitcoin_nl.ts qt/locale/bitcoin_pl.ts \
qt/locale/bitcoin_pt_BR.ts qt/locale/bitcoin_pt_PT.ts \
qt/locale/bitcoin_ro_RO.ts qt/locale/bitcoin_ru.ts qt/locale/bitcoin_sk.ts \
qt/locale/bitcoin_sr.ts qt/locale/bitcoin_sv.ts qt/locale/bitcoin_th_TH.ts \
qt/locale/bitcoin_tr.ts qt/locale/bitcoin_uk.ts qt/locale/bitcoin_zh_CN.ts \
qt/locale/bitcoin_zh_TW.ts

QT_QM = qt/locale/bitcoin_af_ZA.qm qt/locale/bitcoin_ar.qm \
qt/locale/bitcoin_bg.qm qt/locale/bitcoin_bs.qm qt/locale/bitcoin_ca_ES.qm \
qt/locale/bitcoin_ca.qm qt/locale/bitcoin_cs.qm qt/locale/bitcoin_cy.qm \
qt/locale/bitcoin_da.qm qt/locale/bitcoin_de.qm qt/locale/bitcoin_el_GR.qm \
qt/locale/bitcoin_en.qm qt/locale/bitcoin_eo.qm qt/locale/bitcoin_es_CL.qm \
qt/locale/bitcoin_es.qm qt/locale/bitcoin_et.qm qt/locale/bitcoin_eu_ES.qm \
qt/locale/bitcoin_fa_IR.qm qt/locale/bitcoin_fa.qm qt/locale/bitcoin_fi.qm \
qt/locale/bitcoin_fr_CA.qm qt/locale/bitcoin_fr.qm qt/locale/bitcoin_gu_IN.qm \
qt/locale/bitcoin_he.qm qt/locale/bitcoin_hi_IN.qm qt/locale/bitcoin_hr.qm \
qt/locale/bitcoin_hu.qm qt/locale/bitcoin_it.qm qt/locale/bitcoin_ja.qm \
qt/locale/bitcoin_la.qm qt/locale/bitcoin_lt.qm qt/locale/bitcoin_lv_LV.qm \
qt/locale/bitcoin_nb.qm qt/locale/bitcoin_nl.qm qt/locale/bitcoin_pl.qm \
qt/locale/bitcoin_pt_BR.qm qt/locale/bitcoin_pt_PT.qm \
qt/locale/bitcoin_ro_RO.qm qt/locale/bitcoin_ru.qm qt/locale/bitcoin_sk.qm \
qt/locale/bitcoin_sr.qm qt/locale/bitcoin_sv.qm qt/locale/bitcoin_th_TH.qm \
qt/locale/bitcoin_tr.qm qt/locale/bitcoin_uk.qm qt/locale/bitcoin_zh_CN.qm \
qt/locale/bitcoin_zh_TW.qm

QT_FORMS_UI = qt/forms/aboutdialog.ui qt/forms/addressbookpage.ui \
qt/forms/askpassphrasedialog.ui qt/forms/editaddressdialog.ui \
qt/forms/optionsdialog.ui qt/forms/overviewpage.ui qt/forms/qrcodedialog.ui \
qt/forms/rpcconsole.ui qt/forms/sendcoinsdialog.ui qt/forms/sendcoinsentry.ui \
qt/forms/signverifymessagedialog.ui qt/forms/transactiondescdialog.ui

QT_MOC_CPP = qt/moc_aboutdialog.cpp qt/moc_addressbookpage.cpp \
qt/moc_addresstablemodel.cpp qt/moc_askpassphrasedialog.cpp \
qt/moc_bitcoinaddressvalidator.cpp qt/moc_bitcoinamountfield.cpp \
qt/moc_bitcoingui.cpp qt/moc_bitcoinunits.cpp qt/moc_clientmodel.cpp \
qt/moc_csvmodelwriter.cpp qt/moc_editaddressdialog.cpp qt/moc_guiutil.cpp \
qt/moc_monitoreddatamapper.cpp qt/moc_notificator.cpp \
qt/moc_optionsdialog.cpp qt/moc_optionsmodel.cpp qt/moc_paymentserver.cpp \
qt/moc_qrcodedialog.cpp qt/moc_qvalidatedlineedit.cpp \
qt/moc_qvaluecombobox.cpp qt/moc_sendcoinsdialog.cpp \
qt/moc_sendcoinsentry.cpp qt/moc_signverifymessagedialog.cpp \
qt/moc_splashscreen.cpp qt/moc_transactiondesc.cpp \
qt/moc_transactiondescdialog.cpp qt/moc_transactionfilterproxy.cpp \
qt/moc_transactiontablemodel.cpp qt/moc_transactionview.cpp \
qt/moc_walletframe.cpp qt/moc_walletmodel.cpp qt/moc_walletstack.cpp \
qt/moc_walletview.cpp qt/moc_rpcconsole.cpp qt/moc_overviewpage.cpp \
qt/moc_macdockiconhandler.cpp qt/moc_macnotificationhandler.cpp

TEST_QT_MOC_CPP = qt/test/moc_uritests.cpp

QT_MOC = qt/overviewpage.moc qt/rpcconsole.moc

QT_QRC_CPP = qt/qrc_bitcoin.cpp
QT_QRC = qt/bitcoin.qrc

QT_FORMS_H = qt/forms/ui_aboutdialog.h qt/forms/ui_askpassphrasedialog.h \
qt/forms/ui_optionsdialog.h qt/forms/ui_sendcoinsentry.h \
qt/forms/ui_transactiondescdialog.h qt/forms/ui_addressbookpage.h \
qt/forms/ui_editaddressdialog.h qt/forms/ui_overviewpage.h \
qt/forms/ui_qrcodedialog.h qt/forms/ui_rpcconsole.h \
qt/forms/ui_sendcoinsdialog.h qt/forms/ui_signverifymessagedialog.h

BITCOIN_QT_H  = qt/aboutdialog.h qt/addressbookpage.h qt/addresstablemodel.h \
qt/askpassphrasedialog.h qt/bitcoinaddressvalidator.h qt/bitcoinamountfield.h \
qt/bitcoingui.h qt/bitcoinunits.h qt/clientmodel.h qt/csvmodelwriter.h \
qt/editaddressdialog.h qt/guiconstants.h qt/guiutil.h qt/macdockiconhandler.h \
qt/macnotificationhandler.h qt/monitoreddatamapper.h qt/notificator.h \
qt/optionsdialog.h qt/optionsmodel.h qt/overviewpage.h qt/paymentserver.h \
qt/qrcodedialog.h qt/qvalidatedlineedit.h qt/qvaluecombobox.h qt/rpcconsole.h \
qt/sendcoinsdialog.h qt/sendcoinsentry.h qt/signverifymessagedialog.h \
qt/splashscreen.h qt/transactiondesc.h qt/transactiondescdialog.h \
qt/transactionfilterproxy.h qt/transactionrecord.h qt/transactiontablemodel.h \
qt/transactionview.h qt/walletframe.h qt/walletmodel.h qt/walletstack.h \
qt/walletview.h qt/test/uritests.h

libbitcoinqt_a_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) -I$(top_srcdir)/qt \
                          -I$(top_srcdir)/qt/forms $(QT_DBUS_INCLUDES)
libbitcoinqt_a_SOURCES =  qt/aboutdialog.cpp qt/addressbookpage.cpp \
qt/addresstablemodel.cpp qt/askpassphrasedialog.cpp \
qt/bitcoinaddressvalidator.cpp qt/bitcoinamountfield.cpp qt/bitcoingui.cpp \
qt/bitcoinstrings.cpp qt/bitcoinunits.cpp qt/clientmodel.cpp \
qt/csvmodelwriter.cpp qt/editaddressdialog.cpp qt/guiutil.cpp \
qt/monitoreddatamapper.cpp qt/notificator.cpp qt/optionsdialog.cpp \
qt/optionsmodel.cpp qt/overviewpage.cpp qt/paymentserver.cpp \
qt/qrc_bitcoin.cpp qt/qvalidatedlineedit.cpp qt/qvaluecombobox.cpp \
qt/rpcconsole.cpp qt/sendcoinsdialog.cpp qt/sendcoinsentry.cpp \
qt/signverifymessagedialog.cpp qt/splashscreen.cpp qt/transactiondesc.cpp \
qt/transactiondescdialog.cpp qt/transactionfilterproxy.cpp \
qt/transactionrecord.cpp qt/transactiontablemodel.cpp qt/transactionview.cpp \
qt/walletframe.cpp qt/walletmodel.cpp qt/walletstack.cpp qt/walletview.cpp \
$(QT_FORMS_H) $(QT_QM) $(QT_QRC_CPP) $(BITCOIN_QT_H) $(QT_FORMS_UI) $(QT_QRC) \
$(QT_TS)

nodist_libbitcoinqt_a_SOURCES = $(QT_MOC_CPP) $(QT_MOC)
if BUILD_QT
noinst_LIBRARIES += libbitcoinqt.a
BUILT_SOURCES = $(QT_FORMS_H) $(QT_MOC_CPP) $(QT_QRC_CPP) $(QT_QM) $(QT_MOC) \
                $(TEST_QT_MOC_CPP)
if TARGET_DARWIN
libbitcoinqt_a_SOURCES += qt/macdockiconhandler.mm qt/macnotificationhandler.mm
endif
if USE_QRCODE
libbitcoinqt_a_SOURCES += qt/qrcodedialog.cpp
endif
endif
#


# bitcoin-qt binary #
bitcoin_qt_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) -I$(top_srcdir)/qt \
                      -I$(top_srcdir)/qt/forms
bitcoin_qt_SOURCES = qt/bitcoin.cpp
bitcoin_qt_LDADD = libbitcoinqt.a $(bitcoind_LDADD) $(QT_LIBS) $(QT_DBUS_LIBS)

if BUILD_QT
bin_PROGRAMS += bitcoin-qt
endif
#

# test_bitcoin-qt binary #
test_bitcoin_qt_CPPFLAGS = $(bitcoin_qt_CPPFLAGS) $(QT_TEST_INCLUDES)
test_bitcoin_qt_SOURCES = qt/test/test_main.cpp qt/test/uritests.cpp
nodist_test_bitcoin_qt_SOURCES = $(TEST_QT_MOC_CPP)
test_bitcoin_qt_LDADD = $(bitcoin_qt_LDADD) $(QT_TEST_LIBS)

.mm.o:
	$(OBJC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(CXXFLAGS)  -c -o $@ $<

ui_%.h: %.ui
	@test -f $(UIC) && $(UIC) -o $@ $< || echo error: could not build $@

$(QT_QRC_CPP): $(QT_QRC) $(QT_QM)
	@test -f $(RCC) && $(RCC) -name bitcoin -o $@ $< || \
        echo error: could not build $@

%.moc: %.cpp
	$(MOC) $(QT_INCLUDES) $(MOC_DEFS) -o $@ $<

moc_%.cpp: %.h
	$(MOC) $(QT_INCLUDES) $(MOC_DEFS) -o $@ $<

%.qm: %.ts
	@test -f $(LRELEASE) && $(LRELEASE) $< $@ || \
        echo error: could not build $@

leveldb/libleveldb.a:
	@echo "Building LevelDB ..." && cd leveldb && CXX=$(CXX) CC=$(CC) \
        AR=$(AR) $(MAKE) $(LEVELDB_TARGET_FLAGS) OPT="$(CXXFLAGS)" libleveldb.a

leveldb/libmemenv.a:
	@echo "Building LevelDB ..." && cd leveldb && CXX=$(CXX) CC=$(CC) \
        AR=$(AR) $(MAKE) $(LEVELDB_TARGET_FLAGS) OPT="$(CXXFLAGS)" libmemenv.a

CLEANFILES = $(BUILT_SOURCES) leveldb/libleveldb.a leveldb/libmemenv.a \
             $(QT_FORMS_H) $(QT_MOC_CPP) $(QT_QRC_CPP) $(QT_QM) $(QT_MOC)

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb qt/res

clean-local:
	-$(MAKE) -C leveldb clean

dist-hook:
	-$(MAKE) -C $(top_distdir)/leveldb clean

distcheck-hook:
	cp -rf $(top_srcdir)/leveldb $(top_distdir)/_build
	-$(MAKE) -C $(top_distdir)/_build/leveldb clean

distcleancheck:
	@:
