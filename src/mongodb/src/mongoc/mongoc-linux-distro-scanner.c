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

#include "mongoc-handshake-os-private.h"

#ifdef MONGOC_OS_IS_LINUX

#include <stdio.h>
#include <sys/utsname.h>

#include "mongoc-error.h"
#include "mongoc-linux-distro-scanner-private.h"
#include "mongoc-log.h"
#include "mongoc-handshake-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-util-private.h"
#include "mongoc-version.h"

#define LINE_BUFFER_SIZE 1024

/*
 * fgets() wrapper which removes '\n' at the end of the string
 * Return 0 on failure or EOF.
 */
static size_t
_fgets_wrapper (char *buffer, size_t buffer_size, FILE *f)
{
   char *fgets_res;
   size_t len;

   fgets_res = fgets (buffer, buffer_size, f);

   if (!fgets_res) {
      /* Didn't read anything. Empty file or error. */
      if (ferror (f)) {
         TRACE ("fgets() failed with error %d", errno);
      }

      return 0;
   }

   /* Chop off trailing \n */
   len = strlen (buffer);

   if (len > 0 && buffer[len - 1] == '\n') {
      buffer[len - 1] = '\0';
      len--;
   } else if (len == buffer_size - 1) {
      /* We read buffer_size bytes without hitting a newline
       * therefore the line is super long, so we say this file is invalid.
       * This is important since if we are in this situation, the NEXT call to
       * fgets() will keep reading where we left off.
       *
       * This protects us from files like:
       * aaaaa...DISTRIB_ID=nasal demons
       */
      TRACE ("Found line of length %ld, bailing out", len);
      return 0;
   }

   return len;
}

static void
_process_line (const char *name_key,
               size_t name_key_len,
               char **name,
               const char *version_key,
               size_t version_key_len,
               char **version,
               const char *line,
               size_t line_len)
{
   size_t key_len;
   const char *equal_sign;
   const char *value;
   const char *needle = "=";
   size_t value_len = 0;

   ENTRY;

   /* Figure out where = is. Everything before is the key,
    * and everything after is the value */
   equal_sign = strstr (line, needle);

   if (equal_sign == NULL) {
      TRACE ("Encountered malformed line: %s", line);
      /* This line is malformed/incomplete, so skip it */
      EXIT;
   }

   /* Should never happen since we null terminated this line */
   BSON_ASSERT (equal_sign < line + line_len);

   key_len = equal_sign - line;
   value = equal_sign + strlen (needle);
   value_len = strlen (value);
   if (value_len > 2 && value[0] == '"' && value[value_len - 1] == '"') {
      value_len -= 2;
      value++;
   }

   /* If we find two copies of either key, the *name == NULL check will fail
    * so we will just keep the first value encountered. */
   if (name_key_len == key_len && strncmp (line, name_key, key_len) == 0 &&
       !(*name)) {
      *name = bson_strndup (value, value_len);
      TRACE ("Found name: %s", *name);
   } else if (version_key_len == key_len &&
              strncmp (line, version_key, key_len) == 0 && !(*version)) {
      *version = bson_strndup (value, value_len);
      TRACE ("Found version: %s", *version);
   }

   EXIT;
}


/*
 * Parse a file of the form:
 * KEY=VALUE
 * Looking for name_key and version_key, and storing
 * their values into *name and *version.
 * The values in *name and *version must be freed with bson_free.
 */
void
_mongoc_linux_distro_scanner_read_key_value_file (const char *path,
                                                  const char *name_key,
                                                  ssize_t name_key_len,
                                                  char **name,
                                                  const char *version_key,
                                                  ssize_t version_key_len,
                                                  char **version)
{
   const int max_lines = 100;
   int lines_read = 0;
   char buffer[LINE_BUFFER_SIZE];
   size_t buflen;
   FILE *f;

   ENTRY;

   *name = NULL;
   *version = NULL;

   if (name_key_len < 0) {
      name_key_len = strlen (name_key);
   }

   if (version_key_len < 0) {
      version_key_len = strlen (version_key);
   }

   if (access (path, R_OK)) {
      TRACE ("No permission to read from %s: errno: %d", path, errno);
      EXIT;
   }

   f = fopen (path, "r");

   if (!f) {
      TRACE ("fopen failed on %s: %d", path, errno);
      EXIT;
   }

   while (lines_read < max_lines) {
      buflen = _fgets_wrapper (buffer, sizeof (buffer), f);

      if (buflen == 0) {
         /* Error or eof */
         break;
      }

      _process_line (name_key,
                     name_key_len,
                     name,
                     version_key,
                     version_key_len,
                     version,
                     buffer,
                     buflen);

      if (*version && *name) {
         /* No point in reading any more */
         break;
      }

      lines_read++;
   }

   fclose (f);
   EXIT;
}

/*
 * Find the first string in a list which is a valid file. Assumes
 * passed in list is NULL terminated!
 */
