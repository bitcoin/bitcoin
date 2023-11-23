// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TRACE_H
#define BITCOIN_UTIL_TRACE_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#ifdef ENABLE_TRACING

// Setting SDT_USE_VARIADIC let's systemtap (sys/sdt.h) know that we want to use
// the optional variadic macros to define tracepoints.
#define SDT_USE_VARIADIC 1

#include <sys/sdt.h>

// A USDT tracepoint with zero to twelve arguments.
#define TRACEPOINT(context, ...) \
    STAP_PROBEV(context, __VA_ARGS__);
#else

#define TRACEPOINT(context, ...)

#endif


#endif // BITCOIN_UTIL_TRACE_H
