:man_page: bson_get_micro_version

bson_get_micro_version()
========================

Synopsis
--------

.. code-block:: c

  int
  bson_get_micro_version (void);

Description
-----------

Get the third number in libbson's MAJOR.MINOR.MICRO release version.

Returns
-------

The value of ``BSON_MICRO_VERSION`` when Libbson was compiled.

