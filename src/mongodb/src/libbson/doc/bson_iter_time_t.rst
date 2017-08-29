:man_page: bson_iter_time_t

bson_iter_time_t()
==================

Synopsis
--------

.. code-block:: c

  time_t
  bson_iter_time_t (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

The :symbol:`bson_iter_time_t()` function shall return the number of seconds since the UNIX epoch, as contained in the BSON_TYPE_DATE_TIME element.

Returns
-------

A ``time_t`` containing the number of seconds since the UNIX epoch.

