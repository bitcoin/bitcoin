/*
 * Copyright 2017 MongoDB, Inc.
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

/* MongoDB documentation examples
 *
 * One page on the MongoDB docs site shows a set of common tasks, with example
 * code for each driver plus the mongo shell. The source files for these code
 * examples are delimited with "Start Example N" / "End Example N" and so on.
 *
 * These are the C examples for that page.
 */

#include <mongoc.h>

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "test-conveniences.h"


typedef void (*sample_command_fn_t) (mongoc_database_t *db);


static void
test_sample_command (sample_command_fn_t fn,
                     int exampleno,
                     mongoc_database_t *db,
                     mongoc_collection_t *collection,
                     bool drop_collection)
{
   char *example_name = bson_strdup_printf ("example %d", exampleno);
   capture_logs (true);

   fn (db);

   ASSERT_NO_CAPTURED_LOGS (example_name);

   if (drop_collection) {
      mongoc_collection_drop (collection, NULL);
   }

   bson_free (example_name);
}


static void
test_example_1 (mongoc_database_t *db)
{
   /* Start Example 1 */
   mongoc_collection_t *collection;
   bson_t *doc;
   bool r;
   bson_error_t error;

   collection = mongoc_database_get_collection (db, "inventory");
   doc = BCON_NEW (
      "item", BCON_UTF8 ("canvas"),
      "qty", BCON_INT64 (100),
      "tags", "[",
      BCON_UTF8 ("cotton"),
      "]",
      "size", "{",
      "h", BCON_DOUBLE (28),
      "w", BCON_DOUBLE (35.5),
      "uom", BCON_UTF8 ("cm"),
      "}");

   /* MONGOC_INSERT_NONE means "no special options" */
   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc, NULL, &error);
   bson_destroy (doc);

   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }
   /* End Example 1 */
   ASSERT_COUNT (1, collection);
   /* Start Example 1 Post */
done:
   mongoc_collection_destroy (collection);
   /* End Example 1 Post */
}


static void
test_example_2 (mongoc_database_t *db)
{
   /* Start Example 2 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("item", BCON_UTF8 ("canvas"));
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 2 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 2 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 2 Post */
}


static void
test_example_3 (mongoc_database_t *db)
{
   /* Start Example 3 */
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc;
   bool r;
   bson_error_t error;
   bson_t reply;

   collection = mongoc_database_get_collection (db, "inventory");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   doc = BCON_NEW (
      "item", BCON_UTF8 ("journal"),
      "qty", BCON_INT64 (25),
      "tags", "[",
      BCON_UTF8 ("blank"), BCON_UTF8 ("red"),
      "]",
      "size", "{",
      "h", BCON_DOUBLE (14),
      "w", BCON_DOUBLE (21),
      "uom", BCON_UTF8 ("cm"),
      "}");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("mat"),
      "qty", BCON_INT64 (85),
      "tags", "[",
      BCON_UTF8 ("gray"),
      "]",
      "size", "{",
      "h", BCON_DOUBLE (27.9),
      "w", BCON_DOUBLE (35.5),
      "uom", BCON_UTF8 ("cm"),
      "}");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("mousepad"),
      "qty", BCON_INT64 (25),
      "tags", "[",
      BCON_UTF8 ("gel"), BCON_UTF8 ("blue"),
      "]",
      "size", "{",
      "h", BCON_DOUBLE (19),
      "w", BCON_DOUBLE (22.85),
      "uom", BCON_UTF8 ("cm"),
      "}");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   /* "reply" is initialized on success or error */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
   }
   /* End Example 3 */
   ASSERT_COUNT (4, collection);
   /* Start Example 3 Post */
done:
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   /* End Example 3 Post */
}


