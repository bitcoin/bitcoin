:man_page: bson_get_minor_version

bson_get_minor_version()
========================

Synopsis
--------

.. code-block:: c

  int
  bson_get_minor_version (void);

Description
-----------

Get the middle number in libbson's MAJOR.MINOR.MICRO release version.

Returns
-------

The value of ``BSON_MINOR_VERSION`` when Libbson was compiled.

