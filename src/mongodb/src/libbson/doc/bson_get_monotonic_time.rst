:man_page: bson_get_monotonic_time

System Clock
============

BSON Clock Abstraction

Synopsis
--------

.. code-block:: c

  int64_t
  bson_get_monotonic_time (void);
  int
  bson_gettimeofday (struct timeval *tv,
                     struct timezone *tz);

Description
-----------

The clock abstraction in Libbson provides a cross-platform way to handle timeouts within the BSON library. It abstracts the differences in implementations of ``gettimeofday()`` as well as providing a monotonic (incrementing only) clock in microseconds.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

