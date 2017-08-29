#include "mongoc-tests.h"

#include "ha-test.h"

#include "mongoc-client-private.h"
#include "mongoc-cursor-private.h"
#include "mongoc-write-concern-private.h"
#include "TestSuite.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "test"


static ha_replica_set_t *replica_set;
static ha_node_t *r1;
static ha_node_t *r2;
static ha_node_t *r3;
static ha_node_t *a1;
static bool use_pool;

static void
insert_test_docs (mongoc_collection_t *collection)
{
   mongoc_write_concern_t *write_concern;
   bson_error_t error;
   bson_oid_t oid;
   bson_t b;
   int i;

   write_concern = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (write_concern, 3);

   {
      bson_t wc = BSON_INITIALIZER;
      char *str;

      mongoc_write_concern_append (write_concern, &wc);
      str = bson_as_canonical_extended_json (&wc, NULL);
      fprintf (stderr, "Write Concern: %s\n", str);
      bson_free (str);
   }

   for (i = 0; i < 200; i++) {
      bson_init (&b);
      bson_oid_init (&oid, NULL);
      bson_append_oid (&b, "_id", 3, &oid);

      ASSERT_OR_PRINT (
         mongoc_collection_insert (
            collection, MONGOC_INSERT_NONE, &b, write_concern, &error),
         error);
      bson_destroy (&b);
   }

   mongoc_write_concern_destroy (write_concern);
}


static ha_node_t *
get_replica (mongoc_client_t *client, uint32_t id)
{
   mongoc_server_description_t *description;
   ha_node_t *iter;

   description = mongoc_topology_server_by_id (client->topology, id, NULL);
   BSON_ASSERT (description);

   for (iter = replica_set->nodes; iter; iter = iter->next) {
      if (iter->port == description->host.port) {
         mongoc_server_description_destroy (description);
         return iter;
      }
   }

   mongoc_server_description_destroy (description);
   BSON_ASSERT (false);
   return NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * test1 --
 *
 *       Tests the failover scenario of a node having a network partition
 *       between the time the client recieves the first OP_REPLY and the
 *       submission of a followup OP_GETMORE.
 *
 *       This function will abort() upon failure.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static void
test1 (void)
{
   mongoc_server_description_t *description;
   mongoc_collection_t *collection;
   mongoc_read_prefs_t *read_prefs;
   mongoc_cursor_t *cursor;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   const bson_t *doc;
   bson_error_t error;
   bool r;
   ha_node_t *replica;
   bson_t q;
   int i;

   bson_init (&q);

   if (use_pool) {
      pool = ha_replica_set_create_client_pool (replica_set);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = ha_replica_set_create_client (replica_set);
   }

   collection = mongoc_client_get_collection (client, "test1", "test1");

   MONGOC_DEBUG ("Inserting test documents.");
   insert_test_docs (collection);
   MONGOC_INFO ("Test documents inserted.");

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);

   MONGOC_DEBUG ("Sending query to a SECONDARY.");
   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 100, &q, NULL, read_prefs);

   BSON_ASSERT (cursor);
   BSON_ASSERT (!cursor->server_id);

   /*
    * Send OP_QUERY to server and get first document back.
    */
   MONGOC_INFO ("Sending OP_QUERY.");
   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (r);
   BSON_ASSERT (cursor->server_id);
   BSON_ASSERT (cursor->sent);
   BSON_ASSERT (!cursor->done);
   BSON_ASSERT (cursor->rpc.reply.n_returned == 100);
   BSON_ASSERT (!cursor->end_of_event);

   /*
    * Make sure we queried a secondary.
    */
   description = mongoc_topology_server_by_id (
      client->topology, cursor->server_id, &error);
   ASSERT_OR_PRINT (description, error);
   BSON_ASSERT (description->type != MONGOC_SERVER_RS_PRIMARY);
   mongoc_server_description_destroy (description);

   /*
    * Exhaust the items in our first OP_REPLY.
    */
   MONGOC_DEBUG ("Exhausting OP_REPLY.");
   for (i = 0; i < 98; i++) {
      r = mongoc_cursor_next (cursor, &doc);
      BSON_ASSERT (r);
      BSON_ASSERT (cursor->server_id);
      BSON_ASSERT (!cursor->done);
      BSON_ASSERT (!cursor->end_of_event);
   }

   /*
    * Finish off the last item in this OP_REPLY.
    */
   MONGOC_INFO ("Fetcing last doc from OP_REPLY.");
   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (r);
   BSON_ASSERT (cursor->server_id);
   BSON_ASSERT (cursor->sent);
   BSON_ASSERT (!cursor->done);
   BSON_ASSERT (!cursor->end_of_event);

   /*
    * Determine which node we queried by using the server_id to
    * get the cluster information.
    */

   BSON_ASSERT (cursor->server_id);
   replica = get_replica (client, cursor->server_id);

   /*
    * Kill the node we are communicating with.
    */
   MONGOC_INFO ("Killing replicaSet node to synthesize failure.");
   ha_node_kill (replica);

   /*
    * Try to fetch the next result set, expect failure.
    */
   MONGOC_DEBUG ("Checking for expected failure.");
   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (!r);

   r = mongoc_cursor_error (cursor, &error);
   BSON_ASSERT (r);
   MONGOC_WARNING ("%s", error.message);

   mongoc_cursor_destroy (cursor);
   mongoc_read_prefs_destroy (read_prefs);
   mongoc_collection_destroy (collection);

   if (use_pool) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }
   bson_destroy (&q);

   ha_node_restart (replica);
}


