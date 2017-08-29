:man_page: bson_oid_is_valid

bson_oid_is_valid()
===================

Synopsis
--------

.. code-block:: c

  bool
  bson_oid_is_valid (const char *str, size_t length);

Parameters
----------

* ``str``: A string.
* ``length``: The length of ``str``.

Description
-----------

Checks if a string containing a hex encoded string is a valid BSON ObjectID.

Returns
-------

true if ``str`` could be parsed.

