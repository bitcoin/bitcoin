:man_page: mongoc_cursors

Cursors
=======

Handling Cursor Failures
------------------------

Cursors exist on a MongoDB server. However, the ``mongoc_cursor_t`` structure gives the local process a handle to the cursor. It is possible for errors to occur on the server while iterating a cursor on the client. Even a network partition may occur. This means that applications should be robust in handling cursor failures.

While iterating cursors, you should check to see if an error has occurred. See the following example for how to robustly check for errors.

.. code-block:: c

  static void
  print_all_documents (mongoc_collection_t *collection)
  {
     mongoc_cursor_t *cursor;
     const bson_t *doc;
     bson_error_t error;
     bson_t query = BSON_INITIALIZER;
     char *str;

     cursor = mongoc_collection_find_with_opts (collection, query, NULL, NULL);

     while (mongoc_cursor_next (cursor, &doc)) {
        str = bson_as_canonical_extended_json (doc, NULL);
        printf ("%s\n", str);
        bson_free (str);
     }

     if (mongoc_cursor_error (cursor, &error)) {
        fprintf (stderr, "Failed to iterate all documents: %s\n", error.message);
     }

     mongoc_cursor_destroy (cursor);
  }

Destroying Server-Side Cursors
------------------------------

The MongoDB C driver will automatically destroy a server-side cursor when :symbol:`mongoc_cursor_destroy()` is called. Failure to call this function when done with a cursor will leak memory client side as well as consume extra memory server side. If the cursor was configured to never timeout, it will become a memory leak on the server.

.. _cursors_tailable:

Tailable Cursors
----------------

Tailable cursors are cursors that remain open even after they've returned a final result. This way, if more documents are added to a collection (i.e., to the cursor's result set), then you can continue to call :symbol:`mongoc_cursor_next()` to retrieve those additional results.

Here's a complete test case that demonstrates the use of tailable cursors.

.. note::

  Tailable cursors are for capped collections only.

An example to tail the oplog from a replica set.

.. literalinclude:: ../examples/mongoc-tail.c
   :language: c
   :caption: mongoc-tail.c

Let's compile and run this example against a replica set to see updates as they are made.

.. code-block:: none

  $ gcc -Wall -o mongoc-tail mongoc-tail.c $(pkg-config --cflags --libs libmongoc-1.0)
  $ ./mongoc-tail mongodb://example.com/?replicaSet=myReplSet
  {
      "h" : -8458503739429355503,
      "ns" : "test.test",
      "o" : {
          "_id" : {
              "$oid" : "5372ab0a25164be923d10d50"
          }
      },
      "op" : "i",
      "ts" : {
          "$timestamp" : {
              "i" : 1,
              "t" : 1400023818
          }
      },
      "v" : 2
  }

The line of output is a sample from performing ``db.test.insert({})`` from the mongo shell on the replica set.

See also :symbol:`mongoc_cursor_set_max_await_time_ms`.

