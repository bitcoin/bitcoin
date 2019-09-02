// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_MINISCRIPT_H
#define BITCOIN_SCRIPT_MINISCRIPT_H

#include <algorithm>
#include <numeric>
#include <memory>
#include <string>
#include <vector>

#include <stdlib.h>
#include <assert.h>

#include <script/script.h>
#include <span.h>
#include <util/spanparsing.h>
#include <util/strencodings.h>
#include <util/vector.h>

namespace miniscript {

/** This type encapsulates the miniscript type system properties.
 *
 * Every miniscript expression is one of 4 basic types, and additionally has
 * a number of boolean type properties.
 *
 * The basic types are:
 * - "B" Base:
 *   - Takes its inputs from the top of the stack.
 *   - When satisfied, pushes a nonzero value of up to 4 bytes onto the stack.
 *   - When dissatisfied, pushes a 0 onto the stack.
 *   - This is used for most expressions, and required for the top level one.
 *   - For example: older(n) = <n> OP_CHECKSEQUENCEVERIFY.
 * - "V" Verify:
 *   - Takes its inputs from the top of the stack.
 *   - When satisfactied, pushes nothing.
 *   - Cannot be dissatisfied.
 *   - This is obtained by adding an OP_VERIFY to a B, modifying the last opcode
 *     of a B to its -VERIFY version (only for OP_CHECKSIG, OP_CHECKSIGVERIFY
 *     and OP_EQUAL), or using IFs where both branches are also Vs.
 *   - For example vc:pk(key) = <key> OP_CHECKSIGVERIFY
 * - "K" Key:
 *   - Takes its inputs from the top of the stack.
 *   - Becomes a B when followed by OP_CHECKSIG.
 *   - Always pushes a public key onto the stack, for which a signature is to be
 *     provided to satisfy the expression.
 *   - For example pk_h(key) = OP_DUP OP_HASH160 <Hash160(key)> OP_EQUALVERIFY
 * - "W" Wrapped:
 *   - Takes its input from one below the top of the stack.
 *   - When satisfied, pushes a nonzero value (like B) on top of the stack, or one below.
 *   - When dissatisfied, pushes 0 op top of the stack or one below.
 *   - Is always "OP_SWAP [B]" or "OP_TOALTSTACK [B] OP_FROMALTSTACK".
 *   - For example sc:pk(key) = OP_SWAP <key> OP_CHECKSIG
 *
 * There a type properties that help reasoning about correctness:
 * - "z" Zero-arg:
 *   - Is known to always consume exactly 0 stack elements.
 *   - For example after(n) = <n> OP_CHECKLOCKTIMEVERIFY
 * - "o" One-arg:
 *   - Is known to always consume exactly 1 stack element.
 *   - Conflicts with property 'z'
 *   - For example sha256(hash) = OP_SIZE 32 OP_EQUALVERIFY OP_SHA256 <hash> OP_EQUAL
 * - "n" Nonzero:
 *   - For every way this expression can be satisfied, a satisfaction exists that never needs
 *     a zero top stack element.
 *   - Conflicts with property 'z' and with type 'W'.
 * - "d" Dissatisfiable:
 *   - There is an easy way to construct a dissatisfaction for this expression.
 *   - Conflicts with type 'V'.
 * - "u" Unit:
 *   - In case of satisfaction, an exact 1 is put on the stack (rather than just nonzero).
 *   - Conflicts with type 'V'.
 *
 * Additional type properties help reasoning about nonmalleability:
 * - "e" Expression:
 *   - This implies property 'd', but the dissatisfaction is nonmalleable.
 *   - This generally requires 'e' for all subexpressions which are invoked for that
 *     dissatifsaction, and property 'f' for the unexecuted subexpressions in that case.
 *   - Conflicts with type 'V'.
 * - "f" Forced:
 *   - Dissatisfactions (if any) for this expression always involve at least one signature.
 *   - Is always true for type 'V'.
 * - "s" Safe:
 *   - Satisfactions for this expression always involve at least one signature.
 * - "m" Nonmalleable:
 *   - For every way this expression can be satisfied (which may be none),
 *     a nonmalleable satisfaction exists.
 *   - This generally requires 'm' for all subexpressions, and 'e' for all subexpressions
 *     which are dissatisfied when satisfying the parent.
 *
 * One final type property is an implementation detail:
 * - "x" Expensive verify:
 *   - Expressions with this property have a script whose last opcode is not EQUAL, CHECKSIG, or CHECKMULTISIG.
 *   - Not having this property means that it can be converted to a V at no cost (by switching to the
 *     -VERIFY version of the last opcode).
 *
 * For each of these properties the subset rule holds: an expression with properties X, Y, and Z, is also
 * valid in places where an X, a Y, a Z, an XY, ... is expected.
*/
class Type {
    //! Internal bitmap of properties (see ""_mst operator for details).
    uint16_t m_flags;

    //! Internal constructed used by the ""_mst operator.
    explicit constexpr Type(uint16_t flags) : m_flags(flags) {}

public:
    //! The only way to publicly construct a Type is using this literal operator.
    friend constexpr Type operator"" _mst(const char* c, size_t l);

    //! Compute the type with the union of properties.
    constexpr Type operator|(Type x) const { return Type(m_flags | x.m_flags); }

    //! Compute the type with the intersection of properties.
    constexpr Type operator&(Type x) const { return Type(m_flags & x.m_flags); }

    //! Check whether the left hand's properties are superset of the right's (= left is a subtype of right).
    constexpr bool operator<<(Type x) const { return (x.m_flags & ~m_flags) == 0; }

    //! Comparison operator to enable use in sets/maps (total ordering incompatible with <<).
    constexpr bool operator<(Type x) const { return m_flags < x.m_flags; }

