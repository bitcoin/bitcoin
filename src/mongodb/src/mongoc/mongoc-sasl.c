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

#ifdef MONGOC_ENABLE_SASL
#include "mongoc-sasl-private.h"
#include "mongoc-util-private.h"

#include "mongoc-trace-private.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "SASL"

void
_mongoc_sasl_set_user (mongoc_sasl_t *sasl, const char *user)
{
   BSON_ASSERT (sasl);

   bson_free (sasl->user);
   sasl->user = user ? bson_strdup (user) : NULL;
}


void
_mongoc_sasl_set_pass (mongoc_sasl_t *sasl, const char *pass)
{
   BSON_ASSERT (sasl);

   bson_free (sasl->pass);
   sasl->pass = pass ? bson_strdup (pass) : NULL;
}


void
_mongoc_sasl_set_service_host (mongoc_sasl_t *sasl, const char *service_host)
{
   BSON_ASSERT (sasl);

   bson_free (sasl->service_host);
   sasl->service_host = service_host ? bson_strdup (service_host) : NULL;
}


void
_mongoc_sasl_set_service_name (mongoc_sasl_t *sasl, const char *service_name)
{
   BSON_ASSERT (sasl);

   bson_free (sasl->service_name);
   sasl->service_name = service_name ? bson_strdup (service_name) : NULL;
}


void
_mongoc_sasl_set_properties (mongoc_sasl_t *sasl, const mongoc_uri_t *uri)
{
   const bson_t *options;
   bson_iter_t iter;
   bson_t properties;
   const char *service_name = NULL;
   bool canonicalize = false;

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
      /* newer "authMechanismProperties" URI syntax takes precedence */
      service_name = bson_iter_utf8 (&iter, NULL);
   }

   _mongoc_sasl_set_service_name (sasl, service_name);

   /*
    * Driver Authentication Spec: "Drivers MAY allow the user to request
    * canonicalization of the hostname. This might be required when the hosts
    * report different hostnames than what is used in the kerberos database.
    * The default is "false".
    *
    * Some underlying GSSAPI layers will do this for us, but can be disabled in
    * their config (krb.conf).
    *
    * See CDRIVER-323 for more information.
    */
   if (bson_iter_init_find_case (
          &iter, options, MONGOC_URI_CANONICALIZEHOSTNAME) &&
       BSON_ITER_HOLDS_BOOL (&iter)) {
      canonicalize = bson_iter_bool (&iter);
   }

   if (bson_iter_init_find_case (
          &iter, &properties, "CANONICALIZE_HOST_NAME") &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      /* newer "authMechanismProperties" URI syntax takes precedence */
      canonicalize = !strcasecmp (bson_iter_utf8 (&iter, NULL), "true");
   }

   sasl->canonicalize_host_name = canonicalize;

   bson_destroy (&properties);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_sasl_get_canonicalized_name --
 *
 *       Query the node to get the canonicalized name. This may happen if
 *       the node has been accessed via an alias.
 *
 *       The gssapi code will use this if canonicalizeHostname is true.
 *
 *       Some underlying layers of krb might do this for us, but they can
 *       be disabled in krb.conf.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_sasl_get_canonicalized_name (mongoc_stream_t *node_stream, /* IN */
                                     char *name,                   /* OUT */
                                     size_t namelen,               /* IN */
                                     bson_error_t *error)          /* OUT */
{
   mongoc_stream_t *stream;
   mongoc_stream_t *tmp;
   mongoc_socket_t *sock = NULL;
   char *canonicalized;

   ENTRY;

   BSON_ASSERT (node_stream);
   BSON_ASSERT (name);

   /*
    * Find the underlying socket used in the stream chain.
    */
   for (stream = node_stream; stream;) {
      if ((tmp = mongoc_stream_get_base_stream (stream))) {
         stream = tmp;
         continue;
      }
      break;
   }

   BSON_ASSERT (stream);

   if (stream->type == MONGOC_STREAM_SOCKET) {
      sock =
         mongoc_stream_socket_get_socket ((mongoc_stream_socket_t *) stream);
      if (sock) {
         canonicalized = mongoc_socket_getnameinfo (sock);
         if (canonicalized) {
            bson_snprintf (name, namelen, "%s", canonicalized);
            bson_free (canonicalized);
            RETURN (true);
         }
      }
   }

   RETURN (false);
}
#endif
