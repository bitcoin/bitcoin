// Copyright 2021 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_TERM_INFO_HPP
#define BOOST_HISTOGRAM_DETAIL_TERM_INFO_HPP

#include <algorithm>

#if defined __has_include
#if __has_include(<sys/ioctl.h>) && __has_include(<unistd.h>)
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#endif
#include <boost/config.hpp>
#include <cstdlib>
#include <cstring>

namespace boost {
namespace histogram {
namespace detail {

namespace term_info {
class env_t {
public:
  env_t(const char* key) {
#if defined(BOOST_MSVC) // msvc complains about using std::getenv
    _dupenv_s(&data_, &size_, key);
#else
    data_ = std::getenv(key);
    if (data_) size_ = std::strlen(data_);
#endif
  }

  ~env_t() {
#if defined(BOOST_MSVC)
    std::free(data_);
#endif
  }

  bool contains(const char* s) {
    const std::size_t n = std::strlen(s);
    if (size_ < n) return false;
    return std::strstr(data_, s);
  }

  operator bool() { return size_ > 0; }

  explicit operator int() { return size_ ? std::atoi(data_) : 0; }

  const char* data() const { return data_; }

private:
  char* data_;
  std::size_t size_ = 0;
};

inline bool utf8() {
  // return false only if LANG exists and does not contain the string UTF
  env_t env("LANG");
  bool b = true;
  if (env) b = env.contains("UTF") || env.contains("utf");
  return b;
}

inline int width() {
  int w = 0;
#if defined TIOCGWINSZ
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  w = (std::max)(static_cast<int>(ws.ws_col), 0); // not sure if ws_col can be less than 0
#endif
  env_t env("COLUMNS");
  const int col = (std::max)(static_cast<int>(env), 0);
  // if both t and w are set, COLUMNS may be used to restrict width
  return w == 0 ? col : (std::min)(col, w);
}
} // namespace term_info

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
