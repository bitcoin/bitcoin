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


#ifndef BSON_TIMEGM_PRIVATE_H
#define BSON_TIMEGM_PRIVATE_H


#include "bson-compat.h"
#include "bson-macros.h"


BSON_BEGIN_DECLS

/* avoid system-dependent struct tm definitions */
struct bson_tm {
   int64_t tm_sec;    /* seconds after the minute [0-60] */
   int64_t tm_min;    /* minutes after the hour [0-59] */
   int64_t tm_hour;   /* hours since midnight [0-23] */
   int64_t tm_mday;   /* day of the month [1-31] */
   int64_t tm_mon;    /* months since January [0-11] */
   int64_t tm_year;   /* years since 1900 */
   int64_t tm_wday;   /* days since Sunday [0-6] */
   int64_t tm_yday;   /* days since January 1 [0-365] */
   int64_t tm_isdst;  /* Daylight Savings Time flag */
   int64_t tm_gmtoff; /* offset from CUT in seconds */
   char *tm_zone;     /* timezone abbreviation */
};

int64_t
_bson_timegm (struct bson_tm *const tmp);

BSON_END_DECLS


#endif /* BSON_TIMEGM_PRIVATE_H */
