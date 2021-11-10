// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_TRAITS_HPP
#define BOOST_HISTOGRAM_AXIS_TRAITS_HPP

#include <boost/histogram/axis/option.hpp>
#include <boost/histogram/detail/args_type.hpp>
#include <boost/histogram/detail/detect.hpp>
#include <boost/histogram/detail/priority.hpp>
#include <boost/histogram/detail/static_if.hpp>
#include <boost/histogram/detail/try_cast.hpp>
#include <boost/histogram/detail/type_name.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/utility.hpp>
#include <boost/throw_exception.hpp>
#include <boost/variant2/variant.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace boost {
namespace histogram {
namespace detail {

template <class Axis>
struct value_type_deducer {
  using type =
      std::remove_cv_t<std::remove_reference_t<detail::arg_type<decltype(&Axis::index)>>>;
};

template <class Axis>
auto traits_options(priority<2>) -> axis::option::bitset<Axis::options()>;

template <class Axis>
auto traits_options(priority<1>) -> decltype(&Axis::update, axis::option::growth_t{});

template <class Axis>
auto traits_options(priority<0>) -> axis::option::none_t;

template <class Axis>
auto traits_is_inclusive(priority<1>) -> std::integral_constant<bool, Axis::inclusive()>;

template <class Axis>
auto traits_is_inclusive(priority<0>)
    -> decltype(traits_options<Axis>(priority<2>{})
                    .test(axis::option::underflow | axis::option::overflow));

template <class Axis>
auto traits_is_ordered(priority<1>) -> std::integral_constant<bool, Axis::ordered()>;

template <class Axis, class ValueType = typename value_type_deducer<Axis>::type>
auto traits_is_ordered(priority<0>) -> typename std::is_arithmetic<ValueType>::type;

template <class I, class D, class A,
          class J = std::decay_t<arg_type<decltype(&A::value)>>>
decltype(auto) value_method_switch(I&& i, D&& d, const A& a, priority<1>) {
  return static_if<std::is_same<J, axis::index_type>>(std::forward<I>(i),
                                                      std::forward<D>(d), a);
}

template <class I, class D, class A>
double value_method_switch(I&&, D&&, const A&, priority<0>) {
  // comma trick to make all compilers happy; some would complain about
  // unreachable code after the throw, others about a missing return
  return BOOST_THROW_EXCEPTION(
             std::runtime_error(type_name<A>() + " has no value method")),
         double{};
}

struct variant_access {
  template <class T, class Variant>
  static auto get_if(Variant* v) noexcept {
    using T0 = mp11::mp_first<std::decay_t<Variant>>;
    return static_if<std::is_pointer<T0>>(
        [](auto* vptr) {
          using TP = mp11::mp_if<std::is_const<std::remove_pointer_t<T0>>, const T*, T*>;
          auto ptp = variant2::get_if<TP>(vptr);
          return ptp ? *ptp : nullptr;
        },
        [](auto* vptr) { return variant2::get_if<T>(vptr); }, &(v->impl));
  }

  template <class T0, class Visitor, class Variant>
  static decltype(auto) visit_impl(mp11::mp_identity<T0>, Visitor&& vis, Variant&& v) {
    return variant2::visit(std::forward<Visitor>(vis), v.impl);
  }

  template <class T0, class Visitor, class Variant>
  static decltype(auto) visit_impl(mp11::mp_identity<T0*>, Visitor&& vis, Variant&& v) {
    return variant2::visit(
        [&vis](auto&& x) -> decltype(auto) { return std::forward<Visitor>(vis)(*x); },
        v.impl);
  }

