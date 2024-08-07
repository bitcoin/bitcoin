// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_MINISCRIPT_H
#define BITCOIN_SCRIPT_MINISCRIPT_H

#include <algorithm>
#include <functional>
#include <numeric>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <assert.h>
#include <cstdlib>

#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/parsing.h>
#include <script/script.h>
#include <span.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/string.h>
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
 *   - When satisfied, pushes nothing.
 *   - Cannot be dissatisfied.
 *   - This can be obtained by adding an OP_VERIFY to a B, modifying the last opcode
 *     of a B to its -VERIFY version (only for OP_CHECKSIG, OP_CHECKSIGVERIFY,
 *     OP_NUMEQUAL and OP_EQUAL), or by combining a V fragment under some conditions.
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
 * - "i" Whether the branch contains an absolute time timelock
 * - "j" Whether the branch contains an absolute height timelock
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

    //! Internal constructor.
    explicit constexpr Type(uint32_t flags) noexcept : m_flags(flags) {}

public:
    //! Construction function used by the ""_mst operator.
    static consteval Type Make(uint32_t flags) noexcept { return Type(flags); }

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
inline consteval Type operator"" _mst(const char* c, size_t l) {
    Type typ{Type::Make(0)};

    for (const char *p = c; p < c + l; p++) {
        typ = typ | Type::Make(
            *p == 'B' ? 1 << 0 : // Base type
            *p == 'V' ? 1 << 1 : // Verify type
            *p == 'K' ? 1 << 2 : // Key type
            *p == 'W' ? 1 << 3 : // Wrapped type
            *p == 'z' ? 1 << 4 : // Zero-arg property
            *p == 'o' ? 1 << 5 : // One-arg property
            *p == 'n' ? 1 << 6 : // Nonzero arg property
            *p == 'd' ? 1 << 7 : // Dissatisfiable property
            *p == 'u' ? 1 << 8 : // Unit property
            *p == 'e' ? 1 << 9 : // Expression property
            *p == 'f' ? 1 << 10 : // Forced property
            *p == 's' ? 1 << 11 : // Safe property
            *p == 'm' ? 1 << 12 : // Nonmalleable property
            *p == 'x' ? 1 << 13 : // Expensive verify
            *p == 'g' ? 1 << 14 : // older: contains relative time timelock   (csv_time)
            *p == 'h' ? 1 << 15 : // older: contains relative height timelock (csv_height)
            *p == 'i' ? 1 << 16 : // after: contains time timelock   (cltv_time)
            *p == 'j' ? 1 << 17 : // after: contains height timelock   (cltv_height)
            *p == 'k' ? 1 << 18 : // does not contain a combination of height and time locks
            (throw std::logic_error("Unknown character in _mst literal"), 0)
        );
    }

    return typ;
}

using Opcode = std::pair<opcodetype, std::vector<unsigned char>>;

template<typename Key> struct Node;
template<typename Key> using NodeRef = std::shared_ptr<const Node<Key>>;

//! Construct a miniscript node as a shared_ptr.
template<typename Key, typename... Args>
NodeRef<Key> MakeNodeRef(Args&&... args) { return std::make_shared<const Node<Key>>(std::forward<Args>(args)...); }

//! The different node types in miniscript.
enum class Fragment {
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
    MULTI,     //!< [k] [key_n]* [n] OP_CHECKMULTISIG (only available within P2WSH context)
    MULTI_A,   //!< [key_0] OP_CHECKSIG ([key_n] OP_CHECKSIGADD)* [k] OP_NUMEQUAL (only within Tapscript ctx)
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

enum class MiniscriptContext {
    P2WSH,
    TAPSCRIPT,
};

/** Whether the context Tapscript, ensuring the only other possibility is P2WSH. */
constexpr bool IsTapscript(MiniscriptContext ms_ctx)
{
    switch (ms_ctx) {
        case MiniscriptContext::P2WSH: return false;
        case MiniscriptContext::TAPSCRIPT: return true;
    }
    assert(false);
}

namespace internal {

//! The maximum size of a witness item for a Miniscript under Tapscript context. (A BIP340 signature with a sighash type byte.)
static constexpr uint32_t MAX_TAPMINISCRIPT_STACK_ELEM_SIZE{65};

//! version + nLockTime
constexpr uint32_t TX_OVERHEAD{4 + 4};
//! prevout + nSequence + scriptSig
constexpr uint32_t TXIN_BYTES_NO_WITNESS{36 + 4 + 1};
//! nValue + script len + OP_0 + pushdata 32.
constexpr uint32_t P2WSH_TXOUT_BYTES{8 + 1 + 1 + 33};
//! Data other than the witness in a transaction. Overhead + vin count + one vin + vout count + one vout + segwit marker
constexpr uint32_t TX_BODY_LEEWAY_WEIGHT{(TX_OVERHEAD + GetSizeOfCompactSize(1) + TXIN_BYTES_NO_WITNESS + GetSizeOfCompactSize(1) + P2WSH_TXOUT_BYTES) * WITNESS_SCALE_FACTOR + 2};
//! Maximum possible stack size to spend a Taproot output (excluding the script itself).
constexpr uint32_t MAX_TAPSCRIPT_SAT_SIZE{GetSizeOfCompactSize(MAX_STACK_SIZE) + (GetSizeOfCompactSize(MAX_TAPMINISCRIPT_STACK_ELEM_SIZE) + MAX_TAPMINISCRIPT_STACK_ELEM_SIZE) * MAX_STACK_SIZE + GetSizeOfCompactSize(TAPROOT_CONTROL_MAX_SIZE) + TAPROOT_CONTROL_MAX_SIZE};
/** The maximum size of a script depending on the context. */
constexpr uint32_t MaxScriptSize(MiniscriptContext ms_ctx)
{
    if (IsTapscript(ms_ctx)) {
        // Leaf scripts under Tapscript are not explicitly limited in size. They are only implicitly
        // bounded by the maximum standard size of a spending transaction. Let the maximum script
        // size conservatively be small enough such that even a maximum sized witness and a reasonably
        // sized spending transaction can spend an output paying to this script without running into
        // the maximum standard tx size limit.
        constexpr auto max_size{MAX_STANDARD_TX_WEIGHT - TX_BODY_LEEWAY_WEIGHT - MAX_TAPSCRIPT_SAT_SIZE};
        return max_size - GetSizeOfCompactSize(max_size);
    }
    return MAX_STANDARD_P2WSH_SCRIPT_SIZE;
}

//! Helper function for Node::CalcType.
Type ComputeType(Fragment fragment, Type x, Type y, Type z, const std::vector<Type>& sub_types, uint32_t k, size_t data_size, size_t n_subs, size_t n_keys, MiniscriptContext ms_ctx);

//! Helper function for Node::CalcScriptLen.
size_t ComputeScriptLen(Fragment fragment, Type sub0typ, size_t subsize, uint32_t k, size_t n_subs, size_t n_keys, MiniscriptContext ms_ctx);

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
    //! Note that this flag does not affect the satisfaction algorithm; it is only used for sanity checking.
    bool non_canon = false;
    //! Serialized witness size.
    size_t size = 0;
    //! Data elements.
    std::vector<std::vector<unsigned char>> stack;
    //! Construct an empty stack (valid).
    InputStack() = default;
    //! Construct a valid single-element stack (with an element up to 75 bytes).
    InputStack(std::vector<unsigned char> in) : size(in.size() + 1), stack(Vector(std::move(in))) {}
    //! Change availability
    InputStack& SetAvailable(Availability avail);
    //! Mark this input stack as having a signature.
    InputStack& SetWithSig();
    //! Mark this input stack as non-canonical (known to not be necessary in non-malleable satisfactions).
    InputStack& SetNonCanon();
    //! Mark this input stack as malleable.
    InputStack& SetMalleable(bool x = true);
    //! Concatenate two input stacks.
    friend InputStack operator+(InputStack a, InputStack b);
    //! Choose between two potential input stacks.
    friend InputStack operator|(InputStack a, InputStack b);
};

/** A stack consisting of a single zero-length element (interpreted as 0 by the script interpreter in numeric context). */
static const auto ZERO = InputStack(std::vector<unsigned char>());
/** A stack consisting of a single malleable 32-byte 0x0000...0000 element (for dissatisfying hash challenges). */
static const auto ZERO32 = InputStack(std::vector<unsigned char>(32, 0)).SetMalleable();
/** A stack consisting of a single 0x01 element (interpreted as 1 by the script interpreted in numeric context). */
static const auto ONE = InputStack(Vector((unsigned char)1));
/** The empty stack. */
static const auto EMPTY = InputStack();
/** A stack representing the lack of any (dis)satisfactions. */
static const auto INVALID = InputStack().SetAvailable(Availability::NO);

//! A pair of a satisfaction and a dissatisfaction InputStack.
struct InputResult {
    InputStack nsat, sat;

    template<typename A, typename B>
    InputResult(A&& in_nsat, B&& in_sat) : nsat(std::forward<A>(in_nsat)), sat(std::forward<B>(in_sat)) {}
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

    friend MaxInt<I> operator|(const MaxInt<I>& a, const MaxInt<I>& b) {
        if (!a.valid) return b;
        if (!b.valid) return a;
        return std::max(a.value, b.value);
    }
};

struct Ops {
    //! Non-push opcodes.
    uint32_t count;
    //! Number of keys in possibly executed OP_CHECKMULTISIG(VERIFY)s to satisfy.
    MaxInt<uint32_t> sat;
    //! Number of keys in possibly executed OP_CHECKMULTISIG(VERIFY)s to dissatisfy.
    MaxInt<uint32_t> dsat;

    Ops(uint32_t in_count, MaxInt<uint32_t> in_sat, MaxInt<uint32_t> in_dsat) : count(in_count), sat(in_sat), dsat(in_dsat) {};
};

/** A data structure to help the calculation of stack size limits.
 *
 * Conceptually, every SatInfo object corresponds to a (possibly empty) set of script execution
 * traces (sequences of opcodes).
 * - SatInfo{} corresponds to the empty set.
 * - SatInfo{n, e} corresponds to a single trace whose net effect is removing n elements from the
 *   stack (may be negative for a net increase), and reaches a maximum of e stack elements more
 *   than it ends with.
 * - operator| is the union operation: (a | b) corresponds to the union of the traces in a and the
 *   traces in b.
 * - operator+ is the concatenation operator: (a + b) corresponds to the set of traces formed by
 *   concatenating any trace in a with any trace in b.
 *
 * Its fields are:
 * - valid is true if the set is non-empty.
 * - netdiff (if valid) is the largest difference between stack size at the beginning and at the
 *   end of the script across all traces in the set.
 * - exec (if valid) is the largest difference between stack size anywhere during execution and at
 *   the end of the script, across all traces in the set (note that this is not necessarily due
 *   to the same trace as the one that resulted in the value for netdiff).
 *
 * This allows us to build up stack size limits for any script efficiently, by starting from the
 * individual opcodes miniscripts correspond to, using concatenation to construct scripts, and
 * using the union operation to choose between execution branches. Since any top-level script
 * satisfaction ends with a single stack element, we know that for a full script:
 * - netdiff+1 is the maximal initial stack size (relevant for P2WSH stack limits).
 * - exec+1 is the maximal stack size reached during execution (relevant for P2TR stack limits).
 *
 * Mathematically, SatInfo forms a semiring:
 * - operator| is the semiring addition operator, with identity SatInfo{}, and which is commutative
 *   and associative.
 * - operator+ is the semiring multiplication operator, with identity SatInfo{0}, and which is
 *   associative.
 * - operator+ is distributive over operator|, so (a + (b | c)) = (a+b | a+c). This means we do not
 *   need to actually materialize all possible full execution traces over the whole script (which
 *   may be exponential in the length of the script); instead we can use the union operation at the
 *   individual subexpression level, and concatenate the result with subexpressions before and
 *   after it.
 * - It is not a commutative semiring, because a+b can differ from b+a. For example, "OP_1 OP_DROP"
 *   has exec=1, while "OP_DROP OP_1" has exec=0.
 */
struct SatInfo {
    //! Whether a canonical satisfaction/dissatisfaction is possible at all.
    const bool valid;
    //! How much higher the stack size at start of execution can be compared to at the end.
    const int32_t netdiff;
    //! Mow much higher the stack size can be during execution compared to at the end.
    const int32_t exec;

    /** Empty script set. */
    constexpr SatInfo() noexcept : valid(false), netdiff(0), exec(0) {}

    /** Script set with a single script in it, with specified netdiff and exec. */
    constexpr SatInfo(int32_t in_netdiff, int32_t in_exec) noexcept :
        valid{true}, netdiff{in_netdiff}, exec{in_exec} {}

    /** Script set union. */
    constexpr friend SatInfo operator|(const SatInfo& a, const SatInfo& b) noexcept
    {
        // Union with an empty set is itself.
        if (!a.valid) return b;
        if (!b.valid) return a;
        // Otherwise the netdiff and exec of the union is the maximum of the individual values.
        return {std::max(a.netdiff, b.netdiff), std::max(a.exec, b.exec)};
    }

