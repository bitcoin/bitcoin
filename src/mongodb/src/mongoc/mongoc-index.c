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


#include "mongoc-index.h"
#include "mongoc-trace-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "gridfs_index"


static mongoc_index_opt_t gMongocIndexOptDefault = {
   1,     /* is_initialized */
   0,     /* background */
   0,     /* unique */
   NULL,  /* name */
   0,     /* drop_dups */
   0,     /* sparse  */
   -1,    /* expire_after_seconds */
   -1,    /* v */
   NULL,  /* weights */
   NULL,  /* default_language */
   NULL,  /* language_override */
   NULL,  /* mongoc_index_opt_geo_t geo_options */
   NULL,  /* mongoc_index_opt_storage_t storage_options */
   NULL,  /* partial_filter_expression */
   NULL,  /* collation */
   {NULL} /* struct padding */
};

static mongoc_index_opt_geo_t gMongocIndexOptGeoDefault = {
   26,    /* twod_sphere_version */
   -90,   /* twod_bits_precision */
   90,    /* twod_location_min */
   -1,    /* twod_location_max */
   2,     /* haystack_bucket_size */
   {NULL} /* struct padding */
};

static mongoc_index_opt_wt_t gMongocIndexOptWTDefault = {
   {MONGOC_INDEX_STORAGE_OPT_WIREDTIGER}, /* mongoc_index_opt_storage_t */
   "",                                    /* config_str */
   {NULL}                                 /* struct padding */
};

const mongoc_index_opt_t *
mongoc_index_opt_get_default (void)
{
   return &gMongocIndexOptDefault;
}

const mongoc_index_opt_geo_t *
mongoc_index_opt_geo_get_default (void)
{
   return &gMongocIndexOptGeoDefault;
}

const mongoc_index_opt_wt_t *
mongoc_index_opt_wt_get_default (void)
{
   return &gMongocIndexOptWTDefault;
}

void
mongoc_index_opt_init (mongoc_index_opt_t *opt)
{
   BSON_ASSERT (opt);

   memcpy (opt, &gMongocIndexOptDefault, sizeof *opt);
}

void
mongoc_index_opt_geo_init (mongoc_index_opt_geo_t *opt)
{
   BSON_ASSERT (opt);

   memcpy (opt, &gMongocIndexOptGeoDefault, sizeof *opt);
}

void
mongoc_index_opt_wt_init (mongoc_index_opt_wt_t *opt)
{
   BSON_ASSERT (opt);

   memcpy (opt, &gMongocIndexOptWTDefault, sizeof *opt);
}
