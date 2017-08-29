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


#include "bson-tests.h"
#include "TestSuite.h"


static void
test_bson_error_basic (void)
{
   bson_error_t error;

   bson_set_error (&error, 123, 456, "%s %u", "localhost", 27017);
   BSON_ASSERT (!strcmp (error.message, "localhost 27017"));
   BSON_ASSERT_CMPINT (error.domain, ==, 123);
   BSON_ASSERT_CMPINT (error.code, ==, 456);
}


void
test_error_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/error/basic", test_bson_error_basic);
}