    /** Script set concatenation. */
    constexpr friend SatInfo operator+(const SatInfo& a, const SatInfo& b) noexcept
    {
        // Concatenation with an empty set yields an empty set.
        if (!a.valid || !b.valid) return {};
        // Otherwise, the maximum stack size difference for the combined scripts is the sum of the
        // netdiffs, and the maximum stack size difference anywhere is either b.exec (if the
        // maximum occurred in b) or b.netdiff+a.exec (if the maximum occurred in a).
        return {a.netdiff + b.netdiff, std::max(b.exec, b.netdiff + a.exec)};
    }

    /** The empty script. */
    static constexpr SatInfo Empty() noexcept { return {0, 0}; }
    /** A script consisting of a single push opcode. */
    static constexpr SatInfo Push() noexcept { return {-1, 0}; }
    /** A script consisting of a single hash opcode. */
    static constexpr SatInfo Hash() noexcept { return {0, 0}; }
    /** A script consisting of just a repurposed nop (OP_CHECKLOCKTIMEVERIFY, OP_CHECKSEQUENCEVERIFY). */
    static constexpr SatInfo Nop() noexcept { return {0, 0}; }
    /** A script consisting of just OP_IF or OP_NOTIF. Note that OP_ELSE and OP_ENDIF have no stack effect. */
    static constexpr SatInfo If() noexcept { return {1, 1}; }
    /** A script consisting of just a binary operator (OP_BOOLAND, OP_BOOLOR, OP_ADD). */
    static constexpr SatInfo BinaryOp() noexcept { return {1, 1}; }

    // Scripts for specific individual opcodes.
    static constexpr SatInfo OP_DUP() noexcept { return {-1, 0}; }
    static constexpr SatInfo OP_IFDUP(bool nonzero) noexcept { return {nonzero ? -1 : 0, 0}; }
    static constexpr SatInfo OP_EQUALVERIFY() noexcept { return {2, 2}; }
    static constexpr SatInfo OP_EQUAL() noexcept { return {1, 1}; }
    static constexpr SatInfo OP_SIZE() noexcept { return {-1, 0}; }
    static constexpr SatInfo OP_CHECKSIG() noexcept { return {1, 1}; }
    static constexpr SatInfo OP_0NOTEQUAL() noexcept { return {0, 0}; }
    static constexpr SatInfo OP_VERIFY() noexcept { return {1, 1}; }
};

struct StackSize {
    const SatInfo sat, dsat;

    constexpr StackSize(SatInfo in_sat, SatInfo in_dsat) noexcept : sat(in_sat), dsat(in_dsat) {};
    constexpr StackSize(SatInfo in_both) noexcept : sat(in_both), dsat(in_both) {};
};

struct WitnessSize {
    //! Maximum witness size to satisfy;
    MaxInt<uint32_t> sat;
    //! Maximum witness size to dissatisfy;
    MaxInt<uint32_t> dsat;

    WitnessSize(MaxInt<uint32_t> in_sat, MaxInt<uint32_t> in_dsat) : sat(in_sat), dsat(in_dsat) {};
};

struct NoDupCheck {};

} // namespace internal

//! A node in a miniscript expression.
template<typename Key>
struct Node {
    //! What node type this node is.
    const Fragment fragment;
    //! The k parameter (time for OLDER/AFTER, threshold for THRESH(_M))
    const uint32_t k = 0;
    //! The keys used by this expression (only for PK_K/PK_H/MULTI)
    const std::vector<Key> keys;
    //! The data bytes in this expression (only for HASH160/HASH256/SHA256/RIPEMD10).
    const std::vector<unsigned char> data;
    //! Subexpressions (for WRAP_*/AND_*/OR_*/ANDOR/THRESH)
    mutable std::vector<NodeRef<Key>> subs;
    //! The Script context for this node. Either P2WSH or Tapscript.
    const MiniscriptContext m_script_ctx;

    /* Destroy the shared pointers iteratively to avoid a stack-overflow due to recursive calls
     * to the subs' destructors. */
    ~Node() {
        while (!subs.empty()) {
            auto node = std::move(subs.back());
            subs.pop_back();
            while (!node->subs.empty()) {
                subs.push_back(std::move(node->subs.back()));
                node->subs.pop_back();
            }
        }
    }

private:
    //! Cached ops counts.
    const internal::Ops ops;
    //! Cached stack size bounds.
    const internal::StackSize ss;
    //! Cached witness size bounds.
    const internal::WitnessSize ws;
    //! Cached expression type (computed by CalcType and fed through SanitizeType).
    const Type typ;
    //! Cached script length (computed by CalcScriptLen).
    const size_t scriptlen;
    //! Whether a public key appears more than once in this node. This value is initialized
    //! by all constructors except the NoDupCheck ones. The NoDupCheck ones skip the
    //! computation, requiring it to be done manually by invoking DuplicateKeyCheck().
    //! DuplicateKeyCheck(), or a non-NoDupCheck constructor, will compute has_duplicate_keys
    //! for all subnodes as well.
    mutable std::optional<bool> has_duplicate_keys;


    //! Compute the length of the script for this miniscript (including children).
    size_t CalcScriptLen() const {
        size_t subsize = 0;
        for (const auto& sub : subs) {
            subsize += sub->ScriptSize();
        }
        static constexpr auto NONE_MST{""_mst};
        Type sub0type = subs.size() > 0 ? subs[0]->GetType() : NONE_MST;
        return internal::ComputeScriptLen(fragment, sub0type, subsize, k, subs.size(), keys.size(), m_script_ctx);
    }

    /* Apply a recursive algorithm to a Miniscript tree, without actual recursive calls.
     *
     * The algorithm is defined by two functions: downfn and upfn. Conceptually, the
     * result can be thought of as first using downfn to compute a "state" for each node,
     * from the root down to the leaves. Then upfn is used to compute a "result" for each
     * node, from the leaves back up to the root, which is then returned. In the actual
     * implementation, both functions are invoked in an interleaved fashion, performing a
     * depth-first traversal of the tree.
     *
     * In more detail, it is invoked as node.TreeEvalMaybe<Result>(root, downfn, upfn):
     * - root is the state of the root node, of type State.
     * - downfn is a callable (State&, const Node&, size_t) -> State, which given a
     *   node, its state, and an index of one of its children, computes the state of that
     *   child. It can modify the state. Children of a given node will have downfn()
     *   called in order.
     * - upfn is a callable (State&&, const Node&, Span<Result>) -> std::optional<Result>,
     *   which given a node, its state, and a Span of the results of its children,
     *   computes the result of the node. If std::nullopt is returned by upfn,
     *   TreeEvalMaybe() immediately returns std::nullopt.
     * The return value of TreeEvalMaybe is the result of the root node.
     *
     * Result type cannot be bool due to the std::vector<bool> specialization.
     */
    template<typename Result, typename State, typename DownFn, typename UpFn>
    std::optional<Result> TreeEvalMaybe(State root_state, DownFn downfn, UpFn upfn) const
    {
        /** Entries of the explicit stack tracked in this algorithm. */
        struct StackElem
        {
            const Node& node; //!< The node being evaluated.
            size_t expanded; //!< How many children of this node have been expanded.
            State state; //!< The state for that node.

            StackElem(const Node& node_, size_t exp_, State&& state_) :
                node(node_), expanded(exp_), state(std::move(state_)) {}
        };
        /* Stack of tree nodes being explored. */
        std::vector<StackElem> stack;
        /* Results of subtrees so far. Their order and mapping to tree nodes
         * is implicitly defined by stack. */
        std::vector<Result> results;
        stack.emplace_back(*this, 0, std::move(root_state));

        /* Here is a demonstration of the algorithm, for an example tree A(B,C(D,E),F).
         * State variables are omitted for simplicity.
         *
         * First: stack=[(A,0)] results=[]
         *        stack=[(A,1),(B,0)] results=[]
         *        stack=[(A,1)] results=[B]
         *        stack=[(A,2),(C,0)] results=[B]
         *        stack=[(A,2),(C,1),(D,0)] results=[B]
         *        stack=[(A,2),(C,1)] results=[B,D]
         *        stack=[(A,2),(C,2),(E,0)] results=[B,D]
         *        stack=[(A,2),(C,2)] results=[B,D,E]
         *        stack=[(A,2)] results=[B,C]
         *        stack=[(A,3),(F,0)] results=[B,C]
         *        stack=[(A,3)] results=[B,C,F]
         * Final: stack=[] results=[A]
         */
        while (stack.size()) {
            const Node& node = stack.back().node;
            if (stack.back().expanded < node.subs.size()) {
                /* We encounter a tree node with at least one unexpanded child.
                 * Expand it. By the time we hit this node again, the result of
                 * that child (and all earlier children) will be at the end of `results`. */
                size_t child_index = stack.back().expanded++;
                State child_state = downfn(stack.back().state, node, child_index);
                stack.emplace_back(*node.subs[child_index], 0, std::move(child_state));
                continue;
            }
            // Invoke upfn with the last node.subs.size() elements of results as input.
            assert(results.size() >= node.subs.size());
            std::optional<Result> result{upfn(std::move(stack.back().state), node,
                Span<Result>{results}.last(node.subs.size()))};
            // If evaluation returns std::nullopt, abort immediately.
            if (!result) return {};
            // Replace the last node.subs.size() elements of results with the new result.
            results.erase(results.end() - node.subs.size(), results.end());
            results.push_back(std::move(*result));
            stack.pop_back();
        }
        // The final remaining results element is the root result, return it.
        assert(results.size() == 1);
        return std::move(results[0]);
    }

    /** Like TreeEvalMaybe, but without downfn or State type.
     * upfn takes (const Node&, Span<Result>) and returns std::optional<Result>. */
    template<typename Result, typename UpFn>
    std::optional<Result> TreeEvalMaybe(UpFn upfn) const
    {
        struct DummyState {};
        return TreeEvalMaybe<Result>(DummyState{},
            [](DummyState, const Node&, size_t) { return DummyState{}; },
            [&upfn](DummyState, const Node& node, Span<Result> subs) {
                return upfn(node, subs);
            }
        );
    }

    /** Like TreeEvalMaybe, but always produces a result. upfn must return Result. */
    template<typename Result, typename State, typename DownFn, typename UpFn>
    Result TreeEval(State root_state, DownFn&& downfn, UpFn upfn) const
    {
        // Invoke TreeEvalMaybe with upfn wrapped to return std::optional<Result>, and then
        // unconditionally dereference the result (it cannot be std::nullopt).
        return std::move(*TreeEvalMaybe<Result>(std::move(root_state),
            std::forward<DownFn>(downfn),
            [&upfn](State&& state, const Node& node, Span<Result> subs) {
                Result res{upfn(std::move(state), node, subs)};
                return std::optional<Result>(std::move(res));
            }
        ));
    }

    /** Like TreeEval, but without downfn or State type.
     *  upfn takes (const Node&, Span<Result>) and returns Result. */
    template<typename Result, typename UpFn>
    Result TreeEval(UpFn upfn) const
    {
        struct DummyState {};
        return std::move(*TreeEvalMaybe<Result>(DummyState{},
            [](DummyState, const Node&, size_t) { return DummyState{}; },
            [&upfn](DummyState, const Node& node, Span<Result> subs) {
                Result res{upfn(node, subs)};
                return std::optional<Result>(std::move(res));
            }
        ));
    }

    /** Compare two miniscript subtrees, using a non-recursive algorithm. */
    friend int Compare(const Node<Key>& node1, const Node<Key>& node2)
    {
        std::vector<std::pair<const Node<Key>&, const Node<Key>&>> queue;
        queue.emplace_back(node1, node2);
        while (!queue.empty()) {
            const auto& [a, b] = queue.back();
            queue.pop_back();
            if (std::tie(a.fragment, a.k, a.keys, a.data) < std::tie(b.fragment, b.k, b.keys, b.data)) return -1;
            if (std::tie(b.fragment, b.k, b.keys, b.data) < std::tie(a.fragment, a.k, a.keys, a.data)) return 1;
            if (a.subs.size() < b.subs.size()) return -1;
            if (b.subs.size() < a.subs.size()) return 1;
            size_t n = a.subs.size();
            for (size_t i = 0; i < n; ++i) {
                queue.emplace_back(*a.subs[n - 1 - i], *b.subs[n - 1 - i]);
            }
        }
        return 0;
    }

