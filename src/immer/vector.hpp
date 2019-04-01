//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/rbts/rbtree.hpp>
#include <immer/detail/rbts/rbtree_iterator.hpp>
#include <immer/memory_policy.hpp>

#if IMMER_DEBUG_PRINT
#include <immer/flex_vector.hpp>
#endif

namespace immer {

template <typename T,
          typename MemoryPolicy,
          detail::rbts::bits_t B,
          detail::rbts::bits_t BL>
class flex_vector;

template <typename T,
          typename MemoryPolicy,
          detail::rbts::bits_t B,
          detail::rbts::bits_t BL>
class vector_transient;

/*!
 * Immutable sequential container supporting both random access and
 * structural sharing.
 *
 * @tparam T The type of the values to be stored in the container.
 * @tparam MemoryPolicy Memory management policy. See @ref
 *         memory_policy.
 *
 * @rst
 *
 * This cotainer provides a good trade-off between cache locality,
 * random access, update performance and structural sharing.  It does
 * so by storing the data in contiguous chunks of :math:`2^{BL}`
 * elements.  By default, when ``sizeof(T) == sizeof(void*)`` then
 * :math:`B=BL=5`, such that data would be stored in contiguous
 * chunks of :math:`32` elements.
 *
 * You may learn more about the meaning and implications of ``B`` and
 * ``BL`` parameters in the :doc:`implementation` section.
 *
 * .. note:: In several methods we say that their complexity is
 *    *effectively* :math:`O(...)`. Do not confuse this with the word
 *    *amortized*, which has a very different meaning.  In this
 *    context, *effective* means that while the
 *    mathematically rigurous
 *    complexity might be higher, for all practical matters the
 *    provided complexity is more useful to think about the actual
 *    cost of the operation.
 *
 * **Example**
 *   .. literalinclude:: ../example/vector/intro.cpp
 *      :language: c++
 *      :start-after: intro/start
 *      :end-before:  intro/end
 *
 * @endrst
 */
template <typename T,
          typename MemoryPolicy   = default_memory_policy,
          detail::rbts::bits_t B  = default_bits,
          detail::rbts::bits_t BL = detail::rbts::derive_bits_leaf<T, MemoryPolicy, B>>
class vector
{
    using impl_t = detail::rbts::rbtree<T, MemoryPolicy, B, BL>;
    using flex_t = flex_vector<T, MemoryPolicy, B, BL>;

    using move_t =
        std::integral_constant<bool, MemoryPolicy::use_transient_rvalues>;

public:
    static constexpr auto bits = B;
    static constexpr auto bits_leaf = BL;
    using memory_policy = MemoryPolicy;

    using value_type = T;
    using reference = const T&;
    using size_type = detail::rbts::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::rbts::rbtree_iterator<T, MemoryPolicy, B, BL>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using transient_type   = vector_transient<T, MemoryPolicy, B, BL>;

    /*!
     * Default constructor.  It creates a vector of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    vector() = default;

    /*!
     * Constructs a vector containing the elements in `values`.
     */
    vector(std::initializer_list<T> values)
        : impl_{impl_t::from_initializer_list(values)}
    {}

    /*!
     * Constructs a vector containing the elements in the range
     * defined by the input iterator `first` and range sentinel `last`.
     */
    template <typename Iter, typename Sent,
              std::enable_if_t
              <detail::compatible_sentinel_v<Iter, Sent>, bool> = true>
    vector(Iter first, Sent last)
        : impl_{impl_t::from_range(first, last)}
    {}

    /*!
     * Constructs a vector containing the element `val` repeated `n`
     * times.
     */
    vector(size_type n, T v = {})
        : impl_{impl_t::from_fill(n, v)}
    {}

    /*!
     * Returns an iterator pointing at the first element of the
     * collection. It does not allocate memory and its complexity is
     * @f$ O(1) @f$.
     */
    iterator begin() const { return {impl_}; }

    /*!
     * Returns an iterator pointing just after the last element of the
     * collection. It does not allocate and its complexity is @f$ O(1) @f$.
     */
    iterator end() const { return {impl_, typename iterator::end_t{}}; }

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
    size_type size() const { return impl_.size; }

