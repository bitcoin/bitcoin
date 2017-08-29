:man_page: mongoc_rand_add

mongoc_rand_add()
=================

Synopsis
--------

.. code-block:: c

  void
  mongoc_rand_add (const void *buf, int num, double entropy);

Description
-----------

Mixes num bytes of data into the mongoc random number generator.  Entropy specifies a lower bound estimate of the randomness contained in buf.

Parameters
----------

* ``buf``: A buffer.
* ``num``: An int of number of bytes in buf.
* ``entropy``: A double of randomness estimate in buf.

