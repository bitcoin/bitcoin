#include <string.h>

#include "ha-test.h"

#include "mongoc-client-private.h"
#include "mongoc-tests.h"
#include "mongoc-write-concern-private.h"
#include "TestSuite.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "test"

static char *gTestCAFile;
static char *gTestPEMFileLocalhost;
static bool use_pool;

static void
test_replica_set_ssl_client (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   ha_replica_set_t *replica_set;
   bson_error_t error;
   bson_t b;

   mongoc_ssl_opt_t sopt = {0};

   sopt.pem_file = gTestPEMFileLocalhost;
   sopt.ca_file = gTestCAFile;

   replica_set = ha_replica_set_new ("repltest1");
   ha_replica_set_ssl (replica_set, &sopt);
   ha_replica_set_add_replica (replica_set, "replica1");
   ha_replica_set_add_replica (replica_set, "replica2");
   ha_replica_set_add_replica (replica_set, "replica3");

   ha_replica_set_start (replica_set);
   ha_replica_set_wait_for_healthy (replica_set);

   if (use_pool) {
      pool = ha_replica_set_create_client_pool (replica_set);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = ha_replica_set_create_client (replica_set);
   }

   BSON_ASSERT (client);

   collection = mongoc_client_get_collection (client, "test", "test");
   BSON_ASSERT (collection);

   bson_init (&b);
   bson_append_utf8 (&b, "hello", -1, "world", -1);

   ASSERT_OR_PRINT (mongoc_collection_insert (
                       collection, MONGOC_INSERT_NONE, &b, NULL, &error),
                    error);

   mongoc_collection_destroy (collection);

   if (use_pool) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   bson_destroy (&b);

   ha_replica_set_shutdown (replica_set);
   ha_replica_set_destroy (replica_set);
}


static void
log_handler (mongoc_log_level_t log_level,
             const char *domain,
             const char *message,
             void *user_data)
{
   /* Do Nothing */
}


int
main (int argc,     /* IN */
      char *argv[]) /* IN */
{
   char *cwd;
   char buf[1024];

   if (argc <= 1 || !!strcmp (argv[1], "-v")) {
      mongoc_log_set_handler (log_handler, NULL);
   }

   mongoc_init ();

   cwd = getcwd (buf, sizeof (buf));
   BSON_ASSERT (cwd);

   gTestCAFile = bson_strdup_printf ("%s/" CERT_CA, cwd);
   gTestPEMFileLocalhost = bson_strdup_printf ("%s/" CERT_SERVER, cwd);

   use_pool = false;
   run_test ("/ReplicaSet/single/ssl/client", &test_replica_set_ssl_client);

   use_pool = true;
   run_test ("/ReplicaSet/pool/ssl/client", &test_replica_set_ssl_client);

   bson_free (gTestCAFile);
   bson_free (gTestPEMFileLocalhost);

   mongoc_cleanup ();

   return 0;
}
