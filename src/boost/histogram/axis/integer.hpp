// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_INTEGER_HPP
#define BOOST_HISTOGRAM_AXIS_INTEGER_HPP

#include <boost/core/nvp.hpp>
#include <boost/histogram/axis/iterator.hpp>
#include <boost/histogram/axis/metadata_base.hpp>
#include <boost/histogram/axis/option.hpp>
#include <boost/histogram/detail/convert_integer.hpp>
#include <boost/histogram/detail/limits.hpp>
#include <boost/histogram/detail/relaxed_equal.hpp>
#include <boost/histogram/detail/replace_type.hpp>
#include <boost/histogram/detail/static_if.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/throw_exception.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace boost {
namespace histogram {
namespace axis {

/**
  Axis for an interval of integer values with unit steps.

  Binning is a O(1) operation. This axis bins faster than a regular axis.

  @tparam Value input value type. Must be integer or floating point.
  @tparam MetaData type to store meta data.
  @tparam Options see boost::histogram::axis::option (all values allowed).
 */
template <class Value, class MetaData, class Options>
class integer : public iterator_mixin<integer<Value, MetaData, Options>>,
                public metadata_base_t<MetaData> {
  // these must be private, so that they are not automatically inherited
  using value_type = Value;
  using metadata_base = metadata_base_t<MetaData>;
  using metadata_type = typename metadata_base::metadata_type;
  using options_type =
      detail::replace_default<Options, decltype(option::underflow | option::overflow)>;

  static_assert(std::is_integral<value_type>::value ||
                    std::is_floating_point<value_type>::value,
                "integer axis requires floating point or integral type");

  static_assert(!options_type::test(option::circular | option::growth) ||
                    (options_type::test(option::circular) ^
                     options_type::test(option::growth)),
                "circular and growth options are mutually exclusive");

  static_assert(std::is_floating_point<value_type>::value ||
                    (!options_type::test(option::circular) &&
                     !options_type::test(option::growth)) ||
                    (!options_type::test(option::overflow) &&
                     !options_type::test(option::underflow)),
                "circular or growing integer axis with integral type "
                "cannot have entries in underflow or overflow bins");

  using local_index_type = std::conditional_t<std::is_integral<value_type>::value,
                                              index_type, real_index_type>;

public:
  constexpr integer() = default;

  /** Construct over semi-open integer interval [start, stop).
   *
   * \param start    first integer of covered range.
   * \param stop     one past last integer of covered range.
   * \param meta     description of the axis.
   */
  integer(value_type start, value_type stop, metadata_type meta = {})
      : metadata_base(std::move(meta))
      , size_(static_cast<index_type>(stop - start))
      , min_(start) {
    if (!(stop >= start))
      BOOST_THROW_EXCEPTION(std::invalid_argument("stop >= start required"));
  }

  /// Constructor used by algorithm::reduce to shrink and rebin.
  integer(const integer& src, index_type begin, index_type end, unsigned merge)
      : integer(src.value(begin), src.value(end), src.metadata()) {
    if (merge > 1)
      BOOST_THROW_EXCEPTION(std::invalid_argument("cannot merge bins for integer axis"));
    if (options_type::test(option::circular) && !(begin == 0 && end == src.size()))
      BOOST_THROW_EXCEPTION(std::invalid_argument("cannot shrink circular axis"));
  }

  /// Return index for value argument.
  index_type index(value_type x) const noexcept {
    return index_impl(options_type::test(axis::option::circular),
                      std::is_floating_point<value_type>{},
                      static_cast<double>(x - min_));
  }

  /// Returns index and shift (if axis has grown) for the passed argument.
  auto update(value_type x) noexcept {
    auto impl = [this](long x) -> std::pair<index_type, index_type> {
      const auto i = x - min_;
      if (i >= 0) {
        const auto k = static_cast<axis::index_type>(i);
        if (k < size()) return {k, 0};
        const auto n = k - size() + 1;
        size_ += n;
        return {k, -n};
      }
      const auto k = static_cast<axis::index_type>(
          detail::static_if<std::is_floating_point<value_type>>(
              [](auto x) { return std::floor(x); }, [](auto x) { return x; }, i));
      min_ += k;
      size_ -= k;
      return {0, -k};
    };

    return detail::static_if<std::is_floating_point<value_type>>(
        [this, impl](auto x) -> std::pair<index_type, index_type> {
          if (std::isfinite(x)) return impl(static_cast<long>(std::floor(x)));
          return {x < 0 ? -1 : this->size(), 0};
        },
        impl, x);
  }

  /// Return value for index argument.
  value_type value(local_index_type i) const noexcept {
    if (!options_type::test(option::circular) &&
        std::is_floating_point<value_type>::value) {
      if (i < 0) return detail::lowest<value_type>();
      if (i > size()) return detail::highest<value_type>();
    }
    return min_ + i;
  }

  /// Return bin for index argument.
  decltype(auto) bin(index_type idx) const noexcept {
    return detail::static_if<std::is_floating_point<value_type>>(
        [this](auto idx) { return interval_view<integer>(*this, idx); },
        [this](auto idx) { return this->value(idx); }, idx);
  }

  /// Returns the number of bins, without over- or underflow.
  index_type size() const noexcept { return size_; }

  /// Returns the options.
  static constexpr unsigned options() noexcept { return options_type::value; }

  /// Whether the axis is inclusive (see axis::traits::is_inclusive).
  static constexpr bool inclusive() noexcept {
    // If axis has underflow and overflow, it is inclusive.
    // If axis is growing or circular:
    // - it is inclusive if value_type is int.
    // - it is not inclusive if value_type is float, because of nan and inf.
    constexpr bool full_flow =
        options() & option::underflow && options() & option::overflow;
    return full_flow || (std::is_integral<value_type>::value &&
                         (options() & (option::growth | option::circular)));
  }

  template <class V, class M, class O>
  bool operator==(const integer<V, M, O>& o) const noexcept {
    return size() == o.size() && min_ == o.min_ &&
           detail::relaxed_equal{}(this->metadata(), o.metadata());
  }

  template <class V, class M, class O>
  bool operator!=(const integer<V, M, O>& o) const noexcept {
    return !operator==(o);
  }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("size", size_);
    ar& make_nvp("meta", this->metadata());
    ar& make_nvp("min", min_);
  }

private:
  // axis not circular
  template <class B>
  index_type index_impl(std::false_type, B, double z) const noexcept {
    if (z < size()) return z >= 0 ? static_cast<index_type>(z) : -1;
    return size();
  }

  // value_type is integer, axis circular
  index_type index_impl(std::true_type, std::false_type, double z) const noexcept {
    return static_cast<index_type>(z - std::floor(z / size()) * size());
  }

  // value_type is floating point, must handle +/-infinite or nan, axis circular
  index_type index_impl(std::true_type, std::true_type, double z) const noexcept {
    if (std::isfinite(z)) return index_impl(std::true_type{}, std::false_type{}, z);
    return z < size() ? -1 : size();
  }

  index_type size_{0};
  value_type min_{0};

  template <class V, class M, class O>
  friend class integer;
};

#if __cpp_deduction_guides >= 201606

template <class T>
integer(T, T) -> integer<detail::convert_integer<T, index_type>, null_type>;

template <class T, class M>
integer(T, T, M)
    -> integer<detail::convert_integer<T, index_type>,
               detail::replace_type<std::decay_t<M>, const char*, std::string>>;

#endif

} // namespace axis
} // namespace histogram
} // namespace boost

#endif
