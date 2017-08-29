/*
 * Copyright 2015 MongoDB, Inc.
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


#include "mongoc-config.h"
#include "mongoc-server-description-private.h"
#include "mongoc-topology-description-private.h"
#include "mongoc-topology-private.h"
#include "mongoc-util-private.h"

#include "TestSuite.h"
#include "test-conveniences.h"

#include "json-test.h"

#ifdef _MSC_VER
#include <io.h>
#else
#include <dirent.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif


mongoc_topology_description_type_t
topology_type_from_test (const char *type)
{
   if (strcmp (type, "ReplicaSetWithPrimary") == 0) {
      return MONGOC_TOPOLOGY_RS_WITH_PRIMARY;
   } else if (strcmp (type, "ReplicaSetNoPrimary") == 0) {
      return MONGOC_TOPOLOGY_RS_NO_PRIMARY;
   } else if (strcmp (type, "Unknown") == 0) {
      return MONGOC_TOPOLOGY_UNKNOWN;
   } else if (strcmp (type, "Single") == 0) {
      return MONGOC_TOPOLOGY_SINGLE;
   } else if (strcmp (type, "Sharded") == 0) {
      return MONGOC_TOPOLOGY_SHARDED;
   }

   fprintf (stderr, "can't parse this: %s", type);
   BSON_ASSERT (0);
   return 0;
}

mongoc_server_description_type_t
server_type_from_test (const char *type)
{
   if (strcmp (type, "RSPrimary") == 0) {
      return MONGOC_SERVER_RS_PRIMARY;
   } else if (strcmp (type, "RSSecondary") == 0) {
      return MONGOC_SERVER_RS_SECONDARY;
   } else if (strcmp (type, "Standalone") == 0) {
      return MONGOC_SERVER_STANDALONE;
   } else if (strcmp (type, "Mongos") == 0) {
      return MONGOC_SERVER_MONGOS;
   } else if (strcmp (type, "PossiblePrimary") == 0) {
      return MONGOC_SERVER_POSSIBLE_PRIMARY;
   } else if (strcmp (type, "RSArbiter") == 0) {
      return MONGOC_SERVER_RS_ARBITER;
   } else if (strcmp (type, "RSOther") == 0) {
      return MONGOC_SERVER_RS_OTHER;
   } else if (strcmp (type, "RSGhost") == 0) {
      return MONGOC_SERVER_RS_GHOST;
   } else if (strcmp (type, "Unknown") == 0) {
      return MONGOC_SERVER_UNKNOWN;
   }
   fprintf (stderr, "ERROR: Unknown server type %s\n", type);
   BSON_ASSERT (0);
   return 0;
}


static mongoc_read_mode_t
read_mode_from_test (const char *mode)
{
   if (strcmp (mode, "Primary") == 0) {
      return MONGOC_READ_PRIMARY;
   } else if (strcmp (mode, "PrimaryPreferred") == 0) {
      return MONGOC_READ_PRIMARY_PREFERRED;
   } else if (strcmp (mode, "Secondary") == 0) {
      return MONGOC_READ_SECONDARY;
   } else if (strcmp (mode, "SecondaryPreferred") == 0) {
      return MONGOC_READ_SECONDARY_PREFERRED;
   } else if (strcmp (mode, "Nearest") == 0) {
      return MONGOC_READ_NEAREST;
   }

   return MONGOC_READ_PRIMARY;
}


static mongoc_ss_optype_t
optype_from_test (const char *op)
{
   if (strcmp (op, "read") == 0) {
      return MONGOC_SS_READ;
   } else if (strcmp (op, "write") == 0) {
      return MONGOC_SS_WRITE;
   }

   return MONGOC_SS_READ;
}


/*
 *-----------------------------------------------------------------------
 *
 * server_description_by_hostname --
 *
 *      Return a reference to a mongoc_server_description_t or NULL.
 *
 *-----------------------------------------------------------------------
 */
