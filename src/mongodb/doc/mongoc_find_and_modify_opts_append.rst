:man_page: mongoc_find_and_modify_opts_append

mongoc_find_and_modify_opts_append()
====================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_find_and_modify_opts_append (mongoc_find_and_modify_opts_t *opts,
                                      const bson_t *extra);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.
* ``extra``: A :symbol:`bson:bson_t` with fields and values to append directly to the findAndModify command sent to the server.

Description
-----------

Adds arbitrary options to a `findAndModify <https://docs.mongodb.org/manual/reference/command/findAndModify/>`_ command.

``extra`` does not have to remain valid after calling this function.

Returns
-------

Returns true on success. If any arguments are invalid, returns false and logs an error.

Appending options to findAndModify
----------------------------------

.. literalinclude:: ../examples/find_and_modify_with_opts/opts.c
   :language: c
   :caption: opts.c

