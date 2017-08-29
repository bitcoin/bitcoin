:man_page: mongoc_tutorial

Tutorial
========

This guide offers a brief introduction to the MongoDB C Driver.

For more information on the C API, please refer to the :doc:`api`.

.. contents::
  :depth: 2

Installing
----------

For detailed instructions on installing the MongoDB C Driver on a particular platform, please see the :doc:`installation guide <installing>`.

Starting MongoDB
----------------

To run the examples in this tutorial, MongoDB must be installed and running on ``localhost`` on the default port, 27017. To check if it is up and running, connect to it with the MongoDB shell.

.. code-block:: none

  $ mongo --host localhost --port 27017
  MongoDB shell version: 3.0.6
  connecting to: localhost:27017/test
  > 

Include and link libmongoc in your C program
--------------------------------------------

Include mongoc.h
^^^^^^^^^^^^^^^^

All libmongoc's functions and types are available in one header file. Simply include ``mongoc.h``:

.. code-block:: c

  #include <mongoc.h>

CMake
'''''

The libmongoc installation includes a `CMake config-file package`_, so you can use CMake's `find_package`_ command to find libmongoc's header and library paths and link to libmongoc:

.. literalinclude:: ../examples/cmake/find_package/CMakeLists.txt
  :caption: CMakeLists.txt
  :start-after: -- sphinx-include-start --

By default, libmongoc is dynamically linked. You can use libmongoc as a static library instead: Use the included ``libmongoc-static-1.0`` config-file package:

.. literalinclude:: ../examples/cmake/find_package_static/CMakeLists.txt
  :start-after: -- sphinx-include-start --
  :emphasize-lines: 2

.. _CMake config-file package: https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html#config-file-packages
.. _find_package: https://cmake.org/cmake/help/latest/command/find_package.html

pkg-config
''''''''''

If you're not using CMake, use `pkg-config`_ on the command line to set header and library paths:

.. literalinclude:: ../examples/compile-with-pkg-config.sh
  :start-after: -- sphinx-include-start --

Or to statically link to libmongoc:

.. literalinclude:: ../examples/compile-with-pkg-config-static.sh
  :start-after: -- sphinx-include-start --

.. _pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/

Specifying header and include paths manually
''''''''''''''''''''''''''''''''''''''''''''

If you aren't using CMake or pkg-config, paths and libraries can be managed manually.

.. code-block:: none

  $ gcc -o hello_mongoc hello_mongoc.c \
      -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0 \
      -lmongoc-1.0 -lbson-1.0
  $ ./hello_mongoc
  { "ok" : 1.000000 }

For Windows users, the code can be compiled and run with the following commands. (This assumes that the MongoDB C Driver has been installed to ``C:\mongo-c-driver``; change the include directory as needed.)

.. code-block:: none

  C:\> cl.exe /IC:\mongo-c-driver\include\libbson-1.0 /IC:\mongo-c-driver\include\libmongoc-1.0 hello_mongoc.c
  C:\> hello_mongoc
  { "ok" : 1.000000 }

.. _tutorial_connecting:

Use libmongoc in a Microsoft Visual Studio Project
--------------------------------------------------

See the :doc:`libmongoc and Visual Studio guide <visual-studio-guide>`.

.. _making-a-connection:

Making a Connection
-------------------

Access MongoDB with a :symbol:`mongoc_client_t`. It transparently connects to standalone servers, replica sets and sharded clusters on demand. To perform operations on a database or collection, create a :symbol:`mongoc_database_t` or :symbol:`mongoc_collection_t` struct from the :symbol:`mongoc_client_t`.

At the start of an application, call :symbol:`mongoc_init` before any other libmongoc functions. At the end, call the appropriate destroy function for each collection, database, or client handle, in reverse order from how they were constructed. Call :symbol:`mongoc_cleanup` before exiting.

The example below establishes a connection to a standalone server on ``localhost``, registers the client application as "connect-example," and performs a simple command.