    //! Equality operator.
    constexpr bool operator==(Type x) const { return m_flags == x.m_flags; }

    //! The empty type if x is false, itself otherwise.
    constexpr Type If(bool x) const { return Type(x ? m_flags : 0); }
};

//! Literal operator to construct Type objects.
inline constexpr Type operator"" _mst(const char* c, size_t l) {
    return l == 0 ? Type(0) : operator"" _mst(c + 1, l - 1) | Type(
        *c == 'B' ? 1 << 0 : // Base type
        *c == 'V' ? 1 << 1 : // Verify type
        *c == 'K' ? 1 << 2 : // Key type
        *c == 'W' ? 1 << 3 : // Wrapped type
        *c == 'z' ? 1 << 4 : // Zero-arg property
        *c == 'o' ? 1 << 5 : // One-arg property
        *c == 'n' ? 1 << 6 : // Nonzero arg property
        *c == 'd' ? 1 << 7 : // Dissatisfiable property
        *c == 'u' ? 1 << 8 : // Unit property
        *c == 'e' ? 1 << 9 : // Expression property
        *c == 'f' ? 1 << 10 : // Forced property
        *c == 's' ? 1 << 11 : // Safe property
        *c == 'm' ? 1 << 12 : // Nonmalleable property
        *c == 'x' ? 1 << 13 : // Expensive verify
        (throw std::logic_error("Unknown character in _mst literal"), 0)
    );
}

template<typename Key> struct Node;
template<typename Key> using NodeRef = std::shared_ptr<const Node<Key>>;

//! Construct a miniscript node as a shared_ptr.
template<typename Key, typename... Args>
NodeRef<Key> MakeNodeRef(Args&&... args) { return std::make_shared<const Node<Key>>(std::forward<Args>(args)...); }

//! The different node types in miniscript.
enum class NodeType {
    JUST_0,     //!< OP_0
    JUST_1,    //!< OP_1
    PK,        //!< [key]
    PK_H,      //!< OP_DUP OP_HASH160 [keyhash] OP_EQUALVERFIFY
    OLDER,     //!< [n] OP_CHECKSEQUENCEVERIFY
    AFTER,     //!< [n] OP_CHECKLOCKTIMEVERIFY
    SHA256,    //!< OP_SIZE 32 OP_EQUALVERIFY OP_SHA256 [hash] OP_EQUAL
    HASH256,   //!< OP_SIZE 32 OP_EQUALVERIFY OP_HASH256 [hash] OP_EQUAL
    RIPEMD160, //!< OP_SIZE 32 OP_EQUALVERIFY OP_RIPEMD160 [hash] OP_EQUAL
    HASH160,   //!< OP_SIZE 32 OP_EQUALVERIFY OP_HASH160 [hash] OP_EQUAL
    WRAP_A,    //!< OP_TOALTSTACK [X] OP_FROMALTSTACK
    WRAP_S,    //!< OP_SWAP [X]
    WRAP_C,    //!< [X] OP_CHECKSIG
    WRAP_D,    //!< OP_DUP OP_IF [X] OP_ENDIF
    WRAP_V,    //!< [X] OP_VERIFY (or -VERIFY version of last opcode in X)
    WRAP_J,    //!< OP_SIZE OP_0NOTEQUAL OP_IF [X] OP_ENDIF
    WRAP_N,    //!< [X] OP_0NOTEQUAL
    AND_V,     //!< [X] [Y]
    AND_B,     //!< [X] [Y] OP_BOOLAND
    OR_B,      //!< [X] [Y] OP_BOOLOR
    OR_C,      //!< [X] OP_NOTIF [Y] OP_ENDIF
    OR_D,      //!< [X] OP_IFDUP OP_NOTIF [Y] OP_ENDIF
    OR_I,      //!< IF [X] OP_ELSE [Y] OP_ENDIF
    ANDOR,     //!< [X] OP_NOTIF [Z] OP_ELSE [Y] OP_ENDIF
    THRESH,    //!< [X1] ([Xn] OP_ADD)* [k] OP_EQUAL
    THRESH_M,  //!< [k] [key_n]* [n] OP_CHECKMULTISIG
    // AND_N(X,Y) is represented as ANDOR(X,Y,0)
    // WRAP_T(X) is represented as AND_V(X,1)
    // WRAP_L(X) is represented as OR_I(0,X)
    // WRAP_U(X) is represented as OR_I(X,0)
};

namespace internal {

//! Helper function for Node::CalcType.
Type ComputeType(NodeType nodetype, Type x, Type y, Type z, const std::vector<Type>& sub_types, uint32_t k, size_t data_size, size_t n_subs, size_t n_keys);

//! Helper function for Node::CalcScriptLen.
size_t ComputeScriptLen(NodeType nodetype, Type sub0typ, size_t subsize, uint32_t k, size_t n_subs, size_t n_keys);

//! A helper sanitizer/checker for the output of CalcType.
Type SanitizeType(Type x);

} // namespace internal

//! A node in a miniscript expression.
template<typename Key>
struct Node {
    //! What node type this node is.
    const NodeType nodetype;
    //! The k parameter (time for OLDER/AFTER, threshold for THRESH(_M))
    const uint32_t k = 0;
    //! The keys used by this expression (only for PK/PK_H/THRESH_M)
    const std::vector<Key> keys;
    //! The data bytes in this expression (only for HASH160/HASH256/SHA256/RIPEMD10).
    const std::vector<unsigned char> data;
    //! Subexpressions (for WRAP_*/AND_*/OR_*/ANDOR/THRESH)
    const std::vector<NodeRef<Key>> subs;

private:
    //! Cached expression type (computed by CalcType and fed through SanitizeType).
    const Type typ;
    //! Cached script length (computed by CalcScriptLen).
    const size_t scriptlen;