mongoc_server_description_t *
server_description_by_hostname (mongoc_topology_description_t *topology,
                                const char *address)
{
   mongoc_set_t *set = topology->servers;
   mongoc_server_description_t *server_iter;
   int i;

   for (i = 0; i < set->items_len; i++) {
      server_iter = (mongoc_server_description_t *) mongoc_set_get_item (
         topology->servers, i);

      if (strcasecmp (address, server_iter->connection_address) == 0) {
         return server_iter;
      }
   }

   return NULL;
}


/*
 *-----------------------------------------------------------------------
 *
 * process_sdam_test_ismaster_responses --
 *
 *      Update a topology description with the ismaster responses in a "phase"
 *      from an SDAM or SDAM Monitoring test, like:
 *
 *      [
 *          [
 *              "a:27017",
 *              {
 *                  "ok": 1,
 *                  "ismaster": false
 *              }
 *          ]
 *      ]
 *
 * See:
 * https://github.com/mongodb/specifications/tree/master/source/server-discovery-and-monitoring/tests
 *
 *-----------------------------------------------------------------------
 */
void
process_sdam_test_ismaster_responses (bson_t *phase,
                                      mongoc_topology_description_t *td)
{
   mongoc_server_description_t *sd;
   bson_t ismasters;
   bson_t ismaster;
   bson_t response;
   bson_iter_t phase_field_iter;
   bson_iter_t ismaster_iter;
   bson_iter_t ismaster_field_iter;
   const char *hostname;

   /* grab ismaster responses out and feed them to topology */
   BSON_ASSERT (bson_iter_init_find (&phase_field_iter, phase, "responses"));
   bson_iter_bson (&phase_field_iter, &ismasters);
   bson_iter_init (&ismaster_iter, &ismasters);

   while (bson_iter_next (&ismaster_iter)) {
      bson_iter_bson (&ismaster_iter, &ismaster);

      bson_iter_init_find (&ismaster_field_iter, &ismaster, "0");
      hostname = bson_iter_utf8 (&ismaster_field_iter, NULL);
      sd = server_description_by_hostname (td, hostname);

      /* if server has been removed from topology, skip */
      if (!sd) {
         continue;
      }

      bson_iter_init_find (&ismaster_field_iter, &ismaster, "1");
      bson_iter_bson (&ismaster_field_iter, &response);

      /* send ismaster through the topology description's handler */
      mongoc_topology_description_handle_ismaster (
         td, sd->id, &response, 1, NULL);
   }
}


/*
 *-----------------------------------------------------------------------
 *
 * check_json_apm_events --
 *
 *      Compare actual APM events with expected sequence. The two docs
 *      are like:
 *
 * [
 *   {
 *     "command_started_event": {
 *       "command": { ... },
 *       "command_name": "count",
 *       "database_name": "command-monitoring-tests"
 *     }
 *   },
 *   {
 *     "command_failed_event": {
 *       "command_name": "count"
 *     }
 *   }
 * ]
 *
 *-----------------------------------------------------------------------
 */
void
check_json_apm_events (const bson_t *events, const bson_t *expectations)
{
   char errmsg[1000] = {0};
   match_ctx_t ctx = {0};
   uint32_t expected_keys;
   uint32_t actual_keys;

   /* Old mongod returns a double for "count", newer returns int32.
    * Ignore this and other insignificant type differences. */
   ctx.strict_numeric_types = false;
   ctx.errmsg = errmsg;
   ctx.errmsg_len = sizeof errmsg;

   expected_keys = bson_count_keys (expectations);
   actual_keys = bson_count_keys (events);

   if (expected_keys != actual_keys) {
      test_error ("command monitoring test failed expectations:\n\n"
                  "%s\n\n"
                  "events:\n%s\n\n"
                  "expected %" PRIu32 " events, got %" PRIu32,
                  bson_as_canonical_extended_json (expectations, NULL),
                  bson_as_canonical_extended_json (events, NULL),
                  expected_keys,
                  actual_keys);

      abort ();
   }

   if (!match_bson_with_ctx (events, expectations, false, &ctx)) {
      test_error ("command monitoring test failed expectations:\n\n"
                  "%s\n\n"
                  "events:\n%s\n\n%s",
                  bson_as_canonical_extended_json (expectations, NULL),
                  bson_as_canonical_extended_json (events, NULL),
                  errmsg);

      abort ();
   }
}


