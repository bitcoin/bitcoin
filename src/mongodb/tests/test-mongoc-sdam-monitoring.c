#include <mongoc.h>
#include <mongoc-set-private.h>
#include <mongoc-topology-description-apm-private.h>

#include "json-test.h"

#include "mongoc-client-private.h"
#include "test-libmongoc.h"
#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif


typedef struct {
   bson_t events;
   uint32_t n_events;
   bson_oid_t topology_id;
} context_t;

static void
context_init (context_t *context)
{
   bson_init (&context->events);
   context->n_events = 0;
   bson_oid_init_from_string (&context->topology_id,
                              "000000000000000000000000");
}

static void
context_append (context_t *context, bson_t *event)
{
   char str[16];
   const char *key;

   bson_uint32_to_string (context->n_events, &key, str, sizeof str);
   BSON_APPEND_DOCUMENT (&context->events, key, event);

   context->n_events++;

   bson_destroy (event);
}

static void
context_destroy (context_t *context)
{
   bson_destroy (&context->events);
}

static void
append_array (bson_t *bson, const char *key, const bson_t *array)
{
   if (array->len) {
      BSON_APPEND_ARRAY (bson, key, array);
   } else {
      bson_t tmp = BSON_INITIALIZER;
      BSON_APPEND_ARRAY (bson, key, &tmp);
      bson_destroy (&tmp);
   }
}

static void
sd_to_bson (const mongoc_server_description_t *sd, bson_t *bson)
{
   const mongoc_host_list_t *host_list;

   host_list = mongoc_server_description_host (sd);

   bson_init (bson);
   BSON_APPEND_UTF8 (bson, "address", host_list->host_and_port);

   append_array (bson, "arbiters", &sd->arbiters);
   append_array (bson, "hosts", &sd->hosts);
   append_array (bson, "passives", &sd->passives);

   if (sd->current_primary) {
      BSON_APPEND_UTF8 (bson, "primary", sd->current_primary);
   }

   if (sd->set_name) {
      BSON_APPEND_UTF8 (bson, "setName", sd->set_name);
   }

   BSON_APPEND_UTF8 (bson, "type", mongoc_server_description_type (sd));
}

static void
td_to_bson (const mongoc_topology_description_t *td, bson_t *bson)
{
   size_t i;
   bson_t servers = BSON_INITIALIZER;
   bson_t server;
   char str[16];
   const char *key;

   for (i = 0; i < td->servers->items_len; i++) {
      bson_uint32_to_string ((uint32_t) i, &key, str, sizeof str);
      sd_to_bson (mongoc_set_get_item (td->servers, (int) i), &server);
      BSON_APPEND_DOCUMENT (&servers, key, &server);
      bson_destroy (&server);
   }

   bson_init (bson);
   BSON_APPEND_UTF8 (
      bson, "topologyType", mongoc_topology_description_type (td));

   if (td->set_name) {
      BSON_APPEND_UTF8 (bson, "setName", td->set_name);
   }

   BSON_APPEND_ARRAY (bson, "servers", &servers);

   bson_destroy (&servers);
}

static void
server_changed (const mongoc_apm_server_changed_t *event)
{
   context_t *ctx;
   bson_oid_t topology_id;
   const char *host_and_port;
   bson_t prev_sd;
   bson_t new_sd;

   ctx = (context_t *) mongoc_apm_server_changed_get_context (event);

   /* check topology id is consistent */
   mongoc_apm_server_changed_get_topology_id (event, &topology_id);
   ASSERT (bson_oid_equal (&topology_id, &ctx->topology_id));

   host_and_port = mongoc_apm_server_changed_get_host (event)->host_and_port;
   sd_to_bson (mongoc_apm_server_changed_get_previous_description (event),
               &prev_sd);
   sd_to_bson (mongoc_apm_server_changed_get_new_description (event), &new_sd);

   context_append (ctx,
                   BCON_NEW ("server_description_changed_event",
                             "{",
                             "topologyId",
                             BCON_UTF8 ("42"),
                             "address",
                             BCON_UTF8 (host_and_port),
                             "previousDescription",
                             BCON_DOCUMENT (&prev_sd),
                             "newDescription",
                             BCON_DOCUMENT (&new_sd),
                             "}"));

   bson_destroy (&prev_sd);
   bson_destroy (&new_sd);
}

