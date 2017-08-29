:man_page: bson_set_error

bson_set_error()
================

Synopsis
--------

.. code-block:: c

  void
  bson_set_error (
     bson_error_t *error, uint32_t domain, uint32_t code, const char *format, ...)
     BSON_GNUC_PRINTF (4, 5);

Parameters
----------

* ``error``: A :symbol:`bson_error_t`.
* ``domain``: A ``uint32_t``.
* ``code``: A ``uint32_t``.
* ``format``: A ``printf()`` style format string.

Description
-----------

This is a helper function to set the parameters of a :symbol:`bson_error_t`. It handles the case where ``error`` is NULL by doing nothing.

