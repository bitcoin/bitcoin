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


#if !defined(BSON_INSIDE) && !defined(BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#ifndef BSON_VERSION_FUNCTIONS_H
#define BSON_VERSION_FUNCTIONS_H

#include "bson-types.h"

BSON_EXPORT (int)
bson_get_major_version (void);
BSON_EXPORT (int)
bson_get_minor_version (void);
BSON_EXPORT (int)
bson_get_micro_version (void);
BSON_EXPORT (const char *)
bson_get_version (void);
BSON_EXPORT (bool)
bson_check_version (int required_major, int required_minor, int required_micro);


#endif /* BSON_VERSION_FUNCTIONS_H */
