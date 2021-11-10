// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_HISTOGRAM_HPP
#define BOOST_HISTOGRAM_HISTOGRAM_HPP

#include <boost/histogram/detail/accumulator_traits.hpp>
#include <boost/histogram/detail/argument_traits.hpp>
#include <boost/histogram/detail/axes.hpp>
#include <boost/histogram/detail/common_type.hpp>
#include <boost/histogram/detail/fill.hpp>
#include <boost/histogram/detail/fill_n.hpp>
#include <boost/histogram/detail/index_translator.hpp>
#include <boost/histogram/detail/mutex_base.hpp>
#include <boost/histogram/detail/nonmember_container_access.hpp>
#include <boost/histogram/detail/span.hpp>
#include <boost/histogram/detail/static_if.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/histogram/indexed.hpp>
#include <boost/histogram/multi_index.hpp>
#include <boost/histogram/sample.hpp>
#include <boost/histogram/storage_adaptor.hpp>
#include <boost/histogram/unsafe_access.hpp>
#include <boost/histogram/weight.hpp>
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/tuple.hpp>
#include <boost/throw_exception.hpp>
#include <mutex>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace boost {
namespace histogram {

/** Central class of the histogram library.

  Histogram uses the call operator to insert data, like the
  [Boost.Accumulators](https://www.boost.org/doc/libs/develop/doc/html/accumulators.html).

  Use factory functions (see
  [make_histogram.hpp](histogram/reference.html#header.boost.histogram.make_histogram_hpp)
  and
  [make_profile.hpp](histogram/reference.html#header.boost.histogram.make_profile_hpp)) to
  conveniently create histograms rather than calling the ctors directly.

  Use the [indexed](boost/histogram/indexed.html) range generator to iterate over filled
  histograms, which is convenient and faster than hand-written loops for multi-dimensional
  histograms.

  @tparam Axes std::tuple of axis types OR std::vector of an axis type or axis::variant
  @tparam Storage class that implements the storage interface
 */
template <class Axes, class Storage>
class histogram : detail::mutex_base<Axes, Storage> {
  static_assert(std::is_same<std::decay_t<Storage>, Storage>::value,
                "Storage may not be a reference or const or volatile");
  static_assert(mp11::mp_size<Axes>::value > 0, "at least one axis required");

public:
  using axes_type = Axes;
  using storage_type = Storage;
  using value_type = typename storage_type::value_type;
  // typedefs for boost::range_iterator
  using iterator = typename storage_type::iterator;
  using const_iterator = typename storage_type::const_iterator;
  using multi_index_type = multi_index<detail::relaxed_tuple_size_t<axes_type>::value>;

private:
  using mutex_base = typename detail::mutex_base<axes_type, storage_type>;

public:
  histogram() = default;

  template <class A, class S>
  explicit histogram(histogram<A, S>&& rhs)
      : storage_(std::move(unsafe_access::storage(rhs)))
      , offset_(unsafe_access::offset(rhs)) {
    detail::axes_assign(axes_, std::move(unsafe_access::axes(rhs)));
    detail::throw_if_axes_is_too_large(axes_);
  }

  template <class A, class S>
  explicit histogram(const histogram<A, S>& rhs)
      : storage_(unsafe_access::storage(rhs)), offset_(unsafe_access::offset(rhs)) {
    detail::axes_assign(axes_, unsafe_access::axes(rhs));
    detail::throw_if_axes_is_too_large(axes_);
  }

  template <class A, class S>
  histogram& operator=(histogram<A, S>&& rhs) {
    detail::axes_assign(axes_, std::move(unsafe_access::axes(rhs)));
    detail::throw_if_axes_is_too_large(axes_);
    storage_ = std::move(unsafe_access::storage(rhs));
    offset_ = unsafe_access::offset(rhs);
    return *this;
  }

