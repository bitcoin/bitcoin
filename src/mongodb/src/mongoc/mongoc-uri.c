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


#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>

/* strcasecmp on windows */
#include "mongoc-util-private.h"

#include "mongoc-config.h"
#include "mongoc-host-list.h"
#include "mongoc-host-list-private.h"
#include "mongoc-log.h"
#include "mongoc-handshake-private.h"
#include "mongoc-socket.h"
#include "mongoc-topology-private.h"
#include "mongoc-uri-private.h"
#include "mongoc-read-concern-private.h"
#include "mongoc-write-concern-private.h"
#include "mongoc-compression-private.h"


struct _mongoc_uri_t {
   char *str;
   mongoc_host_list_t *hosts;
   char *username;
   char *password;
   char *database;
   bson_t options;
   bson_t credentials;
   bson_t compressors;
   mongoc_read_prefs_t *read_prefs;
   mongoc_read_concern_t *read_concern;
   mongoc_write_concern_t *write_concern;
};

#define MONGOC_URI_ERROR(error, format, ...)         \
   bson_set_error (error,                            \
                   MONGOC_ERROR_COMMAND,             \
                   MONGOC_ERROR_COMMAND_INVALID_ARG, \
                   format,                           \
                   __VA_ARGS__);


static const char *escape_instructions = "Percent-encode username and password"
                                         " according to RFC 3986.";

bool
_mongoc_uri_set_option_as_int32 (mongoc_uri_t *uri,
                                 const char *option,
                                 int32_t value);

static void
mongoc_uri_do_unescape (char **str)
{
   char *tmp;

   if ((tmp = *str)) {
      *str = mongoc_uri_unescape (tmp);
      bson_free (tmp);
   }
}

