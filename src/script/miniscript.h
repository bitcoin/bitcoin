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

#include <policy/policy.h>
#include <script/script.h>
#include <span.h>
#include <util/spanparsing.h>
#include <util/strencodings.h>
#include <util/vector.h>
#include <primitives/transaction.h>

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
 *   - For example vc:pk_k(key) = <key> OP_CHECKSIGVERIFY
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
 *   - For example sc:pk_k(key) = OP_SWAP <key> OP_CHECKSIG
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
 * One type property is an implementation detail:
 * - "x" Expensive verify:
 *   - Expressions with this property have a script whose last opcode is not EQUAL, CHECKSIG, or CHECKMULTISIG.
 *   - Not having this property means that it can be converted to a V at no cost (by switching to the
 *     -VERIFY version of the last opcode).
 *
 * Five more type properties for representing timelock information. Spend paths
 * in miniscripts containing conflicting timelocks and heightlocks cannot be spent together.
 * This helps users detect if miniscript does not match the semantic behaviour the
 * user expects.
 * - "g" Whether the branch contains a relative time timelock
 * - "h" Whether the branch contains a relative height timelock
 * - "i" Whether the branch contains a absolute time timelock
 * - "j" Whether the branch contains a absolute time heightlock
 * - "k"
 *   - Whether all satisfactions of this expression don't contain a mix of heightlock and timelock
 *     of the same type.
 *   - If the miniscript does not have the "k" property, the miniscript template will not match
 *     the user expectation of the corresponding spending policy.
 * For each of these properties the subset rule holds: an expression with properties X, Y, and Z, is also
 * valid in places where an X, a Y, a Z, an XY, ... is expected.
*/
class Type {
    //! Internal bitmap of properties (see ""_mst operator for details).
    uint32_t m_flags;

    //! Internal constructor used by the ""_mst operator.
    explicit constexpr Type(uint32_t flags) : m_flags(flags) {}

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
        *c == 'g' ? 1 << 14 : // older: contains relative time timelock   (csv_time)
        *c == 'h' ? 1 << 15 : // older: contains relative height timelock (csv_height)
        *c == 'i' ? 1 << 16 : // after: contains time timelock   (cltv_time)
        *c == 'j' ? 1 << 17 : // after: contains height timelock   (cltv_height)
        *c == 'k' ? 1 << 18 : // does not contain a combination of height and time locks
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
    JUST_0,    //!< OP_0
    JUST_1,    //!< OP_1
    PK_K,      //!< [key]
    PK_H,      //!< OP_DUP OP_HASH160 [keyhash] OP_EQUALVERIFY
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
    OR_I,      //!< OP_IF [X] OP_ELSE [Y] OP_ENDIF
    ANDOR,     //!< [X] OP_NOTIF [Z] OP_ELSE [Y] OP_ENDIF
    THRESH,    //!< [X1] ([Xn] OP_ADD)* [k] OP_EQUAL
    MULTI,     //!< [k] [key_n]* [n] OP_CHECKMULTISIG
    // AND_N(X,Y) is represented as ANDOR(X,Y,0)
    // WRAP_T(X) is represented as AND_V(X,1)
    // WRAP_L(X) is represented as OR_I(0,X)
    // WRAP_U(X) is represented as OR_I(X,0)
};

enum class Availability {
    NO,
    YES,
    MAYBE,
};

namespace internal {

//! Helper function for Node::CalcType.
Type ComputeType(NodeType nodetype, Type x, Type y, Type z, const std::vector<Type>& sub_types, uint32_t k, size_t data_size, size_t n_subs, size_t n_keys);

//! Helper function for Node::CalcScriptLen.
size_t ComputeScriptLen(NodeType nodetype, Type sub0typ, size_t subsize, uint32_t k, size_t n_subs, size_t n_keys);

//! A helper sanitizer/checker for the output of CalcType.
Type SanitizeType(Type x);

//! An object representing a sequence of witness stack elements.
struct InputStack {
    /** Whether this stack is valid for its intended purpose (satisfaction or dissatisfaction of a Node).
     *  The MAYBE value is used for size estimation, when keys/preimages may actually be unavailable,
     *  but may be available at signing time. This makes the InputStack structure and signing logic,
     *  filled with dummy signatures/preimages usable for witness size estimation.
     */
    Availability available = Availability::YES;
    //! Whether this stack contains a digital signature.
    bool has_sig = false;
    //! Whether this stack is malleable (can be turned into an equally valid other stack by a third party).
    bool malleable = false;
    //! Whether this stack is non-canonical (using a construction known to be unnecessary for satisfaction).
    bool non_canon = false;
    //! Serialized witness size.
    size_t size = 0;
    //! Data elements.
    std::vector<std::vector<unsigned char>> stack;
    //! Construct an empty stack (valid).
    InputStack() {}
    //! Construct a valid single-element stack (with an element up to 75 bytes).
    InputStack(std::vector<unsigned char> in) : size(in.size() + 1), stack(Vector(std::move(in))) {}
    //! Change availability
    InputStack& Available(Availability avail);
    //! Mark this input stack as having a signature.
    InputStack& WithSig();
    //! Mark this input stack as non-canonical (known to not be necessary in non-malleable satisfactions).
    InputStack& NonCanon();
    //! Mark this input stack as malleable.
    InputStack& Malleable(bool x = true);
    //! Concatenate two input stacks.
    friend InputStack operator+(InputStack a, InputStack b);
    //! Choose between two potential input stacks.
    friend InputStack Choose(InputStack a, InputStack b, bool nonmalleable);
};

//! A pair of a satisfaction and a dissatisfaction InputStack.
struct InputResult {
    InputStack nsat, sat;
    InputResult(InputStack in_nsat, InputStack in_sat) : nsat(std::move(in_nsat)), sat(std::move(in_sat)) {}
};

//! Class whose objects represent the maximum of a list of integers.
template<typename I>
struct MaxInt {
    const bool valid;
    const I value;

    MaxInt() : valid(false), value(0) {}
    MaxInt(I val) : valid(true), value(val) {}

    friend MaxInt<I> operator+(const MaxInt<I>& a, const MaxInt<I>& b) {
        if (!a.valid || !b.valid) return {};
        return a.value + b.value;
    }

    friend MaxInt<I> Choose(const MaxInt<I>& a, const MaxInt<I>& b) {
        if (!a.valid) return b;
        if (!b.valid) return a;
        return std::max(a.value, b.value);
    }
};

struct Ops {
    //! Non-push opcodes.
    uint32_t stat;
    //! Number of keys in possibly executed OP_CHECKMULTISIG(VERIFY)s to satisfy.
    MaxInt<uint32_t> sat;
    //! Number of keys in possibly executed OP_CHECKMULTISIG(VERIFY)s to dissatisfy.
    MaxInt<uint32_t> dsat;

    Ops(uint32_t in_stat, MaxInt<uint32_t> in_sat, MaxInt<uint32_t> in_dsat) : stat(in_stat), sat(in_sat), dsat(in_dsat) {};
};

struct StackSize {
    //! Maximum stack size to satisfy;
    MaxInt<uint32_t> sat;
    //! Maximum stack size to dissatisfy;
    MaxInt<uint32_t> dsat;

    StackSize(MaxInt<uint32_t> in_sat, MaxInt<uint32_t> in_dsat) : sat(in_sat), dsat(in_dsat) {};
};

} // namespace internal