More information about database operations can be found in the :ref:`CRUD Operations <tutorial_crud_operations>` and :ref:`Executing Commands <tutorial_executing_commands>` sections. Examples of connecting to replica sets and sharded clusters can be found on the :doc:`Advanced Connections <advanced-connections>` page.

.. literalinclude:: ../examples/hello_mongoc.c
  :caption: hello_mongoc.c
  :language: c
  :start-after: -- sphinx-include-start --

Creating BSON Documents
-----------------------

Documents are stored in MongoDB's data format, BSON. The C driver uses :doc:`libbson <bson:index>` to create BSON documents. There are several ways to construct them: appending key-value pairs, using BCON, or parsing JSON.

Appending BSON
^^^^^^^^^^^^^^

A BSON document, represented as a :doc:`bson_t <bson:bson_t>` in code, can be constructed one field at a time using libbson's append functions.

For example, to create a document like this:

.. code-block:: none

  {
     born : ISODate("1906-12-09"),
     died : ISODate("1992-01-01"),
     name : {
        first : "Grace",
        last : "Hopper"
     },
     languages : [ "MATH-MATIC", "FLOW-MATIC", "COBOL" ],
     degrees: [ { degree: "BA", school: "Vassar" }, { degree: "PhD", school: "Yale" } ]
  }

Use the following code:

.. code-block:: c

  #include <bson.h>

  int
  main (int   argc,
        char *argv[])
  {
     struct tm   born = { 0 };
     struct tm   died = { 0 };
     const char *lang_names[] = {"MATH-MATIC", "FLOW-MATIC", "COBOL"};
     const char *schools[] = {"Vassar", "Yale"};
     const char *degrees[] = {"BA", "PhD"};
     uint32_t    i;
     char        buf[16];
     const       char *key;
     size_t      keylen;
     bson_t     *document;
     bson_t      child;
     bson_t      child2;
     char       *str;

     document = bson_new ();

     /*
      * Append { "born" : ISODate("1906-12-09") } to the document.
      * Passing -1 for the length argument tells libbson to calculate the string length.
      */
     born.tm_year = 6;  /* years are 1900-based */
     born.tm_mon = 11;  /* months are 0-based */
     born.tm_mday = 9;
     bson_append_date_time (document, "born", -1, mktime (&born) * 1000);

     /*
      * Append { "died" : ISODate("1992-01-01") } to the document.
      */
     died.tm_year = 92;
     died.tm_mon = 0;
     died.tm_mday = 1;

     /*
      * For convenience, this macro passes length -1 by default.
      */
     BSON_APPEND_DATE_TIME (document, "died", mktime (&died) * 1000);

     /*
      * Append a subdocument.
      */
     BSON_APPEND_DOCUMENT_BEGIN (document, "name", &child);
     BSON_APPEND_UTF8 (&child, "first", "Grace");
     BSON_APPEND_UTF8 (&child, "last", "Hopper");
     bson_append_document_end (document, &child);

     /*
      * Append array of strings. Generate keys "0", "1", "2".
      */
     BSON_APPEND_ARRAY_BEGIN (document, "languages", &child);
     for (i = 0; i < sizeof lang_names / sizeof (char *); ++i) {
        keylen = bson_uint32_to_string (i, &key, buf, sizeof buf);
        bson_append_utf8 (&child, key, (int) keylen, lang_names[i], -1);
     }
     bson_append_array_end (document, &child);

     /*
      * Array of subdocuments:
      *    degrees: [ { degree: "BA", school: "Vassar" }, ... ]
      */
     BSON_APPEND_ARRAY_BEGIN (document, "degrees", &child);
     for (i = 0; i < sizeof degrees / sizeof (char *); ++i) {
        keylen = bson_uint32_to_string (i, &key, buf, sizeof buf);
        bson_append_document_begin (&child, key, (int) keylen, &child2);
        BSON_APPEND_UTF8 (&child2, "degree", degrees[i]);
        BSON_APPEND_UTF8 (&child2, "school", schools[i]);
        bson_append_document_end (&child, &child2);
     }
     bson_append_array_end (document, &child);

     /*
      * Print the document as a JSON string.
      */
     str = bson_as_canonical_extended_json (document, NULL);
     printf ("%s\n", str);
     bson_free (str);

     /*
      * Clean up allocated bson documents.
      */
     bson_destroy (document);
     return 0;
  }