static void
server_opening (const mongoc_apm_server_opening_t *event)
{
   context_t *ctx;
   bson_oid_t topology_id;
   const char *host_and_port;

   ctx = (context_t *) mongoc_apm_server_opening_get_context (event);

   mongoc_apm_server_opening_get_topology_id (event, &topology_id);
   ASSERT (bson_oid_equal (&topology_id, &ctx->topology_id));

   host_and_port = mongoc_apm_server_opening_get_host (event)->host_and_port;
   context_append (ctx,
                   BCON_NEW ("server_opening_event",
                             "{",
                             "address",
                             BCON_UTF8 (host_and_port),
                             "topologyId",
                             BCON_UTF8 ("42"),
                             "}"));
}

static void
server_closed (const mongoc_apm_server_closed_t *event)
{
   context_t *ctx;
   bson_oid_t topology_id;
   const char *host_and_port;

   ctx = (context_t *) mongoc_apm_server_closed_get_context (event);

   mongoc_apm_server_closed_get_topology_id (event, &topology_id);
   ASSERT (bson_oid_equal (&topology_id, &ctx->topology_id));

   host_and_port = mongoc_apm_server_closed_get_host (event)->host_and_port;
   context_append (ctx,
                   BCON_NEW ("server_closed_event",
                             "{",
                             "address",
                             BCON_UTF8 (host_and_port),
                             "topologyId",
                             BCON_UTF8 ("42"),
                             "}"));
}

static void
topology_changed (const mongoc_apm_topology_changed_t *event)
{
   context_t *ctx;
   bson_oid_t topology_id;
   bson_t prev_td;
   bson_t new_td;

   ctx = (context_t *) mongoc_apm_topology_changed_get_context (event);

   mongoc_apm_topology_changed_get_topology_id (event, &topology_id);
   ASSERT (bson_oid_equal (&topology_id, &ctx->topology_id));

   td_to_bson (mongoc_apm_topology_changed_get_previous_description (event),
               &prev_td);
   td_to_bson (mongoc_apm_topology_changed_get_new_description (event),
               &new_td);

   context_append (ctx,
                   BCON_NEW ("topology_description_changed_event",
                             "{",
                             "newDescription",
                             BCON_DOCUMENT (&new_td),
                             "previousDescription",
                             BCON_DOCUMENT (&prev_td),
                             "topologyId",
                             BCON_UTF8 ("42"),
                             "}"));

   bson_destroy (&prev_td);
   bson_destroy (&new_td);
}

static void
topology_opening (const mongoc_apm_topology_opening_t *event)
{
   context_t *ctx;
   bson_oid_t zeroes;

   /* new event's topology id is NOT all zeroes */
   bson_oid_init_from_string (&zeroes, "000000000000000000000000");
   ASSERT (!bson_oid_equal (&event->topology_id, &zeroes));

   ctx = (context_t *) mongoc_apm_topology_opening_get_context (event);
   mongoc_apm_topology_opening_get_topology_id (event, &ctx->topology_id);
   context_append (
      ctx,
      BCON_NEW (
         "topology_opening_event", "{", "topologyId", BCON_UTF8 ("42"), "}"));
}

static void
topology_closed (const mongoc_apm_topology_closed_t *event)
{
   context_t *ctx;
   bson_oid_t topology_id;

   ctx = (context_t *) mongoc_apm_topology_closed_get_context (event);
   mongoc_apm_topology_closed_get_topology_id (event, &topology_id);
   ASSERT (bson_oid_equal (&topology_id, &ctx->topology_id));
   context_append (
      ctx,
      BCON_NEW (
         "topology_closed_event", "{", "topologyId", BCON_UTF8 ("42"), "}"));
}