    //! Compute the type for this miniscript.
    Type CalcType() const {
        using namespace internal;

        // THRESH has a variable number of subexpressions
        std::vector<Type> sub_types;
        if (fragment == Fragment::THRESH) {
            for (const auto& sub : subs) sub_types.push_back(sub->GetType());
        }
        // All other nodes than THRESH can be computed just from the types of the 0-3 subexpressions.
        static constexpr auto NONE_MST{""_mst};
        Type x = subs.size() > 0 ? subs[0]->GetType() : NONE_MST;
        Type y = subs.size() > 1 ? subs[1]->GetType() : NONE_MST;
        Type z = subs.size() > 2 ? subs[2]->GetType() : NONE_MST;

        return SanitizeType(ComputeType(fragment, x, y, z, sub_types, k, data.size(), subs.size(), keys.size(), m_script_ctx));
    }

public:
    template<typename Ctx>
    CScript ToScript(const Ctx& ctx) const
    {
        // To construct the CScript for a Miniscript object, we use the TreeEval algorithm.
        // The State is a boolean: whether or not the node's script expansion is followed
        // by an OP_VERIFY (which may need to be combined with the last script opcode).
        auto downfn = [](bool verify, const Node& node, size_t index) {
            // For WRAP_V, the subexpression is certainly followed by OP_VERIFY.
            if (node.fragment == Fragment::WRAP_V) return true;
            // The subexpression of WRAP_S, and the last subexpression of AND_V
            // inherit the followed-by-OP_VERIFY property from the parent.
            if (node.fragment == Fragment::WRAP_S ||
                (node.fragment == Fragment::AND_V && index == 1)) return verify;
            return false;
        };
        // The upward function computes for a node, given its followed-by-OP_VERIFY status
        // and the CScripts of its child nodes, the CScript of the node.
        const bool is_tapscript{IsTapscript(m_script_ctx)};
        auto upfn = [&ctx, is_tapscript](bool verify, const Node& node, Span<CScript> subs) -> CScript {
            switch (node.fragment) {
                case Fragment::PK_K: return BuildScript(ctx.ToPKBytes(node.keys[0]));
                case Fragment::PK_H: return BuildScript(OP_DUP, OP_HASH160, ctx.ToPKHBytes(node.keys[0]), OP_EQUALVERIFY);
                case Fragment::OLDER: return BuildScript(node.k, OP_CHECKSEQUENCEVERIFY);
                case Fragment::AFTER: return BuildScript(node.k, OP_CHECKLOCKTIMEVERIFY);
                case Fragment::SHA256: return BuildScript(OP_SIZE, 32, OP_EQUALVERIFY, OP_SHA256, node.data, verify ? OP_EQUALVERIFY : OP_EQUAL);
                case Fragment::RIPEMD160: return BuildScript(OP_SIZE, 32, OP_EQUALVERIFY, OP_RIPEMD160, node.data, verify ? OP_EQUALVERIFY : OP_EQUAL);
                case Fragment::HASH256: return BuildScript(OP_SIZE, 32, OP_EQUALVERIFY, OP_HASH256, node.data, verify ? OP_EQUALVERIFY : OP_EQUAL);
                case Fragment::HASH160: return BuildScript(OP_SIZE, 32, OP_EQUALVERIFY, OP_HASH160, node.data, verify ? OP_EQUALVERIFY : OP_EQUAL);
                case Fragment::WRAP_A: return BuildScript(OP_TOALTSTACK, subs[0], OP_FROMALTSTACK);
                case Fragment::WRAP_S: return BuildScript(OP_SWAP, subs[0]);
                case Fragment::WRAP_C: return BuildScript(std::move(subs[0]), verify ? OP_CHECKSIGVERIFY : OP_CHECKSIG);
                case Fragment::WRAP_D: return BuildScript(OP_DUP, OP_IF, subs[0], OP_ENDIF);
                case Fragment::WRAP_V: {
                    if (node.subs[0]->GetType() << "x"_mst) {
                        return BuildScript(std::move(subs[0]), OP_VERIFY);
                    } else {
                        return std::move(subs[0]);
                    }
                }
                case Fragment::WRAP_J: return BuildScript(OP_SIZE, OP_0NOTEQUAL, OP_IF, subs[0], OP_ENDIF);
                case Fragment::WRAP_N: return BuildScript(std::move(subs[0]), OP_0NOTEQUAL);
                case Fragment::JUST_1: return BuildScript(OP_1);
                case Fragment::JUST_0: return BuildScript(OP_0);
                case Fragment::AND_V: return BuildScript(std::move(subs[0]), subs[1]);
                case Fragment::AND_B: return BuildScript(std::move(subs[0]), subs[1], OP_BOOLAND);
                case Fragment::OR_B: return BuildScript(std::move(subs[0]), subs[1], OP_BOOLOR);
                case Fragment::OR_D: return BuildScript(std::move(subs[0]), OP_IFDUP, OP_NOTIF, subs[1], OP_ENDIF);
                case Fragment::OR_C: return BuildScript(std::move(subs[0]), OP_NOTIF, subs[1], OP_ENDIF);
                case Fragment::OR_I: return BuildScript(OP_IF, subs[0], OP_ELSE, subs[1], OP_ENDIF);
                case Fragment::ANDOR: return BuildScript(std::move(subs[0]), OP_NOTIF, subs[2], OP_ELSE, subs[1], OP_ENDIF);
                case Fragment::MULTI: {
                    CHECK_NONFATAL(!is_tapscript);
                    CScript script = BuildScript(node.k);
                    for (const auto& key : node.keys) {
                        script = BuildScript(std::move(script), ctx.ToPKBytes(key));
                    }
                    return BuildScript(std::move(script), node.keys.size(), verify ? OP_CHECKMULTISIGVERIFY : OP_CHECKMULTISIG);
                }
                case Fragment::MULTI_A: {
                    CHECK_NONFATAL(is_tapscript);
                    CScript script = BuildScript(ctx.ToPKBytes(*node.keys.begin()), OP_CHECKSIG);
                    for (auto it = node.keys.begin() + 1; it != node.keys.end(); ++it) {
                        script = BuildScript(std::move(script), ctx.ToPKBytes(*it), OP_CHECKSIGADD);
                    }
                    return BuildScript(std::move(script), node.k, verify ? OP_NUMEQUALVERIFY : OP_NUMEQUAL);
                }
                case Fragment::THRESH: {
                    CScript script = std::move(subs[0]);
                    for (size_t i = 1; i < subs.size(); ++i) {
                        script = BuildScript(std::move(script), subs[i], OP_ADD);
                    }
                    return BuildScript(std::move(script), node.k, verify ? OP_EQUALVERIFY : OP_EQUAL);
                }
            }
            assert(false);
        };
        return TreeEval<CScript>(false, downfn, upfn);
    }

