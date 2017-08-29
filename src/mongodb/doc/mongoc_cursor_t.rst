:man_page: mongoc_cursor_t

mongoc_cursor_t
===============

Client-side cursor abstraction

Synopsis
--------

.. code-block:: c

  typedef struct _mongoc_cursor_t mongoc_cursor_t;

``mongoc_cursor_t`` provides access to a MongoDB query cursor.
It wraps up the wire protocol negotiation required to initiate a query and retrieve an unknown number of documents.

Cursors are lazy, meaning that no network traffic occurs until the first call to :symbol:`mongoc_cursor_next()`.

At that point we can:

* Determine which host we've connected to with :symbol:`mongoc_cursor_get_host()`.
* Retrieve more records with repeated calls to :symbol:`mongoc_cursor_next()`.
* Clone a query to repeat execution at a later point with :symbol:`mongoc_cursor_clone()`.
* Test for errors with :symbol:`mongoc_cursor_error()`.

Thread Safety
-------------

``mongoc_cursor_t`` is *NOT* thread safe. It may only be used from the thread it was created from.

Example
-------

.. literalinclude:: ../examples/example-client.c
   :language: c
   :caption: Query MongoDB and iterate results

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_cursor_clone
    mongoc_cursor_current
    mongoc_cursor_destroy
    mongoc_cursor_error
    mongoc_cursor_error_document
    mongoc_cursor_get_batch_size
    mongoc_cursor_get_hint
    mongoc_cursor_get_host
    mongoc_cursor_get_id
    mongoc_cursor_get_limit
    mongoc_cursor_get_max_await_time_ms
    mongoc_cursor_is_alive
    mongoc_cursor_more
    mongoc_cursor_new_from_command_reply
    mongoc_cursor_next
    mongoc_cursor_set_batch_size
    mongoc_cursor_set_hint
    mongoc_cursor_set_limit
    mongoc_cursor_set_max_await_time_ms

