// Copyright 2018-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_STORAGE_ADAPTOR_HPP
#define BOOST_HISTOGRAM_STORAGE_ADAPTOR_HPP

#include <algorithm>
#include <boost/core/nvp.hpp>
#include <boost/histogram/accumulators/is_thread_safe.hpp>
#include <boost/histogram/detail/array_wrapper.hpp>
#include <boost/histogram/detail/detect.hpp>
#include <boost/histogram/detail/iterator_adaptor.hpp>
#include <boost/histogram/detail/safe_comparison.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11/utility.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
struct vector_impl : T {
  using allocator_type = typename T::allocator_type;

  static constexpr bool has_threading_support =
      accumulators::is_thread_safe<typename T::value_type>::value;

  vector_impl(const allocator_type& a = {}) : T(a) {}
  vector_impl(const vector_impl&) = default;
  vector_impl& operator=(const vector_impl&) = default;
  vector_impl(vector_impl&&) = default;
  vector_impl& operator=(vector_impl&&) = default;

  explicit vector_impl(T&& t) : T(std::move(t)) {}
  explicit vector_impl(const T& t) : T(t) {}

  template <class U, class = requires_iterable<U>>
  explicit vector_impl(const U& u, const allocator_type& a = {})
      : T(std::begin(u), std::end(u), a) {}

  template <class U, class = requires_iterable<U>>
  vector_impl& operator=(const U& u) {
    T::resize(u.size());
    auto it = T::begin();
    for (auto&& x : u) *it++ = x;
    return *this;
  }

  void reset(std::size_t n) {
    using value_type = typename T::value_type;
    const auto old_size = T::size();
    T::resize(n, value_type());
    std::fill_n(T::begin(), (std::min)(n, old_size), value_type());
  }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("vector", static_cast<T&>(*this));
  }
};

template <class T>
struct array_impl : T {
  static constexpr bool has_threading_support =
      accumulators::is_thread_safe<typename T::value_type>::value;

  array_impl() = default;
  array_impl(const array_impl& t) : T(t), size_(t.size_) {}
  array_impl& operator=(const array_impl& t) {
    T::operator=(t);
    size_ = t.size_;
    return *this;
  }

  explicit array_impl(T&& t) : T(std::move(t)) {}
  explicit array_impl(const T& t) : T(t) {}

  template <class U, class = requires_iterable<U>>
  explicit array_impl(const U& u) : size_(u.size()) {
    using std::begin;
    using std::end;
    std::copy(begin(u), end(u), this->begin());
  }

  template <class U, class = requires_iterable<U>>
  array_impl& operator=(const U& u) {
    if (u.size() > T::max_size()) // for std::arra
      BOOST_THROW_EXCEPTION(std::length_error("argument size exceeds maximum capacity"));
    size_ = u.size();
    using std::begin;
    using std::end;
    std::copy(begin(u), end(u), T::begin());
    return *this;
  }

  void reset(std::size_t n) {
    using value_type = typename T::value_type;
    if (n > T::max_size()) // for std::array
      BOOST_THROW_EXCEPTION(std::length_error("argument size exceeds maximum capacity"));
    std::fill_n(T::begin(), n, value_type());
    size_ = n;
  }

  typename T::iterator end() noexcept { return T::begin() + size_; }
  typename T::const_iterator end() const noexcept { return T::begin() + size_; }

  std::size_t size() const noexcept { return size_; }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("size", size_);
    auto w = detail::make_array_wrapper(T::data(), size_);
    ar& make_nvp("array", w);
  }

  std::size_t size_ = 0;
};

template <class T>
struct map_impl : T {
  static_assert(std::is_same<typename T::key_type, std::size_t>::value,
                "requires std::size_t as key_type");

  using value_type = typename T::mapped_type;
  using const_reference = const value_type&;

  static constexpr bool has_threading_support = false;
  static_assert(
      !accumulators::is_thread_safe<value_type>::value,
      "std::map and std::unordered_map do not support thread-safe element access. "
      "If you have a map with thread-safe element access, please file an issue and"
      "support will be added.");

  struct reference {
    reference(map_impl* m, std::size_t i) noexcept : map(m), idx(i) {}

    reference(const reference&) noexcept = default;
    reference& operator=(const reference& o) {
      if (this != &o) operator=(static_cast<const_reference>(o));
      return *this;
    }

    operator const_reference() const noexcept {
      return static_cast<const map_impl*>(map)->operator[](idx);
    }

    reference& operator=(const_reference u) {
      auto it = map->find(idx);
      if (u == value_type{}) {
        if (it != static_cast<T*>(map)->end()) { map->erase(it); }
      } else {
        if (it != static_cast<T*>(map)->end()) {
          it->second = u;
        } else {
          map->emplace(idx, u);
        }
      }
      return *this;
    }

    template <class U, class V = value_type,
              class = std::enable_if_t<has_operator_radd<V, U>::value>>
    reference& operator+=(const U& u) {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end()) {
        it->second += u;
      } else {
        map->emplace(idx, u);
      }
      return *this;
    }