See the :doc:`libbson documentation <bson:bson_t>` for all of the types that can be appended to a :symbol:`bson:bson_t`.

Using BCON
^^^^^^^^^^

*BSON C Object Notation*, BCON for short, is an alternative way of constructing BSON documents in a manner closer to the intended format. It has less type-safety than BSON's append functions but results in less code.

.. code-block:: c

  #include <bson.h>

  int
  main (int   argc,
        char *argv[])
  {
     struct tm born = { 0 };
     struct tm died = { 0 };
     bson_t   *document;
     char     *str;

     born.tm_year = 6;
     born.tm_mon = 11;
     born.tm_mday = 9;

     died.tm_year = 92;
     died.tm_mon = 0;
     died.tm_mday = 1;

     document = BCON_NEW (
        "born", BCON_DATE_TIME (mktime (&born) * 1000),
        "died", BCON_DATE_TIME (mktime (&died) * 1000),
        "name", "{",
        "first", BCON_UTF8 ("Grace"),
        "last", BCON_UTF8 ("Hopper"),
        "}",
        "languages", "[",
        BCON_UTF8 ("MATH-MATIC"),
        BCON_UTF8 ("FLOW-MATIC"),
        BCON_UTF8 ("COBOL"),
        "]",
        "degrees", "[",
        "{", "degree", BCON_UTF8 ("BA"), "school", BCON_UTF8 ("Vassar"), "}",
        "{", "degree", BCON_UTF8 ("PhD"), "school", BCON_UTF8 ("Yale"), "}",
        "]");

     /*
      * Print the document as a JSON string.
      */
     str = bson_as_canonical_extended_json (document, NULL);
     printf ("%s\n", str);
     bson_free (str);

     /*
      * Clean up allocated bson documents.
      */
     bson_destroy (document);
     return 0;
  }

Notice that BCON can create arrays, subdocuments and arbitrary fields.

Creating BSON from JSON
^^^^^^^^^^^^^^^^^^^^^^^

For *single* documents, BSON can be created from JSON strings via :doc:`bson_new_from_json <bson:bson_new_from_json>`.

.. code-block:: c

  #include <bson.h>

  int
  main (int   argc,
        char *argv[])
  {
     bson_error_t error;
     bson_t      *bson;
     char        *string;

     const char *json = "{\"name\": {\"first\":\"Grace\", \"last\":\"Hopper\"}}";
     bson = bson_new_from_json ((const uint8_t *)json, -1, &error);

     if (!bson) {
        fprintf (stderr, "%s\n", error.message);
        return EXIT_FAILURE;
     }

     string = bson_as_canonical_extended_json (bson, NULL);
     printf ("%s\n", string);
     bson_free (string);

     return 0;
  }

To initialize BSON from a sequence of JSON documents, use :doc:`bson_json_reader_t <bson:bson_json_reader_t>`.

.. _tutorial_crud_operations:

Basic CRUD Operations
---------------------

This section demonstrates the basics of using the C Driver to interact with MongoDB.

Inserting a Document
^^^^^^^^^^^^^^^^^^^^

To insert documents into a collection, first obtain a handle to a ``mongoc_collection_t`` via a ``mongoc_client_t``. Then, use :doc:`mongoc_collection_insert() <mongoc_collection_insert>` to add BSON documents to the collection. This example inserts into the database "mydb" and collection "mycoll".

When finished, ensure that allocated structures are freed by using their respective destroy functions.

