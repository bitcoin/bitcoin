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


#include "bson-version.h"
#include "bson-version-functions.h"


/**
 * bson_get_major_version:
 *
 * Helper function to return the runtime major version of the library.
 */
int
bson_get_major_version (void)
{
   return BSON_MAJOR_VERSION;
}


/**
 * bson_get_minor_version:
 *
 * Helper function to return the runtime minor version of the library.
 */
int
bson_get_minor_version (void)
{
   return BSON_MINOR_VERSION;
}

/**
 * bson_get_micro_version:
 *
 * Helper function to return the runtime micro version of the library.
 */
int
bson_get_micro_version (void)
{
   return BSON_MICRO_VERSION;
}

/**
 * bson_get_version:
 *
 * Helper function to return the runtime string version of the library.
 */
const char *
bson_get_version (void)
{
   return BSON_VERSION_S;
}

/**
 * bson_check_version:
 *
 * True if libmongoc's version is greater than or equal to the required
 * version.
 */
bool
bson_check_version (int required_major, int required_minor, int required_micro)
{
   return BSON_CHECK_VERSION (required_major, required_minor, required_micro);
}
