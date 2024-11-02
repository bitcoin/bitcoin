#pragma once

#include <immer/config.hpp>
#include <immer/detail/hamts/champ.hpp>
#include <immer/detail/hamts/champ_iterator.hpp>
#include <immer/memory_policy.hpp>
#include <type_traits>

namespace immer {

template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          detail::hamts::bits_t B>
class table_transient;

/*!
 * Function template to get the key in `immer::table_key_fn`.
 * It assumes the key is `id` class member.
 */
template <typename T>
auto get_table_key(T const& x) -> decltype(x.id)
{
    return x.id;
}

/*!
 * Function template to set the key in `immer::table_key_fn`.
 * It assumes the key is `id` class member.
 */
template <typename T, typename K>
auto set_table_key(T x, K&& k) -> T
{
    x.id = std::forward<K>(k);
    return x;
}

/*!
 * Default value for `KeyFn` in `immer::table`.
 * It assumes the key is `id` class member.
 */
struct table_key_fn
{
    template <typename T>
    decltype(auto) operator()(T&& x) const
    {
        return get_table_key(std::forward<T>(x));
    }

    template <typename T, typename K>
    auto operator()(T&& x, K&& k) const
    {
        return set_table_key(std::forward<T>(x), std::forward<K>(k));
    }
};

template <typename KeyFn, typename T>
using table_key_t = std::decay_t<decltype(KeyFn{}(std::declval<T>()))>;

/*!
 * Immutable unordered set of values of type `T`. Values are indexed via
 * `operator()(const T&)` from `KeyFn` template parameter.
 * By default, key is `&T::id`.
 *
 * @tparam T The type of the values to be stored in the container.
 * @tparam KeyFn Type which implements `operator()(const T&)`
 * @tparam Hash  The type of a function object capable of hashing
 *               values of type `T`.
 * @tparam Equal The type of a function object capable of comparing
 *               values of type `T`.
 * @tparam MemoryPolicy Memory management policy. See @ref
 *              memory_policy.
 *
 * @rst
 *
 * This container is based on the `immer::map` underlying data structure.
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
 *   .. literalinclude:: ../example/table/intro.cpp
 *      :language: c++
 *      :start-after: intro/start
 *      :end-before:  intro/end
 *
 * @endrst
 *
 */
template <typename T,
          typename KeyFn          = table_key_fn,
          typename Hash           = std::hash<table_key_t<KeyFn, T>>,
          typename Equal          = std::equal_to<table_key_t<KeyFn, T>>,
          typename MemoryPolicy   = default_memory_policy,
          detail::hamts::bits_t B = default_bits>
class table
{
    using K       = table_key_t<KeyFn, T>;
    using value_t = T;

    using move_t =
        std::integral_constant<bool, MemoryPolicy::use_transient_rvalues>;

    struct project_value
    {
        const T& operator()(const value_t& v) const noexcept { return v; }
        T&& operator()(value_t&& v) const noexcept { return std::move(v); }
    };

    struct project_value_ptr
    {
        const T* operator()(const value_t& v) const noexcept
        {
            return std::addressof(v);
        }
    };

    struct combine_value
    {
        template <typename Kf, typename Tf>
        auto operator()(Kf&& k, Tf&& v) const
        {
            return KeyFn{}(std::forward<Tf>(v), std::forward<Kf>(k));
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
        std::size_t operator()(const value_t& v) const
        {
            return Hash{}(KeyFn{}(v));
        }

        template <typename Key>
        std::size_t operator()(const Key& v) const
        {
            return Hash{}(v);
        }
    };

    struct equal_key
    {
        bool operator()(const value_t& a, const value_t& b) const
        {
            auto ke = KeyFn{};
            return Equal{}(ke(a), ke(b));
        }

        template <typename Key>
        bool operator()(const value_t& a, const Key& b) const
        {
            return Equal{}(KeyFn{}(a), b);
        }
    };

    struct equal_value
    {
        bool operator()(const value_t& a, const value_t& b) const
        {
            return a == b;
        }
    };

    using impl_t =
        detail::hamts::champ<value_t, hash_key, equal_key, MemoryPolicy, B>;

public:
    using key_type        = K;
    using mapped_type     = T;
    using value_type      = T;
    using size_type       = detail::hamts::size_t;
    using diference_type  = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = Equal;
    using reference       = const value_type&;
    using const_reference = const value_type&;

    using iterator = detail::hamts::
        champ_iterator<value_t, hash_key, equal_key, MemoryPolicy, B>;
    using const_iterator = iterator;

    using transient_type =
        table_transient<T, KeyFn, Hash, Equal, MemoryPolicy, B>;

    using memory_policy_type = MemoryPolicy;

    /*!
     * Constructs a table containing the elements in `values`.
     */
    table(std::initializer_list<value_type> values)
        : impl_{impl_t::from_initializer_list(values)}
    {}

    /*!
     * Constructs a table containing the elements in the range
     * defined by the input iterator `first` and range sentinel `last`.
     */
    template <typename Iter,
              typename Sent,
              std::enable_if_t<detail::compatible_sentinel_v<Iter, Sent>,
                               bool> = true>
    table(Iter first, Sent last)
        : impl_{impl_t::from_range(first, last)}
    {}

