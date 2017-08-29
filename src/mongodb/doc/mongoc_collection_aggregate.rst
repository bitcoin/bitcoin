:man_page: mongoc_collection_aggregate

mongoc_collection_aggregate()
=============================

Synopsis
--------

.. code-block:: c

  mongoc_cursor_t *
  mongoc_collection_aggregate (mongoc_collection_t *collection,
                               mongoc_query_flags_t flags,
                               const bson_t *pipeline,
                               const bson_t *opts,
                               const mongoc_read_prefs_t *read_prefs)
     BSON_GNUC_WARN_UNUSED_RESULT;

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``flags``: A :symbol:`mongoc_query_flags_t`.
* ``pipeline``: A :symbol:`bson:bson_t` containing the pipeline array.
* ``opts``: A :symbol:`bson:bson_t` containing options for the command, or ``NULL``.
* ``read_prefs``: A :symbol:`mongoc_read_prefs_t` or ``NULL``.

Description
-----------

This function shall execute an aggregation query on the underlying 'collection'. The bson 'pipeline' is not validated, simply passed along as appropriate to the server.  As such, compatibility and errors should be validated in the appropriate server documentation.

In the case of older server versions, < v2.5, the returned cursor is a synthetic iterator over the result set. This provides a limitation insofar as returned documents can be no larger than 16MB. When connecting to newer servers this limitation doesn't exist. The specific test is for wire_version > 0.

For more information on building MongoDB pipelines, see `MongoDB Aggregation Command <http://docs.mongodb.org/manual/reference/command/aggregate/>`_ on the MongoDB website.

.. note::

  The ``pipeline`` parameter should contain a field named ``pipeline`` containing a BSON array of pipeline stages.

To target a specific server, include an integer "serverId" field in ``opts`` with an id obtained first by calling :symbol:`mongoc_client_select_server`, then :symbol:`mongoc_server_description_id` on its return value.

The :symbol:`mongoc_read_concern_t` and the :symbol:`mongoc_write_concern_t` specified on the :symbol:`mongoc_collection_t` will be used, if any.

Returns
-------

This function returns a newly allocated :symbol:`mongoc_cursor_t` that should be freed with :symbol:`mongoc_cursor_destroy()` when no longer in use. The returned :symbol:`mongoc_cursor_t` is never ``NULL``; if the parameters are invalid, the :symbol:`bson:bson_error_t` in the :symbol:`mongoc_cursor_t` is filled out, and the :symbol:`mongoc_cursor_t` is returned before the server is selected.

.. warning::

  Failure to handle the result of this function is a programming error.

Example
-------

.. code-block:: c

  #include <bcon.h>
  #include <mongoc.h>

  static mongoc_cursor_t *
  pipeline_query (mongoc_collection_t *collection)
  {
     mongoc_cursor_t *cursor;
     bson_t *pipeline;

     pipeline = BCON_NEW ("pipeline",
                          "[",
                          "{",
                          "$match",
                          "{",
                          "foo",
                          BCON_UTF8 ("A"),
                          "}",
                          "}",
                          "{",
                          "$match",
                          "{",
                          "bar",
                          BCON_BOOL (false),
                          "}",
                          "}",
                          "]");

     cursor = mongoc_collection_aggregate (
        collection, MONGOC_QUERY_NONE, pipeline, NULL, NULL, NULL);

     bson_destroy (pipeline);

     return cursor;
  }

Other Parameters
----------------

When using ``$out``, the pipeline stage that writes, the write_concern field of the :symbol:`mongoc_cursor_t` will be set to the :symbol:`mongoc_write_concern_t` parameter, if it is valid, and applied to the write command when :symbol:`mongoc_cursor_next()` is called. Pass any other parameters to the ``aggregate`` command, besides ``pipeline``, as fields in ``opts``:

.. code-block:: c

  mongoc_write_concern_t *write_concern = mongoc_write_concern_new ();
  mongoc_write_concern_set_w (write_concern, 3);

  pipeline =
     BCON_NEW ("pipeline", "[", "{", "$out", BCON_UTF8 ("collection2"), "}", "]");

  opts = BCON_NEW ("bypassDocumentValidation", BCON_BOOL (true));
  mongoc_write_concern_append (write_concern, opts);

  cursor = mongoc_collection_aggregate (
     collection1, MONGOC_QUERY_NONE, pipeline, opts, NULL);