    template <class U, class V = value_type,
              class = std::enable_if_t<has_operator_rsub<V, U>::value>>
    reference& operator-=(const U& u) {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end()) {
        it->second -= u;
      } else {
        map->emplace(idx, -u);
      }
      return *this;
    }

    template <class U, class V = value_type,
              class = std::enable_if_t<has_operator_rmul<V, U>::value>>
    reference& operator*=(const U& u) {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end()) it->second *= u;
      return *this;
    }

    template <class U, class V = value_type,
              class = std::enable_if_t<has_operator_rdiv<V, U>::value>>
    reference& operator/=(const U& u) {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end()) {
        it->second /= u;
      } else if (!(value_type{} / u == value_type{})) {
        map->emplace(idx, value_type{} / u);
      }
      return *this;
    }

    template <class V = value_type,
              class = std::enable_if_t<has_operator_preincrement<V>::value>>
    reference operator++() {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end()) {
        ++it->second;
      } else {
        value_type tmp{};
        ++tmp;
        map->emplace(idx, tmp);
      }
      return *this;
    }

    template <class V = value_type,
              class = std::enable_if_t<has_operator_preincrement<V>::value>>
    value_type operator++(int) {
      const value_type tmp = *this;
      operator++();
      return tmp;
    }

    template <class U, class = std::enable_if_t<has_operator_equal<value_type, U>::value>>
    bool operator==(const U& rhs) const {
      return operator const_reference() == rhs;
    }

    template <class U, class = std::enable_if_t<has_operator_equal<value_type, U>::value>>
    bool operator!=(const U& rhs) const {
      return !operator==(rhs);
    }

    template <class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits>& operator<<(
        std::basic_ostream<CharT, Traits>& os, reference x) {
      os << static_cast<const_reference>(x);
      return os;
    }

    template <class... Ts>
    auto operator()(const Ts&... args) -> decltype(std::declval<value_type>()(args...)) {
      return (*map)[idx](args...);
    }

    map_impl* map;
    std::size_t idx;
  };

  template <class Value, class Reference, class MapPtr>
  struct iterator_t
      : iterator_adaptor<iterator_t<Value, Reference, MapPtr>, std::size_t, Reference> {
    iterator_t() = default;
    template <class V, class R, class M,
              class = std::enable_if_t<std::is_convertible<M, MapPtr>::value>>
    iterator_t(const iterator_t<V, R, M>& it) noexcept : iterator_t(it.map_, it.base()) {}
    iterator_t(MapPtr m, std::size_t i) noexcept
        : iterator_t::iterator_adaptor_(i), map_(m) {}
    template <class V, class R, class M>
    bool equal(const iterator_t<V, R, M>& rhs) const noexcept {
      return map_ == rhs.map_ && iterator_t::base() == rhs.base();
    }
    Reference operator*() const { return (*map_)[iterator_t::base()]; }
    MapPtr map_ = nullptr;
  };

  using iterator = iterator_t<value_type, reference, map_impl*>;
  using const_iterator = iterator_t<const value_type, const_reference, const map_impl*>;

  using allocator_type = typename T::allocator_type;

  map_impl(const allocator_type& a = {}) : T(a) {}

  map_impl(const map_impl&) = default;
  map_impl& operator=(const map_impl&) = default;
  map_impl(map_impl&&) = default;
  map_impl& operator=(map_impl&&) = default;

  map_impl(const T& t) : T(t), size_(t.size()) {}
  map_impl(T&& t) : T(std::move(t)), size_(t.size()) {}

  template <class U, class = requires_iterable<U>>
  explicit map_impl(const U& u, const allocator_type& a = {}) : T(a), size_(u.size()) {
    using std::begin;
    using std::end;
    std::copy(begin(u), end(u), this->begin());
  }

  template <class U, class = requires_iterable<U>>
  map_impl& operator=(const U& u) {
    if (u.size() < size_)
      reset(u.size());
    else
      size_ = u.size();
    using std::begin;
    using std::end;
    std::copy(begin(u), end(u), this->begin());
    return *this;
  }

  void reset(std::size_t n) {
    T::clear();
    size_ = n;
  }

  reference operator[](std::size_t i) noexcept { return {this, i}; }
  const_reference operator[](std::size_t i) const noexcept {
    auto it = T::find(i);
    static const value_type null = value_type{};
    if (it == T::end()) return null;
    return it->second;
  }

  iterator begin() noexcept { return {this, 0}; }
  iterator end() noexcept { return {this, size_}; }

  const_iterator begin() const noexcept { return {this, 0}; }
  const_iterator end() const noexcept { return {this, size_}; }

  std::size_t size() const noexcept { return size_; }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("size", size_);
    ar& make_nvp("map", static_cast<T&>(*this));
  }

  std::size_t size_ = 0;
};

template <class T>
struct ERROR_type_passed_to_storage_adaptor_not_recognized;

// clang-format off
template <class T>
using storage_adaptor_impl =
  mp11::mp_cond<
    is_vector_like<T>, vector_impl<T>,
    is_array_like<T>, array_impl<T>,
    is_map_like<T>, map_impl<T>,
    std::true_type, ERROR_type_passed_to_storage_adaptor_not_recognized<T>
  >;
// clang-format on
} // namespace detail

/// Turns any vector-like, array-like, and map-like container into a storage type.
template <class T>
class storage_adaptor : public detail::storage_adaptor_impl<T> {
  using impl_type = detail::storage_adaptor_impl<T>;

public:
  // standard copy, move, assign
  storage_adaptor(storage_adaptor&&) = default;
  storage_adaptor(const storage_adaptor&) = default;
  storage_adaptor& operator=(storage_adaptor&&) = default;
  storage_adaptor& operator=(const storage_adaptor&) = default;

  // forwarding constructor
  template <class... Ts>
  storage_adaptor(Ts&&... ts) : impl_type(std::forward<Ts>(ts)...) {}

  // forwarding assign
  template <class U>
  storage_adaptor& operator=(U&& u) {
    impl_type::operator=(std::forward<U>(u));
    return *this;
  }

  template <class U, class = detail::requires_iterable<U>>
  bool operator==(const U& u) const {
    using std::begin;
    using std::end;
    return std::equal(this->begin(), this->end(), begin(u), end(u), detail::safe_equal{});
  }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("impl", static_cast<impl_type&>(*this));
  }

private:
  friend struct unsafe_access;
};

} // namespace histogram
} // namespace boost

#endif
