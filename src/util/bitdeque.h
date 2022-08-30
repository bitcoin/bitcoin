// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_UTIL_BITDEQUE_H
#define SYSCOIN_UTIL_BITDEQUE_H

#include <bitset>
#include <cstddef>
#include <deque>
#include <limits>
#include <stdexcept>
#include <tuple>

/** Class that mimics std::deque<bool>, but with std::vector<bool>'s bit packing.
 *
 * BlobSize selects the (minimum) number of bits that are allocated at once.
 * Larger values reduce the asymptotic memory usage overhead, at the cost of
 * needing larger up-front allocations. The default is 4096 bytes.
 */
template<int BlobSize = 4096 * 8>
class bitdeque
{
    // Internal definitions
    using word_type = std::bitset<BlobSize>;
    using deque_type = std::deque<word_type>;
    static_assert(BlobSize > 0);
    static constexpr int BITS_PER_WORD = BlobSize;

    // Forward and friend declarations of iterator types.
    template<bool Const> class Iterator;
    template<bool Const> friend class Iterator;

    /** Iterator to a bitdeque element, const or not. */
    template<bool Const>
    class Iterator
    {
        using deque_iterator = std::conditional_t<Const, typename deque_type::const_iterator, typename deque_type::iterator>;

        deque_iterator m_it;
        int m_bitpos{0};
        Iterator(const deque_iterator& it, int bitpos) : m_it(it), m_bitpos(bitpos) {}
        friend class bitdeque;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = bool;
        using pointer = void;
        using const_pointer = void;
        using reference = std::conditional_t<Const, bool, typename word_type::reference>;
        using const_reference = bool;
        using difference_type = std::ptrdiff_t;

        /** Default constructor. */
        Iterator() = default;

        /** Default copy constructor. */
        Iterator(const Iterator&) = default;

        /** Conversion from non-const to const iterator. */
        template<bool ConstArg = Const, typename = std::enable_if_t<Const && ConstArg>>
        Iterator(const Iterator<false>& x) : m_it(x.m_it), m_bitpos(x.m_bitpos) {}

        Iterator& operator+=(difference_type dist)
        {
            if (dist > 0) {
                if (dist + m_bitpos >= BITS_PER_WORD) {
                    ++m_it;
                    dist -= BITS_PER_WORD - m_bitpos;
                    m_bitpos = 0;
                }
                auto jump = dist / BITS_PER_WORD;
                m_it += jump;
                m_bitpos += dist - jump * BITS_PER_WORD;
            } else if (dist < 0) {
                dist = -dist;
                if (dist > m_bitpos) {
                    --m_it;
                    dist -= m_bitpos + 1;
                    m_bitpos = BITS_PER_WORD - 1;
                }
                auto jump = dist / BITS_PER_WORD;
                m_it -= jump;
                m_bitpos -= dist - jump * BITS_PER_WORD;
            }
            return *this;
        }

        friend difference_type operator-(const Iterator& x, const Iterator& y)
        {
            return BITS_PER_WORD * (x.m_it - y.m_it) + x.m_bitpos - y.m_bitpos;
        }

