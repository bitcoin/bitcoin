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

#ifndef MONGOC_URI_H
#define MONGOC_URI_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"
#include "mongoc-host-list.h"
#include "mongoc-read-prefs.h"
#include "mongoc-read-concern.h"
#include "mongoc-write-concern.h"
#include "mongoc-config.h"


#ifndef MONGOC_DEFAULT_PORT
#define MONGOC_DEFAULT_PORT 27017
#endif

#define MONGOC_URI_APPNAME "appname"
#define MONGOC_URI_AUTHMECHANISM "authmechanism"
#define MONGOC_URI_AUTHMECHANISMPROPERTIES "authmechanismproperties"
#define MONGOC_URI_AUTHSOURCE "authsource"
#define MONGOC_URI_CANONICALIZEHOSTNAME "canonicalizehostname"
#define MONGOC_URI_CONNECTTIMEOUTMS "connecttimeoutms"
#define MONGOC_URI_COMPRESSORS "compressors"
#define MONGOC_URI_GSSAPISERVICENAME "gssapiservicename"
#define MONGOC_URI_HEARTBEATFREQUENCYMS "heartbeatfrequencyms"
#define MONGOC_URI_JOURNAL "journal"
#define MONGOC_URI_LOCALTHRESHOLDMS "localthresholdms"
#define MONGOC_URI_MAXIDLETIMEMS "maxidletimems"
#define MONGOC_URI_MAXPOOLSIZE "maxpoolsize"
#define MONGOC_URI_MAXSTALENESSSECONDS "maxstalenessseconds"
#define MONGOC_URI_MINPOOLSIZE "minpoolsize"
#define MONGOC_URI_READCONCERNLEVEL "readconcernlevel"
#define MONGOC_URI_READPREFERENCE "readpreference"
#define MONGOC_URI_READPREFERENCETAGS "readpreferencetags"
#define MONGOC_URI_REPLICASET "replicaset"
#define MONGOC_URI_SAFE "safe"
#define MONGOC_URI_SERVERSELECTIONTIMEOUTMS "serverselectiontimeoutms"
#define MONGOC_URI_SERVERSELECTIONTRYONCE "serverselectiontryonce"
#define MONGOC_URI_SLAVEOK "slaveok"
#define MONGOC_URI_SOCKETCHECKINTERVALMS "socketcheckintervalms"
#define MONGOC_URI_SOCKETTIMEOUTMS "sockettimeoutms"
#define MONGOC_URI_SSL "ssl"
#define MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE "sslclientcertificatekeyfile"
#define MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD \
   "sslclientcertificatekeypassword"
#define MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE "sslcertificateauthorityfile"
#define MONGOC_URI_SSLALLOWINVALIDCERTIFICATES "sslallowinvalidcertificates"
#define MONGOC_URI_SSLALLOWINVALIDHOSTNAMES "sslallowinvalidhostnames"
#define MONGOC_URI_W "w"
#define MONGOC_URI_WAITQUEUEMULTIPLE "waitqueuemultiple"
#define MONGOC_URI_WAITQUEUETIMEOUTMS "waitqueuetimeoutms"
#define MONGOC_URI_WTIMEOUTMS "wtimeoutms"
#define MONGOC_URI_ZLIBCOMPRESSIONLEVEL "zlibcompressionlevel"

BSON_BEGIN_DECLS


typedef struct _mongoc_uri_t mongoc_uri_t;


MONGOC_EXPORT (mongoc_uri_t *)
mongoc_uri_copy (const mongoc_uri_t *uri);
MONGOC_EXPORT (void)
mongoc_uri_destroy (mongoc_uri_t *uri);
MONGOC_EXPORT (mongoc_uri_t *)
mongoc_uri_new (const char *uri_string) BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (mongoc_uri_t *)
mongoc_uri_new_with_error (const char *uri_string,
                           bson_error_t *error) BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (mongoc_uri_t *)
mongoc_uri_new_for_host_port (const char *hostname,
                              uint16_t port) BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_uri_get_hosts (const mongoc_uri_t *uri);