.. code-block:: c

  #include <bson.h>
  #include <mongoc.h>
  #include <stdio.h>

  int
  main (int   argc,
        char *argv[])
  {
      mongoc_client_t *client;
      mongoc_collection_t *collection;
      bson_error_t error;
      bson_oid_t oid;
      bson_t *doc;

      mongoc_init ();

      client = mongoc_client_new ("mongodb://localhost:27017/?appname=insert-example");
      collection = mongoc_client_get_collection (client, "mydb", "mycoll");

      doc = bson_new ();
      bson_oid_init (&oid, NULL);
      BSON_APPEND_OID (doc, "_id", &oid);
      BSON_APPEND_UTF8 (doc, "hello", "world");

      if (!mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
          fprintf (stderr, "%s\n", error.message);
      }

      bson_destroy (doc);
      mongoc_collection_destroy (collection);
      mongoc_client_destroy (client);
      mongoc_cleanup ();

      return 0;
  }

Compile the code and run it:

.. code-block:: none

  $ gcc -o insert insert.c $(pkg-config --cflags --libs libmongoc-1.0)
  $ ./insert

On Windows:

.. code-block:: none

  C:\> cl.exe /IC:\mongo-c-driver\include\libbson-1.0 /IC:\mongo-c-driver\include\libmongoc-1.0 insert.c
  C:\> insert

To verify that the insert succeeded, connect with the MongoDB shell.

.. code-block:: none

  $ mongo
  MongoDB shell version: 3.0.6
  connecting to: test
  > use mydb
  switched to db mydb
  > db.mycoll.find()
  { "_id" : ObjectId("55ef43766cb5f36a3bae6ee4"), "hello" : "world" }
  > 

.. _tutorial_find:

Finding a Document
^^^^^^^^^^^^^^^^^^

To query a MongoDB collection with the C driver, use the function :doc:`mongoc_collection_find_with_opts() <mongoc_collection_find_with_opts>`. This returns a :doc:`cursor <mongoc_cursor_t>` to the matching documents. The following examples iterate through the result cursors and print the matches to ``stdout`` as JSON strings.

Use a document as a query specifier; for example,

.. code-block:: none

  { "color" : "red" }

will match any document with a field named "color" with value "red". An empty document ``{}`` can be used to match all documents.

This first example uses an empty query specifier to find all documents in the database "mydb" and collection "mycoll".

.. code-block:: c

  #include <bson.h>
  #include <mongoc.h>
  #include <stdio.h>

  int
  main (int argc, char *argv[])
  {
     mongoc_client_t *client;
     mongoc_collection_t *collection;
     mongoc_cursor_t *cursor;
     const bson_t *doc;
     bson_t *query;
     char *str;

     mongoc_init ();

     client =
        mongoc_client_new ("mongodb://localhost:27017/?appname=find-example");
     collection = mongoc_client_get_collection (client, "mydb", "mycoll");
     query = bson_new ();
     cursor = mongoc_collection_find_with_opts (collection, query, NULL, NULL);

     while (mongoc_cursor_next (cursor, &doc)) {
        str = bson_as_canonical_extended_json (doc, NULL);
        printf ("%s\n", str);
        bson_free (str);
     }

     bson_destroy (query);
     mongoc_cursor_destroy (cursor);
     mongoc_collection_destroy (collection);
     mongoc_client_destroy (client);
     mongoc_cleanup ();

     return 0;
  }

Compile the code and run it: 

.. code-block:: none

  $ gcc -o find find.c $(pkg-config --cflags --libs libmongoc-1.0)
  $ ./find
  { "_id" : { "$oid" : "55ef43766cb5f36a3bae6ee4" }, "hello" : "world" }

On Windows:

.. code-block:: none

  C:\> cl.exe /IC:\mongo-c-driver\include\libbson-1.0 /IC:\mongo-c-driver\include\libmongoc-1.0 find.c
  C:\> find
  { "_id" : { "$oid" : "55ef43766cb5f36a3bae6ee4" }, "hello" : "world" }

To look for a specific document, add a specifier to ``query``. This example adds a call to ``BSON_APPEND_UTF8()`` to look for all documents matching ``{"hello" : "world"}``.

