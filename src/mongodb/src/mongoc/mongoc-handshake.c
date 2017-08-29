/*
 * Copyright 2016 MongoDB, Inc.
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

#include <bson.h>

#ifdef _POSIX_VERSION
#include <sys/utsname.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include "mongoc-linux-distro-scanner-private.h"
#include "mongoc-handshake.h"
#include "mongoc-handshake-compiler-private.h"
#include "mongoc-handshake-os-private.h"
#include "mongoc-handshake-private.h"
#include "mongoc-client.h"
#include "mongoc-client-private.h"
#include "mongoc-error.h"
#include "mongoc-log.h"
#include "mongoc-version.h"
#include "mongoc-util-private.h"

/*
 * Global handshake data instance. Initialized at startup from mongoc_init ()
 *
 * Can be modified by calls to mongoc_handshake_data_append ()
 */
static mongoc_handshake_t gMongocHandshake;


static uint32_t
_get_config_bitfield (void)
{
   uint32_t bf = 0;

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
   bf |= MONGOC_MD_FLAG_ENABLE_SSL_SECURE_CHANNEL;
#endif

#ifdef MONGOC_ENABLE_CRYPTO_CNG
   bf |= MONGOC_MD_FLAG_ENABLE_CRYPTO_CNG;
#endif

#ifdef MONGOC_ENABLE_SSL_SECURE_TRANSPORT
   bf |= MONGOC_MD_FLAG_ENABLE_SSL_SECURE_TRANSPORT;
#endif

#ifdef MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO
   bf |= MONGOC_MD_FLAG_ENABLE_CRYPTO_COMMON_CRYPTO;
#endif

#ifdef MONGOC_ENABLE_SSL_OPENSSL
   bf |= MONGOC_MD_FLAG_ENABLE_SSL_OPENSSL;
#endif

#ifdef MONGOC_ENABLE_CRYPTO_LIBCRYPTO
   bf |= MONGOC_MD_FLAG_ENABLE_CRYPTO_LIBCRYPTO;
#endif

#ifdef MONGOC_ENABLE_SSL
   bf |= MONGOC_MD_FLAG_ENABLE_SSL;
#endif

#ifdef MONGOC_ENABLE_CRYPTO
   bf |= MONGOC_MD_FLAG_ENABLE_CRYPTO;
#endif

#ifdef MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE
   bf |= MONGOC_MD_FLAG_ENABLE_CRYPTO_SYSTEM_PROFILE;
#endif

#ifdef MONGOC_ENABLE_SASL
   bf |= MONGOC_MD_FLAG_ENABLE_SASL;
#endif

#ifdef MONGOC_HAVE_SASL_CLIENT_DONE
   bf |= MONGOC_MD_FLAG_HAVE_SASL_CLIENT_DONE;
#endif

#ifdef MONGOC_HAVE_WEAK_SYMBOLS
   bf |= MONGOC_MD_FLAG_HAVE_WEAK_SYMBOLS;
#endif

#ifdef MONGOC_NO_AUTOMATIC_GLOBALS
   bf |= MONGOC_MD_FLAG_NO_AUTOMATIC_GLOBALS;
#endif

#ifdef MONGOC_EXPERIMENTAL_FEATURES
   bf |= MONGOC_MD_FLAG_EXPERIMENTAL_FEATURES;
#endif

#ifdef MONGOC_ENABLE_SSL_LIBRESSL
   bf |= MONGOC_MD_FLAG_ENABLE_SSL_LIBRESSL;
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
   bf |= MONGOC_MD_FLAG_ENABLE_SASL_CYRUS;
#endif

#ifdef MONGOC_ENABLE_SASL_SSPI
   bf |= MONGOC_MD_FLAG_ENABLE_SASL_SSPI;
#endif

#ifdef MONGOC_HAVE_SOCKLEN
   bf |= MONGOC_MD_FLAG_HAVE_SOCKLEN;
#endif

#ifdef MONGOC_ENABLE_COMPRESSION
   bf |= MONGOC_MD_FLAG_ENABLE_COMPRESSION;
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   bf |= MONGOC_MD_FLAG_ENABLE_COMPRESSION_SNAPPY;
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
   bf |= MONGOC_MD_FLAG_ENABLE_COMPRESSION_ZLIB;
#endif

#ifdef MONGOC_MD_FLAG_ENABLE_SASL_GSSAPI
   bf |= MONGOC_MD_FLAG_ENABLE_SASL_GSSAPI;
#endif

   return bf;
}