bool
mongoc_uri_append_host (mongoc_uri_t *uri, const char *host, uint16_t port)
{
   mongoc_host_list_t *iter;
   mongoc_host_list_t *link_;

   if (strlen (host) > BSON_HOST_NAME_MAX) {
      MONGOC_ERROR ("Hostname provided in URI is too long, max is %d chars",
                    BSON_HOST_NAME_MAX);
      return false;
   }

   link_ = (mongoc_host_list_t *) bson_malloc0 (sizeof *link_);
   bson_strncpy (link_->host, host, sizeof link_->host);
   if (strchr (host, ':')) {
      bson_snprintf (link_->host_and_port,
                     sizeof link_->host_and_port,
                     "[%s]:%hu",
                     host,
                     port);
      link_->family = AF_INET6;
   } else if (strstr (host, ".sock")) {
      bson_snprintf (
         link_->host_and_port, sizeof link_->host_and_port, "%s", host);
      link_->family = AF_UNIX;
   } else {
      bson_snprintf (link_->host_and_port,
                     sizeof link_->host_and_port,
                     "%s:%hu",
                     host,
                     port);
      link_->family = AF_INET;
   }
   link_->host_and_port[sizeof link_->host_and_port - 1] = '\0';
   link_->port = port;

   if ((iter = uri->hosts)) {
      for (; iter && iter->next; iter = iter->next) {
      }
      iter->next = link_;
   } else {
      uri->hosts = link_;
   }

   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * scan_to_unichar --
 *
 *       Scans 'str' until either a character matching 'match' is found,
 *       until one of the characters in 'terminators' is encountered, or
 *       until we reach the end of 'str'.
 *
 *       NOTE: 'terminators' may not include multibyte UTF-8 characters.
 *
 * Returns:
 *       If 'match' is found, returns a copy of the section of 'str' before
 *       that character.  Otherwise, returns NULL.
 *
 * Side Effects:
 *       If 'match' is found, sets 'end' to begin at the matching character
 *       in 'str'.
 *
 *--------------------------------------------------------------------------
 */

static char *
scan_to_unichar (const char *str,
                 bson_unichar_t match,
                 const char *terminators,
                 const char **end)
{
   bson_unichar_t c;
   const char *iter;

   for (iter = str; iter && *iter && (c = bson_utf8_get_char (iter));
        iter = bson_utf8_next_char (iter)) {
      if (c == match) {
         *end = iter;
         return bson_strndup (str, iter - str);
      } else if (c == '\\') {
         iter = bson_utf8_next_char (iter);
         if (!bson_utf8_get_char (iter)) {
            break;
         }
      } else {
         const char *term_iter;
         for (term_iter = terminators; *term_iter; term_iter++) {
            if (c == *term_iter) {
               return NULL;
            }
         }
      }
   }

   return NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * ends_with --
 *
 *       Return true if str ends with suffix.
 *
 *--------------------------------------------------------------------------
 */
static bool
ends_with (const char *str, const char *suffix)
{
   size_t str_len = strlen (str);
   size_t suffix_len = strlen (suffix);
   const char *s1, *s2;

   if (str_len < suffix_len) {
      return false;
   }

   /* start at the ends of both strings */
   s1 = str + str_len;
   s2 = suffix + suffix_len;

   /* until either pointer reaches start of its string, compare the pointers */
   for (; s1 >= str && s2 >= suffix; s1--, s2--) {
      if (*s1 != *s2) {
         return false;
      }
   }

   return true;
}


static bool
mongoc_uri_parse_scheme (const char *str, const char **end)
{
   if (!!strncmp (str, "mongodb://", 10)) {
      return false;
   }

   *end = str + 10;

   return true;
}


static bool
mongoc_uri_has_unescaped_chars (const char *str, const char *chars)
{
   const char *c;
   const char *tmp;
   char *s;

   for (c = chars; *c; c++) {
      s = scan_to_unichar (str, (bson_unichar_t) *c, "", &tmp);
      if (s) {
         bson_free (s);
         return true;
      }
   }

   return false;
}


/* "str" is non-NULL, the part of URI between "mongodb://" and first "@" */
static bool
mongoc_uri_parse_userpass (mongoc_uri_t *uri,
                           const char *str,
                           bson_error_t *error)
{
   const char *prohibited = "@:/";
   const char *end_user;

   BSON_ASSERT (str);

   if ((uri->username = scan_to_unichar (str, ':', "", &end_user))) {
      uri->password = bson_strdup (end_user + 1);
   } else {
      uri->username = bson_strdup (str);
      uri->password = NULL;
   }

   if (mongoc_uri_has_unescaped_chars (uri->username, prohibited)) {
      MONGOC_URI_ERROR (error,
                        "Username \"%s\" must not have unescaped chars. %s",
                        uri->username,
                        escape_instructions);
      return false;
   }

   mongoc_uri_do_unescape (&uri->username);
   if (!uri->username) {
      MONGOC_URI_ERROR (
         error, "Incorrect URI escapes in username. %s", escape_instructions);
      return false;
   }

   /* Providing password at all is optional */
   if (uri->password) {
      if (mongoc_uri_has_unescaped_chars (uri->password, prohibited)) {
         MONGOC_URI_ERROR (error,
                           "Password \"%s\" must not have unescaped chars. %s",
                           uri->password,
                           escape_instructions);
         return false;
      }

      mongoc_uri_do_unescape (&uri->password);
      if (!uri->password) {
         MONGOC_URI_ERROR (error, "%s", "Incorrect URI escapes in password.");
         return false;
      }
   }

   return true;
}

static bool
mongoc_uri_parse_host6 (mongoc_uri_t *uri, const char *str)
{
   uint16_t port = MONGOC_DEFAULT_PORT;
   const char *portstr;
   const char *end_host;
   char *hostname;
   bool r;

   if ((portstr = strrchr (str, ':')) && !strstr (portstr, "]")) {
      if (!mongoc_parse_port (&port, portstr + 1)) {
         return false;
      }
   }

   hostname = scan_to_unichar (str + 1, ']', "", &end_host);

   mongoc_uri_do_unescape (&hostname);
   if (!hostname) {
      return false;
   }

   mongoc_lowercase (hostname, hostname);
   r = mongoc_uri_append_host (uri, hostname, port);
   bson_free (hostname);

   return r;
}


bool
mongoc_uri_parse_host (mongoc_uri_t *uri, const char *str, bool downcase)
{
   uint16_t port;
   const char *end_host;
   char *hostname;
   bool r;

   if (*str == '\0') {
      MONGOC_WARNING ("Empty hostname in URI");
      return false;
   }

   if (*str == '[' && strchr (str, ']')) {
      return mongoc_uri_parse_host6 (uri, str);
   }

   if ((hostname = scan_to_unichar (str, ':', "?/,", &end_host))) {
      end_host++;
      if (!mongoc_parse_port (&port, end_host)) {
         bson_free (hostname);
         return false;
      }
   } else {
      hostname = bson_strdup (str);
      port = MONGOC_DEFAULT_PORT;
   }

   if (mongoc_uri_has_unescaped_chars (hostname, "/")) {
      MONGOC_WARNING ("Unix Domain Sockets must be escaped (e.g. / = %%2F)");
      bson_free (hostname);
      return false;
   }

   mongoc_uri_do_unescape (&hostname);
   if (!hostname) {
      /* invalid */
      return false;
   }

   if (downcase) {
      mongoc_lowercase (hostname, hostname);
   }

   r = mongoc_uri_append_host (uri, hostname, port);
   bson_free (hostname);

   return r;
}


/* "hosts" is non-NULL, the part between "mongodb://" or "@" and last "/" */
static bool
mongoc_uri_parse_hosts (mongoc_uri_t *uri, const char *hosts)
{
   const char *next;
   const char *end_hostport;
   char *s;
   BSON_ASSERT (hosts);
   /*
    * Parsing the series of hosts is a lot more complicated than you might
    * imagine. This is due to some characters being both separators as well as
    * valid characters within the "hostname". In particularly, we can have file
    * paths to specify paths to UNIX domain sockets. We impose the restriction
    * that they must be suffixed with ".sock" to simplify the parsing.
    *
    * You can separate hosts and file system paths to UNIX domain sockets with
    * ",".
    */
   s = scan_to_unichar (hosts, '?', "", &end_hostport);
   if (s) {
      MONGOC_WARNING (
         "%s", "A '/' is required between the host list and any options.");
      goto error;
   }
   next = hosts;
   do {
      /* makes a copy of the section of the string */
      s = scan_to_unichar (next, ',', "", &end_hostport);
      if (s) {
         next = (char *) end_hostport + 1;
      } else {
         s = bson_strdup (next);
         next = NULL;
      }
      if (ends_with (s, ".sock")) {
         if (!mongoc_uri_parse_host (uri, s, false /* downcase */)) {
            goto error;
         }
      } else if (!mongoc_uri_parse_host (uri, s, true /* downcase */)) {
         goto error;
      }
      bson_free (s);
   } while (next);
   return true;
error:
   bson_free (s);
   return false;
}


static bool
mongoc_uri_parse_database (mongoc_uri_t *uri, const char *str, const char **end)
{
   const char *end_database;
   const char *c;
   char *invalid_c;
   const char *tmp;

   if ((uri->database = scan_to_unichar (str, '?', "", &end_database))) {
      *end = end_database;
   } else if (*str) {
      uri->database = bson_strdup (str);
      *end = str + strlen (str);
   }

   mongoc_uri_do_unescape (&uri->database);
   if (!uri->database) {
      /* invalid */
      return false;
   }

   /* invalid characters in database name */
   for (c = "/\\. \"$"; *c; c++) {
      invalid_c =
         scan_to_unichar (uri->database, (bson_unichar_t) *c, "", &tmp);
      if (invalid_c) {
         bson_free (invalid_c);
         return false;
      }
   }

   return true;
}


static bool
mongoc_uri_parse_auth_mechanism_properties (mongoc_uri_t *uri, const char *str)
{
   char *field;
   char *value;
   const char *end_scan;
   bson_t properties;

   bson_init (&properties);

   /* build up the properties document */
   while ((field = scan_to_unichar (str, ':', "&", &end_scan))) {
      str = end_scan + 1;
      if (!(value = scan_to_unichar (str, ',', ":&", &end_scan))) {
         value = bson_strdup (str);
         str = "";
      } else {
         str = end_scan + 1;
      }
      bson_append_utf8 (&properties, field, -1, value, -1);
      bson_free (field);
      bson_free (value);
   }

   /* append our auth properties to our credentials */
   mongoc_uri_set_mechanism_properties (uri, &properties);
   return true;
}

static bool
mongoc_uri_parse_tags (mongoc_uri_t *uri, /* IN */
                       const char *str)   /* IN */
{
   const char *end_keyval;
   const char *end_key;
   bson_t b;
   char *keyval;
   char *key;

   bson_init (&b);

again:
   if ((keyval = scan_to_unichar (str, ',', "", &end_keyval))) {
      if (!(key = scan_to_unichar (keyval, ':', "", &end_key))) {
         bson_free (keyval);
         goto fail;
      }

      bson_append_utf8 (&b, key, -1, end_key + 1, -1);
      bson_free (key);
      bson_free (keyval);
      str = end_keyval + 1;
      goto again;
   } else if ((key = scan_to_unichar (str, ':', "", &end_key))) {
      bson_append_utf8 (&b, key, -1, end_key + 1, -1);
      bson_free (key);
   } else if (strlen (str)) {
      /* we're not finished but we couldn't parse the string */
      goto fail;
   }

   mongoc_read_prefs_add_tag (uri->read_prefs, &b);
   bson_destroy (&b);

   return true;

fail:
   MONGOC_WARNING ("Unsupported value for \"" MONGOC_URI_READPREFERENCETAGS
                   "\": \"%s\"",
                   str);
   bson_destroy (&b);
   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_bson_append_or_replace_key --
 *
 *
 *       Appends 'option' to the end of 'options' if not already set.
 *
 *       Since we cannot grow utf8 strings inline, we have to allocate a
 *       temporary bson variable and splice in the new value if the key
 *       is already set.
 *
 *       NOTE: This function keeps the order of the BSON keys.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 *
 *--------------------------------------------------------------------------
 */

static void
mongoc_uri_bson_append_or_replace_key (bson_t *options,
                                       const char *option,
                                       const char *value)
{
   bson_iter_t iter;
   bool found = false;

   if (bson_iter_init (&iter, options)) {
      bson_t tmp = BSON_INITIALIZER;

      while (bson_iter_next (&iter)) {
         const bson_value_t *bvalue;

         if (!strcasecmp (bson_iter_key (&iter), option)) {
            bson_append_utf8 (&tmp, option, -1, value, -1);
            found = true;
            continue;
         }

         bvalue = bson_iter_value (&iter);
         BSON_APPEND_VALUE (&tmp, bson_iter_key (&iter), bvalue);
      }

      if (!found) {
         bson_append_utf8 (&tmp, option, -1, value, -1);
      }

      bson_destroy (options);
      bson_copy_to (&tmp, options);
      bson_destroy (&tmp);
   }
}


bool
mongoc_uri_option_is_int32 (const char *key)
{
   return !strcasecmp (key, MONGOC_URI_CONNECTTIMEOUTMS) ||
          !strcasecmp (key, MONGOC_URI_HEARTBEATFREQUENCYMS) ||
          !strcasecmp (key, MONGOC_URI_SERVERSELECTIONTIMEOUTMS) ||
          !strcasecmp (key, MONGOC_URI_SOCKETCHECKINTERVALMS) ||
          !strcasecmp (key, MONGOC_URI_SOCKETTIMEOUTMS) ||
          !strcasecmp (key, MONGOC_URI_LOCALTHRESHOLDMS) ||
          !strcasecmp (key, MONGOC_URI_MAXPOOLSIZE) ||
          !strcasecmp (key, MONGOC_URI_MAXSTALENESSSECONDS) ||
          !strcasecmp (key, MONGOC_URI_MINPOOLSIZE) ||
          !strcasecmp (key, MONGOC_URI_MAXIDLETIMEMS) ||
          !strcasecmp (key, MONGOC_URI_WAITQUEUEMULTIPLE) ||
          !strcasecmp (key, MONGOC_URI_WAITQUEUETIMEOUTMS) ||
          !strcasecmp (key, MONGOC_URI_WTIMEOUTMS) ||
          !strcasecmp (key, MONGOC_URI_ZLIBCOMPRESSIONLEVEL);
}

bool
mongoc_uri_option_is_bool (const char *key)
{
   return !strcasecmp (key, MONGOC_URI_CANONICALIZEHOSTNAME) ||
          !strcasecmp (key, MONGOC_URI_JOURNAL) ||
          !strcasecmp (key, MONGOC_URI_SAFE) ||
          !strcasecmp (key, MONGOC_URI_SERVERSELECTIONTRYONCE) ||
          !strcasecmp (key, MONGOC_URI_SLAVEOK) ||
          !strcasecmp (key, MONGOC_URI_SSL) ||
          !strcasecmp (key, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES) ||
          !strcasecmp (key, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES);
}

bool
mongoc_uri_option_is_utf8 (const char *key)
{
   return !strcasecmp (key, MONGOC_URI_APPNAME) ||
          !strcasecmp (key, MONGOC_URI_GSSAPISERVICENAME) ||
          !strcasecmp (key, MONGOC_URI_REPLICASET) ||
          !strcasecmp (key, MONGOC_URI_READPREFERENCE) ||
          !strcasecmp (key, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE) ||
          !strcasecmp (key, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD) ||
          !strcasecmp (key, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE);
}

static bool
mongoc_uri_parse_int32 (const char *key, const char *value, int32_t *result)
{
   char *endptr;
   int64_t i;

   errno = 0;
   i = bson_ascii_strtoll (value, &endptr, 10);
   if (errno || endptr < value + strlen (value)) {
      MONGOC_WARNING ("Invalid %s: cannot parse integer\n", key);
      return false;
   }

   if (i > INT32_MAX || i < INT32_MIN) {
      MONGOC_WARNING ("Invalid %s: cannot fit in int32\n", key);
      return false;
   }

   *result = (int32_t) i;
   return true;
}

static bool
mongoc_uri_parse_option (mongoc_uri_t *uri, const char *str)
{
   int32_t v_int;
   const char *end_key;
   char *lkey = NULL;
   char *key = NULL;
   char *value = NULL;
   bool ret = false;

   if (!(key = scan_to_unichar (str, '=', "", &end_key))) {
      goto CLEANUP;
   }

   value = bson_strdup (end_key + 1);
   mongoc_uri_do_unescape (&value);
   if (!value) {
      /* do_unescape detected invalid UTF-8 and freed value */
      goto CLEANUP;
   }

   lkey = bson_strdup (key);
   mongoc_lowercase (key, lkey);

   if (bson_has_field (&uri->options, lkey)) {
      MONGOC_WARNING ("Overwriting previously provided value for '%s'", key);
   }

   if (mongoc_uri_option_is_int32 (lkey)) {
      if (!mongoc_uri_parse_int32 (lkey, value, &v_int)) {
         goto UNSUPPORTED_VALUE;
      }

      if (!mongoc_uri_set_option_as_int32 (uri, lkey, v_int)) {
         goto CLEANUP;
      }
   } else if (!strcmp (lkey, MONGOC_URI_W)) {
      if (*value == '-' || isdigit (*value)) {
         v_int = (int) strtol (value, NULL, 10);
         _mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_W, v_int);
      } else if (0 == strcasecmp (value, "majority")) {
         mongoc_uri_bson_append_or_replace_key (
            &uri->options, MONGOC_URI_W, "majority");
      } else if (*value) {
         mongoc_uri_bson_append_or_replace_key (
            &uri->options, MONGOC_URI_W, value);
      }
   } else if (mongoc_uri_option_is_bool (lkey)) {
      if (0 == strcasecmp (value, "true")) {
         mongoc_uri_set_option_as_bool (uri, lkey, true);
      } else if (0 == strcasecmp (value, "false")) {
         mongoc_uri_set_option_as_bool (uri, lkey, false);
      } else if ((0 == strcmp (value, "1")) ||
                 (0 == strcasecmp (value, "yes")) ||
                 (0 == strcasecmp (value, "y")) ||
                 (0 == strcasecmp (value, "t"))) {
         MONGOC_WARNING ("Deprecated boolean value for \"%1$s\": \"%2$s\", "
                         "please update to \"%1$s=true\"",
                         key,
                         value);
         mongoc_uri_set_option_as_bool (uri, lkey, true);
      } else if ((0 == strcasecmp (value, "0")) ||
                 (0 == strcasecmp (value, "-1")) ||
                 (0 == strcmp (value, "no")) || (0 == strcmp (value, "n")) ||
                 (0 == strcmp (value, "f"))) {
         MONGOC_WARNING ("Deprecated boolean value for \"%1$s\": \"%2$s\", "
                         "please update to \"%1$s=false\"",
                         key,
                         value);
         mongoc_uri_set_option_as_bool (uri, lkey, false);
      } else {
         goto UNSUPPORTED_VALUE;
      }
   } else if (!strcmp (lkey, MONGOC_URI_READPREFERENCETAGS)) {
      /* Allows providing this key multiple times */
      if (!mongoc_uri_parse_tags (uri, value)) {
         goto UNSUPPORTED_VALUE;
      }
   } else if (!strcmp (lkey, MONGOC_URI_AUTHMECHANISM) ||
              !strcmp (lkey, MONGOC_URI_AUTHSOURCE)) {
      if (bson_has_field (&uri->credentials, lkey)) {
         MONGOC_WARNING ("Overwriting previously provided value for '%s'", key);
      }
      mongoc_uri_bson_append_or_replace_key (&uri->credentials, lkey, value);
   } else if (!strcmp (lkey, MONGOC_URI_READCONCERNLEVEL)) {
      if (!mongoc_read_concern_is_default (uri->read_concern)) {
         MONGOC_WARNING ("Overwriting previously provided value for '%s'", key);
      }
      mongoc_read_concern_set_level (uri->read_concern, value);
   } else if (!strcmp (lkey, MONGOC_URI_AUTHMECHANISMPROPERTIES)) {
      if (bson_has_field (&uri->credentials, lkey)) {
         MONGOC_WARNING ("Overwriting previously provided value for '%s'", key);
      }
      if (!mongoc_uri_parse_auth_mechanism_properties (uri, value)) {
         goto UNSUPPORTED_VALUE;
      }
   } else if (!strcmp (lkey, MONGOC_URI_APPNAME)) {
      /* Part of uri->options */
      if (!mongoc_uri_set_appname (uri, value)) {
         goto UNSUPPORTED_VALUE;
      }
   } else if (!strcmp (lkey, MONGOC_URI_COMPRESSORS)) {
      if (!mongoc_uri_set_compressors (uri, value)) {
         goto UNSUPPORTED_VALUE;
      }
   } else if (mongoc_uri_option_is_utf8 (lkey)) {
      mongoc_uri_bson_append_or_replace_key (&uri->options, lkey, value);
   } else {
      /*
       * Keys that aren't supported by a driver MUST be ignored.
       *
       * A WARN level logging message MUST be issued
       * https://github.com/mongodb/specifications/blob/master/source/connection-string/connection-string-spec.rst#keys
       */
      MONGOC_WARNING ("Unsupported URI option \"%s\"", key);
   }

   ret = true;

UNSUPPORTED_VALUE:
   if (!ret) {
      MONGOC_WARNING ("Unsupported value for \"%s\": \"%s\"", key, value);
   }

CLEANUP:
   bson_free (key);
   bson_free (lkey);
   bson_free (value);

   return ret;
}


static bool
mongoc_uri_parse_options (mongoc_uri_t *uri,
                          const char *str,
                          bson_error_t *error)
{
   const char *end_option;
   char *option;

again:
   if ((option = scan_to_unichar (str, '&', "", &end_option))) {
      if (!mongoc_uri_parse_option (uri, option)) {
         MONGOC_URI_ERROR (error, "Unknown option or value for '%s'", option);
         bson_free (option);
         return false;
      }
      bson_free (option);
      str = end_option + 1;
      goto again;
   } else if (*str) {
      if (!mongoc_uri_parse_option (uri, str)) {
         MONGOC_URI_ERROR (error, "Unknown option or value for '%s'", str);
         return false;
      }
   }

   return true;
}


static bool
mongoc_uri_finalize_auth (mongoc_uri_t *uri, bson_error_t *error)
{
   bson_iter_t iter;
   const char *source = NULL;
   const char *mechanism = mongoc_uri_get_auth_mechanism (uri);

   if (bson_iter_init_find_case (
          &iter, &uri->credentials, MONGOC_URI_AUTHSOURCE)) {
      source = bson_iter_utf8 (&iter, NULL);
   }

   /* authSource with GSSAPI or X509 should always be external */
   if (mechanism) {
      if (!strcasecmp (mechanism, "GSSAPI") ||
          !strcasecmp (mechanism, "MONGODB-X509")) {
         if (source) {
            if (strcasecmp (source, "$external")) {
               MONGOC_URI_ERROR (
                  error,
                  "%s",
                  "GSSAPI and X509 require \"$external\" authSource");
               return false;
            }
         } else {
            bson_append_utf8 (
               &uri->credentials, MONGOC_URI_AUTHSOURCE, -1, "$external", -1);
         }
      }
   }
   return true;
}

static bool
mongoc_uri_parse_before_slash (mongoc_uri_t *uri,
                               const char *before_slash,
                               bson_error_t *error)
{
   char *userpass;
   const char *hosts;

   userpass = scan_to_unichar (before_slash, '@', "", &hosts);
   if (userpass) {
      if (!mongoc_uri_parse_userpass (uri, userpass, error)) {
         goto error;
      }

      hosts++; /* advance past "@" */
      if (*hosts == '@') {
         /* special case: "mongodb://alice@@localhost" */
         MONGOC_URI_ERROR (
            error, "Invalid username or password. %s", escape_instructions);
         goto error;
      }
   } else {
      hosts = before_slash;
   }

   if (!mongoc_uri_parse_hosts (uri, hosts)) {
      MONGOC_URI_ERROR (error, "%s", "Invalid host string in URI");
      goto error;
   }

   bson_free (userpass);
   return true;

error:
   bson_free (userpass);
   return false;
}


static bool
mongoc_uri_parse (mongoc_uri_t *uri, const char *str, bson_error_t *error)
{
   char *before_slash = NULL;
   const char *tmp;

   if (!mongoc_uri_parse_scheme (str, &str)) {
      MONGOC_URI_ERROR (
         error, "%s", "Invalid URI Schema, expecting 'mongodb://'");
      goto error;
   }

   before_slash = scan_to_unichar (str, '/', "", &tmp);
   if (!before_slash) {
      before_slash = bson_strdup (str);
      str += strlen (before_slash);
   } else {
      str = tmp;
   }

   if (!mongoc_uri_parse_before_slash (uri, before_slash, error)) {
      goto error;
   }

   if (*str) {
      if (*str == '/') {
         str++;
         if (*str) {
            if (!mongoc_uri_parse_database (uri, str, &str)) {
               MONGOC_URI_ERROR (error, "%s", "Invalid database name in URI");
               goto error;
            }
         }

         if (*str == '?') {
            str++;
            if (*str) {
               if (!mongoc_uri_parse_options (uri, str, error)) {
                  goto error;
               }
            }
         }
      } else {
         MONGOC_URI_ERROR (error, "%s", "Expected end of hostname delimiter");
         goto error;
      }
   }

   if (!mongoc_uri_finalize_auth (uri, error)) {
      goto error;
   }

   bson_free (before_slash);
   return true;

error:
   bson_free (before_slash);
   return false;
}


const mongoc_host_list_t *
mongoc_uri_get_hosts (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return uri->hosts;
}


const char *
mongoc_uri_get_replica_set (const mongoc_uri_t *uri)
{
   bson_iter_t iter;

   BSON_ASSERT (uri);

   if (bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_REPLICASET) &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      return bson_iter_utf8 (&iter, NULL);
   }

   return NULL;
}


const bson_t *
mongoc_uri_get_credentials (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return &uri->credentials;
}


const char *
mongoc_uri_get_auth_mechanism (const mongoc_uri_t *uri)
{
   bson_iter_t iter;

   BSON_ASSERT (uri);

   if (bson_iter_init_find_case (
          &iter, &uri->credentials, MONGOC_URI_AUTHMECHANISM) &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      return bson_iter_utf8 (&iter, NULL);
   }

   return NULL;
}


bool
mongoc_uri_set_auth_mechanism (mongoc_uri_t *uri, const char *value)
{
   size_t len;

   BSON_ASSERT (value);

   len = strlen (value);

   if (!bson_utf8_validate (value, len, false)) {
      return false;
   }

   mongoc_uri_bson_append_or_replace_key (
      &uri->credentials, MONGOC_URI_AUTHMECHANISM, value);

   return true;
}


bool
mongoc_uri_get_mechanism_properties (const mongoc_uri_t *uri,
                                     bson_t *properties /* OUT */)
{
   bson_iter_t iter;

   BSON_ASSERT (uri);
   BSON_ASSERT (properties);

   if (bson_iter_init_find_case (
          &iter, &uri->credentials, MONGOC_URI_AUTHMECHANISMPROPERTIES) &&
       BSON_ITER_HOLDS_DOCUMENT (&iter)) {
      uint32_t len = 0;
      const uint8_t *data = NULL;

      bson_iter_document (&iter, &len, &data);
      bson_init_static (properties, data, len);

      return true;
   }

   return false;
}


bool
mongoc_uri_set_mechanism_properties (mongoc_uri_t *uri,
                                     const bson_t *properties)
{
   bson_iter_t iter;
   bson_t tmp = BSON_INITIALIZER;
   bool r;

   BSON_ASSERT (uri);
   BSON_ASSERT (properties);

   if (bson_iter_init_find (
          &iter, &uri->credentials, MONGOC_URI_AUTHMECHANISMPROPERTIES)) {
      /* copy all elements to tmp besides authMechanismProperties */
      bson_copy_to_excluding_noinit (&uri->credentials,
                                     &tmp,
                                     MONGOC_URI_AUTHMECHANISMPROPERTIES,
                                     (char *) NULL);

      r = BSON_APPEND_DOCUMENT (
         &tmp, MONGOC_URI_AUTHMECHANISMPROPERTIES, properties);
      if (!r) {
         bson_destroy (&tmp);
         return false;
      }

      bson_destroy (&uri->credentials);
      bson_copy_to (&tmp, &uri->credentials);
      bson_destroy (&tmp);

      return true;
   } else {
      return BSON_APPEND_DOCUMENT (
         &uri->credentials, MONGOC_URI_AUTHMECHANISMPROPERTIES, properties);
   }
}


static bool
_mongoc_uri_assign_read_prefs_mode (mongoc_uri_t *uri, bson_error_t *error)
{
   const char *str;
   bson_iter_t iter;

   BSON_ASSERT (uri);

   if (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SLAVEOK, false)) {
      mongoc_read_prefs_set_mode (uri->read_prefs,
                                  MONGOC_READ_SECONDARY_PREFERRED);
   }

   if (bson_iter_init_find_case (
          &iter, &uri->options, MONGOC_URI_READPREFERENCE) &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      str = bson_iter_utf8 (&iter, NULL);

      if (0 == strcasecmp ("primary", str)) {
         mongoc_read_prefs_set_mode (uri->read_prefs, MONGOC_READ_PRIMARY);
      } else if (0 == strcasecmp ("primarypreferred", str)) {
         mongoc_read_prefs_set_mode (uri->read_prefs,
                                     MONGOC_READ_PRIMARY_PREFERRED);
      } else if (0 == strcasecmp ("secondary", str)) {
         mongoc_read_prefs_set_mode (uri->read_prefs, MONGOC_READ_SECONDARY);
      } else if (0 == strcasecmp ("secondarypreferred", str)) {
         mongoc_read_prefs_set_mode (uri->read_prefs,
                                     MONGOC_READ_SECONDARY_PREFERRED);
      } else if (0 == strcasecmp ("nearest", str)) {
         mongoc_read_prefs_set_mode (uri->read_prefs, MONGOC_READ_NEAREST);
      } else {
         MONGOC_URI_ERROR (
            error,
            "Unsupported readPreference value [readPreference=%s].",
            str);
         return false;
      }
   }
   return true;
}


