#include "mongoc.h"
#include "mongoc-set-private.h"
#include "mongoc-client-pool-private.h"
#include "mongoc-client-private.h"

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "test-conveniences.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "topology-test"

static void
_test_has_readable_writable_server (bool pooled)
{
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   mongoc_topology_description_t *td;
   mongoc_read_prefs_t *prefs;
   bool r;
   bson_error_t error;

   if (pooled) {
      pool = test_framework_client_pool_new ();
      client = mongoc_client_pool_pop (pool);
      td = &_mongoc_client_pool_get_topology (pool)->description;
   } else {
      client = test_framework_client_new ();
      td = &client->topology->description;
   }

   prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   mongoc_read_prefs_set_tags (prefs, tmp_bson ("[{'tag': 'does-not-exist'}]"));

   /* not yet connected */
   ASSERT (!mongoc_topology_description_has_writable_server (td));
   ASSERT (!mongoc_topology_description_has_readable_server (td, NULL));
   ASSERT (!mongoc_topology_description_has_readable_server (td, prefs));

   /* trigger connection */
   r = mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);
   ASSERT_OR_PRINT (r, error);

   ASSERT (mongoc_topology_description_has_writable_server (td));
   ASSERT (mongoc_topology_description_has_readable_server (td, NULL));

   if (test_framework_is_replset ()) {
      /* prefs still don't match any server */
      ASSERT (!mongoc_topology_description_has_readable_server (td, prefs));
   } else {
      /* topology type single ignores read preference */
      ASSERT (mongoc_topology_description_has_readable_server (td, prefs));
   }

   mongoc_read_prefs_destroy (prefs);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }
}


static void
test_has_readable_writable_server_single (void)
{
   _test_has_readable_writable_server (false);
}


static void
test_has_readable_writable_server_pooled (void)
{
   _test_has_readable_writable_server (true);
}


static mongoc_server_description_t *
_sd_for_host (mongoc_topology_description_t *td, const char *host)
{
   int i;
   mongoc_server_description_t *sd;

   for (i = 0; i < (int) td->servers->items_len; i++) {
      sd = (mongoc_server_description_t *) mongoc_set_get_item (td->servers, i);

      if (!strcmp (sd->host.host, host)) {
         return sd;
      }
   }

   return NULL;
}


static void
test_get_servers (void)
{
   mongoc_uri_t *uri;
   mongoc_topology_t *topology;
   mongoc_topology_description_t *td;
   mongoc_server_description_t *sd_a;
   mongoc_server_description_t *sd_c;
   mongoc_server_description_t **sds;
   size_t n;

   uri = mongoc_uri_new ("mongodb://a,b,c");
   topology = mongoc_topology_new (uri, true /* single-threaded */);
   td = &topology->description;

   /* servers "a" and "c" are mongos, but "b" remains unknown */
   sd_a = _sd_for_host (td, "a");
   mongoc_topology_description_handle_ismaster (
      td, sd_a->id, tmp_bson ("{'ok': 1, 'msg': 'isdbgrid'}"), 100, NULL);

   sd_c = _sd_for_host (td, "c");
   mongoc_topology_description_handle_ismaster (
      td, sd_c->id, tmp_bson ("{'ok': 1, 'msg': 'isdbgrid'}"), 100, NULL);

   sds = mongoc_topology_description_get_servers (td, &n);
   ASSERT_CMPSIZE_T ((size_t) 2, ==, n);

   /* we don't care which order the servers are returned */
   if (sds[0]->id == sd_a->id) {
      ASSERT_CMPSTR ("a", sds[0]->host.host);
      ASSERT_CMPSTR ("c", sds[1]->host.host);
   } else {
      ASSERT_CMPSTR ("c", sds[0]->host.host);
      ASSERT_CMPSTR ("a", sds[1]->host.host);
   }

   mongoc_server_descriptions_destroy_all (sds, n);
   mongoc_topology_destroy (topology);
   mongoc_uri_destroy (uri);
}


void
test_topology_description_install (TestSuite *suite)
{
   TestSuite_AddLive (suite,
                      "/TopologyDescription/readable_writable/single",
                      test_has_readable_writable_server_single);
   TestSuite_AddLive (suite,
                      "/TopologyDescription/readable_writable/pooled",
                      test_has_readable_writable_server_pooled);
   TestSuite_Add (suite, "/TopologyDescription/get_servers", test_get_servers);
}
