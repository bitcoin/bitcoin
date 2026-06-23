// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_POINTER_TAG_PAIR_H
#define BITCOIN_UTIL_POINTER_TAG_PAIR_H

#include <cstdint>

/**
 * Stores a pointer together with a small integer tag in the same word, reusing
 * the low alignment bits of the pointer that are always zero. This is a
 * stop-gap for std::pointer_tag_pair (P3125); the interface is intentionally
 * close so it can be replaced once that is available.
 *
 * `Bits` low bits are reserved for the tag. The caller must ensure `T` is
 * aligned to at least `1 << Bits` so those bits are genuinely free; because `T`
 * may be incomplete here (e.g. a self-referential node type), that check has to
 * be made at the use site once `T` is complete, e.g.
 *   static_assert(alignof(T) >= (1u << Bits));
 *
 * A default-constructed pair holds a null pointer and a zero tag, with the
 * whole word equal to 0.
 */
template <typename T, unsigned Bits>
class PointerTagPair
{
    static constexpr uintptr_t TAG_MASK{(uintptr_t{1} << Bits) - 1};
    uintptr_t m_value{0};

public:
    PointerTagPair() noexcept = default;

    T* pointer() const noexcept { return reinterpret_cast<T*>(m_value & ~TAG_MASK); }
    void set_pointer(T* ptr) noexcept { m_value = reinterpret_cast<uintptr_t>(ptr) | (m_value & TAG_MASK); }

    unsigned tag() const noexcept { return static_cast<unsigned>(m_value & TAG_MASK); }
    void set_tag(unsigned tag) noexcept { m_value = (m_value & ~TAG_MASK) | (uintptr_t{tag} & TAG_MASK); }

    //! True iff both the pointer is null and the tag is zero.
    bool empty() const noexcept { return m_value == 0; }
    void clear() noexcept { m_value = 0; }
};

#endif // BITCOIN_UTIL_POINTER_TAG_PAIR_H
