// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef KEY_UTIL_H
#define KEY_UTIL_H

#include <vector>
#include <inttypes.h>

uint32_t BitcoinChecksum(uint8_t *p, uint32_t nBytes);
void AppendChecksum(std::vector<uint8_t> &data);
bool VerifyChecksum(const std::vector<uint8_t> &data);


#endif  // KEY_UTIL_H
