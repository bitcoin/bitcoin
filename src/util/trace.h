// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TRACE_H
#define BITCOIN_UTIL_TRACE_H

#ifdef ENABLE_TRACING

#ifdef __APPLE__

#include <util/macros.h>
#include <util/probes.h>

// since the dtrace macros are automatically generated in uppercase, additional
// macros are needed to translate the lowercase context & event names into the
// required uppercase CONTEXT_EVENT macros
#define net_inbound_message NET_INBOUND_MESSAGE
#define net_outbound_message NET_OUTBOUND_MESSAGE
#define validation_block_connected VALIDATION_BLOCK_CONNECTED
#define utxocache_add UTXOCACHE_ADD
#define utxocache_spent UTXOCACHE_SPENT
#define utxocache_uncache UTXOCACHE_UNCACHE
#define utxocache_flush UTXOCACHE_FLUSH
#define coin_selection_selected_coins COIN_SELECTION_SELECTED_COINS
#define coin_selection_normal_create_tx_internal COIN_SELECTION_NORMAL_CREATE_TX_INTERNAL
#define coin_selection_attempting_aps_create_tx COIN_SELECTION_ATTEMPTING_APS_CREATE_TX
#define coin_selection_aps_create_tx_internal COIN_SELECTION_APS_CREATE_TX_INTERNAL

// The DTRACE_PROBE*(context, event, ...args) macros are not supported on macOS.
// Instead we are using macros of the format CONTEXT_EVENT(...args).
#define TRACE(context, event) PASTE2(PASTE2(context, _), event)()
#define TRACE1(context, event, a) PASTE2(PASTE2(context, _), event)(a)
#define TRACE2(context, event, a, b) PASTE2(PASTE2(context, _), event)(a, b)
#define TRACE3(context, event, a, b, c) PASTE2(PASTE2(context, _), event)(a, b, c)
#define TRACE4(context, event, a, b, c, d) PASTE2(PASTE2(context, _), event)(a, b, c, d)
#define TRACE5(context, event, a, b, c, d, e) PASTE2(PASTE2(context, _), event)(a, b, c, d, e)
#define TRACE6(context, event, a, b, c, d, e, f) PASTE2(PASTE2(context, _), event)(a, b, c, d, e, f)
#define TRACE7(context, event, a, b, c, d, e, f, g) PASTE2(PASTE2(context, _), event)(a, b, c, d, e, f, g)
#define TRACE8(context, event, a, b, c, d, e, f, g, h) PASTE2(PASTE2(context, _), event)(a, b, c, d, e, f, g, h)
#define TRACE9(context, event, a, b, c, d, e, f, g, h, i) PASTE2(PASTE2(context, _), event)(a, b, c, d, e, f, g, h, i)
#define TRACE10(context, event, a, b, c, d, e, f, g, h, i, j) PASTE2(PASTE2(context, _), event)(a, b, c, d, e, f, g, h, i, j)
#define TRACE11(context, event, a, b, c, d, e, f, g, h, i, j, k) PASTE2(PASTE2(context, _), event)(a, b, c, d, e, f, g, h, i, j, k)
#define TRACE12(context, event, a, b, c, d, e, f, g, h, i, j, k, l) PASTE2(PASTE2(context, _), event)(a, b, c, d, e, f, g, h, i, j, k, l)

#else

#include <sys/sdt.h>

#define TRACE(context, event) DTRACE_PROBE(context, event)
#define TRACE1(context, event, a) DTRACE_PROBE1(context, event, a)
#define TRACE2(context, event, a, b) DTRACE_PROBE2(context, event, a, b)
#define TRACE3(context, event, a, b, c) DTRACE_PROBE3(context, event, a, b, c)
#define TRACE4(context, event, a, b, c, d) DTRACE_PROBE4(context, event, a, b, c, d)
#define TRACE5(context, event, a, b, c, d, e) DTRACE_PROBE5(context, event, a, b, c, d, e)
#define TRACE6(context, event, a, b, c, d, e, f) DTRACE_PROBE6(context, event, a, b, c, d, e, f)
#define TRACE7(context, event, a, b, c, d, e, f, g) DTRACE_PROBE7(context, event, a, b, c, d, e, f, g)
#define TRACE8(context, event, a, b, c, d, e, f, g, h) DTRACE_PROBE8(context, event, a, b, c, d, e, f, g, h)
#define TRACE9(context, event, a, b, c, d, e, f, g, h, i) DTRACE_PROBE9(context, event, a, b, c, d, e, f, g, h, i)
#define TRACE10(context, event, a, b, c, d, e, f, g, h, i, j) DTRACE_PROBE10(context, event, a, b, c, d, e, f, g, h, i, j)
#define TRACE11(context, event, a, b, c, d, e, f, g, h, i, j, k) DTRACE_PROBE11(context, event, a, b, c, d, e, f, g, h, i, j, k)
#define TRACE12(context, event, a, b, c, d, e, f, g, h, i, j, k, l) DTRACE_PROBE12(context, event, a, b, c, d, e, f, g, h, i, j, k, l)

#endif // __APPLE__

#else

#define TRACE(context, event)
#define TRACE1(context, event, a)
#define TRACE2(context, event, a, b)
#define TRACE3(context, event, a, b, c)
#define TRACE4(context, event, a, b, c, d)
#define TRACE5(context, event, a, b, c, d, e)
#define TRACE6(context, event, a, b, c, d, e, f)
#define TRACE7(context, event, a, b, c, d, e, f, g)
#define TRACE8(context, event, a, b, c, d, e, f, g, h)
#define TRACE9(context, event, a, b, c, d, e, f, g, h, i)
#define TRACE10(context, event, a, b, c, d, e, f, g, h, i, j)
#define TRACE11(context, event, a, b, c, d, e, f, g, h, i, j, k)
#define TRACE12(context, event, a, b, c, d, e, f, g, h, i, j, k, l)

#endif // ENABLE_TRACING


#endif // BITCOIN_UTIL_TRACE_H