const char *
_get_first_existing (const char **paths)
{
   const char **p = &paths[0];

   ENTRY;

   for (; *p != NULL; p++) {
      if (access (*p, F_OK)) {
         /* Just doesn't exist */
         continue;
      }

      if (access (*p, R_OK)) {
         TRACE ("file %s exists, but cannot be read: error %d", *p, errno);
         continue;
      }

      RETURN (*p);
   }

   RETURN (NULL);
}


/*
 * Given a line of text, split it by the word "release." For example:
 * Ubuntu release 14.04 =>
 * *name = Ubuntu
 * *version = 14.04
 * If the word "release" isn't found then we put the whole string into *name
 * (even if the string is empty).
 */
void
_mongoc_linux_distro_scanner_split_line_by_release (const char *line,
                                                    ssize_t line_len,
                                                    char **name,
                                                    char **version)
{
   const char *needle_loc;
   const char *const needle = " release ";
   const char *version_string;

   *name = NULL;
   *version = NULL;

   if (line_len < 0) {
      line_len = strlen (line);
   }

   needle_loc = strstr (line, needle);

   if (!needle_loc) {
      *name = bson_strdup (line);
      return;
   } else if (needle_loc == line) {
      /* The file starts with the word " release "
       * This file is weird enough we will just abandon it. */
      return;
   }

   *name = bson_strndup (line, needle_loc - line);

   version_string = needle_loc + strlen (needle);

   if (version_string == line + line_len) {
      /* Weird. The file just ended with "release " */
      return;
   }

   *version = bson_strdup (version_string);
}

/*
 * Search for a *-release file, and read its contents.
 */
void
_mongoc_linux_distro_scanner_read_generic_release_file (const char **paths,
                                                        char **name,
                                                        char **version)
{
   const char *path;
   size_t buflen;
   char buffer[LINE_BUFFER_SIZE];
   FILE *f;

   ENTRY;

   *name = NULL;
   *version = NULL;

   path = _get_first_existing (paths);

   if (!path) {
      EXIT;
   }

   f = fopen (path, "r");

   if (!f) {
      TRACE ("Found %s exists and readable but couldn't open: %d", path, errno);
      EXIT;
   }

   /* Read the first line of the file, look for the word "release" */
   buflen = _fgets_wrapper (buffer, sizeof (buffer), f);

   if (buflen > 0) {
      TRACE ("Trying to split buffer with contents %s", buffer);
      /* Try splitting the string. If we can't it'll store everything in
       * *name. */
      _mongoc_linux_distro_scanner_split_line_by_release (
         buffer, buflen, name, version);
   }

   fclose (f);

   EXIT;
}

static void
_get_kernel_version_from_uname (char **version)
{
   struct utsname system_info;

   if (uname (&system_info) >= 0) {
      *version = bson_strdup_printf ("kernel %s", system_info.release);
   } else {
      *version = NULL;
   }
}

/*
 * Some boilerplate logic that tries to set *name and *version to new_name
 * and new_version if it's not already set. Values of new_name and new_version
 * should not be used after this call.
 */
static bool
_set_name_and_version_if_needed (char **name,
                                 char **version,
                                 char *new_name,
                                 char *new_version)
{
   if (new_name && !(*name)) {
      *name = new_name;
   } else {
      bson_free (new_name);
   }

   if (new_version && !(*version)) {
      *version = new_version;
   } else {
      bson_free (new_version);
   }

   return (*name) && (*version);
}

bool
_mongoc_linux_distro_scanner_get_distro (char **name, char **version)
{
   /* In case we decide to try looking up name/version again */
   char *new_name;
   char *new_version;
   const char *generic_release_paths[] = {
      "/etc/redhat-release",
      "/etc/novell-release",
      "/etc/gentoo-release",
      "/etc/SuSE-release",
      "/etc/SUSE-release",
      "/etc/sles-release",
      "/etc/debian_release",
      "/etc/slackware-version",
      "/etc/centos-release",
      NULL,
   };

   ENTRY;

   *name = NULL;
   *version = NULL;

   _mongoc_linux_distro_scanner_read_key_value_file (
      "/etc/os-release", "NAME", -1, name, "VERSION_ID", -1, version);

   if (*name && *version) {
      RETURN (true);
   }

   _mongoc_linux_distro_scanner_read_key_value_file ("/etc/lsb-release",
                                                     "DISTRIB_ID",
                                                     -1,
                                                     &new_name,
                                                     "DISTRIB_RELEASE",
                                                     -1,
                                                     &new_version);

   if (_set_name_and_version_if_needed (name, version, new_name, new_version)) {
      RETURN (true);
   }

   /* Try to read from a generic release file */
   _mongoc_linux_distro_scanner_read_generic_release_file (
      generic_release_paths, &new_name, &new_version);

   if (_set_name_and_version_if_needed (name, version, new_name, new_version)) {
      RETURN (true);
   }

   if (*version == NULL) {
      _get_kernel_version_from_uname (version);
   }

   if (*name && *version) {
      RETURN (true);
   }

   bson_free (*name);
   bson_free (*version);

   *name = NULL;
   *version = NULL;

   RETURN (false);
}

#endif
