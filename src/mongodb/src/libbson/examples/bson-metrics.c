/*
 * Copyright 2014 MongoDB, Inc.
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
 * This program will scan each BSON document contained in the provided files
 * and print metrics to STDOUT.
 */

#include <bson.h>
#include <stdio.h>
#include <math.h>

#define MAX_RECURSION 100

static double
dtimeofday (void)
{
   struct timeval timeval;
   bson_gettimeofday (&timeval);
   return (timeval.tv_sec + timeval.tv_usec * 0.000001);
}

typedef struct {
   uint64_t count;
   char *description;
} bson_type_metrics_t;

typedef struct {
   uint64_t doc_count;
   uint64_t element_count;
   uint64_t doc_size_max;
   uint64_t key_size_tally;
   uint64_t utf8_size_tally;
   uint32_t depth;
   bson_type_metrics_t bson_type_metrics[256];
} bson_metrics_state_t;

static bson_metrics_state_t initial_state = {
   0L,
   0L,
   0L,
   0L,
   0L,
   0L,
   {{/* BSON_TYPE_EOD        = 0x00 */ 0L, "End of document"},
    {/* BSON_TYPE_DOUBLE     = 0x01 */ 0L, "Floating point"},
    {/* BSON_TYPE_UTF8       = 0x02 */ 0L, "UTF-8 string"},
    {/* BSON_TYPE_DOCUMENT   = 0x03 */ 0L, "Embedded document"},
    {/* BSON_TYPE_ARRAY      = 0x04 */ 0L, "Array"},
    {/* BSON_TYPE_BINARY     = 0x05 */ 0L, "Binary data"},
    {/* BSON_TYPE_UNDEFINED  = 0x06 */ 0L, "Undefined - Deprecated"},
    {/* BSON_TYPE_OID        = 0x07 */ 0L, "ObjectId"},
    {/* BSON_TYPE_BOOL       = 0x08 */ 0L, "Boolean"},
    {/* BSON_TYPE_DATE_TIME  = 0x09 */ 0L, "UTC datetime"},
    {/* BSON_TYPE_NULL       = 0x0A */ 0L, "Null value"},
    {/* BSON_TYPE_REGEX      = 0x0B */ 0L, "Regular expression"},
    {/* BSON_TYPE_DBPOINTER  = 0x0C */ 0L, "DBPointer - Deprecated"},
    {/* BSON_TYPE_CODE       = 0x0D */ 0L, "JavaScript code"},
    {/* BSON_TYPE_SYMBOL     = 0x0E */ 0L, "Symbol - Deprecated"},
    {/* BSON_TYPE_CODEWSCOPE = 0x0F */ 0L, "JavaScript code w/ scope"},
    {/* BSON_TYPE_INT32      = 0x10 */ 0L, "32-bit Integer"},
    {/* BSON_TYPE_TIMESTAMP  = 0x11 */ 0L, "Timestamp"},
    {/* BSON_TYPE_INT64      = 0x12 */ 0L, "64-bit Integer"},
    {0L, NULL}}};

static bson_metrics_state_t state;

static int
compar_bson_type_metrics (const void *a, const void *b)
{
   return (((bson_type_metrics_t *) b)->count -
           ((bson_type_metrics_t *) a)->count);
}

/*
 * Forward declarations.
 */
static bool
bson_metrics_visit_array (const bson_iter_t *iter,
                          const char *key,
                          const bson_t *v_array,
                          void *data);
static bool
bson_metrics_visit_document (const bson_iter_t *iter,
                             const char *key,
                             const bson_t *v_document,
                             void *data);

static bool
bson_metrics_visit_utf8 (const bson_iter_t *iter,
                         const char *key,
                         size_t v_utf8_len,
                         const char *v_utf8,
                         void *data)
{
   bson_metrics_state_t *state = data;
   state->utf8_size_tally += v_utf8_len;

   return false;
}

static bool
bson_metrics_visit_before (const bson_iter_t *iter, const char *key, void *data)
{
   bson_metrics_state_t *state = data;
   bson_type_t btype;
   ++state->element_count;
   state->key_size_tally += strlen (key); /* TODO - filter out array keys(?) */
   btype = bson_iter_type (iter);
   ++state->bson_type_metrics[btype].count;

   return false;
}

static const bson_visitor_t bson_metrics_visitors = {
   bson_metrics_visit_before,
   NULL, /* visit_after */
   NULL, /* visit_corrupt */
   NULL, /* visit_double */
   bson_metrics_visit_utf8,
   bson_metrics_visit_document,
   bson_metrics_visit_array,
   NULL, /* visit_binary */
   NULL, /* visit_undefined */
   NULL, /* visit_oid */
   NULL, /* visit_bool */
   NULL, /* visit_date_time */
   NULL, /* visit_null */
   NULL, /* visit_regex */
   NULL, /* visit_dbpointer */
   NULL, /* visit_code */
   NULL, /* visit_symbol */
   NULL, /* visit_codewscope */
   NULL, /* visit_int32 */
   NULL, /* visit_timestamp */
   NULL, /* visit_int64 */
   NULL, /* visit_maxkey */
   NULL, /* visit_minkey */
};

