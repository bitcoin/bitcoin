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


#include <stdio.h>
#include <stdarg.h>

#include "bson-compat.h"
#include "bson-config.h"
#include "bson-error.h"
#include "bson-memory.h"
#include "bson-string.h"
#include "bson-types.h"


/*
 *--------------------------------------------------------------------------
 *
 * bson_set_error --
 *
 *       Initializes @error using the parameters specified.
 *
 *       @domain is an application specific error domain which should
 *       describe which module initiated the error. Think of this as the
 *       exception type.
 *
 *       @code is the @domain specific error code.
 *
 *       @format is used to generate the format string. It uses vsnprintf()
 *       internally so the format should match what you would use there.
 *
 * Parameters:
 *       @error: A #bson_error_t.
 *       @domain: The error domain.
 *       @code: The error code.
 *       @format: A printf style format string.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @error is initialized.
 *
 *--------------------------------------------------------------------------
 */

void
bson_set_error (bson_error_t *error, /* OUT */
                uint32_t domain,     /* IN */
                uint32_t code,       /* IN */
                const char *format,  /* IN */
                ...)                 /* IN */
{
   va_list args;

   if (error) {
      error->domain = domain;
      error->code = code;

      va_start (args, format);
      bson_vsnprintf (error->message, sizeof error->message, format, args);
      va_end (args);

      error->message[sizeof error->message - 1] = '\0';
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_strerror_r --
 *
 *       This is a reentrant safe macro for strerror.
 *
 *       The resulting string may be stored in @buf.
 *
 * Returns:
 *       A pointer to a static string or @buf.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

char *
bson_strerror_r (int err_code,  /* IN */
                 char *buf,     /* IN */
                 size_t buflen) /* IN */
{
   static const char *unknown_msg = "Unknown error";
   char *ret = NULL;

#if defined(_WIN32)
   if (strerror_s (buf, buflen, err_code) != 0) {
      ret = buf;
   }
#elif defined(__GNUC__) && defined(_GNU_SOURCE)
   ret = strerror_r (err_code, buf, buflen);
#else /* XSI strerror_r */
   if (strerror_r (err_code, buf, buflen) == 0) {
      ret = buf;
   }
#endif

   if (!ret) {
      bson_strncpy (buf, unknown_msg, buflen);
      ret = buf;
   }

   return ret;
}