/*
 *-----------------------------------------------------------------------
 *
 * test_server_selection_logic_cb --
 *
 *      Runs the JSON tests for server selection logic that are
 *      included with the Server Selection spec.
 *
 *-----------------------------------------------------------------------
 */
void
test_server_selection_logic_cb (bson_t *test)
{
   bool expected_error;
   bson_error_t error;
   int32_t heartbeat_msec;
   mongoc_topology_description_t topology;
   mongoc_server_description_t *sd;
   mongoc_read_prefs_t *read_prefs;
   mongoc_read_mode_t read_mode;
   mongoc_ss_optype_t op;
   bson_iter_t iter;
   bson_iter_t topology_iter;
   bson_iter_t server_iter;
   bson_iter_t sd_iter;
   bson_iter_t read_pref_iter;
   bson_iter_t tag_sets_iter;
   bson_iter_t last_write_iter;
   bson_iter_t expected_servers_iter;
   bson_t first_tag_set;
   bson_t test_topology;
   bson_t test_servers;
   bson_t server;
   bson_t test_read_pref;
   bson_t test_tag_sets;
   const char *type;
   uint32_t i = 0;
   bool matched_servers[50];
   mongoc_array_t selected_servers;

   _mongoc_array_init (&selected_servers,
                       sizeof (mongoc_server_description_t *));

   BSON_ASSERT (test);

   expected_error =
      bson_iter_init_find (&iter, test, "error") && bson_iter_as_bool (&iter);

   heartbeat_msec = MONGOC_TOPOLOGY_HEARTBEAT_FREQUENCY_MS_SINGLE_THREADED;

   if (bson_iter_init_find (&iter, test, "heartbeatFrequencyMS")) {
      heartbeat_msec = bson_iter_int32 (&iter);
   }

   /* pull out topology description field */
   BSON_ASSERT (bson_iter_init_find (&iter, test, "topology_description"));
   bson_iter_bson (&iter, &test_topology);

   /* set topology state from test */
   BSON_ASSERT (bson_iter_init_find (&topology_iter, &test_topology, "type"));
   type = bson_iter_utf8 (&topology_iter, NULL);

   if (strcmp (type, "Single") == 0) {
      mongoc_topology_description_init (
         &topology, MONGOC_TOPOLOGY_SINGLE, heartbeat_msec);
   } else {
      mongoc_topology_description_init (
         &topology, MONGOC_TOPOLOGY_UNKNOWN, heartbeat_msec);
      topology.type =
         topology_type_from_test (bson_iter_utf8 (&topology_iter, NULL));
   }

   /* for each server description in test, add server to our topology */
   BSON_ASSERT (
      bson_iter_init_find (&topology_iter, &test_topology, "servers"));
   bson_iter_bson (&topology_iter, &test_servers);

   bson_iter_init (&server_iter, &test_servers);
   while (bson_iter_next (&server_iter)) {
      bson_iter_bson (&server_iter, &server);

      /* initialize new server description with given address */
      sd = (mongoc_server_description_t *) bson_malloc0 (sizeof *sd);
      BSON_ASSERT (bson_iter_init_find (&sd_iter, &server, "address"));
      mongoc_server_description_init (sd, bson_iter_utf8 (&sd_iter, NULL), i++);

      BSON_ASSERT (bson_iter_init_find (&sd_iter, &server, "type"));
      sd->type = server_type_from_test (bson_iter_utf8 (&sd_iter, NULL));

      if (bson_iter_init_find (&sd_iter, &server, "avg_rtt_ms")) {
         sd->round_trip_time_msec = bson_iter_int32 (&sd_iter);
      } else if (sd->type != MONGOC_SERVER_UNKNOWN) {
         test_error ("%s has no avg_rtt_ms", sd->host.host_and_port);
         abort ();
      }

      if (bson_iter_init_find (&sd_iter, &server, "maxWireVersion")) {
         sd->max_wire_version = (int32_t) bson_iter_as_int64 (&sd_iter);
      }

      if (bson_iter_init_find (&sd_iter, &server, "lastUpdateTime")) {
         sd->last_update_time_usec = bson_iter_as_int64 (&sd_iter) * 1000;
      }

      if (bson_iter_init_find (&sd_iter, &server, "lastWrite")) {
         BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (&sd_iter) &&
                      bson_iter_recurse (&sd_iter, &last_write_iter) &&
                      bson_iter_find (&last_write_iter, "lastWriteDate") &&
                      BSON_ITER_HOLDS_INT (&last_write_iter));
         sd->last_write_date_ms = bson_iter_as_int64 (&last_write_iter);
      }

      if (bson_iter_init_find (&sd_iter, &server, "tags")) {
         bson_iter_bson (&sd_iter, &sd->tags);
      } else {
         bson_init (&sd->tags);
      }

      /* add new server to our topology description */
      mongoc_set_add (topology.servers, sd->id, sd);
   }

   /* create read preference document from test */
   BSON_ASSERT (bson_iter_init_find (&iter, test, "read_preference"));
   bson_iter_bson (&iter, &test_read_pref);

   if (bson_iter_init_find (&read_pref_iter, &test_read_pref, "mode")) {
      read_mode = read_mode_from_test (bson_iter_utf8 (&read_pref_iter, NULL));
      ASSERT_CMPINT (read_mode, !=, 0);
   } else {
      read_mode = MONGOC_READ_PRIMARY;
   }

   read_prefs = mongoc_read_prefs_new (read_mode);

   if (bson_iter_init_find (&read_pref_iter, &test_read_pref, "tag_sets")) {
      /* ignore  "tag_sets: [{}]" */
      if (bson_iter_recurse (&read_pref_iter, &tag_sets_iter) &&
          bson_iter_next (&tag_sets_iter) &&
          BSON_ITER_HOLDS_DOCUMENT (&tag_sets_iter)) {
         bson_iter_bson (&tag_sets_iter, &first_tag_set);
         if (!bson_empty (&first_tag_set)) {
            /* not empty */
            bson_iter_bson (&read_pref_iter, &test_tag_sets);
            mongoc_read_prefs_set_tags (read_prefs, &test_tag_sets);
         }
      }
   }

   if (bson_iter_init_find (
          &read_pref_iter, &test_read_pref, "maxStalenessSeconds")) {
      mongoc_read_prefs_set_max_staleness_seconds (
         read_prefs, bson_iter_as_int64 (&read_pref_iter));
   }

   /* get operation type */
   op = MONGOC_SS_READ;

   if (bson_iter_init_find (&iter, test, "operation")) {
      op = optype_from_test (bson_iter_utf8 (&iter, NULL));
   }

   if (expected_error) {
      BSON_ASSERT (!mongoc_read_prefs_is_valid (read_prefs) ||
                   !mongoc_topology_compatible (&topology, read_prefs, &error));
      goto DONE;
   }

   /* no expected error */
   BSON_ASSERT (mongoc_read_prefs_is_valid (read_prefs));
   BSON_ASSERT (mongoc_topology_compatible (&topology, read_prefs, &error));

   /* read in latency window servers */
   BSON_ASSERT (bson_iter_init_find (&iter, test, "in_latency_window"));

   /* TODO: use topology_select instead? */
   mongoc_topology_description_suitable_servers (
      &selected_servers,
      op,
      &topology,
      read_prefs,
      MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS);

   /* check each server in expected_servers is in selected_servers */
   memset (matched_servers, 0, sizeof (matched_servers));
   bson_iter_recurse (&iter, &expected_servers_iter);
   while (bson_iter_next (&expected_servers_iter)) {
      bool found = false;
      bson_iter_t host;

      BSON_ASSERT (bson_iter_recurse (&expected_servers_iter, &host));
      BSON_ASSERT (bson_iter_find (&host, "address"));

      for (i = 0; i < selected_servers.len; i++) {
         sd = _mongoc_array_index (
            &selected_servers, mongoc_server_description_t *, i);

         if (strcmp (sd->host.host_and_port, bson_iter_utf8 (&host, NULL)) ==
             0) {
            found = true;
            break;
         }
      }

      if (!found) {
         test_error ("Should have been selected but wasn't: %s",
                     bson_iter_utf8 (&host, NULL));
         abort ();
      }

      matched_servers[i] = true;
   }

   /* check each server in selected_servers is in expected_servers */
   for (i = 0; i < selected_servers.len; i++) {
      if (!matched_servers[i]) {
         sd = _mongoc_array_index (
            &selected_servers, mongoc_server_description_t *, i);

         test_error ("Shouldn't have been selected but was: %s",
                     sd->host.host_and_port);
         abort ();
      }
   }

