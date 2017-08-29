/* gcc example-sdam-monitoring.c -o example-sdam-monitoring \
 *     $(pkg-config --cflags --libs libmongoc-1.0) */

/* ./example-sdam-monitoring [CONNECTION_STRING] */

#include <mongoc.h>
#include <stdio.h>


typedef struct {
   int server_changed_events;
   int server_opening_events;
   int server_closed_events;
   int topology_changed_events;
   int topology_opening_events;
   int topology_closed_events;
   int heartbeat_started_events;
   int heartbeat_succeeded_events;
   int heartbeat_failed_events;
} stats_t;


static void
server_changed (const mongoc_apm_server_changed_t *event)
{
   stats_t *context;
   const mongoc_server_description_t *prev_sd, *new_sd;

   context = (stats_t *) mongoc_apm_server_changed_get_context (event);
   context->server_changed_events++;

   prev_sd = mongoc_apm_server_changed_get_previous_description (event);
   new_sd = mongoc_apm_server_changed_get_new_description (event);

   printf ("server changed: %s %s -> %s\n",
           mongoc_apm_server_changed_get_host (event)->host_and_port,
           mongoc_server_description_type (prev_sd),
           mongoc_server_description_type (new_sd));
}


static void
server_opening (const mongoc_apm_server_opening_t *event)
{
   stats_t *context;

   context = (stats_t *) mongoc_apm_server_opening_get_context (event);
   context->server_opening_events++;

   printf ("server opening: %s\n",
           mongoc_apm_server_opening_get_host (event)->host_and_port);
}


static void
server_closed (const mongoc_apm_server_closed_t *event)
{
   stats_t *context;

   context = (stats_t *) mongoc_apm_server_closed_get_context (event);
   context->server_closed_events++;

   printf ("server closed: %s\n",
           mongoc_apm_server_closed_get_host (event)->host_and_port);
}


static void
topology_changed (const mongoc_apm_topology_changed_t *event)
{
   stats_t *context;
   const mongoc_topology_description_t *prev_td;
   const mongoc_topology_description_t *new_td;
   mongoc_server_description_t **prev_sds;
   size_t n_prev_sds;
   mongoc_server_description_t **new_sds;
   size_t n_new_sds;
   size_t i;

   context = (stats_t *) mongoc_apm_topology_changed_get_context (event);
   context->topology_changed_events++;

   prev_td = mongoc_apm_topology_changed_get_previous_description (event);
   prev_sds = mongoc_topology_description_get_servers (prev_td, &n_prev_sds);
   new_td = mongoc_apm_topology_changed_get_new_description (event);
   new_sds = mongoc_topology_description_get_servers (new_td, &n_new_sds);

   printf ("topology changed: %s -> %s\n",
           mongoc_topology_description_type (prev_td),
           mongoc_topology_description_type (new_td));

   if (n_prev_sds) {
      printf ("  previous servers:\n");
      for (i = 0; i < n_prev_sds; i++) {
         printf ("      %s %s\n",
                 mongoc_server_description_type (prev_sds[i]),
                 mongoc_server_description_host (prev_sds[i])->host_and_port);
      }
   }

   if (n_new_sds) {
      printf ("  new servers:\n");
      for (i = 0; i < n_new_sds; i++) {
         printf ("      %s %s\n",
                 mongoc_server_description_type (new_sds[i]),
                 mongoc_server_description_host (new_sds[i])->host_and_port);
      }
   }

   mongoc_server_descriptions_destroy_all (prev_sds, n_prev_sds);
   mongoc_server_descriptions_destroy_all (new_sds, n_new_sds);
}


static void
topology_opening (const mongoc_apm_topology_opening_t *event)
{
   stats_t *context;

   context = (stats_t *) mongoc_apm_topology_opening_get_context (event);
   context->topology_opening_events++;

   printf ("topology opening\n");
}


static void
topology_closed (const mongoc_apm_topology_closed_t *event)
{
   stats_t *context;

   context = (stats_t *) mongoc_apm_topology_closed_get_context (event);
   context->topology_closed_events++;

   printf ("topology closed\n");
}