static void
_mongoc_uri_build_write_concern (mongoc_uri_t *uri) /* IN */
{
   mongoc_write_concern_t *write_concern;
   const char *str;
   bson_iter_t iter;
   int32_t wtimeoutms;
   int value;

   BSON_ASSERT (uri);

   write_concern = mongoc_write_concern_new ();

   if (bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_SAFE) &&
       BSON_ITER_HOLDS_BOOL (&iter)) {
      mongoc_write_concern_set_w (
         write_concern,
         bson_iter_bool (&iter) ? 1 : MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
   }

   wtimeoutms = mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 0);

   if (bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_JOURNAL) &&
       BSON_ITER_HOLDS_BOOL (&iter)) {
      mongoc_write_concern_set_journal (write_concern, bson_iter_bool (&iter));
   }

   if (bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_W)) {
      if (BSON_ITER_HOLDS_INT32 (&iter)) {
         value = bson_iter_int32 (&iter);

         switch (value) {
         case MONGOC_WRITE_CONCERN_W_ERRORS_IGNORED:
         case MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED:
            /* Warn on conflict, since write concern will be validated later */
            if (mongoc_write_concern_get_journal (write_concern)) {
               MONGOC_WARNING ("Journal conflicts with w value [w=%d].", value);
            }
            mongoc_write_concern_set_w (write_concern, value);
            break;
         default:
            if (value > 0) {
               mongoc_write_concern_set_w (write_concern, value);
               if (value > 1) {
                  mongoc_write_concern_set_wtimeout (write_concern, wtimeoutms);
               }
               break;
            }
            MONGOC_WARNING ("Unsupported w value [w=%d].", value);
            break;
         }
      } else if (BSON_ITER_HOLDS_UTF8 (&iter)) {
         str = bson_iter_utf8 (&iter, NULL);

         if (0 == strcasecmp ("majority", str)) {
            mongoc_write_concern_set_wmajority (write_concern, wtimeoutms);
         } else {
            mongoc_write_concern_set_wtag (write_concern, str);
            mongoc_write_concern_set_wtimeout (write_concern, wtimeoutms);
         }
      } else {
         BSON_ASSERT (false);
      }
   }

   uri->write_concern = write_concern;
}