DONE:
   mongoc_read_prefs_destroy (read_prefs);
   mongoc_topology_description_destroy (&topology);
   _mongoc_array_destroy (&selected_servers);
}

/*
 *-----------------------------------------------------------------------
 *
 * assemble_path --
 *
 *       Given a parent directory and filename, compile a full path to
 *       the child file.
 *
 *       "dst" receives the joined path, delimited by "/" even on Windows.
 *
 *-----------------------------------------------------------------------
 */
void
assemble_path (const char *parent_path,
               const char *child_name,
               char *dst /* OUT */)
{
   char *p;
   int path_len = (int) strlen (parent_path);
   int name_len = (int) strlen (child_name);

   BSON_ASSERT (path_len + name_len + 1 < MAX_TEST_NAME_LENGTH);

   memset (dst, '\0', MAX_TEST_NAME_LENGTH * sizeof (char));
   strncat (dst, parent_path, path_len);
   strncat (dst, "/", 1);
   strncat (dst, child_name, name_len);

   for (p = dst; *p; ++p) {
      if (*p == '\\') {
         *p = '/';
      }
   }
}

/*
 *-----------------------------------------------------------------------
 *
 * collect_tests_from_dir --
 *
 *       Recursively search the directory at @dir_path for files with
 *       '.json' in their filenames. Append all found file paths to
 *       @paths, and return the number of files found.
 *
 *-----------------------------------------------------------------------
 */
