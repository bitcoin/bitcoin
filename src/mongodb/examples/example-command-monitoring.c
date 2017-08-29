/* gcc example-command-monitoring.c -o example-command-monitoring \
 *     $(pkg-config --cflags --libs libmongoc-1.0) */

/* ./example-command-monitoring [CONNECTION_STRING] */

#include <mongoc.h>
#include <stdio.h>


typedef struct {
   int started;
   int succeeded;
   int failed;
} stats_t;


void
command_started (const mongoc_apm_command_started_t *event)
{
   char *s;

   s = bson_as_canonical_extended_json (
      mongoc_apm_command_started_get_command (event), NULL);
   printf ("Command %s started on %s:\n%s\n\n",
           mongoc_apm_command_started_get_command_name (event),
           mongoc_apm_command_started_get_host (event)->host,
           s);

   ((stats_t *) mongoc_apm_command_started_get_context (event))->started++;

   bson_free (s);
}


void
command_succeeded (const mongoc_apm_command_succeeded_t *event)
{
   char *s;

   s = bson_as_canonical_extended_json (
      mongoc_apm_command_succeeded_get_reply (event), NULL);
   printf ("Command %s succeeded:\n%s\n\n",
           mongoc_apm_command_succeeded_get_command_name (event),
           s);

   ((stats_t *) mongoc_apm_command_succeeded_get_context (event))->succeeded++;

   bson_free (s);
}


void
command_failed (const mongoc_apm_command_failed_t *event)
{
   bson_error_t error;

   mongoc_apm_command_failed_get_error (event, &error);
   printf ("Command %s failed:\n\"%s\"\n\n",
           mongoc_apm_command_failed_get_command_name (event),
           error.message);

   ((stats_t *) mongoc_apm_command_failed_get_context (event))->failed++;
}


int
main (int argc, char *argv[])
{
   mongoc_client_t *client;
   mongoc_apm_callbacks_t *callbacks;
   stats_t stats = {0};
   mongoc_collection_t *collection;
   const char *uristr = "mongodb://127.0.0.1/?appname=cmd-monitoring-example";
   const char *collection_name = "test";
   bson_t doc;

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
   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, command_started);
   mongoc_apm_set_command_succeeded_cb (callbacks, command_succeeded);
   mongoc_apm_set_command_failed_cb (callbacks, command_failed);
   mongoc_client_set_apm_callbacks (
      client, callbacks, (void *) &stats /* context pointer */);

   bson_init (&doc);
   BSON_APPEND_INT32 (&doc, "_id", 1);

   collection = mongoc_client_get_collection (client, "test", collection_name);
   mongoc_collection_drop (collection, NULL);
   mongoc_collection_insert (collection, MONGOC_INSERT_NONE, &doc, NULL, NULL);
   /* duplicate key error on the second insert */
   mongoc_collection_insert (collection, MONGOC_INSERT_NONE, &doc, NULL, NULL);

   printf ("started: %d\nsucceeded: %d\nfailed: %d\n",
           stats.started,
           stats.succeeded,
           stats.failed);

   bson_destroy (&doc);
   mongoc_collection_destroy (collection);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   return EXIT_SUCCESS;
}
