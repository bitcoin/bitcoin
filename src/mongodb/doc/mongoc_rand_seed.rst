:man_page: mongoc_rand_seed

mongoc_rand_seed()
==================

Synopsis
--------

.. code-block:: c

  void
  mongoc_rand_seed (const void *buf, int num);

Description
-----------

Seeds the mongoc random number generator with num bytes of entropy.

Parameters
----------

* ``buf``: A buffer.
* ``num``: An int of number of bytes in buf.