  template <class A, class S>
  histogram& operator=(const histogram<A, S>& rhs) {
    detail::axes_assign(axes_, unsafe_access::axes(rhs));
    detail::throw_if_axes_is_too_large(axes_);
    storage_ = unsafe_access::storage(rhs);
    offset_ = unsafe_access::offset(rhs);
    return *this;
  }

  template <class A, class = detail::requires_axes<A>>
  histogram(A&& a, Storage s)
      : axes_(std::forward<A>(a))
      , storage_(std::move(s))
      , offset_(detail::offset(axes_)) {
    detail::throw_if_axes_is_too_large(axes_);
    storage_.reset(detail::bincount(axes_));
  }

  explicit histogram(Axes axes) : histogram(axes, storage_type()) {}

  template <class... As, class = detail::requires_axes<std::tuple<std::decay_t<As>...>>>
  explicit histogram(As&&... as)
      : histogram(std::tuple<std::decay_t<As>...>(std::forward<As>(as)...),
                  storage_type()) {}

  /// Number of axes (dimensions).
  constexpr unsigned rank() const noexcept { return detail::axes_rank(axes_); }

  /// Total number of bins (including underflow/overflow).
  std::size_t size() const noexcept { return storage_.size(); }

  /// Reset all bins to default initialized values.
  void reset() { storage_.reset(size()); }

  /// Get N-th axis using a compile-time number.
  /// This version is more efficient than the one accepting a run-time number.
  template <unsigned N = 0>
  decltype(auto) axis(std::integral_constant<unsigned, N> = {}) const {
    assert(N < rank());
    return detail::axis_get<N>(axes_);
  }

  /// Get N-th axis with run-time number.
  /// Prefer the version that accepts a compile-time number, if you can use it.
  decltype(auto) axis(unsigned i) const {
    assert(i < rank());
    return detail::axis_get(axes_, i);
  }

  /// Apply unary functor/function to each axis.
  template <class Unary>
  auto for_each_axis(Unary&& unary) const {
    return detail::for_each_axis(axes_, std::forward<Unary>(unary));
  }

  /** Fill histogram with values, an optional weight, and/or a sample.

    Returns iterator to located cell.

    Arguments are passed in order to the axis objects. Passing an argument type that is
    not convertible to the value type accepted by the axis or passing the wrong number
    of arguments causes a throw of `std::invalid_argument`.

    __Optional weight__

    An optional weight can be passed as the first or last argument
    with the [weight](boost/histogram/weight.html) helper function. Compilation fails if
    the storage elements do not support weights.

    __Samples__

    If the storage elements accept samples, pass them with the sample helper function
    in addition to the axis arguments, which can be the first or last argument. The
    [sample](boost/histogram/sample.html) helper function can pass one or more arguments
    to the storage element. If samples and weights are used together, they can be passed
    in any order at the beginning or end of the argument list.

    __Axis with multiple arguments__

    If the histogram contains an axis which accepts a `std::tuple` of arguments, the
    arguments for that axis need to be passed as a `std::tuple`, for example,
    `std::make_tuple(1.2, 2.3)`. If the histogram contains only this axis and no other,
    the arguments can be passed directly.
  */
  template <class T0, class... Ts,
            class = std::enable_if_t<(detail::is_tuple<T0>::value == false ||
                                      sizeof...(Ts) > 0)>>
  iterator operator()(const T0& arg0, const Ts&... args) {
    return operator()(std::forward_as_tuple(arg0, args...));
  }

  /// Fill histogram with values, an optional weight, and/or a sample from a `std::tuple`.
  template <class... Ts>
  iterator operator()(const std::tuple<Ts...>& args) {
    using arg_traits = detail::argument_traits<std::decay_t<Ts>...>;
    using acc_traits = detail::accumulator_traits<value_type>;
    constexpr bool weight_valid =
        arg_traits::wpos::value == -1 || acc_traits::weight_support;
    static_assert(weight_valid, "error: accumulator does not support weights");
    detail::sample_args_passed_vs_expected<typename arg_traits::sargs,
                                           typename acc_traits::args>();
    constexpr bool sample_valid =
        std::is_convertible<typename arg_traits::sargs, typename acc_traits::args>::value;
    std::lock_guard<typename mutex_base::type> guard{mutex_base::get()};
    return detail::fill(mp11::mp_bool<(weight_valid && sample_valid)>{}, arg_traits{},
                        offset_, storage_, axes_, args);
  }

