#include <mongoc.h>

#include <mongoc-set-private.h>

#include "json-test.h"

#include "mongoc-client-private.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif


static void
_topology_has_description (mongoc_topology_description_t *topology,
                           bson_t *server,
                           const char *address)
{
   mongoc_server_description_t *sd;
   bson_iter_t server_iter;
   const char *set_name;

   sd = server_description_by_hostname (topology, address);
   BSON_ASSERT (sd);

   bson_iter_init (&server_iter, server);
   while (bson_iter_next (&server_iter)) {
      if (strcmp ("setName", bson_iter_key (&server_iter)) == 0) {
         set_name = bson_iter_utf8 (&server_iter, NULL);
         if (set_name) {
            BSON_ASSERT (sd->set_name);
            ASSERT_CMPSTR (sd->set_name, set_name);
         }
      } else if (strcmp ("type", bson_iter_key (&server_iter)) == 0) {
         BSON_ASSERT (sd->type == server_type_from_test (
                                     bson_iter_utf8 (&server_iter, NULL)));
      } else if (strcmp ("setVersion", bson_iter_key (&server_iter)) == 0) {
         int64_t expected_set_version;
         if (BSON_ITER_HOLDS_NULL (&server_iter)) {
            expected_set_version = MONGOC_NO_SET_VERSION;
         } else {
            expected_set_version = bson_iter_as_int64 (&server_iter);
         }
         BSON_ASSERT (sd->set_version == expected_set_version);
      } else if (strcmp ("electionId", bson_iter_key (&server_iter)) == 0) {
         bson_oid_t expected_oid;
         if (BSON_ITER_HOLDS_NULL (&server_iter)) {
            bson_oid_init_from_string (&expected_oid,
                                       "000000000000000000000000");
         } else {
            ASSERT (BSON_ITER_HOLDS_OID (&server_iter));
            bson_oid_copy (bson_iter_oid (&server_iter), &expected_oid);
         }

         ASSERT_CMPOID (&sd->election_id, &expected_oid);
      } else {
         fprintf (
            stderr, "ERROR: unparsed field %s\n", bson_iter_key (&server_iter));
         BSON_ASSERT (0);
      }
   }
}

/*
 *-----------------------------------------------------------------------
 *
 * Run the JSON tests from the Server Discovery and Monitoring spec.
 *
 *-----------------------------------------------------------------------
 */
static void
test_sdam_cb (bson_t *test)
{
   mongoc_client_t *client;
   bson_t phase;
   bson_t phases;
   bson_t servers;
   bson_t server;
   bson_t outcome;
   bson_iter_t phase_iter;
   bson_iter_t phase_field_iter;
   bson_iter_t servers_iter;
   bson_iter_t outcome_iter;
   bson_iter_t iter;
   const char *set_name;
   const char *hostname;

   /* parse out the uri and use it to create a client */
   BSON_ASSERT (bson_iter_init_find (&iter, test, "uri"));
   client = mongoc_client_new (bson_iter_utf8 (&iter, NULL));

   /* for each phase, parse and validate */
   BSON_ASSERT (bson_iter_init_find (&iter, test, "phases"));
   bson_iter_bson (&iter, &phases);
   bson_iter_init (&phase_iter, &phases);

   while (bson_iter_next (&phase_iter)) {
      bson_iter_bson (&phase_iter, &phase);

      process_sdam_test_ismaster_responses (&phase,
                                            &client->topology->description);

      /* parse out "outcome" and validate */
      BSON_ASSERT (bson_iter_init_find (&phase_field_iter, &phase, "outcome"));
      bson_iter_bson (&phase_field_iter, &outcome);
      bson_iter_init (&outcome_iter, &outcome);

      while (bson_iter_next (&outcome_iter)) {
         if (strcmp ("servers", bson_iter_key (&outcome_iter)) == 0) {
            bson_iter_bson (&outcome_iter, &servers);
            ASSERT_CMPINT (
               bson_count_keys (&servers),
               ==,
               (int) client->topology->description.servers->items_len);

            bson_iter_init (&servers_iter, &servers);

            /* for each server, ensure topology has a matching entry */
            while (bson_iter_next (&servers_iter)) {
               hostname = bson_iter_key (&servers_iter);
               bson_iter_bson (&servers_iter, &server);

               _topology_has_description (
                  &client->topology->description, &server, hostname);
            }

         } else if (strcmp ("setName", bson_iter_key (&outcome_iter)) == 0) {
            set_name = bson_iter_utf8 (&outcome_iter, NULL);
            if (set_name) {
               BSON_ASSERT (&client->topology->description.set_name);
               ASSERT_CMPSTR (client->topology->description.set_name, set_name);
            }
         } else if (strcmp ("topologyType", bson_iter_key (&outcome_iter)) ==
                    0) {
            ASSERT_CMPSTR (mongoc_topology_description_type (
                              &client->topology->description),
                           bson_iter_utf8 (&outcome_iter, NULL));
         } else {
            fprintf (stderr,
                     "ERROR: unparsed test field %s\n",
                     bson_iter_key (&outcome_iter));
            BSON_ASSERT (false);
         }
      }
   }
   mongoc_client_destroy (client);
}

/*
 *-----------------------------------------------------------------------
 *
 * Runner for the JSON tests for server discovery and monitoring..
 *
 *-----------------------------------------------------------------------
 */
static void
test_all_spec_tests (TestSuite *suite)
{
   char resolved[PATH_MAX];

   /* Single */
   ASSERT (
      realpath (JSON_DIR "/server_discovery_and_monitoring/single", resolved));
   install_json_test_suite (suite, resolved, &test_sdam_cb);

   /* Replica set */
   ASSERT (realpath (JSON_DIR "/server_discovery_and_monitoring/rs", resolved));
   install_json_test_suite (suite, resolved, &test_sdam_cb);

   /* Sharded */
   ASSERT (
      realpath (JSON_DIR "/server_discovery_and_monitoring/sharded", resolved));
   install_json_test_suite (suite, resolved, &test_sdam_cb);

   /* Tests not in official Server Discovery And Monitoring Spec */
   ASSERT (realpath (JSON_DIR "/server_discovery_and_monitoring/supplemental",
                     resolved));
   install_json_test_suite (suite, resolved, &test_sdam_cb);
}

void
test_sdam_install (TestSuite *suite)
{
   test_all_spec_tests (suite);
}
