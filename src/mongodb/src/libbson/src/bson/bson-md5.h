/*
  Copyright (C) 1999, 2002 Aladdin Enterprises.  All rights reserved.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  L. Peter Deutsch
  ghost@aladdin.com

 */
/* $Id: md5.h,v 1.4 2002/04/13 19:20:28 lpd Exp $ */
/*
  Independent implementation of MD5 (RFC 1321).

  This code implements the MD5 Algorithm defined in RFC 1321, whose
  text is available at
    http://www.ietf.org/rfc/rfc1321.txt
  The code is derived from the text of the RFC, including the test suite
  (section A.5) but excluding the rest of Appendix A.  It does not include
  any code or documentation that is identified in the RFC as being
  copyrighted.

  The original and principal author of md5.h is L. Peter Deutsch
  <ghost@aladdin.com>.  Other authors are noted in the change history
  that follows (in reverse chronological order):

  2002-04-13 lpd Removed support for non-ANSI compilers; removed
    references to Ghostscript; clarified derivation from RFC 1321;
    now handles byte order either statically or dynamically.
  1999-11-04 lpd Edited comments slightly for automatic TOC extraction.
  1999-10-18 lpd Fixed typo in header comment (ansi2knr rather than md5);
    added conditionalization for C++ compilation from Martin
    Purschke <purschke@bnl.gov>.
  1999-05-03 lpd Original version.
 */


/*
 * The following MD5 implementation has been modified to use types as
 * specified in libbson.
 */


#ifndef BSON_MD5_H
#define BSON_MD5_H


#if !defined(BSON_INSIDE) && !defined(BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#include "bson-endian.h"


BSON_BEGIN_DECLS


typedef struct {
   uint32_t count[2]; /* message length in bits, lsw first */
   uint32_t abcd[4];  /* digest buffer */
   uint8_t buf[64];   /* accumulate block */
} bson_md5_t;


BSON_EXPORT (void)
bson_md5_init (bson_md5_t *pms);
BSON_EXPORT (void)
bson_md5_append (bson_md5_t *pms, const uint8_t *data, uint32_t nbytes);
BSON_EXPORT (void)
bson_md5_finish (bson_md5_t *pms, uint8_t digest[16]);


BSON_END_DECLS


#endif /* BSON_MD5_H */
