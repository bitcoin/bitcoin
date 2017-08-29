:man_page: bson_oid_init_from_string

bson_oid_init_from_string()
===========================

Synopsis
--------

.. code-block:: c

  void
  bson_oid_init_from_string (bson_oid_t *oid, const char *str);

Parameters
----------

* ``oid``: A :symbol:`bson_oid_t`.
* ``str``: A string containing a hex encoded version of the oid.

Description
-----------

Parses the string containing hex encoded oid and initialize the bytes in ``oid``.

Example
-------

.. code-block:: c

  bson_oid_init_from_string (&oid, "012345678901234567890123");

