:man_page: mongoc_write_concern_get_wmajority

mongoc_write_concern_get_wmajority()
====================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_write_concern_get_wmajority (
     const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Fetches if the write should be written to a majority of nodes before indicating success.

Returns
-------

Returns true if wmajority is set.

