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


#include <bson.h>


#ifdef BSON_OS_UNIX


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


#pragma pack(1)
typedef struct {
   uint32_t offset;
   uint32_t slot;
   char category[24];
   char name[32];
   char description[64];
} mongoc_counter_info_t;
#pragma pack()


BSON_STATIC_ASSERT (sizeof (mongoc_counter_info_t) == 128);


#pragma pack(1)
typedef struct {
   uint32_t size;
   uint32_t n_cpu;
   uint32_t n_counters;
   uint32_t infos_offset;
   uint32_t values_offset;
   uint8_t padding[44];
} mongoc_counters_t;
#pragma pack()


BSON_STATIC_ASSERT (sizeof (mongoc_counters_t) == 64);


typedef struct {
   int64_t slots[8];
} mongoc_counter_slots_t;


BSON_STATIC_ASSERT (sizeof (mongoc_counter_slots_t) == 64);


typedef struct {
   mongoc_counter_slots_t *cpus;
} mongoc_counter_t;


static void
mongoc_counters_destroy (mongoc_counters_t *counters)
{
   BSON_ASSERT (counters);
   munmap ((void *) counters, counters->size);
}


static mongoc_counters_t *
mongoc_counters_new_from_pid (unsigned pid)
{
   mongoc_counters_t *counters;
   int32_t len;
   size_t size;
   void *mem;
   char name[32];
   int fd;

   snprintf (name, sizeof name, "/mongoc-%u", pid);
   name[sizeof name - 1] = '\0';

   if (-1 == (fd = shm_open (name, O_RDONLY, 0))) {
      perror ("Failed to load shared memory segment");
      return NULL;
   }

   if (4 != pread (fd, &len, 4, 0)) {
      perror ("Failed to load shared memory segment");
      return NULL;
   }

   if (!len) {
      perror ("Shared memory area is not yet initialized by owning process.");
      return NULL;
   }

   size = len;

   if (MAP_FAILED == (mem = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0))) {
      fprintf (stderr,
               "Failed to mmap shared memory segment of size: %u",
               (unsigned) size);
      close (fd);
      return NULL;
   }

   close (fd);

   counters = (mongoc_counters_t *) mem;
   if (counters->size != len) {
      perror ("Corrupted shared memory segment.");
      mongoc_counters_destroy (counters);
      return NULL;
   }

   return counters;
}


static mongoc_counter_info_t *
mongoc_counters_get_infos (mongoc_counters_t *counters, uint32_t *n_infos)
{
   mongoc_counter_info_t *info;
   char *base = (char *) counters;

   BSON_ASSERT (counters);
   BSON_ASSERT (n_infos);

   info = (mongoc_counter_info_t *) (base + counters->infos_offset);
   *n_infos = counters->n_counters;

   return info;
}


static int64_t
mongoc_counters_get_value (mongoc_counters_t *counters,
                           mongoc_counter_info_t *info,
                           mongoc_counter_t *counter)
{
   int64_t value = 0;
   unsigned i;

   for (i = 0; i < counters->n_cpu; i++) {
      value += counter->cpus[i].slots[info->slot];
   }

   return value;
}


static void
mongoc_counters_print_info (mongoc_counters_t *counters,
                            mongoc_counter_info_t *info,
                            FILE *file)
{
   mongoc_counter_t ctr;
   int64_t value;

   BSON_ASSERT (info);
   BSON_ASSERT (file);
   BSON_ASSERT ((info->offset & 0x7) == 0);

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#endif
   ctr.cpus = (mongoc_counter_slots_t *) (((char *) counters) + info->offset);
#ifdef __clang__
#pragma clang diagnostic pop
#endif

   value = mongoc_counters_get_value (counters, info, &ctr);

   fprintf (file,
            "%24s : %-24s : %-50s : %lld\n",
            info->category,
            info->name,
            info->description,
            (long long) value);
}


int
main (int argc, char *argv[])
{
   mongoc_counter_info_t *infos;
   mongoc_counters_t *counters;
   uint32_t n_counters = 0;
   unsigned i;
   int pid;

   if (argc != 2) {
      fprintf (stderr, "usage: %s PID\n", argv[0]);
      return 1;
   }

   pid = strtol (argv[1], NULL, 10);
   if (!(counters = mongoc_counters_new_from_pid (pid))) {
      fprintf (stderr, "Failed to load shared memory for pid %u.\n", pid);
      return EXIT_FAILURE;
   }

   infos = mongoc_counters_get_infos (counters, &n_counters);
   for (i = 0; i < n_counters; i++) {
      mongoc_counters_print_info (counters, &infos[i], stdout);
   }

   mongoc_counters_destroy (counters);

   return EXIT_SUCCESS;
}

#else

#include <stdio.h>

int
main (int argc, char *argv[])
{
   fprintf (stderr, "mongoc-stat is not supported on your platform.\n");
   return EXIT_FAILURE;
}

#endif
