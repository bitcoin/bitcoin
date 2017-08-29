:man_page: mongoc_stream_socket_new

mongoc_stream_socket_new()
==========================

Synopsis
--------

.. code-block:: c

  mongoc_stream_t *
  mongoc_stream_socket_new (mongoc_socket_t *socket);

Parameters
----------

* ``socket``: A :symbol:`mongoc_socket_t`.

Creates a new :symbol:`mongoc_stream_socket_t` using the :symbol:`mongoc_socket_t` provided.

.. warning::

  This function transfers ownership of ``socket`` to the newly allocated stream.

Returns
-------

A newly allocated :symbol:`mongoc_stream_socket_t` that should be freed with :symbol:`mongoc_stream_destroy()` when no longer in use.

