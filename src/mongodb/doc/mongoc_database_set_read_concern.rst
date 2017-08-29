:man_page: mongoc_database_set_read_concern

mongoc_database_set_read_concern()
==================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_database_set_read_concern (mongoc_database_t *database,
                                    const mongoc_read_concern_t *read_concern);

This function sets the read concern to use on operations performed with ``database``. Collections created with :symbol:`mongoc_database_get_collection()` after this call will inherit this read concern.

The default read concern is empty: No readConcern is sent to the server unless explicitly configured.

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``read_concern``: A :symbol:`mongoc_read_concern_t`.

