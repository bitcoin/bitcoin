:man_page: bson_json_reader_t

bson_json_reader_t
==================

Bulk JSON to BSON conversion

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  typedef struct _bson_json_reader_t bson_json_reader_t;

  typedef enum {
     BSON_JSON_ERROR_READ_CORRUPT_JS = 1,
     BSON_JSON_ERROR_READ_INVALID_PARAM,
     BSON_JSON_ERROR_READ_CB_FAILURE,
  } bson_json_error_code_t;

Description
-----------

The :symbol:`bson_json_reader_t` structure is used for reading a sequence of JSON documents and transforming them to :symbol:`bson_t` documents.

This can often be useful if you want to perform bulk operations that are defined in a file containing JSON documents.

.. tip::

  :symbol:`bson_json_reader_t` works upon JSON documents formatted in `MongoDB extended JSON <https://docs.mongodb.com/manual/reference/mongodb-extended-json/>`_ format.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_json_data_reader_ingest
    bson_json_data_reader_new
    bson_json_reader_destroy
    bson_json_reader_new
    bson_json_reader_new_from_fd
    bson_json_reader_new_from_file
    bson_json_reader_read

Example
-------

.. code-block:: c

  /*
   * Copyright 2013 MongoDB, Inc.
   *
   * Licensed under the Apache License, Version 2.0 (the "License");
   * you may not use this file except in compliance with the License.
   * You may obtain a copy of the License at
   *
   *   http://www.apache.org/licenses/LICENSE-2.0
   *
   * Unless required by applicable law or agreed to in writing, software
   * distributed under the License is distributed on an "AS IS" BASIS,
   * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   * See the License for the specific language governing permissions and
   * limitations under the License.
   */


  /*
   * This program will print each JSON document contained in the provided files
   * as a BSON string to STDOUT.
   */


  #include <bson.h>
  #include <stdlib.h>
  #include <stdio.h>


  int
  main (int argc, char *argv[])
  {
     bson_json_reader_t *reader;
     bson_error_t error;
     const char *filename;
     bson_t doc = BSON_INITIALIZER;
     int i;
     int b;

     /*
      * Print program usage if no arguments are provided.
      */
     if (argc == 1) {
        fprintf (stderr, "usage: %s FILE...\n", argv[0]);
        return 1;
     }

     /*
      * Process command line arguments expecting each to be a filename.
      */
     for (i = 1; i < argc; i++) {
        filename = argv[i];

        /*
         * Open the filename provided in command line arguments.
         */
        if (0 == strcmp (filename, "-")) {
           reader = bson_json_reader_new_from_fd (STDIN_FILENO, false);
        } else {
           if (!(reader = bson_json_reader_new_from_file (filename, &error))) {
              fprintf (
                 stderr, "Failed to open \"%s\": %s\n", filename, error.message);
              continue;
           }
        }

        /*
         * Convert each incoming document to BSON and print to stdout.
         */
        while ((b = bson_json_reader_read (reader, &doc, &error))) {
           if (b < 0) {
              fprintf (stderr, "Error in json parsing:\n%s\n", error.message);
              abort ();
           }

           if (fwrite (bson_get_data (&doc), 1, doc.len, stdout) != doc.len) {
              fprintf (stderr, "Failed to write to stdout, exiting.\n");
              exit (1);
           }
           bson_reinit (&doc);
        }

        bson_json_reader_destroy (reader);
        bson_destroy (&doc);
     }

     return 0;
  }

