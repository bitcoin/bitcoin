// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_COUNTING_STREAMBUF_HPP
#define BOOST_HISTOGRAM_DETAIL_COUNTING_STREAMBUF_HPP

#include <boost/core/exchange.hpp>
#include <ostream>
#include <streambuf>

namespace boost {
namespace histogram {
namespace detail {

// detect how many characters will be printed by formatted output
template <class CharT, class Traits = std::char_traits<CharT>>
struct counting_streambuf : std::basic_streambuf<CharT, Traits> {
  using base_t = std::basic_streambuf<CharT, Traits>;
  using typename base_t::char_type;
  using typename base_t::int_type;

  std::streamsize* p_count;

  counting_streambuf(std::streamsize& c) : p_count(&c) {}

  std::streamsize xsputn(const char_type* /* s */, std::streamsize n) override {
    *p_count += n;
    return n;
  }

  int_type overflow(int_type ch) override {
    ++*p_count;
    return ch;
  }
};

template <class C, class T>
struct count_guard {
  using bos = std::basic_ostream<C, T>;
  using bsb = std::basic_streambuf<C, T>;

  counting_streambuf<C, T> csb;
  bos* p_os;
  bsb* p_rdbuf;

  count_guard(bos& os, std::streamsize& s) : csb(s), p_os(&os), p_rdbuf(os.rdbuf(&csb)) {}

  count_guard(count_guard&& o)
      : csb(o.csb), p_os(boost::exchange(o.p_os, nullptr)), p_rdbuf(o.p_rdbuf) {}

  count_guard& operator=(count_guard&& o) {
    if (this != &o) {
      csb = std::move(o.csb);
      p_os = boost::exchange(o.p_os, nullptr);
      p_rdbuf = o.p_rdbuf;
    }
    return *this;
  }

  ~count_guard() {
    if (p_os) p_os->rdbuf(p_rdbuf);
  }
};

template <class C, class T>
count_guard<C, T> make_count_guard(std::basic_ostream<C, T>& os, std::streamsize& s) {
  return {os, s};
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