  /** Fill histogram with several values at once.

    The argument must be an iterable with a size that matches the
    rank of the histogram. The element of an iterable may be 1) a value or 2) an iterable
    with contiguous storage over values or 3) a variant of 1) and 2). Sub-iterables must
    have the same length.

    Values are passed to the corresponding histogram axis in order. If a single value is
    passed together with an iterable of values, the single value is treated like an
    iterable with matching length of copies of this value.

    If the histogram has only one axis, an iterable of values may be passed directly.

    @param args iterable as explained in the long description.
  */
  template <class Iterable, class = detail::requires_iterable<Iterable>>
  void fill(const Iterable& args) {
    using acc_traits = detail::accumulator_traits<value_type>;
    constexpr unsigned n_sample_args_expected =
        std::tuple_size<typename acc_traits::args>::value;
    static_assert(n_sample_args_expected == 0,
                  "sample argument is missing but required by accumulator");
    std::lock_guard<typename mutex_base::type> guard{mutex_base::get()};
    detail::fill_n(mp11::mp_bool<(n_sample_args_expected == 0)>{}, offset_, storage_,
                   axes_, detail::make_span(args));
  }

  /** Fill histogram with several values and weights at once.

    @param args iterable of values.
    @param weights single weight or an iterable of weights.
  */
  template <class Iterable, class T, class = detail::requires_iterable<Iterable>>
  void fill(const Iterable& args, const weight_type<T>& weights) {
    using acc_traits = detail::accumulator_traits<value_type>;
    constexpr bool weight_valid = acc_traits::weight_support;
    static_assert(weight_valid, "error: accumulator does not support weights");
    detail::sample_args_passed_vs_expected<std::tuple<>, typename acc_traits::args>();
    constexpr bool sample_valid =
        std::is_convertible<std::tuple<>, typename acc_traits::args>::value;
    std::lock_guard<typename mutex_base::type> guard{mutex_base::get()};
    detail::fill_n(mp11::mp_bool<(weight_valid && sample_valid)>{}, offset_, storage_,
                   axes_, detail::make_span(args),
                   weight(detail::to_ptr_size(weights.value)));
  }

  /** Fill histogram with several values and weights at once.

    @param weights single weight or an iterable of weights.
    @param args iterable of values.
  */
  template <class Iterable, class T, class = detail::requires_iterable<Iterable>>
  void fill(const weight_type<T>& weights, const Iterable& args) {
    fill(args, weights);
  }

  /** Fill histogram with several values and samples at once.

    @param args iterable of values.
    @param samples single sample or an iterable of samples.
  */
  template <class Iterable, class... Ts, class = detail::requires_iterable<Iterable>>
  void fill(const Iterable& args, const sample_type<std::tuple<Ts...>>& samples) {
    using acc_traits = detail::accumulator_traits<value_type>;
    using sample_args_passed =
        std::tuple<decltype(*detail::to_ptr_size(std::declval<Ts>()).first)...>;
    detail::sample_args_passed_vs_expected<sample_args_passed,
                                           typename acc_traits::args>();
    std::lock_guard<typename mutex_base::type> guard{mutex_base::get()};
    mp11::tuple_apply( // LCOV_EXCL_LINE: gcc-11 is missing this line for no reason
        [&](const auto&... sargs) {
          constexpr bool sample_valid =
              std::is_convertible<sample_args_passed, typename acc_traits::args>::value;
          detail::fill_n(mp11::mp_bool<(sample_valid)>{}, offset_, storage_, axes_,
                         detail::make_span(args), detail::to_ptr_size(sargs)...);
        },
        samples.value);
  }

