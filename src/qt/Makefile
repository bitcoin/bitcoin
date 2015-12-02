.PHONY: FORCE
all: FORCE
	$(MAKE) -C .. syscoin_qt test_syscoin_qt
clean: FORCE
	$(MAKE) -C .. syscoin_qt_clean test_syscoin_qt_clean
check: FORCE
	$(MAKE) -C .. test_syscoin_qt_check
syscoin-qt syscoin-qt.exe: FORCE
	 $(MAKE) -C .. syscoin_qt
