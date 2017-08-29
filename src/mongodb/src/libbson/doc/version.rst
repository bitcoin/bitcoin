:man_page: bson_version

Libbson Versioning
==================

Versioning Macros and Functions

Macros
------

The following preprocessor macros can be used to perform various checks based on the version of the library you are compiling against. This may be useful if you only want to enable a feature on a certain version of the library.

Synopsis
--------

.. code-block:: c

  #define BSON_CHECK_VERSION(major, minor, micro)

  #define BSON_MAJOR_VERSION (1)
  #define BSON_MINOR_VERSION (4)
  #define BSON_MICRO_VERSION (1)
  #define BSON_VERSION_S "1.4.1"

  #define BSON_VERSION_HEX                                  \
     (BSON_MAJOR_VERSION << 24 | BSON_MINOR_VERSION << 16 | \
      BSON_MICRO_VERSION << 8)

Only compile a block on Libbson 1.1.0 and newer.

.. code-block:: c

  #if BSON_CHECK_VERSION(1, 1, 0)
  static void
  do_something (void)
  {
  }
  #endif

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_check_version
    bson_get_major_version
    bson_get_micro_version
    bson_get_minor_version
    bson_get_version

