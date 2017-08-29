:man_page: bson_iter_timestamp

bson_iter_timestamp()
=====================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_TIMESTAMP(iter) \
     (bson_iter_type ((iter)) == BSON_TYPE_TIMESTAMP)

  void
  bson_iter_timestamp (const bson_iter_t *iter,
                       uint32_t *timestamp,
                       uint32_t *increment);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``timestamp``: A uint32_t.
* ``increment``: A uint32_t.

Description
-----------

The BSON_TYPE_TIMESTAMP type is not a date/time and is typically used for intra-server communication.

You probably want :symbol:`bson_iter_date_time()`.

The :symbol:`bson_iter_timestamp()` function shall return the contents of a BSON_TYPE_TIMESTAMP element. It is invalid to call this function while observing an element that is not of type BSON_TYPE_TIMESTAMP.

