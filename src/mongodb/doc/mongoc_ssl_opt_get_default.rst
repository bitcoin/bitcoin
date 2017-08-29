:man_page: mongoc_ssl_opt_get_default

mongoc_ssl_opt_get_default()
============================

Synopsis
--------

.. code-block:: c

  const mongoc_ssl_opt_t *
  mongoc_ssl_opt_get_default (void) BSON_GNUC_CONST;

Returns
-------

Returns the default SSL options for the process. This should not be modified or freed.

