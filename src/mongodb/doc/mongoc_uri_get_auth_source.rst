:man_page: mongoc_uri_get_auth_source

mongoc_uri_get_auth_source()
============================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_uri_get_auth_source (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches the ``authSource`` parameter of an URI if provided.

Returns
-------

A string which should not be modified or freed.

.. only:: html

  .. taglist:: See Also:
    :tags: authmechanism