static void
server_heartbeat_started (const mongoc_apm_server_heartbeat_started_t *event)
{
   stats_t *context;

   context =
      (stats_t *) mongoc_apm_server_heartbeat_started_get_context (event);
   context->heartbeat_started_events++;

   printf ("%s heartbeat started\n",
           mongoc_apm_server_heartbeat_started_get_host (event)->host_and_port);
}


static void
server_heartbeat_succeeded (
   const mongoc_apm_server_heartbeat_succeeded_t *event)
{
   stats_t *context;
   char *reply;

   context =
      (stats_t *) mongoc_apm_server_heartbeat_succeeded_get_context (event);
   context->heartbeat_succeeded_events++;

   reply = bson_as_canonical_extended_json (
      mongoc_apm_server_heartbeat_succeeded_get_reply (event), NULL);

   printf (
      "%s heartbeat succeeded: %s\n",
      mongoc_apm_server_heartbeat_succeeded_get_host (event)->host_and_port,
      reply);

   bson_free (reply);
}


static void
server_heartbeat_failed (const mongoc_apm_server_heartbeat_failed_t *event)
{
   stats_t *context;
   bson_error_t error;

   context = (stats_t *) mongoc_apm_server_heartbeat_failed_get_context (event);
   context->heartbeat_failed_events++;
   mongoc_apm_server_heartbeat_failed_get_error (event, &error);

   printf ("%s heartbeat failed: %s\n",
           mongoc_apm_server_heartbeat_failed_get_host (event)->host_and_port,
           error.message);
}


int
main (int argc, char *argv[])
{
   mongoc_client_t *client;
   mongoc_apm_callbacks_t *cbs;
   stats_t stats = {0};
   const char *uristr = "mongodb://127.0.0.1/?appname=sdam-monitoring-example";
   bson_t cmd = BSON_INITIALIZER;
   bson_t reply;
   bson_error_t error;

   mongoc_init ();

   if (argc > 1) {
      uristr = argv[1];
   }

   client = mongoc_client_new (uristr);

   if (!client) {
      fprintf (stderr, "Failed to parse URI.\n");
      return EXIT_FAILURE;
   }

   mongoc_client_set_error_api (client, 2);
   cbs = mongoc_apm_callbacks_new ();
   mongoc_apm_set_server_changed_cb (cbs, server_changed);
   mongoc_apm_set_server_opening_cb (cbs, server_opening);
   mongoc_apm_set_server_closed_cb (cbs, server_closed);
   mongoc_apm_set_topology_changed_cb (cbs, topology_changed);
   mongoc_apm_set_topology_opening_cb (cbs, topology_opening);
   mongoc_apm_set_topology_closed_cb (cbs, topology_closed);
   mongoc_apm_set_server_heartbeat_started_cb (cbs, server_heartbeat_started);
   mongoc_apm_set_server_heartbeat_succeeded_cb (cbs,
                                                 server_heartbeat_succeeded);
   mongoc_apm_set_server_heartbeat_failed_cb (cbs, server_heartbeat_failed);
   mongoc_client_set_apm_callbacks (
      client, cbs, (void *) &stats /* context pointer */);

   /* the driver connects on demand to perform first operation */
   BSON_APPEND_INT32 (&cmd, "buildinfo", 1);
   mongoc_client_command_simple (client, "admin", &cmd, NULL, &reply, &error);
   mongoc_client_destroy (client);

   printf ("Events:\n"
           "   server changed: %d\n"
           "   server opening: %d\n"
           "   server closed: %d\n"
           "   topology changed: %d\n"
           "   topology opening: %d\n"
           "   topology closed: %d\n"
           "   heartbeat started: %d\n"
           "   heartbeat succeeded: %d\n"
           "   heartbeat failed: %d\n",
           stats.server_changed_events,
           stats.server_opening_events,
           stats.server_closed_events,
           stats.topology_changed_events,
           stats.topology_opening_events,
           stats.topology_closed_events,
           stats.heartbeat_started_events,
           stats.heartbeat_succeeded_events,
           stats.heartbeat_failed_events);

   bson_destroy (&cmd);
   bson_destroy (&reply);
   mongoc_apm_callbacks_destroy (cbs);

   mongoc_cleanup ();

   return EXIT_SUCCESS;
}
