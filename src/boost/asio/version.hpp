//
// version.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_VERSION_HPP
#define BOOST_ASIO_VERSION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

// BOOST_ASIO_VERSION % 100 is the sub-minor version
// BOOST_ASIO_VERSION / 100 % 1000 is the minor version
// BOOST_ASIO_VERSION / 100000 is the major version
#define BOOST_ASIO_VERSION 102000 // 1.20.0

#endif // BOOST_ASIO_VERSION_HPP
