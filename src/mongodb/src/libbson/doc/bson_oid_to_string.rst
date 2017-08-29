:man_page: bson_oid_to_string

bson_oid_to_string()
====================

Synopsis
--------

.. code-block:: c

  void
  bson_oid_to_string (const bson_oid_t *oid, char str[25]);

Parameters
----------

* ``oid``: A :symbol:`bson_oid_t`.
* ``str``: A location for the resulting string.

Description
-----------

Converts ``oid`` into a hex encoded string.

