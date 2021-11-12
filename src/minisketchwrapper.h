// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_MINISKETCHWRAPPER_H
#define SYSCOIN_MINISKETCHWRAPPER_H

#include <minisketch.h>
#include <cstddef>
#include <cstdint>

/** Wrapper around Minisketch::Minisketch(32, implementation, capacity). */
Minisketch MakeMinisketch32(size_t capacity);
/** Wrapper around Minisketch::CreateFP. */
Minisketch MakeMinisketch32FP(size_t max_elements, uint32_t fpbits);

#endif // SYSCOIN_MINISKETCHWRAPPER_H