    //! Compute the length of the script for this miniscript (including children).
    size_t CalcScriptLen() const {
        size_t subsize = 0;
        for (const auto& sub : subs) {
            subsize += sub->ScriptSize();
        }
        Type sub0type = subs.size() > 0 ? subs[0]->GetType() : ""_mst;
        return internal::ComputeScriptLen(nodetype, sub0type, subsize, k, subs.size(), keys.size());
    }

    //! Compute the type for this miniscript.
    Type CalcType() const {
        using namespace internal;

        // THRESH has a variable number of subexpression
        std::vector<Type> sub_types;
        if (nodetype == NodeType::THRESH) {
            for (const auto& sub : subs) sub_types.push_back(sub->GetType());
        }
        // All other nodes than THRESH can be computed just from the types of the 0-3 subexpexpressions.
        Type x = subs.size() > 0 ? subs[0]->GetType() : ""_mst;
        Type y = subs.size() > 1 ? subs[1]->GetType() : ""_mst;
        Type z = subs.size() > 2 ? subs[2]->GetType() : ""_mst;

        return SanitizeType(ComputeType(nodetype, x, y, z, sub_types, k, data.size(), subs.size(), keys.size()));
    }

    //! Internal code for ToScript.
    template<typename Ctx>
    CScript MakeScript(const Ctx& ctx, bool verify = false) const {
        std::vector<unsigned char> bytes;
        switch (nodetype) {
            case NodeType::PK: return CScript() << ctx.ToPKBytes(keys[0]);
            case NodeType::PK_H: return CScript() << OP_DUP << OP_HASH160 << ctx.ToPKHBytes(keys[0]) << OP_EQUALVERIFY;
            case NodeType::OLDER: return CScript() << k << OP_CHECKSEQUENCEVERIFY;
            case NodeType::AFTER: return CScript() << k << OP_CHECKLOCKTIMEVERIFY;
            case NodeType::SHA256: return CScript() << OP_SIZE << 32 << OP_EQUALVERIFY << OP_SHA256 << data << (verify ? OP_EQUALVERIFY : OP_EQUAL);
            case NodeType::RIPEMD160: return CScript() << OP_SIZE << 32 << OP_EQUALVERIFY << OP_RIPEMD160 << data << (verify ? OP_EQUALVERIFY : OP_EQUAL);
            case NodeType::HASH256: return CScript() << OP_SIZE << 32 << OP_EQUALVERIFY << OP_HASH256 << data << (verify ? OP_EQUALVERIFY : OP_EQUAL);
            case NodeType::HASH160: return CScript() << OP_SIZE << 32 << OP_EQUALVERIFY << OP_HASH160 << data << (verify ? OP_EQUALVERIFY : OP_EQUAL);
            case NodeType::WRAP_A: return (CScript() << OP_TOALTSTACK) + subs[0]->MakeScript(ctx) + (CScript() << OP_FROMALTSTACK);
            case NodeType::WRAP_S: return (CScript() << OP_SWAP) + subs[0]->MakeScript(ctx, verify);
            case NodeType::WRAP_C: return subs[0]->MakeScript(ctx) + CScript() << (verify ? OP_CHECKSIGVERIFY : OP_CHECKSIG);
            case NodeType::WRAP_D: return (CScript() << OP_DUP << OP_IF) + subs[0]->MakeScript(ctx) + (CScript() << OP_ENDIF);
            case NodeType::WRAP_V: return subs[0]->MakeScript(ctx, true) + (subs[0]->GetType() << "x"_mst ? (CScript() << OP_VERIFY) : CScript());
            case NodeType::WRAP_J: return (CScript() << OP_SIZE << OP_0NOTEQUAL << OP_IF) + subs[0]->MakeScript(ctx) + (CScript() << OP_ENDIF);
            case NodeType::WRAP_N: return subs[0]->MakeScript(ctx) + CScript() << OP_0NOTEQUAL;
            case NodeType::JUST_1: return CScript() << OP_1;
            case NodeType::JUST_0: return CScript() << OP_0;
            case NodeType::AND_V: return subs[0]->MakeScript(ctx) + subs[1]->MakeScript(ctx, verify);
            case NodeType::AND_B: return subs[0]->MakeScript(ctx) + subs[1]->MakeScript(ctx) + (CScript() << OP_BOOLAND);
            case NodeType::OR_B: return subs[0]->MakeScript(ctx) + subs[1]->MakeScript(ctx) + (CScript() << OP_BOOLOR);
            case NodeType::OR_D: return subs[0]->MakeScript(ctx) + (CScript() << OP_IFDUP << OP_NOTIF) + subs[1]->MakeScript(ctx) + (CScript() << OP_ENDIF);
            case NodeType::OR_C: return subs[0]->MakeScript(ctx) + (CScript() << OP_NOTIF) + subs[1]->MakeScript(ctx) + (CScript() << OP_ENDIF);
            case NodeType::OR_I: return (CScript() << OP_IF) + subs[0]->MakeScript(ctx) + (CScript() << OP_ELSE) + subs[1]->MakeScript(ctx) + (CScript() << OP_ENDIF);
            case NodeType::ANDOR: return subs[0]->MakeScript(ctx) + (CScript() << OP_NOTIF) + subs[2]->MakeScript(ctx) + (CScript() << OP_ELSE) + subs[1]->MakeScript(ctx) + (CScript() << OP_ENDIF);
            case NodeType::THRESH_M: {
                CScript script = CScript() << k;
                for (const auto& key : keys) {
                    script << ctx.ToPKBytes(key);
                }
                return script << keys.size() << (verify ? OP_CHECKMULTISIGVERIFY : OP_CHECKMULTISIG);
            }
            case NodeType::THRESH: {
                CScript script = subs[0]->MakeScript(ctx);
                for (size_t i = 1; i < subs.size(); ++i) {
                    script = (script + subs[i]->MakeScript(ctx)) << OP_ADD;
                }
                return script << k << (verify ? OP_EQUALVERIFY : OP_EQUAL);
            }
        }
        assert(false);
        return {};
    }

