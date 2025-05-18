// Copyright (c) The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_UTIL_VECDEQUE_H
#define TORTOISECOIN_UTIL_VECDEQUE_H

#include <util/check.h>

#include <cstring>
#include <memory>
#include <type_traits>

/** Data structure largely mimicking std::deque, but using single preallocated ring buffer.
 *
 * - More efficient and better memory locality than std::deque.
 * - Most operations ({push_,pop_,emplace_,}{front,back}(), operator[], ...) are O(1),
 *   unless reallocation is needed (in which case they are O(n)).
 * - Supports reserve(), capacity(), shrink_to_fit() like vectors.
 * - No iterator support.
 * - Data is not stored in a single contiguous block, so no data().
 */
template<typename T>
class VecDeque
{
    /** Pointer to allocated memory. Can contain constructed and uninitialized T objects. */
    T* m_buffer{nullptr};
    /** m_buffer + m_offset points to first object in queue. m_offset = 0 if m_capacity is 0;
     *  otherwise 0 <= m_offset < m_capacity. */
    size_t m_offset{0};
    /** Number of objects in the container. 0 <= m_size <= m_capacity. */
    size_t m_size{0};
    /** The size of m_buffer, expressed as a multiple of the size of T. */
    size_t m_capacity{0};

    /** Returns the number of populated objects between m_offset and the end of the buffer. */
    size_t FirstPart() const noexcept { return std::min(m_capacity - m_offset, m_size); }

    void Reallocate(size_t capacity)
    {
        Assume(capacity >= m_size);
        Assume((m_offset == 0 && m_capacity == 0) || m_offset < m_capacity);
        // Allocate new buffer.
        T* new_buffer = capacity ? std::allocator<T>().allocate(capacity) : nullptr;
        if (capacity) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                // When T is trivially copyable, just copy the data over from old to new buffer.
                size_t first_part = FirstPart();
                if (first_part != 0) {
                    std::memcpy(new_buffer, m_buffer + m_offset, first_part * sizeof(T));
                }
                if (first_part != m_size) {
                    std::memcpy(new_buffer + first_part, m_buffer, (m_size - first_part) * sizeof(T));
                }
            } else {
                // Otherwise move-construct in place in the new buffer, and destroy old buffer objects.
                size_t old_pos = m_offset;
                for (size_t new_pos = 0; new_pos < m_size; ++new_pos) {
                    std::construct_at(new_buffer + new_pos, std::move(*(m_buffer + old_pos)));
                    std::destroy_at(m_buffer + old_pos);
                    ++old_pos;
                    if (old_pos == m_capacity) old_pos = 0;
                }
            }
        }
        // Deallocate old buffer and update housekeeping.
        std::allocator<T>().deallocate(m_buffer, m_capacity);
        m_buffer = new_buffer;
        m_offset = 0;
        m_capacity = capacity;
        Assume((m_offset == 0 && m_capacity == 0) || m_offset < m_capacity);
    }

    /** What index in the buffer does logical entry number pos have? */
    size_t BufferIndex(size_t pos) const noexcept
    {
        Assume(pos < m_capacity);
        // The expression below is used instead of the more obvious (pos + m_offset >= m_capacity),
        // because the addition there could in theory overflow with very large deques.
        if (pos >= m_capacity - m_offset) {
            return (m_offset + pos) - m_capacity;
        } else {
            return m_offset + pos;
        }
    }

    /** Specialization of resize() that can only shrink. Separate so that clear() can call it
     *  without requiring a default T constructor. */
    void ResizeDown(size_t size) noexcept
    {
        Assume(size <= m_size);
        if constexpr (std::is_trivially_destructible_v<T>) {
            // If T is trivially destructible, we do not need to do anything but update the
            // housekeeping record. Default constructor or zero-filling will be used when
            // the space is reused.
            m_size = size;
        } else {
            // If not, we need to invoke the destructor for every element separately.
            while (m_size > size) {
                std::destroy_at(m_buffer + BufferIndex(m_size - 1));
                --m_size;
            }
        }
    }

