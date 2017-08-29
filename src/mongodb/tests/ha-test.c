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


#include <fcntl.h>
#include <limits.h>
#include <mongoc.h>
#include <stdio.h>
#include <signal.h>
#ifdef __linux
#include <sys/prctl.h>
#include <sys/wait.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __FreeBSD__
#include <sys/wait.h>
#endif

#include "ha-test.h"

#ifdef BSON_OS_WIN32
#include <shlwapi.h>
#ifdef _MSC_VER
#define PATH_MAX 1024
#define S_ISREG(b) ((b) &_S_IFREG)
#define S_ISDIR(b) ((b) &_S_IFDIR)
#endif
#define bson_chdir _chdir
#define bson_unlink _unlink
#define bson_mkdir(_d, _m) _mkdir (_d)
#define sleep(_n) Sleep ((_n) *1000)
#else
#include <dirent.h>
#define bson_mkdir mkdir
#define bson_chdir chdir
#define bson_unlink unlink
#endif


void
ha_mkdir (const char *name)
{
   char buf[1024];
   if (bson_mkdir (name, 0750)) {
      char *err = bson_strerror_r (errno, buf, sizeof buf);
      fprintf (stderr,
               "Failed to create directory \"%s\" because of %s\n",
               name,
               err);

      abort ();
   }
}

mongoc_client_pool_t *
ha_replica_set_create_client_pool (ha_replica_set_t *replica_set)
{
   mongoc_client_pool_t *client;
   bson_string_t *str;
   ha_node_t *iter;
   char *portstr;
   mongoc_uri_t *uri;

   str = bson_string_new ("mongodb://");

   for (iter = replica_set->nodes; iter; iter = iter->next) {
      bson_string_append (str, "127.0.0.1:");

      portstr = bson_strdup_printf ("%hu", iter->port);
      bson_string_append (str, portstr);
      bson_free (portstr);

      if (iter->next) {
         bson_string_append (str, ",");
      }
   }

   bson_string_append (str, "/?replicaSet=");
   bson_string_append (str, replica_set->name);

#ifdef MONGOC_ENABLE_SSL
   if (replica_set->ssl_opt) {
      bson_string_append (str, "&ssl=true");
   }
#endif

   uri = mongoc_uri_new (str->str);

   client = mongoc_client_pool_new (uri);

#ifdef MONGOC_ENABLE_SSL
   if (replica_set->ssl_opt) {
      mongoc_client_pool_set_ssl_opts (client, replica_set->ssl_opt);
   }
#endif

   mongoc_uri_destroy (uri);

   bson_string_free (str, true);

   return client;
}


mongoc_client_t *
ha_replica_set_create_client (ha_replica_set_t *replica_set)
{
   mongoc_client_t *client;
   bson_string_t *str;
   ha_node_t *iter;
   char *portstr;

   str = bson_string_new ("mongodb://");

   for (iter = replica_set->nodes; iter; iter = iter->next) {
      bson_string_append (str, "127.0.0.1:");

      portstr = bson_strdup_printf ("%hu", iter->port);
      bson_string_append (str, portstr);
      bson_free (portstr);

      if (iter->next) {
         bson_string_append (str, ",");
      }
   }

   bson_string_append (str, "/?replicaSet=");
   bson_string_append (str, replica_set->name);

#ifdef MONGOC_ENABLE_SSL
   if (replica_set->ssl_opt) {
      bson_string_append (str, "&ssl=true");
   }
#endif

   client = mongoc_client_new (str->str);

#ifdef MONGOC_ENABLE_SSL
   if (replica_set->ssl_opt) {
      mongoc_client_set_ssl_opts (client, replica_set->ssl_opt);
   }
#endif

   bson_string_free (str, true);

   return client;
}


static ha_node_t *
ha_node_new (const char *name,
             const char *repl_set,
             const char *dbpath,
             bool is_arbiter,
             bool is_config,
             bool is_router,
             uint16_t port)
{
   ha_node_t *node;

   node = (ha_node_t *) bson_malloc0 (sizeof *node);
   node->name = bson_strdup (name);
   node->repl_set = bson_strdup (repl_set);
   node->dbpath = bson_strdup (dbpath);
   node->is_arbiter = is_arbiter;
   node->is_config = is_config;
   node->is_router = is_router;
   node->port = port;

   return node;
}


