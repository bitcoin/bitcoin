include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS =  $(INCLUDES) -I$(top_builddir)/src/obj \
  -I$(top_srcdir)/src/leveldb/include -I$(top_srcdir)/src \
  -I$(top_srcdir)/src/leveldb/helpers -I$(top_srcdir)/src/qt \
  -I$(top_builddir)/src/qt $(BOOST_INCLUDES) $(PROTOBUF_CFLAGS) \
    $(QR_CFLAGS)
AM_LDFLAGS = $(PTHREAD_CFLAGS)
bin_PROGRAMS = test_ppcoin-qt
TESTS = test_ppcoin-qt

TEST_QT_MOC_CPP = moc_uritests.cpp 

TEST_QT_H = uritests.h 

BUILT_SOURCES = $(TEST_QT_MOC_CPP)

test_ppcoin_qt_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) $(QT_TEST_INCLUDES)
test_ppcoin_qt_SOURCES = test_main.cpp uritests.cpp $(TEST_QT_H)
nodist_test_ppcoin_qt_SOURCES = $(TEST_QT_MOC_CPP)
test_ppcoin_qt_LDADD = $(LIBPPCOINQT) $(LIBPPCOIN) $(LIBLEVELDB) \
  $(LIBMEMENV) $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS) $(QT_TEST_LIBS) \
  $(QR_LIBS) $(PROTOBUF_LIBS)

CLEANFILES = $(BUILT_SOURCES) *.gcda *.gcno