    template<typename CTx>
    std::optional<std::string> ToString(const CTx& ctx) const {
        // To construct the std::string representation for a Miniscript object, we use
        // the TreeEvalMaybe algorithm. The State is a boolean: whether the parent node is a
        // wrapper. If so, non-wrapper expressions must be prefixed with a ":".
        auto downfn = [](bool, const Node& node, size_t) {
            return (node.fragment == Fragment::WRAP_A || node.fragment == Fragment::WRAP_S ||
                    node.fragment == Fragment::WRAP_D || node.fragment == Fragment::WRAP_V ||
                    node.fragment == Fragment::WRAP_J || node.fragment == Fragment::WRAP_N ||
                    node.fragment == Fragment::WRAP_C ||
                    (node.fragment == Fragment::AND_V && node.subs[1]->fragment == Fragment::JUST_1) ||
                    (node.fragment == Fragment::OR_I && node.subs[0]->fragment == Fragment::JUST_0) ||
                    (node.fragment == Fragment::OR_I && node.subs[1]->fragment == Fragment::JUST_0));
        };
        // The upward function computes for a node, given whether its parent is a wrapper,
        // and the string representations of its child nodes, the string representation of the node.
        const bool is_tapscript{IsTapscript(m_script_ctx)};
        auto upfn = [&ctx, is_tapscript](bool wrapped, const Node& node, Span<std::string> subs) -> std::optional<std::string> {
            std::string ret = wrapped ? ":" : "";

            switch (node.fragment) {
                case Fragment::WRAP_A: return "a" + std::move(subs[0]);
                case Fragment::WRAP_S: return "s" + std::move(subs[0]);
                case Fragment::WRAP_C:
                    if (node.subs[0]->fragment == Fragment::PK_K) {
                        // pk(K) is syntactic sugar for c:pk_k(K)
                        auto key_str = ctx.ToString(node.subs[0]->keys[0]);
                        if (!key_str) return {};
                        return std::move(ret) + "pk(" + std::move(*key_str) + ")";
                    }
                    if (node.subs[0]->fragment == Fragment::PK_H) {
                        // pkh(K) is syntactic sugar for c:pk_h(K)
                        auto key_str = ctx.ToString(node.subs[0]->keys[0]);
                        if (!key_str) return {};
                        return std::move(ret) + "pkh(" + std::move(*key_str) + ")";
                    }
                    return "c" + std::move(subs[0]);
                case Fragment::WRAP_D: return "d" + std::move(subs[0]);
                case Fragment::WRAP_V: return "v" + std::move(subs[0]);
                case Fragment::WRAP_J: return "j" + std::move(subs[0]);
                case Fragment::WRAP_N: return "n" + std::move(subs[0]);
                case Fragment::AND_V:
                    // t:X is syntactic sugar for and_v(X,1).
                    if (node.subs[1]->fragment == Fragment::JUST_1) return "t" + std::move(subs[0]);
                    break;
                case Fragment::OR_I:
                    if (node.subs[0]->fragment == Fragment::JUST_0) return "l" + std::move(subs[1]);
                    if (node.subs[1]->fragment == Fragment::JUST_0) return "u" + std::move(subs[0]);
                    break;
                default: break;
            }
            switch (node.fragment) {
                case Fragment::PK_K: {
                    auto key_str = ctx.ToString(node.keys[0]);
                    if (!key_str) return {};
                    return std::move(ret) + "pk_k(" + std::move(*key_str) + ")";
                }
                case Fragment::PK_H: {
                    auto key_str = ctx.ToString(node.keys[0]);
                    if (!key_str) return {};
                    return std::move(ret) + "pk_h(" + std::move(*key_str) + ")";
                }
                case Fragment::AFTER: return std::move(ret) + "after(" + util::ToString(node.k) + ")";
                case Fragment::OLDER: return std::move(ret) + "older(" + util::ToString(node.k) + ")";
                case Fragment::HASH256: return std::move(ret) + "hash256(" + HexStr(node.data) + ")";
                case Fragment::HASH160: return std::move(ret) + "hash160(" + HexStr(node.data) + ")";
                case Fragment::SHA256: return std::move(ret) + "sha256(" + HexStr(node.data) + ")";
                case Fragment::RIPEMD160: return std::move(ret) + "ripemd160(" + HexStr(node.data) + ")";
                case Fragment::JUST_1: return std::move(ret) + "1";
                case Fragment::JUST_0: return std::move(ret) + "0";
                case Fragment::AND_V: return std::move(ret) + "and_v(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::AND_B: return std::move(ret) + "and_b(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::OR_B: return std::move(ret) + "or_b(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::OR_D: return std::move(ret) + "or_d(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::OR_C: return std::move(ret) + "or_c(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::OR_I: return std::move(ret) + "or_i(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::ANDOR:
                    // and_n(X,Y) is syntactic sugar for andor(X,Y,0).
                    if (node.subs[2]->fragment == Fragment::JUST_0) return std::move(ret) + "and_n(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                    return std::move(ret) + "andor(" + std::move(subs[0]) + "," + std::move(subs[1]) + "," + std::move(subs[2]) + ")";
                case Fragment::MULTI: {
                    CHECK_NONFATAL(!is_tapscript);
                    auto str = std::move(ret) + "multi(" + util::ToString(node.k);
                    for (const auto& key : node.keys) {
                        auto key_str = ctx.ToString(key);
                        if (!key_str) return {};
                        str += "," + std::move(*key_str);
                    }
                    return std::move(str) + ")";
                }
                case Fragment::MULTI_A: {
                    CHECK_NONFATAL(is_tapscript);
                    auto str = std::move(ret) + "multi_a(" + util::ToString(node.k);
                    for (const auto& key : node.keys) {
                        auto key_str = ctx.ToString(key);
                        if (!key_str) return {};
                        str += "," + std::move(*key_str);
                    }
                    return std::move(str) + ")";
                }
                case Fragment::THRESH: {
                    auto str = std::move(ret) + "thresh(" + util::ToString(node.k);
                    for (auto& sub : subs) {
                        str += "," + std::move(sub);
                    }
                    return std::move(str) + ")";
                }
                default: break;
            }
            assert(false);
        };

        return TreeEvalMaybe<std::string>(false, downfn, upfn);
    }

private:
    internal::Ops CalcOps() const {
        switch (fragment) {
            case Fragment::JUST_1: return {0, 0, {}};
            case Fragment::JUST_0: return {0, {}, 0};
            case Fragment::PK_K: return {0, 0, 0};
            case Fragment::PK_H: return {3, 0, 0};
            case Fragment::OLDER:
            case Fragment::AFTER: return {1, 0, {}};
            case Fragment::SHA256:
            case Fragment::RIPEMD160:
            case Fragment::HASH256:
            case Fragment::HASH160: return {4, 0, {}};
            case Fragment::AND_V: return {subs[0]->ops.count + subs[1]->ops.count, subs[0]->ops.sat + subs[1]->ops.sat, {}};
            case Fragment::AND_B: {
                const auto count{1 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{subs[0]->ops.sat + subs[1]->ops.sat};
                const auto dsat{subs[0]->ops.dsat + subs[1]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::OR_B: {
                const auto count{1 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{(subs[0]->ops.sat + subs[1]->ops.dsat) | (subs[1]->ops.sat + subs[0]->ops.dsat)};
                const auto dsat{subs[0]->ops.dsat + subs[1]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::OR_D: {
                const auto count{3 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{subs[0]->ops.sat | (subs[1]->ops.sat + subs[0]->ops.dsat)};
                const auto dsat{subs[0]->ops.dsat + subs[1]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::OR_C: {
                const auto count{2 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{subs[0]->ops.sat | (subs[1]->ops.sat + subs[0]->ops.dsat)};
                return {count, sat, {}};
            }
            case Fragment::OR_I: {
                const auto count{3 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{subs[0]->ops.sat | subs[1]->ops.sat};
                const auto dsat{subs[0]->ops.dsat | subs[1]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::ANDOR: {
                const auto count{3 + subs[0]->ops.count + subs[1]->ops.count + subs[2]->ops.count};
                const auto sat{(subs[1]->ops.sat + subs[0]->ops.sat) | (subs[0]->ops.dsat + subs[2]->ops.sat)};
                const auto dsat{subs[0]->ops.dsat + subs[2]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::MULTI: return {1, (uint32_t)keys.size(), (uint32_t)keys.size()};
            case Fragment::MULTI_A: return {(uint32_t)keys.size() + 1, 0, 0};
            case Fragment::WRAP_S:
            case Fragment::WRAP_C:
            case Fragment::WRAP_N: return {1 + subs[0]->ops.count, subs[0]->ops.sat, subs[0]->ops.dsat};
            case Fragment::WRAP_A: return {2 + subs[0]->ops.count, subs[0]->ops.sat, subs[0]->ops.dsat};
            case Fragment::WRAP_D: return {3 + subs[0]->ops.count, subs[0]->ops.sat, 0};
            case Fragment::WRAP_J: return {4 + subs[0]->ops.count, subs[0]->ops.sat, 0};
            case Fragment::WRAP_V: return {subs[0]->ops.count + (subs[0]->GetType() << "x"_mst), subs[0]->ops.sat, {}};
            case Fragment::THRESH: {
                uint32_t count = 0;
                auto sats = Vector(internal::MaxInt<uint32_t>(0));
                for (const auto& sub : subs) {
                    count += sub->ops.count + 1;
                    auto next_sats = Vector(sats[0] + sub->ops.dsat);
                    for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back((sats[j] + sub->ops.dsat) | (sats[j - 1] + sub->ops.sat));
                    next_sats.push_back(sats[sats.size() - 1] + sub->ops.sat);
                    sats = std::move(next_sats);
                }
                assert(k <= sats.size());
                return {count, sats[k], sats[0]};
            }
        }
        assert(false);
    }

    internal::StackSize CalcStackSize() const {
        using namespace internal;
        switch (fragment) {
            case Fragment::JUST_0: return {{}, SatInfo::Push()};
            case Fragment::JUST_1: return {SatInfo::Push(), {}};
            case Fragment::OLDER:
            case Fragment::AFTER: return {SatInfo::Push() + SatInfo::Nop(), {}};
            case Fragment::PK_K: return {SatInfo::Push()};
            case Fragment::PK_H: return {SatInfo::OP_DUP() + SatInfo::Hash() + SatInfo::Push() + SatInfo::OP_EQUALVERIFY()};
            case Fragment::SHA256:
            case Fragment::RIPEMD160:
            case Fragment::HASH256:
            case Fragment::HASH160: return {
                SatInfo::OP_SIZE() + SatInfo::Push() + SatInfo::OP_EQUALVERIFY() + SatInfo::Hash() + SatInfo::Push() + SatInfo::OP_EQUAL(),
                {}
            };
            case Fragment::ANDOR: {
                const auto& x{subs[0]->ss};
                const auto& y{subs[1]->ss};
                const auto& z{subs[2]->ss};
                return {
                    (x.sat + SatInfo::If() + y.sat) | (x.dsat + SatInfo::If() + z.sat),
                    x.dsat + SatInfo::If() + z.dsat
                };
            }
            case Fragment::AND_V: {
                const auto& x{subs[0]->ss};
                const auto& y{subs[1]->ss};
                return {x.sat + y.sat, {}};
            }
            case Fragment::AND_B: {
                const auto& x{subs[0]->ss};
                const auto& y{subs[1]->ss};
                return {x.sat + y.sat + SatInfo::BinaryOp(), x.dsat + y.dsat + SatInfo::BinaryOp()};
            }
            case Fragment::OR_B: {
                const auto& x{subs[0]->ss};
                const auto& y{subs[1]->ss};
                return {
                    ((x.sat + y.dsat) | (x.dsat + y.sat)) + SatInfo::BinaryOp(),
                    x.dsat + y.dsat + SatInfo::BinaryOp()
                };
            }
            case Fragment::OR_C: {
                const auto& x{subs[0]->ss};
                const auto& y{subs[1]->ss};
                return {(x.sat + SatInfo::If()) | (x.dsat + SatInfo::If() + y.sat), {}};
            }
            case Fragment::OR_D: {
                const auto& x{subs[0]->ss};
                const auto& y{subs[1]->ss};
                return {
                    (x.sat + SatInfo::OP_IFDUP(true) + SatInfo::If()) | (x.dsat + SatInfo::OP_IFDUP(false) + SatInfo::If() + y.sat),
                    x.dsat + SatInfo::OP_IFDUP(false) + SatInfo::If() + y.dsat
                };
            }
            case Fragment::OR_I: {
                const auto& x{subs[0]->ss};
                const auto& y{subs[1]->ss};
                return {SatInfo::If() + (x.sat | y.sat), SatInfo::If() + (x.dsat | y.dsat)};
            }
            // multi(k, key1, key2, ..., key_n) starts off with k+1 stack elements (a 0, plus k
            // signatures), then reaches n+k+3 stack elements after pushing the n keys, plus k and
            // n itself, and ends with 1 stack element (success or failure). Thus, it net removes
            // k elements (from k+1 to 1), while reaching k+n+2 more than it ends with.
            case Fragment::MULTI: return {SatInfo(k, k + keys.size() + 2)};
            // multi_a(k, key1, key2, ..., key_n) starts off with n stack elements (the
            // signatures), reaches 1 more (after the first key push), and ends with 1. Thus it net
            // removes n-1 elements (from n to 1) while reaching n more than it ends with.
            case Fragment::MULTI_A: return {SatInfo(keys.size() - 1, keys.size())};
            case Fragment::WRAP_A:
            case Fragment::WRAP_N:
            case Fragment::WRAP_S: return subs[0]->ss;
            case Fragment::WRAP_C: return {
                subs[0]->ss.sat + SatInfo::OP_CHECKSIG(),
                subs[0]->ss.dsat + SatInfo::OP_CHECKSIG()
            };
            case Fragment::WRAP_D: return {
                SatInfo::OP_DUP() + SatInfo::If() + subs[0]->ss.sat,
                SatInfo::OP_DUP() + SatInfo::If()
            };
            case Fragment::WRAP_V: return {subs[0]->ss.sat + SatInfo::OP_VERIFY(), {}};
            case Fragment::WRAP_J: return {
                SatInfo::OP_SIZE() + SatInfo::OP_0NOTEQUAL() + SatInfo::If() + subs[0]->ss.sat,
                SatInfo::OP_SIZE() + SatInfo::OP_0NOTEQUAL() + SatInfo::If()
            };
            case Fragment::THRESH: {
                // sats[j] is the SatInfo corresponding to all traces reaching j satisfactions.
                auto sats = Vector(SatInfo::Empty());
                for (size_t i = 0; i < subs.size(); ++i) {
                    // Loop over the subexpressions, processing them one by one. After adding
                    // element i we need to add OP_ADD (if i>0).
                    auto add = i ? SatInfo::BinaryOp() : SatInfo::Empty();
                    // Construct a variable that will become the next sats, starting with index 0.
                    auto next_sats = Vector(sats[0] + subs[i]->ss.dsat + add);
                    // Then loop to construct next_sats[1..i].
                    for (size_t j = 1; j < sats.size(); ++j) {
                        next_sats.push_back(((sats[j] + subs[i]->ss.dsat) | (sats[j - 1] + subs[i]->ss.sat)) + add);
                    }
                    // Finally construct next_sats[i+1].
                    next_sats.push_back(sats[sats.size() - 1] + subs[i]->ss.sat + add);
                    // Switch over.
                    sats = std::move(next_sats);
                }
                // To satisfy thresh we need k satisfactions; to dissatisfy we need 0. In both
                // cases a push of k and an OP_EQUAL follow.
                return {
                    sats[k] + SatInfo::Push() + SatInfo::OP_EQUAL(),
                    sats[0] + SatInfo::Push() + SatInfo::OP_EQUAL()
                };
            }
        }
        assert(false);
    }

    internal::WitnessSize CalcWitnessSize() const {
        const uint32_t sig_size = IsTapscript(m_script_ctx) ? 1 + 65 : 1 + 72;
        const uint32_t pubkey_size = IsTapscript(m_script_ctx) ? 1 + 32 : 1 + 33;
        switch (fragment) {
            case Fragment::JUST_0: return {{}, 0};
            case Fragment::JUST_1:
            case Fragment::OLDER:
            case Fragment::AFTER: return {0, {}};
            case Fragment::PK_K: return {sig_size, 1};
            case Fragment::PK_H: return {sig_size + pubkey_size, 1 + pubkey_size};
            case Fragment::SHA256:
            case Fragment::RIPEMD160:
            case Fragment::HASH256:
            case Fragment::HASH160: return {1 + 32, {}};
            case Fragment::ANDOR: {
                const auto sat{(subs[0]->ws.sat + subs[1]->ws.sat) | (subs[0]->ws.dsat + subs[2]->ws.sat)};
                const auto dsat{subs[0]->ws.dsat + subs[2]->ws.dsat};
                return {sat, dsat};
            }
            case Fragment::AND_V: return {subs[0]->ws.sat + subs[1]->ws.sat, {}};
            case Fragment::AND_B: return {subs[0]->ws.sat + subs[1]->ws.sat, subs[0]->ws.dsat + subs[1]->ws.dsat};
            case Fragment::OR_B: {
                const auto sat{(subs[0]->ws.dsat + subs[1]->ws.sat) | (subs[0]->ws.sat + subs[1]->ws.dsat)};
                const auto dsat{subs[0]->ws.dsat + subs[1]->ws.dsat};
                return {sat, dsat};
            }
            case Fragment::OR_C: return {subs[0]->ws.sat | (subs[0]->ws.dsat + subs[1]->ws.sat), {}};
            case Fragment::OR_D: return {subs[0]->ws.sat | (subs[0]->ws.dsat + subs[1]->ws.sat), subs[0]->ws.dsat + subs[1]->ws.dsat};
            case Fragment::OR_I: return {(subs[0]->ws.sat + 1 + 1) | (subs[1]->ws.sat + 1), (subs[0]->ws.dsat + 1 + 1) | (subs[1]->ws.dsat + 1)};
            case Fragment::MULTI: return {k * sig_size + 1, k + 1};
            case Fragment::MULTI_A: return {k * sig_size + static_cast<uint32_t>(keys.size()) - k, static_cast<uint32_t>(keys.size())};
            case Fragment::WRAP_A:
            case Fragment::WRAP_N:
            case Fragment::WRAP_S:
            case Fragment::WRAP_C: return subs[0]->ws;
            case Fragment::WRAP_D: return {1 + 1 + subs[0]->ws.sat, 1};
            case Fragment::WRAP_V: return {subs[0]->ws.sat, {}};
            case Fragment::WRAP_J: return {subs[0]->ws.sat, 1};
            case Fragment::THRESH: {
                auto sats = Vector(internal::MaxInt<uint32_t>(0));
                for (const auto& sub : subs) {
                    auto next_sats = Vector(sats[0] + sub->ws.dsat);
                    for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back((sats[j] + sub->ws.dsat) | (sats[j - 1] + sub->ws.sat));
                    next_sats.push_back(sats[sats.size() - 1] + sub->ws.sat);
                    sats = std::move(next_sats);
                }
                assert(k <= sats.size());
                return {sats[k], sats[0]};
            }
        }
        assert(false);
    }

    template<typename Ctx>
    internal::InputResult ProduceInput(const Ctx& ctx) const {
        using namespace internal;

        // Internal function which is invoked for every tree node, constructing satisfaction/dissatisfactions
        // given those of its subnodes.
        auto helper = [&ctx](const Node& node, Span<InputResult> subres) -> InputResult {
            switch (node.fragment) {
                case Fragment::PK_K: {
                    std::vector<unsigned char> sig;
                    Availability avail = ctx.Sign(node.keys[0], sig);
                    return {ZERO, InputStack(std::move(sig)).SetWithSig().SetAvailable(avail)};
                }
                case Fragment::PK_H: {
                    std::vector<unsigned char> key = ctx.ToPKBytes(node.keys[0]), sig;
                    Availability avail = ctx.Sign(node.keys[0], sig);
                    return {ZERO + InputStack(key), (InputStack(std::move(sig)).SetWithSig() + InputStack(key)).SetAvailable(avail)};
                }
                case Fragment::MULTI_A: {
                    // sats[j] represents the best stack containing j valid signatures (out of the first i keys).
                    // In the loop below, these stacks are built up using a dynamic programming approach.
                    std::vector<InputStack> sats = Vector(EMPTY);
                    for (size_t i = 0; i < node.keys.size(); ++i) {
                        // Get the signature for the i'th key in reverse order (the signature for the first key needs to
                        // be at the top of the stack, contrary to CHECKMULTISIG's satisfaction).
                        std::vector<unsigned char> sig;
                        Availability avail = ctx.Sign(node.keys[node.keys.size() - 1 - i], sig);
                        // Compute signature stack for just this key.
                        auto sat = InputStack(std::move(sig)).SetWithSig().SetAvailable(avail);
                        // Compute the next sats vector: next_sats[0] is a copy of sats[0] (no signatures). All further
                        // next_sats[j] are equal to either the existing sats[j] + ZERO, or sats[j-1] plus a signature
                        // for the current (i'th) key. The very last element needs all signatures filled.
                        std::vector<InputStack> next_sats;
                        next_sats.push_back(sats[0] + ZERO);
                        for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back((sats[j] + ZERO) | (std::move(sats[j - 1]) + sat));
                        next_sats.push_back(std::move(sats[sats.size() - 1]) + std::move(sat));
                        // Switch over.
                        sats = std::move(next_sats);
                    }
                    // The dissatisfaction consists of as many empty vectors as there are keys, which is the same as
                    // satisfying 0 keys.
                    auto& nsat{sats[0]};
                    assert(node.k != 0);
                    assert(node.k <= sats.size());
                    return {std::move(nsat), std::move(sats[node.k])};
                }
                case Fragment::MULTI: {
                    // sats[j] represents the best stack containing j valid signatures (out of the first i keys).
                    // In the loop below, these stacks are built up using a dynamic programming approach.
                    // sats[0] starts off being {0}, due to the CHECKMULTISIG bug that pops off one element too many.
                    std::vector<InputStack> sats = Vector(ZERO);
                    for (size_t i = 0; i < node.keys.size(); ++i) {
                        std::vector<unsigned char> sig;
                        Availability avail = ctx.Sign(node.keys[i], sig);
                        // Compute signature stack for just the i'th key.
                        auto sat = InputStack(std::move(sig)).SetWithSig().SetAvailable(avail);
                        // Compute the next sats vector: next_sats[0] is a copy of sats[0] (no signatures). All further
                        // next_sats[j] are equal to either the existing sats[j], or sats[j-1] plus a signature for the
                        // current (i'th) key. The very last element needs all signatures filled.
                        std::vector<InputStack> next_sats;
                        next_sats.push_back(sats[0]);
                        for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back(sats[j] | (std::move(sats[j - 1]) + sat));
                        next_sats.push_back(std::move(sats[sats.size() - 1]) + std::move(sat));
                        // Switch over.
                        sats = std::move(next_sats);
                    }
                    // The dissatisfaction consists of k+1 stack elements all equal to 0.
                    InputStack nsat = ZERO;
                    for (size_t i = 0; i < node.k; ++i) nsat = std::move(nsat) + ZERO;
                    assert(node.k <= sats.size());
                    return {std::move(nsat), std::move(sats[node.k])};
                }
                case Fragment::THRESH: {
                    // sats[k] represents the best stack that satisfies k out of the *last* i subexpressions.
                    // In the loop below, these stacks are built up using a dynamic programming approach.
                    // sats[0] starts off empty.
                    std::vector<InputStack> sats = Vector(EMPTY);
                    for (size_t i = 0; i < subres.size(); ++i) {
                        // Introduce an alias for the i'th last satisfaction/dissatisfaction.
                        auto& res = subres[subres.size() - i - 1];
                        // Compute the next sats vector: next_sats[0] is sats[0] plus res.nsat (thus containing all dissatisfactions
                        // so far. next_sats[j] is either sats[j] + res.nsat (reusing j earlier satisfactions) or sats[j-1] + res.sat
                        // (reusing j-1 earlier satisfactions plus a new one). The very last next_sats[j] is all satisfactions.
                        std::vector<InputStack> next_sats;
                        next_sats.push_back(sats[0] + res.nsat);
                        for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back((sats[j] + res.nsat) | (std::move(sats[j - 1]) + res.sat));
                        next_sats.push_back(std::move(sats[sats.size() - 1]) + std::move(res.sat));
                        // Switch over.
                        sats = std::move(next_sats);
                    }
                    // At this point, sats[k].sat is the best satisfaction for the overall thresh() node. The best dissatisfaction
                    // is computed by gathering all sats[i].nsat for i != k.
                    InputStack nsat = INVALID;
                    for (size_t i = 0; i < sats.size(); ++i) {
                        // i==k is the satisfaction; i==0 is the canonical dissatisfaction;
                        // the rest are non-canonical (a no-signature dissatisfaction - the i=0
                        // form - is always available) and malleable (due to overcompleteness).
                        // Marking the solutions malleable here is not strictly necessary, as they
                        // should already never be picked in non-malleable solutions due to the
                        // availability of the i=0 form.
                        if (i != 0 && i != node.k) sats[i].SetMalleable().SetNonCanon();
                        // Include all dissatisfactions (even these non-canonical ones) in nsat.
                        if (i != node.k) nsat = std::move(nsat) | std::move(sats[i]);
                    }
                    assert(node.k <= sats.size());
                    return {std::move(nsat), std::move(sats[node.k])};
                }
                case Fragment::OLDER: {
                    return {INVALID, ctx.CheckOlder(node.k) ? EMPTY : INVALID};
                }
                case Fragment::AFTER: {
                    return {INVALID, ctx.CheckAfter(node.k) ? EMPTY : INVALID};
                }
                case Fragment::SHA256: {
                    std::vector<unsigned char> preimage;
                    Availability avail = ctx.SatSHA256(node.data, preimage);
                    return {ZERO32, InputStack(std::move(preimage)).SetAvailable(avail)};
                }
                case Fragment::RIPEMD160: {
                    std::vector<unsigned char> preimage;
                    Availability avail = ctx.SatRIPEMD160(node.data, preimage);
                    return {ZERO32, InputStack(std::move(preimage)).SetAvailable(avail)};
                }
                case Fragment::HASH256: {
                    std::vector<unsigned char> preimage;
                    Availability avail = ctx.SatHASH256(node.data, preimage);
                    return {ZERO32, InputStack(std::move(preimage)).SetAvailable(avail)};
                }
                case Fragment::HASH160: {
                    std::vector<unsigned char> preimage;
                    Availability avail = ctx.SatHASH160(node.data, preimage);
                    return {ZERO32, InputStack(std::move(preimage)).SetAvailable(avail)};
                }
                case Fragment::AND_V: {
                    auto& x = subres[0], &y = subres[1];
                    // As the dissatisfaction here only consist of a single option, it doesn't
                    // actually need to be listed (it's not required for reasoning about malleability of
                    // other options), and is never required (no valid miniscript relies on the ability
                    // to satisfy the type V left subexpression). It's still listed here for
                    // completeness, as a hypothetical (not currently implemented) satisfier that doesn't
                    // care about malleability might in some cases prefer it still.
                    return {(y.nsat + x.sat).SetNonCanon(), y.sat + x.sat};
                }
                case Fragment::AND_B: {
                    auto& x = subres[0], &y = subres[1];
                    // Note that it is not strictly necessary to mark the 2nd and 3rd dissatisfaction here
                    // as malleable. While they are definitely malleable, they are also non-canonical due
                    // to the guaranteed existence of a no-signature other dissatisfaction (the 1st)
                    // option. Because of that, the 2nd and 3rd option will never be chosen, even if they
                    // weren't marked as malleable.
                    return {(y.nsat + x.nsat) | (y.sat + x.nsat).SetMalleable().SetNonCanon() | (y.nsat + x.sat).SetMalleable().SetNonCanon(), y.sat + x.sat};
                }
                case Fragment::OR_B: {
                    auto& x = subres[0], &z = subres[1];
                    // The (sat(Z) sat(X)) solution is overcomplete (attacker can change either into dsat).
                    return {z.nsat + x.nsat, (z.nsat + x.sat) | (z.sat + x.nsat) | (z.sat + x.sat).SetMalleable().SetNonCanon()};
                }
                case Fragment::OR_C: {
                    auto& x = subres[0], &z = subres[1];
                    return {INVALID, std::move(x.sat) | (z.sat + x.nsat)};
                }
                case Fragment::OR_D: {
                    auto& x = subres[0], &z = subres[1];
                    return {z.nsat + x.nsat, std::move(x.sat) | (z.sat + x.nsat)};
                }
                case Fragment::OR_I: {
                    auto& x = subres[0], &z = subres[1];
                    return {(x.nsat + ONE) | (z.nsat + ZERO), (x.sat + ONE) | (z.sat + ZERO)};
                }
                case Fragment::ANDOR: {
                    auto& x = subres[0], &y = subres[1], &z = subres[2];
                    return {(y.nsat + x.sat).SetNonCanon() | (z.nsat + x.nsat), (y.sat + x.sat) | (z.sat + x.nsat)};
                }
                case Fragment::WRAP_A:
                case Fragment::WRAP_S:
                case Fragment::WRAP_C:
                case Fragment::WRAP_N:
                    return std::move(subres[0]);
                case Fragment::WRAP_D: {
                    auto &x = subres[0];
                    return {ZERO, x.sat + ONE};
                }
                case Fragment::WRAP_J: {
                    auto &x = subres[0];
                    // If a dissatisfaction with a nonzero top stack element exists, an alternative dissatisfaction exists.
                    // As the dissatisfaction logic currently doesn't keep track of this nonzeroness property, and thus even
                    // if a dissatisfaction with a top zero element is found, we don't know whether another one with a
                    // nonzero top stack element exists. Make the conservative assumption that whenever the subexpression is weakly
                    // dissatisfiable, this alternative dissatisfaction exists and leads to malleability.
                    return {InputStack(ZERO).SetMalleable(x.nsat.available != Availability::NO && !x.nsat.has_sig), std::move(x.sat)};
                }
                case Fragment::WRAP_V: {
                    auto &x = subres[0];
                    return {INVALID, std::move(x.sat)};
                }
                case Fragment::JUST_0: return {EMPTY, INVALID};
                case Fragment::JUST_1: return {INVALID, EMPTY};
            }
            assert(false);
            return {INVALID, INVALID};
        };

        auto tester = [&helper](const Node& node, Span<InputResult> subres) -> InputResult {
            auto ret = helper(node, subres);

            // Do a consistency check between the satisfaction code and the type checker
            // (the actual satisfaction code in ProduceInputHelper does not use GetType)

            // For 'z' nodes, available satisfactions/dissatisfactions must have stack size 0.
            if (node.GetType() << "z"_mst && ret.nsat.available != Availability::NO) assert(ret.nsat.stack.size() == 0);
            if (node.GetType() << "z"_mst && ret.sat.available != Availability::NO) assert(ret.sat.stack.size() == 0);

            // For 'o' nodes, available satisfactions/dissatisfactions must have stack size 1.
            if (node.GetType() << "o"_mst && ret.nsat.available != Availability::NO) assert(ret.nsat.stack.size() == 1);
            if (node.GetType() << "o"_mst && ret.sat.available != Availability::NO) assert(ret.sat.stack.size() == 1);

            // For 'n' nodes, available satisfactions/dissatisfactions must have stack size 1 or larger. For satisfactions,
            // the top element cannot be 0.
            if (node.GetType() << "n"_mst && ret.sat.available != Availability::NO) assert(ret.sat.stack.size() >= 1);
            if (node.GetType() << "n"_mst && ret.nsat.available != Availability::NO) assert(ret.nsat.stack.size() >= 1);
            if (node.GetType() << "n"_mst && ret.sat.available != Availability::NO) assert(!ret.sat.stack.back().empty());

            // For 'd' nodes, a dissatisfaction must exist, and they must not need a signature. If it is non-malleable,
            // it must be canonical.
            if (node.GetType() << "d"_mst) assert(ret.nsat.available != Availability::NO);
            if (node.GetType() << "d"_mst) assert(!ret.nsat.has_sig);
            if (node.GetType() << "d"_mst && !ret.nsat.malleable) assert(!ret.nsat.non_canon);

            // For 'f'/'s' nodes, dissatisfactions/satisfactions must have a signature.
            if (node.GetType() << "f"_mst && ret.nsat.available != Availability::NO) assert(ret.nsat.has_sig);
            if (node.GetType() << "s"_mst && ret.sat.available != Availability::NO) assert(ret.sat.has_sig);

            // For non-malleable 'e' nodes, a non-malleable dissatisfaction must exist.
            if (node.GetType() << "me"_mst) assert(ret.nsat.available != Availability::NO);
            if (node.GetType() << "me"_mst) assert(!ret.nsat.malleable);

            // For 'm' nodes, if a satisfaction exists, it must be non-malleable.
            if (node.GetType() << "m"_mst && ret.sat.available != Availability::NO) assert(!ret.sat.malleable);

            // If a non-malleable satisfaction exists, it must be canonical.
            if (ret.sat.available != Availability::NO && !ret.sat.malleable) assert(!ret.sat.non_canon);

            return ret;
        };

        return TreeEval<InputResult>(tester);
    }

public:
    /** Update duplicate key information in this Node.
     *
     * This uses a custom key comparator provided by the context in order to still detect duplicates
     * for more complicated types.
     */
    template<typename Ctx> void DuplicateKeyCheck(const Ctx& ctx) const
    {
        // We cannot use a lambda here, as lambdas are non assignable, and the set operations
        // below require moving the comparators around.
        struct Comp {
            const Ctx* ctx_ptr;
            Comp(const Ctx& ctx) : ctx_ptr(&ctx) {}
            bool operator()(const Key& a, const Key& b) const { return ctx_ptr->KeyCompare(a, b); }
        };

        // state in the recursive computation:
        // - std::nullopt means "this node has duplicates"
        // - an std::set means "this node has no duplicate keys, and they are: ...".
        using keyset = std::set<Key, Comp>;
        using state = std::optional<keyset>;

        auto upfn = [&ctx](const Node& node, Span<state> subs) -> state {
            // If this node is already known to have duplicates, nothing left to do.
            if (node.has_duplicate_keys.has_value() && *node.has_duplicate_keys) return {};

            // Check if one of the children is already known to have duplicates.
            for (auto& sub : subs) {
                if (!sub.has_value()) {
                    node.has_duplicate_keys = true;
                    return {};
                }
            }

            // Start building the set of keys involved in this node and children.
            // Start by keys in this node directly.
            size_t keys_count = node.keys.size();
            keyset key_set{node.keys.begin(), node.keys.end(), Comp(ctx)};
            if (key_set.size() != keys_count) {
                // It already has duplicates; bail out.
                node.has_duplicate_keys = true;
                return {};
            }

            // Merge the keys from the children into this set.
            for (auto& sub : subs) {
                keys_count += sub->size();
                // Small optimization: std::set::merge is linear in the size of the second arg but
                // logarithmic in the size of the first.
                if (key_set.size() < sub->size()) std::swap(key_set, *sub);
                key_set.merge(*sub);
                if (key_set.size() != keys_count) {
                    node.has_duplicate_keys = true;
                    return {};
                }
            }

            node.has_duplicate_keys = false;
            return key_set;
        };

        TreeEval<state>(upfn);
    }

    //! Return the size of the script for this expression (faster than ToScript().size()).
    size_t ScriptSize() const { return scriptlen; }

    //! Return the maximum number of ops needed to satisfy this script non-malleably.
    std::optional<uint32_t> GetOps() const {
        if (!ops.sat.valid) return {};
        return ops.count + ops.sat.value;
    }

    //! Return the number of ops in the script (not counting the dynamic ones that depend on execution).
    uint32_t GetStaticOps() const { return ops.count; }

    //! Check the ops limit of this script against the consensus limit.
    bool CheckOpsLimit() const {
        if (IsTapscript(m_script_ctx)) return true;
        if (const auto ops = GetOps()) return *ops <= MAX_OPS_PER_SCRIPT;
        return true;
    }

    /** Whether this node is of type B, K or W. (That is, anything but V.) */
    bool IsBKW() const {
        return !((GetType() & "BKW"_mst) == ""_mst);
    }

    /** Return the maximum number of stack elements needed to satisfy this script non-malleably. */
    std::optional<uint32_t> GetStackSize() const {
        if (!ss.sat.valid) return {};
        return ss.sat.netdiff + static_cast<int32_t>(IsBKW());
    }

    //! Return the maximum size of the stack during execution of this script.
    std::optional<uint32_t> GetExecStackSize() const {
        if (!ss.sat.valid) return {};
        return ss.sat.exec + static_cast<int32_t>(IsBKW());
    }

    //! Check the maximum stack size for this script against the policy limit.
    bool CheckStackSize() const {
        // Since in Tapscript there is no standardness limit on the script and witness sizes, we may run
        // into the maximum stack size while executing the script. Make sure it doesn't happen.
        if (IsTapscript(m_script_ctx)) {
            if (const auto exec_ss = GetExecStackSize()) return exec_ss <= MAX_STACK_SIZE;
            return true;
        }
        if (const auto ss = GetStackSize()) return *ss <= MAX_STANDARD_P2WSH_STACK_ITEMS;
        return true;
    }

    //! Whether no satisfaction exists for this node.
    bool IsNotSatisfiable() const { return !GetStackSize(); }

    /** Return the maximum size in bytes of a witness to satisfy this script non-malleably. Note this does
     * not include the witness script push. */
    std::optional<uint32_t> GetWitnessSize() const {
        if (!ws.sat.valid) return {};
        return ws.sat.value;
    }

    //! Return the expression type.
    Type GetType() const { return typ; }

    //! Return the script context for this node.
    MiniscriptContext GetMsCtx() const { return m_script_ctx; }

    //! Find an insane subnode which has no insane children. Nullptr if there is none.
    const Node* FindInsaneSub() const {
        return TreeEval<const Node*>([](const Node& node, Span<const Node*> subs) -> const Node* {
            for (auto& sub: subs) if (sub) return sub;
            if (!node.IsSaneSubexpression()) return &node;
            return nullptr;
        });
    }

    //! Determine whether a Miniscript node is satisfiable. fn(node) will be invoked for all
    //! key, time, and hashing nodes, and should return their satisfiability.
    template<typename F>
    bool IsSatisfiable(F fn) const
    {
        // TreeEval() doesn't support bool as NodeType, so use int instead.
        return TreeEval<int>([&fn](const Node& node, Span<int> subs) -> bool {
            switch (node.fragment) {
                case Fragment::JUST_0:
                    return false;
                case Fragment::JUST_1:
                    return true;
                case Fragment::PK_K:
                case Fragment::PK_H:
                case Fragment::MULTI:
                case Fragment::MULTI_A:
                case Fragment::AFTER:
                case Fragment::OLDER:
                case Fragment::HASH256:
                case Fragment::HASH160:
                case Fragment::SHA256:
                case Fragment::RIPEMD160:
                    return bool{fn(node)};
                case Fragment::ANDOR:
                    return (subs[0] && subs[1]) || subs[2];
                case Fragment::AND_V:
                case Fragment::AND_B:
                    return subs[0] && subs[1];
                case Fragment::OR_B:
                case Fragment::OR_C:
                case Fragment::OR_D:
                case Fragment::OR_I:
                    return subs[0] || subs[1];
                case Fragment::THRESH:
                    return static_cast<uint32_t>(std::count(subs.begin(), subs.end(), true)) >= node.k;
                default: // wrappers
                    assert(subs.size() == 1);
                    return subs[0];
            }
        });
    }

    //! Check whether this node is valid at all.
    bool IsValid() const {
        if (GetType() == ""_mst) return false;
        return ScriptSize() <= internal::MaxScriptSize(m_script_ctx);
    }

    //! Check whether this node is valid as a script on its own.
    bool IsValidTopLevel() const { return IsValid() && GetType() << "B"_mst; }

    //! Check whether this script can always be satisfied in a non-malleable way.
    bool IsNonMalleable() const { return GetType() << "m"_mst; }

    //! Check whether this script always needs a signature.
    bool NeedsSignature() const { return GetType() << "s"_mst; }

    //! Check whether there is no satisfaction path that contains both timelocks and heightlocks
    bool CheckTimeLocksMix() const { return GetType() << "k"_mst; }

    //! Check whether there is no duplicate key across this fragment and all its sub-fragments.
    bool CheckDuplicateKey() const { return has_duplicate_keys && !*has_duplicate_keys; }

    //! Whether successful non-malleable satisfactions are guaranteed to be valid.
    bool ValidSatisfactions() const { return IsValid() && CheckOpsLimit() && CheckStackSize(); }

    //! Whether the apparent policy of this node matches its script semantics. Doesn't guarantee it is a safe script on its own.
    bool IsSaneSubexpression() const { return ValidSatisfactions() && IsNonMalleable() && CheckTimeLocksMix() && CheckDuplicateKey(); }

    //! Check whether this node is safe as a script on its own.
    bool IsSane() const { return IsValidTopLevel() && IsSaneSubexpression() && NeedsSignature(); }

    //! Produce a witness for this script, if possible and given the information available in the context.
    //! The non-malleable satisfaction is guaranteed to be valid if it exists, and ValidSatisfaction()
    //! is true. If IsSane() holds, this satisfaction is guaranteed to succeed in case the node's
    //! conditions are satisfied (private keys and hash preimages available, locktimes satisfied).
    template<typename Ctx>
    Availability Satisfy(const Ctx& ctx, std::vector<std::vector<unsigned char>>& stack, bool nonmalleable = true) const {
        auto ret = ProduceInput(ctx);
        if (nonmalleable && (ret.sat.malleable || !ret.sat.has_sig)) return Availability::NO;
        stack = std::move(ret.sat.stack);
        return ret.sat.available;
    }

    //! Equality testing.
    bool operator==(const Node<Key>& arg) const { return Compare(*this, arg) == 0; }

    // Constructors with various argument combinations, which bypass the duplicate key check.
    Node(internal::NoDupCheck, MiniscriptContext script_ctx, Fragment nt, std::vector<NodeRef<Key>> sub, std::vector<unsigned char> arg, uint32_t val = 0)
        : fragment(nt), k(val), data(std::move(arg)), subs(std::move(sub)), m_script_ctx{script_ctx}, ops(CalcOps()), ss(CalcStackSize()), ws(CalcWitnessSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, MiniscriptContext script_ctx, Fragment nt, std::vector<unsigned char> arg, uint32_t val = 0)
        : fragment(nt), k(val), data(std::move(arg)), m_script_ctx{script_ctx}, ops(CalcOps()), ss(CalcStackSize()), ws(CalcWitnessSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, MiniscriptContext script_ctx, Fragment nt, std::vector<NodeRef<Key>> sub, std::vector<Key> key, uint32_t val = 0)
        : fragment(nt), k(val), keys(std::move(key)), m_script_ctx{script_ctx}, subs(std::move(sub)), ops(CalcOps()), ss(CalcStackSize()), ws(CalcWitnessSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, MiniscriptContext script_ctx, Fragment nt, std::vector<Key> key, uint32_t val = 0)
        : fragment(nt), k(val), keys(std::move(key)), m_script_ctx{script_ctx}, ops(CalcOps()), ss(CalcStackSize()), ws(CalcWitnessSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, MiniscriptContext script_ctx, Fragment nt, std::vector<NodeRef<Key>> sub, uint32_t val = 0)
        : fragment(nt), k(val), subs(std::move(sub)), m_script_ctx{script_ctx}, ops(CalcOps()), ss(CalcStackSize()), ws(CalcWitnessSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, MiniscriptContext script_ctx, Fragment nt, uint32_t val = 0)
        : fragment(nt), k(val), m_script_ctx{script_ctx}, ops(CalcOps()), ss(CalcStackSize()), ws(CalcWitnessSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}

    // Constructors with various argument combinations, which do perform the duplicate key check.
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<NodeRef<Key>> sub, std::vector<unsigned char> arg, uint32_t val = 0)
        : Node(internal::NoDupCheck{}, ctx.MsContext(), nt, std::move(sub), std::move(arg), val) { DuplicateKeyCheck(ctx); }
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<unsigned char> arg, uint32_t val = 0)
        : Node(internal::NoDupCheck{}, ctx.MsContext(), nt, std::move(arg), val) { DuplicateKeyCheck(ctx);}
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<NodeRef<Key>> sub, std::vector<Key> key, uint32_t val = 0)
        : Node(internal::NoDupCheck{}, ctx.MsContext(), nt, std::move(sub), std::move(key), val) { DuplicateKeyCheck(ctx); }
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<Key> key, uint32_t val = 0)
        : Node(internal::NoDupCheck{}, ctx.MsContext(), nt, std::move(key), val) { DuplicateKeyCheck(ctx); }
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<NodeRef<Key>> sub, uint32_t val = 0)
        : Node(internal::NoDupCheck{}, ctx.MsContext(), nt, std::move(sub), val) { DuplicateKeyCheck(ctx); }
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, uint32_t val = 0)
        : Node(internal::NoDupCheck{}, ctx.MsContext(), nt, val) { DuplicateKeyCheck(ctx); }
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

/** Parse a key string ending at the end of the fragment's text representation. */
template<typename Key, typename Ctx>
std::optional<std::pair<Key, int>> ParseKeyEnd(Span<const char> in, const Ctx& ctx)
{
    int key_size = FindNextChar(in, ')');
    if (key_size < 1) return {};
    auto key = ctx.FromString(in.begin(), in.begin() + key_size);
    if (!key) return {};
    return {{std::move(*key), key_size}};
}

/** Parse a hex string ending at the end of the fragment's text representation. */
template<typename Ctx>
std::optional<std::pair<std::vector<unsigned char>, int>> ParseHexStrEnd(Span<const char> in, const size_t expected_size,
                                                                         const Ctx& ctx)
{
    int hash_size = FindNextChar(in, ')');
    if (hash_size < 1) return {};
    std::string val = std::string(in.begin(), in.begin() + hash_size);
    if (!IsHex(val)) return {};
    auto hash = ParseHex(val);
    if (hash.size() != expected_size) return {};
    return {{std::move(hash), hash_size}};
}

/** BuildBack pops the last two elements off `constructed` and wraps them in the specified Fragment */
template<typename Key>
void BuildBack(const MiniscriptContext script_ctx, Fragment nt, std::vector<NodeRef<Key>>& constructed, const bool reverse = false)
{
    NodeRef<Key> child = std::move(constructed.back());
    constructed.pop_back();
    if (reverse) {
        constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, script_ctx, nt, Vector(std::move(child), std::move(constructed.back())));
    } else {
        constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, script_ctx, nt, Vector(std::move(constructed.back()), std::move(child)));
    }
}

/**
 * Parse a miniscript from its textual descriptor form.
 * This does not check whether the script is valid, let alone sane. The caller is expected to use
 * the `IsValidTopLevel()` and `IsSaneTopLevel()` to check for these properties on the node.
 */
template<typename Key, typename Ctx>
inline NodeRef<Key> Parse(Span<const char> in, const Ctx& ctx)
{
    using namespace script;

    // Account for the minimum script size for all parsed fragments so far. It "borrows" 1
    // script byte from all leaf nodes, counting it instead whenever a space for a recursive
    // expression is added (through andor, and_*, or_*, thresh). This guarantees that all fragments
    // increment the script_size by at least one, except for:
    // - "0", "1": these leafs are only a single byte, so their subtracted-from increment is 0.
    //   This is not an issue however, as "space" for them has to be created by combinators,
    //   which do increment script_size.
    // - "v:": the v wrapper adds nothing as in some cases it results in no opcode being added
    //   (instead transforming another opcode into its VERIFY form). However, the v: wrapper has
    //   to be interleaved with other fragments to be valid, so this is not a concern.
    size_t script_size{1};
    size_t max_size{internal::MaxScriptSize(ctx.MsContext())};

    // The two integers are used to hold state for thresh()
    std::vector<std::tuple<ParseContext, int64_t, int64_t>> to_parse;
    std::vector<NodeRef<Key>> constructed;

    to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);

    // Parses a multi() or multi_a() from its string representation. Returns false on parsing error.
    const auto parse_multi_exp = [&](Span<const char>& in, const bool is_multi_a) -> bool {
        const auto max_keys{is_multi_a ? MAX_PUBKEYS_PER_MULTI_A : MAX_PUBKEYS_PER_MULTISIG};
        const auto required_ctx{is_multi_a ? MiniscriptContext::TAPSCRIPT : MiniscriptContext::P2WSH};
        if (ctx.MsContext() != required_ctx) return false;
        // Get threshold
        int next_comma = FindNextChar(in, ',');
        if (next_comma < 1) return false;
        const auto k_to_integral{ToIntegral<int64_t>(std::string_view(in.begin(), next_comma))};
        if (!k_to_integral.has_value()) return false;
        const int64_t k{k_to_integral.value()};
        in = in.subspan(next_comma + 1);
        // Get keys. It is compatible for both compressed and x-only keys.
        std::vector<Key> keys;
        while (next_comma != -1) {
            next_comma = FindNextChar(in, ',');
            int key_length = (next_comma == -1) ? FindNextChar(in, ')') : next_comma;
            if (key_length < 1) return false;
            auto key = ctx.FromString(in.begin(), in.begin() + key_length);
            if (!key) return false;
            keys.push_back(std::move(*key));
            in = in.subspan(key_length + 1);
        }
        if (keys.size() < 1 || keys.size() > max_keys) return false;
        if (k < 1 || k > (int64_t)keys.size()) return false;
        if (is_multi_a) {
            // (push + xonly-key + CHECKSIG[ADD]) * n + k + OP_NUMEQUAL(VERIFY), minus one.
            script_size += (1 + 32 + 1) * keys.size() + BuildScript(k).size();
            constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::MULTI_A, std::move(keys), k));
        } else {
            script_size += 2 + (keys.size() > 16) + (k > 16) + 34 * keys.size();
            constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::MULTI, std::move(keys), k));
        }
        return true;
    };

    while (!to_parse.empty()) {
        if (script_size > max_size) return {};

        // Get the current context we are decoding within
        auto [cur_context, n, k] = to_parse.back();
        to_parse.pop_back();

        switch (cur_context) {
        case ParseContext::WRAPPED_EXPR: {
            std::optional<size_t> colon_index{};
            for (size_t i = 1; i < in.size(); ++i) {
                if (in[i] == ':') {
                    colon_index = i;
                    break;
                }
                if (in[i] < 'a' || in[i] > 'z') break;
            }
            // If there is no colon, this loop won't execute
            bool last_was_v{false};
            for (size_t j = 0; colon_index && j < *colon_index; ++j) {
                if (script_size > max_size) return {};
                if (in[j] == 'a') {
                    script_size += 2;
                    to_parse.emplace_back(ParseContext::ALT, -1, -1);
                } else if (in[j] == 's') {
                    script_size += 1;
                    to_parse.emplace_back(ParseContext::SWAP, -1, -1);
                } else if (in[j] == 'c') {
                    script_size += 1;
                    to_parse.emplace_back(ParseContext::CHECK, -1, -1);
                } else if (in[j] == 'd') {
                    script_size += 3;
                    to_parse.emplace_back(ParseContext::DUP_IF, -1, -1);
                } else if (in[j] == 'j') {
                    script_size += 4;
                    to_parse.emplace_back(ParseContext::NON_ZERO, -1, -1);
                } else if (in[j] == 'n') {
                    script_size += 1;
                    to_parse.emplace_back(ParseContext::ZERO_NOTEQUAL, -1, -1);
                } else if (in[j] == 'v') {
                    // do not permit "...vv...:"; it's not valid, and also doesn't trigger early
                    // failure as script_size isn't incremented.
                    if (last_was_v) return {};
                    to_parse.emplace_back(ParseContext::VERIFY, -1, -1);
                } else if (in[j] == 'u') {
                    script_size += 4;
                    to_parse.emplace_back(ParseContext::WRAP_U, -1, -1);
                } else if (in[j] == 't') {
                    script_size += 1;
                    to_parse.emplace_back(ParseContext::WRAP_T, -1, -1);
                } else if (in[j] == 'l') {
                    // The l: wrapper is equivalent to or_i(0,X)
                    script_size += 4;
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::JUST_0));
                    to_parse.emplace_back(ParseContext::OR_I, -1, -1);
                } else {
                    return {};
                }
                last_was_v = (in[j] == 'v');
            }
            to_parse.emplace_back(ParseContext::EXPR, -1, -1);
            if (colon_index) in = in.subspan(*colon_index + 1);
            break;
        }
        case ParseContext::EXPR: {
            if (Const("0", in)) {
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::JUST_0));
            } else if (Const("1", in)) {
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::JUST_1));
            } else if (Const("pk(", in)) {
                auto res = ParseKeyEnd<Key, Ctx>(in, ctx);
                if (!res) return {};
                auto& [key, key_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_C, Vector(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::PK_K, Vector(std::move(key))))));
                in = in.subspan(key_size + 1);
                script_size += IsTapscript(ctx.MsContext()) ? 33 : 34;
            } else if (Const("pkh(", in)) {
                auto res = ParseKeyEnd<Key>(in, ctx);
                if (!res) return {};
                auto& [key, key_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_C, Vector(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::PK_H, Vector(std::move(key))))));
                in = in.subspan(key_size + 1);
                script_size += 24;
            } else if (Const("pk_k(", in)) {
                auto res = ParseKeyEnd<Key>(in, ctx);
                if (!res) return {};
                auto& [key, key_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::PK_K, Vector(std::move(key))));
                in = in.subspan(key_size + 1);
                script_size += IsTapscript(ctx.MsContext()) ? 32 : 33;
            } else if (Const("pk_h(", in)) {
                auto res = ParseKeyEnd<Key>(in, ctx);
                if (!res) return {};
                auto& [key, key_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::PK_H, Vector(std::move(key))));
                in = in.subspan(key_size + 1);
                script_size += 23;
            } else if (Const("sha256(", in)) {
                auto res = ParseHexStrEnd(in, 32, ctx);
                if (!res) return {};
                auto& [hash, hash_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::SHA256, std::move(hash)));
                in = in.subspan(hash_size + 1);
                script_size += 38;
            } else if (Const("ripemd160(", in)) {
                auto res = ParseHexStrEnd(in, 20, ctx);
                if (!res) return {};
                auto& [hash, hash_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::RIPEMD160, std::move(hash)));
                in = in.subspan(hash_size + 1);
                script_size += 26;
            } else if (Const("hash256(", in)) {
                auto res = ParseHexStrEnd(in, 32, ctx);
                if (!res) return {};
                auto& [hash, hash_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::HASH256, std::move(hash)));
                in = in.subspan(hash_size + 1);
                script_size += 38;
            } else if (Const("hash160(", in)) {
                auto res = ParseHexStrEnd(in, 20, ctx);
                if (!res) return {};
                auto& [hash, hash_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::HASH160, std::move(hash)));
                in = in.subspan(hash_size + 1);
                script_size += 26;
            } else if (Const("after(", in)) {
                int arg_size = FindNextChar(in, ')');
                if (arg_size < 1) return {};
                const auto num{ToIntegral<int64_t>(std::string_view(in.begin(), arg_size))};
                if (!num.has_value() || *num < 1 || *num >= 0x80000000L) return {};
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::AFTER, *num));
                in = in.subspan(arg_size + 1);
                script_size += 1 + (*num > 16) + (*num > 0x7f) + (*num > 0x7fff) + (*num > 0x7fffff);
            } else if (Const("older(", in)) {
                int arg_size = FindNextChar(in, ')');
                if (arg_size < 1) return {};
                const auto num{ToIntegral<int64_t>(std::string_view(in.begin(), arg_size))};
                if (!num.has_value() || *num < 1 || *num >= 0x80000000L) return {};
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::OLDER, *num));
                in = in.subspan(arg_size + 1);
                script_size += 1 + (*num > 16) + (*num > 0x7f) + (*num > 0x7fff) + (*num > 0x7fffff);
            } else if (Const("multi(", in)) {
                if (!parse_multi_exp(in, /* is_multi_a = */false)) return {};
            } else if (Const("multi_a(", in)) {
                if (!parse_multi_exp(in, /* is_multi_a = */true)) return {};
            } else if (Const("thresh(", in)) {
                int next_comma = FindNextChar(in, ',');
                if (next_comma < 1) return {};
                const auto k{ToIntegral<int64_t>(std::string_view(in.begin(), next_comma))};
                if (!k.has_value() || *k < 1) return {};
                in = in.subspan(next_comma + 1);
                // n = 1 here because we read the first WRAPPED_EXPR before reaching THRESH
                to_parse.emplace_back(ParseContext::THRESH, 1, *k);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                script_size += 2 + (*k > 16) + (*k > 0x7f) + (*k > 0x7fff) + (*k > 0x7fffff);
            } else if (Const("andor(", in)) {
                to_parse.emplace_back(ParseContext::ANDOR, -1, -1);
                to_parse.emplace_back(ParseContext::CLOSE_BRACKET, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                to_parse.emplace_back(ParseContext::COMMA, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                to_parse.emplace_back(ParseContext::COMMA, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                script_size += 5;
            } else {
                if (Const("and_n(", in)) {
                    to_parse.emplace_back(ParseContext::AND_N, -1, -1);
                    script_size += 5;
                } else if (Const("and_b(", in)) {
                    to_parse.emplace_back(ParseContext::AND_B, -1, -1);
                    script_size += 2;
                } else if (Const("and_v(", in)) {
                    to_parse.emplace_back(ParseContext::AND_V, -1, -1);
                    script_size += 1;
                } else if (Const("or_b(", in)) {
                    to_parse.emplace_back(ParseContext::OR_B, -1, -1);
                    script_size += 2;
                } else if (Const("or_c(", in)) {
                    to_parse.emplace_back(ParseContext::OR_C, -1, -1);
                    script_size += 3;
                } else if (Const("or_d(", in)) {
                    to_parse.emplace_back(ParseContext::OR_D, -1, -1);
                    script_size += 4;
                } else if (Const("or_i(", in)) {
                    to_parse.emplace_back(ParseContext::OR_I, -1, -1);
                    script_size += 4;
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
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_A, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::SWAP: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_S, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::CHECK: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_C, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::DUP_IF: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_D, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::NON_ZERO: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_J, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::ZERO_NOTEQUAL: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_N, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::VERIFY: {
            script_size += (constructed.back()->GetType() << "x"_mst);
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_V, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::WRAP_U: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::OR_I, Vector(std::move(constructed.back()), MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::JUST_0)));
            break;
        }
        case ParseContext::WRAP_T: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::AND_V, Vector(std::move(constructed.back()), MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::JUST_1)));
            break;
        }
        case ParseContext::AND_B: {
            BuildBack(ctx.MsContext(), Fragment::AND_B, constructed);
            break;
        }
        case ParseContext::AND_N: {
            auto mid = std::move(constructed.back());
            constructed.pop_back();
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::ANDOR, Vector(std::move(constructed.back()), std::move(mid), MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::JUST_0)));
            break;
        }
        case ParseContext::AND_V: {
            BuildBack(ctx.MsContext(), Fragment::AND_V, constructed);
            break;
        }
        case ParseContext::OR_B: {
            BuildBack(ctx.MsContext(), Fragment::OR_B, constructed);
            break;
        }
        case ParseContext::OR_C: {
            BuildBack(ctx.MsContext(), Fragment::OR_C, constructed);
            break;
        }
        case ParseContext::OR_D: {
            BuildBack(ctx.MsContext(), Fragment::OR_D, constructed);
            break;
        }
        case ParseContext::OR_I: {
            BuildBack(ctx.MsContext(), Fragment::OR_I, constructed);
            break;
        }
        case ParseContext::ANDOR: {
            auto right = std::move(constructed.back());
            constructed.pop_back();
            auto mid = std::move(constructed.back());
            constructed.pop_back();
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::ANDOR, Vector(std::move(constructed.back()), std::move(mid), std::move(right)));
            break;
        }
        case ParseContext::THRESH: {
            if (in.size() < 1) return {};
            if (in[0] == ',') {
                in = in.subspan(1);
                to_parse.emplace_back(ParseContext::THRESH, n+1, k);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                script_size += 2;
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
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::THRESH, std::move(subs), k));
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
    assert(constructed[0]->ScriptSize() == script_size);
    if (in.size() > 0) return {};
    NodeRef<Key> tl_node = std::move(constructed.front());
    tl_node->DuplicateKeyCheck(ctx);
    return tl_node;
}

