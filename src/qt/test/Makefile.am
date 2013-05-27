include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS =  $(INCLUDES) -I$(top_builddir)/src/obj \
  -I$(top_srcdir)/src/leveldb/include -I$(top_srcdir)/src \
  -I$(top_srcdir)/src/leveldb/helpers -I$(top_srcdir)/src/qt \
  -I$(top_builddir)/src/qt $(BOOST_INCLUDES) $(PROTOBUF_CFLAGS) \
    $(QR_CFLAGS)
AM_LDFLAGS = $(PTHREAD_CFLAGS)
bin_PROGRAMS = test_bitcoin-qt
TESTS = test_bitcoin-qt

TEST_QT_MOC_CPP = moc_uritests.cpp moc_paymentservertests.cpp

TEST_QT_H = uritests.h paymentservertests.h paymentrequestdata.h

BUILT_SOURCES = $(TEST_QT_MOC_CPP)

test_bitcoin_qt_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) $(QT_TEST_INCLUDES)
test_bitcoin_qt_SOURCES = test_main.cpp uritests.cpp paymentservertests.cpp $(TEST_QT_H)
nodist_test_bitcoin_qt_SOURCES = $(TEST_QT_MOC_CPP)
test_bitcoin_qt_LDADD = $(LIBBITCOINQT) $(LIBBITCOIN) $(LIBLEVELDB) \
  $(LIBMEMENV) $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS) $(QT_TEST_LIBS) \
  $(QR_LIBS) $(PROTOBUF_LIBS)

CLEANFILES = $(BUILT_SOURCES) *.gcda *.gcno

