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


#include "bson.h"

#include "bson-tests.h"
#include "TestSuite.h"


static void
test_swap16 (void)
{
   uint16_t v = 0xFCBA;

   BSON_ASSERT (BSON_UINT16_SWAP_LE_BE (v) == 0xBAFC);
   BSON_ASSERT (__bson_uint16_swap_slow (v) == 0xBAFC);
}


static void
test_swap32 (void)
{
   uint32_t v = 0x00112233;

   BSON_ASSERT (BSON_UINT32_SWAP_LE_BE (v) == 0x33221100);
   BSON_ASSERT (__bson_uint32_swap_slow (v) == 0x33221100);
}


static void
test_swap64 (void)
{
   uint64_t v = 0x0011223344556677ULL;

   BSON_ASSERT (BSON_UINT64_SWAP_LE_BE (v) == 0x7766554433221100ULL);
   BSON_ASSERT (__bson_uint64_swap_slow (v) == 0x7766554433221100ULL);
}


void
test_endian_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/endian/swap16", test_swap16);
   TestSuite_Add (suite, "/endian/swap32", test_swap32);
   TestSuite_Add (suite, "/endian/swap64", test_swap64);
}
