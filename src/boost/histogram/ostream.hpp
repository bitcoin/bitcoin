// Copyright 2015-2019 Hans Dembinski
// Copyright 2019 Przemyslaw Bartosik
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_OSTREAM_HPP
#define BOOST_HISTOGRAM_OSTREAM_HPP

#include <boost/histogram/accumulators/ostream.hpp>
#include <boost/histogram/axis/ostream.hpp>
#include <boost/histogram/detail/counting_streambuf.hpp>
#include <boost/histogram/detail/detect.hpp>
#include <boost/histogram/detail/priority.hpp>
#include <boost/histogram/detail/term_info.hpp>
#include <boost/histogram/indexed.hpp>
#include <cmath>
#include <iomanip>
#include <ios>
#include <limits>
#include <numeric>
#include <ostream>
#include <streambuf>
#include <type_traits>

/**
  \file boost/histogram/ostream.hpp

  A simple streaming operator for the histogram type. The text representation is
  rudimentary and not guaranteed to be stable between versions of Boost.Histogram. This
  header is not included by any other header and must be explicitly included to use the
  streaming operator.

  To use your own, simply include your own implementation instead of this header.
 */

namespace boost {
namespace histogram {
namespace detail {

template <class OStream, unsigned N>
class tabular_ostream_wrapper : public std::array<int, N> {
  using base_t = std::array<int, N>;
  using char_type = typename OStream::char_type;
  using traits_type = typename OStream::traits_type;

public:
  template <class T>
  tabular_ostream_wrapper& operator<<(const T& t) {
    if (collect_) {
      if (static_cast<unsigned>(iter_ - base_t::begin()) == size_) {
        ++size_;
        assert(size_ <= N);
        assert(iter_ != end());
        *iter_ = 0;
      }
      count_ = 0;
      os_ << t;
      *iter_ = std::max(*iter_, static_cast<int>(count_));
    } else {
      assert(iter_ != end());
      os_ << std::setw(*iter_) << t;
    }
    ++iter_;
    return *this;
  }

  tabular_ostream_wrapper& operator<<(decltype(std::setprecision(0)) t) {
    os_ << t;
    return *this;
  }

  tabular_ostream_wrapper& operator<<(decltype(std::fixed) t) {
    os_ << t;
    return *this;
  }

  tabular_ostream_wrapper& row() {
    iter_ = base_t::begin();
    return *this;
  }

  explicit tabular_ostream_wrapper(OStream& os)
      : os_(os), cbuf_(count_), orig_(os_.rdbuf(&cbuf_)) {}

  auto end() { return base_t::begin() + size_; }
  auto end() const { return base_t::begin() + size_; }
  auto cend() const { return base_t::cbegin() + size_; }

