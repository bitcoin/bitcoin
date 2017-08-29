:man_page: mongoc_rand

mongoc_rand
===========

MongoDB Random Number Generator

Synopsis
--------

.. code-block:: c

  void
  mongoc_rand_add (const void *buf, int num, doubel entropy);

  void
  mongoc_rand_seed (const void *buf, int num);

  int
  mongoc_rand_status (void);

Description
-----------

The ``mongoc_rand`` family of functions provide access to the low level randomness primitives used by the MongoDB C Driver.  In particular, they control the creation of cryptographically strong pseudo-random bytes required by some security mechanisms.

While we can usually pull enough entropy from the environment, you may be required to seed the PRNG manually depending on your OS, hardware and other entropy consumers running on the same system.

Entropy
-------

``mongoc_rand_add`` and ``mongoc_rand_seed`` allow the user to directly provide entropy.  They differ insofar as ``mongoc_rand_seed`` requires that each bit provided is fully random.  ``mongoc_rand_add`` allows the user to specify the degree of randomness in the provided bytes as well.

Status
------

The ``mongoc_rand_status`` function allows the user to check the status of the mongoc PRNG.  This can be used to guarantee sufficient entropy at program startup, rather than waiting for runtime errors to occur.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_rand_add
    mongoc_rand_seed
    mongoc_rand_status