void
ha_node_setup (ha_node_t *node)
{
   ha_mkdir (node->dbpath);
}


void
ha_node_kill (ha_node_t *node)
{
#ifdef BSON_OS_UNIX
   if (node->pid) {
      int status;

      kill (node->pid, SIGKILL);
      waitpid (node->pid, &status, 0);

      node->pid = 0;
   }
#else
   if (node->pid.is_alive) {
      if (!TerminateProcess (node->pid.proc, 0)) {
         fprintf (stderr, "Couldn't kill: %d\n", (int) GetLastError ());
         abort ();
      }
      WaitForSingleObject (node->pid.proc, INFINITE);
      CloseHandle (node->pid.proc);
      CloseHandle (node->pid.thread);
      node->pid.is_alive = 0;
   }
#endif
}

#ifdef BSON_OS_WIN32
bson_ha_pid_t
ha_spawn_win32_node (char **argv)
{
   char **argn;
   char path[MAX_PATH];
   bson_string_t *args;
   STARTUPINFO si = {0};
   PROCESS_INFORMATION pi = {0};
   bool r;
   bson_ha_pid_t out;

   si.cb = sizeof (si);


   out.is_alive = true;

   args = bson_string_new ("");

   bson_snprintf (path, sizeof path, "%s.exe", argv[0]);

   if (!PathFindOnPath (path, NULL)) {
      fprintf (
         stderr, "Failed to find path to binary: %s - %s\n", path, argv[0]);
      abort ();
   }
   bson_string_append_printf (args, "\"%s\"", path);

   for (argn = argv + 1; *argn != NULL; argn++) {
      bson_string_append_printf (args, " \"%s\"", *argn);
   }

   r = CreateProcess (
      path, args->str, NULL, NULL, false, 0, NULL, NULL, &si, &pi);

   if (r) {
      out.proc = pi.hProcess;
      out.thread = pi.hThread;
   } else {
      out.is_alive = false;
      fprintf (stderr,
               "Failed to create %s - %s: %d\n",
               path,
               args->str,
               (int) GetLastError ());
      abort ();
   }

   bson_string_free (args, true);

   return out;
}
#endif

