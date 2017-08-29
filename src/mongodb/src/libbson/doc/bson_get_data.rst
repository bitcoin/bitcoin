:man_page: bson_get_data

bson_get_data()
===============

Synopsis
--------

.. code-block:: c

  const uint8_t *
  bson_get_data (const bson_t *bson);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_get_data()` function shall return the raw buffer of a bson document. This can be used in conjunction with the ``len`` property of a :symbol:`bson_t` if you want to copy the raw buffer around.

Returns
-------

A buffer which should not be modified or freed.

