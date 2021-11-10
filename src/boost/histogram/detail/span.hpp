// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_SPAN_HPP
#define BOOST_HISTOGRAM_DETAIL_SPAN_HPP

#ifdef __has_include
#if __has_include(<version>)
#include <version>
#ifdef __cpp_lib_span
#if __cpp_lib_span >= 201902
#define BOOST_HISTOGRAM_DETAIL_HAS_STD_SPAN
#endif
#endif
#endif
#endif

#ifdef BOOST_HISTOGRAM_DETAIL_HAS_STD_SPAN

#include <span>

namespace boost {
namespace histogram {
namespace detail {
using std::span;
} // namespace detail
} // namespace histogram
} // namespace boost

#else // C++17 span not available, so we use our implementation

// to be replaced by boost::span

#include <array>
#include <boost/histogram/detail/nonmember_container_access.hpp>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

namespace dtl = ::boost::histogram::detail;

static constexpr std::size_t dynamic_extent = ~static_cast<std::size_t>(0);

template <class T, std::size_t N>
class span_base {
public:
  constexpr T* data() noexcept { return begin_; }
  constexpr const T* data() const noexcept { return begin_; }
  constexpr std::size_t size() const noexcept { return N; }

protected:
  constexpr span_base(T* b, std::size_t s) noexcept : begin_(b) {
    (void)s;
    assert(N == s);
  }
  constexpr void set(T* b, std::size_t s) noexcept {
    (void)s;
    begin_ = b;
    assert(N == s);
  }

private:
  T* begin_;
};

template <class T>
class span_base<T, dynamic_extent> {
public:
  constexpr span_base() noexcept : begin_(nullptr), size_(0) {}

  constexpr T* data() noexcept { return begin_; }
  constexpr const T* data() const noexcept { return begin_; }
  constexpr std::size_t size() const noexcept { return size_; }

protected:
  constexpr span_base(T* b, std::size_t s) noexcept : begin_(b), size_(s) {}
  constexpr void set(T* b, std::size_t s) noexcept {
    begin_ = b;
    size_ = s;
  }

private:
  T* begin_;
  std::size_t size_;
};

template <class T, std::size_t Extent = dynamic_extent>
class span : public span_base<T, Extent> {
  using base = span_base<T, Extent>;

public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using index_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static constexpr std::size_t extent = Extent;

  using base::base;

  constexpr span(pointer first, pointer last)
      : span(first, static_cast<std::size_t>(last - first)) {
    assert(extent == dynamic_extent ||
           static_cast<difference_type>(extent) == (last - first));
  }

  constexpr span(pointer ptr, index_type count) : base(ptr, count) {}

  template <std::size_t N>
  constexpr span(element_type (&arr)[N]) noexcept : span(dtl::data(arr), N) {
    static_assert(extent == dynamic_extent || extent == N, "static sizes do not match");
  }

  template <std::size_t N,
            class = std::enable_if_t<(extent == dynamic_extent || extent == N)> >
  constexpr span(std::array<value_type, N>& arr) noexcept : span(dtl::data(arr), N) {}

  template <std::size_t N,
            class = std::enable_if_t<(extent == dynamic_extent || extent == N)> >
  constexpr span(const std::array<value_type, N>& arr) noexcept
      : span(dtl::data(arr), N) {}

  template <class Container, class = std::enable_if_t<std::is_convertible<
                                 decltype(dtl::size(std::declval<const Container&>()),
                                          dtl::data(std::declval<const Container&>())),
                                 pointer>::value> >
  constexpr span(const Container& cont) : span(dtl::data(cont), dtl::size(cont)) {}

  template <class Container, class = std::enable_if_t<std::is_convertible<
                                 decltype(dtl::size(std::declval<Container&>()),
                                          dtl::data(std::declval<Container&>())),
                                 pointer>::value> >
  constexpr span(Container& cont) : span(dtl::data(cont), dtl::size(cont)) {}