void
ha_node_restart (ha_node_t *node)
{
   struct stat st;
   bson_ha_pid_t pid;
   char portstr[12];
   char *argv[30];
   int i = 0;

   bson_snprintf (portstr, sizeof portstr, "%hu", node->port);
   portstr[sizeof portstr - 1] = '\0';

   ha_node_kill (node);

   if (!node->is_router && !node->is_config) {
      argv[i++] = (char *) "mongod";
      argv[i++] = (char *) "--dbpath";
      argv[i++] = (char *) ".";
      argv[i++] = (char *) "--port";
      argv[i++] = portstr;
      argv[i++] = (char *) "--nojournal";
      argv[i++] = (char *) "--noprealloc";
      argv[i++] = (char *) "--smallfiles";
      argv[i++] = (char *) "--nohttpinterface";
      argv[i++] = (char *) "--bind_ip";
      argv[i++] = (char *) "127.0.0.1";
      argv[i++] = (char *) "--replSet";
      argv[i++] = node->repl_set;

#ifdef MONGOC_ENABLE_SSL
      if (node->ssl_opt) {
         if (node->ssl_opt->pem_file) {
            argv[i++] = (char *) "--sslPEMKeyFile";
            argv[i++] = (char *) (node->ssl_opt->pem_file);
            argv[i++] = (char *) "--sslClusterFile";
            argv[i++] = (char *) (node->ssl_opt->pem_file);
         }
         if (node->ssl_opt->ca_file) {
            argv[i++] = (char *) "--sslCAFile";
            argv[i++] = (char *) (node->ssl_opt->ca_file);
         }
         argv[i++] = (char *) "--sslOnNormalPorts";
      }
#endif

      argv[i++] = "--logpath";
      argv[i++] = "log";
      argv[i++] = NULL;
   } else if (node->is_config) {
      argv[i++] = (char *) "mongod";
      argv[i++] = (char *) "--configsvr";
      argv[i++] = (char *) "--dbpath";
      argv[i++] = (char *) ".";
      argv[i++] = (char *) "--port";
      argv[i++] = (char *) portstr;
      argv[i++] = "--logpath";
      argv[i++] = "log";
      argv[i++] = NULL;
   } else {
      argv[i++] = (char *) "mongos";
      argv[i++] = (char *) "--bind_ip";
      argv[i++] = (char *) "127.0.0.1";
      argv[i++] = (char *) "--nohttpinterface";
      argv[i++] = (char *) "--port";
      argv[i++] = (char *) portstr;
      argv[i++] = (char *) "--configdb";
      argv[i++] = node->configopt;
      argv[i++] = "--logpath";
      argv[i++] = "log";
      argv[i++] = NULL;
   }

#ifdef BSON_OS_UNIX

   pid = fork ();
   if (pid < 0) {
      perror ("Failed to fork process");
      abort ();
   }

   if (!pid) {
      int fd;

#ifdef __linux
      prctl (PR_SET_PDEATHSIG, 15);
#endif

      if (0 != chdir (node->dbpath)) {
         perror ("Failed to chdir");
         abort ();
      }

      if (0 == stat ("mongod.lock", &st)) {
         unlink ("mongod.lock");
      }

      fd = open ("/dev/null", O_RDWR);
      if (fd == -1) {
         perror ("Failed to open /dev/null");
         abort ();
      }

      dup2 (fd, STDIN_FILENO);
      dup2 (fd, STDOUT_FILENO);
      dup2 (fd, STDERR_FILENO);

      close (fd);

      if (-1 == execvp (argv[0], argv)) {
         perror ("Failed to spawn unix process");
         abort ();
      }
   }

   fprintf (stderr, "[%d]: ", (int) pid);
#else
   {
      char pathbuf[1024];
      GetCurrentDirectory (sizeof pathbuf, pathbuf);

      if (0 != bson_chdir (node->dbpath)) {
         perror ("Failed to chdir");
         abort ();
      }

      if (0 == stat ("mongod.lock", &st)) {
         bson_unlink ("mongod.lock");
      }

      pid = ha_spawn_win32_node (argv);

      if (!pid.is_alive) {
         perror ("Failed to launch win32 process");
         abort ();
      }

      if (0 != bson_chdir (pathbuf)) {
         perror ("Failed to chdir");
         abort ();
      }
   }
#endif

   for (i = 0; argv[i]; i++)
      fprintf (stderr, "%s ", argv[i]);
   fprintf (stderr, "\n");


   node->pid = pid;
}


static void
ha_node_destroy (ha_node_t *node)
{
   ha_node_kill (node);
   bson_free (node->name);
   bson_free (node->repl_set);
   bson_free (node->dbpath);
   bson_free (node->configopt);
   bson_free (node);
}


static MONGOC_ONCE_FUN (random_init)
{
   srand ((unsigned) time (NULL));
   MONGOC_ONCE_RETURN;
}


static int
random_int (void)
{
   static mongoc_once_t once = MONGOC_ONCE_INIT;
   mongoc_once (&once, random_init);
   return rand ();
}


static int
random_int_range (int low, int high)
{
   return low + (random_int () % (high - low));
}


#ifdef MONGOC_ENABLE_SSL
void
ha_replica_set_ssl (ha_replica_set_t *repl_set, mongoc_ssl_opt_t *opt)
{
   repl_set->ssl_opt = opt;
}
#endif


ha_replica_set_t *
ha_replica_set_new (const char *name)
{
   ha_replica_set_t *repl_set;

   repl_set = (ha_replica_set_t *) bson_malloc0 (sizeof *repl_set);
   repl_set->name = bson_strdup (name);
   repl_set->next_port = random_int_range (30000, 40000);

   return repl_set;
}