.. code-block:: c

  #include <bson.h>
  #include <mongoc.h>
  #include <stdio.h>

  int
  main (int argc, char *argv[])
  {
     mongoc_client_t *client;
     mongoc_collection_t *collection;
     mongoc_cursor_t *cursor;
     const bson_t *doc;
     bson_t *query;
     char *str;

     mongoc_init ();

     client = mongoc_client_new (
        "mongodb://localhost:27017/?appname=find-specific-example");
     collection = mongoc_client_get_collection (client, "mydb", "mycoll");
     query = bson_new ();
     BSON_APPEND_UTF8 (query, "hello", "world");

     cursor = mongoc_collection_find_with_opts (collection, query, NULL, NULL);

     while (mongoc_cursor_next (cursor, &doc)) {
        str = bson_as_canonical_extended_json (doc, NULL);
        printf ("%s\n", str);
        bson_free (str);
     }

     bson_destroy (query);
     mongoc_cursor_destroy (cursor);
     mongoc_collection_destroy (collection);
     mongoc_client_destroy (client);
     mongoc_cleanup ();

     return 0;
  }

.. code-block:: none

  $ gcc -o find-specific find-specific.c $(pkg-config --cflags --libs libmongoc-1.0)
  $ ./find-specific
  { "_id" : { "$oid" : "55ef43766cb5f36a3bae6ee4" }, "hello" : "world" }

.. code-block:: none

  C:\> cl.exe /IC:\mongo-c-driver\include\libbson-1.0 /IC:\mongo-c-driver\include\libmongoc-1.0 find-specific.c
  C:\> find-specific
  { "_id" : { "$oid" : "55ef43766cb5f36a3bae6ee4" }, "hello" : "world" }

Updating a Document
^^^^^^^^^^^^^^^^^^^

This code snippet gives an example of using :doc:`mongoc_collection_update() <mongoc_collection_update>` to update the fields of a document.

Using the "mydb" database, the following example inserts an example document into the "mycoll" collection. Then, using its ``_id`` field, the document is updated with different values and a new field.

