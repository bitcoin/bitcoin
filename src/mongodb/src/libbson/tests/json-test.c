/*
 * Copyright 2015 MongoDB, Inc.
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

#include "json-test.h"

#ifdef _MSC_VER
#include <io.h>
#else
#include <dirent.h>
#endif


/*
 *-----------------------------------------------------------------------
 *
 * assemble_path --
 *
 *       Given a parent directory and filename, compile a full path to
 *       the child file.
 *
 *       "dst" receives the joined path, delimited by "/" even on Windows.
 *
 *-----------------------------------------------------------------------
 */
void
assemble_path (const char *parent_path,
               const char *child_name,
               char *dst /* OUT */)
{
   char *p;
   int path_len = (int) strlen (parent_path);
   int name_len = (int) strlen (child_name);

   BSON_ASSERT (path_len + name_len + 1 < MAX_TEST_NAME_LENGTH);

   memset (dst, '\0', MAX_TEST_NAME_LENGTH * sizeof (char));
   strncat (dst, parent_path, path_len);
   strncat (dst, "/", 1);
   strncat (dst, child_name, name_len);

   for (p = dst; *p; ++p) {
      if (*p == '\\') {
         *p = '/';
      }
   }
}

/*
 *-----------------------------------------------------------------------
 *
 * collect_tests_from_dir --
 *
 *       Recursively search the directory at @dir_path for files with
 *       '.json' in their filenames. Append all found file paths to
 *       @paths, and return the number of files found.
 *
 *-----------------------------------------------------------------------
 */
int
collect_tests_from_dir (char (*paths)[MAX_TEST_NAME_LENGTH] /* OUT */,
                        const char *dir_path,
                        int paths_index,
                        int max_paths)
{
#ifdef _MSC_VER
   intptr_t handle;
   struct _finddata_t info;

   char child_path[MAX_TEST_NAME_LENGTH];

   handle = _findfirst (dir_path, &info);

   if (handle == -1) {
      return 0;
   }

   while (1) {
      BSON_ASSERT (paths_index < max_paths);

      if (_findnext (handle, &info) == -1) {
         break;
      }

      if (info.attrib & _A_SUBDIR) {
         /* recursively call on child directories */
         if (strcmp (info.name, "..") != 0 && strcmp (info.name, ".") != 0) {
            assemble_path (dir_path, info.name, child_path);
            paths_index = collect_tests_from_dir (
               paths, child_path, paths_index, max_paths);
         }
      } else if (strstr (info.name, ".json")) {
         /* if this is a JSON test, collect its path */
         assemble_path (dir_path, info.name, paths[paths_index++]);
      }
   }

   _findclose (handle);

   return paths_index;
#else
   struct dirent *entry;
   struct stat dir_stat;
   char child_path[MAX_TEST_NAME_LENGTH];
   DIR *dir;

   dir = opendir (dir_path);
   BSON_ASSERT (dir);
   while ((entry = readdir (dir))) {
      BSON_ASSERT (paths_index < max_paths);
      if (strcmp (entry->d_name, "..") == 0 ||
          strcmp (entry->d_name, ".") == 0) {
         continue;
      }

      assemble_path (dir_path, entry->d_name, child_path);

      if (0 == stat (entry->d_name, &dir_stat) && S_ISDIR (dir_stat.st_mode)) {
         /* recursively call on child directories */
         paths_index =
            collect_tests_from_dir (paths, child_path, paths_index, max_paths);
      } else if (strstr (entry->d_name, ".json")) {
         /* if this is a JSON test, collect its path */
         assemble_path (dir_path, entry->d_name, paths[paths_index++]);
      }
   }

   closedir (dir);

   return paths_index;
#endif
}

/*
 *-----------------------------------------------------------------------
 *
 * get_bson_from_json_file --
 *
 *        Open the file at @filename and store its contents in a
 *        bson_t. This function assumes that @filename contains a
 *        single JSON object.
 *
 *        NOTE: caller owns returned bson_t and must free it.
 *
 *-----------------------------------------------------------------------
 */
bson_t *
get_bson_from_json_file (char *filename)
{
   FILE *file;
   long length;
   bson_t *data;
   bson_error_t error;
   const char *buffer;

   file = fopen (filename, "rb");
   if (!file) {
      return NULL;
   }

   /* get file length */
   fseek (file, 0, SEEK_END);
   length = ftell (file);
   fseek (file, 0, SEEK_SET);
   if (length < 1) {
      return NULL;
   }

   /* read entire file into buffer */
   buffer = (const char *) bson_malloc0 (length);
   if (fread ((void *) buffer, 1, length, file) != length) {
      abort ();
   }

   fclose (file);
   if (!buffer) {
      return NULL;
   }

   /* convert to bson */
   data = bson_new_from_json ((const uint8_t *) buffer, length, &error);
   if (!data) {
      fprintf (stderr, "Cannot parse %s: %s\n", filename, error.message);
      abort ();
   }

   bson_free ((void *) buffer);

   return data;
}

/*
 *-----------------------------------------------------------------------
 *
 * install_json_test_suite --
 *
 *      Given a path to a directory containing JSON tests, import each
 *      test into a BSON blob and call the provided callback for
 *      evaluation.
 *
 *      It is expected that the callback will BSON_ASSERT on failure, so if
 *      callback returns quietly the test is considered to have passed.
 *
 *-----------------------------------------------------------------------
 */
void
install_json_test_suite (TestSuite *suite,
                         const char *dir_path,
                         test_hook callback)
{
   char test_paths[MAX_NUM_TESTS][MAX_TEST_NAME_LENGTH];
   int num_tests;
   int i;
   bson_t *test;
   char *skip_json;
   char *ext;

   num_tests =
      collect_tests_from_dir (&test_paths[0], dir_path, 0, MAX_NUM_TESTS);

   for (i = 0; i < num_tests; i++) {
      test = get_bson_from_json_file (test_paths[i]);
      skip_json = strstr (test_paths[i], "/json") + strlen ("/json");
      BSON_ASSERT (skip_json);
      ext = strstr (skip_json, ".json");
      BSON_ASSERT (ext);
      ext[0] = '\0';

      TestSuite_AddWC (suite,
                       skip_json,
                       (void (*) (void *)) callback,
                       (void (*) (void *)) bson_destroy,
                       test);
   }
}
