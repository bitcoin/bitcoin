:man_page: mongoc_uri_set_write_concern

mongoc_uri_set_write_concern()
==============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_uri_set_write_concern (mongoc_uri_t *uri,
                                const mongoc_write_concern_t *wc);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``rc``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Sets a MongoDB URI's write concern option, after the URI has been parsed from a string.

