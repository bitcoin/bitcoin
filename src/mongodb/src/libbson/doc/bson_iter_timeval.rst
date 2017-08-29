:man_page: bson_iter_timeval

bson_iter_timeval()
===================

Synopsis
--------

.. code-block:: c

  void
  bson_iter_timeval (const bson_iter_t *iter, struct timeval *tv);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``tv``: A struct timeval.

Description
-----------

The :symbol:`bson_iter_timeval()` function shall return the number of seconds and microseconds since the UNIX epoch, as contained in the BSON_TYPE_DATE_TIME element into ``tv``.