  /** Fill histogram with several values and samples at once.

    @param samples single sample or an iterable of samples.
    @param args iterable of values.
  */
  template <class Iterable, class T, class = detail::requires_iterable<Iterable>>
  void fill(const sample_type<T>& samples, const Iterable& args) {
    fill(args, samples);
  }

  template <class Iterable, class T, class... Ts,
            class = detail::requires_iterable<Iterable>>
  void fill(const Iterable& args, const weight_type<T>& weights,
            const sample_type<std::tuple<Ts...>>& samples) {
    using acc_traits = detail::accumulator_traits<value_type>;
    using sample_args_passed =
        std::tuple<decltype(*detail::to_ptr_size(std::declval<Ts>()).first)...>;
    detail::sample_args_passed_vs_expected<sample_args_passed,
                                           typename acc_traits::args>();
    std::lock_guard<typename mutex_base::type> guard{mutex_base::get()};
    mp11::tuple_apply( // LCOV_EXCL_LINE: gcc-11 is missing this line for no reason
        [&](const auto&... sargs) {
          constexpr bool weight_valid = acc_traits::weight_support;
          static_assert(weight_valid, "error: accumulator does not support weights");
          constexpr bool sample_valid =
              std::is_convertible<sample_args_passed, typename acc_traits::args>::value;
          detail::fill_n(mp11::mp_bool<(weight_valid && sample_valid)>{}, offset_,
                         storage_, axes_, detail::make_span(args),
                         weight(detail::to_ptr_size(weights.value)),
                         detail::to_ptr_size(sargs)...);
        },
        samples.value);
  }

  template <class Iterable, class T, class U, class = detail::requires_iterable<Iterable>>
  void fill(const sample_type<T>& samples, const weight_type<U>& weights,
            const Iterable& args) {
    fill(args, weights, samples);
  }

  template <class Iterable, class T, class U, class = detail::requires_iterable<Iterable>>
  void fill(const weight_type<T>& weights, const sample_type<U>& samples,
            const Iterable& args) {
    fill(args, weights, samples);
  }

  template <class Iterable, class T, class U, class = detail::requires_iterable<Iterable>>
  void fill(const Iterable& args, const sample_type<T>& samples,
            const weight_type<U>& weights) {
    fill(args, weights, samples);
  }

  /** Access cell value at integral indices.

    You can pass indices as individual arguments, as a std::tuple of integers, or as an
    interable range of integers. Passing the wrong number of arguments causes a throw of
    std::invalid_argument. Passing an index which is out of bounds causes a throw of
    std::out_of_range.

    @param i index of first axis.
    @param is indices of second, third, ... axes.
    @returns reference to cell value.
  */
  template <class... Is>
  decltype(auto) at(axis::index_type i, Is... is) {
    return at(multi_index_type{i, static_cast<axis::index_type>(is)...});
  }

  /// Access cell value at integral indices (read-only).
  template <class... Is>
  decltype(auto) at(axis::index_type i, Is... is) const {
    return at(multi_index_type{i, static_cast<axis::index_type>(is)...});
  }

  /// Access cell value at integral indices stored in iterable.
  decltype(auto) at(const multi_index_type& is) {
    if (rank() != is.size())
      BOOST_THROW_EXCEPTION(
          std::invalid_argument("number of arguments != histogram rank"));
    const auto idx = detail::linearize_indices(axes_, is);
    if (!is_valid(idx))
      BOOST_THROW_EXCEPTION(std::out_of_range("at least one index out of bounds"));
    assert(idx < storage_.size());
    return storage_[idx];
  }

  /// Access cell value at integral indices stored in iterable (read-only).
  decltype(auto) at(const multi_index_type& is) const {
    if (rank() != is.size())
      BOOST_THROW_EXCEPTION(
          std::invalid_argument("number of arguments != histogram rank"));
    const auto idx = detail::linearize_indices(axes_, is);
    if (!is_valid(idx))
      BOOST_THROW_EXCEPTION(std::out_of_range("at least one index out of bounds"));
    assert(idx < storage_.size());
    return storage_[idx];
  }

