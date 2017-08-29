:man_page: mongoc_write_concern_get_w

mongoc_write_concern_get_w()
============================

Synopsis
--------

.. code-block:: c

  int32_t
  mongoc_write_concern_get_w (const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Fetches the ``w`` parameter of the write concern.

Returns
-------

Returns an integer containing the w value. If wmajority is set, this would be MONGOC_WRITE_CONCERN_W_MAJORITY.

