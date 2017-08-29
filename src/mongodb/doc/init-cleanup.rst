:man_page: mongoc_init_cleanup

Initialization and cleanup
==========================

Synopsis
--------

.. include:: includes/init_cleanup.txt

.. only:: html

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_init
    mongoc_cleanup

Deprecated feature: automatic initialization and cleanup
--------------------------------------------------------

On some platforms the driver can automatically call :symbol:`mongoc_init` before ``main``, and call :symbol:`mongoc_cleanup` as the process exits. This is problematic in situations where related libraries also execute cleanup code on shutdown, and it creates inconsistent rules across platforms. Therefore the automatic initialization and cleanup feature is deprecated, and will be dropped in version 2.0. Meanwhile, for backward compatibility, the feature is *enabled* by default on platforms where it is available.

For portable, future-proof code, always call :symbol:`mongoc_init` and :symbol:`mongoc_cleanup` yourself, and configure the driver like:

.. code-block:: none

  ./configure --disable-automatic-init-and-cleanup

Or with CMake:

.. code-block:: none

  cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=NO
