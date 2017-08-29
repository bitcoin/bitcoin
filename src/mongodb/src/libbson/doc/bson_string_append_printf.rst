:man_page: bson_string_append_printf

bson_string_append_printf()
===========================

Synopsis
--------

.. code-block:: c

  void
  bson_string_append_printf (bson_string_t *string, const char *format, ...)
     BSON_GNUC_PRINTF (2, 3);

Parameters
----------

* ``string``: A :symbol:`bson_string_t`.
* ``format``: A printf style format string.

Description
-----------

Like bson_string_append() but formats a printf style string and then appends that to ``string``.