/** Decode a script into opcode/push pairs.
 *
 * Construct a vector with one element per opcode in the script, in reverse order.
 * Each element is a pair consisting of the opcode, as well as the data pushed by
 * the opcode (including OP_n), if any. OP_CHECKSIGVERIFY, OP_CHECKMULTISIGVERIFY,
 * OP_NUMEQUALVERIFY and OP_EQUALVERIFY are decomposed into OP_CHECKSIG, OP_CHECKMULTISIG,
 * OP_EQUAL and OP_NUMEQUAL respectively, plus OP_VERIFY.
 */
std::optional<std::vector<Opcode>> DecomposeScript(const CScript& script);

/** Determine whether the passed pair (created by DecomposeScript) is pushing a number. */
std::optional<int64_t> ParseScriptNumber(const Opcode& in);

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
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::JUST_1));
                break;
            }
            if (in[0].first == OP_0) {
                ++in;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::JUST_0));
                break;
            }
            // Public keys
            if (in[0].second.size() == 33 || in[0].second.size() == 32) {
                auto key = ctx.FromPKBytes(in[0].second.begin(), in[0].second.end());
                if (!key) return {};
                ++in;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::PK_K, Vector(std::move(*key))));
                break;
            }
            if (last - in >= 5 && in[0].first == OP_VERIFY && in[1].first == OP_EQUAL && in[3].first == OP_HASH160 && in[4].first == OP_DUP && in[2].second.size() == 20) {
                auto key = ctx.FromPKHBytes(in[2].second.begin(), in[2].second.end());
                if (!key) return {};
                in += 5;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::PK_H, Vector(std::move(*key))));
                break;
            }
            // Time locks
            std::optional<int64_t> num;
            if (last - in >= 2 && in[0].first == OP_CHECKSEQUENCEVERIFY && (num = ParseScriptNumber(in[1]))) {
                in += 2;
                if (*num < 1 || *num > 0x7FFFFFFFL) return {};
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::OLDER, *num));
                break;
            }
            if (last - in >= 2 && in[0].first == OP_CHECKLOCKTIMEVERIFY && (num = ParseScriptNumber(in[1]))) {
                in += 2;
                if (num < 1 || num > 0x7FFFFFFFL) return {};
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::AFTER, *num));
                break;
            }
            // Hashes
            if (last - in >= 7 && in[0].first == OP_EQUAL && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && (num = ParseScriptNumber(in[5])) && num == 32 && in[6].first == OP_SIZE) {
                if (in[2].first == OP_SHA256 && in[1].second.size() == 32) {
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::SHA256, in[1].second));
                    in += 7;
                    break;
                } else if (in[2].first == OP_RIPEMD160 && in[1].second.size() == 20) {
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::RIPEMD160, in[1].second));
                    in += 7;
                    break;
                } else if (in[2].first == OP_HASH256 && in[1].second.size() == 32) {
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::HASH256, in[1].second));
                    in += 7;
                    break;
                } else if (in[2].first == OP_HASH160 && in[1].second.size() == 20) {
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::HASH160, in[1].second));
                    in += 7;
                    break;
                }
            }
            // Multi
            if (last - in >= 3 && in[0].first == OP_CHECKMULTISIG) {
                if (IsTapscript(ctx.MsContext())) return {};
                std::vector<Key> keys;
                const auto n = ParseScriptNumber(in[1]);
                if (!n || last - in < 3 + *n) return {};
                if (*n < 1 || *n > 20) return {};
                for (int i = 0; i < *n; ++i) {
                    if (in[2 + i].second.size() != 33) return {};
                    auto key = ctx.FromPKBytes(in[2 + i].second.begin(), in[2 + i].second.end());
                    if (!key) return {};
                    keys.push_back(std::move(*key));
                }
                const auto k = ParseScriptNumber(in[2 + *n]);
                if (!k || *k < 1 || *k > *n) return {};
                in += 3 + *n;
                std::reverse(keys.begin(), keys.end());
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::MULTI, std::move(keys), *k));
                break;
            }
            // Tapscript's equivalent of multi
            if (last - in >= 4 && in[0].first == OP_NUMEQUAL) {
                if (!IsTapscript(ctx.MsContext())) return {};
                // The necessary threshold of signatures.
                const auto k = ParseScriptNumber(in[1]);
                if (!k) return {};
                if (*k < 1 || *k > MAX_PUBKEYS_PER_MULTI_A) return {};
                if (last - in < 2 + *k * 2) return {};
                std::vector<Key> keys;
                keys.reserve(*k);
                // Walk through the expected (pubkey, CHECKSIG[ADD]) pairs.
                for (int pos = 2;; pos += 2) {
                    if (last - in < pos + 2) return {};
                    // Make sure it's indeed an x-only pubkey and a CHECKSIG[ADD], then parse the key.
                    if (in[pos].first != OP_CHECKSIGADD && in[pos].first != OP_CHECKSIG) return {};
                    if (in[pos + 1].second.size() != 32) return {};
                    auto key = ctx.FromPKBytes(in[pos + 1].second.begin(), in[pos + 1].second.end());
                    if (!key) return {};
                    keys.push_back(std::move(*key));
                    // Make sure early we don't parse an arbitrary large expression.
                    if (keys.size() > MAX_PUBKEYS_PER_MULTI_A) return {};
                    // OP_CHECKSIG means it was the last one to parse.
                    if (in[pos].first == OP_CHECKSIG) break;
                }
                if (keys.size() < (size_t)*k) return {};
                in += 2 + keys.size() * 2;
                std::reverse(keys.begin(), keys.end());
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::MULTI_A, std::move(keys), *k));
                break;
            }
            /** In the following wrappers, we only need to push SINGLE_BKV_EXPR rather
             * than BKV_EXPR, because and_v commutes with these wrappers. For example,
             * c:and_v(X,Y) produces the same script as and_v(X,c:Y). */
            // c: wrapper
            if (in[0].first == OP_CHECKSIG) {
                ++in;
                to_parse.emplace_back(DecodeContext::CHECK, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                break;
            }
            // v: wrapper
            if (in[0].first == OP_VERIFY) {
                ++in;
                to_parse.emplace_back(DecodeContext::VERIFY, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                break;
            }
            // n: wrapper
            if (in[0].first == OP_0NOTEQUAL) {
                ++in;
                to_parse.emplace_back(DecodeContext::ZERO_NOTEQUAL, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                break;
            }
            // Thresh
            if (last - in >= 3 && in[0].first == OP_EQUAL && (num = ParseScriptNumber(in[1]))) {
                if (*num < 1) return {};
                in += 2;
                to_parse.emplace_back(DecodeContext::THRESH_W, 0, *num);
                break;
            }
            // OP_ENDIF can be WRAP_J, WRAP_D, ANDOR, OR_C, OR_D, or OR_I
            if (in[0].first == OP_ENDIF) {
                ++in;
                to_parse.emplace_back(DecodeContext::ENDIF, -1, -1);
                to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
                break;
            }
            /** In and_b and or_b nodes, we only look for SINGLE_BKV_EXPR, because
             * or_b(and_v(X,Y),Z) has script [X] [Y] [Z] OP_BOOLOR, the same as
             * and_v(X,or_b(Y,Z)). In this example, the former of these is invalid as
             * miniscript, while the latter is valid. So we leave the and_v "outside"
             * while decoding. */
            // and_b
            if (in[0].first == OP_BOOLAND) {
                ++in;
                to_parse.emplace_back(DecodeContext::AND_B, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                to_parse.emplace_back(DecodeContext::W_EXPR, -1, -1);
                break;
            }
            // or_b
            if (in[0].first == OP_BOOLOR) {
                ++in;
                to_parse.emplace_back(DecodeContext::OR_B, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                to_parse.emplace_back(DecodeContext::W_EXPR, -1, -1);
                break;
            }
            // Unrecognised expression
            return {};
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
            if (in >= last || in[0].first != OP_SWAP || constructed.empty()) return {};
            ++in;
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_S, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::ALT: {
            if (in >= last || in[0].first != OP_TOALTSTACK || constructed.empty()) return {};
            ++in;
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_A, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::CHECK: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_C, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::DUP_IF: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_D, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::VERIFY: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_V, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::NON_ZERO: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_J, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::ZERO_NOTEQUAL: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::WRAP_N, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::AND_V: {
            if (constructed.size() < 2) return {};
            BuildBack(ctx.MsContext(), Fragment::AND_V, constructed, /*reverse=*/true);
            break;
        }
        case DecodeContext::AND_B: {
            if (constructed.size() < 2) return {};
            BuildBack(ctx.MsContext(), Fragment::AND_B, constructed, /*reverse=*/true);
            break;
        }
        case DecodeContext::OR_B: {
            if (constructed.size() < 2) return {};
            BuildBack(ctx.MsContext(), Fragment::OR_B, constructed, /*reverse=*/true);
            break;
        }
        case DecodeContext::OR_C: {
            if (constructed.size() < 2) return {};
            BuildBack(ctx.MsContext(), Fragment::OR_C, constructed, /*reverse=*/true);
            break;
        }
        case DecodeContext::OR_D: {
            if (constructed.size() < 2) return {};
            BuildBack(ctx.MsContext(), Fragment::OR_D, constructed, /*reverse=*/true);
            break;
        }
        case DecodeContext::ANDOR: {
            if (constructed.size() < 3) return {};
            NodeRef<Key> left = std::move(constructed.back());
            constructed.pop_back();
            NodeRef<Key> right = std::move(constructed.back());
            constructed.pop_back();
            NodeRef<Key> mid = std::move(constructed.back());
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::ANDOR, Vector(std::move(left), std::move(mid), std::move(right)));
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
            if (k < 1 || k > n || constructed.size() < static_cast<size_t>(n)) return {};
            std::vector<NodeRef<Key>> subs;
            for (int i = 0; i < n; ++i) {
                NodeRef<Key> sub = std::move(constructed.back());
                constructed.pop_back();
                subs.push_back(std::move(sub));
            }
            constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, ctx.MsContext(), Fragment::THRESH, std::move(subs), k));
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
                BuildBack(ctx.MsContext(), Fragment::OR_I, constructed, /*reverse=*/true);
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
    NodeRef<Key> tl_node = std::move(constructed.front());
    tl_node->DuplicateKeyCheck(ctx);
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
    // A too large Script is necessarily invalid, don't bother parsing it.
    if (script.size() > MaxScriptSize(ctx.MsContext())) return {};
    auto decomposed = DecomposeScript(script);
    if (!decomposed) return {};
    auto it = decomposed->begin();
    auto ret = DecodeScript<typename Ctx::Key>(it, decomposed->end(), ctx);
    if (!ret) return {};
    if (it != decomposed->end()) return {};
    return ret;
}

} // namespace miniscript

#endif // BITCOIN_SCRIPT_MINISCRIPT_H
