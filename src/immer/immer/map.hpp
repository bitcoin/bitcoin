//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/detail/hamts/champ.hpp>
#include <immer/detail/hamts/champ_iterator.hpp>
#include <immer/memory_policy.hpp>

#include <cassert>
#include <functional>
#include <stdexcept>

namespace immer {

template <typename K,
          typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          detail::hamts::bits_t B>
class map_transient;

/*!
 * Immutable unordered mapping of values from type `K` to type `T`.
 *
 * @tparam K    The type of the keys.
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
 * search, update performance and structural sharing.  It does so by
 * storing the data in contiguous chunks of :math:`2^{B}` elements.
 * When storing big objects, the size of these contiguous chunks can
 * become too big, damaging performance.  If this is measured to be
 * problematic for a specific use-case, it can be solved by using a
 * `immer::box` to wrap the type `T`.
 *
 * **Example**
 *   .. literalinclude:: ../example/map/intro.cpp
 *      :language: c++
 *      :start-after: intro/start
 *      :end-before:  intro/end
 *
 * @endrst
 *
 */
template <typename K,
          typename T,
          typename Hash           = std::hash<K>,
          typename Equal          = std::equal_to<K>,
          typename MemoryPolicy   = default_memory_policy,
          detail::hamts::bits_t B = default_bits>
class map
{
    using value_t = std::pair<K, T>;

    using move_t =
        std::integral_constant<bool, MemoryPolicy::use_transient_rvalues>;

    struct project_value
    {
        const T& operator()(const value_t& v) const noexcept
        {
            return v.second;
        }
        T&& operator()(value_t&& v) const noexcept
        {
            return std::move(v.second);
        }
    };

    struct project_value_ptr
    {
        const T* operator()(const value_t& v) const noexcept
        {
            return &v.second;
        }
    };

    struct combine_value
    {
        template <typename Kf, typename Tf>
        value_t operator()(Kf&& k, Tf&& v) const
        {
            return {std::forward<Kf>(k), std::forward<Tf>(v)};
        }
    };

    struct default_value
    {
        const T& operator()() const
        {
            static T v{};
            return v;
        }
    };

    struct error_value
    {
        const T& operator()() const
        {
            IMMER_THROW(std::out_of_range{"key not found"});
        }
    };

    struct hash_key
    {
        auto operator()(const value_t& v) { return Hash{}(v.first); }

        template <typename Key>
        auto operator()(const Key& v)
        {
            return Hash{}(v);
        }
    };

    struct equal_key
    {
        auto operator()(const value_t& a, const value_t& b)
        {
            return Equal{}(a.first, b.first);
        }

        template <typename Key>
        auto operator()(const value_t& a, const Key& b)
        {
            return Equal{}(a.first, b);
        }
    };

    struct equal_value
    {
        auto operator()(const value_t& a, const value_t& b)
        {
            return Equal{}(a.first, b.first) && a.second == b.second;
        }
    };

    using impl_t =
        detail::hamts::champ<value_t, hash_key, equal_key, MemoryPolicy, B>;

public:
    using key_type        = K;
    using mapped_type     = T;
    using value_type      = std::pair<K, T>;
    using size_type       = detail::hamts::size_t;
    using diference_type  = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = Equal;
    using reference       = const value_type&;
    using const_reference = const value_type&;

    using iterator = detail::hamts::
        champ_iterator<value_t, hash_key, equal_key, MemoryPolicy, B>;
    using const_iterator = iterator;

    using transient_type = map_transient<K, T, Hash, Equal, MemoryPolicy, B>;

    using memory_policy_type = MemoryPolicy;

    /*!
     * Constructs a map containing the elements in `values`.
     */
    map(std::initializer_list<value_type> values)
        : impl_{impl_t::from_initializer_list(values)}
    {}

    /*!
     * Constructs a map containing the elements in the range
     * defined by the input iterator `first` and range sentinel `last`.
     */
    template <typename Iter,
              typename Sent,
              std::enable_if_t<detail::compatible_sentinel_v<Iter, Sent>,
                               bool> = true>
    map(Iter first, Sent last)
        : impl_{impl_t::from_range(first, last)}
    {}

    /*!
     * Default constructor.  It creates a map of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    map() = default;

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
        return impl_.template get<project_value, default_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, it returns a
     * default constructed value.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD const T& operator[](const K& k) const
    {
        return impl_.template get<project_value, default_value>(k);
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
        return impl_.template get<project_value, error_value>(k);
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
        return impl_.template get<project_value, error_value>(k);
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
        return impl_.template get<project_value_ptr,
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
        return impl_.template get<project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
    }

    /*!
     * Returns whether the maps are equal.
     */
    IMMER_NODISCARD bool operator==(const map& other) const
    {
        return impl_.template equals<equal_value>(other.impl_);
    }
    IMMER_NODISCARD bool operator!=(const map& other) const
    {
        return !(*this == other);
    }

