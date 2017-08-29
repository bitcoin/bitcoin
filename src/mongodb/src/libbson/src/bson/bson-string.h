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


#ifndef BSON_STRING_H
#define BSON_STRING_H


#if !defined(BSON_INSIDE) && !defined(BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#include <stdarg.h>

#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


typedef struct {
   char *str;
   uint32_t len;
   uint32_t alloc;
} bson_string_t;


BSON_EXPORT (bson_string_t *)
bson_string_new (const char *str);
BSON_EXPORT (char *)
bson_string_free (bson_string_t *string, bool free_segment);
BSON_EXPORT (void)
bson_string_append (bson_string_t *string, const char *str);
BSON_EXPORT (void)
bson_string_append_c (bson_string_t *string, char str);
BSON_EXPORT (void)
bson_string_append_unichar (bson_string_t *string, bson_unichar_t unichar);
BSON_EXPORT (void)
bson_string_append_printf (bson_string_t *string, const char *format, ...)
   BSON_GNUC_PRINTF (2, 3);
BSON_EXPORT (void)
bson_string_truncate (bson_string_t *string, uint32_t len);
BSON_EXPORT (char *)
bson_strdup (const char *str);
BSON_EXPORT (char *)
bson_strdup_printf (const char *format, ...) BSON_GNUC_PRINTF (1, 2);
BSON_EXPORT (char *)
bson_strdupv_printf (const char *format, va_list args) BSON_GNUC_PRINTF (1, 0);
BSON_EXPORT (char *)
bson_strndup (const char *str, size_t n_bytes);
BSON_EXPORT (void)
bson_strncpy (char *dst, const char *src, size_t size);
BSON_EXPORT (int)
bson_vsnprintf (char *str, size_t size, const char *format, va_list ap)
   BSON_GNUC_PRINTF (3, 0);
BSON_EXPORT (int)
bson_snprintf (char *str, size_t size, const char *format, ...)
   BSON_GNUC_PRINTF (3, 4);
BSON_EXPORT (void)
bson_strfreev (char **strv);
BSON_EXPORT (size_t)
bson_strnlen (const char *s, size_t maxlen);
BSON_EXPORT (int64_t)
bson_ascii_strtoll (const char *str, char **endptr, int base);
BSON_EXPORT (int)
bson_strcasecmp (const char *s1, const char *s2);


BSON_END_DECLS


#endif /* BSON_STRING_H */
