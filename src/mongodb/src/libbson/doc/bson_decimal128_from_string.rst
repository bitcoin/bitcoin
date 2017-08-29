:man_page: bson_decimal128_from_string

bson_decimal128_from_string()
=============================

Synopsis
--------

.. code-block:: c

  bool
  bson_decimal128_from_string (const char *string, bson_decimal128_t *dec);

Parameters
----------

* ``string``: A string containing ASCII encoded Decimal128.
* ``dec``: A :symbol:`bson_decimal128_t`.

Description
-----------

Parses the string containing ascii encoded Decimal128 and initialize the bytes
in ``dec``. See the `Decimal128 specification
<https://github.com/mongodb/specifications/blob/master/source/bson-decimal128/decimal128.rst>`_
for the exact string format.

Returns
-------

Returns ``true`` if valid Decimal128 string was provided, otherwise ``false``
and ``dec`` will be set to ``NaN``.

Example
-------

.. code-block:: c

  bson_decimal128_t dec;
  bson_decimal128_from_string ("1.00", &dec);

