:man_page: mongoc_version

Version Checks
==============

Conditional compilation based on mongoc version

Description
-----------

The following preprocessor macros can be used to perform various checks based on the version of the library you are compiling against.
This may be useful if you only want to enable a feature on a certain version of the library.

.. parsed-literal::

  #include <mongoc.h>

  #define MONGOC_MAJOR_VERSION (|release_major|)
  #define MONGOC_MINOR_VERSION (|release_minor|)
  #define MONGOC_MICRO_VERSION (|release_patch|)
  #define MONGOC_VERSION_S     "|release|"
  #define MONGOC_VERSION_HEX   ((1 << 24) | (0 << 16) | (0 << 8) | 0)
  #define MONGOC_CHECK_VERSION(major, minor, micro)

Only compile a block on MongoDB C Driver 1.1.0 and newer.

.. code-block:: c

  #if MONGOC_CHECK_VERSION(1, 1, 0)
  static void
  do_something (void)
  {
  }
  #endif

.. only:: html

  Run-Time Version Checks
  -----------------------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_check_version
    mongoc_get_major_version
    mongoc_get_micro_version
    mongoc_get_minor_version
    mongoc_get_version