/* can't use mongoc_uri_get_option_as_int32, it treats 0 specially */
static int32_t
_mongoc_uri_get_max_staleness_option (const mongoc_uri_t *uri)
{
   const bson_t *options;
   bson_iter_t iter;
   int32_t retval = MONGOC_NO_MAX_STALENESS;

   if ((options = mongoc_uri_get_options (uri)) &&
       bson_iter_init_find_case (
          &iter, options, MONGOC_URI_MAXSTALENESSSECONDS) &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      retval = bson_iter_int32 (&iter);
      if (retval == 0) {
         MONGOC_WARNING (
            "Unsupported value for \"" MONGOC_URI_MAXSTALENESSSECONDS
            "\": \"%d\"",
            retval);
         retval = -1;
      } else if (retval < 0 && retval != -1) {
         MONGOC_WARNING (
            "Unsupported value for \"" MONGOC_URI_MAXSTALENESSSECONDS
            "\": \"%d\"",
            retval);
         retval = MONGOC_NO_MAX_STALENESS;
      }
   }

   return retval;
}

mongoc_uri_t *
mongoc_uri_new_with_error (const char *uri_string, bson_error_t *error)
{
   mongoc_uri_t *uri;
   int32_t max_staleness_seconds;

   uri = (mongoc_uri_t *) bson_malloc0 (sizeof *uri);
   bson_init (&uri->options);
   bson_init (&uri->credentials);
   bson_init (&uri->compressors);

   /* Initialize read_prefs, since parsing may add to it */
   uri->read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   /* Initialize empty read_concern */
   uri->read_concern = mongoc_read_concern_new ();

   if (!uri_string) {
      uri_string = "mongodb://127.0.0.1/";
   }

   if (!mongoc_uri_parse (uri, uri_string, error)) {
      mongoc_uri_destroy (uri);
      return NULL;
   }

   uri->str = bson_strdup (uri_string);

   if (!_mongoc_uri_assign_read_prefs_mode (uri, error)) {
      mongoc_uri_destroy (uri);
      return NULL;
   }
   max_staleness_seconds = _mongoc_uri_get_max_staleness_option (uri);
   mongoc_read_prefs_set_max_staleness_seconds (uri->read_prefs,
                                                max_staleness_seconds);

   if (!mongoc_read_prefs_is_valid (uri->read_prefs)) {
      mongoc_uri_destroy (uri);
      MONGOC_URI_ERROR (error, "%s", "Invalid readPreferences");
      return NULL;
   }

   _mongoc_uri_build_write_concern (uri);

   if (!mongoc_write_concern_is_valid (uri->write_concern)) {
      mongoc_uri_destroy (uri);
      MONGOC_URI_ERROR (error, "%s", "Invalid writeConcern");
      return NULL;
   }

   return uri;
}

