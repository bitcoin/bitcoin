/*
 * Copyright 2014 MongoDB Inc.
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

#ifndef MONGOC_B64_PRIVATE_H
#define MONGOC_B64_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-config.h"

int
mongoc_b64_ntop (uint8_t const *src,
                 size_t srclength,
                 char *target,
                 size_t targsize);

void
mongoc_b64_initialize_rmap (void);

int
mongoc_b64_pton (char const *src, uint8_t *target, size_t targsize);

#endif /* MONGOC_B64_PRIVATE_H */
