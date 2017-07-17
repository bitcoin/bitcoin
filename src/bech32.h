// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef BITCOIN_BECH32_H
#define BITCOIN_BECH32_H

#include "chainparams.h"

#include <string>
#include <vector>

int Bech32Encode(char *output, const char *hrp, const uint8_t *data, size_t data_len);
int Bech32Decode(char* hrp, uint8_t *data, size_t *data_len, const char *input);


#endif // BITCOIN_BECH32_H
