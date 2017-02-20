/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_HMAC_H_
#define _DB_HMAC_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Algorithm specific information.
 */
/*
 * SHA1 checksumming
 */
typedef struct {
    u_int32_t   state[5];
    u_int32_t   count[2];
    unsigned char   buffer[64];
} SHA1_CTX;

/*
 * AES assumes the SHA1 checksumming (also called MAC)
 */
#define DB_MAC_MAGIC    "mac derivation key magic value"
#define DB_ENC_MAGIC    "encryption and decryption key value magic"

#if defined(__cplusplus)
}
#endif

#include "dbinc_auto/hmac_ext.h"
#endif /* !_DB_HMAC_H_ */
