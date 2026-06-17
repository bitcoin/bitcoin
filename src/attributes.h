// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ATTRIBUTES_H
#define BITCOIN_ATTRIBUTES_H

#if defined(__clang__)
#  if __has_attribute(lifetimebound)
#    define LIFETIMEBOUND [[clang::lifetimebound]]
#  else
#    define LIFETIMEBOUND
#  endif
#else
#  define LIFETIMEBOUND
#endif

#if !defined(_DEBUG) && !defined(__NO_INLINE__) && !defined(__OPTIMIZE_SIZE__)
#  if (defined(__GNUC__) || defined(__clang__)) && defined(__OPTIMIZE__)
#    define ALWAYS_INLINE inline __attribute__((always_inline))
#  elif defined(_MSC_VER)
#    define ALWAYS_INLINE __forceinline
#  endif
#endif
#ifndef ALWAYS_INLINE
#  define ALWAYS_INLINE inline
#endif

#endif // BITCOIN_ATTRIBUTES_H