mongoc_uri_t *
mongoc_uri_new (const char *uri_string)
{
   bson_error_t error = {0};
   mongoc_uri_t *uri;

   uri = mongoc_uri_new_with_error (uri_string, &error);
   if (error.domain) {
      MONGOC_WARNING ("Error parsing URI: '%s'", error.message);
   }

   return uri;
}


mongoc_uri_t *
mongoc_uri_new_for_host_port (const char *hostname, uint16_t port)
{
   mongoc_uri_t *uri;
   char *str;

   BSON_ASSERT (hostname);
   BSON_ASSERT (port);

   str = bson_strdup_printf ("mongodb://%s:%hu/", hostname, port);
   uri = mongoc_uri_new (str);
   bson_free (str);

   return uri;
}


const char *
mongoc_uri_get_username (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);

   return uri->username;
}

bool
mongoc_uri_set_username (mongoc_uri_t *uri, const char *username)
{
   size_t len;

   BSON_ASSERT (username);

   len = strlen (username);

   if (!bson_utf8_validate (username, len, false)) {
      return false;
   }

   if (uri->username) {
      bson_free (uri->username);
   }

   uri->username = bson_strdup (username);
   return true;
}


const char *
mongoc_uri_get_password (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);

   return uri->password;
}

bool
mongoc_uri_set_password (mongoc_uri_t *uri, const char *password)
{
   size_t len;

   BSON_ASSERT (password);

   len = strlen (password);

   if (!bson_utf8_validate (password, len, false)) {
      return false;
   }

   if (uri->password) {
      bson_free (uri->password);
   }

   uri->password = bson_strdup (password);
   return true;
}


