:man_page: mongoc_client_set_appname

mongoc_client_set_appname()
===========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_client_set_appname (mongoc_client_t *client, const char *appname)

Sets the application name for this client. This string, along with other internal driver details, is sent to the server as part of the initial connection handshake (`"isMaster" <https://docs.mongodb.org/manual/reference/command/isMaster/>`_).

``appname`` is copied, and doesn't have to remain valid after the call to ``mongoc_client_set_appname()``.

This function will log an error and return false in the following cases:

* ``appname`` is longer than ``MONGOC_HANDSHAKE_APPNAME_MAX``
* ``client`` has already initiated a handshake
* ``client`` is from a :symbol:`mongoc_client_pool_t`

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``appname``: The application name, of length at most ``MONGOC_HANDSHAKE_APPNAME_MAX``.

Returns
-------

true if the appname is set successfully. Otherwise, false.

