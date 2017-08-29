/*
 * Copyright 2017 MongoDB, Inc.
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

#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_SASL_CYRUS
#include "mongoc-cyrus-private.h"
#include "mongoc-cluster-cyrus-private.h"
#include "mongoc-error.h"
#include "mongoc-trace-private.h"

bool
_mongoc_cluster_auth_node_cyrus (mongoc_cluster_t *cluster,
                                 mongoc_stream_t *stream,
                                 const char *hostname,
                                 bson_error_t *error)
{
   mongoc_cmd_parts_t parts;
   uint32_t buflen = 0;
   mongoc_cyrus_t sasl;
   bson_iter_t iter;
   bool ret = false;
   const char *tmpstr;
   uint8_t buf[4096] = {0};
   bson_t cmd;
   bson_t reply;
   int conv_id = 0;

   BSON_ASSERT (cluster);
   BSON_ASSERT (stream);

   if (!_mongoc_cyrus_new_from_cluster (
          &sasl, cluster, stream, hostname, error)) {
      return false;
   }

   for (;;) {
      mongoc_cmd_parts_init (&parts, "$external", MONGOC_QUERY_SLAVE_OK, &cmd);

      if (!_mongoc_cyrus_step (
             &sasl, buf, buflen, buf, sizeof buf, &buflen, error)) {
         goto failure;
      }

      bson_init (&cmd);

      if (sasl.step == 1) {
         _mongoc_cluster_build_sasl_start (
            &cmd, sasl.credentials.mechanism, (const char *) buf, buflen);
      } else {
         _mongoc_cluster_build_sasl_continue (
            &cmd, conv_id, (const char *) buf, buflen);
      }

      TRACE ("SASL: authenticating (step %d)", sasl.step);

      if (!mongoc_cluster_run_command_private (
             cluster, &parts, stream, 0, &reply, error)) {
         bson_destroy (&cmd);
         bson_destroy (&reply);
         goto failure;
      }

      bson_destroy (&cmd);

      if (bson_iter_init_find (&iter, &reply, "done") &&
          bson_iter_as_bool (&iter)) {
         bson_destroy (&reply);
         mongoc_cmd_parts_cleanup (&parts);
         break;
      }

      conv_id = _mongoc_cluster_get_conversation_id (&reply);

      if (!bson_iter_init_find (&iter, &reply, "payload") ||
          !BSON_ITER_HOLDS_UTF8 (&iter)) {
         MONGOC_DEBUG ("SASL: authentication failed");
         bson_destroy (&reply);
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_AUTHENTICATE,
                         "Received invalid SASL reply from MongoDB server.");
         goto failure;
      }

      tmpstr = bson_iter_utf8 (&iter, &buflen);

      if (buflen > sizeof buf) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_AUTHENTICATE,
                         "SASL reply from MongoDB is too large.");

         bson_destroy (&reply);
         goto failure;
      }

      memcpy (buf, tmpstr, buflen);

      bson_destroy (&reply);
      mongoc_cmd_parts_cleanup (&parts);
   }

   TRACE ("%s", "SASL: authenticated");

   ret = true;

failure:
   _mongoc_cyrus_destroy (&sasl);
   mongoc_cmd_parts_cleanup (&parts);

   return ret;
}
#endif
