#ifndef INTEGRATION_REFERENCE_BTC_INTERPRETER_HPP
#define INTEGRATION_REFERENCE_BTC_INTERPRETER_HPP


#include <script/script.h>
#include <script/script_error.h>
#include <vbk/entity/pop.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/service_locator.hpp>

namespace VeriBlock {

namespace {
inline bool set_error(ScriptError* ret, const ScriptError serror)
{
    if (ret)
        *ret = serror;
    return false;
}
typedef std::vector<unsigned char> valtype;

#define stacktop(i) (stack.at(stack.size() + (i)))
inline void popstack(std::vector<valtype>& stack)
{
    if (stack.empty())
        throw std::runtime_error("popstack(): stack empty");
    stack.pop_back();
}

} // namespace

inline bool EvalScriptImpl(const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, VeriBlock::Publications* publications, VeriBlock::Context* context, PopTxType* type, bool with_checks)
{
    CScript::const_iterator pc = script.begin();
    CScript::const_iterator pend = script.end();
    static const valtype vchFalse(0);
    static const valtype vchTrue(1, 1);
    opcodetype opcode;
    std::vector<unsigned char> vchPushValue;
    auto& config = getService<Config>();
    auto& pop = getService<PopService>();
    Publications pub;
    Context ctx;

    bool isPublicationsTx = false;
    bool isContextTx = false;

    if (script.size() > config.max_pop_script_size) {
        return set_error(serror, SCRIPT_ERR_SCRIPT_SIZE);
    }

    try {
        while (pc < pend) {
            if (!script.GetOp(pc, opcode, vchPushValue)) {
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }

            if (!vchPushValue.empty()) {
                stack.push_back(vchPushValue);
                continue;
            }

            switch (opcode) {
            case OP_POPBTCHEADER: {
                // we do not allow single pop tx simultaneously contain publications and context
                isContextTx = true;
                if (isPublicationsTx) {
                    return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
                }

                const auto& el = stacktop(-1);
                if (el.size() != config.btc_header_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                ctx.btc.push_back(el);
                popstack(stack);

                break;
            }
            case OP_POPVBKHEADER: {
                // we do not allow single pop tx simultaneously contain publications and context
                isContextTx = true;
                if (isPublicationsTx) {
                    return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
                }

                const auto& el = stacktop(-1);
                if (el.size() != config.vbk_header_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                ctx.vbk.push_back(el);
                popstack(stack);

                break;
            }
            case OP_CHECKATV: {
                // we do not allow single pop tx simultaneously contain publications and context
                isPublicationsTx = true;
                if (isContextTx) {
                    return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
                }

                // tx has one ATV
                if (!pub.atv.empty()) {
                    return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                }

                // validate ATV size
                const auto& el = stacktop(-1);
                if (el.size() > config.max_atv_size || el.size() < config.min_atv_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                // validate ATV content
                if (with_checks && !pop.checkATVinternally(el)) {
                    return set_error(serror, SCRIPT_ERR_VBK_ATVFAIL);
                }

                pub.atv = el;
                popstack(stack);
                break;
            }
            case OP_CHECKVTB: {
                // we do not allow single pop tx simultaneously contain publications and context
                isPublicationsTx = true;
                if (isContextTx) {
                    return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
                }

                // validate VTB size
                const auto& el = stacktop(-1);
                if (el.size() > config.max_vtb_size || el.size() < config.min_vtb_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                // validate VTB content
                if (with_checks && !pop.checkVTBinternally(el)) {
                    return set_error(serror, SCRIPT_ERR_VBK_VTBFAIL);
                }

                // tx has many VTBs
                pub.vtbs.push_back(el);
                popstack(stack);
                break;
            }
            case OP_CHECKPOP: {
                // OP_CHECKPOP should be last opcode
                if (script.GetOp(pc, opcode, vchPushValue)) {
                    // we could read next opcode. extra opcodes is an error
                    return set_error(serror, SCRIPT_ERR_VBK_EXTRA_OPCODE);
                }

                // stack should be empty at this point
                if (!stack.empty()) {
                    return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
                }

                // we did all checks, put true on stack to signal successful execution
                stack.push_back(vchTrue);
                break;
            }
            default:
                // forbid any other opcodes in pop transactions
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }
        }
    } catch (...) {
        return set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    }

    if (isPublicationsTx && pub.atv.empty()) {
        // must contain non-empty ATV
        return set_error(serror, SCRIPT_ERR_VBK_ATVFAIL);
    }

    // set return value of publications
    if (publications != nullptr) {
        *publications = pub;
    }

    // set return value of context
    if (context != nullptr) {
        *context = ctx;
    }

    // set return value of tx type
    if (type != nullptr) {
        // can not be simultaneously context and publication tx
        if (isContextTx && isPublicationsTx) {
            return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
        }

        if (isContextTx) {
            *type = PopTxType::CONTEXT;
        } else if (isPublicationsTx) {
            *type = PopTxType::PUBLICATIONS;
        } else {
            *type = PopTxType::UNKNOWN;
            return false;
        }
    }

    return true;
}

} // namespace VeriBlock

#undef stacktop

#endif //INTEGRATION_REFERENCE_BTC_INTERPRETER_HPP
