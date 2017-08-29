:man_page: bson_get_major_version

bson_get_major_version()
========================

Synopsis
--------

.. code-block:: c

  int
  bson_get_major_version (void);

Description
-----------

Get the first number in libbson's MAJOR.MINOR.MICRO release version.

Returns
-------

The value of ``BSON_MAJOR_VERSION`` when Libbson was compiled.