    /*!
     * Default constructor.  It creates a table of `size() == 0`. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    table() = default;

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
     * Returns `1` when the key `k` is contained in the table or `0`
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
     * Returns `1` when the key `k` is contained in the table or `0`
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
     * `k`.  If there is no entry with such a key in the table, it returns a
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
     * `k`.  If there is no entry with such a key in the table, it returns a
     * default constructed value.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD const T& operator[](const K& k) const
    {
        return impl_.template get<project_value, default_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`. If there is no entry with such a key in the table, throws an
     * `std::out_of_range` error.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     *
     * This overload participates in overload resolution only if
     * `Hash::is_transparent` is valid and denotes a type.
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
     * `k`. If there is no entry with such a key in the table, throws an
     * `std::out_of_range` error.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    const T& at(const K& k) const
    {
        return impl_.template get<project_value, error_value>(k);
    }

    /*!
     * Returns a pointer to the value associated with the key `k`.
     * If there is no entry with such a key in the table,
     * a `nullptr` is returned. It does not allocate memory and
     * its complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD const T* find(const K& k) const
    {
        return impl_.template get<project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
    }

    /*!
     * Returns a pointer to the value associated with the key `k`.
     * If there is no entry with such a key in the table,
     * a `nullptr` is returned. It does not allocate memory and
     * its complexity is *effectively* @f$ O(1) @f$.
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

    IMMER_NODISCARD bool operator==(const table& other) const
    {
        return impl_.template equals<equal_value>(other.impl_);
    }

    IMMER_NODISCARD bool operator!=(const table& other) const
    {
        return !(*this == other);
    }

    /*!
     * Returns a table containing the `value`.
     * If there is an entry with its key is already,
     * it replaces this entry by `value`.
     * It may allocate memory and its complexity is *effectively* @f$
     * O(1) @f$.
     */
    IMMER_NODISCARD table insert(value_type value) const&
    {
        return impl_.add(std::move(value));
    }

    /*!
     * Returns a table containing the `value`.
     * If there is an entry with its key is already,
     * it replaces this entry by `value`.
     * It may allocate memory and its complexity is *effectively* @f$
     * O(1) @f$.
     */
    IMMER_NODISCARD decltype(auto) insert(value_type value) &&
    {
        return insert_move(move_t{}, std::move(value));
    }

    /*!
     * Returns `this->insert(fn((*this)[k]))`. In particular, `fn` maps
     * `T` to `T`. The key `k` will be replaced inside the value returned by
     * `fn`. It may allocate memory and its complexity is *effectively* @f$ O(1)
     * @f$.
     */
    template <typename Fn>
    IMMER_NODISCARD table update(key_type k, Fn&& fn) const&
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
     * Returns `this.count(k) ? this->insert(fn((*this)[k])) : *this`. In
     * particular, `fn` maps `T` to `T`. The key `k` will be replaced inside the
     * value returned by `fn`.  It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    template <typename Fn>
    IMMER_NODISCARD table update_if_exists(key_type k, Fn&& fn) const&
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
     * Returns a table without entries with given key `k`. If the key is not
     * present it returns `*this`. It may allocate
     * memory and its complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD table erase(const K& k) const& { return impl_.sub(k); }

    /*!
     * Returns a table without entries with given key `k`. If the key is not
     * present it returns `*this`. It may allocate
     * memory and its complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD decltype(auto) erase(const K& k) &&
    {
        return erase_move(move_t{}, k);
    }

    /*!
     * Returns a @a transient form of this container, an
     * `immer::table_transient`.
     */
    IMMER_NODISCARD transient_type transient() const&
    {
        return transient_type{impl_};
    }

    /*!
     * Returns a @a transient form of this container, an
     * `immer::table_transient`.
     */
    IMMER_NODISCARD transient_type transient() &&
    {
        return transient_type{std::move(impl_)};
    }

    // Semi-private
    const impl_t& impl() const { return impl_; }

private:
    friend transient_type;

    table&& insert_move(std::true_type, value_type value)
    {
        impl_.add_mut({}, std::move(value));
        return std::move(*this);
    }
    table insert_move(std::false_type, value_type value)
    {
        return impl_.add(std::move(value));
    }

    template <typename Fn>
    table&& update_move(std::true_type, key_type k, Fn&& fn)
    {
        impl_.template update_mut<project_value, default_value, combine_value>(
            {}, std::move(k), std::forward<Fn>(fn));
        return std::move(*this);
    }
    template <typename Fn>
    table update_move(std::false_type, key_type k, Fn&& fn)
    {
        return impl_
            .template update<project_value, default_value, combine_value>(
                std::move(k), std::forward<Fn>(fn));
    }

    template <typename Fn>
    table&& update_if_exists_move(std::true_type, key_type k, Fn&& fn)
    {
        impl_.template update_if_exists_mut<project_value, combine_value>(
            {}, std::move(k), std::forward<Fn>(fn));
        return std::move(*this);
    }
    template <typename Fn>
    table update_if_exists_move(std::false_type, key_type k, Fn&& fn)
    {
        return impl_.template update_if_exists<project_value, combine_value>(
            std::move(k), std::forward<Fn>(fn));
    }

    table&& erase_move(std::true_type, const key_type& value)
    {
        impl_.sub_mut({}, value);
        return std::move(*this);
    }
    table erase_move(std::false_type, const key_type& value)
    {
        return impl_.sub(value);
    }

    table(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();
};

} // namespace immer
