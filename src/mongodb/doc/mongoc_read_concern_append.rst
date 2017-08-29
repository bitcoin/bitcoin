:man_page: mongoc_read_concern_append

mongoc_read_concern_append()
============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_read_concern_append (mongoc_read_concern_t *read_concern, bson_t *opts);

Parameters
----------

* ``read_concern``: A pointer to a :symbol:`mongoc_read_concern_t`.
* ``command``: A pointer to a :symbol:`bson:bson_t`.

Description
-----------

This function appends a read concern to command options. It is useful for appending a read concern to command options before passing them to :symbol:`mongoc_client_read_command_with_opts` or a related function that takes an options document.

Returns
-------

Returns true on success. If any arguments are invalid, returns false and logs an error.

Example
-------

See the example code for :symbol:`mongoc_client_read_command_with_opts`.