static bool
bson_metrics_visit_document (const bson_iter_t *iter,
                             const char *key,
                             const bson_t *v_document,
                             void *data)
{
   bson_metrics_state_t *state = data;
   bson_iter_t child;

   if (state->depth >= MAX_RECURSION) {
      fprintf (stderr, "Invalid document, max recursion reached.\n");
      return true;
   }

   if (bson_iter_init (&child, v_document)) {
      state->depth++;
      bson_iter_visit_all (&child, &bson_metrics_visitors, data);
      state->depth--;
   }

   return false;
}

static bool
bson_metrics_visit_array (const bson_iter_t *iter,
                          const char *key,
                          const bson_t *v_array,
                          void *data)
{
   bson_metrics_state_t *state = data;
   bson_iter_t child;

   if (state->depth >= MAX_RECURSION) {
      fprintf (stderr, "Invalid document, max recursion reached.\n");
      return true;
   }

   if (bson_iter_init (&child, v_array)) {
      state->depth++;
      bson_iter_visit_all (&child, &bson_metrics_visitors, data);
      state->depth--;
   }

   return false;
}

static void
bson_metrics (const bson_t *bson, size_t *length, void *data)
{
   bson_iter_t iter;
   bson_metrics_state_t *state = data;
   ++state->doc_count;

   if (bson_iter_init (&iter, bson)) {
      bson_iter_visit_all (&iter, &bson_metrics_visitors, data);
   }
}

int
main (int argc, char *argv[])
{
   bson_reader_t *reader;
   const bson_t *b;
   bson_error_t error;
   const char *filename;
   int i, j;
   double dtime_before, dtime_after, dtime_delta;
   uint64_t aggregate_count;
   off_t mark;

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
   printf ("[");
   for (i = 1; i < argc; i++) {
      if (i > 1)
         printf (",");
      filename = argv[i];

      /*
       * Initialize a new reader for this file descriptor.
       */
      if (!(reader = bson_reader_new_from_file (filename, &error))) {
         fprintf (
            stderr, "Failed to open \"%s\": %s\n", filename, error.message);
         continue;
      }

      state = initial_state;
      dtime_before = dtimeofday ();
      mark = 0;
      while ((b = bson_reader_read (reader, NULL))) {
         off_t pos = bson_reader_tell (reader);
         state.doc_size_max = BSON_MAX (pos - mark, state.doc_size_max);
         mark = pos;
         bson_metrics (b, NULL, &state);
      }
      dtime_after = dtimeofday ();
      dtime_delta = BSON_MAX (dtime_after - dtime_before, 0.000001);
      state.bson_type_metrics[BSON_TYPE_MAXKEY].description = "Max key";
      state.bson_type_metrics[BSON_TYPE_MINKEY].description = "Min key";
      aggregate_count = state.bson_type_metrics[BSON_TYPE_DOCUMENT].count +
                        state.bson_type_metrics[BSON_TYPE_ARRAY].count;
      qsort (state.bson_type_metrics,
             256,
             sizeof (bson_type_metrics_t),
             compar_bson_type_metrics);

      printf ("\n  {\n");
      printf ("    \"file\": \"%s\",\n", filename);
      printf ("    \"secs\": %.2f,\n", dtime_delta);
      printf ("    \"docs_per_sec\": %" PRIu64 ",\n",
              (uint64_t) floor (state.doc_count / dtime_delta));
      printf ("    \"docs\": %" PRIu64 ",\n", state.doc_count);
      printf ("    \"elements\": %" PRIu64 ",\n", state.element_count);
      printf ("    \"elements_per_doc\": %" PRIu64 ",\n",
              (uint64_t) floor ((double) state.element_count /
                                (double) BSON_MAX (state.doc_count, 1)));
      printf ("    \"aggregates\": %" PRIu64 ",\n", aggregate_count);
      printf ("    \"aggregates_per_doc\": %" PRIu64 ",\n",
              (uint64_t) floor ((double) aggregate_count /
                                (double) BSON_MAX (state.doc_count, 1)));
      printf ("    \"degree\": %" PRIu64 ",\n",
              (uint64_t) floor (
                 (double) state.element_count /
                 ((double) BSON_MAX (state.doc_count + aggregate_count, 1))));
      printf ("    \"doc_size_max\": %" PRIu64 ",\n", state.doc_size_max);
      printf ("    \"doc_size_average\": %" PRIu64 ",\n",
              (uint64_t) floor ((double) bson_reader_tell (reader) /
                                (double) BSON_MAX (state.doc_count, 1)));
      printf ("    \"key_size_average\": %" PRIu64 ",\n",
              (uint64_t) floor ((double) state.key_size_tally /
                                (double) BSON_MAX (state.element_count, 1)));
      printf ("    \"string_size_average\": %" PRIu64 ",\n",
              (uint64_t) floor (
                 (double) state.utf8_size_tally /
                 (double) BSON_MAX (
                    state.bson_type_metrics[BSON_TYPE_UTF8].count, 1)));
      printf ("    \"percent_by_type\": {\n");
      for (j = 0; state.bson_type_metrics[j].count > 0; j++) {
         bson_type_metrics_t bson_type_metrics = state.bson_type_metrics[j];
         printf ("      \"%s\": %" PRIu64 ",\n",
                 bson_type_metrics.description,
                 (uint64_t) floor ((double) bson_type_metrics.count * 100.0 /
                                   (double) BSON_MAX (state.element_count, 1)));
      }
      printf ("    }\n");
      printf ("  }");

      /*
       * Cleanup after our reader, which closes the file descriptor.
       */
      bson_reader_destroy (reader);
   }
   printf ("\n]\n");

   return 0;
}