static ha_node_t *
ha_replica_set_add_node (ha_replica_set_t *replica_set,
                         const char *name,
                         bool is_arbiter)
{
   ha_node_t *node;
   ha_node_t *iter;
   char dbpath[PATH_MAX];

   bson_snprintf (dbpath, sizeof dbpath, "%s/%s", replica_set->name, name);
   dbpath[sizeof dbpath - 1] = '\0';

   node = ha_node_new (name,
                       replica_set->name,
                       dbpath,
                       is_arbiter,
                       false,
                       false,
                       replica_set->next_port++);
#ifdef MONGOC_ENABLE_SSL
   node->ssl_opt = replica_set->ssl_opt;
#endif

   if (!replica_set->nodes) {
      replica_set->nodes = node;
   } else {
      for (iter = replica_set->nodes; iter->next; iter = iter->next) {
      }
      iter->next = node;
   }

   return node;
}


ha_node_t *
ha_replica_set_add_arbiter (ha_replica_set_t *replica_set, const char *name)
{
   return ha_replica_set_add_node (replica_set, name, true);
}


ha_node_t *
ha_replica_set_add_replica (ha_replica_set_t *replica_set, const char *name)
{
   return ha_replica_set_add_node (replica_set, name, false);
}


static void
ha_replica_set_configure (ha_replica_set_t *replica_set, ha_node_t *primary)
{
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_error_t error;
   bson_iter_t iter;
   ha_node_t *node;
   bson_t ar;
   bson_t cmd;
   bson_t config;
   bson_t member;
   char *str;
   char *uristr;
   char hoststr[32];
   char key[8];
   int i = 0;

#ifdef MONGOC_ENABLE_SSL
   if (replica_set->ssl_opt) {
      uristr = bson_strdup_printf ("mongodb://127.0.0.1:%hu/?ssl=true",
                                   primary->port);
   } else {
      uristr = bson_strdup_printf ("mongodb://127.0.0.1:%hu/", primary->port);
   }
#else
   uristr = bson_strdup_printf ("mongodb://127.0.0.1:%hu/", primary->port);
#endif

   client = mongoc_client_new (uristr);
#ifdef MONGOC_ENABLE_SSL
   if (replica_set->ssl_opt) {
      mongoc_client_set_ssl_opts (client, replica_set->ssl_opt);
   }
#endif
   bson_free (uristr);

   bson_init (&cmd);
   bson_append_document_begin (&cmd, "replSetInitiate", -1, &config);
   bson_append_utf8 (&config, "_id", 3, replica_set->name, -1);
   bson_append_array_begin (&config, "members", -1, &ar);
   for (node = replica_set->nodes; node; node = node->next) {
      bson_snprintf (key, sizeof key, "%u", i);
      key[sizeof key - 1] = '\0';

      bson_snprintf (hoststr, sizeof hoststr, "127.0.0.1:%hu", node->port);
      hoststr[sizeof hoststr - 1] = '\0';

      bson_append_document_begin (&ar, key, -1, &member);
      bson_append_int32 (&member, "_id", -1, i);
      bson_append_utf8 (&member, "host", -1, hoststr, -1);
      bson_append_bool (&member, "arbiterOnly", -1, node->is_arbiter);
      bson_append_document_end (&ar, &member);

      i++;
   }
   bson_append_array_end (&config, &ar);
   bson_append_document_end (&cmd, &config);

   str = bson_as_canonical_extended_json (&cmd, NULL);
   MONGOC_DEBUG ("Config: %s", str);
   bson_free (str);

   database = mongoc_client_get_database (client, "admin");

again:
   cursor = mongoc_database_command (
      database, MONGOC_QUERY_NONE, 0, 1, 0, &cmd, NULL, NULL);

   while (mongoc_cursor_next (cursor, &doc)) {
      str = bson_as_canonical_extended_json (doc, NULL);
      MONGOC_DEBUG ("Reply: %s", str);
      bson_free (str);
      if (bson_iter_init_find (&iter, doc, "ok") && bson_iter_as_bool (&iter)) {
         goto cleanup;
      }
   }

   if (mongoc_cursor_error (cursor, &error)) {
      mongoc_cursor_destroy (cursor);
      MONGOC_WARNING ("%s: Retrying in 1 second.", error.message);
      sleep (1);
      goto again;
   }

cleanup:
   mongoc_cursor_destroy (cursor);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
   bson_destroy (&cmd);
}

