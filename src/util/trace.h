// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TRACE_H
#define BITCOIN_UTIL_TRACE_H

#ifdef ENABLE_TRACING

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

#endif


#endif /* BITCOIN_UTIL_TRACE_H */
