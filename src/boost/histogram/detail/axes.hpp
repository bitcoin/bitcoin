// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_AXES_HPP
#define BOOST_HISTOGRAM_DETAIL_AXES_HPP

#include <array>
#include <boost/core/nvp.hpp>
#include <boost/histogram/axis/traits.hpp>
#include <boost/histogram/axis/variant.hpp>
#include <boost/histogram/detail/make_default.hpp>
#include <boost/histogram/detail/nonmember_container_access.hpp>
#include <boost/histogram/detail/optional_index.hpp>
#include <boost/histogram/detail/priority.hpp>
#include <boost/histogram/detail/relaxed_tuple_size.hpp>
#include <boost/histogram/detail/static_if.hpp>
#include <boost/histogram/detail/sub_array.hpp>
#include <boost/histogram/detail/try_cast.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/integer_sequence.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/tuple.hpp>
#include <boost/mp11/utility.hpp>
#include <boost/throw_exception.hpp>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace boost {
namespace histogram {
namespace detail {

template <class T, class Unary>
void for_each_axis_impl(dynamic_size, T& t, Unary& p) {
  for (auto& a : t) axis::visit(p, a);
}

template <class N, class T, class Unary>
void for_each_axis_impl(N, T& t, Unary& p) {
  mp11::tuple_for_each(t, p);
}

// also matches const T and const Unary
template <class T, class Unary>
void for_each_axis(T&& t, Unary&& p) {
  for_each_axis_impl(relaxed_tuple_size(t), t, p);
}

// merge if a and b are discrete and growing
struct axis_merger {
  template <class T, class U>
  T operator()(const T& a, const U& u) {
    const T* bp = ptr_cast<T>(&u);
    if (!bp) BOOST_THROW_EXCEPTION(std::invalid_argument("axes not mergable"));
    using O = axis::traits::get_options<T>;
    constexpr bool discrete_and_growing =
        axis::traits::is_continuous<T>::value == false && O::test(axis::option::growth);
    return impl(mp11::mp_bool<discrete_and_growing>{}, a, *bp);
  }

  template <class T>
  T impl(std::false_type, const T& a, const T& b) {
    if (!relaxed_equal{}(a, b))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes not mergable"));
    return a;
  }

