:man_page: bson_iter_date_time

bson_iter_date_time()
=====================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_DATE_TIME(iter) \
     (bson_iter_type ((iter)) == BSON_TYPE_DATE_TIME)

  int64_t
  bson_iter_date_time (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

The bson_iter_date_time() function shall return the number of milliseconds since the UNIX epoch, as contained in the BSON_TYPE_DATE_TIME element.

Returns
-------

A 64-bit integer containing the number of milliseconds since the UNIX epoch.

