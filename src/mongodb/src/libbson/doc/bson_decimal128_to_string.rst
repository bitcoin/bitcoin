:man_page: bson_decimal128_to_string

bson_decimal128_to_string()
===========================

Synopsis
--------

.. code-block:: c

  void
  bson_decimal128_to_string (const bson_decimal128_t *dec, char *str);

Parameters
----------

* ``dec``: A :symbol:`bson_decimal128_t`.
* ``str``: A location of length BSON_DECIMAL128_STRING for the resulting string.

Description
-----------

Converts ``dec`` into a printable string.

Example
-------

.. code-block:: c

  char decimal128_string[BSON_DECIMAL128_STRING];
  bson_decimal128_to_string (&decimal128t, decimal128_string);