    //! Internal code for ToString.
    template<typename Ctx>
    std::string MakeString(const Ctx& ctx, bool& success, bool wrapped = false) const {
        switch (nodetype) {
            case NodeType::WRAP_A: return "a" + subs[0]->MakeString(ctx, success, true);
            case NodeType::WRAP_S: return "s" + subs[0]->MakeString(ctx, success, true);
            case NodeType::WRAP_C: return "c" + subs[0]->MakeString(ctx, success, true);
            case NodeType::WRAP_D: return "d" + subs[0]->MakeString(ctx, success, true);
            case NodeType::WRAP_V: return "v" + subs[0]->MakeString(ctx, success, true);
            case NodeType::WRAP_J: return "j" + subs[0]->MakeString(ctx, success, true);
            case NodeType::WRAP_N: return "n" + subs[0]->MakeString(ctx, success, true);
            case NodeType::AND_V:
                // t:X is syntactic sugar for and_v(X,1).
                if (subs[1]->nodetype == NodeType::JUST_1) return "t" + subs[0]->MakeString(ctx, success, true);
                break;
            case NodeType::OR_I:
                if (subs[0]->nodetype == NodeType::JUST_0) return "l" + subs[1]->MakeString(ctx, success, true);
                if (subs[1]->nodetype == NodeType::JUST_0) return "u" + subs[0]->MakeString(ctx, success, true);
                break;
            default:
                break;
        }

        std::string ret = wrapped ? ":" : "";

        switch (nodetype) {
            case NodeType::PK: {
                std::string key_str;
                success = ctx.ToString(keys[0], key_str);
                return std::move(ret) + "pk(" + std::move(key_str) + ")";
            }
            case NodeType::PK_H: {
                std::string key_str;
                success = ctx.ToString(keys[0], key_str);
                return std::move(ret) + "pk_h(" + std::move(key_str) + ")";
            }
            case NodeType::AFTER: return std::move(ret) + "after(" + std::to_string(k) + ")";
            case NodeType::OLDER: return std::move(ret) + "older(" + std::to_string(k) + ")";
            case NodeType::HASH256: return std::move(ret) + "hash256(" + HexStr(data.begin(), data.end()) + ")";
            case NodeType::HASH160: return std::move(ret) + "hash160(" + HexStr(data.begin(), data.end()) + ")";
            case NodeType::SHA256: return std::move(ret) + "sha256(" + HexStr(data.begin(), data.end()) + ")";
            case NodeType::RIPEMD160: return std::move(ret) + "ripemd160(" + HexStr(data.begin(), data.end()) + ")";
            case NodeType::JUST_1: return std::move(ret) + "1";
            case NodeType::JUST_0: return std::move(ret) + "0";
            case NodeType::AND_V: return std::move(ret) + "and_v(" + subs[0]->MakeString(ctx, success) + "," + subs[1]->MakeString(ctx, success) + ")";
            case NodeType::AND_B: return std::move(ret) + "and_b(" + subs[0]->MakeString(ctx, success) + "," + subs[1]->MakeString(ctx, success) + ")";
            case NodeType::OR_B: return std::move(ret) + "or_b(" + subs[0]->MakeString(ctx, success) + "," + subs[1]->MakeString(ctx, success) + ")";
            case NodeType::OR_D: return std::move(ret) + "or_d(" + subs[0]->MakeString(ctx, success) + "," + subs[1]->MakeString(ctx, success) + ")";
            case NodeType::OR_C: return std::move(ret) + "or_c(" + subs[0]->MakeString(ctx, success) + "," + subs[1]->MakeString(ctx, success) + ")";
            case NodeType::OR_I: return std::move(ret) + "or_i(" + subs[0]->MakeString(ctx, success) + "," + subs[1]->MakeString(ctx, success) + ")";
            case NodeType::ANDOR:
                // and_n(X,Y) is syntactic sugar for andor(X,Y,0).
                if (subs[2]->nodetype == NodeType::JUST_0) return std::move(ret) + "and_n(" + subs[0]->MakeString(ctx, success) + "," + subs[1]->MakeString(ctx, success) + ")";
                return std::move(ret) + "andor(" + subs[0]->MakeString(ctx, success) + "," + subs[1]->MakeString(ctx, success) + "," + subs[2]->MakeString(ctx, success) + ")";
            case NodeType::THRESH_M: {
                auto str = std::move(ret) + "thresh_m(" + std::to_string(k);
                for (const auto& key : keys) {
                    std::string key_str;
                    success &= ctx.ToString(key, key_str);
                    str += "," + std::move(key_str);
                }
                return std::move(str) + ")";
            }
            case NodeType::THRESH: {
                auto str = std::move(ret) + "thresh(" + std::to_string(k);
                for (const auto& sub : subs) {
                    str += "," + sub->MakeString(ctx, success);
                }
                return std::move(str) + ")";
            }
            default: assert(false); // Wrappers should have been handled above
        }
        return "";
    }

public:
    //! Return the size of the script for this expression (faster than ToString().size()).
    size_t ScriptSize() const { return scriptlen; }

    //! Return the expression type.
    Type GetType() const { return typ; }

    //! Check whether this node is valid at all.
    bool IsValid() const { return !(GetType() == ""_mst); }

    //! Check whether this node is valid as a script on its own.
    bool IsValidTopLevel() const { return GetType() << "B"_mst; }

