:man_page: bson_reader_t

bson_reader_t
=============

Streaming BSON Document Reader

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  typedef struct _bson_reader_t bson_reader_t;

  bson_reader_t *
  bson_reader_new_from_handle (void *handle,
                               bson_reader_read_func_t rf,
                               bson_reader_destroy_func_t df);
  bson_reader_t *
  bson_reader_new_from_fd (int fd, bool close_on_destroy);
  bson_reader_t *
  bson_reader_new_from_file (const char *path, bson_error_t *error);
  bson_reader_t *
  bson_reader_new_from_data (const uint8_t *data, size_t length);

  void
  bson_reader_destroy (bson_reader_t *reader);

Description
-----------

:symbol:`bson_reader_t` is a structure used for reading a sequence of BSON documents. The sequence can come from a file-descriptor, memory region, or custom callbacks.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_reader_destroy
    bson_reader_destroy_func_t
    bson_reader_new_from_data
    bson_reader_new_from_fd
    bson_reader_new_from_file
    bson_reader_new_from_handle
    bson_reader_read
    bson_reader_read_func_t
    bson_reader_reset
    bson_reader_set_destroy_func
    bson_reader_set_read_func
    bson_reader_tell

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

