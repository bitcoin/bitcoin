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

#ifdef MONGOC_ENABLE_SASL_SSPI

#include "mongoc-cluster-sspi-private.h"
#include "mongoc-cluster-sasl-private.h"
#include "mongoc-sasl-private.h"
#include "mongoc-sspi-private.h"
#include "mongoc-error.h"
#include "mongoc-util-private.h"


mongoc_sspi_client_state_t *
_mongoc_cluster_sspi_new (mongoc_uri_t *uri, const char *hostname)
{
   WCHAR *service; /* L"serviceName@hostname@REALM" */
   const char *service_name = "mongodb";
   ULONG flags = ISC_REQ_MUTUAL_AUTH;
   const char *service_realm = NULL;
   char *service_ascii = NULL;
   mongoc_sspi_client_state_t *state;
   const char *tmp_creds;
   int service_ascii_len;
   const bson_t *options;
   int tmp_creds_len;
   bson_t properties;
   bson_iter_t iter;
   int service_len;
   int user_len = 0;
   int pass_len = 0;
   WCHAR *pass = NULL;
   WCHAR *user = NULL;
   int res;

   options = mongoc_uri_get_options (uri);

   if (!mongoc_uri_get_mechanism_properties (uri, &properties)) {
      bson_init (&properties);
   }

   if (bson_iter_init_find_case (
          &iter, options, MONGOC_URI_GSSAPISERVICENAME) &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      service_name = bson_iter_utf8 (&iter, NULL);
   }
   if (bson_iter_init_find_case (&iter, &properties, "SERVICE_NAME") &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      service_name = bson_iter_utf8 (&iter, NULL);
   }

   if (bson_iter_init_find_case (&iter, &properties, "SERVICE_REALM") &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      service_realm = bson_iter_utf8 (&iter, NULL);
      service_ascii =
         bson_strdup_printf ("%s@%s@%s", service_name, hostname, service_realm);
   } else {
      service_ascii = bson_strdup_printf ("%s@%s", service_name, hostname);
   }
   service_ascii_len = strlen (service_ascii);

   /* this is donated to the sspi */
   service = calloc (service_ascii_len + 1, sizeof (WCHAR));
   service_len = MultiByteToWideChar (
      CP_UTF8, 0, service_ascii, service_ascii_len, service, service_ascii_len);
   service[service_len] = L'\0';
   bson_free (service_ascii);


   tmp_creds = mongoc_uri_get_password (uri);
   if (tmp_creds) {
      tmp_creds_len = strlen (tmp_creds);

      /* this is donated to the sspi */
      pass = calloc (tmp_creds_len + 1, sizeof (WCHAR));
      pass_len = MultiByteToWideChar (
         CP_UTF8, 0, tmp_creds, tmp_creds_len, pass, tmp_creds_len);
      pass[pass_len] = L'\0';
   }

   tmp_creds = mongoc_uri_get_username (uri);
   if (tmp_creds) {
      tmp_creds_len = strlen (tmp_creds);

      /* this is donated to the sspi */
      user = calloc (tmp_creds_len + 1, sizeof (WCHAR));
      user_len = MultiByteToWideChar (
         CP_UTF8, 0, tmp_creds, tmp_creds_len, user, tmp_creds_len);
      user[user_len] = L'\0';
   }

   state = (mongoc_sspi_client_state_t *) bson_malloc0 (sizeof *state);
   res = _mongoc_sspi_auth_sspi_client_init (
      service, flags, user, user_len, NULL, 0, pass, pass_len, state);


   if (res != MONGOC_SSPI_AUTH_GSS_ERROR) {
      return state;
   }
   bson_free (state);
   return NULL;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_auth_node_sspi --
 *
 *       Perform authentication for a cluster node using SSPI
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 * Side effects:
 *       error may be set.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_cluster_auth_node_sspi (mongoc_cluster_t *cluster,
                                mongoc_stream_t *stream,
                                const char *hostname,
                                bson_error_t *error)
{
   mongoc_cmd_parts_t parts;
   mongoc_sspi_client_state_t *state;
   uint8_t buf[4096] = {0};
   bson_iter_t iter;
   uint32_t buflen;
   bson_t reply;
   char *tmpstr;
   int conv_id;
   bson_t cmd;
   int res = MONGOC_SSPI_AUTH_GSS_CONTINUE;
   int step;
   bool canonicalize = false;
   const bson_t *options;
   bson_t properties;
   char real_name[BSON_HOST_NAME_MAX + 1];

   options = mongoc_uri_get_options (cluster->uri);

   if (bson_iter_init_find_case (
          &iter, options, MONGOC_URI_CANONICALIZEHOSTNAME) &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      canonicalize = bson_iter_bool (&iter);
   }

   if (mongoc_uri_get_mechanism_properties (cluster->uri, &properties)) {
      if (bson_iter_init_find_case (
             &iter, &properties, "CANONICALIZE_HOST_NAME") &&
          BSON_ITER_HOLDS_UTF8 (&iter)) {
         canonicalize = !strcasecmp (bson_iter_utf8 (&iter, NULL), "true");
      }
      bson_destroy (&properties);
   }

   if (canonicalize && _mongoc_sasl_get_canonicalized_name (
                          stream, real_name, sizeof real_name, error)) {
      state = _mongoc_cluster_sspi_new (cluster->uri, real_name);
   } else {
      state = _mongoc_cluster_sspi_new (cluster->uri, hostname);
   }


   if (!state) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_AUTHENTICATE,
                      "Couldn't initialize SSPI service.");
      goto failure;
   }

   for (step = 0;; step++) {
      mongoc_cmd_parts_init (&parts, "$external", MONGOC_QUERY_SLAVE_OK, &cmd);
      bson_init (&cmd);

      if (res == MONGOC_SSPI_AUTH_GSS_CONTINUE) {
         res = _mongoc_sspi_auth_sspi_client_step (state, buf);
      } else if (res == MONGOC_SSPI_AUTH_GSS_COMPLETE) {
         char *response;
         const char *tmp_creds = mongoc_uri_get_username (cluster->uri);
         int tmp_creds_len = strlen (tmp_creds);

         res = _mongoc_sspi_auth_sspi_client_unwrap (state, buf);
         response = bson_strdup (state->response);
         _mongoc_sspi_auth_sspi_client_wrap (
            state, response, tmp_creds, tmp_creds_len, 0);
         bson_free (response);
      }

      if (res == MONGOC_SSPI_AUTH_GSS_ERROR) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_AUTHENTICATE,
                         "Received invalid SSPI data.");

         mongoc_cmd_parts_cleanup (&parts);
         bson_destroy (&cmd);
         break;
      }

      if (step == 0) {
         _mongoc_cluster_build_sasl_start (
            &cmd, "GSSAPI", state->response, strlen (state->response));
      } else {
         if (state->response) {
            _mongoc_cluster_build_sasl_continue (
               &cmd, conv_id, state->response, strlen (state->response));
         } else {
            _mongoc_cluster_build_sasl_continue (&cmd, conv_id, "", 0);
         }
      }

      if (!mongoc_cluster_run_command_private (cluster,
                                               &parts,
                                               stream,
                                               0,
                                               &reply,
                                               error)) {
         mongoc_cmd_parts_cleanup (&parts);
         bson_destroy (&cmd);
         bson_destroy (&reply);
         break;
      }

      mongoc_cmd_parts_cleanup (&parts);
      bson_destroy (&cmd);

      if (bson_iter_init_find (&iter, &reply, "done") &&
          bson_iter_as_bool (&iter)) {
         bson_destroy (&reply);
         break;
      }

      conv_id = _mongoc_cluster_get_conversation_id (&reply);

      if (!bson_iter_init_find (&iter, &reply, "payload") ||
          !BSON_ITER_HOLDS_UTF8 (&iter)) {
         bson_destroy (&reply);
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_AUTHENTICATE,
                         "Received invalid SASL reply from MongoDB server.");
         break;
      }


      tmpstr = bson_iter_utf8 (&iter, &buflen);

      if (buflen > sizeof buf) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_AUTHENTICATE,
                         "SASL reply from MongoDB is too large.");

         bson_destroy (&reply);
         break;
      }

      memcpy (buf, tmpstr, buflen);

      bson_destroy (&reply);
   }

   bson_free (state);

failure:

   if (error->domain) {
      return false;
   }

   return true;
}

#endif
