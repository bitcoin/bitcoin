//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/hamts/champ.hpp>
#include <immer/memory_policy.hpp>

#include <functional>

namespace immer {

template <typename K,
          typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          detail::hamts::bits_t B>
class map;

/*!
 * Mutable version of `immer::map`.
 *
 * @rst
 *
 * Refer to :doc:`transients` to learn more about when and how to use
 * the mutable versions of immutable containers.
 *
 * @endrst
 */
template <typename K,
          typename T,
          typename Hash           = std::hash<K>,
          typename Equal          = std::equal_to<K>,
          typename MemoryPolicy   = default_memory_policy,
          detail::hamts::bits_t B = default_bits>
class map_transient : MemoryPolicy::transience_t::owner
{
    using base_t  = typename MemoryPolicy::transience_t::owner;
    using owner_t = base_t;

public:
    using persistent_type = map<K, T, Hash, Equal, MemoryPolicy, B>;

    using key_type        = K;
    using mapped_type     = T;
    using value_type      = std::pair<K, T>;
    using size_type       = detail::hamts::size_t;
    using diference_type  = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = Equal;
    using reference       = const value_type&;
    using const_reference = const value_type&;

    using iterator       = typename persistent_type::iterator;
    using const_iterator = iterator;

    /*!
     * Default constructor.  It creates a map of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    map_transient() = default;

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
     * Returns `1` when the key `k` is contained in the map or `0`
     * otherwise. It won't allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * This overload participates in overload resolution only if
     * `Hash::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD size_type count(const Key& k) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(k);
    }

    /*!
     * Returns `1` when the key `k` is contained in the map or `0`
     * otherwise. It won't allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD size_type count(const K& k) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, it returns a
     * default constructed value.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     *
     * This overload participates in overload resolution only if
     * `Hash::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T& operator[](const Key& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::default_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, it returns a
     * default constructed value.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD const T& operator[](const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::default_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, throws an
     * `std::out_of_range` error.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    const T& at(const Key& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::error_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, throws an
     * `std::out_of_range` error.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     *
     * This overload participates in overload resolution only if
     * `Hash::is_transparent` is valid and denotes a type.
     */
    const T& at(const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::error_value>(k);
    }

    /*!
     * Returns a pointer to the value associated with the key `k`.  If
     * the key is not contained in the map, a `nullptr` is returned.
     * It does not allocate memory and its complexity is *effectively*
     * @f$ O(1) @f$.
     *
     * @rst
     *
     * .. admonition:: Why doesn't this function return an iterator?
     *
     *   Associative containers from the C++ standard library provide a
     *   ``find`` method that returns an iterator pointing to the
     *   element in the container or ``end()`` when the key is missing.
     *   In the case of an unordered container, the only meaningful
     *   thing one may do with it is to compare it with the end, to
     *   test if the find was succesfull, and dereference it.  This
     *   comparison is cumbersome compared to testing for a non-empty
     *   optional value.  Furthermore, for an immutable container,
     *   returning an iterator would have some additional performance
     *   cost, with no benefits otherwise.
     *
     *   In our opinion, this function should return a
     *   ``std::optional<const T&>`` but this construction is not valid
     *   in any current standard.  As a compromise we return a
     *   pointer, which has similar syntactic properties yet it is
     *   unfortunately unnecessarily unrestricted.
     *
     * @endrst
     */
    IMMER_NODISCARD const T* find(const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
    }

    /*!
     * Returns a pointer to the value associated with the key `k`.  If
     * the key is not contained in the map, a `nullptr` is returned.
     * It does not allocate memory and its complexity is *effectively*
     * @f$ O(1) @f$.
     *
     * This overload participates in overload resolution only if
     * `Hash::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T* find(const Key& k) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
    }

    /*!
     * Inserts the association `value`.  If the key is already in the map, it
     * replaces its association in the map.  It may allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    void insert(value_type value) { impl_.add_mut(*this, std::move(value)); }

    /*!
     * Inserts the association `(k, v)`.  If the key is already in the map, it
     * replaces its association in the map.  It may allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    void set(key_type k, mapped_type v)
    {
        impl_.add_mut(*this, {std::move(k), std::move(v)});
    }

    /*!
     * Replaces the association `(k, v)` by the association new association `(k,
     * fn(v))`, where `v` is the currently associated value for `k` in the map
     * or a default constructed value otherwise. It may allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    template <typename Fn>
    void update(key_type k, Fn&& fn)
    {
        impl_.template update_mut<typename persistent_type::project_value,
                                  typename persistent_type::default_value,
                                  typename persistent_type::combine_value>(
            *this, std::move(k), std::forward<Fn>(fn));
    }

    /*!
     * Replaces the association `(k, v)` by the association new association `(k,
     * fn(v))`, where `v` is the currently associated value for `k` in the map
     * or does nothing if `k` is not present in the map. It may allocate memory
     * and its complexity is *effectively* @f$ O(1) @f$.
     */
    template <typename Fn>
    void update_if_exists(key_type k, Fn&& fn)
    {
        impl_.template update_if_exists_mut<
            typename persistent_type::project_value,
            typename persistent_type::combine_value>(
            *this, std::move(k), std::forward<Fn>(fn));
    }

    /*!
     * Removes the key `k` from the k.  Does nothing if the key is not
     * associated in the map.  It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    void erase(const K& k) { impl_.sub_mut(*this, k); }

    /*!
     * Returns an @a immutable form of this container, an
     * `immer::map`.
     */
    IMMER_NODISCARD persistent_type persistent() &
    {
        this->owner_t::operator=(owner_t{});
        return impl_;
    }
    IMMER_NODISCARD persistent_type persistent() && { return std::move(impl_); }

private:
    friend persistent_type;
    using impl_t = typename persistent_type::impl_t;

    map_transient(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();

public:
    // Semi-private
    const impl_t& impl() const { return impl_; }
};

} // namespace immer
