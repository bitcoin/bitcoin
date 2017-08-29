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

#include <mongoc.h>
#include "mongoc-client-private.h"
#include "mongoc-linux-distro-scanner-private.h"
#include "mongoc-handshake-os-private.h"

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "test-conveniences.h"

#ifdef MONGOC_OS_IS_LINUX
static void
test_read_generic_release_file (void)
{
   char *name;
   char *version;
   const char *paths[] = {
      OS_RELEASE_FILE_DIR "/lol-im-not-here.txt",
      OS_RELEASE_FILE_DIR "/also-not-here.txt",
      OS_RELEASE_FILE_DIR "/example-etc-fedora-release.txt",
      NULL,
   };

   const char *paths2[] = {
      OS_RELEASE_FILE_DIR "/example-etc-xyz-release-no-delimiter.txt", NULL,
   };

   const char *paths3[] = {
      OS_RELEASE_FILE_DIR "/empty-file.txt", NULL,
   };

   _mongoc_linux_distro_scanner_read_generic_release_file (
      paths, &name, &version);
   ASSERT (name);
   ASSERT (version);
   ASSERT_CMPSTR ("Fedora", name);
   ASSERT_CMPSTR ("8 (Werewolf)", version);
   bson_free (name);
   bson_free (version);

   _mongoc_linux_distro_scanner_read_generic_release_file (
      paths2, &name, &version);
   ASSERT (name);
   ASSERT_CMPSTR ("This one just has name, not that R word", name);
   ASSERT (version == NULL);
   bson_free (name);

   _mongoc_linux_distro_scanner_read_generic_release_file (
      paths3, &name, &version);
   ASSERT (name == NULL);
   ASSERT (version == NULL);

   _mongoc_linux_distro_scanner_split_line_by_release (
      " release ", -1, &name, &version);
   ASSERT (name == NULL);
   ASSERT (version == NULL);

   _mongoc_linux_distro_scanner_split_line_by_release (
      "ends with release ", -1, &name, &version);
   ASSERT_CMPSTR ("ends with", name);
   ASSERT (version == NULL);
   bson_free (name);

   _mongoc_linux_distro_scanner_split_line_by_release ("", -1, &name, &version);
   ASSERT_CMPSTR (name, "");
   ASSERT (version == NULL);
   bson_free (name);
}


static void
test_read_key_value_file (void)
{
   char *name = NULL;
   char *version = NULL;

   _mongoc_linux_distro_scanner_read_key_value_file (OS_RELEASE_FILE_DIR
                                                     "/example-lsb-file.txt",
                                                     "DISTRIB_ID",
                                                     -1,
                                                     &name,
                                                     "DISTRIB_RELEASE",
                                                     -1,
                                                     &version);

   ASSERT (name);
   ASSERT_CMPSTR (name, "Ubuntu");
   ASSERT (version);
   ASSERT_CMPSTR (version, "12.04");

   bson_free (name);
   bson_free (version);

   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-lsb-file-with-super-long-line.txt",
      "DISTRIB_ID",
      -1,
      &name,
      "DISTRIB_RELEASE",
      -1,
      &version);
   ASSERT (!name);
   ASSERT (version);
   ASSERT_CMPSTR (version, "12.04");
   bson_free (version);


   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-etc-os-release.txt",
      "NAME",
      -1,
      &name,
      "VERSION_ID",
      -1,
      &version);

   ASSERT_CMPSTR (name, "Fedora");
   ASSERT_CMPSTR (version, "17");

   bson_free (name);
   bson_free (version);

   /* Now try some weird inputs */
   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-etc-os-release.txt",
      "ID=",
      -1,
      &name,
      "VERSION_ID=",
      -1,
      &version);

   ASSERT (name == NULL);
   ASSERT (version == NULL);

   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-etc-os-release.txt",
      "",
      -1,
      &name,
      "",
      -1,
      &version);

   ASSERT (name == NULL);
   ASSERT (version == NULL);

   /* Test case where we get one but not the other */
   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-etc-os-release.txt",
      "NAME",
      -1,
      &name,
      "VERSION_",
      -1,
      &version);

   ASSERT_CMPSTR (name, "Fedora");
   ASSERT (version == NULL);
   bson_free (name);

   /* Case where we say the key is the whole line */
   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-etc-os-release.txt",
      "NAME",
      -1,
      &name,
      "VERSION_ID=17",
      -1,
      &version);
   ASSERT_CMPSTR (name, "Fedora");
   ASSERT (version == NULL);
   bson_free (name);

   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-etc-os-release-ubuntu1604.txt",
      "NAME",
      -1,
      &name,
      "VERSION_ID",
      -1,
      &version);
   ASSERT_CMPSTR ("Ubuntu", name);
   ASSERT_CMPSTR ("16.04", version);
   bson_free (name);
   bson_free (version);


   /* Case where the key is duplicated, make sure we keep first version */
   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-key-value-file.txt",
      "key",
      -1,
      &name,
      "normalkey",
      -1,
      &version);
   ASSERT_CMPSTR (name, "first value");
   ASSERT_CMPSTR (version, "normalval");
   bson_free (name);
   bson_free (version);

   /* Case where the key is duplicated, make sure we keep first version */
   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-key-value-file.txt",
      "a-key-without-a-value",
      -1,
      &name,
      "normalkey",
      -1,
      &version);
   ASSERT_CMPSTR (name, "");
   ASSERT_CMPSTR (version, "normalval");
   bson_free (name);
   bson_free (version);

   /* Try to get value from a line like:
    * just-a-key
    * (No equals, no value) */
   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-key-value-file.txt",
      "just-a-key",
      -1,
      &name,
      "normalkey",
      -1,
      &version);
   ASSERT (name == NULL);
   ASSERT_CMPSTR (version, "normalval");
   bson_free (name);
   bson_free (version);

   /* Try to get a key which is on line 101 of the file
    * (we stop reading at line 100) */
   _mongoc_linux_distro_scanner_read_key_value_file (
      OS_RELEASE_FILE_DIR "/example-key-value-file.txt",
      "lastkey",
      -1,
      &name,
      "normalkey",
      -1,
      &version);
   ASSERT (name == NULL);
   ASSERT_CMPSTR (version, "normalval");
   bson_free (version);
}

/* We only expect this function to actually read anything on linux platforms */
static void
test_distro_scanner_reads (void)
{
   char *name;
   char *version;

   _mongoc_linux_distro_scanner_get_distro (&name, &version);

#ifdef __linux__
   ASSERT (name);
   ASSERT (strlen (name) > 0);

   /* Some linux distros don't have a version (like arch) but we should always
    * return a version (at the very least, we'll return the kernel version) */
   ASSERT (version);
   ASSERT (strlen (version));
#endif
   bson_free (name);
   bson_free (version);
}
#endif

void
test_linux_distro_scanner_install (TestSuite *suite)
{
#ifdef MONGOC_OS_IS_LINUX
   TestSuite_Add (suite,
                  "/LinuxDistroScanner/test_read_generic_release_file",
                  test_read_generic_release_file);
   TestSuite_Add (suite,
                  "/LinuxDistroScanner/test_read_key_value_file",
                  test_read_key_value_file);
   TestSuite_Add (suite,
                  "/LinuxDistroScanner/test_distro_scanner_reads",
                  test_distro_scanner_reads);
#endif
}
