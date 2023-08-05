// Copyright (c) 2020-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TRACE_H
#define BITCOIN_UTIL_TRACE_H

#ifdef ENABLE_TRACING

#include <util/macros.h>

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

// The RAII object calls the given operation on exit.
// This is useful to reliably trace at any return point.
template <typename Op>
class TraceOnExit
{
    Op m_op;

public:
    TraceOnExit(TraceOnExit&&) = delete;
    TraceOnExit(TraceOnExit const&) = delete;
    TraceOnExit& operator=(TraceOnExit&&) = delete;
    TraceOnExit& operator=(TraceOnExit const&) = delete;

    explicit TraceOnExit(Op op) : m_op(op)
    {
    }

    ~TraceOnExit()
    {
        m_op();
    }
};

// Traces entry & exit point. This can be useful to gather statistics
// like average runtime of a code block that has multiple exit points.
//
// The given trace context is called with a single argument 1 for entry
// and 0 for the exit.
#define TRACE_RAII(context, event)                \
    TRACE1(context, event, 1);                    \
    auto UNIQUE_NAME(tracer) = ::TraceOnExit([] { \
        TRACE1(context, event, 0);                \
    })

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

#define TRACE_RAII(context, event)

#endif


#endif // BITCOIN_UTIL_TRACE_H
