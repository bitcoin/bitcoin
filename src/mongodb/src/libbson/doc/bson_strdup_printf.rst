:man_page: bson_strdup_printf

bson_strdup_printf()
====================

Synopsis
--------

.. code-block:: c

  char *
  bson_strdup_printf (const char *format, ...) BSON_GNUC_PRINTF (1, 2);

Parameters
----------

* ``format``: A printf style format string.

Description
-----------

This function performs a printf style format but into a newly allocated string.

Returns
-------

A newly allocated string that should be freed with bson_free().

