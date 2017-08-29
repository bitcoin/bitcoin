:man_page: mongoc_client_pool_set_error_api

mongoc_client_pool_set_error_api()
==================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_client_pool_set_error_api (mongoc_client_pool_t *client,
                                    int32_t version);

Configure how the C Driver reports errors. See :ref:`Setting the Error API Version <errors_error_api_version>`.

Parameters
----------

* ``pool``: A :symbol:`mongoc_client_pool_t`.
* ``version``: The version of the error API, either ``MONGOC_ERROR_API_VERSION_LEGACY`` or ``MONGOC_ERROR_API_VERSION_2``.

Returns
-------

Returns true if the error API version was set, or logs an error message and returns false if ``version`` is invalid.

.. include:: includes/mongoc_client_pool_call_once.txt
