// Copyright (c) 2016 Jeremy Rubin
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CUCKOOCACHE_H
#define BITCOIN_CUCKOOCACHE_H

#include <util/fastrange.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <utility>
#include <vector>


/** High-performance cache primitives.
 *
 * Summary:
 *
 * 1. @ref bit_packed_atomic_flags is bit-packed atomic flags for garbage collection
 *
 * 2. @ref cache is a cache which is performant in memory usage and lookup speed. It
 * is lockfree for erase operations. Elements are lazily erased on the next insert.
 */
namespace CuckooCache
{
/** @ref bit_packed_atomic_flags implements a container for garbage collection flags
 * that is only thread unsafe on calls to setup. This class bit-packs collection
 * flags for memory efficiency.
 *
 * All operations are `std::memory_order_relaxed` so external mechanisms must
 * ensure that writes and reads are properly synchronized.
 *
 * On setup(n), all bits up to `n` are marked as collected.
 *
 * Under the hood, because it is an 8-bit type, it makes sense to use a multiple
 * of 8 for setup, but it will be safe if that is not the case as well.
 */
class bit_packed_atomic_flags
{
    std::unique_ptr<std::atomic<uint8_t>[]> mem;

public:
    /** No default constructor, as there must be some size. */
    bit_packed_atomic_flags() = delete;

    /**
     * bit_packed_atomic_flags constructor creates memory to sufficiently
     * keep track of garbage collection information for `size` entries.
     *
     * @param size the number of elements to allocate space for
     *
     * @post bit_set, bit_unset, and bit_is_set function properly forall x. x <
     * size
     * @post All calls to bit_is_set (without subsequent bit_unset) will return
     * true.
     */
    explicit bit_packed_atomic_flags(uint32_t size)
    {
        // pad out the size if needed
        size = (size + 7) / 8;
        mem.reset(new std::atomic<uint8_t>[size]);
        for (uint32_t i = 0; i < size; ++i)
            mem[i].store(0xFF);
    };

    /** setup marks all entries and ensures that bit_packed_atomic_flags can store
     * at least `b` entries.
     *
     * @param b the number of elements to allocate space for
     * @post bit_set, bit_unset, and bit_is_set function properly forall x. x <
     * b
     * @post All calls to bit_is_set (without subsequent bit_unset) will return
     * true.
     */
    inline void setup(uint32_t b)
    {
        bit_packed_atomic_flags d(b);
        std::swap(mem, d.mem);
    }

    /** bit_set sets an entry as discardable.
     *
     * @param s the index of the entry to bit_set
     * @post immediately subsequent call (assuming proper external memory
     * ordering) to bit_is_set(s) == true.
     */
    inline void bit_set(uint32_t s)
    {
        mem[s >> 3].fetch_or(uint8_t(1 << (s & 7)), std::memory_order_relaxed);
    }

    /** bit_unset marks an entry as something that should not be overwritten.
     *
     * @param s the index of the entry to bit_unset
     * @post immediately subsequent call (assuming proper external memory
     * ordering) to bit_is_set(s) == false.
     */
    inline void bit_unset(uint32_t s)
    {
        mem[s >> 3].fetch_and(uint8_t(~(1 << (s & 7))), std::memory_order_relaxed);
    }

    /** bit_is_set queries the table for discardability at `s`.
     *
     * @param s the index of the entry to read
     * @returns true if the bit at index `s` was set, false otherwise
     * */
    inline bool bit_is_set(uint32_t s) const
    {
        return (1 << (s & 7)) & mem[s >> 3].load(std::memory_order_relaxed);
    }
};

/** @ref cache implements a cache with properties similar to a cuckoo-set.
 *
 *  The cache is able to hold up to `(~(uint32_t)0) - 1` elements.
 *
 *  Read Operations:
 *      - contains() for `erase=false`
 *
 *  Read+Erase Operations:
 *      - contains() for `erase=true`
 *
 *  Erase Operations:
 *      - allow_erase()
 *
 *  Write Operations:
 *      - setup()
 *      - setup_bytes()
 *      - insert()
 *      - please_keep()
 *
 *  Synchronization Free Operations:
 *      - invalid()
 *      - compute_hashes()
 *
 * User Must Guarantee:
 *
 * 1. Write requires synchronized access (e.g. a lock)
 * 2. Read requires no concurrent Write, synchronized with last insert.
 * 3. Erase requires no concurrent Write, synchronized with last insert.
 * 4. An Erase caller must release all memory before allowing a new Writer.
 *
 *
 * Note on function names:
 *   - The name "allow_erase" is used because the real discard happens later.
 *   - The name "please_keep" is used because elements may be erased anyways on insert.
 *
 * @tparam Element should be a movable and copyable type
 * @tparam Hash should be a function/callable which takes a template parameter
 * hash_select and an Element and extracts a hash from it. Should return
 * high-entropy uint32_t hashes for `Hash h; h<0>(e) ... h<7>(e)`.
 */
template <typename Element, typename Hash>
class cache
{
private:
    /** table stores all the elements */
    std::vector<Element> table;