public:
    VecDeque() noexcept = default;

    /** Resize the deque to be exactly size size (adding default-constructed elements if needed). */
    void resize(size_t size)
    {
        if (size < m_size) {
            // Delegate to ResizeDown when shrinking.
            ResizeDown(size);
        } else if (size > m_size) {
            // When growing, first see if we need to allocate more space.
            if (size > m_capacity) Reallocate(size);
            while (m_size < size) {
                std::construct_at(m_buffer + BufferIndex(m_size));
                ++m_size;
            }
        }
    }

    /** Resize the deque to be size 0. The capacity will remain unchanged. */
    void clear() noexcept { ResizeDown(0); }

    /** Destroy a deque. */
    ~VecDeque()
    {
        clear();
        Reallocate(0);
    }

    /** Copy-assign a deque. */
    VecDeque& operator=(const VecDeque& other)
    {
        if (&other == this) [[unlikely]] return *this;
        clear();
        Reallocate(other.m_size);
        if constexpr (std::is_trivially_copyable_v<T>) {
            size_t first_part = other.FirstPart();
            Assume(first_part > 0 || m_size == 0);
            if (first_part != 0) {
                std::memcpy(m_buffer, other.m_buffer + other.m_offset, first_part * sizeof(T));
            }
            if (first_part != other.m_size) {
                std::memcpy(m_buffer + first_part, other.m_buffer, (other.m_size - first_part) * sizeof(T));
            }
            m_size = other.m_size;
        } else {
            while (m_size < other.m_size) {
                std::construct_at(m_buffer + BufferIndex(m_size), other[m_size]);
                ++m_size;
            }
        }
        return *this;
    }

    /** Swap two deques. */
    void swap(VecDeque& other) noexcept
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_offset, other.m_offset);
        std::swap(m_size, other.m_size);
        std::swap(m_capacity, other.m_capacity);
    }

    /** Non-member version of swap. */
    friend void swap(VecDeque& a, VecDeque& b) noexcept { a.swap(b); }

    /** Move-assign a deque. */
    VecDeque& operator=(VecDeque&& other) noexcept
    {
        swap(other);
        return *this;
    }

    /** Copy-construct a deque. */
    VecDeque(const VecDeque& other) { *this = other; }
    /** Move-construct a deque. */
    VecDeque(VecDeque&& other) noexcept { swap(other); }

    /** Equality comparison between two deques (only compares size+contents, not capacity). */
    bool friend operator==(const VecDeque& a, const VecDeque& b)
    {
        if (a.m_size != b.m_size) return false;
        for (size_t i = 0; i < a.m_size; ++i) {
            if (a[i] != b[i]) return false;
        }
        return true;
    }

    /** Comparison between two deques, implementing lexicographic ordering on the contents. */
    std::strong_ordering friend operator<=>(const VecDeque& a, const VecDeque& b)
    {
        size_t pos_a{0}, pos_b{0};
        while (pos_a < a.m_size && pos_b < b.m_size) {
            auto cmp = a[pos_a++] <=> b[pos_b++];
            if (cmp != 0) return cmp;
        }
        return a.m_size <=> b.m_size;
    }

    /** Increase the capacity to capacity. Capacity will not shrink. */
    void reserve(size_t capacity)
    {
        if (capacity > m_capacity) Reallocate(capacity);
    }

    /** Make the capacity equal to the size. The contents does not change. */
    void shrink_to_fit()
    {
        if (m_capacity > m_size) Reallocate(m_size);
    }

    /** Construct a new element at the end of the deque. */
    template<typename... Args>
    void emplace_back(Args&&... args)
    {
        if (m_size == m_capacity) Reallocate((m_size + 1) * 2);
        std::construct_at(m_buffer + BufferIndex(m_size), std::forward<Args>(args)...);
        ++m_size;
    }

    /** Move-construct a new element at the end of the deque. */
    void push_back(T&& elem) { emplace_back(std::move(elem)); }

    /** Copy-construct a new element at the end of the deque. */
    void push_back(const T& elem) { emplace_back(elem); }

    /** Construct a new element at the beginning of the deque. */
    template<typename... Args>
    void emplace_front(Args&&... args)
    {
        if (m_size == m_capacity) Reallocate((m_size + 1) * 2);
        std::construct_at(m_buffer + BufferIndex(m_capacity - 1), std::forward<Args>(args)...);
        if (m_offset == 0) m_offset = m_capacity;
        --m_offset;
        ++m_size;
    }

    /** Copy-construct a new element at the beginning of the deque. */
    void push_front(const T& elem) { emplace_front(elem); }

    /** Move-construct a new element at the beginning of the deque. */
    void push_front(T&& elem) { emplace_front(std::move(elem)); }

    /** Remove the first element of the deque. Requires !empty(). */
    void pop_front()
    {
        Assume(m_size);
        std::destroy_at(m_buffer + m_offset);
        --m_size;
        ++m_offset;
        if (m_offset == m_capacity) m_offset = 0;
    }

    /** Remove the last element of the deque. Requires !empty(). */
    void pop_back()
    {
        Assume(m_size);
        std::destroy_at(m_buffer + BufferIndex(m_size - 1));
        --m_size;
    }

    /** Get a mutable reference to the first element of the deque. Requires !empty(). */
    T& front() noexcept
    {
        Assume(m_size);
        return m_buffer[m_offset];
    }

    /** Get a const reference to the first element of the deque. Requires !empty(). */
    const T& front() const noexcept
    {
        Assume(m_size);
        return m_buffer[m_offset];
    }

    /** Get a mutable reference to the last element of the deque. Requires !empty(). */
    T& back() noexcept
    {
        Assume(m_size);
        return m_buffer[BufferIndex(m_size - 1)];
    }

    /** Get a const reference to the last element of the deque. Requires !empty(). */
    const T& back() const noexcept
    {
        Assume(m_size);
        return m_buffer[BufferIndex(m_size - 1)];
    }

    /** Get a mutable reference to the element in the deque at the given index. Requires idx < size(). */
    T& operator[](size_t idx) noexcept
    {
        Assume(idx < m_size);
        return m_buffer[BufferIndex(idx)];
    }

    /** Get a const reference to the element in the deque at the given index. Requires idx < size(). */
    const T& operator[](size_t idx) const noexcept
    {
        Assume(idx < m_size);
        return m_buffer[BufferIndex(idx)];
    }

    /** Test whether the contents of this deque is empty. */
    bool empty() const noexcept { return m_size == 0; }
    /** Get the number of elements in this deque. */
    size_t size() const noexcept { return m_size; }
    /** Get the capacity of this deque (maximum size it can have without reallocating). */
    size_t capacity() const noexcept { return m_capacity; }
};

#endif // TORTOISECOIN_UTIL_VECDEQUE_H