  void complete() {
    assert(collect_); // only call this once
    collect_ = false;
    os_.rdbuf(orig_);
  }

private:
  typename base_t::iterator iter_ = base_t::begin();
  unsigned size_ = 0;
  std::streamsize count_ = 0;
  bool collect_ = true;
  OStream& os_;
  counting_streambuf<char_type, traits_type> cbuf_;
  std::basic_streambuf<char_type, traits_type>* orig_;
};

template <class OStream, class T>
void ostream_value_impl(OStream& os, const T& t,
                        decltype(static_cast<double>(t), priority<1>{})) {
  // a value from histogram cell
  const auto d = static_cast<double>(t);
  if (std::numeric_limits<int>::min() <= d && d <= std::numeric_limits<int>::max()) {
    const auto i = static_cast<int>(d);
    if (i == d) {
      os << i;
      return;
    }
  }
  os << std::defaultfloat << std::setprecision(4) << d;
}

template <class OStream, class T>
void ostream_value_impl(OStream& os, const T& t, priority<0>) {
  os << t;
}

template <class OStream, class T>
void ostream_value(OStream& os, const T& t) {
  ostream_value_impl(os << std::left, t, priority<1>{});
}

template <class OStream, class Axis>
auto ostream_bin(OStream& os, const Axis& ax, axis::index_type i, std::true_type,
                 priority<1>) -> decltype((void)ax.value(i)) {
  auto a = ax.value(i), b = ax.value(i + 1);
  os << std::right << std::defaultfloat << std::setprecision(4);
  // round edges to zero if deviation from zero is small
  const auto eps = 1e-8 * std::abs(b - a);
  if (std::abs(a) < 1e-14 && std::abs(a) < eps) a = 0;
  if (std::abs(b) < 1e-14 && std::abs(b) < eps) b = 0;
  os << "[" << a << ", " << b << ")";
}

template <class OStream, class Axis>
auto ostream_bin(OStream& os, const Axis& ax, axis::index_type i, std::false_type,
                 priority<1>) -> decltype((void)ax.value(i)) {
  os << std::right;
  os << ax.value(i);
}

template <class OStream, class... Ts>
void ostream_bin(OStream& os, const axis::category<Ts...>& ax, axis::index_type i,
                 std::false_type, priority<1>) {
  os << std::right;
  if (i < ax.size())
    os << ax.value(i);
  else
    os << "other";
}

template <class OStream, class Axis, class B>
void ostream_bin(OStream& os, const Axis&, axis::index_type i, B, priority<0>) {
  os << std::right;
  os << i;
}

struct line {
  const char* ch;
  const int size;
  line(const char* a, int b) : ch{a}, size{std::max(b, 0)} {}
};

template <class T>
std::basic_ostream<char, T>& operator<<(std::basic_ostream<char, T>& os, line&& l) {
  for (int i = 0; i < l.size; ++i) os << l.ch;
  return os;
}

template <class OStream, class Axis>
void ostream_head(OStream& os, const Axis& ax, int index, double val) {
  axis::visit(
      [&](const auto& ax) {
        using A = std::decay_t<decltype(ax)>;
        ostream_bin(os, ax, index, axis::traits::is_continuous<A>{}, priority<1>{});
        os << ' ';
        ostream_value(os, val);
      },
      ax);
}

template <class OStream>
void ostream_bar(OStream& os, int zero_offset, double z, int width, bool utf8) {
  int k = static_cast<int>(std::lround(z * width));
  if (utf8) {
    os << " │";
    if (z > 0) {
      const char* scale[8] = {" ", "▏", "▎", "▍", "▌", "▋", "▊", "▉"};
      int j = static_cast<int>(std::lround(8 * (z * width - k)));
      if (j < 0) {
        --k;
        j += 8;
      }
      os << line(" ", zero_offset) << line("█", k);
      os << scale[j];
      os << line(" ", width - zero_offset - k);
    } else if (z < 0) {
      os << line(" ", zero_offset + k) << line("█", -k)
         << line(" ", width - zero_offset + 1);
    } else {
      os << line(" ", width + 1);
    }
    os << "│\n";
  } else {
    os << " |";
    if (z >= 0) {
      os << line(" ", zero_offset) << line("=", k) << line(" ", width - zero_offset - k);
    } else {
      os << line(" ", zero_offset + k) << line("=", -k) << line(" ", width - zero_offset);
    }
    os << " |\n";
  }
}

// cannot display generalized histograms yet; line not reachable by coverage tests
template <class OStream, class Histogram>
void plot(OStream&, const Histogram&, int, std::false_type) {} // LCOV_EXCL_LINE

template <class OStream, class Histogram>
void plot(OStream& os, const Histogram& h, int w_total, std::true_type) {
  if (w_total == 0) {
    w_total = term_info::width();
    if (w_total == 0 || w_total > 78) w_total = 78;
  }
  bool utf8 = term_info::utf8();

  const auto& ax = h.axis();

  // value range; can be integer or float, positive or negative
  double vmin = 0;
  double vmax = 0;
  tabular_ostream_wrapper<OStream, 7> tos(os);
  // first pass to get widths
  for (auto&& v : indexed(h, coverage::all)) {
    auto w = static_cast<double>(*v);
    ostream_head(tos.row(), ax, v.index(), w);
    vmin = std::min(vmin, w);
    vmax = std::max(vmax, w);
  }
  tos.complete();
  if (vmax == 0) vmax = 1;

  // calculate width useable by bar (notice extra space at top)
  // <-- head --> |<--- bar ---> |
  // w_head + 2 + 2
  const int w_head = std::accumulate(tos.begin(), tos.end(), 0);
  const int w_bar = w_total - 4 - w_head;
  if (w_bar < 0) return;

  // draw upper line
  os << '\n' << line(" ", w_head + 1);
  if (utf8)
    os << "┌" << line("─", w_bar + 1) << "┐\n";
  else
    os << '+' << line("-", w_bar + 1) << "+\n";

  const int zero_offset = static_cast<int>(std::lround((-vmin) / (vmax - vmin) * w_bar));
  for (auto&& v : indexed(h, coverage::all)) {
    auto w = static_cast<double>(*v);
    ostream_head(tos.row(), ax, v.index(), w);
    // rest uses os, not tos
    ostream_bar(os, zero_offset, w / (vmax - vmin), w_bar, utf8);
  }

  // draw lower line
  os << line(" ", w_head + 1);
  if (utf8)
    os << "└" << line("─", w_bar + 1) << "┘\n";
  else
    os << '+' << line("-", w_bar + 1) << "+\n";
}

template <class OStream, class Histogram>
void ostream(OStream& os, const Histogram& h, const bool show_values = true) {
  os << "histogram(";

  unsigned iaxis = 0;
  const auto rank = h.rank();
  h.for_each_axis([&](const auto& ax) {
    if ((show_values && rank > 0) || rank > 1) os << "\n  ";
    ostream_any(os, ax);
  });

  if (show_values && rank > 0) {
    tabular_ostream_wrapper<OStream, (BOOST_HISTOGRAM_DETAIL_AXES_LIMIT + 1)> tos(os);
    for (auto&& v : indexed(h, coverage::all)) {
      tos.row();
      for (auto i : v.indices()) tos << std::right << i;
      ostream_value(tos, *v);
    }
    tos.complete();

    const int w_item = std::accumulate(tos.begin(), tos.end(), 0) + 4 + h.rank();
    const int nrow = std::max(1, 65 / w_item);
    int irow = 0;
    for (auto&& v : indexed(h, coverage::all)) {
      os << (irow == 0 ? "\n  (" : " (");
      tos.row();
      iaxis = 0;
      for (auto i : v.indices()) {
        tos << std::right << i;
        os << (++iaxis == h.rank() ? "):" : " ");
      }
      os << ' ';
      ostream_value(tos, *v);
      ++irow;
      if (nrow > 0 && irow == nrow) irow = 0;
    }
    os << '\n';
  }
  os << ')';
}

} // namespace detail

#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED

template <class CharT, class Traits, class A, class S>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const histogram<A, S>& h) {
  // save fmt
  const auto flags = os.flags();

  os.flags(std::ios::dec | std::ios::left);

  const auto w = static_cast<int>(os.width());
  os.width(0);

  using value_type = typename histogram<A, S>::value_type;

  using convertible = detail::is_explicitly_convertible<value_type, double>;
  // must be non-const to avoid a msvc warning about possible use of if constexpr
  bool show_plot = convertible::value && h.rank() == 1;
  if (show_plot) {
    detail::ostream(os, h, false);
    detail::plot(os, h, w, convertible{});
  } else {
    detail::ostream(os, h);
  }

  // restore fmt
  os.flags(flags);
  return os;
}

#endif // BOOST_HISTOGRAM_DOXYGEN_INVOKED

} // namespace histogram
} // namespace boost

#endif