    //! Check whether this script can always be satisfied in a non-malleable way.
    bool IsNonMalleable() const { return GetType() << "m"_mst; }

    //! Check whether this script always needs a signature.
    bool NeedsSignature() const { return GetType() << "s"_mst; }

    //! Do all sanity checks.
    bool IsSafeTopLevel() const { return GetType() << "Bms"_mst; }

    //! Construct the script for this miniscript (including subexpressions).
    template<typename Ctx>
    CScript ToScript(const Ctx& ctx) const { return MakeScript(ctx); }

    //! Convert this miniscript to its textual descriptor notation.
    template<typename Ctx>
    bool ToString(const Ctx& ctx, std::string& out) const {
        bool ret = true;
        out = MakeString(ctx, ret);
        if (!ret) out = "";
        return ret;
    }

    //! Equality testing.
    bool operator==(const Node<Key>& arg) const
    {
        if (nodetype != arg.nodetype) return false;
        if (k != arg.k) return false;
        if (data != arg.data) return false;
        if (keys != arg.keys) return false;
        if (subs.size() != arg.subs.size()) return false;
        for (size_t i = 0; i < subs.size(); ++i) {
            if (!(*subs[i] == *arg.subs[i])) return false;
        }
        assert(scriptlen == arg.scriptlen);
        assert(typ == arg.typ);
        return true;
    }

    // Constructors with various argument combinations.
    Node(NodeType nt, std::vector<NodeRef<Key>> sub, std::vector<unsigned char> arg, uint32_t val = 0) : nodetype(nt), k(val), data(std::move(arg)), subs(std::move(sub)), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, std::vector<unsigned char> arg, uint32_t val = 0) : nodetype(nt), k(val), data(std::move(arg)), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, std::vector<NodeRef<Key>> sub, std::vector<Key> key, uint32_t val = 0) : nodetype(nt), k(val), keys(std::move(key)), subs(std::move(sub)), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, std::vector<Key> key, uint32_t val = 0) : nodetype(nt), k(val), keys(std::move(key)), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, std::vector<NodeRef<Key>> sub, uint32_t val = 0) : nodetype(nt), k(val), subs(std::move(sub)), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, uint32_t val = 0) : nodetype(nt), k(val), typ(CalcType()), scriptlen(CalcScriptLen()) {}
};

