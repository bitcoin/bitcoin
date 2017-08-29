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


#ifndef BSON_ERROR_H
#define BSON_ERROR_H


#include "bson-compat.h"
#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


#define BSON_ERROR_JSON 1
#define BSON_ERROR_READER 2
#define BSON_ERROR_INVALID 3


BSON_EXPORT (void)
bson_set_error (bson_error_t *error,
                uint32_t domain,
                uint32_t code,
                const char *format,
                ...) BSON_GNUC_PRINTF (4, 5);
BSON_EXPORT (char *)
bson_strerror_r (int err_code, char *buf, size_t buflen);


BSON_END_DECLS


#endif /* BSON_ERROR_H */
