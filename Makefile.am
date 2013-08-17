ACLOCAL_AMFLAGS = -I src/m4
SUBDIRS = src
.PHONY: windows-deploy osx-deploy FORCE

BITCOIND_BIN=$(top_builddir)/src/bitcoind$(EXEEXT)
BITCOIN_QT_BIN=$(top_builddir)/src/qt/bitcoin-qt$(EXEEXT)

WINDOWS_PACKAGING =  $(top_srcdir)/share/setup.nsi.in \
  $(top_srcdir)/share/pixmaps/bitcoin.ico \
  $(top_srcdir)/share/pixmaps/nsis-header.bmp share/pixmaps/nsis-wizard.bmp \
  $(top_srcdir)/doc/README_windows.txt

dist-hook:
	-$(MAKE) -C $(top_distdir)/src/leveldb clean
	-$(GIT) archive --format=tar HEAD -- src/version.cpp | $(AMTAR) -C $(top_distdir) -xf -

distcheck-hook:
	$(MKDIR_P) $(top_distdir)/_build/src/leveldb
	cp -rf $(top_srcdir)/src/leveldb/ $(top_distdir)/_build/src/
	-$(MAKE) -C $(top_distdir)/_build/src/leveldb clean

distcleancheck:
	@:

$(PACKAGE)-$(PACKAGE_VERSION)-win32-setup.exe: $(BITCOIND_BIN) $(BITCOIN_QT_BIN)
	mkdir -p $(top_srcdir)/release
	cp -f $(top_builddir)/src/bitcoind$(EXEEXT) $(top_srcdir)/release
	cp -f $(top_builddir)/src/qt/bitcoin-qt$(EXEEXT) $(top_srcdir)/release
	$(STRIP) $(top_srcdir)/release/bitcoind$(EXEEXT)
	$(STRIP) $(top_srcdir)/release/bitcoin-qt$(EXEEXT)
	@test -f $(MAKENSIS) && $(MAKENSIS) $(top_srcdir)/share/setup.nsi || \
          echo error: could not build $@

$(BITCOIND_BIN):
	make -C $(top_srcdir)/src bitcoind$(EXEEXT)

$(BITCOIN_QT_BIN):
	make -C $(top_srcdir)/src/qt bitcoin-qt$(EXEEXT)

windows-deploy: $(PACKAGE)-$(PACKAGE_VERSION)-win32-setup.exe

EXTRA_DIST = $(top_srcdir)/share/genbuild.sh $(WINDOWS_PACKAGING)
