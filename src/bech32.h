// Copyright (c) 2017 Pieter Wuille
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BECH32_H
#define BITCOIN_BECH32_H

#include "chainparams.h"

#include <string>
#include <vector>

int bech32_encode(char *output, const char *hrp, const uint8_t *data, size_t data_len);
int bech32_decode(char* hrp, uint8_t *data, size_t *data_len, const char *input);

int Bech32Encode(char *output, const char *hrp, const uint8_t *data, size_t data_len);
int Bech32Decode(char* hrp, uint8_t *data, size_t *data_len, const char *input);


#endif // BITCOIN_BECH32_H