  template <class Visitor, class Variant>
  static decltype(auto) visit(Visitor&& vis, Variant&& v) {
    using T0 = mp11::mp_first<std::decay_t<Variant>>;
    return visit_impl(mp11::mp_identity<T0>{}, std::forward<Visitor>(vis),
                      std::forward<Variant>(v));
  }
};

template <class A>
decltype(auto) metadata_impl(A&& a, decltype(a.metadata(), 0)) {
  return std::forward<A>(a).metadata();
}

template <class A>
axis::null_type& metadata_impl(A&&, float) {
  static axis::null_type null_value;
  return null_value;
}

} // namespace detail

namespace axis {
namespace traits {

/** Value type for axis type.

  Doxygen does not render this well. This is a meta-function (template alias), it accepts
  an axis type and returns the value type.

  The value type is deduced from the argument of the `Axis::index` method. Const
  references are decayed to the their value types, for example, the type deduced for
  `Axis::index(const int&)` is `int`.

  The deduction always succeeds if the axis type models the Axis concept correctly. Errors
  come from violations of the concept, in particular, an index method that is templated or
  overloaded is not allowed.

  @tparam Axis axis type.
*/
template <class Axis>
#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
using value_type = typename detail::value_type_deducer<Axis>::type;
#else
struct value_type;
#endif

/** Whether axis is continuous or discrete.

  Doxygen does not render this well. This is a meta-function (template alias), it accepts
  an axis type and returns a compile-time boolean.

  If the boolean is true, the axis is continuous (covers a continuous range of values).
  Otherwise it is discrete (covers discrete values).
*/
template <class Axis>
#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
using is_continuous = typename std::is_floating_point<traits::value_type<Axis>>::type;
#else
struct is_continuous;
#endif

/** Meta-function to detect whether an axis is reducible.

  Doxygen does not render this well. This is a meta-function (template alias), it accepts
  an axis type and represents compile-time boolean which is true or false, depending on
  whether the axis can be reduced with boost::histogram::algorithm::reduce().

  An axis can be made reducible by adding a special constructor, see Axis concept for
  details.

  @tparam Axis axis type.
 */
template <class Axis>
#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
using is_reducible = std::is_constructible<Axis, const Axis&, axis::index_type,
                                           axis::index_type, unsigned>;
#else
struct is_reducible;
#endif

/** Get axis options for axis type.

  Doxygen does not render this well. This is a meta-function (template alias), it accepts
  an axis type and returns the boost::histogram::axis::option::bitset.

  If Axis::options() is valid and constexpr, get_options is the corresponding
  option type. Otherwise, it is boost::histogram::axis::option::growth_t, if the
  axis has a method `update`, else boost::histogram::axis::option::none_t.

  @tparam Axis axis type
*/
template <class Axis>
#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
using get_options = decltype(detail::traits_options<Axis>(detail::priority<2>{}));

template <class Axis>
using static_options [[deprecated("use get_options instead")]] = get_options<Axis>;

#else
struct get_options;
#endif

/** Meta-function to detect whether an axis is inclusive.

  Doxygen does not render this well. This is a meta-function (template alias), it accepts
  an axis type and represents compile-time boolean which is true or false, depending on
  whether the axis is inclusive or not.

  An axis with underflow and overflow bins is always inclusive, but an axis may be
  inclusive under other conditions. The meta-function checks for the method `constexpr
  static bool inclusive()`, and uses the result. If this method is not present, it uses
  get_options<Axis> and checks whether the underflow and overflow bits are present.

  An inclusive axis has a bin for every possible input value. A histogram which consists
  only of inclusive axes can be filled more efficiently, since input values always
  end up in a valid cell and there is no need to keep track of input tuples that need to
  be discarded.

  @tparam Axis axis type
*/
template <class Axis>
#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
using is_inclusive = decltype(detail::traits_is_inclusive<Axis>(detail::priority<1>{}));

template <class Axis>
using static_is_inclusive [[deprecated("use is_inclusive instead")]] = is_inclusive<Axis>;

#else
struct is_inclusive;
#endif

/** Meta-function to detect whether an axis is ordered.

  Doxygen does not render this well. This is a meta-function (template alias), it accepts
  an axis type and returns a compile-time boolean. If the boolean is true, the axis is
  ordered.

  The meta-function checks for the method `constexpr static bool ordered()`, and uses the
  result. If this method is not present, it returns true if the value type of the Axis is
  arithmetic and false otherwise.

  An ordered axis has a value type that is ordered, which means that indices i <
  j < k implies either value(i) < value(j) < value(k) or value(i) > value(j) > value(k)
  for all i,j,k. For example, the integer axis is ordered, but the category axis is not.
  Axis which are not ordered must not have underflow bins, because they only have an
  "other" category, which is identified with the overflow bin if it is available.

  @tparam Axis axis type
*/
template <class Axis>
#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
using is_ordered = decltype(detail::traits_is_ordered<Axis>(detail::priority<1>{}));
#else
struct is_ordered;
#endif

/** Returns axis options as unsigned integer.

  See get_options for details.

  @param axis any axis instance
*/
template <class Axis>
constexpr unsigned options(const Axis& axis) noexcept {
  (void)axis;
  return get_options<Axis>::value;
}

// specialization for variant
template <class... Ts>
unsigned options(const variant<Ts...>& axis) noexcept {
  return axis.options();
}

/** Returns true if axis is inclusive or false.

  See is_inclusive for details.

  @param axis any axis instance
*/
template <class Axis>
constexpr bool inclusive(const Axis& axis) noexcept {
  (void)axis;
  return is_inclusive<Axis>::value;
}

// specialization for variant
template <class... Ts>
bool inclusive(const variant<Ts...>& axis) noexcept {
  return axis.inclusive();
}

/** Returns true if axis is ordered or false.

  See is_ordered for details.

  @param axis any axis instance
*/
template <class Axis>
constexpr bool ordered(const Axis& axis) noexcept {
  (void)axis;
  return is_ordered<Axis>::value;
}

// specialization for variant
template <class... Ts>
bool ordered(const variant<Ts...>& axis) noexcept {
  return axis.ordered();
}

/** Returns true if axis is continuous or false.

  See is_continuous for details.

  @param axis any axis instance
*/
template <class Axis>
constexpr bool continuous(const Axis& axis) noexcept {
  (void)axis;
  return is_continuous<Axis>::value;
}

// specialization for variant
template <class... Ts>
bool continuous(const variant<Ts...>& axis) noexcept {
  return axis.continuous();
}

/** Returns axis size plus any extra bins for under- and overflow.

  @param axis any axis instance
*/
template <class Axis>
index_type extent(const Axis& axis) noexcept {
  const auto opt = options(axis);
  return axis.size() + (opt & option::underflow ? 1 : 0) +
         (opt & option::overflow ? 1 : 0);
}

/** Returns reference to metadata of an axis.

  If the expression x.metadata() for an axis instance `x` (maybe const) is valid, return
  the result. Otherwise, return a reference to a static instance of
  boost::histogram::axis::null_type.

  @param axis any axis instance
*/
template <class Axis>
decltype(auto) metadata(Axis&& axis) noexcept {
  return detail::metadata_impl(std::forward<Axis>(axis), 0);
}

/** Returns axis value for index.

  If the axis has no `value` method, throw std::runtime_error. If the method exists and
  accepts a floating point index, pass the index and return the result. If the method
  exists but accepts only integer indices, cast the floating point index to int, pass this
  index and return the result.

  @param axis any axis instance
  @param index floating point axis index
*/
template <class Axis>
decltype(auto) value(const Axis& axis, real_index_type index) {
  return detail::value_method_switch(
      [index](const auto& a) { return a.value(static_cast<index_type>(index)); },
      [index](const auto& a) { return a.value(index); }, axis, detail::priority<1>{});
}

/** Returns axis value for index if it is convertible to target type or throws.

  Like boost::histogram::axis::traits::value, but converts the result into the requested
  return type. If the conversion is not possible, throws std::runtime_error.

  @tparam Result requested return type
  @tparam Axis axis type
  @param axis any axis instance
  @param index floating point axis index
*/
template <class Result, class Axis>
Result value_as(const Axis& axis, real_index_type index) {
  return detail::try_cast<Result, std::runtime_error>(
      axis::traits::value(axis, index)); // avoid conversion warning
}

/** Returns axis index for value.

  Throws std::invalid_argument if the value argument is not implicitly convertible.

  @param axis any axis instance
  @param value argument to be passed to `index` method
*/
template <class Axis, class U>
axis::index_type index(const Axis& axis, const U& value) noexcept(
    std::is_convertible<U, value_type<Axis>>::value) {
  return axis.index(detail::try_cast<value_type<Axis>, std::invalid_argument>(value));
}

// specialization for variant
template <class... Ts, class U>
axis::index_type index(const variant<Ts...>& axis, const U& value) {
  return axis.index(value);
}

/** Return axis rank (how many arguments it processes).

  @param axis any axis instance
*/
// gcc workaround: must use unsigned int not unsigned as return type
template <class Axis>
constexpr unsigned int rank(const Axis& axis) {
  (void)axis;
  using T = value_type<Axis>;
  // cannot use mp_eval_or since T could be a fixed-sized sequence
  return mp11::mp_eval_if_not<detail::is_tuple<T>, mp11::mp_size_t<1>, mp11::mp_size,
                              T>::value;
}

// specialization for variant
// gcc workaround: must use unsigned int not unsigned as return type
template <class... Ts>
unsigned int rank(const axis::variant<Ts...>& axis) {
  return detail::variant_access::visit(
      [](const auto& a) { return axis::traits::rank(a); }, axis);
}

/** Returns pair of axis index and shift for the value argument.

  Throws `std::invalid_argument` if the value argument is not implicitly convertible to
  the argument expected by the `index` method. If the result of
  boost::histogram::axis::traits::get_options<decltype(axis)> has the growth flag set,
  call `update` method with the argument and return the result. Otherwise, call `index`
  and return the pair of the result and a zero shift.

  @param axis any axis instance
  @param value argument to be passed to `update` or `index` method
*/
template <class Axis, class U>
std::pair<index_type, index_type> update(Axis& axis, const U& value) noexcept(
    std::is_convertible<U, value_type<Axis>>::value) {
  return detail::static_if_c<get_options<Axis>::test(option::growth)>(
      [&value](auto& a) {
        return a.update(detail::try_cast<value_type<Axis>, std::invalid_argument>(value));
      },
      [&value](auto& a) -> std::pair<index_type, index_type> {
        return {axis::traits::index(a, value), 0};
      },
      axis);
}

// specialization for variant
template <class... Ts, class U>
std::pair<index_type, index_type> update(variant<Ts...>& axis, const U& value) {
  return visit([&value](auto& a) { return a.update(value); }, axis);
}

/** Returns bin width at axis index.

  If the axis has no `value` method, throw std::runtime_error. If the method exists and
  accepts a floating point index, return the result of `axis.value(index + 1) -
  axis.value(index)`. If the method exists but accepts only integer indices, return 0.

  @param axis any axis instance
  @param index bin index
 */
template <class Axis>
decltype(auto) width(const Axis& axis, index_type index) {
  return detail::value_method_switch(
      [](const auto&) { return 0; },
      [index](const auto& a) { return a.value(index + 1) - a.value(index); }, axis,
      detail::priority<1>{});
}

/** Returns bin width at axis index.

  Like boost::histogram::axis::traits::width, but converts the result into the requested
  return type. If the conversion is not possible, throw std::runtime_error.

  @param axis any axis instance
  @param index bin index
 */
template <class Result, class Axis>
Result width_as(const Axis& axis, index_type index) {
  return detail::value_method_switch(
      [](const auto&) { return Result{}; },
      [index](const auto& a) {
        return detail::try_cast<Result, std::runtime_error>(a.value(index + 1) -
                                                            a.value(index));
      },
      axis, detail::priority<1>{});
}

} // namespace traits
} // namespace axis
} // namespace histogram
} // namespace boost

#endif
