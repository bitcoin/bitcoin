/*
 * Copyright 2014 MongoDB, Inc.
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


#ifndef BSON_ISO8601_PRIVATE_H
#define BSON_ISO8601_PRIVATE_H


#include "bson-compat.h"
#include "bson-macros.h"
#include "bson-string.h"


BSON_BEGIN_DECLS

bool
_bson_iso8601_date_parse (const char *str,
                          int32_t len,
                          int64_t *out,
                          bson_error_t *error);

/**
 * _bson_iso8601_date_format:
 * @msecs_since_epoch: A positive number of milliseconds since Jan 1, 1970.
 * @str: The string to append the ISO8601-formatted to.
 *
 * Appends a date formatted like "2012-12-24T12:15:30.500Z" to @str.
 */
void
_bson_iso8601_date_format (int64_t msecs_since_epoch, bson_string_t *str);

BSON_END_DECLS


#endif /* BSON_ISO8601_PRIVATE_H */
