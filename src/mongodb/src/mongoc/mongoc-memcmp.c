/*
 * Copyright (c) 2013-2015
 * Frank Denis <j at pureftpd dot org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "mongoc-memcmp-private.h"

#ifdef MONGOC_HAVE_WEAK_SYMBOLS
__attribute__ ((weak)) void
_mongoc_dummy_symbol_to_prevent_memcmp_lto (const unsigned char *b1,
                                            const unsigned char *b2,
                                            const size_t len)
{
   (void) b1;
   (void) b2;
   (void) len;
}
#endif

/* See: http://doc.libsodium.org/helpers/index.html#constant-time-comparison */
int
mongoc_memcmp (const void *const b1_, const void *const b2_, size_t len)
{
#ifdef MONGOC_HAVE_WEAK_SYMBOLS
   const unsigned char *b1 = (const unsigned char *) b1_;
   const unsigned char *b2 = (const unsigned char *) b2_;
#else
   const volatile unsigned char *b1 = (const volatile unsigned char *) b1_;
   const volatile unsigned char *b2 = (const volatile unsigned char *) b2_;
#endif
   size_t i;
   unsigned char d = (unsigned char) 0U;

#if MONGOC_HAVE_WEAK_SYMBOLS
   _mongoc_dummy_symbol_to_prevent_memcmp_lto (b1, b2, len);
#endif
   for (i = 0U; i < len; i++) {
      d |= b1[i] ^ b2[i];
   }
   return (int) ((1 & ((d - 1) >> 8)) - 1);
}
