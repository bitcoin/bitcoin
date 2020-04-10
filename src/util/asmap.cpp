// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <map>
#include <memory>
#include <vector>
#include <assert.h>
#include <crypto/common.h>
#include <util/vector.h>

#include <stdio.h>

namespace {

constexpr uint32_t INVALID = 0xFFFFFFFF;

uint32_t DecodeBits(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos, uint8_t minval, const std::vector<uint8_t> &bit_sizes)
{
    uint32_t val = minval;
    bool bit;
    for (std::vector<uint8_t>::const_iterator bit_sizes_it = bit_sizes.begin();
        bit_sizes_it != bit_sizes.end(); ++bit_sizes_it) {
        if (bit_sizes_it + 1 != bit_sizes.end()) {
            if (bitpos == endpos) break;
            bit = *bitpos;
            bitpos++;
        } else {
            bit = 0;
        }
        if (bit) {
            val += (1 << *bit_sizes_it);
        } else {
            for (int b = 0; b < *bit_sizes_it; b++) {
                if (bitpos == endpos) return INVALID; // Reached EOF in mantissa
                bit = *bitpos;
                bitpos++;
                val += bit << (*bit_sizes_it - 1 - b);
            }
            return val;
        }
    }
    return INVALID; // Reached EOF in exponent
}

/** Append an encoding of value (with specified minval/bit_sizes parameters) to out. */
void EncodeBits(std::vector<bool>& out, uint32_t val, uint32_t minval, const std::vector<uint8_t>& bit_sizes)
{
    val -= minval;
    for (size_t pos = 0; pos < bit_sizes.size(); ++pos) {
        uint8_t bit_size = bit_sizes[pos];
        if (val >> bit_size) {
            val -= (1 << bit_size);
            out.push_back(true);
        } else {
            if (pos + 1 < bit_sizes.size()) out.push_back(false);
            for (int b = 0; b < bit_size; ++b) out.push_back((val >> (bit_size - 1 - b)) & 1);
            return;
        }
    }
    assert(false);
}

/** Predict how big an encoding of value (with specified minval/bit_sizes parameters) will be. */
size_t SizeBits(uint32_t val, uint32_t minval, const std::vector<uint8_t>& bit_sizes)
{
    size_t ret = 0;
    val -= minval;
    for (size_t pos = 0; pos < bit_sizes.size(); ++pos) {
        uint8_t bit_size = bit_sizes[pos];
        if (val >> bit_size) {
            val -= (1 << bit_size);
            ++ret;
        } else {
            if (pos + 1 < bit_sizes.size()) ++ret;
            return ret + bit_size;
        }
    }
    assert(false);
}

enum class Instruction : uint32_t
{
    RETURN = 0,
    JUMP = 1,
    MATCH = 2,
    DEFAULT = 3,
};

const std::vector<uint8_t> TYPE_BIT_SIZES{0, 0, 1};
Instruction DecodeType(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return Instruction(DecodeBits(bitpos, endpos, 0, TYPE_BIT_SIZES));
}

void EncodeType(std::vector<bool>& out, Instruction opcode) { EncodeBits(out, uint32_t(opcode), 0, TYPE_BIT_SIZES); }
size_t SizeType(Instruction opcode) { return SizeBits(uint32_t(opcode), 0, TYPE_BIT_SIZES); }

const std::vector<uint8_t> ASN_BIT_SIZES{15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
uint32_t DecodeASN(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return DecodeBits(bitpos, endpos, 1, ASN_BIT_SIZES);
}

void EncodeASN(std::vector<bool>& out, uint32_t asn) { EncodeBits(out, asn, 1, ASN_BIT_SIZES); }
size_t SizeASN(uint32_t asn) { return SizeBits(asn, 1, ASN_BIT_SIZES); }

const std::vector<uint8_t> MATCH_BIT_SIZES{1, 2, 3, 4, 5, 6, 7, 8};
uint32_t DecodeMatch(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return DecodeBits(bitpos, endpos, 2, MATCH_BIT_SIZES);
}

void EncodeMatch(std::vector<bool>& out, uint32_t match) { EncodeBits(out, match, 2, MATCH_BIT_SIZES); }
size_t SizeMatch(uint32_t match) { return SizeBits(match, 2, MATCH_BIT_SIZES); }
constexpr uint32_t MATCH_MAX = 0x1FF;

const std::vector<uint8_t> JUMP_BIT_SIZES{5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
uint32_t DecodeJump(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return DecodeBits(bitpos, endpos, 17, JUMP_BIT_SIZES);
}

void EncodeJump(std::vector<bool>& out, uint32_t offset) { EncodeBits(out, offset, 17, JUMP_BIT_SIZES); }
size_t SizeJump(uint32_t offset) { return SizeBits(offset, 17, JUMP_BIT_SIZES); }

/** A (node of) a binary prefix tree that represents a mapping.
 *
 * This data structure is used as an intermediate representation when encoding.
 *
 * Each node is either:
 * - A leaf (with sub[0,1] set to nullptr, and asn set)
 * - An inner node (with sub[0,1] pointing to child nodes, and asn ignored)
 */
struct ASMapTrie {
    /** The asn this node maps to if it is a leaf.
     *
     * - 0 means unmatched (no mapping exists for this prefix)
     * - INVALID means dontcare (the encoding is allowed to map this to anything)
     * - Any other value is the ASN itself.
     *
     * This is always 0 for inner nodes in the tree.
     */
    uint32_t asn = 0; /* 0=unmatched; INVALID=dontcare; others=asn */

    /** The child nodes of this tree. They're nullptr for leaf nodes. */
    std::unique_ptr<ASMapTrie> sub[2];

    /** Check whether this node is a leaf. */
    bool IsLeaf() const { return !sub[0]; }

    /** Convert this node into a leaf with ASN v, regardless of what it was. */
    void MakeLeaf(uint32_t v)
    {
        asn = v;
        sub[0].reset(nullptr);
        sub[1].reset(nullptr);
    }

    /** Make the prefix identified by begin..end map to ASN v. */
    void MapTo(std::vector<bool>::const_iterator begin, std::vector<bool>::const_iterator end, uint32_t v)
    {
        if (begin == end) {
            MakeLeaf(v);
        } else {
            if (IsLeaf()) {
                // We need to descend into this node, but it's a leaf. Convert to inner node.
                sub[0].reset(new ASMapTrie());
                sub[1].reset(new ASMapTrie());
                sub[0]->asn = sub[1]->asn = asn;
                asn = 0;
            }
            sub[*begin]->MapTo(std::next(begin), end, v);
        }
    }

    /** Remove unnecessary branches. */
    void Compact()
    {
        if (IsLeaf()) return; // If a node is already a leaf we can't compact it further.
        // Compact the child nodes, if possible.
        sub[0]->Compact();
        sub[1]->Compact();
        // If both are (now) leaf nodes, maybe then can be merged into one.
        if (sub[0]->IsLeaf() && sub[1]->IsLeaf()) {
            if (sub[0]->asn == sub[1]->asn || sub[1]->asn == INVALID) {
                // If they map to the same thing, or the second one does not matter, turn this
                // node into a leaf equal to the first one.
                MakeLeaf(sub[0]->asn);
            } else if (sub[0]->asn == INVALID) {
                // If the first one does not matter, turn this node into a leaf equal to the
                // second one.
                MakeLeaf(sub[1]->asn);
            }
        }
    }
};

/** This class represents a potential encoding for an ASMapTrie node (=subtree). */
struct ASMapEncoding {
    /** Encodings for the subtrees.
     *  - For RETURN: both are nullptr
     *  - For DEFAULT and MATCH: left is set but right is nullptr
     *  - For JUMP: both are set (left represents the 0 path, right the 1 path)
     *
     * std::shared_ptr is used because these subtrees are built in a generic
     * programming fashion. We first construct the optimal encodings for every
     * ASMapTrie node's children, and from those, compute the optimal encodings
     * for the node itself. Since there may be multiple parent-encodings that
     * are built on the same child-encodings, we need to allow them to be
     * shared, or we'd end up with an exponential memory growth.
     */
    std::shared_ptr<const ASMapEncoding> left, right;

    /** The number of bits this encoding will encode to (including all children).
     *
     * This value can be INVALID for RETURN instructions with ASN 0. These do not
     * actually have a valid encoding, but they're used as temporaries that can
     * get absorbed into MATCH instructions at parent levels.
     */
    uint32_t bits;

    /** What opcode to encode. */
    Instruction opcode;

    /** What value to encode.
     *  - For RETURN: the value to return (0=no match, INVALID=dontcare, anything else: ASN)
     *  - For DEFAULT: the ASN to set default_asn to (only valid ASNs>0 allowed)
     *  - For MATCH: the matchval (encoding the bits to match with; see PushMatchVal documentation)
     *  - For JUMP: 0
     */
    uint32_t value;

    /** Encodings can be generic or contextual. They're contextual if they are only
     * valid when interpreted in an environment where default_asn has a specific
     * pre-set value. This is stored in the context member. It is:
     * - 0 if it has be interpreted with default_asn=0 (the initial value).
     * - INVALID if it is a generic encoding (which is valid in any context).
     * - Any other value if the last executed DEFAULT instruction before this
     *   encoding has to be that exact value.
    */
    uint32_t context;

    /** Whether this encoding will be valid in any context. */
    bool IsGeneric() const { return context == INVALID; }

    /** Whether an encoding actually exists for this node. */
    bool IsValid() const { return bits != INVALID; }

    /** Compute number of bits of a prospective encoding. 0 = cannot be encoded.
     *
     * This is a separate static function, so that we can avoid constructing a
     * new object if it wouldn't actually be an improvement.
     */
    static size_t ComputeBits(Instruction opcode, uint32_t value, const std::shared_ptr<const ASMapEncoding>& left, const std::shared_ptr<const ASMapEncoding>& right)
    {
        if (opcode == Instruction::RETURN) {
            // Return 0 is invalid, so output INVALID. Return dontcare (INVALID) is encoded as RETURN 1.
            return value == 0 ? INVALID : SizeType(opcode) + SizeASN(value == INVALID ? 1 : value);
        } else if (opcode == Instruction::JUMP) {
            return SizeType(opcode) + SizeJump(left->bits) + left->bits + right->bits;
        } else if (opcode == Instruction::MATCH) {
            return SizeType(opcode) + SizeMatch(value) + left->bits;
        } else {
            return SizeType(opcode) + SizeASN(value) + left->bits;
        }
    }

    /** Construct a new potential encoding object.
     * - o: the instruction opcode
     * - c: the context (incl. 0 or INVALID to represent default_asn=0 or dontcare)
     * - v: the value (ASN for RETURN/DEFAULT, matchval for MATCH, 0 for JUMP)
     * - l, r: the child encodings
     */
    ASMapEncoding(Instruction o, uint32_t c, uint32_t v, const std::shared_ptr<const ASMapEncoding>& l, const std::shared_ptr<const ASMapEncoding>& r)
    {
        opcode = o;
        value = v;
        context = c;
        bits = ComputeBits(o, v, l, r);
        left = l;
        right = r;
        if (o == Instruction::RETURN) {
            assert(IsGeneric()); // RETURN instructions are always generic,
            assert(!left && !right); // .. and never have child encodings
        } else if (o == Instruction::JUMP) {
            assert(left && right && left->IsValid() && right->IsValid()); // JUMP instructions have 2 valid child encodings,
            assert(value == 0); // ... and don't have any value.
            assert(l->IsGeneric() || r->IsGeneric() || l->context == r->context); // If both children are contextual, they must have the same context.
            assert(context == (l->IsGeneric() ? r->context : l->context)); // The JUMP's context is the more specific one of its children's contexts.
        } else if (o == Instruction::MATCH) {
            assert(left && !right && left->IsValid()); // MATCH instructions have 1 valid child encoding
            assert(value >= 2 && value <= MATCH_MAX); // Their value must be in range
            // If the child is contextual, the MATCH must inherit it. However,
            // if the child is generic, the MATCH may be more specific, as it
            // will return the default_asn in case of failure).
            assert(left->IsGeneric() || left->context == context);
        } else if (o == Instruction::DEFAULT) {
            assert(left && !right && left->IsValid()); // DEFAULT instructions have 1 valid child encoding,
            assert(value != 0 && value != INVALID); // ... they cannot be 0 or dontcare,
            assert(IsGeneric()); // ...and they're valid in every context.
        } else {
            assert(false);
        }
    }

    /** Append the encoding to out. */
    void Encode(std::vector<bool>& out) const
    {
        assert(IsValid()); // Only valid nodes can be encoded.
        size_t old_size = out.size(); // Keep track of the old size.
        EncodeType(out, opcode);
        if (opcode == Instruction::RETURN) {
            EncodeASN(out, value == INVALID ? 1 : value);
        } else if (opcode == Instruction::JUMP) {
            EncodeJump(out, left->bits);
            left->Encode(out);
            right->Encode(out);
        } else if (opcode == Instruction::MATCH) {
            EncodeMatch(out, value);
            left->Encode(out);
        } else if (opcode == Instruction::DEFAULT) {
            EncodeASN(out, value);
            left->Encode(out);
        }
        assert(out.size() == old_size + bits); // Verify that exactly bits bits were added.
    }
};

/** A data structure to hold all best encodings for a subtree. */
struct ASMapEncodings {
    // We only keep the single best generic encoding (if one exists), and at
    // most one contextual encoding per context value (including 0).
    //
    // If it were possible to have a MATCH encoding and a non-MATCH encoding
    // for the same node and the same context, where the non-MATCH encoding was
    // smaller, we would have needed to keep both. This is because MATCH nodes
    // may be extended more cheaply than others, so an extension of a MATCH
    // in that case could be smaller than the extension of the non-MATCH one.
    //
    // It turns out this is unnecessary, because when a MATCH encoding exists
    // for a certain node and context, it is always the smallest encoding.

    /** The best generic encoding for the subtree (nullptr if none). */
    std::shared_ptr<const ASMapEncoding> generic;
    /** The best contextual encodings for the subtree (for every context value). */
    std::map<uint32_t, std::shared_ptr<const ASMapEncoding>> contextual;

    /** Update this data structure with a new prospective encoding. */
    void Consider(Instruction opcode, uint32_t context, uint32_t value = 0, const std::shared_ptr<const ASMapEncoding>& left = {}, const std::shared_ptr<const ASMapEncoding>& right = {})
    {
        // Compute its validity/size without constructing a new object first.
        size_t bits = ASMapEncoding::ComputeBits(opcode, value, left, right);
        if (context == INVALID) {
            // Update best generic encoding if it's generic, and better than what we had.
            if (!generic || bits < generic->bits) {
                generic = std::make_shared<const ASMapEncoding>(opcode, context, value, left, right);
            }
        } else {
            // Contextual encodings are never invalid (only RETURN can be invalid, and those are always generic).
            assert(bits != INVALID);
            auto it = contextual.emplace(context, std::shared_ptr<const ASMapEncoding>());
            if (it.second || bits < it.first->second->bits) {
                // Update best contextual encoding for the provided context, if it's new, or better than what we had.
                it.first->second = std::make_shared<const ASMapEncoding>(opcode, context, value, left, right);
            }
        }
    }
};

/** This function computes an extension to a matchval.
 *
 * Matchings are encoded as integers whose bits are the bits to be matched with,
 * prefixed by a 1.
 *
 * So for example:
 * - 2 = 0b10: match 0
 * - 3 = 0b11: match 1
 * - 9 = 0b1001: match 0,0,1
 * - 25 = 0b11001: match 1,0,0,1
 *
 * This function computes a matchval that represents first matching with bit,
 * and then matching with whatever is represented by matchval val.
 */
uint32_t PushMatchVal(uint32_t val, bool bit)
{
    int bits = CountBits(val) - 1;
    return val + ((1 + bit) << bits);
}

/** Given an inner AsMapTrie node, with encodings l and r for its two children,
 *  update encs with all potential encodings for the node based on this child
 *  encodings.
 */
void ConsiderBranch(ASMapEncodings& encs, const std::shared_ptr<const ASMapEncoding>& l, const std::shared_ptr<const ASMapEncoding>& r) {
    // If the right side is a RETURN with a value that compatible with the left side's context,
    // consider a MATCH node that elides the RETURN.
    if (r->opcode == Instruction::RETURN && (l->context == r->value || l->IsGeneric() || r->value == INVALID)) {
        uint32_t context = l->IsGeneric() ? r->value : l->context;
        if (l->opcode == Instruction::MATCH && PushMatchVal(l->value, false) <= MATCH_MAX) {
            // The left side is a MATCH we can extend with a 0 bit.
            encs.Consider(Instruction::MATCH, context, PushMatchVal(l->value, false), l->left);
        } else if (l->IsValid()) {
            // The left side is a valid node we can choose (2 means "match 0").
            encs.Consider(Instruction::MATCH, context, 2, l);
        }
    }
    // If the left side is a RETURN with a value that compatible with the right side's context,
    // consider a MATCH node that elides the RETURN.
    if (l->opcode == Instruction::RETURN && (r->context == l->value || r->IsGeneric() || l->value == INVALID)) {
        uint32_t context = r->IsGeneric() ? l->value : r->context;
        if (r->opcode == Instruction::MATCH && PushMatchVal(r->value, true) <= MATCH_MAX) {
            // The right side is a MATCH we can extend with a 1 bit.
            encs.Consider(Instruction::MATCH, context, PushMatchVal(r->value, true), r->left);
        } else if (r->IsValid()) {
            // The right side is a valid node we can choose (3 means "match 1").
            encs.Consider(Instruction::MATCH, context, 3, r);
        }
    }

    // If left and right are both valid, and they have compatible contexts, consider a JUMP node.
    if (l->IsValid() && r->IsValid() && (l->context == r->context || l->IsGeneric() || r->IsGeneric())) {
        encs.Consider(Instruction::JUMP, l->IsGeneric() ? r->context : l->context, 0, l, r);
    }
}

/** Compute the best encodings (generic and contextual for any relevant context) for trie. */
ASMapEncodings Encode(const std::unique_ptr<ASMapTrie>& trie)
{
    ASMapEncodings ret;

    if (trie->IsLeaf()) {
        // Leaf nodes are just directly turned into RETURN encodings (which may be invalid if asn=0).
        ret.Consider(Instruction::RETURN, INVALID, trie->asn);
    } else {
        // Otherwise, compute the best encodings for the child nodes.
        auto left = Encode(trie->sub[0]);
        auto right = Encode(trie->sub[1]);

        // Consider a branch between the two best generic encodings.
        if (left.generic && right.generic) ConsiderBranch(ret, left.generic, right.generic);
        // If there is a generic encoding on the left, combine it with all contextual encodings on the right.
        if (left.generic) {
            for (const auto& entry : right.contextual) ConsiderBranch(ret, left.generic, entry.second);
        }
        // If there is a generic encoding on the right, combine it with all contextual encodings on the left
        if (right.generic) {
            for (const auto& entry : left.contextual) ConsiderBranch(ret, entry.second, right.generic);
        }
        // Combine all contextual encodings where the context matches left and right.
        for (const auto& left_entry : left.contextual) {
            auto it = right.contextual.find(left_entry.first);
            if (it != right.contextual.end()) ConsiderBranch(ret, left_entry.second, it->second);
        }
        // Any resulting contextual encoding with context!=0 can be converted into a generic one using a DEFAULT instruction.
        for (const auto& entry : ret.contextual) {
            if (entry.first != 0) ret.Consider(Instruction::DEFAULT, INVALID, entry.first, entry.second);
        }
        // If we have a valid generic encoding, any contextual ones that are not better can be removed.
        if (ret.generic && ret.generic->IsValid()) {
            for (auto it = ret.contextual.begin(); it != ret.contextual.end(); ) {
                if (it->second->bits >= ret.generic->bits) {
                    it = ret.contextual.erase(it);
                } else {
                    it = std::next(it);
                }
            }
        }
    }

    return ret;
}

}

uint32_t Interpret(const std::vector<bool> &asmap, const std::vector<bool> &ip)
{
    std::vector<bool>::const_iterator pos = asmap.begin();
    const std::vector<bool>::const_iterator endpos = asmap.end();
    uint8_t bits = ip.size();
    uint32_t default_asn = 0;
    uint32_t jump, match, matchlen;
    Instruction opcode;
    while (pos != endpos) {
        opcode = DecodeType(pos, endpos);
        if (opcode == Instruction::RETURN) {
            default_asn = DecodeASN(pos, endpos);
            if (default_asn == INVALID) break; // ASN straddles EOF
            return default_asn;
        } else if (opcode == Instruction::JUMP) {
            jump = DecodeJump(pos, endpos);
            if (jump == INVALID) break; // Jump offset straddles EOF
            if (bits == 0) break; // No input bits left
            if (jump >= endpos - pos) break; // Jumping past EOF
            if (ip[ip.size() - bits]) {
                pos += jump;
            }
            bits--;
        } else if (opcode == Instruction::MATCH) {
            match = DecodeMatch(pos, endpos);
            if (match == INVALID) break; // Match bits straddle EOF
            matchlen = CountBits(match) - 1;
            if (bits < matchlen) break; // Not enough input bits
            for (uint32_t bit = 0; bit < matchlen; bit++) {
                if ((ip[ip.size() - bits]) != ((match >> (matchlen - 1 - bit)) & 1)) {
                    return default_asn;
                }
                bits--;
            }
        } else if (opcode == Instruction::DEFAULT) {
            default_asn = DecodeASN(pos, endpos);
            if (default_asn == INVALID) break; // ASN straddles EOF
        } else {
            break; // Instruction straddles EOF
        }
    }
    assert(false); // Reached EOF without RETURN, or aborted (see any of the breaks above) - should have been caught by SanityCheckASMap below
    return 0; // 0 is not a valid ASN
}

bool SanityCheckASMap(const std::vector<bool>& asmap, int bits)
{
    const std::vector<bool>::const_iterator begin = asmap.begin(), endpos = asmap.end();
    std::vector<bool>::const_iterator pos = begin;
    std::vector<std::pair<uint32_t, int>> jumps; // All future positions we may jump to (bit offset in asmap -> bits to consume left)
    jumps.reserve(bits);
    Instruction prevopcode = Instruction::JUMP;
    bool had_incomplete_match = false;
    while (pos != endpos) {
        uint32_t offset = pos - begin;
        if (!jumps.empty() && offset >= jumps.back().first) return false; // There was a jump into the middle of the previous instruction
        Instruction opcode = DecodeType(pos, endpos);
        if (opcode == Instruction::RETURN) {
            if (prevopcode == Instruction::DEFAULT) return false; // There should not be any RETURN immediately after a DEFAULT (could be combined into just RETURN)
            uint32_t asn = DecodeASN(pos, endpos);
            if (asn == INVALID) return false; // ASN straddles EOF
            if (jumps.empty()) {
                // Nothing to execute anymore
                if (endpos - pos > 7) return false; // Excessive padding
                while (pos != endpos) {
                    if (*pos) return false; // Nonzero padding bit
                    ++pos;
                }
                return true; // Sanely reached EOF
            } else {
                // Continue by pretending we jumped to the next instruction
                offset = pos - begin;
                if (offset != jumps.back().first) return false; // Unreachable code
                bits = jumps.back().second; // Restore the number of bits we would have had left after this jump
                jumps.pop_back();
                prevopcode = Instruction::JUMP;
            }
        } else if (opcode == Instruction::JUMP) {
            uint32_t jump = DecodeJump(pos, endpos);
            if (jump == INVALID) return false; // Jump offset straddles EOF
            if (jump > endpos - pos) return false; // Jump out of range
            if (bits == 0) return false; // Consuming bits past the end of the input
            --bits;
            uint32_t jump_offset = pos - begin + jump;
            if (!jumps.empty() && jump_offset >= jumps.back().first) return false; // Intersecting jumps
            jumps.emplace_back(jump_offset, bits);
            prevopcode = Instruction::JUMP;
        } else if (opcode == Instruction::MATCH) {
            uint32_t match = DecodeMatch(pos, endpos);
            if (match == INVALID) return false; // Match bits straddle EOF
            int matchlen = CountBits(match) - 1;
            if (prevopcode != Instruction::MATCH) had_incomplete_match = false;
            if (matchlen < 8 && had_incomplete_match) return false; // Within a sequence of matches only at most one should be incomplete
            had_incomplete_match = (matchlen < 8);
            if (bits < matchlen) return false; // Consuming bits past the end of the input
            bits -= matchlen;
            prevopcode = Instruction::MATCH;
        } else if (opcode == Instruction::DEFAULT) {
            if (prevopcode == Instruction::DEFAULT) return false; // There should not be two successive DEFAULTs (they could be combined into one)
            uint32_t asn = DecodeASN(pos, endpos);
            if (asn == INVALID) return false; // ASN straddles EOF
            prevopcode = Instruction::DEFAULT;
        } else {
            return false; // Instruction straddles EOF
        }
    }
    return false; // Reached EOF without RETURN instruction
}

std::vector<std::pair<std::vector<bool>, uint32_t>> DecodeASMap(const std::vector<bool>& asmap, int bits)
{
    std::vector<std::pair<std::vector<bool>, uint32_t>> ret;

    const std::vector<bool>::const_iterator begin = asmap.begin(), endpos = asmap.end();
    std::vector<bool>::const_iterator pos = begin;

    std::vector<bool> branch_bits; //!< The prefix currently assumed in the input
    std::vector<bool> branch_jumps; //!< For each bit in branch_bits, whether it's due to a JUMP (true) or a MATCH (false)
    branch_bits.reserve(bits);
    branch_jumps.reserve(bits);

    while (pos != endpos) {
        Instruction opcode = DecodeType(pos, endpos);
        if (opcode == Instruction::RETURN || opcode == Instruction::DEFAULT) {
            uint32_t asn = DecodeASN(pos, endpos);
            if (asn == INVALID) return {};
            ret.emplace_back(branch_bits, asn);
            if (opcode == Instruction::RETURN) {
                while (true) {
                    // Drop MATCH bits
                    while (branch_jumps.size() && !branch_jumps.back()) {
                        branch_bits.pop_back();
                        branch_jumps.pop_back();
                    }
                    if (branch_jumps.empty()) return ret; // Done
                    // Process JUMP branch
                    if (branch_bits.back()) {
                        // We were on the 1 side, we're done with this JUMP
                        branch_bits.pop_back();
                        branch_jumps.pop_back();
                    } else {
                        // We were on the 0 side of a JUMP, switch to the 1 side
                        branch_bits.back() = true;
                        break;
                    }
                }
            }
        } else if (opcode == Instruction::JUMP) {
            uint32_t jump = DecodeJump(pos, endpos);
            if (jump == INVALID) return {};
            if (branch_bits.size() == (size_t)bits) return {}; // Cannot jump when all bits are already consumed
            branch_bits.push_back(false); // A new 0 bit is assumed in what follows
            branch_jumps.push_back(true); // ... and it's due to a JUMP
        } else if (opcode == Instruction::MATCH) {
            uint32_t match = DecodeMatch(pos, endpos);
            if (match == INVALID) return {};
            int matchlen = CountBits(match) - 1;
            if (branch_bits.size() + matchlen > (size_t)bits) return {}; // Comsuming more bits than exist in the input
            for (int bitpos = 0; bitpos < matchlen; ++bitpos) {
                branch_bits.push_back((match >> (matchlen - 1 - bitpos)) & 1);
                branch_jumps.push_back(false); // These bits added are MATCHes, not JUMPs
            }
        } else {
            return {};
        }
    }
    return {}; // Unexpected EOF
}

std::vector<bool> EncodeASMap(std::vector<std::pair<std::vector<bool>, uint32_t>> input, bool approx)
{
    // Sort the list from short to long prefixes.
    using item = std::pair<std::vector<bool>, uint32_t>;
    std::sort(input.begin(), input.end(), [](const item& a, const item& b) { return a.first.size() < b.first.size(); });

    std::unique_ptr<ASMapTrie> elem(new ASMapTrie());
    // If approx is set, we first assign the entire range to dontcare.
    if (approx) elem->MakeLeaf(INVALID);
    // Then overwrite the mapping for the provided prefixes to the provided ASNs.
    // This is done in order from short to long, so that a mapping for a shorter
    // prefix doesn't overwrite their children.
    for (const item& entry : input) {
        elem->MapTo(entry.first.begin(), entry.first.end(), entry.second);
    }
    // Remove unnecessary branches from the tree.
    elem->Compact();

    auto data = Encode(elem);
    std::vector<bool> ret;
    if (data.contextual.count(0) && data.contextual[0]->IsValid()) {
        // Return the best encoding for context 0 (which is valid at the top level), if it exists.
        data.contextual[0]->Encode(ret);
    } else {
        // Or the best generic one otherwise.
        data.generic->Encode(ret);
    }
    return ret;
}
