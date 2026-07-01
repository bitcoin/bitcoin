// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/trace.h>

#include <sync.h>

#include <exception>
#include <utility>

static GlobalMutex g_script_trace_mutex;
static ScriptTraceCallback g_script_trace_callback GUARDED_BY(g_script_trace_mutex){nullptr};

void TraceScript(const ScriptTraceCallback& callback, const ScriptTraceFrame& trace_frame)
{
    if (callback) {
        try {
            callback(trace_frame);
        } catch (...) {
            std::terminate();
        }
    }
}

void ScriptTraceRegisterCallback(ScriptTraceCallback callback)
{
    // Copy the callback in case another thread is busy invoking the callback
    ScriptTraceCallback old_callback;
    {
        LOCK(g_script_trace_mutex);
        old_callback = std::move(g_script_trace_callback);
        g_script_trace_callback = std::move(callback);
    }
}

ScriptTraceCallback ScriptTraceGetCallback()
{
    return WITH_LOCK(g_script_trace_mutex, return g_script_trace_callback);
}
