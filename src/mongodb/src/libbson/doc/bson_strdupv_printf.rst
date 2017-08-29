:man_page: bson_strdupv_printf

bson_strdupv_printf()
=====================

Synopsis
--------

.. code-block:: c

  char *
  bson_strdupv_printf (const char *format, va_list args) BSON_GNUC_PRINTF (1, 0);

Parameters
----------

* ``format``: A printf style format string.
* ``args``: A va_list.

Description
-----------

This function is like :symbol:`bson_strdup_printf()` except takes a va_list of parameters.

Returns
-------

A newly allocated string that should be freed with bson_free().