    /** size stores the total available slots in the hash table */
    uint32_t size{0};

    /** The bit_packed_atomic_flags array is marked mutable because we want
     * garbage collection to be allowed to occur from const methods */
    mutable bit_packed_atomic_flags collection_flags;

    /** epoch_flags tracks how recently an element was inserted into
     * the cache. true denotes recent, false denotes not-recent. See insert()
     * method for full semantics.
     */
    mutable std::vector<bool> epoch_flags;

    /** epoch_heuristic_counter is used to determine when an epoch might be aged
     * & an expensive scan should be done. epoch_heuristic_counter is
     * decremented on insert and reset to the new number of inserts which would
     * cause the epoch to reach epoch_size when it reaches zero.
     */
    uint32_t epoch_heuristic_counter{0};

    /** epoch_size is set to be the number of elements supposed to be in a
     * epoch. When the number of non-erased elements in an epoch
     * exceeds epoch_size, a new epoch should be started and all
     * current entries demoted. epoch_size is set to be 45% of size because
     * we want to keep load around 90%, and we support 3 epochs at once --
     * one "dead" which has been erased, one "dying" which has been marked to be
     * erased next, and one "living" which new inserts add to.
     */
    uint32_t epoch_size{0};

    /** depth_limit determines how many elements insert should try to replace.
     * Should be set to log2(n).
     */
    uint8_t depth_limit{0};

    /** hash_function is a const instance of the hash function. It cannot be
     * static or initialized at call time as it may have internal state (such as
     * a nonce).
     */
    const Hash hash_function;

    /** compute_hashes is convenience for not having to write out this
     * expression everywhere we use the hash values of an Element.
     *
     * We need to map the 32-bit input hash onto a hash bucket in a range [0, size) in a
     *  manner which preserves as much of the hash's uniformity as possible. Ideally
     *  this would be done by bitmasking but the size is usually not a power of two.
     *
     * The naive approach would be to use a mod -- which isn't perfectly uniform but so
     *  long as the hash is much larger than size it is not that bad. Unfortunately,
     *  mod/division is fairly slow on ordinary microprocessors (e.g. 90-ish cycles on
     *  haswell, ARM doesn't even have an instruction for it.); when the divisor is a
     *  constant the compiler will do clever tricks to turn it into a multiply+add+shift,
     *  but size is a run-time value so the compiler can't do that here.
     *
     * One option would be to implement the same trick the compiler uses and compute the
     *  constants for exact division based on the size, as described in "{N}-bit Unsigned
     *  Division via {N}-bit Multiply-Add" by Arch D. Robison in 2005. But that code is
     *  somewhat complicated and the result is still slower than an even simpler option:
     *  see the FastRange32 function in util/fastrange.h.
     *
     * The resulting non-uniformity is also more equally distributed which would be
     *  advantageous for something like linear probing, though it shouldn't matter
     *  one way or the other for a cuckoo table.
     *
     * The primary disadvantage of this approach is increased intermediate precision is
     *  required but for a 32-bit random number we only need the high 32 bits of a
     *  32*32->64 multiply, which means the operation is reasonably fast even on a
     *  typical 32-bit processor.
     *
     * @param e The element whose hashes will be returned
     * @returns Deterministic hashes derived from `e` uniformly mapped onto the range [0, size)
     */
    inline std::array<uint32_t, 8> compute_hashes(const Element& e) const
    {
        return {{FastRange32(hash_function.template operator()<0>(e), size),
                 FastRange32(hash_function.template operator()<1>(e), size),
                 FastRange32(hash_function.template operator()<2>(e), size),
                 FastRange32(hash_function.template operator()<3>(e), size),
                 FastRange32(hash_function.template operator()<4>(e), size),
                 FastRange32(hash_function.template operator()<5>(e), size),
                 FastRange32(hash_function.template operator()<6>(e), size),
                 FastRange32(hash_function.template operator()<7>(e), size)}};
    }