static void
test_example_6 (mongoc_database_t *db)
{
   /* Start Example 6 */
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc;
   bool r;
   bson_error_t error;
   bson_t reply;

   collection = mongoc_database_get_collection (db, "inventory");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   doc = BCON_NEW (
      "item", BCON_UTF8 ("journal"),
      "qty", BCON_INT64 (25),
      "size", "{",
      "h", BCON_DOUBLE (14),
      "w", BCON_DOUBLE (21),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("notebook"),
      "qty", BCON_INT64 (50),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("paper"),
      "qty", BCON_INT64 (100),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "status", BCON_UTF8 ("D"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("planner"),
      "qty", BCON_INT64 (75),
      "size", "{",
      "h", BCON_DOUBLE (22.85),
      "w", BCON_DOUBLE (30),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("D"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("postcard"),
      "qty", BCON_INT64 (45),
      "size", "{",
      "h", BCON_DOUBLE (10),
      "w", BCON_DOUBLE (15.25),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   /* "reply" is initialized on success or error */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
   }
   /* End Example 6 */
   ASSERT_COUNT (5, collection);
   /* Start Example 6 Post */
done:
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   /* End Example 6 Post */
}


static void
test_example_7 (mongoc_database_t *db)
{
   /* Start Example 7 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (NULL);
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 7 */
   ASSERT_CURSOR_COUNT (5, cursor);
   /* Start Example 7 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 7 Post */
}


static void
test_example_9 (mongoc_database_t *db)
{
   /* Start Example 9 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("status", BCON_UTF8 ("D"));
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 9 */
   ASSERT_CURSOR_COUNT (2, cursor);
   /* Start Example 9 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 9 Post */
}


static void
test_example_10 (mongoc_database_t *db)
{
   /* Start Example 10 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "status", "{",
      "$in", "[",
      BCON_UTF8 ("A"), BCON_UTF8 ("D"),
      "]",
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 10 */
   ASSERT_CURSOR_COUNT (5, cursor);
   /* Start Example 10 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 10 Post */
}


static void
test_example_11 (mongoc_database_t *db)
{
   /* Start Example 11 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "status", BCON_UTF8 ("A"),
      "qty", "{",
      "$lt", BCON_INT64 (30),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 11 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 11 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 11 Post */
}


static void
test_example_12 (mongoc_database_t *db)
{
   /* Start Example 12 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "$or", "[",
      "{",
      "status", BCON_UTF8 ("A"),
      "}","{",
      "qty", "{",
      "$lt", BCON_INT64 (30),
      "}",
      "}",
      "]");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 12 */
   ASSERT_CURSOR_COUNT (3, cursor);
   /* Start Example 12 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 12 Post */
}


static void
test_example_13 (mongoc_database_t *db)
{
   /* Start Example 13 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "status", BCON_UTF8 ("A"),
      "$or", "[",
      "{",
      "qty", "{",
      "$lt", BCON_INT64 (30),
      "}",
      "}","{",
      "item", BCON_REGEX ("^p", ""),
      "}",
      "]");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 13 */
   ASSERT_CURSOR_COUNT (2, cursor);
   /* Start Example 13 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 13 Post */
}


static void
test_example_14 (mongoc_database_t *db)
{
   /* Start Example 14 */
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc;
   bool r;
   bson_error_t error;
   bson_t reply;

   collection = mongoc_database_get_collection (db, "inventory");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   doc = BCON_NEW (
      "item", BCON_UTF8 ("journal"),
      "qty", BCON_INT64 (25),
      "size", "{",
      "h", BCON_DOUBLE (14),
      "w", BCON_DOUBLE (21),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("notebook"),
      "qty", BCON_INT64 (50),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("paper"),
      "qty", BCON_INT64 (100),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "status", BCON_UTF8 ("D"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("planner"),
      "qty", BCON_INT64 (75),
      "size", "{",
      "h", BCON_DOUBLE (22.85),
      "w", BCON_DOUBLE (30),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("D"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("postcard"),
      "qty", BCON_INT64 (45),
      "size", "{",
      "h", BCON_DOUBLE (10),
      "w", BCON_DOUBLE (15.25),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   /* "reply" is initialized on success or error */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
   }
   /* End Example 14 */

   /* Start Example 14 Post */
done:
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   /* End Example 14 Post */
}


static void
test_example_15 (mongoc_database_t *db)
{
   /* Start Example 15 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "size", "{",
      "h", BCON_DOUBLE (14),
      "w", BCON_DOUBLE (21),
      "uom", BCON_UTF8 ("cm"),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 15 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 15 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 15 Post */
}


static void
test_example_16 (mongoc_database_t *db)
{
   /* Start Example 16 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "size", "{",
      "w", BCON_DOUBLE (21),
      "h", BCON_DOUBLE (14),
      "uom", BCON_UTF8 ("cm"),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 16 */
   ASSERT_CURSOR_COUNT (0, cursor);
   /* Start Example 16 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 16 Post */
}


static void
test_example_17 (mongoc_database_t *db)
{
   /* Start Example 17 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("size.uom", BCON_UTF8 ("in"));
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 17 */
   ASSERT_CURSOR_COUNT (2, cursor);
   /* Start Example 17 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 17 Post */
}


static void
test_example_18 (mongoc_database_t *db)
{
   /* Start Example 18 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "size.h", "{",
      "$lt", BCON_INT64 (15),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 18 */
   ASSERT_CURSOR_COUNT (4, cursor);
   /* Start Example 18 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 18 Post */
}


static void
test_example_19 (mongoc_database_t *db)
{
   /* Start Example 19 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "size.h", "{",
      "$lt", BCON_INT64 (15),
      "}",
      "size.uom", BCON_UTF8 ("in"),
      "status", BCON_UTF8 ("D"));
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 19 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 19 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 19 Post */
}


static void
test_example_20 (mongoc_database_t *db)
{
   /* Start Example 20 */
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc;
   bool r;
   bson_error_t error;
   bson_t reply;

   collection = mongoc_database_get_collection (db, "inventory");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   doc = BCON_NEW (
      "item", BCON_UTF8 ("journal"),
      "qty", BCON_INT64 (25),
      "tags", "[",
      BCON_UTF8 ("blank"), BCON_UTF8 ("red"),
      "]",
      "dim_cm", "[",
      BCON_INT64 (14), BCON_INT64 (21),
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("notebook"),
      "qty", BCON_INT64 (50),
      "tags", "[",
      BCON_UTF8 ("red"), BCON_UTF8 ("blank"),
      "]",
      "dim_cm", "[",
      BCON_INT64 (14), BCON_INT64 (21),
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("paper"),
      "qty", BCON_INT64 (100),
      "tags", "[",
      BCON_UTF8 ("red"), BCON_UTF8 ("blank"), BCON_UTF8 ("plain"),
      "]",
      "dim_cm", "[",
      BCON_INT64 (14), BCON_INT64 (21),
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("planner"),
      "qty", BCON_INT64 (75),
      "tags", "[",
      BCON_UTF8 ("blank"), BCON_UTF8 ("red"),
      "]",
      "dim_cm", "[",
      BCON_DOUBLE (22.85), BCON_INT64 (30),
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("postcard"),
      "qty", BCON_INT64 (45),
      "tags", "[",
      BCON_UTF8 ("blue"),
      "]",
      "dim_cm", "[",
      BCON_INT64 (10), BCON_DOUBLE (15.25),
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   /* "reply" is initialized on success or error */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
   }
   /* End Example 20 */

   /* Start Example 20 Post */
done:
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   /* End Example 20 Post */
}


static void
test_example_21 (mongoc_database_t *db)
{
   /* Start Example 21 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "tags", "[",
      BCON_UTF8 ("red"), BCON_UTF8 ("blank"),
      "]");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 21 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 21 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 21 Post */
}


static void
test_example_22 (mongoc_database_t *db)
{
   /* Start Example 22 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "tags", "{",
      "$all", "[",
      BCON_UTF8 ("red"), BCON_UTF8 ("blank"),
      "]",
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 22 */
   ASSERT_CURSOR_COUNT (4, cursor);
   /* Start Example 22 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 22 Post */
}


static void
test_example_23 (mongoc_database_t *db)
{
   /* Start Example 23 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("tags", BCON_UTF8 ("red"));
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 23 */
   ASSERT_CURSOR_COUNT (4, cursor);
   /* Start Example 23 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 23 Post */
}


static void
test_example_24 (mongoc_database_t *db)
{
   /* Start Example 24 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "dim_cm", "{",
      "$gt", BCON_INT64 (25),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 24 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 24 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 24 Post */
}


static void
test_example_25 (mongoc_database_t *db)
{
   /* Start Example 25 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "dim_cm", "{",
      "$gt", BCON_INT64 (15),
      "$lt", BCON_INT64 (20),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 25 */
   ASSERT_CURSOR_COUNT (4, cursor);
   /* Start Example 25 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 25 Post */
}


static void
test_example_26 (mongoc_database_t *db)
{
   /* Start Example 26 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "dim_cm", "{",
      "$elemMatch", "{",
      "$gt", BCON_INT64 (22),
      "$lt", BCON_INT64 (30),
      "}",
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 26 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 26 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 26 Post */
}


static void
test_example_27 (mongoc_database_t *db)
{
   /* Start Example 27 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "dim_cm.1", "{",
      "$gt", BCON_INT64 (25),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 27 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 27 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 27 Post */
}


static void
test_example_28 (mongoc_database_t *db)
{
   /* Start Example 28 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "tags", "{",
      "$size", BCON_INT64 (3),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 28 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 28 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 28 Post */
}


static void
test_example_29 (mongoc_database_t *db)
{
   /* Start Example 29 */
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc;
   bool r;
   bson_error_t error;
   bson_t reply;

   collection = mongoc_database_get_collection (db, "inventory");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   doc = BCON_NEW (
      "item", BCON_UTF8 ("journal"),
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("A"),
      "qty", BCON_INT64 (5),
      "}","{",
      "warehouse", BCON_UTF8 ("C"),
      "qty", BCON_INT64 (15),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("notebook"),
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("C"),
      "qty", BCON_INT64 (5),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("paper"),
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("A"),
      "qty", BCON_INT64 (60),
      "}","{",
      "warehouse", BCON_UTF8 ("B"),
      "qty", BCON_INT64 (15),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("planner"),
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("A"),
      "qty", BCON_INT64 (40),
      "}","{",
      "warehouse", BCON_UTF8 ("B"),
      "qty", BCON_INT64 (5),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("postcard"),
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("B"),
      "qty", BCON_INT64 (15),
      "}","{",
      "warehouse", BCON_UTF8 ("C"),
      "qty", BCON_INT64 (35),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   /* "reply" is initialized on success or error */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
   }
   /* End Example 29 */

   /* Start Example 29 Post */
done:
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   /* End Example 29 Post */
}


static void
test_example_30 (mongoc_database_t *db)
{
   /* Start Example 30 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "instock", "{",
      "warehouse", BCON_UTF8 ("A"),
      "qty", BCON_INT64 (5),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 30 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 30 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 30 Post */
}


static void
test_example_31 (mongoc_database_t *db)
{
   /* Start Example 31 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "instock", "{",
      "qty", BCON_INT64 (5),
      "warehouse", BCON_UTF8 ("A"),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 31 */
   ASSERT_CURSOR_COUNT (0, cursor);
   /* Start Example 31 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 31 Post */
}


static void
test_example_32 (mongoc_database_t *db)
{
   /* Start Example 32 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "instock.0.qty", "{",
      "$lte", BCON_INT64 (20),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 32 */
   ASSERT_CURSOR_COUNT (3, cursor);
   /* Start Example 32 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 32 Post */
}


static void
test_example_33 (mongoc_database_t *db)
{
   /* Start Example 33 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "instock.qty", "{",
      "$lte", BCON_INT64 (20),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 33 */
   ASSERT_CURSOR_COUNT (5, cursor);
   /* Start Example 33 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 33 Post */
}


static void
test_example_34 (mongoc_database_t *db)
{
   /* Start Example 34 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "instock", "{",
      "$elemMatch", "{",
      "qty", BCON_INT64 (5),
      "warehouse", BCON_UTF8 ("A"),
      "}",
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 34 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 34 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 34 Post */
}


static void
test_example_35 (mongoc_database_t *db)
{
   /* Start Example 35 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "instock", "{",
      "$elemMatch", "{",
      "qty", "{",
      "$gt", BCON_INT64 (10),
      "$lte", BCON_INT64 (20),
      "}",
      "}",
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 35 */
   ASSERT_CURSOR_COUNT (3, cursor);
   /* Start Example 35 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 35 Post */
}


static void
test_example_36 (mongoc_database_t *db)
{
   /* Start Example 36 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "instock.qty", "{",
      "$gt", BCON_INT64 (10),
      "$lte", BCON_INT64 (20),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 36 */
   ASSERT_CURSOR_COUNT (4, cursor);
   /* Start Example 36 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 36 Post */
}


static void
test_example_37 (mongoc_database_t *db)
{
   /* Start Example 37 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "instock.qty", BCON_INT64 (5),
      "instock.warehouse", BCON_UTF8 ("A"));
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 37 */
   ASSERT_CURSOR_COUNT (2, cursor);
   /* Start Example 37 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 37 Post */
}


static void
test_example_38 (mongoc_database_t *db)
{
   /* Start Example 38 */
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc;
   bool r;
   bson_error_t error;
   bson_t reply;

   collection = mongoc_database_get_collection (db, "inventory");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   doc = BCON_NEW (
      "_id", BCON_INT64 (1),
      "item", BCON_NULL);

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW ("_id", BCON_INT64 (2));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   /* "reply" is initialized on success or error */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
   }
   /* End Example 38 */

   /* Start Example 38 Post */
done:
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   /* End Example 38 Post */
}


static void
test_example_39 (mongoc_database_t *db)
{
   /* Start Example 39 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("item", BCON_NULL);
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 39 */
   ASSERT_CURSOR_COUNT (2, cursor);
   /* Start Example 39 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 39 Post */
}


static void
test_example_40 (mongoc_database_t *db)
{
   /* Start Example 40 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "item", "{",
      "$type", BCON_INT64 (10),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 40 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 40 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 40 Post */
}


static void
test_example_41 (mongoc_database_t *db)
{
   /* Start Example 41 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW (
      "item", "{",
      "$exists", BCON_BOOL (false),
      "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 41 */
   ASSERT_CURSOR_COUNT (1, cursor);
   /* Start Example 41 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 41 Post */
}


static void
test_example_42 (mongoc_database_t *db)
{
   /* Start Example 42 */
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc;
   bool r;
   bson_error_t error;
   bson_t reply;

   collection = mongoc_database_get_collection (db, "inventory");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   doc = BCON_NEW (
      "item", BCON_UTF8 ("journal"),
      "status", BCON_UTF8 ("A"),
      "size", "{",
      "h", BCON_DOUBLE (14),
      "w", BCON_DOUBLE (21),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("A"),
      "qty", BCON_INT64 (5),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("notebook"),
      "status", BCON_UTF8 ("A"),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("C"),
      "qty", BCON_INT64 (5),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("paper"),
      "status", BCON_UTF8 ("D"),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("A"),
      "qty", BCON_INT64 (60),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("planner"),
      "status", BCON_UTF8 ("D"),
      "size", "{",
      "h", BCON_DOUBLE (22.85),
      "w", BCON_DOUBLE (30),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("A"),
      "qty", BCON_INT64 (40),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("postcard"),
      "status", BCON_UTF8 ("A"),
      "size", "{",
      "h", BCON_DOUBLE (10),
      "w", BCON_DOUBLE (15.25),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("B"),
      "qty", BCON_INT64 (15),
      "}","{",
      "warehouse", BCON_UTF8 ("C"),
      "qty", BCON_INT64 (35),
      "}",
      "]");

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   /* "reply" is initialized on success or error */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
   }
   /* End Example 42 */

   /* Start Example 42 Post */
done:
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   /* End Example 42 Post */
}


static void
test_example_43 (mongoc_database_t *db)
{
   /* Start Example 43 */
   mongoc_collection_t *collection;
   bson_t *filter;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("status", BCON_UTF8 ("A"));
   cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
   /* End Example 43 */
   ASSERT_CURSOR_COUNT (3, cursor);
   /* Start Example 43 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 43 Post */
}


static void
test_example_44 (mongoc_database_t *db)
{
   /* Start Example 44 */
   mongoc_collection_t *collection;
   bson_t *filter;
   bson_t *opts;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("status", BCON_UTF8 ("A"));
   opts = BCON_NEW ("projection", "{", "item", BCON_INT64 (1),
   "status", BCON_INT64 (1), "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, opts, NULL);
   /* End Example 44 */
   {
      const bson_t *doc;

      while (mongoc_cursor_next (cursor, &doc)) {
         ASSERT_HAS_FIELD (doc, "_id");
         ASSERT_HAS_FIELD (doc, "item");
         ASSERT_HAS_FIELD (doc, "status");
         ASSERT_HAS_NOT_FIELD (doc, "size");
         ASSERT_HAS_NOT_FIELD (doc, "instock");
      }
   }
   /* Start Example 44 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 44 Post */
}


static void
test_example_45 (mongoc_database_t *db)
{
   /* Start Example 45 */
   mongoc_collection_t *collection;
   bson_t *filter;
   bson_t *opts;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("status", BCON_UTF8 ("A"));
   opts = BCON_NEW ("projection", "{", "item", BCON_INT64 (1),
   "status", BCON_INT64 (1),
   "_id", BCON_INT64 (0), "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, opts, NULL);
   /* End Example 45 */
   {
      const bson_t *doc;

      while (mongoc_cursor_next (cursor, &doc)) {
         ASSERT_HAS_NOT_FIELD (doc, "_id");
         ASSERT_HAS_FIELD (doc, "item");
         ASSERT_HAS_FIELD (doc, "status");
         ASSERT_HAS_NOT_FIELD (doc, "size");
         ASSERT_HAS_NOT_FIELD (doc, "instock");
      }
   }
   /* Start Example 45 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 45 Post */
}


static void
test_example_46 (mongoc_database_t *db)
{
   /* Start Example 46 */
   mongoc_collection_t *collection;
   bson_t *filter;
   bson_t *opts;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("status", BCON_UTF8 ("A"));
   opts = BCON_NEW ("projection", "{", "status", BCON_INT64 (0),
   "instock", BCON_INT64 (0), "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, opts, NULL);
   /* End Example 46 */
   {
      const bson_t *doc;

      while (mongoc_cursor_next (cursor, &doc)) {
         ASSERT_HAS_FIELD (doc, "_id");
         ASSERT_HAS_FIELD (doc, "item");
         ASSERT_HAS_NOT_FIELD (doc, "status");
         ASSERT_HAS_FIELD (doc, "size");
         ASSERT_HAS_NOT_FIELD (doc, "instock");
      }
   }
   /* Start Example 46 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 46 Post */
}


static void
test_example_47 (mongoc_database_t *db)
{
   /* Start Example 47 */
   mongoc_collection_t *collection;
   bson_t *filter;
   bson_t *opts;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("status", BCON_UTF8 ("A"));
   opts = BCON_NEW ("projection", "{", "item", BCON_INT64 (1),
   "status", BCON_INT64 (1),
   "size.uom", BCON_INT64 (1), "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, opts, NULL);
   /* End Example 47 */
   {
      const bson_t *doc;

      while (mongoc_cursor_next (cursor, &doc)) {
         bson_t size;

         ASSERT_HAS_FIELD (doc, "_id");
         ASSERT_HAS_FIELD (doc, "item");
         ASSERT_HAS_FIELD (doc, "status");
         ASSERT_HAS_FIELD (doc, "size");
         ASSERT_HAS_NOT_FIELD (doc, "instock");
         bson_lookup_doc (doc, "size", &size);
         ASSERT_HAS_FIELD (&size, "uom");
         ASSERT_HAS_NOT_FIELD (&size, "h");
         ASSERT_HAS_NOT_FIELD (&size, "w");
      }
   }
   /* Start Example 47 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 47 Post */
}


static void
test_example_48 (mongoc_database_t *db)
{
   /* Start Example 48 */
   mongoc_collection_t *collection;
   bson_t *filter;
   bson_t *opts;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("status", BCON_UTF8 ("A"));
   opts = BCON_NEW ("projection", "{", "size.uom", BCON_INT64 (0), "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, opts, NULL);
   /* End Example 48 */
   {
      const bson_t *doc;

      while (mongoc_cursor_next (cursor, &doc)) {
         bson_t size;

         ASSERT_HAS_FIELD (doc, "_id");
         ASSERT_HAS_FIELD (doc, "item");
         ASSERT_HAS_FIELD (doc, "status");
         ASSERT_HAS_FIELD (doc, "size");
         ASSERT_HAS_FIELD (doc, "instock");
         bson_lookup_doc (doc, "size", &size);
         ASSERT_HAS_NOT_FIELD (&size, "uom");
         ASSERT_HAS_FIELD (&size, "h");
         ASSERT_HAS_FIELD (&size, "w");
      }
   }
   /* Start Example 48 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 48 Post */
}


static void
test_example_49 (mongoc_database_t *db)
{
   /* Start Example 49 */
   mongoc_collection_t *collection;
   bson_t *filter;
   bson_t *opts;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("status", BCON_UTF8 ("A"));
   opts = BCON_NEW ("projection", "{", "item", BCON_INT64 (1),
   "status", BCON_INT64 (1),
   "instock.qty", BCON_INT64 (1), "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, opts, NULL);
   /* End Example 49 */
   {
      const bson_t *doc;

      while (mongoc_cursor_next (cursor, &doc)) {
         ASSERT_HAS_FIELD (doc, "_id");
         ASSERT_HAS_FIELD (doc, "item");
         ASSERT_HAS_FIELD (doc, "status");
         ASSERT_HAS_NOT_FIELD (doc, "size");
         ASSERT_HAS_FIELD (doc, "instock");
         {
            bson_iter_t iter;

            BSON_ASSERT (bson_iter_init_find (&iter, doc, "instock"));
            while (bson_iter_next (&iter)) {
               bson_t subdoc;

               bson_iter_bson (&iter, &subdoc);
               ASSERT_HAS_NOT_FIELD (&subdoc, "warehouse");
               ASSERT_HAS_FIELD (&subdoc, "qty");
            }
         }
      }
   }
   /* Start Example 49 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 49 Post */
}


static void
test_example_50 (mongoc_database_t *db)
{
   /* Start Example 50 */
   mongoc_collection_t *collection;
   bson_t *filter;
   bson_t *opts;
   mongoc_cursor_t *cursor;

   collection = mongoc_database_get_collection (db, "inventory");
   filter = BCON_NEW ("status", BCON_UTF8 ("A"));
   opts = BCON_NEW ("projection", "{", "item", BCON_INT64 (1),
   "status", BCON_INT64 (1),
   "instock", "{",
   "$slice", BCON_INT64 (-1),
   "}", "}");
   cursor = mongoc_collection_find_with_opts (collection, filter, opts, NULL);
   /* End Example 50 */
   {
      const bson_t *doc;

      while (mongoc_cursor_next (cursor, &doc)) {
         bson_t subdoc;

         ASSERT_HAS_FIELD (doc, "_id");
         ASSERT_HAS_FIELD (doc, "item");
         ASSERT_HAS_FIELD (doc, "status");
         ASSERT_HAS_NOT_FIELD (doc, "size");
         ASSERT_HAS_FIELD (doc, "instock");
         bson_lookup_doc (doc, "instock", &subdoc);
         ASSERT_CMPUINT32 (1, ==, bson_count_keys (&subdoc));
      }
   }
   /* Start Example 50 Post */
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   bson_destroy (filter);
   mongoc_collection_destroy (collection);
   /* End Example 50 Post */
}


static void
test_example_51 (mongoc_database_t *db)
{
   /* Start Example 51 */
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc;
   bool r;
   bson_error_t error;
   bson_t reply;

   collection = mongoc_database_get_collection (db, "inventory");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   doc = BCON_NEW (
      "item", BCON_UTF8 ("canvas"),
      "qty", BCON_INT64 (100),
      "size", "{",
      "h", BCON_DOUBLE (28),
      "w", BCON_DOUBLE (35.5),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("journal"),
      "qty", BCON_INT64 (25),
      "size", "{",
      "h", BCON_DOUBLE (14),
      "w", BCON_DOUBLE (21),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("mat"),
      "qty", BCON_INT64 (85),
      "size", "{",
      "h", BCON_DOUBLE (27.9),
      "w", BCON_DOUBLE (35.5),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("mousepad"),
      "qty", BCON_INT64 (25),
      "size", "{",
      "h", BCON_DOUBLE (19),
      "w", BCON_DOUBLE (22.85),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("P"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("notebook"),
      "qty", BCON_INT64 (50),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "status", BCON_UTF8 ("P"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("paper"),
      "qty", BCON_INT64 (100),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "status", BCON_UTF8 ("D"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("planner"),
      "qty", BCON_INT64 (75),
      "size", "{",
      "h", BCON_DOUBLE (22.85),
      "w", BCON_DOUBLE (30),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("D"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("postcard"),
      "qty", BCON_INT64 (45),
      "size", "{",
      "h", BCON_DOUBLE (10),
      "w", BCON_DOUBLE (15.25),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("sketchbook"),
      "qty", BCON_INT64 (80),
      "size", "{",
      "h", BCON_DOUBLE (14),
      "w", BCON_DOUBLE (21),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("sketch pad"),
      "qty", BCON_INT64 (95),
      "size", "{",
      "h", BCON_DOUBLE (22.85),
      "w", BCON_DOUBLE (30.5),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   /* "reply" is initialized on success or error */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
   }
   /* End Example 51 */

   /* Start Example 51 Post */
done:
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   /* End Example 51 Post */
}


static void
test_example_52 (mongoc_database_t *db)
{
   /* Start Example 52 */
   mongoc_collection_t *collection;
   bson_t *selector;
   bson_t *update;
   bool r;
   bson_error_t error;

   collection = mongoc_database_get_collection (db, "inventory");
   selector = BCON_NEW ("item", BCON_UTF8 ("paper"));
   update = BCON_NEW (
      "$set", "{",
      "size.uom", BCON_UTF8 ("cm"),
      "status", BCON_UTF8 ("P"),
      "}",
      "$currentDate", "{",
      "lastModified", BCON_BOOL (true),
      "}");

   /* MONGOC_UPDATE_NONE means "no special options" */
   r = mongoc_collection_update (collection, MONGOC_UPDATE_NONE, selector,
                                 update, NULL, &error);
   bson_destroy (selector);
   bson_destroy (update);

   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }
   /* End Example 52 */
   {
      bson_t *filter;
      mongoc_cursor_t *cursor;
      const bson_t *doc;

      filter = BCON_NEW ("item", BCON_UTF8 ("paper"));
      cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
      while (mongoc_cursor_next (cursor, &doc)) {
         ASSERT_CMPSTR (bson_lookup_utf8 (doc, "size.uom"), "cm");
         ASSERT_CMPSTR (bson_lookup_utf8 (doc, "status"), "P");
         ASSERT_HAS_FIELD (doc, "lastModified");
      }
      mongoc_cursor_destroy (cursor);
      bson_destroy (filter);
   }
   /* Start Example 52 Post */
done:
   mongoc_collection_destroy (collection);
   /* End Example 52 Post */
}


static void
test_example_53 (mongoc_database_t *db)
{
   /* Start Example 53 */
   mongoc_collection_t *collection;
   bson_t *selector;
   bson_t *update;
   bool r;
   bson_error_t error;

   collection = mongoc_database_get_collection (db, "inventory");
   selector = BCON_NEW (
      "qty", "{",
      "$lt", BCON_INT64 (50),
      "}");
   update = BCON_NEW (
      "$set", "{",
      "size.uom", BCON_UTF8 ("in"),
      "status", BCON_UTF8 ("P"),
      "}",
      "$currentDate", "{",
      "lastModified", BCON_BOOL (true),
      "}");

   r = mongoc_collection_update (collection, MONGOC_UPDATE_MULTI_UPDATE, selector, update, NULL, &error);
   bson_destroy (selector);
   bson_destroy (update);

   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }
   /* End Example 53 */
   {
      bson_t *filter;
      mongoc_cursor_t *cursor;
      const bson_t *doc;

      filter = BCON_NEW (
         "qty", "{",
         "$lt", BCON_INT64 (50),
         "}");
      cursor = mongoc_collection_find_with_opts (collection, filter, NULL, NULL);
      while (mongoc_cursor_next (cursor, &doc)) {
         ASSERT_CMPSTR (bson_lookup_utf8 (doc, "size.uom"), "in");
         ASSERT_CMPSTR (bson_lookup_utf8 (doc, "status"), "P");
         ASSERT_HAS_FIELD (doc, "lastModified");
      }
      mongoc_cursor_destroy (cursor);
      bson_destroy (filter);
   }
   /* Start Example 53 Post */
done:
   mongoc_collection_destroy (collection);
   /* End Example 53 Post */
}


static void
test_example_54 (mongoc_database_t *db)
{
   /* Start Example 54 */
   mongoc_collection_t *collection;
   bson_t *selector;
   bson_t *replacement;
   bool r;
   bson_error_t error;

   collection = mongoc_database_get_collection (db, "inventory");
   selector = BCON_NEW ("item", BCON_UTF8 ("paper"));
   replacement = BCON_NEW (
      "item", BCON_UTF8 ("paper"),
      "instock", "[",
      "{",
      "warehouse", BCON_UTF8 ("A"),
      "qty", BCON_INT64 (60),
      "}","{",
      "warehouse", BCON_UTF8 ("B"),
      "qty", BCON_INT64 (40),
      "}",
      "]");

   /* MONGOC_UPDATE_NONE means "no special options" */
   r = mongoc_collection_update (collection, MONGOC_UPDATE_NONE, selector, replacement, NULL, &error);
   bson_destroy (selector);
   bson_destroy (replacement);

   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }
   /* End Example 54 */
   {
      bson_t *filter;
      bson_t *opts;
      mongoc_cursor_t *cursor;
      const bson_t *doc;

      filter = BCON_NEW ("item", BCON_UTF8 ("paper"));
      opts = BCON_NEW ("projection", "{", "_id", BCON_INT64 (0), "}");
      cursor = mongoc_collection_find_with_opts (collection, filter, opts, NULL);
      while (mongoc_cursor_next (cursor, &doc)) {
         bson_t subdoc;

         ASSERT_CMPUINT32 (2, ==, bson_count_keys (doc));
         ASSERT_HAS_FIELD (doc, "item");
         ASSERT_HAS_FIELD (doc, "instock");
         bson_lookup_doc (doc, "instock", &subdoc);
         ASSERT_CMPUINT32 (2, ==, bson_count_keys (&subdoc));
      }
      mongoc_cursor_destroy (cursor);
      bson_destroy (opts);
      bson_destroy (filter);
   }
   /* Start Example 54 Post */
done:
   mongoc_collection_destroy (collection);
   /* End Example 54 Post */
}


static void
test_example_55 (mongoc_database_t *db)
{
   /* Start Example 55 */
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc;
   bool r;
   bson_error_t error;
   bson_t reply;

   collection = mongoc_database_get_collection (db, "inventory");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   doc = BCON_NEW (
      "item", BCON_UTF8 ("journal"),
      "qty", BCON_INT64 (25),
      "size", "{",
      "h", BCON_DOUBLE (14),
      "w", BCON_DOUBLE (21),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("notebook"),
      "qty", BCON_INT64 (50),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "status", BCON_UTF8 ("P"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("paper"),
      "qty", BCON_INT64 (100),
      "size", "{",
      "h", BCON_DOUBLE (8.5),
      "w", BCON_DOUBLE (11),
      "uom", BCON_UTF8 ("in"),
      "}",
      "status", BCON_UTF8 ("D"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("planner"),
      "qty", BCON_INT64 (75),
      "size", "{",
      "h", BCON_DOUBLE (22.85),
      "w", BCON_DOUBLE (30),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("D"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   doc = BCON_NEW (
      "item", BCON_UTF8 ("postcard"),
      "qty", BCON_INT64 (45),
      "size", "{",
      "h", BCON_DOUBLE (10),
      "w", BCON_DOUBLE (15.25),
      "uom", BCON_UTF8 ("cm"),
      "}",
      "status", BCON_UTF8 ("A"));

   r = mongoc_bulk_operation_insert_with_opts (bulk, doc, NULL, &error);
   bson_destroy (doc);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }

   /* "reply" is initialized on success or error */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
   }
   /* End Example 55 */
   ASSERT_COUNT (5, collection);
   /* Start Example 55 Post */
done:
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   /* End Example 55 Post */
}


static void
test_example_57 (mongoc_database_t *db)
{
   /* Start Example 57 */
   mongoc_collection_t *collection;
   bson_t *selector;
   bool r;
   bson_error_t error;

   collection = mongoc_database_get_collection (db, "inventory");
   selector = BCON_NEW ("status", BCON_UTF8 ("A"));

   /* MONGOC_REMOVE_NONE means "no special options" */
   r = mongoc_collection_remove (collection, MONGOC_REMOVE_NONE, selector, NULL, &error);
   bson_destroy (selector);

   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }
   /* End Example 57 */
   ASSERT_COUNT (3, collection);
   /* Start Example 57 Post */
done:
   mongoc_collection_destroy (collection);
   /* End Example 57 Post */
}


static void
test_example_58 (mongoc_database_t *db)
{
   /* Start Example 58 */
   mongoc_collection_t *collection;
   bson_t *selector;
   bool r;
   bson_error_t error;

   collection = mongoc_database_get_collection (db, "inventory");
   selector = BCON_NEW ("status", BCON_UTF8 ("D"));

   r = mongoc_collection_remove (collection, MONGOC_REMOVE_SINGLE_REMOVE, selector, NULL, &error);
   bson_destroy (selector);

   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }
   /* End Example 58 */
   ASSERT_COUNT (2, collection);
   /* Start Example 58 Post */
done:
   mongoc_collection_destroy (collection);
   /* End Example 58 Post */
}


static void
test_example_56 (mongoc_database_t *db)
{
   /* Start Example 56 */
   mongoc_collection_t *collection;
   bson_t *selector;
   bool r;
   bson_error_t error;

   collection = mongoc_database_get_collection (db, "inventory");
   selector = BCON_NEW (NULL);

   /* MONGOC_REMOVE_NONE means "no special options" */
   r = mongoc_collection_remove (collection, MONGOC_REMOVE_NONE, selector, NULL, &error);
   bson_destroy (selector);

   if (!r) {
      MONGOC_ERROR ("%s\n", error.message);
      goto done;
   }
   /* End Example 56 */
   ASSERT_COUNT (0, collection);
   /* Start Example 56 Post */
done:
   mongoc_collection_destroy (collection);
   /* End Example 56 Post */
}


static void
test_sample_commands (void *ctx)
{
   mongoc_client_t *client;
   mongoc_database_t *db;
   mongoc_collection_t *collection;

   client = test_framework_client_new ();
   db = mongoc_client_get_database (client, "test_sample_command");
   collection = mongoc_database_get_collection (db, "inventory");
   mongoc_collection_drop (collection, NULL);

   test_sample_command (test_example_1, 1, db, collection, false);
   test_sample_command (test_example_2, 2, db, collection, false);
   test_sample_command (test_example_3, 3, db, collection, true);
   test_sample_command (test_example_6, 6, db, collection, false);
   test_sample_command (test_example_7, 7, db, collection, false);
   test_sample_command (test_example_9, 9, db, collection, false);
   test_sample_command (test_example_10, 10, db, collection, false);
   test_sample_command (test_example_11, 11, db, collection, false);
   test_sample_command (test_example_12, 12, db, collection, false);
   test_sample_command (test_example_13, 13, db, collection, true);
   test_sample_command (test_example_14, 14, db, collection, false);
   test_sample_command (test_example_15, 15, db, collection, false);
   test_sample_command (test_example_16, 16, db, collection, false);
   test_sample_command (test_example_17, 17, db, collection, false);
   test_sample_command (test_example_18, 18, db, collection, false);
   test_sample_command (test_example_19, 19, db, collection, true);
   test_sample_command (test_example_20, 20, db, collection, false);
   test_sample_command (test_example_21, 21, db, collection, false);
   test_sample_command (test_example_22, 22, db, collection, false);
   test_sample_command (test_example_23, 23, db, collection, false);
   test_sample_command (test_example_24, 24, db, collection, false);
   test_sample_command (test_example_25, 25, db, collection, false);
   test_sample_command (test_example_26, 26, db, collection, false);
   test_sample_command (test_example_27, 27, db, collection, false);
   test_sample_command (test_example_28, 28, db, collection, true);
   test_sample_command (test_example_29, 29, db, collection, false);
   test_sample_command (test_example_30, 30, db, collection, false);
   test_sample_command (test_example_31, 31, db, collection, false);
   test_sample_command (test_example_32, 32, db, collection, false);
   test_sample_command (test_example_33, 33, db, collection, false);
   test_sample_command (test_example_34, 34, db, collection, false);
   test_sample_command (test_example_35, 35, db, collection, false);
   test_sample_command (test_example_36, 36, db, collection, false);
   test_sample_command (test_example_37, 37, db, collection, true);
   test_sample_command (test_example_38, 38, db, collection, false);
   test_sample_command (test_example_39, 39, db, collection, false);
   test_sample_command (test_example_40, 40, db, collection, false);
   test_sample_command (test_example_41, 41, db, collection, true);
   test_sample_command (test_example_42, 42, db, collection, false);
   test_sample_command (test_example_43, 43, db, collection, false);
   test_sample_command (test_example_44, 44, db, collection, false);
   test_sample_command (test_example_45, 45, db, collection, false);
   test_sample_command (test_example_46, 46, db, collection, false);
   test_sample_command (test_example_47, 47, db, collection, false);
   test_sample_command (test_example_48, 48, db, collection, false);
   test_sample_command (test_example_49, 49, db, collection, false);
   test_sample_command (test_example_50, 50, db, collection, true);
   test_sample_command (test_example_51, 51, db, collection, false);
   test_sample_command (test_example_52, 52, db, collection, false);
   test_sample_command (test_example_53, 53, db, collection, false);
   test_sample_command (test_example_54, 54, db, collection, true);
   test_sample_command (test_example_55, 55, db, collection, false);
   test_sample_command (test_example_57, 57, db, collection, false);
   test_sample_command (test_example_58, 58, db, collection, false);
   test_sample_command (test_example_56, 56, db, collection, true);

   mongoc_collection_drop (collection, NULL);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
}


void
test_samples_install (TestSuite *suite)
{
   /* One of the examples uses MongoDB 2.6+'s $currentDate */
   TestSuite_AddFull (suite,
                      "/Samples",
                      test_sample_commands,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_1);
}
