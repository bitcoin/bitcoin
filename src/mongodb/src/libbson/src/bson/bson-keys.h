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


#ifndef BSON_KEYS_H
#define BSON_KEYS_H


#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


BSON_EXPORT (size_t)
bson_uint32_to_string (uint32_t value,
                       const char **strptr,
                       char *str,
                       size_t size);


BSON_END_DECLS


#endif /* BSON_KEYS_H */