const char *
mongoc_uri_get_database (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return uri->database;
}

bool
mongoc_uri_set_database (mongoc_uri_t *uri, const char *database)
{
   size_t len;

   BSON_ASSERT (database);

   len = strlen (database);

   if (!bson_utf8_validate (database, len, false)) {
      return false;
   }

   if (uri->database) {
      bson_free (uri->database);
   }

   uri->database = bson_strdup (database);
   return true;
}


const char *
mongoc_uri_get_auth_source (const mongoc_uri_t *uri)
{
   bson_iter_t iter;

   BSON_ASSERT (uri);

   if (bson_iter_init_find_case (
          &iter, &uri->credentials, MONGOC_URI_AUTHSOURCE)) {
      return bson_iter_utf8 (&iter, NULL);
   }

   return uri->database ? uri->database : "admin";
}


bool
mongoc_uri_set_auth_source (mongoc_uri_t *uri, const char *value)
{
   size_t len;

   BSON_ASSERT (value);

   len = strlen (value);

   if (!bson_utf8_validate (value, len, false)) {
      return false;
   }

   mongoc_uri_bson_append_or_replace_key (
      &uri->credentials, MONGOC_URI_AUTHSOURCE, value);

   return true;
}


const char *
mongoc_uri_get_appname (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);

   return mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_APPNAME, NULL);
}


bool
mongoc_uri_set_appname (mongoc_uri_t *uri, const char *value)
{
   BSON_ASSERT (value);

   if (!bson_utf8_validate (value, strlen (value), false)) {
      return false;
   }

   if (!_mongoc_handshake_appname_is_valid (value)) {
      return false;
   }

   mongoc_uri_bson_append_or_replace_key (
      &uri->options, MONGOC_URI_APPNAME, value);

   return true;
}

bool
mongoc_uri_set_compressors (mongoc_uri_t *uri, const char *value)
{
   const char *end_compressor;
   char *entry;

   bson_destroy (&uri->compressors);
   bson_init (&uri->compressors);

   if (value && !bson_utf8_validate (value, strlen (value), false)) {
      return false;
   }
   while ((entry = scan_to_unichar (value, ',', "", &end_compressor))) {
      if (mongoc_compressor_supported (entry)) {
         mongoc_uri_bson_append_or_replace_key (
            &uri->compressors, entry, "yes");
      } else {
         MONGOC_WARNING ("Unsupported compressor: '%s'", entry);
      }
      value = end_compressor + 1;
      bson_free (entry);
   }
   if (value) {
      if (mongoc_compressor_supported (value)) {
         mongoc_uri_bson_append_or_replace_key (
            &uri->compressors, value, "yes");
      } else {
         MONGOC_WARNING ("Unsupported compressor: '%s'", value);
      }
   }

   return true;
}