    /*!
     * Returns a map containing the association `value`.  If the key is
     * already in the map, it replaces its association in the map.
     * It may allocate memory and its complexity is *effectively* @f$
     * O(1) @f$.
     */
    IMMER_NODISCARD map insert(value_type value) const&
    {
        return impl_.add(std::move(value));
    }
    IMMER_NODISCARD decltype(auto) insert(value_type value) &&
    {
        return insert_move(move_t{}, std::move(value));
    }

    /*!
     * Returns a map containing the association `(k, v)`.  If the key
     * is already in the map, it replaces its association in the map.
     * It may allocate memory and its complexity is *effectively* @f$
     * O(1) @f$.
     */
    IMMER_NODISCARD map set(key_type k, mapped_type v) const&
    {
        return impl_.add({std::move(k), std::move(v)});
    }
    IMMER_NODISCARD decltype(auto) set(key_type k, mapped_type v) &&
    {
        return set_move(move_t{}, std::move(k), std::move(v));
    }

    /*!
     * Returns a map replacing the association `(k, v)` by the
     * association new association `(k, fn(v))`, where `v` is the
     * currently associated value for `k` in the map or a default
     * constructed value otherwise. It may allocate memory
     * and its complexity is *effectively* @f$ O(1) @f$.
     */
    template <typename Fn>
    IMMER_NODISCARD map update(key_type k, Fn&& fn) const&
    {
        return impl_
            .template update<project_value, default_value, combine_value>(
                std::move(k), std::forward<Fn>(fn));
    }
    template <typename Fn>
    IMMER_NODISCARD decltype(auto) update(key_type k, Fn&& fn) &&
    {
        return update_move(move_t{}, std::move(k), std::forward<Fn>(fn));
    }

    /*!
     * Returns a map replacing the association `(k, v)` by the association new
     * association `(k, fn(v))`, where `v` is the currently associated value for
     * `k` in the map.  It does nothing if `k` is not present in the map. It
     * may allocate memory and its complexity is *effectively* @f$ O(1) @f$.
     */
    template <typename Fn>
    IMMER_NODISCARD map update_if_exists(key_type k, Fn&& fn) const&
    {
        return impl_.template update_if_exists<project_value, combine_value>(
            std::move(k), std::forward<Fn>(fn));
    }
    template <typename Fn>
    IMMER_NODISCARD decltype(auto) update_if_exists(key_type k, Fn&& fn) &&
    {
        return update_if_exists_move(
            move_t{}, std::move(k), std::forward<Fn>(fn));
    }

    /*!
     * Returns a map without the key `k`.  If the key is not
     * associated in the map it returns the same map.  It may allocate
     * memory and its complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD map erase(const K& k) const& { return impl_.sub(k); }
    IMMER_NODISCARD decltype(auto) erase(const K& k) &&
    {
        return erase_move(move_t{}, k);
    }

    /*!
     * Returns a @a transient form of this container, an
     * `immer::map_transient`.
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

    map&& insert_move(std::true_type, value_type value)
    {
        impl_.add_mut({}, std::move(value));
        return std::move(*this);
    }
    map insert_move(std::false_type, value_type value)
    {
        return impl_.add(std::move(value));
    }

    map&& set_move(std::true_type, key_type k, mapped_type m)
    {
        impl_.add_mut({}, {std::move(k), std::move(m)});
        return std::move(*this);
    }
    map set_move(std::false_type, key_type k, mapped_type m)
    {
        return impl_.add({std::move(k), std::move(m)});
    }

    template <typename Fn>
    map&& update_move(std::true_type, key_type k, Fn&& fn)
    {
        impl_.template update_mut<project_value, default_value, combine_value>(
            {}, std::move(k), std::forward<Fn>(fn));
        return std::move(*this);
    }
    template <typename Fn>
    map update_move(std::false_type, key_type k, Fn&& fn)
    {
        return impl_
            .template update<project_value, default_value, combine_value>(
                std::move(k), std::forward<Fn>(fn));
    }

    template <typename Fn>
    map&& update_if_exists_move(std::true_type, key_type k, Fn&& fn)
    {
        impl_.template update_if_exists_mut<project_value, combine_value>(
            {}, std::move(k), std::forward<Fn>(fn));
        return std::move(*this);
    }
    template <typename Fn>
    map update_if_exists_move(std::false_type, key_type k, Fn&& fn)
    {
        return impl_.template update_if_exists<project_value, combine_value>(
            std::move(k), std::forward<Fn>(fn));
    }

    map&& erase_move(std::true_type, const key_type& value)
    {
        impl_.sub_mut({}, value);
        return std::move(*this);
    }
    map erase_move(std::false_type, const key_type& value)
    {
        return impl_.sub(value);
    }

    map(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();
};

} // namespace immer
