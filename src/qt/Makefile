.PHONY: FORCE
all: FORCE
	$(MAKE) -C .. revolt_qt test_revolt_qt
clean: FORCE
	$(MAKE) -C .. revolt_qt_clean test_revolt_qt_clean
check: FORCE
	$(MAKE) -C .. test_revolt_qt_check
revolt-qt revolt-qt.exe: FORCE
	 $(MAKE) -C .. revolt_qt
apk: FORCE
	$(MAKE) -C .. revolt_qt_apk