const bson_t *
mongoc_uri_get_compressors (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return &uri->compressors;
}


/* can't use mongoc_uri_get_option_as_int32, it treats 0 specially */
int32_t
mongoc_uri_get_local_threshold_option (const mongoc_uri_t *uri)
{
   const bson_t *options;
   bson_iter_t iter;
   int32_t retval = MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS;

   if ((options = mongoc_uri_get_options (uri)) &&
       bson_iter_init_find_case (&iter, options, "localthresholdms") &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      retval = bson_iter_int32 (&iter);

      if (retval < 0) {
         MONGOC_WARNING ("Invalid localThresholdMS: %d", retval);
         retval = MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS;
      }
   }

   return retval;
}

const bson_t *
mongoc_uri_get_options (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return &uri->options;
}


void
mongoc_uri_destroy (mongoc_uri_t *uri)
{
   if (uri) {
      _mongoc_host_list_destroy_all (uri->hosts);
      bson_free (uri->str);
      bson_free (uri->database);
      bson_free (uri->username);
      bson_destroy (&uri->options);
      bson_destroy (&uri->credentials);
      bson_destroy (&uri->compressors);
      mongoc_read_prefs_destroy (uri->read_prefs);
      mongoc_read_concern_destroy (uri->read_concern);
      mongoc_write_concern_destroy (uri->write_concern);

      if (uri->password) {
         bson_zero_free (uri->password, strlen (uri->password));
      }

      bson_free (uri);
   }
}


mongoc_uri_t *
mongoc_uri_copy (const mongoc_uri_t *uri)
{
   mongoc_uri_t *copy;
   mongoc_host_list_t *iter;

   BSON_ASSERT (uri);

   copy = (mongoc_uri_t *) bson_malloc0 (sizeof (*copy));

   copy->str = bson_strdup (uri->str);
   copy->username = bson_strdup (uri->username);
   copy->password = bson_strdup (uri->password);
   copy->database = bson_strdup (uri->database);

   copy->read_prefs = mongoc_read_prefs_copy (uri->read_prefs);
   copy->read_concern = mongoc_read_concern_copy (uri->read_concern);
   copy->write_concern = mongoc_write_concern_copy (uri->write_concern);

   for (iter = uri->hosts; iter; iter = iter->next) {
      if (!mongoc_uri_append_host (copy, iter->host, iter->port)) {
         mongoc_uri_destroy (copy);
         return NULL;
      }
   }

   bson_copy_to (&uri->options, &copy->options);
   bson_copy_to (&uri->credentials, &copy->credentials);
   bson_copy_to (&uri->compressors, &copy->compressors);

   return copy;
}


const char *
mongoc_uri_get_string (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return uri->str;
}


