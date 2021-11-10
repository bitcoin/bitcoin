// Copyright 2015-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_DETECT_HPP
#define BOOST_HISTOGRAM_DETAIL_DETECT_HPP

#include <boost/histogram/fwd.hpp>
#include <boost/mp11/function.hpp> // mp_and, mp_or
#include <boost/mp11/integral.hpp> // mp_not
#include <boost/mp11/list.hpp>     // mp_first
#include <iterator>
#include <tuple>
#include <type_traits>

// forward declaration
namespace boost {
namespace variant2 {
template <class...>
class variant;
} // namespace variant2
} // namespace boost

namespace boost {
namespace histogram {
namespace detail {

template <class...>
using void_t = void;

struct detect_base {
  template <class T>
  static T&& val();
  template <class T>
  static T& ref();
  template <class T>
  static T const& cref();
};

#define BOOST_HISTOGRAM_DETAIL_DETECT(name, cond)       \
  template <class U>                                    \
  struct name##_impl : detect_base {                    \
    template <class T>                                  \
    static mp11::mp_true test(T& t, decltype(cond, 0)); \
    template <class T>                                  \
    static mp11::mp_false test(T&, float);              \
    using type = decltype(test<U>(ref<U>(), 0));        \
  };                                                    \
  template <class T>                                    \
  using name = typename name##_impl<T>::type

#define BOOST_HISTOGRAM_DETAIL_DETECT_BINARY(name, cond)      \
  template <class V, class W>                                 \
  struct name##_impl : detect_base {                          \
    template <class T, class U>                               \
    static mp11::mp_true test(T& t, U& u, decltype(cond, 0)); \
    template <class T, class U>                               \
    static mp11::mp_false test(T&, U&, float);                \
    using type = decltype(test<V, W>(ref<V>(), ref<W>(), 0)); \
  };                                                          \
  template <class T, class U = T>                             \
  using name = typename name##_impl<T, U>::type

// reset has overloads, trying to get pmf in this case always fails
BOOST_HISTOGRAM_DETAIL_DETECT(has_method_reset, t.reset(0));

BOOST_HISTOGRAM_DETAIL_DETECT(is_indexable, t[0]);

BOOST_HISTOGRAM_DETAIL_DETECT_BINARY(is_transform, (t.inverse(t.forward(u))));

BOOST_HISTOGRAM_DETAIL_DETECT(is_indexable_container,
                              (t[0], t.size(), std::begin(t), std::end(t)));

BOOST_HISTOGRAM_DETAIL_DETECT(is_vector_like,
                              (t[0], t.size(), t.resize(0), std::begin(t), std::end(t)));

BOOST_HISTOGRAM_DETAIL_DETECT(is_array_like, (t[0], t.size(), std::tuple_size<T>::value,
                                              std::begin(t), std::end(t)));

BOOST_HISTOGRAM_DETAIL_DETECT(is_map_like, ((typename T::key_type*)nullptr,
                                            (typename T::mapped_type*)nullptr,
                                            std::begin(t), std::end(t)));

// ok: is_axis is false for axis::variant, because T::index is templated
BOOST_HISTOGRAM_DETAIL_DETECT(is_axis, (t.size(), &T::index));

BOOST_HISTOGRAM_DETAIL_DETECT(is_iterable, (std::begin(t), std::end(t)));

BOOST_HISTOGRAM_DETAIL_DETECT(is_iterator,
                              (typename std::iterator_traits<T>::iterator_category{}));

BOOST_HISTOGRAM_DETAIL_DETECT(is_streamable, (std::declval<std::ostream&>() << t));

BOOST_HISTOGRAM_DETAIL_DETECT(has_operator_preincrement, ++t);

BOOST_HISTOGRAM_DETAIL_DETECT_BINARY(has_operator_equal, (cref<T>() == u));

BOOST_HISTOGRAM_DETAIL_DETECT_BINARY(has_operator_radd, (t += u));

BOOST_HISTOGRAM_DETAIL_DETECT_BINARY(has_operator_rsub, (t -= u));

BOOST_HISTOGRAM_DETAIL_DETECT_BINARY(has_operator_rmul, (t *= u));

BOOST_HISTOGRAM_DETAIL_DETECT_BINARY(has_operator_rdiv, (t /= u));

BOOST_HISTOGRAM_DETAIL_DETECT_BINARY(has_method_eq, (cref<T>().operator==(u)));

BOOST_HISTOGRAM_DETAIL_DETECT(has_threading_support, (T::has_threading_support));

// stronger form of std::is_convertible that works with explicit operator T and ctors
BOOST_HISTOGRAM_DETAIL_DETECT_BINARY(is_explicitly_convertible, static_cast<U>(t));

BOOST_HISTOGRAM_DETAIL_DETECT(is_complete, sizeof(T));

template <class T>
using is_storage = mp11::mp_and<is_indexable_container<T>, has_method_reset<T>,
                                has_threading_support<T>>;

template <class T>
using is_adaptible =
    mp11::mp_and<mp11::mp_not<is_storage<T>>,
                 mp11::mp_or<is_vector_like<T>, is_array_like<T>, is_map_like<T>>>;

template <class T>
struct is_tuple_impl : std::false_type {};

template <class... Ts>
struct is_tuple_impl<std::tuple<Ts...>> : std::true_type {};

template <class T>
using is_tuple = typename is_tuple_impl<T>::type;

template <class T>
struct is_variant_impl : std::false_type {};

template <class... Ts>
struct is_variant_impl<boost::variant2::variant<Ts...>> : std::true_type {};

template <class T>
using is_variant = typename is_variant_impl<T>::type;

template <class T>
struct is_axis_variant_impl : std::false_type {};

template <class... Ts>
struct is_axis_variant_impl<axis::variant<Ts...>> : std::true_type {};

template <class T>
using is_axis_variant = typename is_axis_variant_impl<T>::type;

template <class T>
using is_any_axis = mp11::mp_or<is_axis<T>, is_axis_variant<T>>;

template <class T>
using is_sequence_of_axis = mp11::mp_and<is_iterable<T>, is_axis<mp11::mp_first<T>>>;

template <class T>
using is_sequence_of_axis_variant =
    mp11::mp_and<is_iterable<T>, is_axis_variant<mp11::mp_first<T>>>;

template <class T>
using is_sequence_of_any_axis =
    mp11::mp_and<is_iterable<T>, is_any_axis<mp11::mp_first<T>>>;

// poor-mans concept checks
template <class T, class = std::enable_if_t<is_storage<std::decay_t<T>>::value>>
struct requires_storage {};

template <class T, class _ = std::decay_t<T>,
          class = std::enable_if_t<(is_storage<_>::value || is_adaptible<_>::value)>>
struct requires_storage_or_adaptible {};

template <class T, class = std::enable_if_t<is_iterator<std::decay_t<T>>::value>>
struct requires_iterator {};

template <class T, class = std::enable_if_t<
                       is_iterable<std::remove_cv_t<std::remove_reference_t<T>>>::value>>
struct requires_iterable {};

template <class T, class = std::enable_if_t<is_axis<std::decay_t<T>>::value>>
struct requires_axis {};

template <class T, class = std::enable_if_t<is_any_axis<std::decay_t<T>>::value>>
struct requires_any_axis {};

template <class T, class = std::enable_if_t<is_sequence_of_axis<std::decay_t<T>>::value>>
struct requires_sequence_of_axis {};

template <class T,
          class = std::enable_if_t<is_sequence_of_axis_variant<std::decay_t<T>>::value>>
struct requires_sequence_of_axis_variant {};

template <class T,
          class = std::enable_if_t<is_sequence_of_any_axis<std::decay_t<T>>::value>>
struct requires_sequence_of_any_axis {};

template <class T,
          class = std::enable_if_t<is_any_axis<mp11::mp_first<std::decay_t<T>>>::value>>
struct requires_axes {};

template <class T, class U,
          class = std::enable_if_t<is_transform<std::decay_t<T>, U>::value>>
struct requires_transform {};

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