    /*!
     * Returns `true` if there are no elements in the container.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    bool empty() const { return impl_.size == 0; }

    /*!
     * Access the last element.
     */
    const T& back() const { return impl_.back(); }

    /*!
     * Access the first element.
     */
    const T& front() const { return impl_.front(); }

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
    bool operator==(const vector& other) const
    { return impl_.equals(other.impl_); }
    bool operator!=(const vector& other) const
    { return !(*this == other); }

    /*!
     * Returns a vector with `value` inserted at the end.  It may
     * allocate memory and its complexity is *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/vector/vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: push-back/start
     *      :end-before:  push-back/end
     *
     * @endrst
     */
    vector push_back(value_type value) const&
    { return impl_.push_back(std::move(value)); }

    decltype(auto) push_back(value_type value) &&
    { return push_back_move(move_t{}, std::move(value)); }

    /*!
     * Returns a vector containing value `value` at position `idx`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/vector/vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: set/start
     *      :end-before:  set/end
     *
     * @endrst
     */
    vector set(size_type index, value_type value) const&
    { return impl_.assoc(index, std::move(value)); }

    decltype(auto) set(size_type index, value_type value) &&
    { return set_move(move_t{}, index, std::move(value)); }

    /*!
     * Returns a vector containing the result of the expression
     * `fn((*this)[idx])` at position `idx`.
     * Undefined for `0 >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/vector/vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: update/start
     *      :end-before:  update/end
     *
     * @endrst
     */
    template <typename FnT>
    vector update(size_type index, FnT&& fn) const&
    { return impl_.update(index, std::forward<FnT>(fn)); }

    template <typename FnT>
    decltype(auto) update(size_type index, FnT&& fn) &&
    { return update_move(move_t{}, index, std::forward<FnT>(fn)); }

    /*!
     * Returns a vector containing only the first `min(elems, size())`
     * elements. It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/vector/vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: take/start
     *      :end-before:  take/end
     *
     * @endrst
     */
    vector take(size_type elems) const&
    { return impl_.take(elems); }

    decltype(auto) take(size_type elems) &&
    { return take_move(move_t{}, elems); }

    /*!
     * Returns an @a transient form of this container, an
     * `immer::vector_transient`.
     */
    transient_type transient() const&
    { return transient_type{ impl_ }; }
    transient_type transient() &&
    { return transient_type{ std::move(impl_) }; }

    // Semi-private
    const impl_t& impl() const { return impl_; }

#if IMMER_DEBUG_PRINT
    void debug_print(std::ostream& out=std::cerr) const
    { flex_t{*this}.debug_print(out); }
#endif

private:
    friend flex_t;
    friend transient_type;

    vector(impl_t impl)
        : impl_(std::move(impl))
    {
#if IMMER_DEBUG_PRINT
        // force the compiler to generate debug_print, so we can call
        // it from a debugger
        [](volatile auto){}(&vector::debug_print);
#endif
    }

    vector&& push_back_move(std::true_type, value_type value)
    { impl_.push_back_mut({}, std::move(value)); return std::move(*this); }
    vector push_back_move(std::false_type, value_type value)
    { return impl_.push_back(std::move(value)); }

    vector&& set_move(std::true_type, size_type index, value_type value)
    { impl_.assoc_mut({}, index, std::move(value)); return std::move(*this); }
    vector set_move(std::false_type, size_type index, value_type value)
    { return impl_.assoc(index, std::move(value)); }

    template <typename Fn>
    vector&& update_move(std::true_type, size_type index, Fn&& fn)
    { impl_.update_mut({}, index, std::forward<Fn>(fn)); return std::move(*this); }
    template <typename Fn>
    vector update_move(std::false_type, size_type index, Fn&& fn)
    { return impl_.update(index, std::forward<Fn>(fn)); }

    vector&& take_move(std::true_type, size_type elems)
    { impl_.take_mut({}, elems); return std::move(*this); }
    vector take_move(std::false_type, size_type elems)
    { return impl_.take(elems); }

    impl_t impl_ = impl_t::empty();
};

} // namespace immer