static char *
_get_os_type (void)
{
#ifdef MONGOC_OS_TYPE
   return bson_strndup (MONGOC_OS_TYPE, HANDSHAKE_OS_TYPE_MAX);
#else
   return bson_strndup ("unknown", HANDSHAKE_OS_TYPE_MAX);
#endif
}

static char *
_get_os_architecture (void)
{
   const char *ret = NULL;

#ifdef _WIN32
   SYSTEM_INFO system_info;
   DWORD arch;
   GetSystemInfo (&system_info);

   arch = system_info.wProcessorArchitecture;

   switch (arch) {
   case PROCESSOR_ARCHITECTURE_AMD64:
      ret = "x86_64";
      break;
   case PROCESSOR_ARCHITECTURE_ARM:
      ret = "ARM";
      break;
   case PROCESSOR_ARCHITECTURE_IA64:
      ret = "IA64";
      break;
   case PROCESSOR_ARCHITECTURE_INTEL:
      ret = "x86";
      break;
   case PROCESSOR_ARCHITECTURE_UNKNOWN:
      ret = "Unknown";
      break;
   default:
      ret = "Other";
      break;
   }

#elif defined(_POSIX_VERSION)
   struct utsname system_info;

   if (uname (&system_info) >= 0) {
      ret = system_info.machine;
   }

#endif

   if (ret) {
      return bson_strndup (ret, HANDSHAKE_OS_ARCHITECTURE_MAX);
   }

   return NULL;
}

#ifndef MONGOC_OS_IS_LINUX
static char *
_get_os_name (void)
{
#ifdef MONGOC_OS_NAME
   return bson_strndup (MONGOC_OS_NAME, HANDSHAKE_OS_NAME_MAX);
#elif defined(_POSIX_VERSION)
   struct utsname system_info;

   if (uname (&system_info) >= 0) {
      return bson_strndup (system_info.sysname, HANDSHAKE_OS_NAME_MAX);
   }

#endif

   return NULL;
}

static char *
_get_os_version (void)
{
   char *ret = bson_malloc (HANDSHAKE_OS_VERSION_MAX);
   bool found = false;

#ifdef _WIN32
   OSVERSIONINFO osvi;
   ZeroMemory (&osvi, sizeof (OSVERSIONINFO));
   osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

   if (GetVersionEx (&osvi)) {
      bson_snprintf (ret,
                     HANDSHAKE_OS_VERSION_MAX,
                     "%lu.%lu (%lu)",
                     osvi.dwMajorVersion,
                     osvi.dwMinorVersion,
                     osvi.dwBuildNumber);
      found = true;
   } else {
      MONGOC_WARNING ("Error with GetVersionEx(): %lu", GetLastError ());
   }

#elif defined(_POSIX_VERSION)
   struct utsname system_info;

   if (uname (&system_info) >= 0) {
      bson_strncpy (ret, system_info.release, HANDSHAKE_OS_VERSION_MAX);
      found = true;
   } else {
      MONGOC_WARNING ("Error with uname(): %d", errno);
   }

#endif

   if (!found) {
      bson_free (ret);
      ret = NULL;
   }

   return ret;
}
#endif

static void
_get_system_info (mongoc_handshake_t *handshake)
{
   handshake->os_type = _get_os_type ();

#ifdef MONGOC_OS_IS_LINUX
   _mongoc_linux_distro_scanner_get_distro (&handshake->os_name,
                                            &handshake->os_version);
#else
   handshake->os_name = _get_os_name ();
   handshake->os_version = _get_os_version ();
#endif

   handshake->os_architecture = _get_os_architecture ();
}