void
ha_rm_dir (const char *name)
{
#ifdef BSON_OS_UNIX
   char *cmd;
   cmd = bson_strdup_printf ("rm -rf \"%s\"", name);
   fprintf (stderr, "%s\n", cmd);
   if (0 != system (cmd)) {
      fprintf (stderr, "%s failed\n", cmd);
   }
   bson_free (cmd);
#else
   SHFILEOPSTRUCT fos = {0};
   char path[MAX_PATH + 1] = {0};
   int r;
   char curdir[1024];
   GetCurrentDirectory (sizeof curdir, curdir);
   bson_snprintf (path, sizeof path, "%s\\%s", curdir, name);
   path[strlen (path) + 1] = '\0';

   fos.wFunc = FO_DELETE;
   fos.pFrom = path;
   fos.fFlags = FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION;
   fprintf (stderr, "win32 removing \"%s\"\n", path);
   r = SHFileOperation (&fos);
   /* file not found isn't an error.  I.e. the 2 */
   if (r && r != 2) {
      fprintf (stderr, "failure to delete %s with %d\n", name, r);
      abort ();
   }
#endif
}


void
ha_replica_set_start (ha_replica_set_t *replica_set)
{
   struct stat st;
   ha_node_t *primary = NULL;
   ha_node_t *node;

   if (!stat (replica_set->name, &st)) {
      if (S_ISDIR (st.st_mode)) {
         ha_rm_dir (replica_set->name);
      }
   }

   ha_mkdir (replica_set->name);

   for (node = replica_set->nodes; node; node = node->next) {
      if (!primary && !node->is_arbiter) {
         primary = node;
      }
      ha_node_setup (node);
      ha_node_restart (node);
   }

   BSON_ASSERT (primary);

   sleep (2);

   ha_replica_set_configure (replica_set, primary);
}


void
ha_replica_set_shutdown (ha_replica_set_t *replica_set)
{
   ha_node_t *node;

   for (node = replica_set->nodes; node; node = node->next) {
      ha_node_kill (node);
   }
}


void
ha_replica_set_destroy (ha_replica_set_t *replica_set)
{
   ha_node_t *node;

   while ((node = replica_set->nodes)) {
      replica_set->nodes = node->next;
      ha_node_destroy (node);
   }

   bson_free (replica_set->name);
   bson_free (replica_set);
}


static bool
ha_replica_set_get_status (ha_replica_set_t *replica_set, bson_t *status)
{
   mongoc_database_t *db;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bool ret = false;
   ha_node_t *node;
   bson_t cmd;
   char *uristr;

   bson_init (&cmd);
   bson_append_int32 (&cmd, "replSetGetStatus", -1, 1);

   for (node = replica_set->nodes; !ret && node; node = node->next) {
      uristr = bson_strdup_printf ("mongodb://127.0.0.1:%hu/?slaveOk=true",
                                   node->port);
      client = mongoc_client_new (uristr);
#ifdef MONGOC_ENABLE_SSL
      if (replica_set->ssl_opt) {
         mongoc_client_set_ssl_opts (client, replica_set->ssl_opt);
      }
#endif
      bson_free (uristr);

      db = mongoc_client_get_database (client, "admin");

      if ((cursor = mongoc_database_command (
              db, MONGOC_QUERY_SLAVE_OK, 0, 1, 0, &cmd, NULL, NULL))) {
         if (mongoc_cursor_next (cursor, &doc)) {
            bson_copy_to (doc, status);
            ret = true;
         }
         mongoc_cursor_destroy (cursor);
      }

      mongoc_database_destroy (db);
      mongoc_client_destroy (client);
   }

   return ret;
}


