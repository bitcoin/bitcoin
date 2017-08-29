:man_page: mongoc_write_concern_append

mongoc_write_concern_append()
=============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_write_concern_append (mongoc_write_concern_t *write_concern,
                               bson_t *command);

Parameters
----------

* ``write_concern``: A pointer to a :symbol:`mongoc_write_concern_t`.
* ``command``: A pointer to a :symbol:`bson:bson_t`.

Description
-----------

This function appends a write concern to command options. It is useful for appending a write concern to command options before passing them to :symbol:`mongoc_client_write_command_with_opts` or a related function that takes an options document.

Returns
-------

Returns true on success. If any arguments are invalid, returns false and logs an error.

Example
-------

See the example code for :symbol:`mongoc_client_read_command_with_opts`.

