.PHONY: FORCE
all: FORCE
	$(MAKE) -C .. bitcoin_qt test_bitcoin_qt
clean: FORCE
	$(MAKE) -C .. bitcoin_qt_clean test_bitcoin_qt_clean
check: FORCE
	$(MAKE) -C .. test_bitcoin_qt_check
bitcoin-qt bitcoin-qt.exe: FORCE
	 $(MAKE) -C .. bitcoin_qt
