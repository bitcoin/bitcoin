# Copyright (c) 2013-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%: FORCE
	@echo '$*'='$($*)'

ACLOCAL_AMFLAGS = -I build-aux/m4
SUBDIRS = src
if ENABLE_MAN
SUBDIRS += doc/man
endif
.PHONY: deploy FORCE
.INTERMEDIATE: $(COVERAGE_INFO)

BITCOIND_BIN=$(top_builddir)/src/$(BITCOIN_DAEMON_NAME)$(EXEEXT)
BITCOIN_QT_BIN=$(top_builddir)/src/qt/$(BITCOIN_GUI_NAME)$(EXEEXT)
BITCOIN_TEST_BIN=$(top_builddir)/src/test/$(BITCOIN_TEST_NAME)$(EXEEXT)
BITCOIN_CLI_BIN=$(top_builddir)/src/$(BITCOIN_CLI_NAME)$(EXEEXT)
BITCOIN_TX_BIN=$(top_builddir)/src/$(BITCOIN_TX_NAME)$(EXEEXT)
BITCOIN_UTIL_BIN=$(top_builddir)/src/$(BITCOIN_UTIL_NAME)$(EXEEXT)
BITCOIN_WALLET_BIN=$(top_builddir)/src/$(BITCOIN_WALLET_TOOL_NAME)$(EXEEXT)
BITCOIN_NODE_BIN=$(top_builddir)/src/$(BITCOIN_MP_NODE_NAME)$(EXEEXT)
BITCOIN_GUI_BIN=$(top_builddir)/src/$(BITCOIN_MP_GUI_NAME)$(EXEEXT)
BITCOIN_WIN_INSTALLER=$(PACKAGE)-$(PACKAGE_VERSION)-win64-setup$(EXEEXT)

empty :=
space := $(empty) $(empty)

OSX_APP=Bitcoin-Qt.app
OSX_VOLNAME = $(subst $(space),-,$(PACKAGE_NAME))
OSX_ZIP = $(OSX_VOLNAME).zip
OSX_DEPLOY_SCRIPT=$(top_srcdir)/contrib/macdeploy/macdeployqtplus
OSX_INSTALLER_ICONS=$(top_srcdir)/src/qt/res/icons/bitcoin.icns
OSX_PLIST=$(top_builddir)/share/qt/Info.plist #not installed

DIST_CONTRIB = \
	       $(top_srcdir)/test/sanitizer_suppressions/lsan \
	       $(top_srcdir)/test/sanitizer_suppressions/tsan \
	       $(top_srcdir)/test/sanitizer_suppressions/ubsan \
	       $(top_srcdir)/contrib/linearize/linearize-data.py \
	       $(top_srcdir)/contrib/linearize/linearize-hashes.py \
	       $(top_srcdir)/contrib/signet/miner

DIST_SHARE = \
  $(top_srcdir)/share/genbuild.sh \
  $(top_srcdir)/share/rpcauth

BIN_CHECKS=$(top_srcdir)/contrib/devtools/symbol-check.py \
           $(top_srcdir)/contrib/devtools/security-check.py \
           $(top_srcdir)/contrib/devtools/utils.py

WINDOWS_PACKAGING = $(top_srcdir)/share/pixmaps/bitcoin.ico \
  $(top_srcdir)/share/pixmaps/nsis-header.bmp \
  $(top_srcdir)/share/pixmaps/nsis-wizard.bmp \
  $(top_srcdir)/doc/README_windows.txt

OSX_PACKAGING = $(OSX_DEPLOY_SCRIPT) $(OSX_INSTALLER_ICONS) \
  $(top_srcdir)/contrib/macdeploy/detached-sig-create.sh

COVERAGE_INFO = $(COV_TOOL_WRAPPER) baseline.info \
  test_bitcoin_filtered.info total_coverage.info \
  baseline_filtered.info functional_test.info functional_test_filtered.info \
  test_bitcoin_coverage.info test_bitcoin.info fuzz.info fuzz_filtered.info fuzz_coverage.info

dist-hook:
	-$(GIT) archive --format=tar HEAD -- src/clientversion.cpp | $(AMTAR) -C $(top_distdir) -xf -

if TARGET_WINDOWS
$(BITCOIN_WIN_INSTALLER): all-recursive
	$(MKDIR_P) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIND_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_QT_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_TEST_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_CLI_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_TX_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_WALLET_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_UTIL_BIN) $(top_builddir)/release
	@test -f $(MAKENSIS) && echo 'OutFile "$@"' | cat $(top_builddir)/share/setup.nsi - | $(MAKENSIS) -V2 - || \
	  echo error: could not build $@
	@echo built $@

deploy: $(BITCOIN_WIN_INSTALLER)
endif