static void
_free_system_info (mongoc_handshake_t *handshake)
{
   bson_free (handshake->os_type);
   bson_free (handshake->os_name);
   bson_free (handshake->os_version);
   bson_free (handshake->os_architecture);
}

static void
_get_driver_info (mongoc_handshake_t *handshake)
{
   handshake->driver_name = bson_strndup ("mongoc", HANDSHAKE_DRIVER_NAME_MAX);
   handshake->driver_version =
      bson_strndup (MONGOC_VERSION_S, HANDSHAKE_DRIVER_VERSION_MAX);
}

static void
_free_driver_info (mongoc_handshake_t *handshake)
{
   bson_free (handshake->driver_name);
   bson_free (handshake->driver_version);
}

static void
_set_platform_string (mongoc_handshake_t *handshake)
{
   bson_string_t *str;

   str = bson_string_new ("");

   bson_string_append_printf (str, "cfg=0x%x", _get_config_bitfield ());

#ifdef _POSIX_VERSION
   bson_string_append_printf (str, " posix=%ld", _POSIX_VERSION);
#endif

#ifdef __STDC_VERSION__
   bson_string_append_printf (str, " stdc=%ld", __STDC_VERSION__);
#endif

   bson_string_append_printf (str, " CC=%s", MONGOC_COMPILER);

#ifdef MONGOC_COMPILER_VERSION
   bson_string_append_printf (str, " %s", MONGOC_COMPILER_VERSION);
#endif

   if (strlen (MONGOC_EVALUATE_STR (MONGOC_USER_SET_CFLAGS)) > 0) {
      bson_string_append_printf (
         str, " CFLAGS=%s", MONGOC_EVALUATE_STR (MONGOC_USER_SET_CFLAGS));
   }

   if (strlen (MONGOC_EVALUATE_STR (MONGOC_USER_SET_LDFLAGS)) > 0) {
      bson_string_append_printf (
         str, " LDFLAGS=%s", MONGOC_EVALUATE_STR (MONGOC_USER_SET_LDFLAGS));
   }

   handshake->platform = bson_string_free (str, false);
}

static void
_free_platform_string (mongoc_handshake_t *handshake)
{
   bson_free (handshake->platform);
}

void
_mongoc_handshake_init (void)
{
   _get_system_info (_mongoc_handshake_get ());
   _get_driver_info (_mongoc_handshake_get ());
   _set_platform_string (_mongoc_handshake_get ());

   _mongoc_handshake_get ()->frozen = false;
}

void
_mongoc_handshake_cleanup (void)
{
   _free_system_info (_mongoc_handshake_get ());
   _free_driver_info (_mongoc_handshake_get ());
   _free_platform_string (_mongoc_handshake_get ());
}

static bool
_append_platform_field (bson_t *doc, const char *platform)
{
   int max_platform_str_size;

   /* Compute space left for platform field */
   max_platform_str_size =
      HANDSHAKE_MAX_SIZE - (doc->len +
                            /* 1 byte for utf8 tag */
                            1 +

                            /* key size */
                            strlen (HANDSHAKE_PLATFORM_FIELD) +
                            1 +

                            /* 4 bytes for length of string */
                            4);

   if (max_platform_str_size <= 0) {
      return false;
   }

   max_platform_str_size =
      BSON_MIN (max_platform_str_size, strlen (platform) + 1);
   bson_append_utf8 (
      doc, HANDSHAKE_PLATFORM_FIELD, -1, platform, max_platform_str_size - 1);

   BSON_ASSERT (doc->len <= HANDSHAKE_MAX_SIZE);
   return true;
}

/*
 * Return true if we build the document, and it's not too big
 * false if there's no way to prevent the doc from being too big. In this
 * case, the caller shouldn't include it with isMaster
 */
