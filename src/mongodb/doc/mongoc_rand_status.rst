:man_page: mongoc_rand_status

mongoc_rand_status()
====================

Synopsis
--------

.. code-block:: c

  int
  mongoc_rand_status (void);

Description
-----------

The status of the mongoc random number generator.

Returns
-------

Returns 1 if the PRNG has been seeded with enough data, 0 otherwise.

