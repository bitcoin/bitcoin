/*-
 * Copyright 2013,2014 Alexander Peslyak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "yescrypt.h"

#define BYTES2CHARS(bytes) \
	((((bytes) * 8) + 5) / 6)

#define HASH_SIZE 32 /* bytes */
#define HASH_LEN BYTES2CHARS(HASH_SIZE) /* base-64 chars */
#define YESCRYPT_FLAGS (YESCRYPT_RW | YESCRYPT_PWXFORM)

static int yescrypt_bsty(const uint8_t * passwd, size_t passwdlen,
                         const uint8_t * salt, size_t saltlen, uint64_t N, uint32_t r, uint32_t p,
                         uint8_t * buf, size_t buflen)
{
    static __thread int initialized = 0;
    static __thread yescrypt_shared_t shared;
    static __thread yescrypt_local_t local;
    int retval;
    if (!initialized) {
        /* "shared" could in fact be shared, but it's simpler to keep it private
         * along with "local".  It's dummy and tiny anyway. */
        if (yescrypt_init_shared(&shared, NULL, 0,
                                 0, 0, 0, YESCRYPT_SHARED_DEFAULTS, 0, NULL, 0))
            return -1;
        if (yescrypt_init_local(&local)) {
            yescrypt_free_shared(&shared);
            return -1;
        }
        initialized = 1;
    }
    retval = yescrypt_kdf(&shared, &local,
                          passwd, passwdlen, salt, saltlen, N, r, p, 0, YESCRYPT_FLAGS,
                          buf, buflen);		
#if 0		
    if (yescrypt_free_local(&local)) {
        yescrypt_free_shared(&shared);
        return -1;
    }
    if (yescrypt_free_shared(&shared))
        return -1;
    initialized = 0;
#endif		
    return retval;
}

void yescrypt_hash(const char *input, char *output)
{
    yescrypt_bsty((const uint8_t *)input, 80, (const uint8_t *) input, 80, 2048, 8, 1, (uint8_t *)output, 32);
}