//! A node in a miniscript expression.
template<typename Key>
struct Node {
    //! What node type this node is.
    const NodeType nodetype;
    //! The k parameter (time for OLDER/AFTER, threshold for THRESH(_M))
    const uint32_t k = 0;
    //! The keys used by this expression (only for PK_K/PK_H/MULTI)
    const std::vector<Key> keys;
    //! The data bytes in this expression (only for HASH160/HASH256/SHA256/RIPEMD10).
    const std::vector<unsigned char> data;
    //! Subexpressions (for WRAP_*/AND_*/OR_*/ANDOR/THRESH)
    const std::vector<NodeRef<Key>> subs;

private:
    //! Cached ops counts.
    const internal::Ops ops;
    //! Cached stack size bounds.
    const internal::StackSize ss;
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
            case NodeType::PK_K: return CScript() << ctx.ToPKBytes(keys[0]);
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
            case NodeType::MULTI: {
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
        std::string ret = wrapped ? ":" : "";

        switch (nodetype) {
            case NodeType::WRAP_A: return "a" + subs[0]->MakeString(ctx, success, true);
            case NodeType::WRAP_S: return "s" + subs[0]->MakeString(ctx, success, true);
            case NodeType::WRAP_C:
                if (subs[0]->nodetype == NodeType::PK_K) {
                    // pk(K) is syntactic sugar for c:pk_k(K)
                    std::string key_str;
                    success = ctx.ToString(subs[0]->keys[0], key_str);
                    return std::move(ret) + "pk(" + std::move(key_str) + ")";
                }
                if (subs[0]->nodetype == NodeType::PK_H) {
                    // pkh(K) is syntactic sugar for c:pk_h(K)
                    std::string key_str;
                    success = ctx.ToString(subs[0]->keys[0], key_str);
                    return std::move(ret) + "pkh(" + std::move(key_str) + ")";
                }
                return "c" + subs[0]->MakeString(ctx, success, true);
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

        switch (nodetype) {
            case NodeType::PK_K: {
                std::string key_str;
                success = ctx.ToString(keys[0], key_str);
                return std::move(ret) + "pk_k(" + std::move(key_str) + ")";
            }
            case NodeType::PK_H: {
                std::string key_str;
                success = ctx.ToString(keys[0], key_str);
                return std::move(ret) + "pk_h(" + std::move(key_str) + ")";
            }
            case NodeType::AFTER: return std::move(ret) + "after(" + std::to_string(k) + ")";
            case NodeType::OLDER: return std::move(ret) + "older(" + std::to_string(k) + ")";
            case NodeType::HASH256: return std::move(ret) + "hash256(" + HexStr(data) + ")";
            case NodeType::HASH160: return std::move(ret) + "hash160(" + HexStr(data) + ")";
            case NodeType::SHA256: return std::move(ret) + "sha256(" + HexStr(data) + ")";
            case NodeType::RIPEMD160: return std::move(ret) + "ripemd160(" + HexStr(data) + ")";
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
            case NodeType::MULTI: {
                auto str = std::move(ret) + "multi(" + std::to_string(k);
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

    internal::Ops CalcOps() const {
        switch (nodetype) {
            case NodeType::PK_K: return {0, 0, 0};
            case NodeType::PK_H: return {3, 0, 0};
            case NodeType::OLDER: return {1, 0, {}};
            case NodeType::AFTER: return {1, 0, {}};
            case NodeType::SHA256: return {4, 0, {}};
            case NodeType::RIPEMD160: return {4, 0, {}};
            case NodeType::HASH256: return {4, 0, {}};
            case NodeType::HASH160: return {4, 0, {}};
            case NodeType::AND_V: return {subs[0]->ops.stat + subs[1]->ops.stat, subs[0]->ops.sat + subs[1]->ops.sat, {}};
            case NodeType::AND_B: return {1 + subs[0]->ops.stat + subs[1]->ops.stat, subs[0]->ops.sat + subs[1]->ops.sat, subs[0]->ops.dsat + subs[1]->ops.dsat};
            case NodeType::OR_B: return {1 + subs[0]->ops.stat + subs[1]->ops.stat, Choose(subs[0]->ops.sat + subs[1]->ops.dsat, subs[1]->ops.sat + subs[0]->ops.dsat), subs[0]->ops.dsat + subs[1]->ops.dsat};
            case NodeType::OR_D: return {3 + subs[0]->ops.stat + subs[1]->ops.stat, Choose(subs[0]->ops.sat, subs[1]->ops.sat + subs[0]->ops.dsat), subs[0]->ops.dsat + subs[1]->ops.dsat};
            case NodeType::OR_C: return {2 + subs[0]->ops.stat + subs[1]->ops.stat, Choose(subs[0]->ops.sat, subs[1]->ops.sat + subs[0]->ops.dsat), {}};
            case NodeType::OR_I: return {3 + subs[0]->ops.stat + subs[1]->ops.stat, Choose(subs[0]->ops.sat, subs[1]->ops.sat), Choose(subs[0]->ops.dsat, subs[1]->ops.dsat)};
            case NodeType::ANDOR: return {3 + subs[0]->ops.stat + subs[1]->ops.stat + subs[2]->ops.stat, Choose(subs[1]->ops.sat + subs[0]->ops.sat, subs[0]->ops.dsat + subs[2]->ops.sat), subs[0]->ops.dsat + subs[2]->ops.dsat};
            case NodeType::MULTI: return {1, (uint32_t)keys.size(), (uint32_t)keys.size()};
            case NodeType::WRAP_A: return {2 + subs[0]->ops.stat, subs[0]->ops.sat, subs[0]->ops.dsat};
            case NodeType::WRAP_S: return {1 + subs[0]->ops.stat, subs[0]->ops.sat, subs[0]->ops.dsat};
            case NodeType::WRAP_C: return {1 + subs[0]->ops.stat, subs[0]->ops.sat, subs[0]->ops.dsat};
            case NodeType::WRAP_D: return {3 + subs[0]->ops.stat, subs[0]->ops.sat, 0};
            case NodeType::WRAP_V: return {subs[0]->ops.stat + (subs[0]->GetType() << "x"_mst), subs[0]->ops.sat, {}};
            case NodeType::WRAP_J: return {4 + subs[0]->ops.stat, subs[0]->ops.sat, 0};
            case NodeType::WRAP_N: return {1 + subs[0]->ops.stat, subs[0]->ops.sat, subs[0]->ops.dsat};
            case NodeType::JUST_1: return {0, 0, {}};
            case NodeType::JUST_0: return {0, {}, 0};
            case NodeType::THRESH: {
                uint32_t stat = 0;
                auto sats = Vector(internal::MaxInt<uint32_t>(0));
                for (const auto& sub : subs) {
                    stat += sub->ops.stat + 1;
                    auto next_sats = Vector(sats[0] + sub->ops.dsat);
                    for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back(Choose(sats[j] + sub->ops.dsat, sats[j - 1] + sub->ops.sat));
                    next_sats.push_back(sats[sats.size() - 1] + sub->ops.sat);
                    sats = std::move(next_sats);
                }
                assert(k <= sats.size());
                return {stat, sats[k], sats[0]};
            }
        }
        assert(false);
        return {0, {}, {}};
    }

    internal::StackSize CalcStackSize() const {
        switch (nodetype) {
            case NodeType::PK_K: return {1, 1};
            case NodeType::PK_H: return {2, 2};
            case NodeType::OLDER: return {0, {}};
            case NodeType::AFTER: return {0, {}};
            case NodeType::SHA256: return {1, {}};
            case NodeType::RIPEMD160: return {1, {}};
            case NodeType::HASH256: return {1, {}};
            case NodeType::HASH160: return {1, {}};
            case NodeType::ANDOR: return {Choose(subs[0]->ss.sat + subs[1]->ss.sat, subs[0]->ss.dsat + subs[2]->ss.sat), subs[0]->ss.dsat + subs[2]->ss.dsat};
            case NodeType::AND_V: return {subs[0]->ss.sat + subs[1]->ss.sat, {}};
            case NodeType::AND_B: return {subs[0]->ss.sat + subs[1]->ss.sat, subs[0]->ss.dsat + subs[1]->ss.dsat};
            case NodeType::OR_B: return {Choose(subs[0]->ss.dsat + subs[1]->ss.sat, subs[0]->ss.sat + subs[1]->ss.dsat), subs[0]->ss.dsat + subs[1]->ss.dsat};
            case NodeType::OR_C: return {Choose(subs[0]->ss.sat, subs[0]->ss.dsat + subs[1]->ss.sat), {}};
            case NodeType::OR_D: return {Choose(subs[0]->ss.sat, subs[0]->ss.dsat + subs[1]->ss.sat), subs[0]->ss.dsat + subs[1]->ss.dsat};
            case NodeType::OR_I: return {Choose(subs[0]->ss.sat + 1, subs[1]->ss.sat + 1), Choose(subs[0]->ss.dsat + 1, subs[1]->ss.dsat + 1)};
            case NodeType::MULTI: return {(uint32_t)keys.size() + 1, (uint32_t)keys.size() + 1};
            case NodeType::WRAP_A: return subs[0]->ss;
            case NodeType::WRAP_S: return subs[0]->ss;
            case NodeType::WRAP_C: return subs[0]->ss;
            case NodeType::WRAP_D: return {1 + subs[0]->ss.sat, 1};
            case NodeType::WRAP_V: return {subs[0]->ss.sat, {}};
            case NodeType::WRAP_J: return {subs[0]->ss.sat, 1};
            case NodeType::WRAP_N: return subs[0]->ss;
            case NodeType::JUST_1: return {0, {}};
            case NodeType::JUST_0: return {{}, 0};
            case NodeType::THRESH: {
                auto sats = Vector(internal::MaxInt<uint32_t>(0));
                for (const auto& sub : subs) {
                    auto next_sats = Vector(sats[0] + sub->ss.dsat);
                    for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back(Choose(sats[j] + sub->ss.dsat, sats[j - 1] + sub->ss.sat));
                    next_sats.push_back(sats[sats.size() - 1] + sub->ss.sat);
                    sats = std::move(next_sats);
                }
                assert(k <= sats.size());
                return {sats[k], sats[0]};
            }
        }
        assert(false);
        return {{}, {}};
    }

    template<typename Ctx>
    internal::InputResult ProduceInput(const Ctx& ctx, bool nonmal) const {
        auto ret = ProduceInputHelper(ctx, nonmal);
        // Do a consistency check between the satisfaction code and the type checker
        // (the actual satisfaction code in ProduceInputHelper does not use GetType)
        if (GetType() << "z"_mst && ret.nsat.available != Availability::NO) assert(ret.nsat.stack.size() == 0);
        if (GetType() << "z"_mst && ret.sat.available != Availability::NO) assert(ret.sat.stack.size() == 0);
        if (GetType() << "o"_mst && ret.nsat.available != Availability::NO) assert(ret.nsat.stack.size() == 1);
        if (GetType() << "o"_mst && ret.sat.available != Availability::NO) assert(ret.sat.stack.size() == 1);
        if (GetType() << "n"_mst && ret.sat.available != Availability::NO) assert(ret.sat.stack.back().size() != 0);
        if (GetType() << "d"_mst) assert(ret.nsat.available != Availability::NO);
        if (GetType() << "f"_mst && ret.nsat.available != Availability::NO) assert(ret.nsat.has_sig);
        if (GetType() << "s"_mst && ret.sat.available != Availability::NO) assert(ret.sat.has_sig);
        if (nonmal) {
            if (GetType() << "d"_mst) assert(!ret.nsat.has_sig);
            if (GetType() << "d"_mst && !ret.nsat.malleable) assert(!ret.nsat.non_canon);
            if (GetType() << "e"_mst) assert(!ret.nsat.malleable);
            if (GetType() << "m"_mst && ret.sat.available != Availability::NO) assert(!ret.sat.malleable);
            if (ret.sat.available != Availability::NO && !ret.sat.malleable) assert(!ret.sat.non_canon);
        }
        return ret;
    }

    template<typename Ctx>
    internal::InputResult ProduceInputHelper(const Ctx& ctx, bool nonmal) const {
        using namespace internal;

        const auto ZERO = InputStack(std::vector<unsigned char>());
        const auto ZERO32 = InputStack(std::vector<unsigned char>(32, 0)).Malleable();
        const auto ONE = InputStack(Vector((unsigned char)1));
        const auto EMPTY = InputStack();
        const auto MALLEABLE_EMPTY = InputStack().Malleable();
        const auto INVALID = InputStack().Available(Availability::NO);

        switch (nodetype) {
            case NodeType::PK_K: {
                std::vector<unsigned char> sig;
                Availability avail = ctx.Sign(keys[0], sig);
                return InputResult(ZERO, InputStack(std::move(sig)).WithSig().Available(avail));
            }
            case NodeType::PK_H: {
                std::vector<unsigned char> key = ctx.ToPKBytes(keys[0]), sig;
                Availability avail = ctx.Sign(keys[0], sig);
                return InputResult(ZERO + InputStack(key), (InputStack(std::move(sig)).WithSig() + InputStack(key)).Available(avail));
            }
            case NodeType::MULTI: {
                std::vector<InputStack> sats = Vector(ZERO);
                for (size_t i = 0; i < keys.size(); ++i) {
                    std::vector<unsigned char> sig;
                    Availability avail = ctx.Sign(keys[i], sig);
                    auto sat = InputStack(std::move(sig)).WithSig().Available(avail);
                    std::vector<InputStack> next_sats;
                    next_sats.push_back(sats[0]);
                    for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back(Choose(sats[j], std::move(sats[j - 1]) + sat, nonmal));
                    next_sats.push_back(std::move(sats[sats.size() - 1]) + std::move(sat));
                    sats = std::move(next_sats);
                }
                InputStack nsat = ZERO;
                for (size_t i = 0; i < k; ++i) nsat = std::move(nsat) + ZERO;
                assert(k <= sats.size());
                return InputResult(std::move(nsat), std::move(sats[k]));
            }
            case NodeType::THRESH: {
                std::vector<InputStack> sats = Vector(EMPTY);
                for (size_t i = 0; i < subs.size(); ++i) {
                    auto res = subs[subs.size() - i - 1]->ProduceInput(ctx, nonmal);
                    std::vector<InputStack> next_sats;
                    next_sats.push_back(sats[0] + res.nsat);
                    for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back(Choose(sats[j] + res.nsat, std::move(sats[j - 1]) + res.sat, nonmal));
                    next_sats.push_back(std::move(sats[sats.size() - 1]) + std::move(res.sat));
                    sats = std::move(next_sats);
                }
                InputStack nsat = INVALID;
                for (size_t i = 0; i < sats.size(); ++i) {
                    if (i != k) nsat = Choose(std::move(nsat), std::move(sats[i]), nonmal);
                }
                assert(k <= sats.size());
                return InputResult(std::move(nsat), std::move(sats[k]));
            }
            case NodeType::OLDER: {
                return InputResult(INVALID, ctx.CheckOlder(k) ? EMPTY : INVALID);
            }
            case NodeType::AFTER: {
                return InputResult(INVALID, ctx.CheckAfter(k) ? EMPTY : INVALID);
            }
            case NodeType::SHA256: {
                std::vector<unsigned char> preimage;
                Availability avail = ctx.SatSHA256(data, preimage);
                return InputResult(ZERO32, InputStack(std::move(preimage)).Available(avail));
            }
            case NodeType::RIPEMD160: {
                std::vector<unsigned char> preimage;
                Availability avail = ctx.SatRIPEMD160(data, preimage);
                return InputResult(ZERO32, InputStack(std::move(preimage)).Available(avail));
            }
            case NodeType::HASH256: {
                std::vector<unsigned char> preimage;
                Availability avail = ctx.SatHASH256(data, preimage);
                return InputResult(ZERO32, InputStack(std::move(preimage)).Available(avail));
            }
            case NodeType::HASH160: {
                std::vector<unsigned char> preimage;
                Availability avail = ctx.SatHASH160(data, preimage);
                return InputResult(ZERO32, InputStack(std::move(preimage)).Available(avail));
            }
            case NodeType::AND_V: {
                auto x = subs[0]->ProduceInput(ctx, nonmal), y = subs[1]->ProduceInput(ctx, nonmal);
                return InputResult((y.nsat + x.sat).NonCanon(), y.sat + x.sat);
            }
            case NodeType::AND_B: {
                auto x = subs[0]->ProduceInput(ctx, nonmal), y = subs[1]->ProduceInput(ctx, nonmal);
                return InputResult(Choose(Choose(y.nsat + x.nsat, (y.sat + x.nsat).NonCanon(), nonmal), (y.nsat + x.sat).NonCanon(), nonmal), y.sat + x.sat);
            }
            case NodeType::OR_B: {
                auto x = subs[0]->ProduceInput(ctx, nonmal), z = subs[1]->ProduceInput(ctx, nonmal);
                return InputResult(z.nsat + x.nsat, Choose(Choose(z.nsat + x.sat, z.sat + x.nsat, nonmal), (z.sat + x.sat).NonCanon(), nonmal));
            }
            case NodeType::OR_C: {
                auto x = subs[0]->ProduceInput(ctx, nonmal), z = subs[1]->ProduceInput(ctx, nonmal);
                return InputResult(INVALID, Choose(x.sat, z.sat + x.nsat, nonmal));
            }
            case NodeType::OR_D: {
                auto x = subs[0]->ProduceInput(ctx, nonmal), z = subs[1]->ProduceInput(ctx, nonmal);
                auto nsat = z.nsat + x.nsat, sat_l = x.sat, sat_r = z.sat + x.nsat;
                return InputResult(z.nsat + x.nsat, Choose(x.sat, z.sat + x.nsat, nonmal));
            }
            case NodeType::OR_I: {
                auto x = subs[0]->ProduceInput(ctx, nonmal), z = subs[1]->ProduceInput(ctx, nonmal);
                return InputResult(Choose(x.nsat + ONE, z.nsat + ZERO, nonmal), Choose(x.sat + ONE, z.sat + ZERO, nonmal));
            }
            case NodeType::ANDOR: {
                auto x = subs[0]->ProduceInput(ctx, nonmal), y = subs[1]->ProduceInput(ctx, nonmal), z = subs[2]->ProduceInput(ctx, nonmal);
                return InputResult(Choose((y.nsat + x.sat).NonCanon(), z.nsat + x.nsat, nonmal), Choose(y.sat + x.sat, z.sat + x.nsat, nonmal));
            }
            case NodeType::WRAP_A:
            case NodeType::WRAP_S:
            case NodeType::WRAP_C:
            case NodeType::WRAP_N:
                return subs[0]->ProduceInput(ctx, nonmal);
            case NodeType::WRAP_D: {
                auto x = subs[0]->ProduceInput(ctx, nonmal);
                return InputResult(ZERO, x.sat + ONE);
            }
            case NodeType::WRAP_J: {
                auto x = subs[0]->ProduceInput(ctx, nonmal);
                // If a dissatisfaction with a nonzero top stack element exists, an alternative dissatisfaction exists.
                // As the dissatisfaction logic currently doesn't keep track of this nonzeroness property, and thus even
                // if a dissatisfaction with a top zero element is found, we don't know whether another one with a
                // nonzero top stack element exists. Make the conservative assumption that whenever the subexpression is weakly
                // dissatisfiable, this alternative dissatisfaction exists and leads to malleability.
                return InputResult(InputStack(ZERO).Malleable(x.nsat.available != Availability::NO && !x.nsat.has_sig), x.sat);
            }
            case NodeType::WRAP_V: {
                auto x = subs[0]->ProduceInput(ctx, nonmal);
                return InputResult(INVALID, x.sat);
            }
            case NodeType::JUST_0: return InputResult(EMPTY, INVALID);
            case NodeType::JUST_1: return InputResult(INVALID, EMPTY);
        }
        assert(false);
        return InputResult(INVALID, INVALID);
    }

public:
    //! Return the size of the script for this expression (faster than ToScript().size()).
    size_t ScriptSize() const { return scriptlen; }

    //! Return the maximum number of ops needed to satisfy this script non-malleably.
    uint32_t GetOps() const { return ops.stat + ops.sat.value; }

    //! Check the ops limit of this script against the consensus limit.
    bool CheckOpsLimit() const { return GetOps() <= MAX_OPS_PER_SCRIPT; }

    //! Return the maximum number of stack elements needed to satisfy this script non-malleably.
    uint32_t GetStackSize() const { return ss.sat.value + 1; }

    //! Check the maximum stack size for this script against the policy limit.
    bool CheckStackSize() const { return GetStackSize() - 1 <= MAX_STANDARD_P2WSH_STACK_ITEMS; }

    //! Return the expression type.
    Type GetType() const { return typ; }

    //! Find the first insane sub, at the highest depth possible.
    std::optional<NodeRef<Key>> FindInsaneSub() const {
        if (IsSaneTopLevel()) return std::nullopt;

        for (const auto& sub : subs) {
            if (const auto subsub = sub->FindInsaneSub()) return subsub;
            if (!sub->IsSane()) return sub;
        }

        return std::nullopt;
    }

    //! Check whether this node is valid at all.
    bool IsValid() const { return !(GetType() == ""_mst) && ScriptSize() <= MAX_STANDARD_P2WSH_SCRIPT_SIZE; }

    //! Check whether this node is valid as a script on its own.
    bool IsValidTopLevel() const { return IsValid() && GetType() << "B"_mst; }

    //! Check whether this script can always be satisfied in a non-malleable way.
    bool IsNonMalleable() const { return GetType() << "m"_mst; }

    //! Check whether this script always needs a signature.
    bool NeedsSignature() const { return GetType() << "s"_mst; }

    //! Do all sanity checks.
    bool IsSane() const { return GetType() << "mk"_mst && CheckOpsLimit() && CheckStackSize() && IsValid(); }

    //! Check whether this node is safe as a script on its own.
    bool IsSaneTopLevel() const { return IsValidTopLevel() && IsSane() && NeedsSignature(); }

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

    template<typename Ctx>
    Availability Satisfy(const Ctx& ctx, std::vector<std::vector<unsigned char>>& stack, bool nonmalleable = true) const {
        auto ret = ProduceInput(ctx, nonmalleable);
        if (nonmalleable && (ret.sat.malleable || !ret.sat.has_sig)) return Availability::NO;
        stack = std::move(ret.sat.stack);
        return ret.sat.available;
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
    Node(NodeType nt, std::vector<NodeRef<Key>> sub, std::vector<unsigned char> arg, uint32_t val = 0) : nodetype(nt), k(val), data(std::move(arg)), subs(std::move(sub)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, std::vector<unsigned char> arg, uint32_t val = 0) : nodetype(nt), k(val), data(std::move(arg)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, std::vector<NodeRef<Key>> sub, std::vector<Key> key, uint32_t val = 0) : nodetype(nt), k(val), keys(std::move(key)), subs(std::move(sub)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, std::vector<Key> key, uint32_t val = 0) : nodetype(nt), k(val), keys(std::move(key)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, std::vector<NodeRef<Key>> sub, uint32_t val = 0) : nodetype(nt), k(val), subs(std::move(sub)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(NodeType nt, uint32_t val = 0) : nodetype(nt), k(val), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
};

namespace internal {

enum class ParseContext {
    /** An expression which may be begin with wrappers followed by a colon. */
    WRAPPED_EXPR,
    /** A miniscript expression which does not begin with wrappers. */
    EXPR,

    /** SWAP wraps the top constructed node with s: */
    SWAP,
    /** ALT wraps the top constructed node with a: */
    ALT,
    /** CHECK wraps the top constructed node with c: */
    CHECK,
    /** DUP_IF wraps the top constructed node with d: */
    DUP_IF,
    /** VERIFY wraps the top constructed node with v: */
    VERIFY,
    /** NON_ZERO wraps the top constructed node with j: */
    NON_ZERO,
    /** ZERO_NOTEQUAL wraps the top constructed node with n: */
    ZERO_NOTEQUAL,
    /** WRAP_U will construct an or_i(X,0) node from the top constructed node. */
    WRAP_U,
    /** WRAP_T will construct an and_v(X,1) node from the top constructed node. */
    WRAP_T,

    /** AND_N will construct an andor(X,Y,0) node from the last two constructed nodes. */
    AND_N,
    /** AND_V will construct an and_v node from the last two constructed nodes. */
    AND_V,
    /** AND_B will construct an and_b node from the last two constructed nodes. */
    AND_B,
    /** ANDOR will construct an andor node from the last three constructed nodes. */
    ANDOR,
    /** OR_B will construct an or_b node from the last two constructed nodes. */
    OR_B,
    /** OR_C will construct an or_c node from the last two constructed nodes. */
    OR_C,
    /** OR_D will construct an or_d node from the last two constructed nodes. */
    OR_D,
    /** OR_I will construct an or_i node from the last two constructed nodes. */
    OR_I,

    /** THRESH will read a wrapped expression, and then look for a COMMA. If
     * no comma follows, it will construct a thresh node from the appropriate
     * number of constructed children. Otherwise, it will recurse with another
     * THRESH. */
    THRESH,

    /** COMMA expects the next element to be ',' and fails if not. */
    COMMA,
    /** CLOSE_BRACKET expects the next element to be ')' and fails if not. */
    CLOSE_BRACKET,
};


int FindNextChar(Span<const char> in, const char m);

/** BuildBack pops the last two elements off `constructed` and wraps them in the specified NodeType */
template<typename Key>
void BuildBack(NodeType nt, std::vector<NodeRef<Key>>& constructed, const bool reverse = false)
{
    NodeRef<Key> child = std::move(constructed.back());
    constructed.pop_back();
    if (reverse) {
        constructed.back() = MakeNodeRef<Key>(nt, Vector(std::move(child), std::move(constructed.back())));
    } else {
        constructed.back() = MakeNodeRef<Key>(nt, Vector(std::move(constructed.back()), std::move(child)));
    }
}

//! Parse a miniscript from its textual descriptor form.
template<typename Key, typename Ctx>
inline NodeRef<Key> Parse(Span<const char> in, const Ctx& ctx)
{
    using namespace spanparsing;

    // The two integers are used to hold state for thresh()
    std::vector<std::tuple<ParseContext, int64_t, int64_t>> to_parse;
    std::vector<NodeRef<Key>> constructed;

    to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);

    while (!to_parse.empty()) {
        // Get the current context we are decoding within
        auto [cur_context, n, k] = to_parse.back();
        to_parse.pop_back();

        switch (cur_context) {
        case ParseContext::WRAPPED_EXPR: {
            int colon_index = -1;
            for (int i = 1; i < (int)in.size(); ++i) {
                if (in[i] == ':') {
                    colon_index = i;
                    break;
                }
                if (in[i] < 'a' || in[i] > 'z') break;
            }
            // If there is no colon, this loop won't execute
            for (int j = 0; j < colon_index; ++j) {
                if (in[j] == 'a') {
                    to_parse.emplace_back(ParseContext::ALT, -1, -1);
                } else if (in[j] == 's') {
                    to_parse.emplace_back(ParseContext::SWAP, -1, -1);
                } else if (in[j] == 'c') {
                    to_parse.emplace_back(ParseContext::CHECK, -1, -1);
                } else if (in[j] == 'd') {
                    to_parse.emplace_back(ParseContext::DUP_IF, -1, -1);
                } else if (in[j] == 'j') {
                    to_parse.emplace_back(ParseContext::NON_ZERO, -1, -1);
                } else if (in[j] == 'n') {
                    to_parse.emplace_back(ParseContext::ZERO_NOTEQUAL, -1, -1);
                } else if (in[j] == 'v') {
                    to_parse.emplace_back(ParseContext::VERIFY, -1, -1);
                } else if (in[j] == 'u') {
                    to_parse.emplace_back(ParseContext::WRAP_U, -1, -1);
                } else if (in[j] == 't') {
                    to_parse.emplace_back(ParseContext::WRAP_T, -1, -1);
                } else if (in[j] == 'l') {
                    // The l: wrapper is equivalent to or_i(0,X)
                    constructed.push_back(MakeNodeRef<Key>(NodeType::JUST_0));
                    to_parse.emplace_back(ParseContext::OR_I, -1, -1);
                } else {
                    return {};
                }
            }
            to_parse.emplace_back(ParseContext::EXPR, -1, -1);
            in = in.subspan(colon_index + 1);
            break;
        }
        case ParseContext::EXPR: {
            if (Const("0", in)) {
                constructed.push_back(MakeNodeRef<Key>(NodeType::JUST_0));
            } else if (Const("1", in)) {
                constructed.push_back(MakeNodeRef<Key>(NodeType::JUST_1));
            } else if (Const("pk(", in)) {
                Key key;
                int key_size = FindNextChar(in, ')');
                if (key_size < 1) return {};
                if (!ctx.FromString(in.begin(), in.begin() + key_size, key)) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::WRAP_C, Vector(MakeNodeRef<Key>(NodeType::PK_K, Vector(std::move(key))))));
                in = in.subspan(key_size + 1);
            } else if (Const("pkh(", in)) {
                Key key;
                int key_size = FindNextChar(in, ')');
                if (key_size < 1) return {};
                if (!ctx.FromString(in.begin(), in.begin() + key_size, key)) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::WRAP_C, Vector(MakeNodeRef<Key>(NodeType::PK_H, Vector(std::move(key))))));
                in = in.subspan(key_size + 1);
            } else if (Const("pk_k(", in)) {
                Key key;
                int key_size = FindNextChar(in, ')');
                if (key_size < 1) return {};
                if (!ctx.FromString(in.begin(), in.begin() + key_size, key)) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::PK_K, Vector(std::move(key))));
                in = in.subspan(key_size + 1);
            } else if (Const("pk_h(", in)) {
                Key key;
                int key_size = FindNextChar(in, ')');
                if (key_size < 1) return {};
                if (!ctx.FromString(in.begin(), in.begin() + key_size, key)) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::PK_H, Vector(std::move(key))));
                in = in.subspan(key_size + 1);
            } else if (Const("sha256(", in)) {
                int hash_size = FindNextChar(in, ')');
                if (hash_size < 1) return {};
                std::string val = std::string(in.begin(), in.begin() + hash_size);
                if (!IsHex(val)) return {};
                auto hash = ParseHex(val);
                if (hash.size() != 32) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::SHA256, std::move(hash)));
                in = in.subspan(hash_size + 1);
            } else if (Const("ripemd160(", in)) {
                int hash_size = FindNextChar(in, ')');
                if (hash_size < 1) return {};
                std::string val = std::string(in.begin(), in.begin() + hash_size);
                if (!IsHex(val)) return {};
                auto hash = ParseHex(val);
                if (hash.size() != 20) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::RIPEMD160, std::move(hash)));
                in = in.subspan(hash_size + 1);
            } else if (Const("hash256(", in)) {
                int hash_size = FindNextChar(in, ')');
                if (hash_size < 1) return {};
                std::string val = std::string(in.begin(), in.begin() + hash_size);
                if (!IsHex(val)) return {};
                auto hash = ParseHex(val);
                if (hash.size() != 32) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::HASH256, std::move(hash)));
                in = in.subspan(hash_size + 1);
            } else if (Const("hash160(", in)) {
                int hash_size = FindNextChar(in, ')');
                if (hash_size < 1) return {};
                std::string val = std::string(in.begin(), in.begin() + hash_size);
                if (!IsHex(val)) return {};
                auto hash = ParseHex(val);
                if (hash.size() != 20) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::HASH160, std::move(hash)));
                in = in.subspan(hash_size + 1);
            } else if (Const("after(", in)) {
                int arg_size = FindNextChar(in, ')');
                if (arg_size < 1) return {};
                int64_t num;
                if (!ParseInt64(std::string(in.begin(), in.begin() + arg_size), &num)) return {};
                if (num < 1 || num >= 0x80000000L) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::AFTER, num));
                in = in.subspan(arg_size + 1);
            } else if (Const("older(", in)) {
                int arg_size = FindNextChar(in, ')');
                if (arg_size < 1) return {};
                int64_t num;
                if (!ParseInt64(std::string(in.begin(), in.begin() + arg_size), &num)) return {};
                if (num < 1 || num >= 0x80000000L) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::OLDER, num));
                in = in.subspan(arg_size + 1);
            } else if (Const("multi(", in)) {
                // Get threshold
                int next_comma = FindNextChar(in, ',');
                if (next_comma < 1) return {};
                if (!ParseInt64(std::string(in.begin(), in.begin() + next_comma), &k)) return {};
                in = in.subspan(next_comma + 1);
                // Get keys
                std::vector<Key> keys;
                while (next_comma != -1) {
                    Key key;
                    next_comma = FindNextChar(in, ',');
                    int key_length = (next_comma == -1) ? FindNextChar(in, ')') : next_comma;
                    if (key_length < 1) return {};
                    if (!ctx.FromString(in.begin(), in.begin() + key_length, key)) return {};
                    keys.push_back(std::move(key));
                    in = in.subspan(key_length + 1);
                }
                if (keys.size() < 1 || keys.size() > 20) return {};
                if (k < 1 || k > (int64_t)keys.size()) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::MULTI, std::move(keys), k));
            } else if (Const("thresh(", in)) {
                int next_comma = FindNextChar(in, ',');
                if (next_comma < 1) return {};
                if (!ParseInt64(std::string(in.begin(), in.begin() + next_comma), &k)) return {};
                if (k < 1) return {};
                in = in.subspan(next_comma + 1);
                // n = 1 here because we read the first WRAPPED_EXPR before reaching THRESH
                to_parse.emplace_back(ParseContext::THRESH, 1, k);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
            } else if (Const("andor(", in)) {
                to_parse.emplace_back(ParseContext::ANDOR, -1, -1);
                to_parse.emplace_back(ParseContext::CLOSE_BRACKET, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                to_parse.emplace_back(ParseContext::COMMA, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                to_parse.emplace_back(ParseContext::COMMA, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
            } else {
                if (Const("and_n(", in)) {
                    to_parse.emplace_back(ParseContext::AND_N, -1, -1);
                } else if (Const("and_b(", in)) {
                    to_parse.emplace_back(ParseContext::AND_B, -1, -1);
                } else if (Const("and_v(", in)) {
                    to_parse.emplace_back(ParseContext::AND_V, -1, -1);
                } else if (Const("or_b(", in)) {
                    to_parse.emplace_back(ParseContext::OR_B, -1, -1);
                } else if (Const("or_c(", in)) {
                    to_parse.emplace_back(ParseContext::OR_C, -1, -1);
                } else if (Const("or_d(", in)) {
                    to_parse.emplace_back(ParseContext::OR_D, -1, -1);
                } else if (Const("or_i(", in)) {
                    to_parse.emplace_back(ParseContext::OR_I, -1, -1);
                } else {
                    return {};
                }
                to_parse.emplace_back(ParseContext::CLOSE_BRACKET, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                to_parse.emplace_back(ParseContext::COMMA, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
            }
            break;
        }
        case ParseContext::ALT: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_A, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::SWAP: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_S, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::CHECK: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_C, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::DUP_IF: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_D, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::NON_ZERO: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_J, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::ZERO_NOTEQUAL: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_N, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::VERIFY: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_V, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::WRAP_U: {
            constructed.back() = MakeNodeRef<Key>(NodeType::OR_I, Vector(std::move(constructed.back()), MakeNodeRef<Key>(NodeType::JUST_0)));
            break;
        }
        case ParseContext::WRAP_T: {
            constructed.back() = MakeNodeRef<Key>(NodeType::AND_V, Vector(std::move(constructed.back()), MakeNodeRef<Key>(NodeType::JUST_1)));
            break;
        }
        case ParseContext::AND_B: {
            BuildBack(NodeType::AND_B, constructed);
            break;
        }
        case ParseContext::AND_N: {
            auto mid = std::move(constructed.back());
            constructed.pop_back();
            constructed.back() = MakeNodeRef<Key>(NodeType::ANDOR, Vector(std::move(constructed.back()), std::move(mid), MakeNodeRef<Key>(NodeType::JUST_0)));
            break;
        }
        case ParseContext::AND_V: {
            BuildBack(NodeType::AND_V, constructed);
            break;
        }
        case ParseContext::OR_B: {
            BuildBack(NodeType::OR_B, constructed);
            break;
        }
        case ParseContext::OR_C: {
            BuildBack(NodeType::OR_C, constructed);
            break;
        }
        case ParseContext::OR_D: {
            BuildBack(NodeType::OR_D, constructed);
            break;
        }
        case ParseContext::OR_I: {
            BuildBack(NodeType::OR_I, constructed);
            break;
        }
        case ParseContext::ANDOR: {
            auto right = std::move(constructed.back());
            constructed.pop_back();
            auto mid = std::move(constructed.back());
            constructed.pop_back();
            constructed.back() = MakeNodeRef<Key>(NodeType::ANDOR, Vector(std::move(constructed.back()), std::move(mid), std::move(right)));
            break;
        }
        case ParseContext::THRESH: {
            if (in.size() < 1) return {};
            if (in[0] == ',') {
                in = in.subspan(1);
                to_parse.emplace_back(ParseContext::THRESH, n+1, k);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
            } else if (in[0] == ')') {
                if (k > n) return {};
                in = in.subspan(1);
                // Children are constructed in reverse order, so iterate from end to beginning
                std::vector<NodeRef<Key>> subs;
                for (int i = 0; i < n; ++i) {
                    subs.push_back(std::move(constructed.back()));
                    constructed.pop_back();
                }
                std::reverse(subs.begin(), subs.end());
                constructed.push_back(MakeNodeRef<Key>(NodeType::THRESH, std::move(subs), k));
            } else {
                return {};
            }
            break;
        }
        case ParseContext::COMMA: {
            if (in.size() < 1 || in[0] != ',') return {};
            in = in.subspan(1);
            break;
        }
        case ParseContext::CLOSE_BRACKET: {
            if (in.size() < 1 || in[0] != ')') return {};
            in = in.subspan(1);
            break;
        }
        }
    }

    // Sanity checks on the produced miniscript
    assert(constructed.size() == 1);
    if (in.size() > 0) return {};
    const NodeRef<Key> tl_node = std::move(constructed.front());
    if (!tl_node->IsValidTopLevel()) return {};
    return tl_node;
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

enum class DecodeContext {
    /** A single expression of type B, K, or V. Specifically, this can't be an
     * and_v or an expression of type W (a: and s: wrappers). */
    SINGLE_BKV_EXPR,
    /** Potentially multiple SINGLE_BKV_EXPRs as children of (potentially multiple)
     * and_v expressions. Syntactic sugar for MAYBE_AND_V + SINGLE_BKV_EXPR. */
    BKV_EXPR,
    /** An expression of type W (a: or s: wrappers). */
    W_EXPR,

    /** SWAP expects the next element to be OP_SWAP (inside a W-type expression that
     * didn't end with FROMALTSTACK), and wraps the top of the constructed stack
     * with s: */
    SWAP,
    /** ALT expects the next element to be TOALTSTACK (we must have already read a
     * FROMALTSTACK earlier), and wraps the top of the constructed stack with a: */
    ALT,
    /** CHECK wraps the top constructed node with c: */
    CHECK,
    /** DUP_IF wraps the top constructed node with d: */
    DUP_IF,
    /** VERIFY wraps the top constructed node with v: */
    VERIFY,
    /** NON_ZERO wraps the top constructed node with j: */
    NON_ZERO,
    /** ZERO_NOTEQUAL wraps the top constructed node with n: */
    ZERO_NOTEQUAL,

    /** MAYBE_AND_V will check if the next part of the script could be a valid
     * miniscript sub-expression, and if so it will push AND_V and SINGLE_BKV_EXPR
     * to decode it and construct the and_v node. This is recursive, to deal with
     * multiple and_v nodes inside each other. */
    MAYBE_AND_V,
    /** AND_V will construct an and_v node from the last two constructed nodes. */
    AND_V,
    /** AND_B will construct an and_b node from the last two constructed nodes. */
    AND_B,
    /** ANDOR will construct an andor node from the last three constructed nodes. */
    ANDOR,
    /** OR_B will construct an or_b node from the last two constructed nodes. */
    OR_B,
    /** OR_C will construct an or_c node from the last two constructed nodes. */
    OR_C,
    /** OR_D will construct an or_d node from the last two constructed nodes. */
    OR_D,

    /** In a thresh expression, all sub-expressions other than the first are W-type,
     * and end in OP_ADD. THRESH_W will check for this OP_ADD and either push a W_EXPR
     * or a SINGLE_BKV_EXPR and jump to THRESH_E accordingly. */
    THRESH_W,
    /** THRESH_E constructs a thresh node from the appropriate number of constructed
     * children. */
    THRESH_E,

    /** ENDIF signals that we are inside some sort of OP_IF structure, which could be
     * or_d, or_c, or_i, andor, d:, or j: wrapper, depending on what follows. We read
     * a BKV_EXPR and then deal with the next opcode case-by-case. */
    ENDIF,
    /** If, inside an ENDIF context, we find an OP_NOTIF before finding an OP_ELSE,
     * we could either be in an or_d or an or_c node. We then check for IFDUP to
     * distinguish these cases. */
    ENDIF_NOTIF,
    /** If, inside an ENDIF context, we find an OP_ELSE, then we could be in either an
     * or_i or an andor node. Read the next BKV_EXPR and find either an OP_IF or an
     * OP_NOTIF. */
    ENDIF_ELSE,
};

//! Parse a miniscript from a bitcoin script
template<typename Key, typename Ctx, typename I>
inline NodeRef<Key> DecodeScript(I& in, I last, const Ctx& ctx)
{
    // The two integers are used to hold state for thresh()
    std::vector<std::tuple<DecodeContext, int64_t, int64_t>> to_parse;
    std::vector<NodeRef<Key>> constructed;

    // This is the top level, so we assume the type is B
    // (in particular, disallowing top level W expressions)
    to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);

    while (!to_parse.empty()) {
        // Exit early if the Miniscript is not going to be valid.
        if (!constructed.empty() && !constructed.back()->IsValid()) return {};

        // Get the current context we are decoding within
        auto [cur_context, n, k] = to_parse.back();
        to_parse.pop_back();

        switch(cur_context) {
        case DecodeContext::SINGLE_BKV_EXPR: {
            if (in >= last) return {};

            // Constants
            if (in[0].first == OP_1) {
                ++in;
                constructed.push_back(MakeNodeRef<Key>(NodeType::JUST_1));
            } else if (in[0].first == OP_0) {
                ++in;
                constructed.push_back(MakeNodeRef<Key>(NodeType::JUST_0));
            }
            // Public keys
            else if (in[0].second.size() == 33) {
                Key key;
                if (!ctx.FromPKBytes(in[0].second.begin(), in[0].second.end(), key)) return {};
                ++in;
                constructed.push_back(MakeNodeRef<Key>(NodeType::PK_K, Vector(std::move(key))));
            } else if (last - in >= 5 && in[0].first == OP_VERIFY && in[1].first == OP_EQUAL && in[3].first == OP_HASH160 && in[4].first == OP_DUP && in[2].second.size() == 20) {
                Key key;
                if (!ctx.FromPKHBytes(in[2].second.begin(), in[2].second.end(), key)) return {};
                in += 5;
                constructed.push_back(MakeNodeRef<Key>(NodeType::PK_H, Vector(std::move(key))));
            }
            // Time locks
            else if (last - in >= 2 && in[0].first == OP_CHECKSEQUENCEVERIFY && ParseScriptNumber(in[1], k)) {
                in += 2;
                if (k < 1 || k > 0x7FFFFFFFL) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::OLDER, k));
            } else if (last - in >= 2 && in[0].first == OP_CHECKLOCKTIMEVERIFY && ParseScriptNumber(in[1], k)) {
                in += 2;
                if (k < 1 || k > 0x7FFFFFFFL) return {};
                constructed.push_back(MakeNodeRef<Key>(NodeType::AFTER, k));
            }
            // Hashes
            else if (last - in >= 7 && in[0].first == OP_EQUAL && in[1].second.size() == 32 && in[2].first == OP_SHA256 && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && ParseScriptNumber(in[5], k) && k == 32 && in[6].first == OP_SIZE) {
                constructed.push_back(MakeNodeRef<Key>(NodeType::SHA256, in[1].second));
                in += 7;
            } else if (last - in >= 7 && in[0].first == OP_EQUAL && in[1].second.size() == 20 && in[2].first == OP_RIPEMD160 && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && ParseScriptNumber(in[5], k) && k == 32 && in[6].first == OP_SIZE) {
                constructed.push_back(MakeNodeRef<Key>(NodeType::RIPEMD160, in[1].second));
                in += 7;
            } else if (last - in >= 7 && in[0].first == OP_EQUAL && in[1].second.size() == 32 && in[2].first == OP_HASH256 && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && ParseScriptNumber(in[5], k) && k == 32 && in[6].first == OP_SIZE) {
                constructed.push_back(MakeNodeRef<Key>(NodeType::HASH256, in[1].second));
                in += 7;
            } else if (last - in >= 7 && in[0].first == OP_EQUAL && in[1].second.size() == 20 && in[2].first == OP_HASH160 && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && ParseScriptNumber(in[5], k) && k == 32 && in[6].first == OP_SIZE) {
                constructed.push_back(MakeNodeRef<Key>(NodeType::HASH160, in[1].second));
                in += 7;
            }
            // Multi
            else if (last - in >= 3 && in[0].first == OP_CHECKMULTISIG) {
                std::vector<Key> keys;
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
                constructed.push_back(MakeNodeRef<Key>(NodeType::MULTI, std::move(keys), k));
            }
            /** In the following wrappers, we only need to push SINGLE_BKV_EXPR rather
             * than BKV_EXPR, because and_v commutes with these wrappers. For example,
             * c:and_v(X,Y) produces the same script as and_v(X,c:Y). */
            // c: wrapper
            else if (in[0].first == OP_CHECKSIG) {
                ++in;
                to_parse.emplace_back(DecodeContext::CHECK, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            }
            // v: wrapper
            else if (in[0].first == OP_VERIFY) {
                ++in;
                to_parse.emplace_back(DecodeContext::VERIFY, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            }
            // n: wrapper
            else if (in[0].first == OP_0NOTEQUAL) {
                ++in;
                to_parse.emplace_back(DecodeContext::ZERO_NOTEQUAL, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            }
            // Thresh
            else if (last - in >= 3 && in[0].first == OP_EQUAL && ParseScriptNumber(in[1], k)) {
                if (k < 1) return {};
                in += 2;
                to_parse.emplace_back(DecodeContext::THRESH_W, 0, k);
            }
            // OP_ENDIF can be WRAP_J, WRAP_D, ANDOR, OR_C, OR_D, or OR_I
            else if (in[0].first == OP_ENDIF) {
                ++in;
                to_parse.emplace_back(DecodeContext::ENDIF, -1, -1);
                to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
            }
            /** In and_b and or_b nodes, we only look for SINGLE_BKV_EXPR, because
             * or_b(and_v(X,Y),Z) has script [X] [Y] [Z] OP_BOOLOR, the same as
             * and_v(X,or_b(Y,Z)). In this example, the former of these is invalid as
             * miniscript, while the latter is valid. So we leave the and_v "outside"
             * while decoding. */
            // and_b
            else if (in[0].first == OP_BOOLAND) {
                ++in;
                to_parse.emplace_back(DecodeContext::AND_B, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                to_parse.emplace_back(DecodeContext::W_EXPR, -1, -1);
            }
            // or_b
            else if (in[0].first == OP_BOOLOR) {
                ++in;
                to_parse.emplace_back(DecodeContext::OR_B, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                to_parse.emplace_back(DecodeContext::W_EXPR, -1, -1);
            } else {
                // Unrecognised expression
                return {};
            }
            break;
        }
        case DecodeContext::BKV_EXPR: {
            to_parse.emplace_back(DecodeContext::MAYBE_AND_V, -1, -1);
            to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            break;
        }
        case DecodeContext::W_EXPR: {
            // a: wrapper
            if (in >= last) return {};
            if (in[0].first == OP_FROMALTSTACK) {
                ++in;
                to_parse.emplace_back(DecodeContext::ALT, -1, -1);
            } else {
                to_parse.emplace_back(DecodeContext::SWAP, -1, -1);
            }
            to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
            break;
        }
        case DecodeContext::MAYBE_AND_V: {
            // If we reach a potential AND_V top-level, check if the next part of the script could be another AND_V child
            // These op-codes cannot end any well-formed miniscript so cannot be used in an and_v node.
            if (in < last && in[0].first != OP_IF && in[0].first != OP_ELSE && in[0].first != OP_NOTIF && in[0].first != OP_TOALTSTACK && in[0].first != OP_SWAP) {
                to_parse.emplace_back(DecodeContext::AND_V, -1, -1);
                // BKV_EXPR can contain more AND_V nodes
                to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
            }
            break;
        }
        case DecodeContext::SWAP: {
            if (in >= last || in[0].first != OP_SWAP) return {};
            ++in;
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_S, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::ALT: {
            if (in >= last || in[0].first != OP_TOALTSTACK) return {};
            ++in;
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_A, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::CHECK: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_C, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::DUP_IF: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_D, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::VERIFY: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_V, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::NON_ZERO: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_J, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::ZERO_NOTEQUAL: {
            constructed.back() = MakeNodeRef<Key>(NodeType::WRAP_N, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::AND_V: {
            BuildBack(NodeType::AND_V, constructed, /* reverse */ true);
            break;
        }
        case DecodeContext::AND_B: {
            BuildBack(NodeType::AND_B, constructed, /* reverse */ true);
            break;
        }
        case DecodeContext::OR_B: {
            BuildBack(NodeType::OR_B, constructed, /* reverse */ true);
            break;
        }
        case DecodeContext::OR_C: {
            BuildBack(NodeType::OR_C, constructed, /* reverse */ true);
            break;
        }
        case DecodeContext::OR_D: {
            BuildBack(NodeType::OR_D, constructed, /* reverse */ true);
            break;
        }
        case DecodeContext::ANDOR: {
            NodeRef<Key> left = std::move(constructed.back());
            constructed.pop_back();
            NodeRef<Key> right = std::move(constructed.back());
            constructed.pop_back();
            NodeRef<Key> mid = std::move(constructed.back());
            constructed.back() = MakeNodeRef<Key>(NodeType::ANDOR, Vector(std::move(left), std::move(mid), std::move(right)));
            break;
        }
        case DecodeContext::THRESH_W: {
            if (in >= last) return {};
            if (in[0].first == OP_ADD) {
                ++in;
                to_parse.emplace_back(DecodeContext::THRESH_W, n+1, k);
                to_parse.emplace_back(DecodeContext::W_EXPR, -1, -1);
            } else {
                to_parse.emplace_back(DecodeContext::THRESH_E, n+1, k);
                // All children of thresh have type modifier d, so cannot be and_v
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            }
            break;
        }
        case DecodeContext::THRESH_E: {
            if (k < 1 || k > n) return {};
            std::vector<NodeRef<Key>> subs;
            for (int i = 0; i < n; ++i) {
                NodeRef<Key> sub = std::move(constructed.back());
                constructed.pop_back();
                subs.push_back(std::move(sub));
            }
            constructed.push_back(MakeNodeRef<Key>(NodeType::THRESH, std::move(subs), k));
            break;
        }
        case DecodeContext::ENDIF: {
            if (in >= last) return {};

            // could be andor or or_i
            if (in[0].first == OP_ELSE) {
                ++in;
                to_parse.emplace_back(DecodeContext::ENDIF_ELSE, -1, -1);
                to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
            }
            // could be j: or d: wrapper
            else if (in[0].first == OP_IF) {
                if (last - in >= 2 && in[1].first == OP_DUP) {
                    in += 2;
                    to_parse.emplace_back(DecodeContext::DUP_IF, -1, -1);
                } else if (last - in >= 3 && in[1].first == OP_0NOTEQUAL && in[2].first == OP_SIZE) {
                    in += 3;
                    to_parse.emplace_back(DecodeContext::NON_ZERO, -1, -1);
                }
                else {
                    return {};
                }
            // could be or_c or or_d
            } else if (in[0].first == OP_NOTIF) {
                ++in;
                to_parse.emplace_back(DecodeContext::ENDIF_NOTIF, -1, -1);
            }
            else {
                return {};
            }
            break;
        }
        case DecodeContext::ENDIF_NOTIF: {
            if (in >= last) return {};
            if (in[0].first == OP_IFDUP) {
                ++in;
                to_parse.emplace_back(DecodeContext::OR_D, -1, -1);
            } else {
                to_parse.emplace_back(DecodeContext::OR_C, -1, -1);
            }
            // or_c and or_d both require X to have type modifier d so, can't contain and_v
            to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            break;
        }
        case DecodeContext::ENDIF_ELSE: {
            if (in >= last) return {};
            if (in[0].first == OP_IF) {
                ++in;
                BuildBack(NodeType::OR_I, constructed, /* reverse */ true);
            } else if (in[0].first == OP_NOTIF) {
                ++in;
                to_parse.emplace_back(DecodeContext::ANDOR, -1, -1);
                // andor requires X to have type modifier d, so it can't be and_v
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            } else {
                return {};
            }
            break;
        }
        }
    }
    if (constructed.size() != 1) return {};
    const NodeRef<Key> tl_node = std::move(constructed.front());
    // Note that due to how ComputeType works (only assign the type to the node if the
    // subs' types are valid) this would fail if any node of tree is badly typed.
    if (!tl_node->IsValidTopLevel()) return {};
    return tl_node;
}

} // namespace internal

template<typename Ctx>
inline NodeRef<typename Ctx::Key> FromString(const std::string& str, const Ctx& ctx) {
    return internal::Parse<typename Ctx::Key>(str, ctx);
}

template<typename Ctx>
inline NodeRef<typename Ctx::Key> FromScript(const CScript& script, const Ctx& ctx) {
    using namespace internal;
    std::vector<std::pair<opcodetype, std::vector<unsigned char>>> decomposed;
    if (!DecomposeScript(script, decomposed)) return {};
    auto it = decomposed.begin();
    auto ret = DecodeScript<typename Ctx::Key>(it, decomposed.end(), ctx);
    if (!ret) return {};
    if (it != decomposed.end()) return {};
    return ret;
}

} // namespace miniscript

#endif // BITCOIN_SCRIPT_MINISCRIPT_H