/* no standard tests in the specs repo for heartbeat events, so invent some */
static void
server_heartbeat_started (const mongoc_apm_server_heartbeat_started_t *event)
{
   context_t *ctx;
   const mongoc_host_list_t *host;

   ctx = (context_t *) mongoc_apm_server_heartbeat_started_get_context (event);
   host = mongoc_apm_server_heartbeat_started_get_host (event);
   context_append (ctx,
                   BCON_NEW ("heartbeat_started_event",
                             "{",
                             "host",
                             BCON_UTF8 (host->host_and_port),
                             "}"));
}

static void
server_heartbeat_succeeded (
   const mongoc_apm_server_heartbeat_succeeded_t *event)
{
   context_t *ctx;
   const mongoc_host_list_t *host;

   ctx =
      (context_t *) mongoc_apm_server_heartbeat_succeeded_get_context (event);
   host = mongoc_apm_server_heartbeat_succeeded_get_host (event);
   context_append (ctx,
                   BCON_NEW ("heartbeat_succeeded_event",
                             "{",
                             "host",
                             BCON_UTF8 (host->host_and_port),
                             "}"));
}

static void
server_heartbeat_failed (const mongoc_apm_server_heartbeat_failed_t *event)
{
   context_t *ctx;
   const mongoc_host_list_t *host;

   ctx = (context_t *) mongoc_apm_server_heartbeat_failed_get_context (event);
   host = mongoc_apm_server_heartbeat_failed_get_host (event);
   context_append (ctx,
                   BCON_NEW ("heartbeat_failed_event",
                             "{",
                             "host",
                             BCON_UTF8 (host->host_and_port),
                             "}"));
}

static mongoc_apm_callbacks_t *
topology_event_callbacks (void)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_server_changed_cb (callbacks, server_changed);
   mongoc_apm_set_server_opening_cb (callbacks, server_opening);
   mongoc_apm_set_server_closed_cb (callbacks, server_closed);
   mongoc_apm_set_topology_changed_cb (callbacks, topology_changed);
   mongoc_apm_set_topology_opening_cb (callbacks, topology_opening);
   mongoc_apm_set_topology_closed_cb (callbacks, topology_closed);

   return callbacks;
}

static void
client_set_topology_event_callbacks (mongoc_client_t *client,
                                     context_t *context)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = topology_event_callbacks ();
   mongoc_client_set_apm_callbacks (client, callbacks, (void *) context);
   mongoc_apm_callbacks_destroy (callbacks);
}

static void
pool_set_topology_event_callbacks (mongoc_client_pool_t *pool,
                                   context_t *context)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = topology_event_callbacks ();
   mongoc_client_pool_set_apm_callbacks (pool, callbacks, (void *) context);
   mongoc_apm_callbacks_destroy (callbacks);
}

static mongoc_apm_callbacks_t *
heartbeat_event_callbacks (void)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_server_heartbeat_started_cb (callbacks,
                                               server_heartbeat_started);
   mongoc_apm_set_server_heartbeat_succeeded_cb (callbacks,
                                                 server_heartbeat_succeeded);
   mongoc_apm_set_server_heartbeat_failed_cb (callbacks,
                                              server_heartbeat_failed);

   return callbacks;
}

static void
client_set_heartbeat_event_callbacks (mongoc_client_t *client,
                                      context_t *context)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = heartbeat_event_callbacks ();
   mongoc_client_set_apm_callbacks (client, callbacks, (void *) context);
   mongoc_apm_callbacks_destroy (callbacks);
}

static void
pool_set_heartbeat_event_callbacks (mongoc_client_pool_t *pool,
                                    context_t *context)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = heartbeat_event_callbacks ();
   mongoc_client_pool_set_apm_callbacks (pool, callbacks, (void *) context);
   mongoc_apm_callbacks_destroy (callbacks);
}

/*
 *-----------------------------------------------------------------------
 *
 * Run the JSON tests from the SDAM Monitoring spec.
 *
 *-----------------------------------------------------------------------
 */