    /** invalid returns a special index that can never be inserted to
     * @returns the special constexpr index that can never be inserted to */
    constexpr uint32_t invalid() const
    {
        return ~(uint32_t)0;
    }

    /** allow_erase marks the element at index `n` as discardable. Threadsafe
     * without any concurrent insert.
     * @param n the index to allow erasure of
     */
    inline void allow_erase(uint32_t n) const
    {
        collection_flags.bit_set(n);
    }

    /** please_keep marks the element at index `n` as an entry that should be kept.
     * Threadsafe without any concurrent insert.
     * @param n the index to prioritize keeping
     */
    inline void please_keep(uint32_t n) const
    {
        collection_flags.bit_unset(n);
    }

    /** epoch_check handles the changing of epochs for elements stored in the
     * cache. epoch_check should be run before every insert.
     *
     * First, epoch_check decrements and checks the cheap heuristic, and then does
     * a more expensive scan if the cheap heuristic runs out. If the expensive
     * scan succeeds, the epochs are aged and old elements are allow_erased. The
     * cheap heuristic is reset to retrigger after the worst case growth of the
     * current epoch's elements would exceed the epoch_size.
     */
    void epoch_check()
    {
        if (epoch_heuristic_counter != 0) {
            --epoch_heuristic_counter;
            return;
        }
        // count the number of elements from the latest epoch which
        // have not been erased.
        uint32_t epoch_unused_count = 0;
        for (uint32_t i = 0; i < size; ++i)
            epoch_unused_count += epoch_flags[i] &&
                                  !collection_flags.bit_is_set(i);
        // If there are more non-deleted entries in the current epoch than the
        // epoch size, then allow_erase on all elements in the old epoch (marked
        // false) and move all elements in the current epoch to the old epoch
        // but do not call allow_erase on their indices.
        if (epoch_unused_count >= epoch_size) {
            for (uint32_t i = 0; i < size; ++i)
                if (epoch_flags[i])
                    epoch_flags[i] = false;
                else
                    allow_erase(i);
            epoch_heuristic_counter = epoch_size;
        } else
            // reset the epoch_heuristic_counter to next do a scan when worst
            // case behavior (no intermittent erases) would exceed epoch size,
            // with a reasonable minimum scan size.
            // Ordinarily, we would have to sanity check std::min(epoch_size,
            // epoch_unused_count), but we already know that `epoch_unused_count
            // < epoch_size` in this branch
            epoch_heuristic_counter = std::max(1u, std::max(epoch_size / 16,
                        epoch_size - epoch_unused_count));
    }

public:
    /** You must always construct a cache with some elements via a subsequent
     * call to setup or setup_bytes, otherwise operations may segfault.
     */
    cache() : table(), collection_flags(0), epoch_flags(), hash_function()
    {
    }

    /** setup initializes the container to store no more than new_size
     * elements and no less than 2 elements.
     *
     * setup should only be called once.
     *
     * @param new_size the desired number of elements to store
     * @returns the maximum number of elements storable
     */
    uint32_t setup(uint32_t new_size)
    {
        // depth_limit must be at least one otherwise errors can occur.
        size = std::max<uint32_t>(2, new_size);
        depth_limit = static_cast<uint8_t>(std::log2(static_cast<float>(size)));
        table.resize(size);
        collection_flags.setup(size);
        epoch_flags.resize(size);
        // Set to 45% as described above
        epoch_size = std::max(uint32_t{1}, (45 * size) / 100);
        // Initially set to wait for a whole epoch
        epoch_heuristic_counter = epoch_size;
        return size;
    }