if TARGET_DARWIN
$(OSX_APP)/Contents/PkgInfo:
	$(MKDIR_P) $(@D)
	@echo "APPL????" > $@

$(OSX_APP)/Contents/Resources/empty.lproj:
	$(MKDIR_P) $(@D)
	@touch $@

$(OSX_APP)/Contents/Info.plist: $(OSX_PLIST)
	$(MKDIR_P) $(@D)
	$(INSTALL_DATA) $< $@

$(OSX_APP)/Contents/Resources/bitcoin.icns: $(OSX_INSTALLER_ICONS)
	$(MKDIR_P) $(@D)
	$(INSTALL_DATA) $< $@

$(OSX_APP)/Contents/MacOS/Bitcoin-Qt: all-recursive
	$(MKDIR_P) $(@D)
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM)  $(BITCOIN_QT_BIN) $@

$(OSX_APP)/Contents/Resources/Base.lproj/InfoPlist.strings:
	$(MKDIR_P) $(@D)
	echo '{	CFBundleDisplayName = "$(PACKAGE_NAME)"; CFBundleName = "$(PACKAGE_NAME)"; }' > $@

OSX_APP_BUILT=$(OSX_APP)/Contents/PkgInfo $(OSX_APP)/Contents/Resources/empty.lproj \
  $(OSX_APP)/Contents/Resources/bitcoin.icns $(OSX_APP)/Contents/Info.plist \
  $(OSX_APP)/Contents/MacOS/Bitcoin-Qt $(OSX_APP)/Contents/Resources/Base.lproj/InfoPlist.strings

if BUILD_DARWIN
$(OSX_ZIP): $(OSX_APP_BUILT) $(OSX_PACKAGING)
	$(PYTHON) $(OSX_DEPLOY_SCRIPT) $(OSX_APP) $(OSX_VOLNAME) -translations-dir=$(QT_TRANSLATION_DIR) -zip

deploydir: $(OSX_ZIP)
else !BUILD_DARWIN
APP_DIST_DIR=$(top_builddir)/dist

$(OSX_ZIP): deploydir
	if [ -n "$(SOURCE_DATE_EPOCH)" ]; then find $(APP_DIST_DIR) -exec touch -d @$(SOURCE_DATE_EPOCH) {} +; fi
	cd $(APP_DIST_DIR) && find . | sort | $(ZIP) -X@ $@

$(APP_DIST_DIR)/$(OSX_APP)/Contents/MacOS/Bitcoin-Qt: $(OSX_APP_BUILT) $(OSX_PACKAGING)
	OBJDUMP=$(OBJDUMP) $(PYTHON) $(OSX_DEPLOY_SCRIPT) $(OSX_APP) $(OSX_VOLNAME) -translations-dir=$(QT_TRANSLATION_DIR)

deploydir: $(APP_DIST_DIR)/$(OSX_APP)/Contents/MacOS/Bitcoin-Qt
endif !BUILD_DARWIN

deploy: $(OSX_ZIP)
endif

$(BITCOIN_QT_BIN): FORCE
	$(MAKE) -C src qt/$(@F)

$(BITCOIND_BIN): FORCE
	$(MAKE) -C src $(@F)

$(BITCOIN_CLI_BIN): FORCE
	$(MAKE) -C src $(@F)

$(BITCOIN_TX_BIN): FORCE
	$(MAKE) -C src $(@F)

$(BITCOIN_UTIL_BIN): FORCE
	$(MAKE) -C src $(@F)

$(BITCOIN_WALLET_BIN): FORCE
	$(MAKE) -C src $(@F)

$(BITCOIN_NODE_BIN): FORCE
	$(MAKE) -C src $(@F)

$(BITCOIN_GUI_BIN): FORCE
	$(MAKE) -C src $(@F)

if USE_LCOV
LCOV_FILTER_PATTERN = \
	-p "/usr/local/" \
	-p "/usr/include/" \
	-p "/usr/lib/" \
	-p "/usr/lib64/" \
	-p "src/leveldb/" \
	-p "src/crc32c/" \
	-p "src/bench/" \
	-p "src/crypto/ctaes" \
	-p "src/minisketch" \
	-p "src/secp256k1" \
	-p "depends"

DIR_FUZZ_SEED_CORPUS ?= qa-assets/fuzz_seed_corpus

$(COV_TOOL_WRAPPER):
	@echo 'exec $(COV_TOOL) "$$@"' > $(COV_TOOL_WRAPPER)
	@chmod +x $(COV_TOOL_WRAPPER)

baseline.info: $(COV_TOOL_WRAPPER)
	$(LCOV) $(LCOV_OPTS) -c -i -d $(abs_builddir)/src -o $@

baseline_filtered.info: baseline.info
	$(abs_builddir)/contrib/filter-lcov.py $(LCOV_FILTER_PATTERN) $< $@
	$(LCOV) -a $@ $(LCOV_OPTS) -o $@