int
collect_tests_from_dir (char (*paths)[MAX_TEST_NAME_LENGTH] /* OUT */,
                        const char *dir_path,
                        int paths_index,
                        int max_paths)
{
#ifdef _MSC_VER
   intptr_t handle;
   struct _finddata_t info;

   char child_path[MAX_TEST_NAME_LENGTH];

   handle = _findfirst (dir_path, &info);

   if (handle == -1) {
      return 0;
   }

   while (1) {
      BSON_ASSERT (paths_index < max_paths);

      if (_findnext (handle, &info) == -1) {
         break;
      }

      if (info.attrib & _A_SUBDIR) {
         /* recursively call on child directories */
         if (strcmp (info.name, "..") != 0 && strcmp (info.name, ".") != 0) {
            assemble_path (dir_path, info.name, child_path);
            paths_index = collect_tests_from_dir (
               paths, child_path, paths_index, max_paths);
         }
      } else if (strstr (info.name, ".json")) {
         /* if this is a JSON test, collect its path */
         assemble_path (dir_path, info.name, paths[paths_index++]);
      }
   }

   _findclose (handle);

   return paths_index;
#else
   struct dirent *entry;
   struct stat dir_stat;
   char child_path[MAX_TEST_NAME_LENGTH];
   DIR *dir;

   dir = opendir (dir_path);
   BSON_ASSERT (dir);
   while ((entry = readdir (dir))) {
      BSON_ASSERT (paths_index < max_paths);
      if (strcmp (entry->d_name, "..") == 0 ||
          strcmp (entry->d_name, ".") == 0) {
         continue;
      }

      assemble_path (dir_path, entry->d_name, child_path);

      if (0 == stat (child_path, &dir_stat) && S_ISDIR (dir_stat.st_mode)) {
         /* recursively call on child directories */
         paths_index =
            collect_tests_from_dir (paths, child_path, paths_index, max_paths);
      } else if (strstr (entry->d_name, ".json")) {
         /* if this is a JSON test, collect its path */
         assemble_path (dir_path, entry->d_name, paths[paths_index++]);
      }
   }

   closedir (dir);

   return paths_index;
#endif
}