        Iterator& operator=(const Iterator&) = default;
        Iterator& operator-=(difference_type dist) { return operator+=(-dist); }
        Iterator& operator++() { ++m_bitpos; if (m_bitpos == BITS_PER_WORD) { m_bitpos = 0; ++m_it; }; return *this; }
        Iterator operator++(int) { auto ret{*this}; operator++(); return ret; }
        Iterator& operator--() { if (m_bitpos == 0) { m_bitpos = BITS_PER_WORD; --m_it; }; --m_bitpos; return *this; }
        Iterator operator--(int) { auto ret{*this}; operator--(); return ret; }
        friend Iterator operator+(Iterator x, difference_type dist) { x += dist; return x; }
        friend Iterator operator+(difference_type dist, Iterator x) { x += dist; return x; }
        friend Iterator operator-(Iterator x, difference_type dist) { x -= dist; return x; }
        friend bool operator<(const Iterator& x, const Iterator& y) { return std::tie(x.m_it, x.m_bitpos) < std::tie(y.m_it, y.m_bitpos); }
        friend bool operator>(const Iterator& x, const Iterator& y) { return std::tie(x.m_it, x.m_bitpos) > std::tie(y.m_it, y.m_bitpos); }
        friend bool operator<=(const Iterator& x, const Iterator& y) { return std::tie(x.m_it, x.m_bitpos) <= std::tie(y.m_it, y.m_bitpos); }
        friend bool operator>=(const Iterator& x, const Iterator& y) { return std::tie(x.m_it, x.m_bitpos) >= std::tie(y.m_it, y.m_bitpos); }
        friend bool operator==(const Iterator& x, const Iterator& y) { return x.m_it == y.m_it && x.m_bitpos == y.m_bitpos; }
        friend bool operator!=(const Iterator& x, const Iterator& y) { return x.m_it != y.m_it || x.m_bitpos != y.m_bitpos; }
        reference operator*() const { return (*m_it)[m_bitpos]; }
        reference operator[](difference_type pos) const { return *(*this + pos); }
    };

public:
    using value_type = bool;
    using size_type = std::size_t;
    using difference_type = typename deque_type::difference_type;
    using reference = typename word_type::reference;
    using const_reference = bool;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using pointer = void;
    using const_pointer = void;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    /** Deque of bitsets storing the actual bit data. */
    deque_type m_deque;

    /** Number of unused bits at the front of m_deque.front(). */
    int m_pad_begin;

    /** Number of unused bits at the back of m_deque.back(). */
    int m_pad_end;

    /** Shrink the container by n bits, removing from the end. */
    void erase_back(size_type n)
    {
        if (n >= static_cast<size_type>(BITS_PER_WORD - m_pad_end)) {
            n -= BITS_PER_WORD - m_pad_end;
            m_pad_end = 0;
            m_deque.erase(m_deque.end() - 1 - (n / BITS_PER_WORD), m_deque.end());
            n %= BITS_PER_WORD;
        }
        if (n) {
            auto& last = m_deque.back();
            while (n) {
                last.reset(BITS_PER_WORD - 1 - m_pad_end);
                ++m_pad_end;
                --n;
            }
        }
    }

    /** Extend the container by n bits, adding at the end. */
    void extend_back(size_type n)
    {
        if (n > static_cast<size_type>(m_pad_end)) {
            n -= m_pad_end + 1;
            m_pad_end = BITS_PER_WORD - 1;
            m_deque.insert(m_deque.end(), 1 + (n / BITS_PER_WORD), {});
            n %= BITS_PER_WORD;
        }
        m_pad_end -= n;
    }

    /** Shrink the container by n bits, removing from the beginning. */
    void erase_front(size_type n)
    {
        if (n >= static_cast<size_type>(BITS_PER_WORD - m_pad_begin)) {
            n -= BITS_PER_WORD - m_pad_begin;
            m_pad_begin = 0;
            m_deque.erase(m_deque.begin(), m_deque.begin() + 1 + (n / BITS_PER_WORD));
            n %= BITS_PER_WORD;
        }
        if (n) {
            auto& first = m_deque.front();
            while (n) {
                first.reset(m_pad_begin);
                ++m_pad_begin;
                --n;
            }
        }
    }

    /** Extend the container by n bits, adding at the beginning. */
    void extend_front(size_type n)
    {
        if (n > static_cast<size_type>(m_pad_begin)) {
            n -= m_pad_begin + 1;
            m_pad_begin = BITS_PER_WORD - 1;
            m_deque.insert(m_deque.begin(), 1 + (n / BITS_PER_WORD), {});
            n %= BITS_PER_WORD;
        }
        m_pad_begin -= n;
    }

