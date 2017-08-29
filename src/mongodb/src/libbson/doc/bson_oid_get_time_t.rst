:man_page: bson_oid_get_time_t

bson_oid_get_time_t()
=====================

Synopsis
--------

.. code-block:: c

  time_t
  bson_oid_get_time_t (const bson_oid_t *oid);

Parameters
----------

* ``oid``: A :symbol:`bson_oid_t`.

Description
-----------

Fetches the generation time in seconds since the UNIX Epoch of ``oid``.

Returns
-------

A time_t containing the seconds since the UNIX epoch of ``oid``.

