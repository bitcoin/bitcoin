:man_page: bson_strncpy

bson_strncpy()
==============

Synopsis
--------

.. code-block:: c

  void
  bson_strncpy (char *dst, const char *src, size_t size);

Parameters
----------

* ``dst``: The destination buffer.
* ``src``: The src buffer.
* ``size``: The number of bytes to copy into dst, which must be at least that size.

Description
-----------

Copies up to ``size`` bytes from ``src`` into ``dst``. ``dst`` must be at least ``size`` bytes in size. A trailing ``\0`` will be set.

