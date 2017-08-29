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

#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL

#include "mongoc-rand.h"
#include "mongoc-rand-private.h"

#include "mongoc.h"

#include <windows.h>
#include <stdio.h>
#include <bcrypt.h>

#define NT_SUCCESS(Status) (((NTSTATUS) (Status)) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS) 0xC0000001L)

int
_mongoc_rand_bytes (uint8_t *buf, int num)
{
   static BCRYPT_ALG_HANDLE algorithm = 0;
   NTSTATUS status = 0;

   if (!algorithm) {
      status = BCryptOpenAlgorithmProvider (
         &algorithm, BCRYPT_RNG_ALGORITHM, NULL, 0);
      if (!NT_SUCCESS (status)) {
         MONGOC_ERROR ("BCryptOpenAlgorithmProvider(): %d", status);
         return 0;
      }
   }

   status = BCryptGenRandom (algorithm, buf, num, 0);
   if (NT_SUCCESS (status)) {
      return 1;
   }

   MONGOC_ERROR ("BCryptGenRandom(): %d", status);
   return 0;
}

void
mongoc_rand_seed (const void *buf, int num)
{
   /* N/A - OS Does not need entropy seed */
}

void
mongoc_rand_add (const void *buf, int num, double entropy)
{
   /* N/A - OS Does not need entropy seed */
}

int
mongoc_rand_status (void)
{
   return 1;
}

#endif