  /// Access value at index (for rank = 1).
  decltype(auto) operator[](axis::index_type i) {
    const axis::index_type shift =
        axis::traits::options(axis()) & axis::option::underflow ? 1 : 0;
    return storage_[static_cast<std::size_t>(i + shift)];
  }

  /// Access value at index (for rank = 1, read-only).
  decltype(auto) operator[](axis::index_type i) const {
    const axis::index_type shift =
        axis::traits::options(axis()) & axis::option::underflow ? 1 : 0;
    return storage_[static_cast<std::size_t>(i + shift)];
  }

  /// Access value at index tuple.
  decltype(auto) operator[](const multi_index_type& is) {
    return storage_[detail::linearize_indices(axes_, is)];
  }

  /// Access value at index tuple (read-only).
  decltype(auto) operator[](const multi_index_type& is) const {
    return storage_[detail::linearize_indices(axes_, is)];
  }

  /// Equality operator, tests equality for all axes and the storage.
  template <class A, class S>
  bool operator==(const histogram<A, S>& rhs) const noexcept {
    // testing offset is redundant, but offers fast return if it fails
    return offset_ == unsafe_access::offset(rhs) &&
           detail::axes_equal(axes_, unsafe_access::axes(rhs)) &&
           storage_ == unsafe_access::storage(rhs);
  }

  /// Negation of the equality operator.
  template <class A, class S>
  bool operator!=(const histogram<A, S>& rhs) const noexcept {
    return !operator==(rhs);
  }

  /** Add values of another histogram.

    This operator is only available if the value_type supports operator+=.

    Both histograms must be compatible to be addable. The histograms are compatible, if
    the axes are either all identical. If the axes only differ in the states of their
    discrete growing axis types, then they are also compatible. The discrete growing
    axes are merged in this case.
  */
  template <class A, class S>
#ifdef BOOST_HISTOGRAM_DOXYGEN_INVOKED
  histogram&
#else
  std::enable_if_t<
      detail::has_operator_radd<value_type, typename histogram<A, S>::value_type>::value,
      histogram&>
#endif
  operator+=(const histogram<A, S>& rhs) {
    if (!detail::axes_equal(axes_, unsafe_access::axes(rhs)))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes of histograms differ"));
    auto rit = unsafe_access::storage(rhs).begin();
    for (auto&& x : storage_) x += *rit++;
    return *this;
  }

  // specialization that allows axes merging
  template <class S>
#ifdef BOOST_HISTOGRAM_DOXYGEN_INVOKED
  histogram&
#else
  std::enable_if_t<detail::has_operator_radd<
                       value_type, typename histogram<axes_type, S>::value_type>::value,
                   histogram&>
#endif
  operator+=(const histogram<axes_type, S>& rhs) {
    const auto& raxes = unsafe_access::axes(rhs);
    if (detail::axes_equal(axes_, unsafe_access::axes(rhs))) {
      auto rit = unsafe_access::storage(rhs).begin();
      for (auto&& x : storage_) x += *rit++;
      return *this;
    }

    if (rank() != detail::axes_rank(raxes))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes have different length"));
    auto h = histogram<axes_type, storage_type>(
        detail::axes_transform(axes_, raxes, detail::axis_merger{}),
        detail::make_default(storage_));
    const auto& axes = unsafe_access::axes(h);
    const auto tr1 = detail::make_index_translator(axes, axes_);
    for (auto&& x : indexed(*this, coverage::all)) h[tr1(x.indices())] += *x;
    const auto tr2 = detail::make_index_translator(axes, raxes);
    for (auto&& x : indexed(rhs, coverage::all)) h[tr2(x.indices())] += *x;
    *this = std::move(h);
    return *this;
  }

