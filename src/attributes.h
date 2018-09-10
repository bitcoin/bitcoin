// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ATTRIBUTES_H
#define BITCOIN_ATTRIBUTES_H

#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
#  define MAYBE_UNUSED [[maybe_unused]]
#elif defined(__GNUC__)
#  define MAYBE_UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
#  define MAYBE_UNUSED __pragma(warning(suppress:4100 4101 4189))
#else
#  define MAYBE_UNUSED
#endif

#endif // BITCOIN_ATTRIBUTES_H