MONGOC_EXPORT (const char *)
mongoc_uri_get_database (const mongoc_uri_t *uri);
MONGOC_EXPORT (bool)
mongoc_uri_set_database (mongoc_uri_t *uri, const char *database);
MONGOC_EXPORT (const bson_t *)
mongoc_uri_get_compressors (const mongoc_uri_t *uri);
MONGOC_EXPORT (const bson_t *)
mongoc_uri_get_options (const mongoc_uri_t *uri);
MONGOC_EXPORT (const char *)
mongoc_uri_get_password (const mongoc_uri_t *uri);
MONGOC_EXPORT (bool)
mongoc_uri_set_password (mongoc_uri_t *uri, const char *password);
MONGOC_EXPORT (bool)
mongoc_uri_option_is_int32 (const char *key);
MONGOC_EXPORT (bool)
mongoc_uri_option_is_bool (const char *key);
MONGOC_EXPORT (bool)
mongoc_uri_option_is_utf8 (const char *key);
MONGOC_EXPORT (int32_t)
mongoc_uri_get_option_as_int32 (const mongoc_uri_t *uri,
                                const char *option,
                                int32_t fallback);
MONGOC_EXPORT (bool)
mongoc_uri_get_option_as_bool (const mongoc_uri_t *uri,
                               const char *option,
                               bool fallback);
MONGOC_EXPORT (const char *)
mongoc_uri_get_option_as_utf8 (const mongoc_uri_t *uri,
                               const char *option,
                               const char *fallback);
MONGOC_EXPORT (bool)
mongoc_uri_set_option_as_int32 (mongoc_uri_t *uri,
                                const char *option,
                                int32_t value);
MONGOC_EXPORT (bool)
mongoc_uri_set_option_as_bool (mongoc_uri_t *uri,
                               const char *option,
                               bool value);
MONGOC_EXPORT (bool)
mongoc_uri_set_option_as_utf8 (mongoc_uri_t *uri,
                               const char *option,
                               const char *value);
MONGOC_EXPORT (const bson_t *)
mongoc_uri_get_read_prefs (const mongoc_uri_t *uri)
   BSON_GNUC_DEPRECATED_FOR (mongoc_uri_get_read_prefs_t);
MONGOC_EXPORT (const char *)
mongoc_uri_get_replica_set (const mongoc_uri_t *uri);
MONGOC_EXPORT (const char *)
mongoc_uri_get_string (const mongoc_uri_t *uri);
MONGOC_EXPORT (const char *)
mongoc_uri_get_username (const mongoc_uri_t *uri);
MONGOC_EXPORT (bool)
mongoc_uri_set_username (mongoc_uri_t *uri, const char *username);
MONGOC_EXPORT (const bson_t *)
mongoc_uri_get_credentials (const mongoc_uri_t *uri);
MONGOC_EXPORT (const char *)
mongoc_uri_get_auth_source (const mongoc_uri_t *uri);
MONGOC_EXPORT (bool)
mongoc_uri_set_auth_source (mongoc_uri_t *uri, const char *value);
MONGOC_EXPORT (const char *)
mongoc_uri_get_appname (const mongoc_uri_t *uri);
MONGOC_EXPORT (bool)
mongoc_uri_set_appname (mongoc_uri_t *uri, const char *value);
MONGOC_EXPORT (bool)
mongoc_uri_set_compressors (mongoc_uri_t *uri, const char *value);
MONGOC_EXPORT (const char *)
mongoc_uri_get_auth_mechanism (const mongoc_uri_t *uri);
MONGOC_EXPORT (bool)
mongoc_uri_set_auth_mechanism (mongoc_uri_t *uri, const char *value);
MONGOC_EXPORT (bool)
mongoc_uri_get_mechanism_properties (const mongoc_uri_t *uri,
                                     bson_t *properties);
MONGOC_EXPORT (bool)
mongoc_uri_set_mechanism_properties (mongoc_uri_t *uri,
                                     const bson_t *properties);
MONGOC_EXPORT (bool)
mongoc_uri_get_ssl (const mongoc_uri_t *uri);
MONGOC_EXPORT (char *)
mongoc_uri_unescape (const char *escaped_string);
MONGOC_EXPORT (const mongoc_read_prefs_t *)
mongoc_uri_get_read_prefs_t (const mongoc_uri_t *uri);
MONGOC_EXPORT (void)
mongoc_uri_set_read_prefs_t (mongoc_uri_t *uri,
                             const mongoc_read_prefs_t *prefs);
MONGOC_EXPORT (const mongoc_write_concern_t *)
mongoc_uri_get_write_concern (const mongoc_uri_t *uri);
MONGOC_EXPORT (void)
mongoc_uri_set_write_concern (mongoc_uri_t *uri,
                              const mongoc_write_concern_t *wc);
MONGOC_EXPORT (const mongoc_read_concern_t *)
mongoc_uri_get_read_concern (const mongoc_uri_t *uri);
MONGOC_EXPORT (void)
mongoc_uri_set_read_concern (mongoc_uri_t *uri,
                             const mongoc_read_concern_t *rc);

BSON_END_DECLS


#endif /* MONGOC_URI_H */
