// Copyright (c) 2016-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_CORE_HPP
#define BOOST_PFR_DETAIL_CORE_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

// Each core provides `boost::pfr::detail::tie_as_tuple` and
// `boost::pfr::detail::for_each_field_dispatcher` functions.
//
// The whole PFR library is build on top of those two functions.
#if BOOST_PFR_USE_CPP17
#   include <boost/pfr/detail/core17.hpp>
#elif BOOST_PFR_USE_LOOPHOLE
#   include <boost/pfr/detail/core14_loophole.hpp>
#else
#   include <boost/pfr/detail/core14_classic.hpp>
#endif

#endif // BOOST_PFR_DETAIL_CORE_HPP
