/*
 * Copyright 2013 MongoDB, Inc.
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


#ifndef HA_TEST_H
#define HA_TEST_H


#include <bson.h>
#include <mongoc.h>
#include <mongoc-thread-private.h>


BSON_BEGIN_DECLS


typedef struct _ha_sharded_cluster_t ha_sharded_cluster_t;
typedef struct _ha_replica_set_t ha_replica_set_t;
typedef struct _ha_node_t ha_node_t;


struct _ha_sharded_cluster_t {
   char *name;
   ha_replica_set_t *replicas[12];
   ha_node_t *configs;
   ha_node_t *routers;
   int next_port;

#ifdef MONGOC_ENABLE_SSL
   mongoc_ssl_opt_t *ssl_opt;
#endif
};


struct _ha_replica_set_t {
   char *name;
   ha_node_t *nodes;
   int next_port;

#ifdef MONGOC_ENABLE_SSL
   mongoc_ssl_opt_t *ssl_opt;
#endif
};

#ifdef BSON_OS_WIN32
typedef struct {
   bool is_alive;
   HANDLE proc;
   HANDLE thread;
} bson_ha_pid_t;
#else
typedef pid_t bson_ha_pid_t;
#endif


struct _ha_node_t {
   ha_node_t *next;
   char *name;
   char *repl_set;
   char *dbpath;
   char *configopt;
   bool is_arbiter : 1;
   bool is_config : 1;
   bool is_router : 1;
   bson_ha_pid_t pid;
   uint16_t port;

#ifdef MONGOC_ENABLE_SSL
   mongoc_ssl_opt_t *ssl_opt;
#endif
};


ha_replica_set_t *
ha_replica_set_new (const char *name);
ha_node_t *
ha_replica_set_add_arbiter (ha_replica_set_t *replica_set, const char *name);
ha_node_t *
ha_replica_set_add_replica (ha_replica_set_t *replica_set, const char *name);
mongoc_client_t *
ha_replica_set_create_client (ha_replica_set_t *replica_set);
mongoc_client_pool_t *
ha_replica_set_create_client_pool (ha_replica_set_t *replica_set);
void
ha_replica_set_start (ha_replica_set_t *replica_set);
void
ha_replica_set_shutdown (ha_replica_set_t *replica_set);
void
ha_replica_set_destroy (ha_replica_set_t *replica_set);
void
ha_replica_set_wait_for_healthy (ha_replica_set_t *replica_set);
#ifdef MONGOC_ENABLE_SSL
void
ha_replica_set_ssl (ha_replica_set_t *repl_set, mongoc_ssl_opt_t *opt);
#endif


void
ha_node_kill (ha_node_t *node);
void
ha_node_restart (ha_node_t *node);


ha_sharded_cluster_t *
ha_sharded_cluster_new (const char *name);
void
ha_sharded_cluster_start (ha_sharded_cluster_t *cluster);
void
ha_sharded_cluster_wait_for_healthy (ha_sharded_cluster_t *cluster);
ha_node_t *
ha_sharded_cluster_add_config (ha_sharded_cluster_t *cluster, const char *name);
ha_node_t *
ha_sharded_cluster_add_router (ha_sharded_cluster_t *cluster, const char *name);
void
ha_sharded_cluster_add_replica_set (ha_sharded_cluster_t *cluster,
                                    ha_replica_set_t *replica_set);
void
ha_sharded_cluster_shutdown (ha_sharded_cluster_t *cluster);
mongoc_client_t *
ha_sharded_cluster_get_client (ha_sharded_cluster_t *cluster);


BSON_END_DECLS


#endif /* HA_TEST_H */