  /** Subtract values of another histogram.

    This operator is only available if the value_type supports operator-=.
  */
  template <class A, class S>
#ifdef BOOST_HISTOGRAM_DOXYGEN_INVOKED
  histogram&
#else
  std::enable_if_t<
      detail::has_operator_rsub<value_type, typename histogram<A, S>::value_type>::value,
      histogram&>
#endif
  operator-=(const histogram<A, S>& rhs) {
    if (!detail::axes_equal(axes_, unsafe_access::axes(rhs)))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes of histograms differ"));
    auto rit = unsafe_access::storage(rhs).begin();
    for (auto&& x : storage_) x -= *rit++;
    return *this;
  }

  /** Multiply by values of another histogram.

    This operator is only available if the value_type supports operator*=.
  */
  template <class A, class S>
#ifdef BOOST_HISTOGRAM_DOXYGEN_INVOKED
  histogram&
#else
  std::enable_if_t<
      detail::has_operator_rmul<value_type, typename histogram<A, S>::value_type>::value,
      histogram&>
#endif
  operator*=(const histogram<A, S>& rhs) {
    if (!detail::axes_equal(axes_, unsafe_access::axes(rhs)))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes of histograms differ"));
    auto rit = unsafe_access::storage(rhs).begin();
    for (auto&& x : storage_) x *= *rit++;
    return *this;
  }

  /** Divide by values of another histogram.

    This operator is only available if the value_type supports operator/=.
  */
  template <class A, class S>
#ifdef BOOST_HISTOGRAM_DOXYGEN_INVOKED
  histogram&
#else
  std::enable_if_t<
      detail::has_operator_rdiv<value_type, typename histogram<A, S>::value_type>::value,
      histogram&>
#endif
  operator/=(const histogram<A, S>& rhs) {
    if (!detail::axes_equal(axes_, unsafe_access::axes(rhs)))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes of histograms differ"));
    auto rit = unsafe_access::storage(rhs).begin();
    for (auto&& x : storage_) x /= *rit++;
    return *this;
  }

  /** Multiply all values with a scalar.

    This operator is only available if the value_type supports operator*=.
  */
#ifdef BOOST_HISTOGRAM_DOXYGEN_INVOKED
  histogram&
#else
  template <class V = value_type>
  std::enable_if_t<(detail::has_operator_rmul<V, double>::value), histogram&>
#endif
  operator*=(const double x) {
    // use special storage implementation of scaling if available,
    // else fallback to scaling item by item
    detail::static_if<detail::has_operator_rmul<storage_type, double>>(
        [x](auto& s) { s *= x; },
        [x](auto& s) {
          for (auto&& si : s) si *= x;
        },
        storage_);
    return *this;
  }

  /** Divide all values by a scalar.

    This operator is only available if operator*= is available.
  */
#ifdef BOOST_HISTOGRAM_DOXYGEN_INVOKED
  histogram&
#else
  template <class H = histogram>
  std::enable_if_t<(detail::has_operator_rmul<H, double>::value), histogram&>
#endif
  operator/=(const double x) {
    return operator*=(1.0 / x);
  }

  /// Return value iterator to the beginning of the histogram.
  iterator begin() noexcept { return storage_.begin(); }

  /// Return value iterator to the end in the histogram.
  iterator end() noexcept { return storage_.end(); }

  /// Return value iterator to the beginning of the histogram (read-only).
  const_iterator begin() const noexcept { return storage_.begin(); }

  /// Return value iterator to the end in the histogram (read-only).
  const_iterator end() const noexcept { return storage_.end(); }

  /// Return value iterator to the beginning of the histogram (read-only).
  const_iterator cbegin() const noexcept { return begin(); }

  /// Return value iterator to the end in the histogram (read-only).
  const_iterator cend() const noexcept { return end(); }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    detail::axes_serialize(ar, axes_);
    ar& make_nvp("storage", storage_);
    if (Archive::is_loading::value) {
      offset_ = detail::offset(axes_);
      detail::throw_if_axes_is_too_large(axes_);
    }
  }

private:
  axes_type axes_;
  storage_type storage_;
  std::size_t offset_ = 0;