void
ha_replica_set_wait_for_healthy (ha_replica_set_t *replica_set)
{
   bson_iter_t iter;
   bson_iter_t ar;
   bson_iter_t member;
   const char *stateStr;
   bson_t status;

again:
   sleep (1);

   if (!ha_replica_set_get_status (replica_set, &status)) {
      MONGOC_INFO ("Failed to get replicaSet status. "
                   "Sleeping 1 second.");
      goto again;
   }

#if 0
   {
      char *str;

      str = bson_as_canonical_extended_json(&status, NULL);
      fprintf(stderr, "%s\n", str);
      bson_free(str);
   }
#endif

   if (!bson_iter_init_find (&iter, &status, "members") ||
       !BSON_ITER_HOLDS_ARRAY (&iter) || !bson_iter_recurse (&iter, &ar)) {
      bson_destroy (&status);
      MONGOC_INFO ("ReplicaSet has not yet come online. "
                   "Sleeping 1 second.");
      goto again;
   }

   while (bson_iter_next (&ar)) {
      if (BSON_ITER_HOLDS_DOCUMENT (&ar) && bson_iter_recurse (&ar, &member) &&
          bson_iter_find (&member, "stateStr") &&
          (stateStr = bson_iter_utf8 (&member, NULL))) {
         if (!!strcmp (stateStr, "PRIMARY") &&
             !!strcmp (stateStr, "SECONDARY") &&
             !!strcmp (stateStr, "ARBITER")) {
            bson_destroy (&status);
            MONGOC_INFO ("Found unhealthy node. Sleeping 1 second.");
            goto again;
         }
      }
   }

   bson_destroy (&status);
}


ha_sharded_cluster_t *
ha_sharded_cluster_new (const char *name)
{
   ha_sharded_cluster_t *cluster;

   cluster = (ha_sharded_cluster_t *) bson_malloc0 (sizeof *cluster);
   cluster->next_port = random_int_range (40000, 41000);
   cluster->name = bson_strdup (name);

   return cluster;
}


void
ha_sharded_cluster_add_replica_set (ha_sharded_cluster_t *cluster,
                                    ha_replica_set_t *replica_set)
{
   int i;

   BSON_ASSERT (cluster);
   BSON_ASSERT (replica_set);

   for (i = 0; i < 12; i++) {
      if (!cluster->replicas[i]) {
         cluster->replicas[i] = replica_set;
         break;
      }
   }
}


ha_node_t *
ha_sharded_cluster_add_config (ha_sharded_cluster_t *cluster, const char *name)
{
   ha_node_t *node;
   char dbpath[PATH_MAX];

   BSON_ASSERT (cluster);

   bson_snprintf (dbpath, sizeof dbpath, "%s/%s", cluster->name, name);
   dbpath[sizeof dbpath - 1] = '\0';

   node = ha_node_new (
      name, NULL, dbpath, false, true, false, cluster->next_port++);
#ifdef MONGOC_ENABLE_SSL
   node->ssl_opt = cluster->ssl_opt;
#endif
   node->next = cluster->configs;
   cluster->configs = node;

   return node;
}


ha_node_t *
ha_sharded_cluster_add_router (ha_sharded_cluster_t *cluster, const char *name)
{
   ha_node_t *node;
   char dbpath[PATH_MAX];

   BSON_ASSERT (cluster);

   bson_snprintf (dbpath, sizeof dbpath, "%s/%s", cluster->name, name);
   dbpath[sizeof dbpath - 1] = '\0';

   node = ha_node_new (
      name, NULL, dbpath, false, false, true, cluster->next_port++);
#ifdef MONGOC_ENABLE_SSL
   node->ssl_opt = cluster->ssl_opt;
#endif
   node->next = cluster->routers;
   cluster->routers = node;

   return node;
}


