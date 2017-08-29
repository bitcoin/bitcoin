:man_page: bson_json

JSON
====

Libbson provides routines for converting to and from the JSON format. In particular, it supports the `MongoDB extended JSON <https://docs.mongodb.com/manual/reference/mongodb-extended-json/>`_ format.

Converting BSON to JSON
-----------------------

There are often times where you might want to convert a BSON document to JSON. It is convenient for debugging as well as an interchange format. To help with this, Libbson contains the functions :symbol:`bson_as_canonical_extended_json()` and :symbol:`bson_as_relaxed_extended_json()`. The canonical format preserves BSON type information for values that may have ambiguous representations in JSON (e.g. numeric types).

.. code-block:: c

  bson_t *b;
  size_t len;
  char *str;

  b = BCON_NEW ("a", BCON_INT32 (1));

  str = bson_as_canonical_extended_json (b, &len);
  printf ("%s\n", str);
  bson_free (str);

  bson_destroy (b);

.. code-block:: none

  { "a" : { "$numberInt": "1" } }

The relaxed format prefers JSON primitives for numeric values and may be used if type fidelity is not required.

.. code-block:: c

  bson_t *b;
  size_t len;
  char *str;

  b = BCON_NEW ("a", BCON_INT32 (1));

  str = bson_as_relaxed_extended_json (b, &len);
  printf ("%s\n", str);
  bson_free (str);

  bson_destroy (b);

.. code-block:: none

  { "a" : 1 }

Converting JSON to BSON
-----------------------

Converting back from JSON is also useful and common enough that we added :symbol:`bson_init_from_json()` and :symbol:`bson_new_from_json()`.

The following example creates a new :symbol:`bson_t` from the JSON string ``{"a":1}``.

.. code-block:: c

  bson_t *b;
  bson_error_t error;

  b = bson_new_from_json ("{\"a\":1}", -1, &error);

  if (!b) {
     printf ("Error: %s\n", error.message);
  } else {
     bson_destroy (b);
  }

Streaming JSON Parsing
----------------------

Libbson provides :symbol:`bson_json_reader_t` to allow for parsing a sequence of JSON documents into BSON. The interface is similar to :symbol:`bson_reader_t` but expects the input to be in the `MongoDB extended JSON <https://docs.mongodb.com/manual/reference/mongodb-extended-json/>`_ format.

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

Examples
--------

The following example reads BSON documents from ``stdin`` and prints them to ``stdout`` as JSON.

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
   * This program will print each BSON document contained in the provided files
   * as a JSON string to STDOUT.
   */


  #include <bson.h>
  #include <stdio.h>


  int
  main (int argc, char *argv[])
  {
     bson_reader_t *reader;
     const bson_t *b;
     bson_error_t error;
     const char *filename;
     char *str;
     int i;

     /*
      * Print program usage if no arguments are provided.
      */
     if (argc == 1) {
        fprintf (stderr, "usage: %s [FILE | -]...\nUse - for STDIN.\n", argv[0]);
        return 1;
     }

     /*
      * Process command line arguments expecting each to be a filename.
      */
     for (i = 1; i < argc; i++) {
        filename = argv[i];

        if (strcmp (filename, "-") == 0) {
           reader = bson_reader_new_from_fd (STDIN_FILENO, false);
        } else {
           if (!(reader = bson_reader_new_from_file (filename, &error))) {
              fprintf (
                 stderr, "Failed to open \"%s\": %s\n", filename, error.message);
              continue;
           }
        }

        /*
         * Convert each incoming document to JSON and print to stdout.
         */
        while ((b = bson_reader_read (reader, NULL))) {
           str = bson_as_canonical_extended_json (b, NULL);
           fprintf (stdout, "%s\n", str);
           bson_free (str);
        }

        /*
         * Cleanup after our reader, which closes the file descriptor.
         */
        bson_reader_destroy (reader);
     }

     return 0;
  }

