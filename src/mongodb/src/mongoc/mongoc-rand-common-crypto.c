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

#ifdef MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO

#include "mongoc-rand.h"
#include "mongoc-rand-private.h"

#include "mongoc.h"
#include <Security/Security.h>
/* rumour has it this wasn't in standard Security.h in ~10.8 */
#include <Security/SecRandom.h>

int
_mongoc_rand_bytes (uint8_t *buf, int num)
{
   return !SecRandomCopyBytes (kSecRandomDefault, num, buf);
}

void
mongoc_rand_seed (const void *buf, int num)
{
   /* No such thing in Common Crypto */
}

void
mongoc_rand_add (const void *buf, int num, double entropy)
{
   /* No such thing in Common Crypto */
}

int
mongoc_rand_status (void)
{
   return 1;
}

#endif