bool
_mongoc_handshake_build_doc_with_application (bson_t *doc, const char *appname)
{
   const mongoc_handshake_t *md = _mongoc_handshake_get ();
   bson_t child;

   if (appname) {
      BSON_APPEND_DOCUMENT_BEGIN (doc, "application", &child);
      BSON_APPEND_UTF8 (&child, "name", appname);
      bson_append_document_end (doc, &child);
   }

   BSON_APPEND_DOCUMENT_BEGIN (doc, "driver", &child);
   BSON_APPEND_UTF8 (&child, "name", md->driver_name);
   BSON_APPEND_UTF8 (&child, "version", md->driver_version);
   bson_append_document_end (doc, &child);

   BSON_APPEND_DOCUMENT_BEGIN (doc, "os", &child);

   BSON_ASSERT (md->os_type);
   BSON_APPEND_UTF8 (&child, "type", md->os_type);

   if (md->os_name) {
      BSON_APPEND_UTF8 (&child, "name", md->os_name);
   }

   if (md->os_version) {
      BSON_APPEND_UTF8 (&child, "version", md->os_version);
   }

   if (md->os_architecture) {
      BSON_APPEND_UTF8 (&child, "architecture", md->os_architecture);
   }

   bson_append_document_end (doc, &child);

   if (doc->len > HANDSHAKE_MAX_SIZE) {
      /* We've done all we can possibly do to ensure the current
       * document is below the maxsize, so if it overflows there is
       * nothing else we can do, so we fail */
      return false;
   }

   if (md->platform) {
      _append_platform_field (doc, md->platform);
   }

   return true;
}

void
_mongoc_handshake_freeze (void)
{
   _mongoc_handshake_get ()->frozen = true;
}

/*
 * free (*s) and make *s point to *s concated with suffix.
 * If *s is NULL it's treated like it's an empty string.
 * If suffix is NULL, nothing happens.
 */
static void
_append_and_truncate (char **s, const char *suffix, int max_len)
{
   char *old_str = *s;
   char *prefix;
   const int delim_len = strlen (" / ");
   int space_for_suffix;

   BSON_ASSERT (s);

   prefix = old_str ? old_str : "";

   if (!suffix) {
      return;
   }

   space_for_suffix = max_len - strlen (prefix) - delim_len;
   BSON_ASSERT (space_for_suffix >= 0);

   *s = bson_strdup_printf ("%s / %.*s", prefix, space_for_suffix, suffix);
   BSON_ASSERT (strlen (*s) <= max_len);

   bson_free (old_str);
}


/*
 * Set some values in our global handshake struct. These values will be sent
 * to the server as part of the initial connection handshake (isMaster).
 * If this function is called more than once, or after we've connected to a
 * mongod, then it will do nothing and return false. It will return true if it
 * successfully sets the values.
 *
 * All arguments are optional.
 */
bool
mongoc_handshake_data_append (const char *driver_name,
                              const char *driver_version,
                              const char *platform)
{
   int max_size = 0;

   if (_mongoc_handshake_get ()->frozen) {
      MONGOC_ERROR ("Cannot set handshake more than once");
      return false;
   }

   _append_and_truncate (&_mongoc_handshake_get ()->driver_name,
                         driver_name,
                         HANDSHAKE_DRIVER_NAME_MAX);

   _append_and_truncate (&_mongoc_handshake_get ()->driver_version,
                         driver_version,
                         HANDSHAKE_DRIVER_VERSION_MAX);

   max_size =
      HANDSHAKE_MAX_SIZE -
      -_mongoc_strlen_or_zero (_mongoc_handshake_get ()->os_type) -
      _mongoc_strlen_or_zero (_mongoc_handshake_get ()->os_name) -
      _mongoc_strlen_or_zero (_mongoc_handshake_get ()->os_version) -
      _mongoc_strlen_or_zero (_mongoc_handshake_get ()->os_architecture) -
      _mongoc_strlen_or_zero (_mongoc_handshake_get ()->driver_name) -
      _mongoc_strlen_or_zero (_mongoc_handshake_get ()->driver_version);
   _append_and_truncate (
      &_mongoc_handshake_get ()->platform, platform, max_size);

   _mongoc_handshake_freeze ();
   return true;
}

mongoc_handshake_t *
_mongoc_handshake_get (void)
{
   return &gMongocHandshake;
}

bool
_mongoc_handshake_appname_is_valid (const char *appname)
{
   return strlen (appname) <= MONGOC_HANDSHAKE_APPNAME_MAX;
}
