/*
 * Copyright 2013-2014 MongoDB, Inc.
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


#include <mongoc.h>
#include <stdio.h>


int
main (int argc, char *argv[])
{
   mongoc_database_t *database;
   mongoc_cursor_t *cursor;
   mongoc_client_t *client;
   const bson_t *reply;
   uint16_t port;
   bson_error_t error;
   bson_t ping;
   char *host_and_port;
   char *str;

   if (argc < 2 || argc > 3) {
      fprintf (stderr, "usage: %s HOSTNAME [PORT]\n", argv[0]);
      return 1;
   }

   mongoc_init ();

   port = (argc == 3) ? atoi (argv[2]) : 27017;

   if (strncmp (argv[1], "mongodb://", 10) == 0) {
      host_and_port = bson_strdup (argv[1]);
   } else {
      host_and_port = bson_strdup_printf ("mongodb://%s:%hu", argv[1], port);
   }

   client = mongoc_client_new (host_and_port);

   if (!client) {
      fprintf (stderr, "Invalid hostname or port: %s\n", host_and_port);
      bson_free (host_and_port);
      return 2;
   }
   bson_free (host_and_port);

   mongoc_client_set_error_api (client, 2);

   bson_init (&ping);
   bson_append_int32 (&ping, "ping", 4, 1);
   database = mongoc_client_get_database (client, "test");
   cursor = mongoc_database_command (
      database, (mongoc_query_flags_t) 0, 0, 1, 0, &ping, NULL, NULL);
   if (mongoc_cursor_next (cursor, &reply)) {
      str = bson_as_canonical_extended_json (reply, NULL);
      fprintf (stdout, "%s\n", str);
      bson_free (str);
   } else if (mongoc_cursor_error (cursor, &error)) {
      fprintf (stderr, "Ping failure: %s\n", error.message);
      mongoc_cursor_destroy (cursor);
      mongoc_database_destroy (database);
      mongoc_client_destroy (client);
      return 3;
   }

   bson_destroy (&ping);
   mongoc_cursor_destroy (cursor);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);

   return 0;
}
