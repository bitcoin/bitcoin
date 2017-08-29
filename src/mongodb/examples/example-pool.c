/* gcc example-pool.c -o example-pool $(pkg-config --cflags --libs
 * libmongoc-1.0) */

/* ./example-pool [CONNECTION_STRING] */

#include <mongoc.h>
#include <pthread.h>
#include <stdio.h>

static pthread_mutex_t mutex;
static bool in_shutdown = false;

static void *
worker (void *data)
{
   mongoc_client_pool_t *pool = data;
   mongoc_client_t *client;
   bson_t ping = BSON_INITIALIZER;
   bson_error_t error;
   bool r;

   BSON_APPEND_INT32 (&ping, "ping", 1);

   while (true) {
      client = mongoc_client_pool_pop (pool);
      /* Do something with client. If you are writing an HTTP server, you
       * probably only want to hold onto the client for the portion of the
       * request performing database queries.
       */
      r = mongoc_client_command_simple (
         client, "admin", &ping, NULL, NULL, &error);

      if (!r) {
         fprintf (stderr, "%s\n", error.message);
      }

      mongoc_client_pool_push (pool, client);

      pthread_mutex_lock (&mutex);
      if (in_shutdown || !r) {
         pthread_mutex_unlock (&mutex);
         break;
      }

      pthread_mutex_unlock (&mutex);
   }

   bson_destroy (&ping);
   return NULL;
}

int
main (int argc, char *argv[])
{
   const char *uristr = "mongodb://127.0.0.1/?appname=pool-example";
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool;
   pthread_t threads[10];
   unsigned i;
   void *ret;

   pthread_mutex_init (&mutex, NULL);
   mongoc_init ();

   if (argc > 1) {
      uristr = argv[1];
   }

   uri = mongoc_uri_new (uristr);
   if (!uri) {
      fprintf (stderr, "Failed to parse URI: \"%s\".\n", uristr);
      return EXIT_FAILURE;
   }

   pool = mongoc_client_pool_new (uri);
   mongoc_client_pool_set_error_api (pool, 2);

   for (i = 0; i < 10; i++) {
      pthread_create (&threads[i], NULL, worker, pool);
   }

   sleep (10);
   pthread_mutex_lock (&mutex);
   in_shutdown = true;
   pthread_mutex_unlock (&mutex);

   for (i = 0; i < 10; i++) {
      pthread_join (threads[i], &ret);
   }

   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);

   mongoc_cleanup ();

   return 0;
}
