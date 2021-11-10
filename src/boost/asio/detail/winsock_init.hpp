//
// detail/winsock_init.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_WINSOCK_INIT_HPP
#define BOOST_ASIO_DETAIL_WINSOCK_INIT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class winsock_init_base
{
protected:
  // Structure to track result of initialisation and number of uses. POD is used
  // to ensure that the values are zero-initialised prior to any code being run.
  struct data
  {
    long init_count_;
    long result_;
  };

  BOOST_ASIO_DECL static void startup(data& d,
      unsigned char major, unsigned char minor);

  BOOST_ASIO_DECL static void manual_startup(data& d);

  BOOST_ASIO_DECL static void cleanup(data& d);

  BOOST_ASIO_DECL static void manual_cleanup(data& d);

  BOOST_ASIO_DECL static void throw_on_error(data& d);
};

template <int Major = 2, int Minor = 0>
class winsock_init : private winsock_init_base
{
public:
  winsock_init(bool allow_throw = true)
  {
    startup(data_, Major, Minor);
    if (allow_throw)
      throw_on_error(data_);
  }

  winsock_init(const winsock_init&)
  {
    startup(data_, Major, Minor);
    throw_on_error(data_);
  }

  ~winsock_init()
  {
    cleanup(data_);
  }

  // This class may be used to indicate that user code will manage Winsock
  // initialisation and cleanup. This may be required in the case of a DLL, for
  // example, where it is not safe to initialise Winsock from global object
  // constructors.
  //
  // To prevent asio from initialising Winsock, the object must be constructed
  // before any Asio's own global objects. With MSVC, this may be accomplished
  // by adding the following code to the DLL:
  //
  //   #pragma warning(push)
  //   #pragma warning(disable:4073)
  //   #pragma init_seg(lib)
  //   boost::asio::detail::winsock_init<>::manual manual_winsock_init;
  //   #pragma warning(pop)
  class manual
  {
  public:
    manual()
    {
      manual_startup(data_);
    }

    manual(const manual&)
    {
      manual_startup(data_);
    }

    ~manual()
    {
      manual_cleanup(data_);
    }
  };

private:
  friend class manual;
  static data data_;
};

template <int Major, int Minor>
winsock_init_base::data winsock_init<Major, Minor>::data_;

// Static variable to ensure that winsock is initialised before main, and
// therefore before any other threads can get started.
static const winsock_init<>& winsock_init_instance = winsock_init<>(false);

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/winsock_init.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)

#endif // BOOST_ASIO_DETAIL_WINSOCK_INIT_HPP