  template <class T>
  T impl(std::true_type, const T& a, const T& b) {
    if (relaxed_equal{}(axis::traits::metadata(a), axis::traits::metadata(b))) {
      auto r = a;
      if (axis::traits::is_ordered<T>::value) {
        r.update(b.value(0));
        r.update(b.value(b.size() - 1));
      } else
        for (auto&& v : b) r.update(v);
      return r;
    }
    return impl(std::false_type{}, a, b);
  }
};

// create empty dynamic axis which can store any axes types from the argument
template <class T>
auto make_empty_dynamic_axes(const T& axes) {
  return make_default(axes);
}

template <class... Ts>
auto make_empty_dynamic_axes(const std::tuple<Ts...>&) {
  using namespace ::boost::mp11;
  using L = mp_unique<axis::variant<Ts...>>;
  // return std::vector<axis::variant<Axis0, Axis1, ...>> or std::vector<Axis0>
  return std::vector<mp_if_c<(mp_size<L>::value == 1), mp_first<L>, L>>{};
}

template <class T, class Functor, std::size_t... Is>
auto axes_transform_impl(const T& t, Functor&& f, mp11::index_sequence<Is...>) {
  return std::make_tuple(f(Is, std::get<Is>(t))...);
}

// warning: sequential order of functor execution is platform-dependent!
template <class... Ts, class Functor>
auto axes_transform(const std::tuple<Ts...>& old_axes, Functor&& f) {
  return axes_transform_impl(old_axes, std::forward<Functor>(f),
                             mp11::make_index_sequence<sizeof...(Ts)>{});
}

// changing axes type is not supported
template <class T, class Functor>
T axes_transform(const T& old_axes, Functor&& f) {
  T axes = make_default(old_axes);
  axes.reserve(old_axes.size());
  for_each_axis(old_axes, [&](const auto& a) { axes.emplace_back(f(axes.size(), a)); });
  return axes;
}

template <class... Ts, class Binary, std::size_t... Is>
std::tuple<Ts...> axes_transform_impl(const std::tuple<Ts...>& lhs,
                                      const std::tuple<Ts...>& rhs, Binary&& bin,
                                      mp11::index_sequence<Is...>) {
  return std::make_tuple(bin(std::get<Is>(lhs), std::get<Is>(rhs))...);
}

template <class... Ts, class Binary>
std::tuple<Ts...> axes_transform(const std::tuple<Ts...>& lhs,
                                 const std::tuple<Ts...>& rhs, Binary&& bin) {
  return axes_transform_impl(lhs, rhs, bin, mp11::make_index_sequence<sizeof...(Ts)>{});
}

template <class T, class Binary>
T axes_transform(const T& lhs, const T& rhs, Binary&& bin) {
  T ax = make_default(lhs);
  ax.reserve(lhs.size());
  using std::begin;
  auto ir = begin(rhs);
  for (auto&& li : lhs) {
    axis::visit(
        [&](const auto& li) {
          axis::visit([&](const auto& ri) { ax.emplace_back(bin(li, ri)); }, *ir);
        },
        li);
    ++ir;
  }
  return ax;
}

template <class T>
unsigned axes_rank(const T& axes) {
  using std::begin;
  using std::end;
  return static_cast<unsigned>(std::distance(begin(axes), end(axes)));
}

template <class... Ts>
constexpr unsigned axes_rank(const std::tuple<Ts...>&) {
  return static_cast<unsigned>(sizeof...(Ts));
}

template <class T>
void throw_if_axes_is_too_large(const T& axes) {
  if (axes_rank(axes) > BOOST_HISTOGRAM_DETAIL_AXES_LIMIT)
    BOOST_THROW_EXCEPTION(
        std::invalid_argument("length of axis vector exceeds internal buffers, "
                              "recompile with "
                              "-DBOOST_HISTOGRAM_DETAIL_AXES_LIMIT=<new max size> "
                              "to increase internal buffers"));
}

// tuple is never too large because internal buffers adapt to size of tuple
template <class... Ts>
void throw_if_axes_is_too_large(const std::tuple<Ts...>&) {}

template <unsigned N, class... Ts>
decltype(auto) axis_get(std::tuple<Ts...>& axes) {
  return std::get<N>(axes);
}

template <unsigned N, class... Ts>
decltype(auto) axis_get(const std::tuple<Ts...>& axes) {
  return std::get<N>(axes);
}

template <unsigned N, class T>
decltype(auto) axis_get(T& axes) {
  return axes[N];
}

template <unsigned N, class T>
decltype(auto) axis_get(const T& axes) {
  return axes[N];
}

template <class... Ts>
auto axis_get(std::tuple<Ts...>& axes, const unsigned i) {
  constexpr auto S = sizeof...(Ts);
  using V = mp11::mp_unique<axis::variant<Ts*...>>;
  return mp11::mp_with_index<S>(i, [&axes](auto i) { return V(&std::get<i>(axes)); });
}

template <class... Ts>
auto axis_get(const std::tuple<Ts...>& axes, const unsigned i) {
  constexpr auto S = sizeof...(Ts);
  using V = mp11::mp_unique<axis::variant<const Ts*...>>;
  return mp11::mp_with_index<S>(i, [&axes](auto i) { return V(&std::get<i>(axes)); });
}

template <class T>
decltype(auto) axis_get(T& axes, const unsigned i) {
  return axes[i];
}

template <class T>
decltype(auto) axis_get(const T& axes, const unsigned i) {
  return axes[i];
}

template <class T, class U, std::size_t... Is>
bool axes_equal_impl(const T& t, const U& u, mp11::index_sequence<Is...>) noexcept {
  bool result = true;
  // operator folding emulation
  (void)std::initializer_list<bool>{
      (result &= relaxed_equal{}(std::get<Is>(t), std::get<Is>(u)))...};
  return result;
}

template <class... Ts, class... Us>
bool axes_equal_impl(const std::tuple<Ts...>& t, const std::tuple<Us...>& u) noexcept {
  return axes_equal_impl(
      t, u, mp11::make_index_sequence<std::min(sizeof...(Ts), sizeof...(Us))>{});
}

template <class... Ts, class U>
bool axes_equal_impl(const std::tuple<Ts...>& t, const U& u) noexcept {
  using std::begin;
  auto iu = begin(u);
  bool result = true;
  mp11::tuple_for_each(t, [&](const auto& ti) {
    axis::visit([&](const auto& ui) { result &= relaxed_equal{}(ti, ui); }, *iu);
    ++iu;
  });
  return result;
}

template <class T, class... Us>
bool axes_equal_impl(const T& t, const std::tuple<Us...>& u) noexcept {
  return axes_equal_impl(u, t);
}

template <class T, class U>
bool axes_equal_impl(const T& t, const U& u) noexcept {
  using std::begin;
  auto iu = begin(u);
  bool result = true;
  for (auto&& ti : t) {
    axis::visit(
        [&](const auto& ti) {
          axis::visit([&](const auto& ui) { result &= relaxed_equal{}(ti, ui); }, *iu);
        },
        ti);
    ++iu;
  }
  return result;
}

template <class T, class U>
bool axes_equal(const T& t, const U& u) noexcept {
  return axes_rank(t) == axes_rank(u) && axes_equal_impl(t, u);
}

// enable_if_t needed by msvc :(
template <class... Ts, class... Us>
std::enable_if_t<!(std::is_same<std::tuple<Ts...>, std::tuple<Us...>>::value)>
axes_assign(std::tuple<Ts...>&, const std::tuple<Us...>&) {
  BOOST_THROW_EXCEPTION(std::invalid_argument("cannot assign axes, types do not match"));
}

template <class... Ts>
void axes_assign(std::tuple<Ts...>& t, const std::tuple<Ts...>& u) {
  t = u;
}

template <class... Ts, class U>
void axes_assign(std::tuple<Ts...>& t, const U& u) {
  if (sizeof...(Ts) == detail::size(u)) {
    using std::begin;
    auto iu = begin(u);
    mp11::tuple_for_each(t, [&](auto& ti) {
      using T = std::decay_t<decltype(ti)>;
      ti = axis::get<T>(*iu);
      ++iu;
    });
    return;
  }
  BOOST_THROW_EXCEPTION(std::invalid_argument("cannot assign axes, sizes do not match"));
}

template <class T, class... Us>
void axes_assign(T& t, const std::tuple<Us...>& u) {
  // resize instead of reserve, because t may not be empty and we want exact capacity
  t.resize(sizeof...(Us));
  using std::begin;
  auto it = begin(t);
  mp11::tuple_for_each(u, [&](const auto& ui) { *it++ = ui; });
}

template <class T, class U>
void axes_assign(T& t, const U& u) {
  t.assign(u.begin(), u.end());
}

template <class Archive, class T>
void axes_serialize(Archive& ar, T& axes) {
  ar& make_nvp("axes", axes);
}

template <class Archive, class... Ts>
void axes_serialize(Archive& ar, std::tuple<Ts...>& axes) {
  // needed to keep serialization format backward compatible
  struct proxy {
    std::tuple<Ts...>& t;
    void serialize(Archive& ar, unsigned /* version */) {
      mp11::tuple_for_each(t, [&ar](auto& x) { ar& make_nvp("item", x); });
    }
  };
  proxy p{axes};
  ar& make_nvp("axes", p);
}

// total number of bins including *flow bins
template <class T>
std::size_t bincount(const T& axes) {
  std::size_t n = 1;
  for_each_axis(axes, [&n](const auto& a) {
    const auto old = n;
    const auto s = axis::traits::extent(a);
    n *= s;
    if (s > 0 && n < old) BOOST_THROW_EXCEPTION(std::overflow_error("bincount overflow"));
  });
  return n;
}

// initial offset for the linear index
template <class T>
std::size_t offset(const T& axes) {
  std::size_t n = 0;
  auto stride = static_cast<std::size_t>(1);
  for_each_axis(axes, [&](const auto& a) {
    if (axis::traits::options(a) & axis::option::growth)
      n = invalid_index;
    else if (n != invalid_index && axis::traits::options(a) & axis::option::underflow)
      n += stride;
    stride *= axis::traits::extent(a);
  });
  return n;
}

// make default-constructed buffer (no initialization for POD types)
template <class T, class A>
auto make_stack_buffer(const A& a) {
  return sub_array<T, buffer_size<A>::value>(axes_rank(a));
}

// make buffer with elements initialized to v
template <class T, class A>
auto make_stack_buffer(const A& a, const T& t) {
  return sub_array<T, buffer_size<A>::value>(axes_rank(a), t);
}

template <class T>
using has_underflow =
    decltype(axis::traits::get_options<T>::test(axis::option::underflow));

template <class T>
using is_growing = decltype(axis::traits::get_options<T>::test(axis::option::growth));

template <class T>
using is_not_inclusive = mp11::mp_not<axis::traits::is_inclusive<T>>;

// for vector<T>
template <class T>
struct axis_types_impl {
  using type = mp11::mp_list<std::decay_t<T>>;
};

// for vector<variant<Ts...>>
template <class... Ts>
struct axis_types_impl<axis::variant<Ts...>> {
  using type = mp11::mp_list<std::decay_t<Ts>...>;
};

// for tuple<Ts...>
template <class... Ts>
struct axis_types_impl<std::tuple<Ts...>> {
  using type = mp11::mp_list<std::decay_t<Ts>...>;
};

template <class T>
using axis_types =
    typename axis_types_impl<mp11::mp_if<is_vector_like<T>, mp11::mp_first<T>, T>>::type;

template <template <class> class Trait, class Axes>
using has_special_axis = mp11::mp_any_of<axis_types<Axes>, Trait>;

template <class Axes>
using has_growing_axis = mp11::mp_any_of<axis_types<Axes>, is_growing>;

template <class Axes>
using has_non_inclusive_axis = mp11::mp_any_of<axis_types<Axes>, is_not_inclusive>;

template <class T>
constexpr std::size_t type_score() {
  return sizeof(T) *
         (std::is_integral<T>::value ? 1 : std::is_floating_point<T>::value ? 10 : 100);
}

// arbitrary ordering of types
template <class T, class U>
using type_less = mp11::mp_bool<(type_score<T>() < type_score<U>())>;

template <class Axes>
using value_types = mp11::mp_sort<
    mp11::mp_unique<mp11::mp_transform<axis::traits::value_type, axis_types<Axes>>>,
    type_less>;

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
