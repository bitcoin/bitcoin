//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/memory_policy.hpp>
#include <immer/detail/arrays/with_capacity.hpp>

namespace immer {

template <typename T, typename MemoryPolicy>
class array_transient;

/*!
 * Immutable container that stores a sequence of elements in
 * contiguous memory.
 *
 * @tparam T The type of the values to be stored in the container.
 *
 * @rst
 *
 * It supports the most efficient iteration and random access,
 * equivalent to a ``std::vector`` or ``std::array``, but all
 * manipulations are :math:`O(size)`.
 *
 * .. tip:: Don't be fooled by the bad complexity of this data
 *    structure.  It is a great choice for short sequence or when it
 *    is seldom or never changed.  This depends on the ``sizeof(T)``
 *    and the expensiveness of its ``T``'s copy constructor, in case
 *    of doubt, measure.  For basic types, using an `array` when
 *    :math:`n < 100` is a good heuristic.
 *
 * .. warning:: The current implementation depends on
 *    ``boost::intrusive_ptr`` and does not support :doc:`memory
 *    policies<memory>`.  This will be fixed soon.
 *
 * @endrst
 */
template <typename T, typename MemoryPolicy = default_memory_policy>
class array
{
    using impl_t = std::conditional_t<
        MemoryPolicy::use_transient_rvalues,
        detail::arrays::with_capacity<T, MemoryPolicy>,
        detail::arrays::no_capacity<T, MemoryPolicy>>;

    using move_t =
        std::integral_constant<bool, MemoryPolicy::use_transient_rvalues>;

public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = const T*;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using memory_policy  = MemoryPolicy;
    using transient_type = array_transient<T, MemoryPolicy>;

    /*!
     * Default constructor.  It creates an array of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    array() = default;

    /*!
     * Constructs a vector containing the elements in `values`.
     */
    array(std::initializer_list<T> values)
        : impl_{impl_t::from_initializer_list(values)}
    {}

    /*!
     * Constructs a vector containing the elements in the range
     * defined by the input iterators `first` and `last`.
     */
    template <typename Iter>
    array(Iter first, Iter last)
        : impl_{impl_t::from_range(first, last)}
    {}

    /*!
     * Constructs a vector containing the element `val` repeated `n`
     * times.
     */
    array(size_type n, T v = {})
        : impl_{impl_t::from_fill(n, v)}
    {}

    /*!
     * Returns an iterator pointing at the first element of the
     * collection. It does not allocate memory and its complexity is
     * @f$ O(1) @f$.
     */
    iterator begin() const { return impl_.data(); }

    /*!
     * Returns an iterator pointing just after the last element of the
     * collection. It does not allocate and its complexity is @f$ O(1) @f$.
     */
    iterator end()   const { return impl_.data() + impl_.size; }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing at the first element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    reverse_iterator rbegin() const { return reverse_iterator{end()}; }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing after the last element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    /*!
     * Returns the number of elements in the container.  It does
     * not allocate memory and its complexity is @f$ O(1) @f$.
     */
    std::size_t size() const { return impl_.size; }

    /*!
     * Returns `true` if there are no elements in the container.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    bool empty() const { return impl_.d->empty(); }

    /*!
     * Access the raw data.
     */
    const T* data() const { return impl_.data(); }

    /*!
     * Access the last element.
     */
    const T& back() const { return data()[size() - 1]; }

    /*!
     * Access the first element.
     */
    const T& front() const { return data()[0]; }

    /*!
     * Returns a `const` reference to the element at position `index`.
     * It is undefined when @f$ 0 index \geq size() @f$.  It does not
     * allocate memory and its complexity is *effectively* @f$ O(1)
     * @f$.
     */
    reference operator[] (size_type index) const
    { return impl_.get(index); }

    /*!
     * Returns a `const` reference to the element at position
     * `index`. It throws an `std::out_of_range` exception when @f$
     * index \geq size() @f$.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    reference at(size_type index) const
    { return impl_.get_check(index); }

    /*!
     * Returns whether the vectors are equal.
     */
    bool operator==(const array& other) const
    { return impl_.equals(other.impl_); }
    bool operator!=(const array& other) const
    { return !(*this == other); }

    /*!
     * Returns an array with `value` inserted at the end.  It may
     * allocate memory and its complexity is @f$ O(size) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/array/array.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: push-back/start
     *      :end-before:  push-back/end
     *
     * @endrst
     */
    array push_back(value_type value) const&
    { return impl_.push_back(std::move(value)); }

    decltype(auto) push_back(value_type value) &&
    { return push_back_move(move_t{}, std::move(value)); }

    /*!
     * Returns an array containing value `value` at position `idx`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is @f$ O(size) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/array/array.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: set/start
     *      :end-before:  set/end
     *
     * @endrst
     */
    array set(std::size_t index, value_type value) const&
    { return impl_.assoc(index, std::move(value)); }

    decltype(auto) set(size_type index, value_type value) &&
    { return set_move(move_t{}, index, std::move(value)); }

    /*!
     * Returns an array containing the result of the expression
     * `fn((*this)[idx])` at position `idx`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is @f$ O(size) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/array/array.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: update/start
     *      :end-before:  update/end
     *
     * @endrst
     */
    template <typename FnT>
    array update(std::size_t index, FnT&& fn) const&
    { return impl_.update(index, std::forward<FnT>(fn)); }

    template <typename FnT>
    decltype(auto) update(size_type index, FnT&& fn) &&
    { return update_move(move_t{}, index, std::forward<FnT>(fn)); }

    /*!
     * Returns a array containing only the first `min(elems, size())`
     * elements. It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/array/array.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: take/start
     *      :end-before:  take/end
     *
     * @endrst
     */
    array take(size_type elems) const&
    { return impl_.take(elems); }

    decltype(auto) take(size_type elems) &&
    { return take_move(move_t{}, elems); }

    /*!
     * Returns an @a transient form of this container, an
     * `immer::array_transient`.
     */
    transient_type transient() const&
    { return transient_type{ impl_ }; }
    transient_type transient() &&
    { return transient_type{ std::move(impl_) }; }

    // Semi-private
    const impl_t& impl() const { return impl_; }

private:
    friend transient_type;

    array(impl_t impl) : impl_(std::move(impl)) {}

    array&& push_back_move(std::true_type, value_type value)
    { impl_.push_back_mut({}, std::move(value)); return std::move(*this); }
    array push_back_move(std::false_type, value_type value)
    { return impl_.push_back(std::move(value)); }

    array&& set_move(std::true_type, size_type index, value_type value)
    { impl_.assoc_mut({}, index, std::move(value)); return std::move(*this); }
    array set_move(std::false_type, size_type index, value_type value)
    { return impl_.assoc(index, std::move(value)); }

    template <typename Fn>
    array&& update_move(std::true_type, size_type index, Fn&& fn)
    { impl_.update_mut({}, index, std::forward<Fn>(fn)); return std::move(*this); }
    template <typename Fn>
    array update_move(std::false_type, size_type index, Fn&& fn)
    { return impl_.update(index, std::forward<Fn>(fn)); }

    array&& take_move(std::true_type, size_type elems)
    { impl_.take_mut({}, elems); return std::move(*this); }
    array take_move(std::false_type, size_type elems)
    { return impl_.take(elems); }

    impl_t impl_ = impl_t::empty;
};

} /* namespace immer */
