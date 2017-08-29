:man_page: mongoc_uri_get_auth_mechanism

mongoc_uri_get_auth_mechanism()
===============================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_uri_get_auth_mechanism (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches the ``authMechanism`` parameter to an URI if provided.

Returns
-------

A string which should not be modified or freed.

.. only:: html

  .. taglist:: See Also:
    :tags: authmechanism