/*
 *-----------------------------------------------------------------------
 *
 * get_bson_from_json_file --
 *
 *        Open the file at @filename and store its contents in a
 *        bson_t. This function assumes that @filename contains a
 *        single JSON object.
 *
 *        NOTE: caller owns returned bson_t and must free it.
 *
 *-----------------------------------------------------------------------
 */
bson_t *
get_bson_from_json_file (char *filename)
{
   FILE *file;
   long length;
   bson_t *data;
   bson_error_t error;
   const char *buffer;

   file = fopen (filename, "rb");
   if (!file) {
      return NULL;
   }

   /* get file length */
   fseek (file, 0, SEEK_END);
   length = ftell (file);
   fseek (file, 0, SEEK_SET);
   if (length < 1) {
      return NULL;
   }

   /* read entire file into buffer */
   buffer = (const char *) bson_malloc0 (length);
   if (fread ((void *) buffer, 1, length, file) != length) {
      abort ();
   }

   fclose (file);
   if (!buffer) {
      return NULL;
   }

   /* convert to bson */
   data = bson_new_from_json ((const uint8_t *) buffer, length, &error);
   if (!data) {
      fprintf (stderr, "Cannot parse %s: %s\n", filename, error.message);
      abort ();
   }

   bson_free ((void *) buffer);

   return data;
}

/*
 *-----------------------------------------------------------------------
 *
 * install_json_test_suite --
 *
 *      Given a path to a directory containing JSON tests, import each
 *      test into a BSON blob and call the provided callback for
 *      evaluation.
 *
 *      It is expected that the callback will BSON_ASSERT on failure, so if
 *      callback returns quietly the test is considered to have passed.
 *
 *-----------------------------------------------------------------------
 */
void
install_json_test_suite (TestSuite *suite,
                         const char *dir_path,
                         test_hook callback)
{
   char test_paths[MAX_NUM_TESTS][MAX_TEST_NAME_LENGTH];
   int num_tests;
   int i;
   bson_t *test;
   char *skip_json;
   char *ext;

   num_tests =
      collect_tests_from_dir (&test_paths[0], dir_path, 0, MAX_NUM_TESTS);

   for (i = 0; i < num_tests; i++) {
      test = get_bson_from_json_file (test_paths[i]);
      skip_json = COALESCE (strstr (test_paths[i], "/json"),
                            strstr (test_paths[i], "\\json"));
      BSON_ASSERT (skip_json);
      skip_json += strlen ("/json");
      ext = strstr (skip_json, ".json");
      BSON_ASSERT (ext);
      ext[0] = '\0';

      TestSuite_AddFull (suite,
                         skip_json,
                         (void (*) (void *)) callback,
                         (void (*) (void *)) bson_destroy,
                         test,
                         TestSuite_CheckLive);
   }
}