fuzz.info: baseline_filtered.info
	@test/fuzz/test_runner.py $(DIR_FUZZ_SEED_CORPUS) -l DEBUG
	$(LCOV) -c $(LCOV_OPTS) -d $(abs_builddir)/src --t fuzz-tests -o $@
	$(LCOV) -z $(LCOV_OPTS) -d $(abs_builddir)/src

fuzz_filtered.info: fuzz.info
	$(abs_builddir)/contrib/filter-lcov.py $(LCOV_FILTER_PATTERN) $< $@
	$(LCOV) -a $@ $(LCOV_OPTS) -o $@

test_bitcoin.info: baseline_filtered.info
	$(MAKE) -C src/ check
	$(LCOV) -c $(LCOV_OPTS) -d $(abs_builddir)/src -t test_bitcoin -o $@
	$(LCOV) -z $(LCOV_OPTS) -d $(abs_builddir)/src

test_bitcoin_filtered.info: test_bitcoin.info
	$(abs_builddir)/contrib/filter-lcov.py $(LCOV_FILTER_PATTERN) $< $@
	$(LCOV) -a $@ $(LCOV_OPTS) -o $@

functional_test.info: test_bitcoin_filtered.info
	@test/functional/test_runner.py $(EXTENDED_FUNCTIONAL_TESTS)
	$(LCOV) -c $(LCOV_OPTS) -d $(abs_builddir)/src --t functional-tests -o $@
	$(LCOV) -z $(LCOV_OPTS) -d $(abs_builddir)/src

functional_test_filtered.info: functional_test.info
	$(abs_builddir)/contrib/filter-lcov.py $(LCOV_FILTER_PATTERN) $< $@
	$(LCOV) -a $@ $(LCOV_OPTS) -o $@

fuzz_coverage.info: fuzz_filtered.info
	$(LCOV) $(LCOV_OPTS) -a baseline_filtered.info -a fuzz_filtered.info -o $@ | $(GREP) "\%" | $(AWK) '{ print substr($$3,2,50) "/" $$5 }' > coverage_percent.txt

test_bitcoin_coverage.info: baseline_filtered.info test_bitcoin_filtered.info
	$(LCOV) $(LCOV_OPTS) -a baseline_filtered.info -a test_bitcoin_filtered.info -o $@

total_coverage.info: test_bitcoin_filtered.info functional_test_filtered.info
	$(LCOV) $(LCOV_OPTS) -a baseline_filtered.info -a test_bitcoin_filtered.info -a functional_test_filtered.info -o $@ | $(GREP) "\%" | $(AWK) '{ print substr($$3,2,50) "/" $$5 }' > coverage_percent.txt

fuzz.coverage/.dirstamp: fuzz_coverage.info
	$(GENHTML) -s $(LCOV_OPTS) $< -o $(@D)
	@touch $@

test_bitcoin.coverage/.dirstamp:  test_bitcoin_coverage.info
	$(GENHTML) -s $(LCOV_OPTS) $< -o $(@D)
	@touch $@

total.coverage/.dirstamp: total_coverage.info
	$(GENHTML) -s $(LCOV_OPTS) $< -o $(@D)
	@touch $@

cov_fuzz: fuzz.coverage/.dirstamp

cov: test_bitcoin.coverage/.dirstamp total.coverage/.dirstamp

endif

dist_noinst_SCRIPTS = autogen.sh

EXTRA_DIST = $(DIST_SHARE) $(DIST_CONTRIB) $(WINDOWS_PACKAGING) $(OSX_PACKAGING) $(BIN_CHECKS)

EXTRA_DIST += \
    test/functional \
    test/fuzz

