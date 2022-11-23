//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/arrays/with_capacity.hpp>
#include <immer/memory_policy.hpp>

#include <cstddef>

namespace immer {

template <typename T, typename MemoryPolicy>
class array;

/*!
 * Mutable version of `immer::array`.
 *
 * @rst
 *
 * Refer to :doc:`transients` to learn more about when and how to use
 * the mutable versions of immutable containers.
 *
 * @endrst
 */
template <typename T, typename MemoryPolicy = default_memory_policy>
class array_transient : MemoryPolicy::transience_t::owner
{
    using impl_t             = detail::arrays::with_capacity<T, MemoryPolicy>;
    using impl_no_capacity_t = detail::arrays::no_capacity<T, MemoryPolicy>;
    using owner_t            = typename MemoryPolicy::transience_t::owner;

public:
    using value_type      = T;
    using reference       = const T&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = const T*;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using memory_policy   = MemoryPolicy;
    using persistent_type = array<T, MemoryPolicy>;

    /*!
     * Default constructor.  It creates a mutable array of `size() ==
     * 0`.  It does not allocate memory and its complexity is
     * @f$ O(1) @f$.
     */
    array_transient() = default;

    /*!
     * Returns an iterator pointing at the first element of the
     * collection. It does not allocate memory and its complexity is
     * @f$ O(1) @f$.
     */
    IMMER_NODISCARD iterator begin() const { return impl_.data(); }

    /*!
     * Returns an iterator pointing just after the last element of the
     * collection. It does not allocate and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD iterator end() const { return impl_.data() + impl_.size; }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing at the first element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD reverse_iterator rbegin() const
    {
        return reverse_iterator{end()};
    }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing after the last element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD reverse_iterator rend() const
    {
        return reverse_iterator{begin()};
    }

    /*!
     * Returns the number of elements in the container.  It does
     * not allocate memory and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD std::size_t size() const { return impl_.size; }

    /*!
     * Returns `true` if there are no elements in the container.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD bool empty() const { return impl_.size == 0; }

    /*!
     * Access the raw data.
     */
    IMMER_NODISCARD const T* data() const { return impl_.data(); }

    /*!
     * Provide mutable access to the raw underlaying data.
     */
    IMMER_NODISCARD T* data_mut() { return impl_.data_mut(*this); }

    /*!
     * Access the last element.
     */
    IMMER_NODISCARD const T& back() const { return data()[size() - 1]; }

    /*!
     * Access the first element.
     */
    IMMER_NODISCARD const T& front() const { return data()[0]; }

    /*!
     * Returns a `const` reference to the element at position `index`.
     * It is undefined when @f$ 0 index \geq size() @f$.  It does not
     * allocate memory and its complexity is *effectively* @f$ O(1)
     * @f$.
     */
    reference operator[](size_type index) const { return impl_.get(index); }

    /*!
     * Returns a `const` reference to the element at position
     * `index`. It throws an `std::out_of_range` exception when @f$
     * index \geq size() @f$.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    reference at(size_type index) const { return impl_.get_check(index); }

    /*!
     * Inserts `value` at the end.  It may allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    void push_back(value_type value)
    {
        impl_.push_back_mut(*this, std::move(value));
    }

    /*!
     * Sets to the value `value` at position `idx`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    void set(size_type index, value_type value)
    {
        impl_.assoc_mut(*this, index, std::move(value));
    }

    /*!
     * Updates the array to contain the result of the expression
     * `fn((*this)[idx])` at position `idx`.
     * Undefined for `0 >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    template <typename FnT>
    void update(size_type index, FnT&& fn)
    {
        impl_.update_mut(*this, index, std::forward<FnT>(fn));
    }

    /*!
     * Resizes the array to only contain the first `min(elems, size())`
     * elements. It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    void take(size_type elems) { impl_.take_mut(*this, elems); }

    /*!
     * Returns an @a immutable form of this container, an
     * `immer::array`.
     */
    IMMER_NODISCARD persistent_type persistent() &
    {
        this->owner_t::operator=(owner_t{});
        return persistent_type{impl_};
    }
    IMMER_NODISCARD persistent_type persistent() &&
    {
        return persistent_type{std::move(impl_)};
    }

private:
    friend persistent_type;

    array_transient(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();
};

} // namespace immer