namespace internal {

//! Parse a miniscript from its textual descriptor form.
template<typename Key, typename Ctx>
inline NodeRef<Key> Parse(Span<const char>& in, const Ctx& ctx) {
    auto expr = Expr(in);
    // Parse wrappers
    for (int i = 0; i < expr.size(); ++i) {
        if (expr[i] == ':') {
            auto in2 = expr.subspan(i + 1);
            auto sub = Parse<Key>(in2, ctx);
            if (!sub || in2.size()) return {};
            for (int j = i; j-- > 0; ) {
                if (expr[j] == 'a') {
                    sub = MakeNodeRef<Key>(NodeType::WRAP_A, Vector(std::move(sub)));
                } else if (expr[j] == 's') {
                    sub = MakeNodeRef<Key>(NodeType::WRAP_S, Vector(std::move(sub)));
                } else if (expr[j] == 'c') {
                    sub = MakeNodeRef<Key>(NodeType::WRAP_C, Vector(std::move(sub)));
                } else if (expr[j] == 'd') {
                    sub = MakeNodeRef<Key>(NodeType::WRAP_D, Vector(std::move(sub)));
                } else if (expr[j] == 'j') {
                    sub = MakeNodeRef<Key>(NodeType::WRAP_J, Vector(std::move(sub)));
                } else if (expr[j] == 'n') {
                    sub = MakeNodeRef<Key>(NodeType::WRAP_N, Vector(std::move(sub)));
                } else if (expr[j] == 'v') {
                    sub = MakeNodeRef<Key>(NodeType::WRAP_V, Vector(std::move(sub)));
                } else if (expr[j] == 't') {
                    sub = MakeNodeRef<Key>(NodeType::AND_V, Vector(std::move(sub), MakeNodeRef<Key>(NodeType::JUST_1)));
                } else if (expr[j] == 'u') {
                    sub = MakeNodeRef<Key>(NodeType::OR_I, Vector(std::move(sub), MakeNodeRef<Key>(NodeType::JUST_0)));
                } else if (expr[j] == 'l') {
                    sub = MakeNodeRef<Key>(NodeType::OR_I, Vector(MakeNodeRef<Key>(NodeType::JUST_0), std::move(sub)));
                } else {
                    return {};
                }
            }
            return sub;
        }
        if (expr[i] < 'a' || expr[i] > 'z') break;
    }
    // Parse the other node types
    NodeType nodetype;
    if (expr == Span<const char>("0", 1)) {
        return MakeNodeRef<Key>(NodeType::JUST_0);
    } else if (expr == Span<const char>("1", 1)) {
        return MakeNodeRef<Key>(NodeType::JUST_1);
    } else if (Func("pk", expr)) {
        Key key;
        if (ctx.FromString(expr.begin(), expr.end(), key)) {
            return MakeNodeRef<Key>(NodeType::PK, Vector(std::move(key)));
        }
        return {};
    } else if (Func("pk_h", expr)) {
        Key key;
        if (ctx.FromString(expr.begin(), expr.end(), key)) {
            return MakeNodeRef<Key>(NodeType::PK_H, Vector(std::move(key)));
        }
        return {};
    } else if (expr == MakeSpan("0")) {
        return MakeNodeRef<Key>(NodeType::JUST_0);
    } else if (expr == MakeSpan("1")) {
        return MakeNodeRef<Key>(NodeType::JUST_1);
    } else if (Func("sha256", expr)) {
        auto hash = ParseHex(std::string(expr.begin(), expr.end()));
        if (hash.size() != 32) return {};
        return MakeNodeRef<Key>(NodeType::SHA256, std::move(hash));
    } else if (Func("ripemd160", expr)) {
        auto hash = ParseHex(std::string(expr.begin(), expr.end()));
        if (hash.size() != 20) return {};
        return MakeNodeRef<Key>(NodeType::RIPEMD160, std::move(hash));
    } else if (Func("hash256", expr)) {
        auto hash = ParseHex(std::string(expr.begin(), expr.end()));
        if (hash.size() != 32) return {};
        return MakeNodeRef<Key>(NodeType::HASH256, std::move(hash));
    } else if (Func("hash160", expr)) {
        auto hash = ParseHex(std::string(expr.begin(), expr.end()));
        if (hash.size() != 20) return {};
        return MakeNodeRef<Key>(NodeType::HASH160, std::move(hash));
    } else if (Func("after", expr)) {
        int64_t num;
        if (!ParseInt64(std::string(expr.begin(), expr.end()), &num)) return {};
        if (num < 1 || num >= 0x80000000L) return {};
        return MakeNodeRef<Key>(NodeType::AFTER, num);
    } else if (Func("older", expr)) {
        int64_t num;
        if (!ParseInt64(std::string(expr.begin(), expr.end()), &num)) return {};
        if (num < 1 || num >= 0x80000000L) return {};
        return MakeNodeRef<Key>(NodeType::OLDER, num);
    } else if (Func("and_n", expr)) {
        auto left = Parse<Key>(expr, ctx);
        if (!left || !Const(",", expr)) return {};
        auto right = Parse<Key>(expr, ctx);
        if (!right || expr.size()) return {};
        return MakeNodeRef<Key>(NodeType::ANDOR, Vector(std::move(left), std::move(right), MakeNodeRef<Key>(NodeType::JUST_0)));
    } else if (Func("andor", expr)) {
        auto left = Parse<Key>(expr, ctx);
        if (!left || !Const(",", expr)) return {};
        auto mid = Parse<Key>(expr, ctx);
        if (!mid || !Const(",", expr)) return {};
        auto right = Parse<Key>(expr, ctx);
        if (!right || expr.size()) return {};
        return MakeNodeRef<Key>(NodeType::ANDOR, Vector(std::move(left), std::move(mid), std::move(right)));
    } else if (Func("thresh_m", expr)) {
        auto arg = Expr(expr);
        int64_t count;
        if (!ParseInt64(std::string(arg.begin(), arg.end()), &count)) return {};
        std::vector<Key> keys;
        while (expr.size()) {
            if (!Const(",", expr)) return {};
            auto keyarg = Expr(expr);
            Key key;
            if (!ctx.FromString(keyarg.begin(), keyarg.end(), key)) return {};
            keys.push_back(std::move(key));
        }
        if (keys.size() < 1 || keys.size() > 20) return {};
        if (count < 1 || count > (int64_t)keys.size()) return {};
        return MakeNodeRef<Key>(NodeType::THRESH_M, std::move(keys), count);
    } else if (Func("thresh", expr)) {
        auto arg = Expr(expr);
        int64_t count;
        if (!ParseInt64(std::string(arg.begin(), arg.end()), &count)) return {};
        std::vector<NodeRef<Key>> subs;
        while (expr.size()) {
            if (!Const(",", expr)) return {};
            auto sub = Parse<Key>(expr, ctx);
            if (!sub) return {};
            subs.push_back(std::move(sub));
        }
        if (count <= 1 || count >= (int64_t)subs.size()) return {};
        return MakeNodeRef<Key>(NodeType::THRESH, std::move(subs), count);
    } else if (Func("and_v", expr)) {
        nodetype = NodeType::AND_V;
    } else if (Func("and_b", expr)) {
        nodetype = NodeType::AND_B;
    } else if (Func("or_c", expr)) {
        nodetype = NodeType::OR_C;
    } else if (Func("or_b", expr)) {
        nodetype = NodeType::OR_B;
    } else if (Func("or_d", expr)) {
        nodetype = NodeType::OR_D;
    } else if (Func("or_i", expr)) {
        nodetype = NodeType::OR_I;
    } else {
        return {};
    }
    auto left = Parse<Key>(expr, ctx);
    if (!left || !Const(",", expr)) return {};
    auto right = Parse<Key>(expr, ctx);
    if (!right || expr.size()) return {};
    return MakeNodeRef<Key>(nodetype, Vector(std::move(left), std::move(right)));
}

/** Decode a script into opcode/push pairs.
 *
 * Construct a vector with one element per opcode in the script, in reverse order.
 * Each element is a pair consisting of the opcode, as well as the data pushed by
 * the opcode (including OP_n), if any. OP_CHECKSIGVERIFY, OP_CHECKMULTISIGVERIFY,
 * and OP_EQUALVERIFY are decomposed into OP_CHECKSIG, OP_CHECKMULTISIG, OP_EQUAL
 * respectively, plus OP_VERIFY.
 */
bool DecomposeScript(const CScript& script, std::vector<std::pair<opcodetype, std::vector<unsigned char>>>& out);

/** Determine whether the passed pair (created by DecomposeScript) is pushing a number. */
bool ParseScriptNumber(const std::pair<opcodetype, std::vector<unsigned char>>& in, int64_t& k);

template<typename Key, typename Ctx, typename I> inline NodeRef<Key> DecodeSingle(I& in, I last, const Ctx& ctx);
template<typename Key, typename Ctx, typename I> inline NodeRef<Key> DecodeMulti(I& in, I last, const Ctx& ctx);
template<typename Key, typename Ctx, typename I> inline NodeRef<Key> DecodeWrapped(I& in, I last, const Ctx& ctx);

//! Decode a list of script elements into a miniscript (except and_v, s:, and a:).
template<typename Key, typename Ctx, typename I>
inline NodeRef<Key> DecodeSingle(I& in, I last, const Ctx& ctx) {
    std::vector<NodeRef<Key>> subs;
    std::vector<Key> keys;
    int64_t k;

    if (last > in && in[0].first == OP_1) {
        ++in;
        return MakeNodeRef<Key>(NodeType::JUST_1);
    }
    if (last > in && in[0].first == OP_0) {
        ++in;
        return MakeNodeRef<Key>(NodeType::JUST_0);
    }
    if (last > in && in[0].second.size() == 33) {
        Key key;
        if (!ctx.FromPKBytes(in[0].second.begin(), in[0].second.end(), key)) return {};
        ++in;
        return MakeNodeRef<Key>(NodeType::PK, Vector(std::move(key)));
    }
    if (last - in >= 5 && in[0].first == OP_VERIFY && in[1].first == OP_EQUAL && in[3].first == OP_HASH160 && in[4].first == OP_DUP && in[2].second.size() == 20) {
        Key key;
        if (!ctx.FromPKHBytes(in[2].second.begin(), in[2].second.end(), key)) return {};
        in += 5;
        return MakeNodeRef<Key>(NodeType::PK_H, Vector(std::move(key)));
    }
    if (last - in >= 2 && in[0].first == OP_CHECKSEQUENCEVERIFY && ParseScriptNumber(in[1], k)) {
        in += 2;
        if (k < 1 || k > 0x7FFFFFFFL) return {};
        return MakeNodeRef<Key>(NodeType::OLDER, k);
    }
    if (last - in >= 2 && in[0].first == OP_CHECKLOCKTIMEVERIFY && ParseScriptNumber(in[1], k)) {
        in += 2;
        if (k < 1 || k > 0x7FFFFFFFL) return {};
        return MakeNodeRef<Key>(NodeType::AFTER, k);
    }
    if (last - in >= 7 && in[0].first == OP_EQUAL && in[1].second.size() == 32 && in[2].first == OP_SHA256 && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && ParseScriptNumber(in[5], k) && k == 32 && in[6].first == OP_SIZE) {
        in += 7;
        return MakeNodeRef<Key>(NodeType::SHA256, in[-6].second);
    }
    if (last - in >= 7 && in[0].first == OP_EQUAL && in[1].second.size() == 20 && in[2].first == OP_RIPEMD160 && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && ParseScriptNumber(in[5], k) && k == 32 && in[6].first == OP_SIZE) {
        in += 7;
        return MakeNodeRef<Key>(NodeType::RIPEMD160, in[-6].second);
    }
    if (last - in >= 7 && in[0].first == OP_EQUAL && in[1].second.size() == 32 && in[2].first == OP_HASH256 && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && ParseScriptNumber(in[5], k) && k == 32 && in[6].first == OP_SIZE) {
        in += 7;
        return MakeNodeRef<Key>(NodeType::HASH256, in[-6].second);
    }
    if (last - in >= 7 && in[0].first == OP_EQUAL && in[1].second.size() == 20 && in[2].first == OP_HASH160 && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && ParseScriptNumber(in[5], k) && k == 32 && in[6].first == OP_SIZE) {
        in += 7;
        return MakeNodeRef<Key>(NodeType::HASH160, in[-6].second);
    }
    if (last - in >= 2 && in[0].first == OP_CHECKSIG) {
        ++in;
        auto sub = DecodeSingle<Key>(in, last, ctx);
        if (!sub) return {};
        return MakeNodeRef<Key>(NodeType::WRAP_C, Vector(std::move(sub)));
    }
    if (last - in >= 3 && in[0].first == OP_BOOLAND) {
        ++in;
        auto sub1 = DecodeWrapped<Key>(in, last, ctx);
        if (!sub1) return {};
        auto sub2 = DecodeSingle<Key>(in, last, ctx);
        if (!sub2) return {};
        return MakeNodeRef<Key>(NodeType::AND_B, Vector(std::move(sub2), std::move(sub1)));
    }
    if (last - in >= 3 && in[0].first == OP_BOOLOR) {
        ++in;
        auto sub1 = DecodeWrapped<Key>(in, last, ctx);
        if (!sub1) return {};
        auto sub2 = DecodeSingle<Key>(in, last, ctx);
        if (!sub2) return {};
        return MakeNodeRef<Key>(NodeType::OR_B, Vector(std::move(sub2), std::move(sub1)));
    }
    if (last - in >= 2 && in[0].first == OP_VERIFY) {
        ++in;
        auto sub = DecodeSingle<Key>(in, last, ctx);
        if (!sub) return {};
        return MakeNodeRef<Key>(NodeType::WRAP_V, Vector(std::move(sub)));
    }
    if (last - in >= 2 && in[0].first == OP_0NOTEQUAL) {
        ++in;
        auto sub = DecodeSingle<Key>(in, last, ctx);
        if (!sub) return {};
        return MakeNodeRef<Key>(NodeType::WRAP_N, Vector(std::move(sub)));
    }
    if (last > in && in[0].first == OP_ENDIF) {
        ++in;
        if (last - in == 0) return {};
        NodeRef<Key> sub1;
        sub1 = DecodeMulti<Key>(in, last, ctx);
        if (!sub1) return {};
        bool have_else = false;
        NodeRef<Key> sub2;
        if (last - in == 0) return {};
        if (in[0].first == OP_ELSE) {
            ++in;
            have_else = true;
            sub2 = DecodeMulti<Key>(in, last, ctx);
            if (!sub2) return {};
        }
        if (last - in == 0 || (in[0].first != OP_IF && in[0].first != OP_NOTIF)) return {};
        bool negated = (in[0].first == OP_NOTIF);
        ++in;

        if (!have_else && !negated) {
            if (last > in && in[0].first == OP_DUP) {
                ++in;
                return MakeNodeRef<Key>(NodeType::WRAP_D, Vector(std::move(sub1)));
            }
            if (last - in >= 2 && in[0].first == OP_0NOTEQUAL && in[1].first == OP_SIZE) {
                in += 2;
                return MakeNodeRef<Key>(NodeType::WRAP_J, Vector(std::move(sub1)));
            }
            return {};
        }
        if (have_else && negated) {
            auto sub3 = DecodeSingle<Key>(in, last, ctx);
            if (!sub3) return {};
            return MakeNodeRef<Key>(NodeType::ANDOR, Vector(std::move(sub3), std::move(sub1), std::move(sub2)));
        }
        if (!have_else && negated) {
            if (last - in >= 2 && in[0].first == OP_IFDUP) {
                ++in;
                auto sub3 = DecodeSingle<Key>(in, last, ctx);
                if (!sub3) return {};
                return MakeNodeRef<Key>(NodeType::OR_D, Vector(std::move(sub3), std::move(sub1)));
            }
            if (last > in) {
                auto sub3 = DecodeSingle<Key>(in, last, ctx);
                if (!sub3) return {};
                return MakeNodeRef<Key>(NodeType::OR_C, Vector(std::move(sub3), std::move(sub1)));
            }
            return {};
        }
        if (have_else && !negated) {
            return MakeNodeRef<Key>(NodeType::OR_I, Vector(std::move(sub2), std::move(sub1)));
        }
        return {};
    }
    keys.clear();
    if (last - in >= 3 && in[0].first == OP_CHECKMULTISIG) {
        int64_t n;
        if (!ParseScriptNumber(in[1], n)) return {};
        if (last - in < 3 + n) return {};
        if (n < 1 || n > 20) return {};
        for (int i = 0; i < n; ++i) {
            Key key;
            if (in[2 + i].second.size() != 33) return {};
            if (!ctx.FromPKBytes(in[2 + i].second.begin(), in[2 + i].second.end(), key)) return {};
            keys.push_back(std::move(key));
        }
        if (!ParseScriptNumber(in[2 + n], k)) return {};
        if (k < 1 || k > n) return {};
        in += 3 + n;
        std::reverse(keys.begin(), keys.end());
        return MakeNodeRef<Key>(NodeType::THRESH_M, std::move(keys), k);
    }
    subs.clear();
    if (last - in >= 3 && in[0].first == OP_EQUAL && ParseScriptNumber(in[1], k)) {
        in += 2;
        while (last - in >= 2 && in[0].first == OP_ADD) {
            ++in;
            auto sub = DecodeWrapped<Key>(in, last, ctx);
            if (!sub) return {};
            subs.push_back(std::move(sub));
        }
        auto sub = DecodeSingle<Key>(in, last, ctx);
        if (!sub) return {};
        subs.push_back(std::move(sub));
        std::reverse(subs.begin(), subs.end());
        return MakeNodeRef<Key>(NodeType::THRESH, std::move(subs), k);
    }

    return {};
}

//! Decode a list of script elements into a miniscript (except a: and s:)
template<typename Key, typename Ctx, typename I>
inline NodeRef<Key> DecodeMulti(I& in, I last, const Ctx& ctx) {
    if (in == last) return {};
    auto sub = DecodeSingle<Key>(in, last, ctx);
    if (!sub) return {};
    while (in != last && in[0].first != OP_ELSE && in[0].first != OP_IF && in[0].first != OP_NOTIF && in[0].first != OP_TOALTSTACK && in[0].first != OP_SWAP) {
        auto sub2 = DecodeSingle<Key>(in, last, ctx);
        if (!sub2) return {};
        sub = MakeNodeRef<Key>(NodeType::AND_V, Vector(std::move(sub2), std::move(sub)));
    }
    return sub;
}

//! Decode a list of script elements into a miniscript (only a: and s:)
template<typename Key, typename Ctx, typename I>
inline NodeRef<Key> DecodeWrapped(I& in, I last, const Ctx& ctx) {
    if (last - in >= 3 && in[0].first == OP_FROMALTSTACK) {
        ++in;
        auto sub = DecodeMulti<Key>(in, last, ctx);
        if (!sub) return {};
        if (in == last || in[0].first != OP_TOALTSTACK) return {};
        ++in;
        return MakeNodeRef<Key>(NodeType::WRAP_A, Vector(std::move(sub)));
    }
    auto sub = DecodeMulti<Key>(in, last, ctx);
    if (!sub) return {};
    if (in == last || in[0].first != OP_SWAP) return {};
    ++in;
    return MakeNodeRef<Key>(NodeType::WRAP_S, Vector(std::move(sub)));
}

} // namespace internal

template<typename Ctx>
inline NodeRef<typename Ctx::Key> FromString(const std::string& str, const Ctx& ctx) {
    using namespace internal;
    Span<const char> span = MakeSpan(str);
    auto ret = Parse<typename Ctx::Key>(span, ctx);
    if (!ret || span.size()) return {};
    return ret;
}

template<typename Ctx>
inline NodeRef<typename Ctx::Key> FromScript(const CScript& script, const Ctx& ctx) {
    using namespace internal;
    std::vector<std::pair<opcodetype, std::vector<unsigned char>>> decomposed;
    if (!DecomposeScript(script, decomposed)) return {};
    auto it = decomposed.begin();
    auto ret = DecodeMulti<typename Ctx::Key>(it, decomposed.end(), ctx);
    if (!ret) return {};
    if (it != decomposed.end()) return {};
    return ret;
}

} // namespace miniscript

#endif // BITCOIN_SCRIPT_MINISCRIPT_H
