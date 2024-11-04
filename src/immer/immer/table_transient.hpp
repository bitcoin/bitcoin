#pragma once

#include <immer/detail/hamts/champ.hpp>
#include <immer/memory_policy.hpp>
#include <type_traits>

namespace immer {

template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          detail::hamts::bits_t B>
class table;

/*!
 * Mutable version of `immer::table`.
 *
 * @rst
 *
 * Refer to :doc:`transients` to learn more about when and how to use
 * the mutable versions of immutable containers.
 *
 * @endrst
 */
template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          detail::hamts::bits_t B>
class table_transient : MemoryPolicy::transience_t::owner
{
    using K       = std::decay_t<decltype(KeyFn{}(std::declval<T>()))>;
    using base_t  = typename MemoryPolicy::transience_t::owner;
    using owner_t = base_t;

public:
    using persistent_type = table<T, KeyFn, Hash, Equal, MemoryPolicy, B>;
    using key_type        = K;
    using mapped_type     = T;
    using value_type      = T;
    using size_type       = detail::hamts::size_t;
    using diference_type  = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = Equal;
    using reference       = const value_type&;
    using const_reference = const value_type&;

    using iterator       = typename persistent_type::iterator;
    using const_iterator = iterator;

    /*!
     * Default constructor.  It creates a table of `size() == 0`. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    table_transient() = default;

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
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::default_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If there is no entry with such a key in the table, it returns a
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
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::error_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`. If there is no entry with such a key in the table, throws an
     * `std::out_of_range` error.  It does not allocate memory and its
     * complexity is *effectively* @f$ O(1) @f$.
     */
    const T& at(const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::error_value>(k);
    }

    /*!
     * Returns a pointer to the value associated with the key `k`.
     * If there is no entry with such a key in the table,
     * a `nullptr` is returned. It does not allocate memory and
     * its complexity is *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD const T* find(const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
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
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
    }

    /*!
     * Inserts `value` to the table.
     * If there is an entry with its key is already,
     * it replaces this entry by `value`.
     * It may allocate memory and its complexity is *effectively* @f$
     * O(1) @f$.
     */
    void insert(value_type value) { impl_.add_mut(*this, std::move(value)); }

    /*!
     * Returns `this->insert(fn((*this)[k]))`. In particular, `fn` maps `T` to
     * `T`. The key `k` will be set into the value returned bu `fn`.  It may
     * allocate memory and its complexity is *effectively* @f$ O(1) @f$.
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
     * Returns `this->insert(fn((*this)[k]))` when `this->count(k) > 0`. In
     * particular, `fn` maps `T` to `T`. The key `k` will be replaced into the
     * value returned by `fn`.  It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
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
     * Removes table entry by given key `k` if there is any. It may allocate
     * memory and its complexity is *effectively* @f$ O(1) @f$.
     */
    void erase(const K& k) { impl_.sub_mut(*this, k); }

    /*!
     * Returns an @a immutable form of this container, an
     * `immer::table`.
     */
    IMMER_NODISCARD persistent_type persistent() &
    {
        this->owner_t::operator=(owner_t{});
        return impl_;
    }

    /*!
     * Returns an @a immutable form of this container, an
     * `immer::table`.
     */
    IMMER_NODISCARD persistent_type persistent() && { return std::move(impl_); }

private:
    friend persistent_type;
    using impl_t = typename persistent_type::impl_t;

    table_transient(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();

public:
    // Semi-private
    const impl_t& impl() const { return impl_; }
};

} // namespace immer
