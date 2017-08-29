:man_page: mongoc_server_descriptions_destroy_all

mongoc_server_descriptions_destroy_all()
========================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_server_descriptions_destroy_all (mongoc_server_description_t **sds,
                                          size_t n);

Frees the array of :symbol:`mongoc_server_description_t` structs returned by :symbol:`mongoc_client_get_server_descriptions`.

Parameters
----------

* ``sds``: The array of server descriptions.
* ``n``: The number of elements in ``sds``.