static void
test2 (void)
{
   mongoc_read_prefs_t *read_prefs;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   const bson_t *doc;
   bson_error_t error;
   bool r;
   bson_t q;

   bson_init (&q);

   /*
    * Start by killing 2 of the replica set nodes.
    */
   ha_node_kill (r1);
   ha_node_kill (r2);

   if (use_pool) {
      pool = ha_replica_set_create_client_pool (replica_set);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = ha_replica_set_create_client (replica_set);
   }

   collection = mongoc_client_get_collection (client, "test2", "test2");

   /*
    * Perform a query and ensure it fails with no nodes available.
    */
   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY_PREFERRED);
   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 100, 0, &q, NULL, read_prefs);

   /*
    * Try to submit OP_QUERY. Since it is SECONDARY PREFERRED, it should
    * succeed if there is any node up (which r3 is up).
    */
   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (!r); /* No docs */

   /* No error, slaveOk was set */
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   mongoc_read_prefs_destroy (read_prefs);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);

   if (use_pool) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   bson_destroy (&q);

   ha_node_restart (r1);
   ha_node_restart (r2);
}


/*
 *--------------------------------------------------------------------------
 *
 * main --
 *
 *       Test various replica-set failure scenarios.
 *
 * Returns:
 *       0 on success; otherwise context specific error code.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

int
main (int argc,     /* IN */
      char *argv[]) /* IN */
{
   mongoc_init ();

   replica_set = ha_replica_set_new ("repltest1");
   r1 = ha_replica_set_add_replica (replica_set, "replica1");
   r2 = ha_replica_set_add_replica (replica_set, "replica2");
   r3 = ha_replica_set_add_replica (replica_set, "replica3");
   a1 = ha_replica_set_add_arbiter (replica_set, "arbiter1");

   ha_replica_set_start (replica_set);

   ha_replica_set_wait_for_healthy (replica_set);
   use_pool = false;
   run_test ("/ReplicaSet/single/lose_node_during_cursor", test1);

   ha_replica_set_wait_for_healthy (replica_set);
   use_pool = true;
   run_test ("/ReplicaSet/pool/lose_node_during_cursor", test1);

   ha_replica_set_wait_for_healthy (replica_set);
   use_pool = false;
   run_test ("/ReplicaSet/single/cursor_with_2_of_3_replicas_down", test2);

   ha_replica_set_wait_for_healthy (replica_set);
   use_pool = true;
   run_test ("/ReplicaSet/pool/cursor_with_2_of_3_replicas_down", test2);

   ha_replica_set_wait_for_healthy (replica_set);

   ha_replica_set_shutdown (replica_set);
   ha_replica_set_destroy (replica_set);

   mongoc_cleanup ();

   return 0;
}
