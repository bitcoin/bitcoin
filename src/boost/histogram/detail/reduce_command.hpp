// Copyright 2020 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_REDUCE_COMMAND_HPP
#define BOOST_HISTOGRAM_DETAIL_REDUCE_COMMAND_HPP

#include <boost/histogram/detail/span.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/throw_exception.hpp>
#include <cassert>
#include <stdexcept>
#include <string>

namespace boost {
namespace histogram {
namespace detail {

struct reduce_command {
  static constexpr unsigned unset = static_cast<unsigned>(-1);
  unsigned iaxis = unset;
  enum class range_t : char {
    none,
    indices,
    values,
  } range = range_t::none;
  union {
    axis::index_type index;
    double value;
  } begin{0}, end{0};
  unsigned merge = 0; // default value indicates unset option
  bool crop = false;
  // for internal use by the reduce algorithm
  bool is_ordered = true;
  bool use_underflow_bin = true;
  bool use_overflow_bin = true;
};

// - puts commands in correct axis order
// - sets iaxis for positional commands
// - detects and fails on invalid settings
// - fuses merge commands with non-merge commands
inline void normalize_reduce_commands(span<reduce_command> out,
                                      span<const reduce_command> in) {
  unsigned iaxis = 0;
  for (const auto& o_in : in) {
    assert(o_in.merge > 0);
    if (o_in.iaxis != reduce_command::unset && o_in.iaxis >= out.size())
      BOOST_THROW_EXCEPTION(std::invalid_argument("invalid axis index"));
    auto& o_out = out.begin()[o_in.iaxis == reduce_command::unset ? iaxis : o_in.iaxis];
    if (o_out.merge == 0) {
      o_out = o_in;
    } else {
      // Some command was already set for this axis, try to fuse commands.
      if (!((o_in.range == reduce_command::range_t::none) ^
            (o_out.range == reduce_command::range_t::none)) ||
          (o_out.merge > 1 && o_in.merge > 1))
        BOOST_THROW_EXCEPTION(std::invalid_argument(
            "multiple conflicting reduce commands for axis " +
            std::to_string(o_in.iaxis == reduce_command::unset ? iaxis : o_in.iaxis)));
      if (o_in.range != reduce_command::range_t::none) {
        o_out.range = o_in.range;
        o_out.begin = o_in.begin;
        o_out.end = o_in.end;
      } else {
        o_out.merge = o_in.merge;
      }
    }
    ++iaxis;
  }

  iaxis = 0;
  for (auto& o : out) o.iaxis = iaxis++;
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
