#include "mongoc-tests.h"

#include <mongoc.h>

#include "ha-test.h"

static ha_sharded_cluster_t *cluster;
static ha_replica_set_t *repl_1;
static ha_replica_set_t *repl_2;
static ha_node_t *node_1_1;
static ha_node_t *node_1_2;
static ha_node_t *node_1_3;
static ha_node_t *node_2_1;
static ha_node_t *node_2_2;
static ha_node_t *node_2_3;

static void
test1 (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error = {0};
   bool r;
   bson_t q = BSON_INITIALIZER;
   int i;

   BSON_ASSERT (cluster);

   bson_append_utf8 (&q, "hello", -1, "world", -1);

   client = ha_sharded_cluster_get_client (cluster);
   collection = mongoc_client_get_collection (client, "test", "test");

   for (i = 0; i < 100; i++) {
      r = mongoc_collection_insert (
         collection, MONGOC_INSERT_NONE, &q, NULL, &error);
      BSON_ASSERT (r);
      BSON_ASSERT (!error.domain);
      BSON_ASSERT (!error.code);
   }

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_destroy (&q);
}

int
main (int argc, char *argv[])
{
   mongoc_init ();

   repl_1 = ha_replica_set_new ("shardtest1");
   node_1_1 = ha_replica_set_add_replica (repl_1, "shardtest1_1");
   node_1_2 = ha_replica_set_add_replica (repl_1, "shardtest1_2");
   node_1_3 = ha_replica_set_add_replica (repl_1, "shardtest1_3");

   repl_2 = ha_replica_set_new ("shardtest2");
   node_2_1 = ha_replica_set_add_replica (repl_2, "shardtest2_1");
   node_2_2 = ha_replica_set_add_replica (repl_2, "shardtest2_2");
   node_2_3 = ha_replica_set_add_replica (repl_2, "shardtest2_3");

   cluster = ha_sharded_cluster_new ("cluster1");
   ha_sharded_cluster_add_replica_set (cluster, repl_1);
   ha_sharded_cluster_add_replica_set (cluster, repl_2);
   ha_sharded_cluster_add_config (cluster, "config1");
   ha_sharded_cluster_add_router (cluster, "router1");
   ha_sharded_cluster_add_router (cluster, "router2");

   ha_sharded_cluster_start (cluster);
   ha_sharded_cluster_wait_for_healthy (cluster);

   run_test ("/ShardedCluster/basic", test1);

   ha_sharded_cluster_shutdown (cluster);

   mongoc_cleanup ();

   return 0;
}