EXTRA_DIST += \
    test/util/test_runner.py \
    test/util/data/bitcoin-util-test.json \
    test/util/data/blanktxv1.hex \
    test/util/data/blanktxv1.json \
    test/util/data/blanktxv2.hex \
    test/util/data/blanktxv2.json \
    test/util/data/tt-delin1-out.hex \
    test/util/data/tt-delin1-out.json \
    test/util/data/tt-delout1-out.hex \
    test/util/data/tt-delout1-out.json \
    test/util/data/tt-locktime317000-out.hex \
    test/util/data/tt-locktime317000-out.json \
    test/util/data/tx394b54bb.hex \
    test/util/data/txcreate1.hex \
    test/util/data/txcreate1.json \
    test/util/data/txcreate2.hex \
    test/util/data/txcreate2.json \
    test/util/data/txcreatedata1.hex \
    test/util/data/txcreatedata1.json \
    test/util/data/txcreatedata2.hex \
    test/util/data/txcreatedata2.json \
    test/util/data/txcreatedata_seq0.hex \
    test/util/data/txcreatedata_seq0.json \
    test/util/data/txcreatedata_seq1.hex \
    test/util/data/txcreatedata_seq1.json \
    test/util/data/txcreatemultisig1.hex \
    test/util/data/txcreatemultisig1.json \
    test/util/data/txcreatemultisig2.hex \
    test/util/data/txcreatemultisig2.json \
    test/util/data/txcreatemultisig3.hex \
    test/util/data/txcreatemultisig3.json \
    test/util/data/txcreatemultisig4.hex \
    test/util/data/txcreatemultisig4.json \
    test/util/data/txcreatemultisig5.json \
    test/util/data/txcreateoutpubkey1.hex \
    test/util/data/txcreateoutpubkey1.json \
    test/util/data/txcreateoutpubkey2.hex \
    test/util/data/txcreateoutpubkey2.json \
    test/util/data/txcreateoutpubkey3.hex \
    test/util/data/txcreateoutpubkey3.json \
    test/util/data/txcreatescript1.hex \
    test/util/data/txcreatescript1.json \
    test/util/data/txcreatescript2.hex \
    test/util/data/txcreatescript2.json \
    test/util/data/txcreatescript3.hex \
    test/util/data/txcreatescript3.json \
    test/util/data/txcreatescript4.hex \
    test/util/data/txcreatescript4.json \
    test/util/data/txcreatescript5.hex \
    test/util/data/txcreatescript6.hex \
    test/util/data/txcreatesignsegwit1.hex \
    test/util/data/txcreatesignv1.hex \
    test/util/data/txcreatesignv1.json \
    test/util/data/txcreatesignv2.hex \
    test/util/data/txreplace1.hex \
    test/util/data/txreplacenoinputs.hex \
    test/util/data/txreplaceomittedn.hex \
    test/util/data/txreplacesingleinput.hex \
    test/util/rpcauth-test.py

CLEANFILES = $(OSX_ZIP) $(BITCOIN_WIN_INSTALLER)

DISTCHECK_CONFIGURE_FLAGS = --enable-man

doc/doxygen/.stamp: doc/Doxyfile FORCE
	$(MKDIR_P) $(@D)
	$(DOXYGEN) $^
	$(AM_V_at) touch $@

if HAVE_DOXYGEN
docs: doc/doxygen/.stamp
else
docs:
	@echo "error: doxygen not found"
endif

clean-docs:
	rm -rf doc/doxygen

clean-local: clean-docs
	rm -rf coverage_percent.txt test_bitcoin.coverage/ total.coverage/ fuzz.coverage/ test/tmp/ cache/ $(OSX_APP)
	rm -rf test/functional/__pycache__ test/functional/test_framework/__pycache__ test/cache share/rpcauth/__pycache__
	rm -rf dist/ test/lint/test_runner/target/ test/lint/__pycache__

test-security-check:
if TARGET_DARWIN
	$(AM_V_at) CC='$(CC)' CFLAGS='$(CFLAGS)' CPPFLAGS='$(CPPFLAGS)' LDFLAGS='$(LDFLAGS)' $(PYTHON) $(top_srcdir)/contrib/devtools/test-security-check.py TestSecurityChecks.test_MACHO
	$(AM_V_at) CC='$(CC)' CFLAGS='$(CFLAGS)' CPPFLAGS='$(CPPFLAGS)' LDFLAGS='$(LDFLAGS)' $(PYTHON) $(top_srcdir)/contrib/devtools/test-symbol-check.py TestSymbolChecks.test_MACHO
endif
if TARGET_WINDOWS
	$(AM_V_at) CC='$(CC)' CFLAGS='$(CFLAGS)' CPPFLAGS='$(CPPFLAGS)' LDFLAGS='$(LDFLAGS)' $(PYTHON) $(top_srcdir)/contrib/devtools/test-security-check.py TestSecurityChecks.test_PE
	$(AM_V_at) CC='$(CC)' CFLAGS='$(CFLAGS)' CPPFLAGS='$(CPPFLAGS)' LDFLAGS='$(LDFLAGS)' $(PYTHON) $(top_srcdir)/contrib/devtools/test-symbol-check.py TestSymbolChecks.test_PE
endif
if TARGET_LINUX
	$(AM_V_at) CC='$(CC)' CFLAGS='$(CFLAGS)' CPPFLAGS='$(CPPFLAGS)' LDFLAGS='$(LDFLAGS)' $(PYTHON) $(top_srcdir)/contrib/devtools/test-security-check.py TestSecurityChecks.test_ELF
	$(AM_V_at) CC='$(CC)' CFLAGS='$(CFLAGS)' CPPFLAGS='$(CPPFLAGS)' LDFLAGS='$(LDFLAGS)' $(PYTHON) $(top_srcdir)/contrib/devtools/test-symbol-check.py TestSymbolChecks.test_ELF
endif