static void
ha_config_add_shard (ha_node_t *node, ha_replica_set_t *replica_set)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_string_t *shardstr;
   bson_error_t error;
   bool r;
   bson_t reply;
   bson_t cmd = BSON_INITIALIZER;
   char *uristr;

   uristr = bson_strdup_printf ("mongodb://127.0.0.1:%hu/", node->port);
   client = mongoc_client_new (uristr);
   collection = mongoc_client_get_collection (client, "admin", "fake");

   shardstr = bson_string_new (NULL);
   bson_string_append_printf (shardstr,
                              "%s/127.0.0.1:%hu",
                              replica_set->name,
                              replica_set->nodes->port);

   bson_append_utf8 (&cmd, "addShard", -1, shardstr->str, shardstr->len);

   bson_string_free (shardstr, true);

again:
   sleep (1);

   r =
      mongoc_collection_command_simple (collection, &cmd, NULL, &reply, &error);

   if (!r) {
      fprintf (stderr, "%s\n", error.message);
      goto again;
   }

#if 1
   {
      char *str;

      str = bson_as_canonical_extended_json (&reply, NULL);
      fprintf (stderr, "%s\n", str);
      bson_free (str);
   }
#endif

   bson_destroy (&reply);
   bson_destroy (&cmd);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_free (uristr);
}

void
ha_sharded_cluster_start (ha_sharded_cluster_t *cluster)
{
   bson_string_t *configopt;
   struct stat st;
   ha_node_t *iter;
   int i;

   BSON_ASSERT (cluster);

   if (!stat (cluster->name, &st)) {
      if (S_ISDIR (st.st_mode)) {
         ha_rm_dir (cluster->name);
      }
   }

   ha_mkdir (cluster->name);

   for (i = 0; i < 12; i++) {
      if (cluster->replicas[i]) {
         ha_replica_set_start (cluster->replicas[i]);
      }
   }

   configopt = bson_string_new (NULL);

   for (iter = cluster->configs; iter; iter = iter->next) {
      ha_node_setup (iter);
      ha_node_restart (iter);
      bson_string_append_printf (
         configopt, "127.0.0.1:%hu%s", iter->port, iter->next ? "," : "");
   }

   sleep (10);

   for (iter = cluster->routers; iter; iter = iter->next) {
      bson_free (iter->configopt);
      iter->configopt = bson_strdup (configopt->str);
      ha_node_setup (iter);
      ha_node_restart (iter);
   }

   ha_sharded_cluster_wait_for_healthy (cluster);

   for (i = 0; i < 12; i++) {
      if (cluster->replicas[i]) {
         ha_config_add_shard (cluster->routers, cluster->replicas[i]);
      }
   }

   bson_string_free (configopt, true);
}


void
ha_sharded_cluster_wait_for_healthy (ha_sharded_cluster_t *cluster)
{
   int i;

   BSON_ASSERT (cluster);

   for (i = 0; i < 12; i++) {
      if (cluster->replicas[i]) {
         ha_replica_set_wait_for_healthy (cluster->replicas[i]);
      }
   }
}


void
ha_sharded_cluster_shutdown (ha_sharded_cluster_t *cluster)
{
   ha_node_t *iter;
   int i;

   BSON_ASSERT (cluster);

   for (i = 0; i < 12; i++) {
      if (cluster->replicas[i]) {
         ha_replica_set_shutdown (cluster->replicas[i]);
      }
   }

   for (iter = cluster->configs; iter; iter = iter->next) {
      ha_node_kill (iter);
   }

   for (iter = cluster->routers; iter; iter = iter->next) {
      ha_node_kill (iter);
   }
}


mongoc_client_t *
ha_sharded_cluster_get_client (ha_sharded_cluster_t *cluster)
{
   const ha_node_t *iter;
   mongoc_client_t *client;
   bson_string_t *str;

   BSON_ASSERT (cluster);
   BSON_ASSERT (cluster->routers);

   str = bson_string_new ("mongodb://");

   for (iter = cluster->routers; iter; iter = iter->next) {
      bson_string_append_printf (
         str, "127.0.0.1:%hu%s", iter->port, iter->next ? "," : "");
   }

   bson_string_append (str, "/");

   client = mongoc_client_new (str->str);

   bson_string_free (str, true);

   return client;
}