    /** Insert a sequence of falses anywhere in the container. */
    void insert_zeroes(size_type before, size_type count)
    {
        size_type after = size() - before;
        if (before < after) {
            extend_front(count);
            std::move(begin() + count, begin() + count + before, begin());
        } else {
            extend_back(count);
            std::move_backward(begin() + before, begin() + before + after, end());
        }
    }

public:
    /** Construct an empty container. */
    explicit bitdeque() : m_pad_begin{0}, m_pad_end{0} {}

    /** Set the container equal to count times the value of val. */
    void assign(size_type count, bool val)
    {
        m_deque.clear();
        m_deque.resize((count + BITS_PER_WORD - 1) / BITS_PER_WORD);
        m_pad_begin = 0;
        m_pad_end = 0;
        if (val) {
            for (auto& elem : m_deque) elem.flip();
        }
        if (count % BITS_PER_WORD) {
            erase_back(BITS_PER_WORD - (count % BITS_PER_WORD));
        }
    }

    /** Construct a container containing count times the value of val. */
    bitdeque(size_type count, bool val) { assign(count, val); }

    /** Construct a container containing count false values. */
    explicit bitdeque(size_t count) { assign(count, false); }

    /** Copy constructor. */
    bitdeque(const bitdeque&) = default;

    /** Move constructor. */
    bitdeque(bitdeque&&) noexcept = default;

    /** Copy assignment operator. */
    bitdeque& operator=(const bitdeque& other) = default;

    /** Move assignment operator. */
    bitdeque& operator=(bitdeque&& other) noexcept = default;

