//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/hamts/champ.hpp>
#include <immer/detail/hamts/champ_iterator.hpp>
#include <immer/memory_policy.hpp>

#include <functional>

namespace immer {

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          detail::hamts::bits_t B>
class set_transient;

/*!
 * Immutable set representing an unordered bag of values.
 *
 * @tparam T    The type of the values to be stored in the container.
 * @tparam Hash The type of a function object capable of hashing
 *              values of type `T`.
 * @tparam Equal The type of a function object capable of comparing
 *              values of type `T`.
 * @tparam MemoryPolicy Memory management policy. See @ref
 *              memory_policy.
 *
 * @rst
 *
 * This container provides a good trade-off between cache locality,
 * membership checks, update performance and structural sharing.  It
 * does so by storing the data in contiguous chunks of :math:`2^{B}`
 * elements.  When storing big objects, the size of these contiguous
 * chunks can become too big, damaging performance.  If this is
 * measured to be problematic for a specific use-case, it can be
 * solved by using a `immer::box` to wrap the type `T`.
 *
 * **Example**
 *   .. literalinclude:: ../example/set/intro.cpp
 *      :language: c++
 *      :start-after: intro/start
 *      :end-before:  intro/end
 *
 * @endrst
 *
 */
template <typename T,
          typename Hash           = std::hash<T>,
          typename Equal          = std::equal_to<T>,
          typename MemoryPolicy   = default_memory_policy,
          detail::hamts::bits_t B = default_bits>
class set
{
    using impl_t = detail::hamts::champ<T, Hash, Equal, MemoryPolicy, B>;

    using move_t =
        std::integral_constant<bool, MemoryPolicy::use_transient_rvalues>;

    struct project_value_ptr
    {
        const T* operator()(const T& v) const noexcept { return &v; }
    };

public:
    using value_type      = T;
    using size_type       = detail::hamts::size_t;
    using diference_type  = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = Equal;
    using reference       = const T&;
    using const_reference = const T&;

    using iterator =
        detail::hamts::champ_iterator<T, Hash, Equal, MemoryPolicy, B>;
    using const_iterator = iterator;

    using transient_type = set_transient<T, Hash, Equal, MemoryPolicy, B>;

    using memory_policy_type = MemoryPolicy;

    /*!
     * Default constructor.  It creates a set of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    set() = default;

    /*!
     * Constructs a set containing the elements in `values`.
     */
    set(std::initializer_list<value_type> values)
        : impl_{impl_t::from_initializer_list(values)}
    {}

    /*!
     * Constructs a set containing the elements in the range
     * defined by the input iterator `first` and range sentinel `last`.
     */
    template <typename Iter,
              typename Sent,
              std::enable_if_t<detail::compatible_sentinel_v<Iter, Sent>,
                               bool> = true>
    set(Iter first, Sent last)
        : impl_{impl_t::from_range(first, last)}
    {}

    /*!
     * Returns an iterator pointing at the first element of the
     * collection. It does not allocate memory and its complexity is
     * @f$ O(1) @f$.
     */
    IMMER_NODISCARD iterator begin() const { return {impl_}; }

    /*!
     * Returns an iterator pointing just after the last element of the
     * collection. It does not allocate and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD iterator end() const
    {
        return {impl_, typename iterator::end_t{}};
    }

    /*!
     * Returns the number of elements in the container.  It does
     * not allocate memory and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD size_type size() const { return impl_.size; }

    /*!
     * Returns `true` if there are no elements in the container.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD bool empty() const { return impl_.size == 0; }

    /*!
     * Returns `1` when `value` is contained in the set or `0`
     * otherwise. It won't allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * This overload participates in overload resolution only if
     * `Hash::is_transparent` is valid and denotes a type.
     */
    template <typename K,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD size_type count(const K& value) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(value);
    }

    /*!
     * Returns `1` when `value` is contained in the set or `0`
     * otherwise. It won't allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD size_type count(const T& value) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(value);
    }

    /*!
     * Returns a pointer to the value if `value` is contained in the
     * set, or nullptr otherwise.
     * It does not allocate memory and its complexity is *effectively*
     * @f$ O(1) @f$.
     */
    IMMER_NODISCARD const T* find(const T& value) const
    {
        return impl_.template get<project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(value);
    }

    /*!
     * Returns a pointer to the value if `value` is contained in the
     * set, or nullptr otherwise.
     * It does not allocate memory and its complexity is *effectively*
     * @f$ O(1) @f$.
     *
     * This overload participates in overload resolution only if
     * `Hash::is_transparent` is valid and denotes a type.
     */
    template <typename K,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T* find(const K& value) const
    {
        return impl_.template get<project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(value);
    }

    /*!
     * Returns whether the sets are equal.
     */
    IMMER_NODISCARD bool operator==(const set& other) const
    {
        return impl_.equals(other.impl_);
    }
    IMMER_NODISCARD bool operator!=(const set& other) const
    {
        return !(*this == other);
    }

    /*!
     * Returns a set containing `value`.  If the `value` is already in
     * the set, it returns the same set.  It may allocate memory and
     * its complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD set insert(T value) const&
    {
        return impl_.add(std::move(value));
    }
    IMMER_NODISCARD decltype(auto) insert(T value) &&
    {
        return insert_move(move_t{}, std::move(value));
    }

    /*!
     * Returns a set without `value`.  If the `value` is not in the
     * set it returns the same set.  It may allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD set erase(const T& value) const&
    {
        return impl_.sub(value);
    }
    IMMER_NODISCARD decltype(auto) erase(const T& value) &&
    {
        return erase_move(move_t{}, value);
    }

    /*!
     * Returns an @a transient form of this container, a
     * `immer::set_transient`.
     */
    IMMER_NODISCARD transient_type transient() const&
    {
        return transient_type{impl_};
    }
    IMMER_NODISCARD transient_type transient() &&
    {
        return transient_type{std::move(impl_)};
    }

    /*!
     * Returns a value that can be used as identity for the container.  If two
     * values have the same identity, they are guaranteed to be equal and to
     * contain the same objects.  However, two equal containers are not
     * guaranteed to have the same identity.
     */
    void* identity() const { return impl_.root; }

    // Semi-private
    const impl_t& impl() const { return impl_; }

private:
    friend transient_type;

    set&& insert_move(std::true_type, value_type value)
    {
        impl_.add_mut({}, std::move(value));
        return std::move(*this);
    }
    set insert_move(std::false_type, value_type value)
    {
        return impl_.add(std::move(value));
    }

    set&& erase_move(std::true_type, const value_type& value)
    {
        impl_.sub_mut({}, value);
        return std::move(*this);
    }
    set erase_move(std::false_type, const value_type& value)
    {
        return impl_.sub(value);
    }

    set(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();
};

} // namespace immer
