include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src \
  -I$(top_srcdir)/src/qt \
  -I$(top_builddir)/src/qt \
  $(PROTOBUF_CFLAGS) \
    $(QR_CFLAGS)
bin_PROGRAMS = test_bitcoin-core-gui
TESTS = test_bitcoin-core-gui

TEST_QT_MOC_CPP = moc_uritests.cpp moc_paymentservertests.cpp

TEST_QT_H = uritests.h paymentservertests.h paymentrequestdata.h

BUILT_SOURCES = $(TEST_QT_MOC_CPP)

test_bitcoin_core_gui_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) $(QT_TEST_INCLUDES)
test_bitcoin_core_gui_SOURCES = test_main.cpp uritests.cpp paymentservertests.cpp $(TEST_QT_H)
nodist_test_bitcoin_core_gui_SOURCES = $(TEST_QT_MOC_CPP)
test_bitcoin_core_gui_LDADD = $(LIBBITCOINCORE_QT) $(LIBBITCOINCORE_SERVER) $(LIBBITCOINCORE_WALLET) $(LIBBITCOINCORE_CLI) $(LIBBITCOINCORE_COMMON) $(LIBLEVELDB) \
  $(LIBMEMENV) $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS) $(QT_TEST_LIBS) \
  $(QR_LIBS) $(PROTOBUF_LIBS) $(BDB_LIBS)

CLEANFILES = $(BUILT_SOURCES) *.gcda *.gcno