  template <class U, std::size_t N,
            class = std::enable_if_t<((extent == dynamic_extent || extent == N) &&
                                      std::is_convertible<U, element_type>::value)> >
  constexpr span(const span<U, N>& s) noexcept : span(s.data(), s.size()) {}

  template <class U, std::size_t N,
            class = std::enable_if_t<((extent == dynamic_extent || extent == N) &&
                                      std::is_convertible<U, element_type>::value)> >
  constexpr span(span<U, N>& s) noexcept : span(s.data(), s.size()) {}

  constexpr span(const span& other) noexcept = default;

  constexpr iterator begin() { return base::data(); }
  constexpr const_iterator begin() const { return base::data(); }
  constexpr const_iterator cbegin() const { return base::data(); }

  constexpr iterator end() { return base::data() + base::size(); }
  constexpr const_iterator end() const { return base::data() + base::size(); }
  constexpr const_iterator cend() const { return base::data() + base::size(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return reverse_iterator(end()); }
  const_reverse_iterator crbegin() { return reverse_iterator(end()); }

  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return reverse_iterator(begin()); }
  const_reverse_iterator crend() { return reverse_iterator(begin()); }

  constexpr reference front() { return *base::data(); }
  constexpr reference back() { return *(base::data() + base::size() - 1); }

  constexpr reference operator[](index_type idx) const { return base::data()[idx]; }

  constexpr std::size_t size_bytes() const noexcept {
    return base::size() * sizeof(element_type);
  }

  constexpr bool empty() const noexcept { return base::size() == 0; }

  template <std::size_t Count>
  constexpr span<element_type, Count> first() const {
    assert(Count <= base::size());
    return span<element_type, Count>(base::data(), Count);
  }

  constexpr span<element_type, dynamic_extent> first(std::size_t count) const {
    assert(count <= base::size());
    return span<element_type, dynamic_extent>(base::data(), count);
  }

  template <std::size_t Count>
  constexpr span<element_type, Count> last() const {
    assert(Count <= base::size());
    return span<element_type, Count>(base::data() + base::size() - Count, Count);
  }

  constexpr span<element_type, dynamic_extent> last(std::size_t count) const {
    assert(count <= base::size());
    return span<element_type, dynamic_extent>(base::data() + base::size() - count, count);
  }

  template <std::size_t Offset, std::size_t Count = dynamic_extent>
  constexpr span<element_type,
                 (Count != dynamic_extent
                      ? Count
                      : (extent != dynamic_extent ? extent - Offset : dynamic_extent))>
  subspan() const {
    assert(Offset <= base::size());
    constexpr std::size_t E =
        (Count != dynamic_extent
             ? Count
             : (extent != dynamic_extent ? extent - Offset : dynamic_extent));
    assert(E == dynamic_extent || E <= base::size());
    return span<element_type, E>(base::data() + Offset,
                                 Count == dynamic_extent ? base::size() - Offset : Count);
  }

  constexpr span<element_type, dynamic_extent> subspan(
      std::size_t offset, std::size_t count = dynamic_extent) const {
    assert(offset <= base::size());
    const std::size_t s = count == dynamic_extent ? base::size() - offset : count;
    assert(s <= base::size());
    return span<element_type, dynamic_extent>(base::data() + offset, s);
  }
};

} // namespace detail
} // namespace histogram
} // namespace boost

#endif

#include <boost/histogram/detail/nonmember_container_access.hpp>
#include <utility>

namespace boost {
namespace histogram {
namespace detail {

namespace dtl = ::boost::histogram::detail;

template <class T>
auto make_span(T* begin, T* end) {
  return dtl::span<T>{begin, end};
}

template <class T>
auto make_span(T* begin, std::size_t size) {
  return dtl::span<T>{begin, size};
}

template <class Container, class = decltype(dtl::size(std::declval<Container>()),
                                            dtl::data(std::declval<Container>()))>
auto make_span(const Container& cont) {
  return make_span(dtl::data(cont), dtl::size(cont));
}

template <class T, std::size_t N>
auto make_span(T (&arr)[N]) {
  return dtl::span<T, N>(arr, N);
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