    /** setup_bytes is a convenience function which accounts for internal memory
     * usage when deciding how many elements to store. It isn't perfect because
     * it doesn't account for any overhead (struct size, MallocUsage, collection
     * and epoch flags). This was done to simplify selecting a power of two
     * size. In the expected use case, an extra two bits per entry should be
     * negligible compared to the size of the elements.
     *
     * @param bytes the approximate number of bytes to use for this data
     * structure
     * @returns A pair of the maximum number of elements storable (see setup()
     * documentation for more detail) and the approximate total size of these
     * elements in bytes.
     */
    std::pair<uint32_t, size_t> setup_bytes(size_t bytes)
    {
        uint32_t requested_num_elems(std::min<size_t>(
            bytes / sizeof(Element),
            std::numeric_limits<uint32_t>::max()));

        auto num_elems = setup(requested_num_elems);

        size_t approx_size_bytes = num_elems * sizeof(Element);
        return std::make_pair(num_elems, approx_size_bytes);
    }

    /** insert loops at most depth_limit times trying to insert a hash
     * at various locations in the table via a variant of the Cuckoo Algorithm
     * with eight hash locations.
     *
     * It drops the last tried element if it runs out of depth before
     * encountering an open slot.
     *
     * Thus:
     *
     * ```
     * insert(x);
     * return contains(x, false);
     * ```
     *
     * is not guaranteed to return true.
     *
     * @param e the element to insert
     * @post one of the following: All previously inserted elements and e are
     * now in the table, one previously inserted element is evicted from the
     * table, the entry attempted to be inserted is evicted.
     */
    inline void insert(Element e)
    {
        epoch_check();
        uint32_t last_loc = invalid();
        bool last_epoch = true;
        std::array<uint32_t, 8> locs = compute_hashes(e);
        // Make sure we have not already inserted this element
        // If we have, make sure that it does not get deleted
        for (const uint32_t loc : locs)
            if (table[loc] == e) {
                please_keep(loc);
                epoch_flags[loc] = last_epoch;
                return;
            }
        for (uint8_t depth = 0; depth < depth_limit; ++depth) {
            // First try to insert to an empty slot, if one exists
            for (const uint32_t loc : locs) {
                if (!collection_flags.bit_is_set(loc))
                    continue;
                table[loc] = std::move(e);
                please_keep(loc);
                epoch_flags[loc] = last_epoch;
                return;
            }
            /** Swap with the element at the location that was
            * not the last one looked at. Example:
            *
            * 1. On first iteration, last_loc == invalid(), find returns last, so
            *    last_loc defaults to locs[0].
            * 2. On further iterations, where last_loc == locs[k], last_loc will
            *    go to locs[k+1 % 8], i.e., next of the 8 indices wrapping around
            *    to 0 if needed.
            *
            * This prevents moving the element we just put in.
            *
            * The swap is not a move -- we must switch onto the evicted element
            * for the next iteration.
            */
            last_loc = locs[(1 + (std::find(locs.begin(), locs.end(), last_loc) - locs.begin())) & 7];
            std::swap(table[last_loc], e);
            // Can't std::swap a std::vector<bool>::reference and a bool&.
            bool epoch = last_epoch;
            last_epoch = epoch_flags[last_loc];
            epoch_flags[last_loc] = epoch;

            // Recompute the locs -- unfortunately happens one too many times!
            locs = compute_hashes(e);
        }
    }

    /** contains iterates through the hash locations for a given element
     * and checks to see if it is present.
     *
     * contains does not check garbage collected state (in other words,
     * garbage is only collected when the space is needed), so:
     *
     * ```
     * insert(x);
     * if (contains(x, true))
     *     return contains(x, false);
     * else
     *     return true;
     * ```
     *
     * executed on a single thread will always return true!
     *
     * This is a great property for re-org performance for example.
     *
     * contains returns a bool set true if the element was found.
     *
     * @param e the element to check
     * @param erase whether to attempt setting the garbage collect flag
     *
     * @post if erase is true and the element is found, then the garbage collect
     * flag is set
     * @returns true if the element is found, false otherwise
     */
    inline bool contains(const Element& e, const bool erase) const
    {
        std::array<uint32_t, 8> locs = compute_hashes(e);
        for (const uint32_t loc : locs)
            if (table[loc] == e) {
                if (erase)
                    allow_erase(loc);
                return true;
            }
        return false;
    }
};
} // namespace CuckooCache

#endif // BITCOIN_CUCKOOCACHE_H