.. code-block:: c

  #include <bcon.h>
  #include <bson.h>
  #include <mongoc.h>
  #include <stdio.h>

  int
  main (int argc, char *argv[])
  {
     mongoc_collection_t *collection;
     mongoc_client_t *client;
     bson_error_t error;
     bson_oid_t oid;
     bson_t *doc = NULL;
     bson_t *update = NULL;
     bson_t *query = NULL;

     mongoc_init ();

     client =
        mongoc_client_new ("mongodb://localhost:27017/?appname=update-example");
     collection = mongoc_client_get_collection (client, "mydb", "mycoll");

     bson_oid_init (&oid, NULL);
     doc = BCON_NEW ("_id", BCON_OID (&oid), "key", BCON_UTF8 ("old_value"));

     if (!mongoc_collection_insert (
            collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
        fprintf (stderr, "%s\n", error.message);
        goto fail;
     }

     query = BCON_NEW ("_id", BCON_OID (&oid));
     update = BCON_NEW ("$set",
                        "{",
                        "key",
                        BCON_UTF8 ("new_value"),
                        "updated",
                        BCON_BOOL (true),
                        "}");

     if (!mongoc_collection_update (
            collection, MONGOC_UPDATE_NONE, query, update, NULL, &error)) {
        fprintf (stderr, "%s\n", error.message);
        goto fail;
     }

  fail:
     if (doc)
        bson_destroy (doc);
     if (query)
        bson_destroy (query);
     if (update)
        bson_destroy (update);

     mongoc_collection_destroy (collection);
     mongoc_client_destroy (client);
     mongoc_cleanup ();

     return 0;
  }

Compile the code and run it:

.. code-block:: none

  $ gcc -o update update.c $(pkg-config --cflags --libs libmongoc-1.0)
  $ ./update

On Windows:

.. code-block:: none

  C:\> cl.exe /IC:\mongo-c-driver\include\libbson-1.0 /IC:\mongo-c-driver\include\libmongoc-1.0 update.c
  C:\> update
  { "_id" : { "$oid" : "55ef43766cb5f36a3bae6ee4" }, "hello" : "world" }

To verify that the update succeeded, connect with the MongoDB shell.

.. code-block:: none

  $ mongo
  MongoDB shell version: 3.0.6
  connecting to: test
  > use mydb
  switched to db mydb
  > db.mycoll.find({"updated" : true})
  { "_id" : ObjectId("55ef549236fe322f9490e17b"), "updated" : true, "key" : "new_value" }
  > 

Deleting a Document
^^^^^^^^^^^^^^^^^^^

This example illustrates the use of :doc:`mongoc_collection_remove() <mongoc_collection_remove>` to delete documents.

The following code inserts a sample document into the database "mydb" and collection "mycoll". Then, it deletes all documents matching ``{"hello" : "world"}``.

.. code-block:: c

  #include <bson.h>
  #include <mongoc.h>
  #include <stdio.h>

  int
  main (int argc, char *argv[])
  {
     mongoc_client_t *client;
     mongoc_collection_t *collection;
     bson_error_t error;
     bson_oid_t oid;
     bson_t *doc;

     mongoc_init ();

     client =
        mongoc_client_new ("mongodb://localhost:27017/?appname=delete-example");
     collection = mongoc_client_get_collection (client, "test", "test");

     doc = bson_new ();
     bson_oid_init (&oid, NULL);
     BSON_APPEND_OID (doc, "_id", &oid);
     BSON_APPEND_UTF8 (doc, "hello", "world");

     if (!mongoc_collection_insert (
            collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
        fprintf (stderr, "Insert failed: %s\n", error.message);
     }

     bson_destroy (doc);

     doc = bson_new ();
     BSON_APPEND_OID (doc, "_id", &oid);

     if (!mongoc_collection_remove (
            collection, MONGOC_REMOVE_SINGLE_REMOVE, doc, NULL, &error)) {
        fprintf (stderr, "Delete failed: %s\n", error.message);
     }

     bson_destroy (doc);
     mongoc_collection_destroy (collection);
     mongoc_client_destroy (client);
     mongoc_cleanup ();

     return 0;
  }

Compile the code and run it:

.. code-block:: none

  $ gcc -o delete delete.c $(pkg-config --cflags --libs libmongoc-1.0)
  $ ./delete

On Windows:

.. code-block:: none

  C:\> cl.exe /IC:\mongo-c-driver\include\libbson-1.0 /IC:\mongo-c-driver\include\libmongoc-1.0 delete.c
  C:\> delete

Use the MongoDB shell to prove that the documents have been removed successfully.

.. code-block:: none

  $ mongo
  MongoDB shell version: 3.0.6
  connecting to: test
  > use mydb
  switched to db mydb
  > db.mycoll.count({"hello" : "world"})
  0
  > 

Counting Documents
^^^^^^^^^^^^^^^^^^

Counting the number of documents in a MongoDB collection is similar to performing a :ref:`find operation <tutorial_find>`. This example counts the number of documents matching ``{"hello" : "world"}`` in the database "mydb" and collection "mycoll".

.. code-block:: c

  #include <bson.h>
  #include <mongoc.h>
  #include <stdio.h>

  int
  main (int argc, char *argv[])
  {
     mongoc_client_t *client;
     mongoc_collection_t *collection;
     bson_error_t error;
     bson_t *doc;
     int64_t count;

     mongoc_init ();

     client =
        mongoc_client_new ("mongodb://localhost:27017/?appname=count-example");
     collection = mongoc_client_get_collection (client, "mydb", "mycoll");
     doc = bson_new_from_json (
        (const uint8_t *) "{\"hello\" : \"world\"}", -1, &error);

     count = mongoc_collection_count (
        collection, MONGOC_QUERY_NONE, doc, 0, 0, NULL, &error);

     if (count < 0) {
        fprintf (stderr, "%s\n", error.message);
     } else {
        printf ("%" PRId64 "\n", count);
     }

     bson_destroy (doc);
     mongoc_collection_destroy (collection);
     mongoc_client_destroy (client);
     mongoc_cleanup ();

     return 0;
  }

Compile the code and run it: 

.. code-block:: none

  $ gcc -o count count.c $(pkg-config --cflags --libs libmongoc-1.0)
  $ ./count
  1

On Windows:

.. code-block:: none

  C:\> cl.exe /IC:\mongo-c-driver\include\libbson-1.0 /IC:\mongo-c-driver\include\libmongoc-1.0 count.c
  C:\> count
  1

.. _tutorial_executing_commands:

Executing Commands
------------------

The driver provides helper functions for executing MongoDB commands on client, database and collection structures. These functions return :doc:`cursors <mongoc_cursor_t>`; the ``_simple`` variants return booleans indicating success or failure.

This example executes the `collStats <http://docs.mongodb.org/manual/reference/command/collStats/>`_ command against the collection "mycoll" in database "mydb".

.. code-block:: c

  #include <bson.h>
  #include <bcon.h>
  #include <mongoc.h>
  #include <stdio.h>

  int
  main (int argc, char *argv[])
  {
     mongoc_client_t *client;
     mongoc_collection_t *collection;
     bson_error_t error;
     bson_t *command;
     bson_t reply;
     char *str;

     mongoc_init ();

     client = mongoc_client_new (
        "mongodb://localhost:27017/?appname=executing-example");
     collection = mongoc_client_get_collection (client, "mydb", "mycoll");

     command = BCON_NEW ("collStats", BCON_UTF8 ("mycoll"));
     if (mongoc_collection_command_simple (
            collection, command, NULL, &reply, &error)) {
        str = bson_as_canonical_extended_json (&reply, NULL);
        printf ("%s\n", str);
        bson_free (str);
     } else {
        fprintf (stderr, "Failed to run command: %s\n", error.message);
     }

     bson_destroy (command);
     bson_destroy (&reply);
     mongoc_collection_destroy (collection);
     mongoc_client_destroy (client);
     mongoc_cleanup ();

     return 0;
  }

Compile the code and run it:

.. code-block:: none

  $ gcc -o executing executing.c $(pkg-config --cflags --libs libmongoc-1.0)
  $ ./executing
  { "ns" : "mydb.mycoll", "count" : 1, "size" : 48, "avgObjSize" : 48, "numExtents" : 1, "storageSize" : 8192,
  "lastExtentSize" : 8192.000000, "paddingFactor" : 1.000000, "userFlags" : 1, "capped" : false, "nindexes" : 1,
  "indexDetails" : {  }, "totalIndexSize" : 8176, "indexSizes" : { "_id_" : 8176 }, "ok" : 1.000000 }

On Windows:

.. code-block:: none

  C:\> cl.exe /IC:\mongo-c-driver\include\libbson-1.0 /IC:\mongo-c-driver\include\libmongoc-1.0 executing.c
  C:\> executing
  { "ns" : "mydb.mycoll", "count" : 1, "size" : 48, "avgObjSize" : 48, "numExtents" : 1, "storageSize" : 8192,
  "lastExtentSize" : 8192.000000, "paddingFactor" : 1.000000, "userFlags" : 1, "capped" : false, "nindexes" : 1,
  "indexDetails" : {  }, "totalIndexSize" : 8176, "indexSizes" : { "_id_" : 8176 }, "ok" : 1.000000 }

Threading
---------

The MongoDB C Driver is thread-unaware in the vast majority of its operations. This means it is up to the programmer to guarantee thread-safety.

However, :symbol:`mongoc_client_pool_t` is thread-safe and is used to fetch a ``mongoc_client_t`` in a thread-safe manner. After retrieving a client from the pool, the client structure should be considered owned by the calling thread. When the thread is finished, the client should be placed back into the pool.

.. literalinclude:: ../examples/example-pool.c
   :language: c
   :caption: example-pool.c

Next Steps
----------

To find information on advanced topics, browse the rest of the :doc:`C driver guide <index>` or the `official MongoDB documentation <https://docs.mongodb.org>`_.

For help with common issues, consult the :doc:`Troubleshooting <basic-troubleshooting>` page. To report a bug or request a new feature, follow :ref:`these instructions <basic-troubleshooting_file_bug>`.

