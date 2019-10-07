// Copyright (c) 2017-2018 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CURVE25519_H
#define BITCOIN_CRYPTO_CURVE25519_H

#include <stdint.h>
#include <stdlib.h>

namespace crypto {

void curve25519_kengen(unsigned char publicKey[32], unsigned char signingKey[32], unsigned char privateKey[32]);
int curve25519_sign(unsigned char sign[32], const unsigned char data[32], const unsigned char privateKey[32], const unsigned char signingKey[32]);
void curve25519_verify(unsigned char verify[32], const unsigned char sign[32], const unsigned char data[32], const unsigned char publicKey[32]);

}

#endif // BITCOIN_CRYPTO_CURVE25519_H