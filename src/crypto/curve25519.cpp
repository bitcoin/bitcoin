// Copyright (c) 2017-2018 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/*
 * Ported parts from Java to C# and refactored by Hans Wolff, 17/09/2013
 * Original: https://github.com/hanswolff/curve25519
 *
 * Ported from C to Java by Dmitry Skiba [sahn0], 23/02/08.
 * Original: http://code.google.com/p/curve25519-java/
 *
 * Generic 64-bit integer implementation of Curve25519 ECDH
 * Written by Matthijs van Duin, 200608242056
 * Public domain.
 *
 * Based on work by Daniel J Bernstein, http://cr.yp.to/ecdh.html
 */

#include <crypto/curve25519.h>
#include <crypto/curve/curve25519_i64.h>
#include <crypto/sha256.h>

#include <cstring>

namespace crypto {

void curve25519_kengen(unsigned char publicKey[32], unsigned char signingKey[32], unsigned char privateKey[32])
{
    keygen25519(publicKey, signingKey, privateKey);
}

int curve25519_sign(unsigned char sign[32], const unsigned char data[32], const unsigned char privateKey[32], const unsigned char signingKey[32])
{
    return sign25519(sign, data, privateKey, signingKey);
}

void curve25519_verify(unsigned char verify[32], const unsigned char sign[32], const unsigned char data[32], const unsigned char publicKey[32])
{
    verify25519(verify, sign, data, publicKey);
}

}