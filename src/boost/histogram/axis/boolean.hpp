// Copyright Hans Dembinski 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_BOOLEAN_HPP
#define BOOST_HISTOGRAM_AXIS_BOOLEAN_HPP

#include <boost/core/nvp.hpp>
#include <boost/histogram/axis/iterator.hpp>
#include <boost/histogram/axis/metadata_base.hpp>
#include <boost/histogram/axis/option.hpp>
#include <boost/histogram/detail/relaxed_equal.hpp>
#include <boost/histogram/detail/replace_type.hpp>
#include <boost/histogram/fwd.hpp>
#include <string>

namespace boost {
namespace histogram {
namespace axis {

/**
  Discrete axis for boolean data.

  Binning is a pass-though operation with zero cost, making this the
  fastest possible axis. The axis has no internal state apart from the
  optional metadata state. The axis has no under- and overflow bins.
  It cannot grow and cannot be reduced.

  @tparam MetaData type to store meta data.
 */
template <class MetaData>
class boolean : public iterator_mixin<boolean<MetaData>>,
                public metadata_base_t<MetaData> {
  using value_type = bool;
  using metadata_base = metadata_base_t<MetaData>;
  using metadata_type = typename metadata_base::metadata_type;

public:
  /** Construct a boolean axis.
   *
   * \param meta     description of the axis.
   */
  explicit boolean(metadata_type meta = {}) : metadata_base(std::move(meta)) {}

  /// Return index for value argument.
  index_type index(value_type x) const noexcept { return static_cast<index_type>(x); }

  /// Return value for index argument.
  value_type value(index_type i) const noexcept { return static_cast<value_type>(i); }

  /// Return bin for index argument.
  value_type bin(index_type i) const noexcept { return value(i); }

  /// Returns the number of bins, without over- or underflow.
  index_type size() const noexcept { return 2; }

  /// Whether the axis is inclusive (see axis::traits::is_inclusive).
  static constexpr bool inclusive() noexcept { return true; }

  /// Returns the options.
  static constexpr unsigned options() noexcept { return option::none_t::value; }

  template <class M>
  bool operator==(const boolean<M>& o) const noexcept {
    return detail::relaxed_equal{}(this->metadata(), o.metadata());
  }

  template <class M>
  bool operator!=(const boolean<M>& o) const noexcept {
    return !operator==(o);
  }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("meta", this->metadata());
  }

private:
  template <class M>
  friend class boolean;
};

#if __cpp_deduction_guides >= 201606

boolean()->boolean<null_type>;

template <class M>
boolean(M) -> boolean<detail::replace_type<std::decay_t<M>, const char*, std::string>>;

#endif

} // namespace axis
} // namespace histogram
} // namespace boost

#endif // BOOST_HISTOGRAM_AXIS_BOOLEAN_HPP
