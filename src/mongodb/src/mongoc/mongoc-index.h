/*
 * Copyright 2013 MongoDB Inc.
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

#ifndef MONGOC_INDEX_H
#define MONGOC_INDEX_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"


BSON_BEGIN_DECLS

typedef struct {
   uint8_t twod_sphere_version;
   uint8_t twod_bits_precision;
   double twod_location_min;
   double twod_location_max;
   double haystack_bucket_size;
   uint8_t *padding[32];
} mongoc_index_opt_geo_t;

typedef struct {
   int type;
} mongoc_index_opt_storage_t;

typedef enum {
   MONGOC_INDEX_STORAGE_OPT_MMAPV1,
   MONGOC_INDEX_STORAGE_OPT_WIREDTIGER,
} mongoc_index_storage_opt_type_t;

typedef struct {
   mongoc_index_opt_storage_t base;
   const char *config_str;
   void *padding[8];
} mongoc_index_opt_wt_t;

typedef struct {
   bool is_initialized;
   bool background;
   bool unique;
   const char *name;
   bool drop_dups;
   bool sparse;
   int32_t expire_after_seconds;
   int32_t v;
   const bson_t *weights;
   const char *default_language;
   const char *language_override;
   mongoc_index_opt_geo_t *geo_options;
   mongoc_index_opt_storage_t *storage_options;
   const bson_t *partial_filter_expression;
   const bson_t *collation;
   void *padding[4];
} mongoc_index_opt_t;


MONGOC_EXPORT (const mongoc_index_opt_t *)
mongoc_index_opt_get_default (void) BSON_GNUC_CONST;
MONGOC_EXPORT (const mongoc_index_opt_geo_t *)
mongoc_index_opt_geo_get_default (void) BSON_GNUC_CONST;
MONGOC_EXPORT (const mongoc_index_opt_wt_t *)
mongoc_index_opt_wt_get_default (void) BSON_GNUC_CONST;
MONGOC_EXPORT (void)
mongoc_index_opt_init (mongoc_index_opt_t *opt);
MONGOC_EXPORT (void)
mongoc_index_opt_geo_init (mongoc_index_opt_geo_t *opt);
MONGOC_EXPORT (void)
mongoc_index_opt_wt_init (mongoc_index_opt_wt_t *opt);

BSON_END_DECLS


#endif /* MONGOC_INDEX_H */