static void
test_sdam_monitoring_cb (bson_t *test)
{
   mongoc_client_t *client;
   mongoc_topology_t *topology;
   bson_t phase;
   bson_t phases;
   bson_t outcome;
   bson_iter_t phase_iter;
   bson_iter_t phase_field_iter;
   bson_iter_t outcome_iter;
   bson_iter_t iter;
   bson_t events_expected;
   context_t context;

   /* parse out the uri and use it to create a client */
   BSON_ASSERT (bson_iter_init_find (&iter, test, "uri"));
   client = mongoc_client_new (bson_iter_utf8 (&iter, NULL));
   topology = client->topology;
   context_init (&context);
   client_set_topology_event_callbacks (client, &context);

   /* for each phase, parse and validate */
   BSON_ASSERT (bson_iter_init_find (&iter, test, "phases"));
   bson_iter_bson (&iter, &phases);
   bson_iter_init (&phase_iter, &phases);

   while (bson_iter_next (&phase_iter)) {
      bson_iter_bson (&phase_iter, &phase);

      /* this test doesn't exercise this code path naturally, see below in
       * _test_topology_events for a non-hacky test of this event */
      _mongoc_topology_description_monitor_opening (&topology->description);
      process_sdam_test_ismaster_responses (&phase,
                                            &client->topology->description);

      /* parse out "outcome" and validate */
      BSON_ASSERT (bson_iter_init_find (&phase_field_iter, &phase, "outcome"));
      bson_iter_bson (&phase_field_iter, &outcome);
      bson_iter_init (&outcome_iter, &outcome);

      while (bson_iter_next (&outcome_iter)) {
         if (strcmp ("events", bson_iter_key (&outcome_iter)) == 0) {
            bson_iter_bson (&outcome_iter, &events_expected);
            check_json_apm_events (&context.events, &events_expected);
         } else {
            fprintf (stderr,
                     "ERROR: unparsed test field %s\n",
                     bson_iter_key (&outcome_iter));
            BSON_ASSERT (false);
         }
      }
   }

   mongoc_client_destroy (client);
   context_destroy (&context);
}

/*
 *-----------------------------------------------------------------------
 *
 * Runner for the JSON tests for SDAM Monitoring..
 *
 *-----------------------------------------------------------------------
 */
static void
test_all_spec_tests (TestSuite *suite)
{
   char resolved[PATH_MAX];

   ASSERT (realpath (JSON_DIR "/server_discovery_and_monitoring/monitoring",
                     resolved));

   install_json_test_suite (suite, resolved, &test_sdam_monitoring_cb);
}

static void
_test_topology_events (bool pooled)
{
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   context_t context;
   bool r;
   bson_error_t error;
   bson_iter_t events_iter;
   bson_iter_t event_iter;
   uint32_t i;

   context_init (&context);

   if (pooled) {
      pool = test_framework_client_pool_new ();
      pool_set_topology_event_callbacks (pool, &context);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = test_framework_client_new ();
      client_set_topology_event_callbacks (client, &context);
   }

   r = mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);
   ASSERT_OR_PRINT (r, error);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   /* first event is topology opening */
   bson_iter_init (&events_iter, &context.events);
   bson_iter_next (&events_iter);
   ASSERT (bson_iter_recurse (&events_iter, &event_iter));
   ASSERT (bson_iter_find (&event_iter, "topology_opening_event"));

   /* last event is topology closed */
   for (i = 1; i < context.n_events; i++) {
      ASSERT (bson_iter_next (&events_iter));
   }

   ASSERT (bson_iter_recurse (&events_iter, &event_iter));
   ASSERT (bson_iter_find (&event_iter, "topology_closed_event"));

   /* no more events */
   ASSERT (!bson_iter_next (&events_iter));

   context_destroy (&context);
}

static void
test_topology_events_single (void)
{
   _test_topology_events (false);
}

static void
test_topology_events_pooled (void)
{
   _test_topology_events (true);
}

