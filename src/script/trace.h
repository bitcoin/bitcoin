// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_TRACE_H
#define BITCOIN_SCRIPT_TRACE_H

#include <script/script.h>
#include <script/script_error.h>

#include <functional>
#include <span>
#include <vector>

enum class ScriptTraceFrameKind : uint8_t {
    Begin = 0,
    Step = 1,
    End = 2,
};

struct ScriptTraceFrame {
    ScriptTraceFrameKind kind;
    std::span<const std::vector<unsigned char>> stack;
    std::span<const std::vector<unsigned char>> altstack;
    const CScript& script;
    uint32_t opcode_pos;
    bool exec;
    uint8_t opcode;
    int op_count;
    uint8_t sig_version;
    const unsigned char* tapleaf_hash;
    uint32_t codeseparator_pos;
    ScriptError script_error;
};

using ScriptTraceCallback = std::function<void(const ScriptTraceFrame&)>;

void TraceScript(const ScriptTraceCallback& callback, const ScriptTraceFrame& frame);
void ScriptTraceRegisterCallback(ScriptTraceCallback callback);
ScriptTraceCallback ScriptTraceGetCallback();

#ifdef ENABLE_SCRIPT_TRACE

struct ScriptTraceScope {
    const ScriptTraceCallback m_callback;
    std::vector<std::vector<unsigned char>>& m_stack;
    const CScript& m_script;
    uint32_t& m_opcode_pos;
    std::vector<std::vector<unsigned char>>& m_altstack;
    std::function<bool()> m_exec_fn;
    int& m_op_count;
    uint8_t m_sig_version;
    const unsigned char* m_tapleaf_hash;
    uint32_t& m_codeseparator_pos;
    const ScriptError* m_error;

    ScriptTraceScope(std::vector<std::vector<unsigned char>>& stack, const CScript& script, uint32_t& opcode_pos,
                          std::vector<std::vector<unsigned char>>& altstack, std::function<bool()> exec_fn,
                          int& nOpCount, uint8_t sigversion, const unsigned char* tapleaf_hash,
                          uint32_t& codeseparator_pos, const ScriptError* error) :
        m_callback{ScriptTraceGetCallback()},
        m_stack{stack},
        m_script{script},
        m_opcode_pos{opcode_pos},
        m_altstack{altstack},
        m_exec_fn{std::move(exec_fn)},
        m_op_count{nOpCount},
        m_sig_version{sigversion},
        m_tapleaf_hash{tapleaf_hash},
        m_codeseparator_pos{codeseparator_pos},
        m_error{error}
    {
        Emit(ScriptTraceFrameKind::Begin, m_exec_fn(), /*opcode=*/0, SCRIPT_ERR_OK);
    }

    void Step(bool exec, uint8_t opcode)
    {
        Emit(ScriptTraceFrameKind::Step, exec, opcode, SCRIPT_ERR_OK);
    }

    ~ScriptTraceScope()
    {
        Emit(ScriptTraceFrameKind::End, m_exec_fn(), /*opcode=*/0, m_error ? *m_error : SCRIPT_ERR_UNKNOWN_ERROR);
    }

private:
    void Emit(ScriptTraceFrameKind kind, bool exec, uint8_t opcode, ScriptError error) const
    {
        if (!m_callback) return;
        auto frame = ScriptTraceFrame{
            .kind = kind,
            .stack = m_stack,
            .altstack = m_altstack,
            .script = m_script,
            .opcode_pos = m_opcode_pos,
            .exec = exec,
            .opcode = opcode,
            .op_count = m_op_count,
            .sig_version = m_sig_version,
            .tapleaf_hash = m_tapleaf_hash,
            .codeseparator_pos = m_codeseparator_pos,
            .script_error = error,
        };
        TraceScript(m_callback, frame);
    }
};

#define SCRIPT_TRACE_SCOPE(stack, script, opcode_pos, altstack, exec_fn, nOpCount, sigversion, tapleaf_hash, codeseparator_pos, error) \
    ScriptTraceScope script_trace_scope { stack, script, opcode_pos, altstack, exec_fn, nOpCount, static_cast<uint8_t>(sigversion), tapleaf_hash, codeseparator_pos, error }

#define SCRIPT_TRACE_STEP(fExec, opcode) \
    script_trace_scope.Step(fExec, static_cast<uint8_t>(opcode))

#else
#define SCRIPT_TRACE_SCOPE(stack, script, opcode_pos, altstack, exec_fn, nOpCount, sigversion, tapleaf_hash, codeseparator_pos, error) static_assert(true)
#define SCRIPT_TRACE_STEP(fExec, opcode) static_assert(true)
#endif // ENABLE_SCRIPT_TRACE

#endif // BITCOIN_SCRIPT_TRACE_H
