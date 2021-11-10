// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_METADATA_BASE_HPP
#define BOOST_HISTOGRAM_AXIS_METADATA_BASE_HPP

#include <boost/histogram/axis/traits.hpp>
#include <boost/histogram/detail/replace_type.hpp>
#include <string>
#include <type_traits>

namespace boost {
namespace histogram {
namespace axis {

/** Meta data holder with space optimization for empty meta data types.

  Allows write-access to metadata even if const.

  @tparam Metadata Wrapped meta data type.
 */
template <class Metadata, bool Detail>
class metadata_base {
protected:
  using metadata_type = Metadata;

  // std::string explicitly guarantees nothrow only in C++17
  static_assert(std::is_same<metadata_type, std::string>::value ||
                    std::is_nothrow_move_constructible<metadata_type>::value,
                "metadata must be nothrow move constructible");

  metadata_base() = default;
  metadata_base(const metadata_base&) = default;
  metadata_base& operator=(const metadata_base&) = default;

  // make noexcept because std::string is nothrow move constructible only in C++17
  metadata_base(metadata_base&& o) noexcept : data_(std::move(o.data_)) {}
  metadata_base(metadata_type&& o) noexcept : data_(std::move(o)) {}
  // make noexcept because std::string is nothrow move constructible only in C++17
  metadata_base& operator=(metadata_base&& o) noexcept {
    data_ = std::move(o.data_);
    return *this;
  }

private:
  mutable metadata_type data_;

public:
  /// Returns reference to metadata.
  metadata_type& metadata() noexcept { return data_; }

  /// Returns reference to mutable metadata from const axis.
  metadata_type& metadata() const noexcept { return data_; }
};

#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED

// specialization for empty metadata
template <class Metadata>
class metadata_base<Metadata, true> {
protected:
  using metadata_type = Metadata;

  metadata_base() = default;

  metadata_base(metadata_type&&) {}
  metadata_base& operator=(metadata_type&&) { return *this; }

public:
  metadata_type& metadata() noexcept {
    return static_cast<const metadata_base&>(*this).metadata();
  }

  metadata_type& metadata() const noexcept {
    static metadata_type data;
    return data;
  }
};

template <class Metadata, class Detail = detail::replace_default<Metadata, std::string>>
using metadata_base_t =
    metadata_base<Detail, (std::is_empty<Detail>::value && std::is_final<Detail>::value)>;

#endif

} // namespace axis
} // namespace histogram
} // namespace boost

#endif