static bool
responder (request_t *request, void *data)
{
   if (!strcmp (request->command_name, "foo")) {
      mock_server_replies_simple (request, "{'ok': 1}");
      request_destroy (request);
      return true;
   }

   return false;
}

static void
_test_heartbeat_events (bool pooled, bool succeeded)
{
   context_t context;
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   future_t *future;
   request_t *request;
   char *expected_json;
   bson_error_t error;

   context_init (&context);

   /* auto-respond to "foo" command */
   server = mock_server_new ();
   mock_server_run (server);
   mock_server_autoresponds (server, responder, NULL, NULL);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, "serverSelectionTimeoutMS", 400);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      pool_set_heartbeat_event_callbacks (pool, &context);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
      client_set_heartbeat_event_callbacks (client, &context);
   }

   /* trigger "ismaster" handshake */
   future = future_client_command_simple (
      client, "admin", tmp_bson ("{'foo': 1}"), NULL, NULL, &error);

   /* topology scanner calls ismaster once */
   request = mock_server_receives_ismaster (server);

   if (succeeded) {
      mock_server_replies_ok_and_destroys (request);
   } else {
      mock_server_hangs_up (request);
      request_destroy (request);
   }

   /* pooled client opens new socket, handshakes it by calling ismaster again */
   if (pooled && succeeded) {
      request = mock_server_receives_ismaster (server);
      mock_server_replies_ok_and_destroys (request);
   }

   if (succeeded) {
      /* "foo" command succeeds */
      ASSERT_OR_PRINT (future_get_bool (future), error);
   } else {
      ASSERT (!future_get_bool (future));
   }

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   /* even if pooled, only topology scanner sends events, so we get one pair */
   if (succeeded) {
      expected_json = bson_strdup_printf (
         "{'0': {'heartbeat_started_event': {'host': '%s'}},"
         " '1': {'heartbeat_succeeded_event': {'host': '%s'}}}",
         mock_server_get_host_and_port (server),
         mock_server_get_host_and_port (server));
   } else {
      expected_json = bson_strdup_printf (
         "{'0': {'heartbeat_started_event': {'host': '%s'}},"
         " '1': {'heartbeat_failed_event': {'host': '%s'}}}",
         mock_server_get_host_and_port (server),
         mock_server_get_host_and_port (server));
   }

   check_json_apm_events (&context.events, tmp_bson (expected_json));

   future_destroy (future);
   bson_free (expected_json);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
   context_destroy (&context);
}

static void
test_heartbeat_events_single_succeeded (void)
{
   _test_heartbeat_events (false, true);
}

static void
test_heartbeat_events_pooled_succeeded (void)
{
   _test_heartbeat_events (true, true);
}


static void
test_heartbeat_events_single_failed (void)
{
   _test_heartbeat_events (false, false);
}

static void
test_heartbeat_events_pooled_failed (void)
{
   _test_heartbeat_events (true, false);
}


void
test_sdam_monitoring_install (TestSuite *suite)
{
   test_all_spec_tests (suite);
   TestSuite_AddLive (
      suite,
      "/server_discovery_and_monitoring/monitoring/topology/single",
      test_topology_events_single);
   TestSuite_AddLive (
      suite,
      "/server_discovery_and_monitoring/monitoring/topology/pooled",
      test_topology_events_pooled);
   TestSuite_AddMockServerTest (
      suite,
      "/server_discovery_and_monitoring/monitoring/heartbeat/single/succeeded",
      test_heartbeat_events_single_succeeded);
   TestSuite_AddMockServerTest (
      suite,
      "/server_discovery_and_monitoring/monitoring/heartbeat/single/failed",
      test_heartbeat_events_single_failed);
   TestSuite_AddMockServerTest (
      suite,
      "/server_discovery_and_monitoring/monitoring/heartbeat/pooled/succeeded",
      test_heartbeat_events_pooled_succeeded);
   TestSuite_AddMockServerTest (
      suite,
      "/server_discovery_and_monitoring/monitoring/heartbeat/pooled/failed",
      test_heartbeat_events_pooled_failed);
}
