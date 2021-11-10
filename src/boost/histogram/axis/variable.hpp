// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_VARIABLE_HPP
#define BOOST_HISTOGRAM_AXIS_VARIABLE_HPP

#include <algorithm>
#include <boost/core/nvp.hpp>
#include <boost/histogram/axis/interval_view.hpp>
#include <boost/histogram/axis/iterator.hpp>
#include <boost/histogram/axis/metadata_base.hpp>
#include <boost/histogram/axis/option.hpp>
#include <boost/histogram/detail/convert_integer.hpp>
#include <boost/histogram/detail/detect.hpp>
#include <boost/histogram/detail/limits.hpp>
#include <boost/histogram/detail/relaxed_equal.hpp>
#include <boost/histogram/detail/replace_type.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/throw_exception.hpp>
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace boost {
namespace histogram {
namespace axis {

/**
  Axis for non-equidistant bins on the real line.

  Binning is a O(log(N)) operation. If speed matters and the problem domain
  allows it, prefer a regular axis, possibly with a transform.

  @tparam Value input value type, must be floating point.
  @tparam MetaData type to store meta data.
  @tparam Options see boost::histogram::axis::option (all values allowed).
  @tparam Allocator allocator to use for dynamic memory management.
 */
template <class Value, class MetaData, class Options, class Allocator>
class variable : public iterator_mixin<variable<Value, MetaData, Options, Allocator>>,
                 public metadata_base_t<MetaData> {
  // these must be private, so that they are not automatically inherited
  using value_type = Value;
  using metadata_base = metadata_base_t<MetaData>;
  using metadata_type = typename metadata_base::metadata_type;
  using options_type =
      detail::replace_default<Options, decltype(option::underflow | option::overflow)>;
  using allocator_type = Allocator;
  using vector_type = std::vector<Value, allocator_type>;

  static_assert(
      std::is_floating_point<value_type>::value,
      "current version of variable axis requires floating point type; "
      "if you need a variable axis with an integral type, please submit an issue");

  static_assert(
      (!options_type::test(option::circular) && !options_type::test(option::growth)) ||
          (options_type::test(option::circular) ^ options_type::test(option::growth)),
      "circular and growth options are mutually exclusive");

public:
  constexpr variable() = default;
  explicit variable(allocator_type alloc) : vec_(alloc) {}

  /** Construct from iterator range of bin edges.
   *
   * \param begin begin of edge sequence.
   * \param end   end of edge sequence.
   * \param meta  description of the axis.
   * \param alloc allocator instance to use.
   */
  template <class It, class = detail::requires_iterator<It>>
  variable(It begin, It end, metadata_type meta = {}, allocator_type alloc = {})
      : metadata_base(std::move(meta)), vec_(std::move(alloc)) {
    if (std::distance(begin, end) < 2)
      BOOST_THROW_EXCEPTION(std::invalid_argument("bins > 0 required"));

    vec_.reserve(std::distance(begin, end));
    vec_.emplace_back(*begin++);
    bool strictly_ascending = true;
    for (; begin != end; ++begin) {
      strictly_ascending &= vec_.back() < *begin;
      vec_.emplace_back(*begin);
    }
    if (!strictly_ascending)
      BOOST_THROW_EXCEPTION(
          std::invalid_argument("input sequence must be strictly ascending"));
  }

  /** Construct variable axis from iterable range of bin edges.
   *
   * \param iterable iterable range of bin edges.
   * \param meta     description of the axis.
   * \param alloc    allocator instance to use.
   */
  template <class U, class = detail::requires_iterable<U>>
  variable(const U& iterable, metadata_type meta = {}, allocator_type alloc = {})
      : variable(std::begin(iterable), std::end(iterable), std::move(meta),
                 std::move(alloc)) {}

  /** Construct variable axis from initializer list of bin edges.
   *
   * @param list  `std::initializer_list` of bin edges.
   * @param meta  description of the axis.
   * @param alloc allocator instance to use.
   */
  template <class U>
  variable(std::initializer_list<U> list, metadata_type meta = {},
           allocator_type alloc = {})
      : variable(list.begin(), list.end(), std::move(meta), std::move(alloc)) {}

  /// Constructor used by algorithm::reduce to shrink and rebin (not for users).
  variable(const variable& src, index_type begin, index_type end, unsigned merge)
      : metadata_base(src), vec_(src.get_allocator()) {
    assert((end - begin) % merge == 0);
    if (options_type::test(option::circular) && !(begin == 0 && end == src.size()))
      BOOST_THROW_EXCEPTION(std::invalid_argument("cannot shrink circular axis"));
    vec_.reserve((end - begin) / merge);
    const auto beg = src.vec_.begin();
    for (index_type i = begin; i <= end; i += merge) vec_.emplace_back(*(beg + i));
  }

  /// Return index for value argument.
  index_type index(value_type x) const noexcept {
    if (options_type::test(option::circular)) {
      const auto a = vec_[0];
      const auto b = vec_[size()];
      x -= std::floor((x - a) / (b - a)) * (b - a);
    }
    return static_cast<index_type>(std::upper_bound(vec_.begin(), vec_.end(), x) -
                                   vec_.begin() - 1);
  }

  std::pair<index_type, index_type> update(value_type x) noexcept {
    const auto i = index(x);
    if (std::isfinite(x)) {
      if (0 <= i) {
        if (i < size()) return std::make_pair(i, 0);
        const auto d = value(size()) - value(size() - 0.5);
        x = std::nextafter(x, (std::numeric_limits<value_type>::max)());
        x = (std::max)(x, vec_.back() + d);
        vec_.push_back(x);
        return {i, -1};
      }
      const auto d = value(0.5) - value(0);
      x = (std::min)(x, value(0) - d);
      vec_.insert(vec_.begin(), x);
      return {0, -i};
    }
    return {x < 0 ? -1 : size(), 0};
  }

  /// Return value for fractional index argument.
  value_type value(real_index_type i) const noexcept {
    if (options_type::test(option::circular)) {
      auto shift = std::floor(i / size());
      i -= shift * size();
      double z;
      const auto k = static_cast<index_type>(std::modf(i, &z));
      const auto a = vec_[0];
      const auto b = vec_[size()];
      return (1.0 - z) * vec_[k] + z * vec_[k + 1] + shift * (b - a);
    }
    if (i < 0) return detail::lowest<value_type>();
    if (i == size()) return vec_.back();
    if (i > size()) return detail::highest<value_type>();
    const auto k = static_cast<index_type>(i); // precond: i >= 0
    const real_index_type z = i - k;
    // check z == 0 needed to avoid returning nan when vec_[k + 1] is infinity
    return (1.0 - z) * vec_[k] + (z == 0 ? 0 : z * vec_[k + 1]);
  }

  /// Return bin for index argument.
  auto bin(index_type idx) const noexcept { return interval_view<variable>(*this, idx); }

  /// Returns the number of bins, without over- or underflow.
  index_type size() const noexcept { return static_cast<index_type>(vec_.size()) - 1; }

  /// Returns the options.
  static constexpr unsigned options() noexcept { return options_type::value; }

  template <class V, class M, class O, class A>
  bool operator==(const variable<V, M, O, A>& o) const noexcept {
    const auto& a = vec_;
    const auto& b = o.vec_;
    return std::equal(a.begin(), a.end(), b.begin(), b.end()) &&
           detail::relaxed_equal{}(this->metadata(), o.metadata());
  }

  template <class V, class M, class O, class A>
  bool operator!=(const variable<V, M, O, A>& o) const noexcept {
    return !operator==(o);
  }

  /// Return allocator instance.
  auto get_allocator() const { return vec_.get_allocator(); }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("seq", vec_);
    ar& make_nvp("meta", this->metadata());
  }

private:
  vector_type vec_;

  template <class V, class M, class O, class A>
  friend class variable;
};

#if __cpp_deduction_guides >= 201606

template <class T>
variable(std::initializer_list<T>)
    ->variable<detail::convert_integer<T, double>, null_type>;

template <class T, class M>
variable(std::initializer_list<T>, M)
    ->variable<detail::convert_integer<T, double>,
               detail::replace_type<std::decay_t<M>, const char*, std::string>>;

template <class Iterable, class = detail::requires_iterable<Iterable>>
variable(Iterable)
    ->variable<
        detail::convert_integer<
            std::decay_t<decltype(*std::begin(std::declval<Iterable&>()))>, double>,
        null_type>;

template <class Iterable, class M>
variable(Iterable, M)
    ->variable<
        detail::convert_integer<
            std::decay_t<decltype(*std::begin(std::declval<Iterable&>()))>, double>,
        detail::replace_type<std::decay_t<M>, const char*, std::string>>;

#endif

} // namespace axis
} // namespace histogram
} // namespace boost

#endif