    // Iterator functions.
    iterator begin() noexcept { return {m_deque.begin(), m_pad_begin}; }
    iterator end() noexcept { return iterator{m_deque.end(), 0} - m_pad_end; }
    const_iterator begin() const noexcept { return const_iterator{m_deque.cbegin(), m_pad_begin}; }
    const_iterator cbegin() const noexcept { return const_iterator{m_deque.cbegin(), m_pad_begin}; }
    const_iterator end() const noexcept { return const_iterator{m_deque.cend(), 0} - m_pad_end; }
    const_iterator cend() const noexcept { return const_iterator{m_deque.cend(), 0} - m_pad_end; }
    reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
    reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{cend()}; }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{cend()}; }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator{cbegin()}; }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator{cbegin()}; }

    /** Count the number of bits in the container. */
    size_type size() const noexcept { return m_deque.size() * BITS_PER_WORD - m_pad_begin - m_pad_end; }

    /** Determine whether the container is empty. */
    bool empty() const noexcept
    {
        return m_deque.size() == 0 || (m_deque.size() == 1 && (m_pad_begin + m_pad_end == BITS_PER_WORD));
    }

    /** Return the maximum size of the container. */
    size_type max_size() const noexcept
    {
        if (m_deque.max_size() < std::numeric_limits<difference_type>::max() / BITS_PER_WORD) {
            return m_deque.max_size() * BITS_PER_WORD;
        } else {
            return std::numeric_limits<difference_type>::max();
        }
    }

    /** Set the container equal to the bits in [first,last). */
    template<typename It>
    void assign(It first, It last)
    {
        size_type count = std::distance(first, last);
        assign(count, false);
        auto it = begin();
        while (first != last) {
            *(it++) = *(first++);
        }
    }

    /** Set the container equal to the bits in ilist. */
    void assign(std::initializer_list<bool> ilist)
    {
        assign(ilist.size(), false);
        auto it = begin();
        auto init = ilist.begin();
        while (init != ilist.end()) {
            *(it++) = *(init++);
        }
    }

    /** Set the container equal to the bits in ilist. */
    bitdeque& operator=(std::initializer_list<bool> ilist)
    {
        assign(ilist);
        return *this;
    }

    /** Construct a container containing the bits in [first,last). */
    template<typename It>
    bitdeque(It first, It last) { assign(first, last); }

    /** Construct a container containing the bits in ilist. */
    bitdeque(std::initializer_list<bool> ilist) { assign(ilist); }

    // Access an element of the container, with bounds checking.
    reference at(size_type position)
    {
        if (position >= size()) throw std::out_of_range("bitdeque::at() out of range");
        return begin()[position];
    }
    const_reference at(size_type position) const
    {
        if (position >= size()) throw std::out_of_range("bitdeque::at() out of range");
        return cbegin()[position];
    }

    // Access elements of the container without bounds checking.
    reference operator[](size_type position) { return begin()[position]; }
    const_reference operator[](size_type position) const { return cbegin()[position]; }
    reference front() { return *begin(); }
    const_reference front() const { return *cbegin(); }
    reference back() { return end()[-1]; }
    const_reference back() const { return cend()[-1]; }

    /** Release unused memory. */
    void shrink_to_fit()
    {
        m_deque.shrink_to_fit();
    }

    /** Empty the container. */
    void clear() noexcept
    {
        m_deque.clear();
        m_pad_begin = m_pad_end = 0;
    }

    // Append an element to the container.
    void push_back(bool val)
    {
        extend_back(1);
        back() = val;
    }
    reference emplace_back(bool val)
    {
        extend_back(1);
        auto ref = back();
        ref = val;
        return ref;
    }

    // Prepend an element to the container.
    void push_front(bool val)
    {
        extend_front(1);
        front() = val;
    }
    reference emplace_front(bool val)
    {
        extend_front(1);
        auto ref = front();
        ref = val;
        return ref;
    }

    // Remove the last element from the container.
    void pop_back()
    {
        erase_back(1);
    }

    // Remove the first element from the container.
    void pop_front()
    {
        erase_front(1);
    }

    /** Resize the container. */
    void resize(size_type n)
    {
        if (n < size()) {
            erase_back(size() - n);
        } else {
            extend_back(n - size());
        }
    }

    // Swap two containers.
    void swap(bitdeque& other) noexcept
    {
        std::swap(m_deque, other.m_deque);
        std::swap(m_pad_begin, other.m_pad_begin);
        std::swap(m_pad_end, other.m_pad_end);
    }
    friend void swap(bitdeque& b1, bitdeque& b2) noexcept { b1.swap(b2); }

    // Erase elements from the container.
    iterator erase(const_iterator first, const_iterator last)
    {
        size_type before = std::distance(cbegin(), first);
        size_type dist = std::distance(first, last);
        size_type after = std::distance(last, cend());
        if (before < after) {
            std::move_backward(begin(), begin() + before, end() - after);
            erase_front(dist);
            return begin() + before;
        } else {
            std::move(end() - after, end(), begin() + before);
            erase_back(dist);
            return end() - after;
        }
    }

    iterator erase(iterator first, iterator last) { return erase(const_iterator{first}, const_iterator{last}); }
    iterator erase(const_iterator pos) { return erase(pos, pos + 1); }
    iterator erase(iterator pos) { return erase(const_iterator{pos}, const_iterator{pos} + 1); }

    // Insert elements into the container.
    iterator insert(const_iterator pos, bool val)
    {
        size_type before = pos - cbegin();
        insert_zeroes(before, 1);
        auto it = begin() + before;
        *it = val;
        return it;
    }

    iterator emplace(const_iterator pos, bool val) { return insert(pos, val); }

    iterator insert(const_iterator pos, size_type count, bool val)
    {
        size_type before = pos - cbegin();
        insert_zeroes(before, count);
        auto it_begin = begin() + before;
        auto it = it_begin;
        auto it_end = it + count;
        while (it != it_end) *(it++) = val;
        return it_begin;
    }

    template<typename It>
    iterator insert(const_iterator pos, It first, It last)
    {
        size_type before = pos - cbegin();
        size_type count = std::distance(first, last);
        insert_zeroes(before, count);
        auto it_begin = begin() + before;
        auto it = it_begin;
        while (first != last) {
            *(it++) = *(first++);
        }
        return it_begin;
    }
};

#endif // SYSCOIN_UTIL_BITDEQUE_H