  friend struct unsafe_access;
};

/**
  Pairwise add cells of two histograms and return histogram with the sum.

  The returned histogram type is the most efficient and safest one constructible from the
  inputs, if they are not the same type. If one histogram has a tuple axis, the result has
  a tuple axis. The chosen storage is the one with the larger dynamic range.
*/
template <class A1, class S1, class A2, class S2>
auto operator+(const histogram<A1, S1>& a, const histogram<A2, S2>& b) {
  auto r = histogram<detail::common_axes<A1, A2>, detail::common_storage<S1, S2>>(a);
  return r += b;
}

/** Pairwise multiply cells of two histograms and return histogram with the product.

  For notes on the returned histogram type, see operator+.
*/
template <class A1, class S1, class A2, class S2>
auto operator*(const histogram<A1, S1>& a, const histogram<A2, S2>& b) {
  auto r = histogram<detail::common_axes<A1, A2>, detail::common_storage<S1, S2>>(a);
  return r *= b;
}

/** Pairwise subtract cells of two histograms and return histogram with the difference.

  For notes on the returned histogram type, see operator+.
*/
template <class A1, class S1, class A2, class S2>
auto operator-(const histogram<A1, S1>& a, const histogram<A2, S2>& b) {
  auto r = histogram<detail::common_axes<A1, A2>, detail::common_storage<S1, S2>>(a);
  return r -= b;
}

/** Pairwise divide cells of two histograms and return histogram with the quotient.

  For notes on the returned histogram type, see operator+.
*/
template <class A1, class S1, class A2, class S2>
auto operator/(const histogram<A1, S1>& a, const histogram<A2, S2>& b) {
  auto r = histogram<detail::common_axes<A1, A2>, detail::common_storage<S1, S2>>(a);
  return r /= b;
}

/** Multiply all cells of the histogram by a number and return a new histogram.

  If the original histogram has integer cells, the result has double cells.
*/
template <class A, class S>
auto operator*(const histogram<A, S>& h, double x) {
  auto r = histogram<A, detail::common_storage<S, dense_storage<double>>>(h);
  return r *= x;
}

/** Multiply all cells of the histogram by a number and return a new histogram.

  If the original histogram has integer cells, the result has double cells.
*/
template <class A, class S>
auto operator*(double x, const histogram<A, S>& h) {
  return h * x;
}

/** Divide all cells of the histogram by a number and return a new histogram.

  If the original histogram has integer cells, the result has double cells.
*/
template <class A, class S>
auto operator/(const histogram<A, S>& h, double x) {
  return h * (1.0 / x);
}

#if __cpp_deduction_guides >= 201606

template <class... Axes, class = detail::requires_axes<std::tuple<std::decay_t<Axes>...>>>
histogram(Axes...) -> histogram<std::tuple<std::decay_t<Axes>...>>;

template <class... Axes, class S, class = detail::requires_storage_or_adaptible<S>>
histogram(std::tuple<Axes...>, S)
    -> histogram<std::tuple<Axes...>, std::conditional_t<detail::is_adaptible<S>::value,
                                                         storage_adaptor<S>, S>>;

template <class Iterable, class = detail::requires_iterable<Iterable>,
          class = detail::requires_any_axis<typename Iterable::value_type>>
histogram(Iterable) -> histogram<std::vector<typename Iterable::value_type>>;

template <class Iterable, class S, class = detail::requires_iterable<Iterable>,
          class = detail::requires_any_axis<typename Iterable::value_type>,
          class = detail::requires_storage_or_adaptible<S>>
histogram(Iterable, S) -> histogram<
    std::vector<typename Iterable::value_type>,
    std::conditional_t<detail::is_adaptible<S>::value, storage_adaptor<S>, S>>;

#endif

} // namespace histogram
} // namespace boost

#endif
