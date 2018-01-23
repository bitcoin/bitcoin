#ifndef _SKEIN_H_
#define _SKEIN_H_     1
/**************************************************************************
**
** Interface declarations and internal definitions for Skein hashing.
**
** Source code author: Doug Whiting, 2008.
**
** This algorithm and source code is released to the public domain.
**
***************************************************************************
** 
** The following compile-time switches may be defined to control some
** tradeoffs between speed, code size, error checking, and security.
**
** The "default" note explains what happens when the switch is not defined.
**
**  SKEIN_DEBUG            -- make callouts from inside Skein code
**                            to examine/display intermediate values.
**                            [default: no callouts (no overhead)]
**
**  SKEIN_ERR_CHECK        -- how error checking is handled inside Skein
**                            code. If not defined, most error checking 
**                            is disabled (for performance). Otherwise, 
**                            the switch value is interpreted as:
**                                0: use assert()      to flag errors
**                                1: return SKEIN_FAIL to flag errors
**
***************************************************************************/
#include "skein_port.h"                      /* get platform-specific definitions */

typedef enum
{
  SKEIN_SUCCESS         =      0,          /* return codes from Skein calls */
  SKEIN_FAIL            =      1,
  SKEIN_BAD_HASHLEN     =      2
}
SkeinHashReturn;

typedef size_t   SkeinDataLength;                /* bit count  type */
typedef u08b_t   SkeinBitSequence;               /* bit stream type */

/* "all-in-one" call */
SkeinHashReturn skein_hash(int hashbitlen,   const SkeinBitSequence *data,
        SkeinDataLength databitlen, SkeinBitSequence *hashval);

#endif  /* ifndef _SKEIN_H_ */
