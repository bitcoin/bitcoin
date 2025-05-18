.PHONY: FORCE
all: FORCE
	$(MAKE) -C .. tortoisecoin_qt test_tortoisecoin_qt
clean: FORCE
	$(MAKE) -C .. tortoisecoin_qt_clean test_tortoisecoin_qt_clean
check: FORCE
	$(MAKE) -C .. test_tortoisecoin_qt_check
tortoisecoin-qt tortoisecoin-qt.exe: FORCE
	 $(MAKE) -C .. tortoisecoin_qt
apk: FORCE
	$(MAKE) -C .. tortoisecoin_qt_apk