const bson_t *
mongoc_uri_get_read_prefs (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return mongoc_read_prefs_get_tags (uri->read_prefs);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_unescape --
 *
 *       Escapes an UTF-8 encoded string containing URI escaped segments
 *       such as %20.
 *
 *       It is a programming error to call this function with a string
 *       that is not UTF-8 encoded!
 *
 * Returns:
 *       A newly allocated string that should be freed with bson_free()
 *       or NULL on failure, such as invalid % encoding.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

char *
mongoc_uri_unescape (const char *escaped_string)
{
   bson_unichar_t c;
   bson_string_t *str;
   unsigned int hex = 0;
   const char *ptr;
   const char *end;
   size_t len;

   BSON_ASSERT (escaped_string);

   len = strlen (escaped_string);

   /*
    * Double check that this is a UTF-8 valid string. Bail out if necessary.
    */
   if (!bson_utf8_validate (escaped_string, len, false)) {
      MONGOC_WARNING ("%s(): escaped_string contains invalid UTF-8", BSON_FUNC);
      return NULL;
   }

   ptr = escaped_string;
   end = ptr + len;
   str = bson_string_new (NULL);

   for (; *ptr; ptr = bson_utf8_next_char (ptr)) {
      c = bson_utf8_get_char (ptr);
      switch (c) {
      case '%':
         if (((end - ptr) < 2) || !isxdigit (ptr[1]) || !isxdigit (ptr[2]) ||
#ifdef _MSC_VER
             (1 != sscanf_s (&ptr[1], "%02x", &hex)) ||
#else
             (1 != sscanf (&ptr[1], "%02x", &hex)) ||
#endif
             !isprint (hex)) {
            bson_string_free (str, true);
            MONGOC_WARNING ("Invalid %% escape sequence");
            return NULL;
         }
         bson_string_append_c (str, hex);
         ptr += 2;
         break;
      default:
         bson_string_append_unichar (str, c);
         break;
      }
   }

   return bson_string_free (str, false);
}


const mongoc_read_prefs_t *
mongoc_uri_get_read_prefs_t (const mongoc_uri_t *uri) /* IN */
{
   BSON_ASSERT (uri);

   return uri->read_prefs;
}


void
mongoc_uri_set_read_prefs_t (mongoc_uri_t *uri,
                             const mongoc_read_prefs_t *prefs)
{
   BSON_ASSERT (uri);
   BSON_ASSERT (prefs);

   mongoc_read_prefs_destroy (uri->read_prefs);
   uri->read_prefs = mongoc_read_prefs_copy (prefs);
}


const mongoc_read_concern_t *
mongoc_uri_get_read_concern (const mongoc_uri_t *uri) /* IN */
{
   BSON_ASSERT (uri);

   return uri->read_concern;
}


void
mongoc_uri_set_read_concern (mongoc_uri_t *uri, const mongoc_read_concern_t *rc)
{
   BSON_ASSERT (uri);
   BSON_ASSERT (rc);

   mongoc_read_concern_destroy (uri->read_concern);
   uri->read_concern = mongoc_read_concern_copy (rc);
}


const mongoc_write_concern_t *
mongoc_uri_get_write_concern (const mongoc_uri_t *uri) /* IN */
{
   BSON_ASSERT (uri);

   return uri->write_concern;
}


void
mongoc_uri_set_write_concern (mongoc_uri_t *uri,
                              const mongoc_write_concern_t *wc)
{
   BSON_ASSERT (uri);
   BSON_ASSERT (wc);

   mongoc_write_concern_destroy (uri->write_concern);
   uri->write_concern = mongoc_write_concern_copy (wc);
}


bool
mongoc_uri_get_ssl (const mongoc_uri_t *uri) /* IN */
{
   bson_iter_t iter;

   BSON_ASSERT (uri);

   if (bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_SSL) &&
       BSON_ITER_HOLDS_BOOL (&iter)) {
      return bson_iter_bool (&iter);
   }
   if (bson_has_field (&uri->options, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE) ||
       bson_has_field (&uri->options, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE) ||
       bson_has_field (&uri->options, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES) ||
       bson_has_field (&uri->options, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES)) {
      return true;
   }

   return false;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_get_option_as_int32 --
 *
 *       Checks if the URI 'option' is set and of correct type (int32).
 *       The special value '0' is considered as "unset".
 *       This is so users can provide
 *       sprintf(mongodb://localhost/?option=%d, myvalue) style connection
 *strings,
 *       and still apply default values.
 *
 *       If not set, or set to invalid type, 'fallback' is returned.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       The value of 'option' if available as int32 (and not 0), or 'fallback'.
 *
 *--------------------------------------------------------------------------
 */

int32_t
mongoc_uri_get_option_as_int32 (const mongoc_uri_t *uri,
                                const char *option,
                                int32_t fallback)
{
   const bson_t *options;
   bson_iter_t iter;
   int32_t retval = fallback;

   if ((options = mongoc_uri_get_options (uri)) &&
       bson_iter_init_find_case (&iter, options, option) &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      if (!(retval = bson_iter_int32 (&iter))) {
         retval = fallback;
      }
   }

   return retval;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_set_option_as_int32 --
 *
 *       Sets a URI option 'after the fact'. Allows users to set individual
 *       URI options without passing them as a connection string.
 *
 *       Only allows a set of known options to be set.
 *       @see mongoc_uri_option_is_int32 ().
 *
 *       Does in-place-update of the option BSON if 'option' is already set.
 *       Appends the option to the end otherwise.
 *
 *       NOTE: If 'option' is already set, and is of invalid type, this
 *       function will return false.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_uri_set_option_as_int32 (mongoc_uri_t *uri,
                                const char *option,
                                int32_t value)
{
   BSON_ASSERT (option);

   if (!mongoc_uri_option_is_int32 (option)) {
      return false;
   }

   /* Server Discovery and Monitoring Spec: "the driver MUST NOT permit users to
    * configure it less than minHeartbeatFrequencyMS (500ms)." */
   if (!bson_strcasecmp (option, MONGOC_URI_HEARTBEATFREQUENCYMS) &&
       value < MONGOC_TOPOLOGY_MIN_HEARTBEAT_FREQUENCY_MS) {
      MONGOC_WARNING ("Invalid \"%s\" of %d: must be at least %d",
                      option,
                      value,
                      MONGOC_TOPOLOGY_MIN_HEARTBEAT_FREQUENCY_MS);
      return false;
   }

   /* zlib levels are from -1 (default) through 9 (best compression) */
   if (!bson_strcasecmp (option, MONGOC_URI_ZLIBCOMPRESSIONLEVEL) &&
       (value < -1 || value > 9)) {
      MONGOC_WARNING (
         "Invalid \"%s\" of %d: must be between -1 and 9", option, value);
      return false;
   }

   return _mongoc_uri_set_option_as_int32 (uri, option, value);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_uri_set_option_as_int32 --
 *
 *       Same as mongoc_uri_set_option_as_int32, except the option is not
 *       validated against valid int32 options
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_uri_set_option_as_int32 (mongoc_uri_t *uri,
                                 const char *option,
                                 int32_t value)
{
   const bson_t *options;
   bson_iter_t iter;

   if ((options = mongoc_uri_get_options (uri)) &&
       bson_iter_init_find_case (&iter, options, option)) {
      if (BSON_ITER_HOLDS_INT32 (&iter)) {
         bson_iter_overwrite_int32 (&iter, value);
         return true;
      } else {
         return false;
      }
   }

   bson_append_int32 (&uri->options, option, -1, value);
   return true;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_get_option_as_bool --
 *
 *       Checks if the URI 'option' is set and of correct type (bool).
 *
 *       If not set, or set to invalid type, 'fallback' is returned.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       The value of 'option' if available as bool, or 'fallback'.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_uri_get_option_as_bool (const mongoc_uri_t *uri,
                               const char *option,
                               bool fallback)
{
   const bson_t *options;
   bson_iter_t iter;

   if ((options = mongoc_uri_get_options (uri)) &&
       bson_iter_init_find_case (&iter, options, option) &&
       BSON_ITER_HOLDS_BOOL (&iter)) {
      return bson_iter_bool (&iter);
   }

   return fallback;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_set_option_as_bool --
 *
 *       Sets a URI option 'after the fact'. Allows users to set individual
 *       URI options without passing them as a connection string.
 *
 *       Only allows a set of known options to be set.
 *       @see mongoc_uri_option_is_bool ().
 *
 *       Does in-place-update of the option BSON if 'option' is already set.
 *       Appends the option to the end otherwise.
 *
 *       NOTE: If 'option' is already set, and is of invalid type, this
 *       function will return false.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_uri_set_option_as_bool (mongoc_uri_t *uri,
                               const char *option,
                               bool value)
{
   const bson_t *options;
   bson_iter_t iter;

   BSON_ASSERT (option);

   if (!mongoc_uri_option_is_bool (option)) {
      return false;
   }

   if ((options = mongoc_uri_get_options (uri)) &&
       bson_iter_init_find_case (&iter, options, option)) {
      if (BSON_ITER_HOLDS_BOOL (&iter)) {
         bson_iter_overwrite_bool (&iter, value);
         return true;
      } else {
         return false;
      }
   }
   bson_append_bool (&uri->options, option, -1, value);
   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_get_option_as_utf8 --
 *
 *       Checks if the URI 'option' is set and of correct type (utf8).
 *
 *       If not set, or set to invalid type, 'fallback' is returned.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       The value of 'option' if available as utf8, or 'fallback'.
 *
 *--------------------------------------------------------------------------
 */

const char *
mongoc_uri_get_option_as_utf8 (const mongoc_uri_t *uri,
                               const char *option,
                               const char *fallback)
{
   const bson_t *options;
   bson_iter_t iter;

   if ((options = mongoc_uri_get_options (uri)) &&
       bson_iter_init_find_case (&iter, options, option) &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      return bson_iter_utf8 (&iter, NULL);
   }

   return fallback;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_set_option_as_utf8 --
 *
 *       Sets a URI option 'after the fact'. Allows users to set individual
 *       URI options without passing them as a connection string.
 *
 *       Only allows a set of known options to be set.
 *       @see mongoc_uri_option_is_utf8 ().
 *
 *       If the option is not already set, this function will append it to the
 *end
 *       of the options bson.
 *       NOTE: If the option is already set the entire options bson will be
 *       overwritten, containing the new option=value (at the same position).
 *
 *       NOTE: If 'option' is already set, and is of invalid type, this
 *       function will return false.
 *
 *       NOTE: 'option' must be valid utf8.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_uri_set_option_as_utf8 (mongoc_uri_t *uri,
                               const char *option,
                               const char *value)
{
   size_t len;

   BSON_ASSERT (option);

   len = strlen (value);

   if (!bson_utf8_validate (value, len, false)) {
      return false;
   }

   if (!mongoc_uri_option_is_utf8 (option)) {
      return false;
   }
   if (!bson_strcasecmp (option, MONGOC_URI_APPNAME)) {
      return mongoc_uri_set_appname (uri, value);
   } else {
      mongoc_uri_bson_append_or_replace_key (&uri->options, option, value);
   }

   return true;
}
