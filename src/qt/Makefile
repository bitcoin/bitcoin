.PHONY: FORCE
all: FORCE
	$(MAKE) -C .. navcoin_qt test_navcoin_qt
clean: FORCE
	$(MAKE) -C .. navcoin_qt_clean test_navcoin_qt_clean
check: FORCE
	$(MAKE) -C .. test_navcoin_qt_check
navcoin-qt navcoin-qt.exe: FORCE
	 $(MAKE) -C .. navcoin_qt
apk: FORCE
	$(MAKE) -C .. navcoin_qt_apk
