// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_HPP
#define BOOST_HISTOGRAM_HPP

/**
  \file boost/histogram.hpp
  Includes all standard headers of the Boost.Histogram library.

  Extra headers not automatically included are:
    - [boost/histogram/ostream.hpp][1]
    - [boost/histogram/axis/ostream.hpp][2]
    - [boost/histogram/accumulators/ostream.hpp][3]
    - [boost/histogram/serialization.hpp][4]

  [1]: histogram/reference.html#header.boost.histogram.ostream_hpp
  [2]: histogram/reference.html#header.boost.histogram.axis.ostream_hpp
  [3]: histogram/reference.html#header.boost.histogram.accumulators.ostream_hpp
  [4]: histogram/reference.html#header.boost.histogram.serialization_hpp
*/

#include <boost/histogram/accumulators.hpp>
#include <boost/histogram/algorithm.hpp>
#include <boost/histogram/axis.hpp>
#include <boost/histogram/histogram.hpp>
#include <boost/histogram/indexed.hpp>
#include <boost/histogram/literals.hpp>
#include <boost/histogram/make_histogram.hpp>
#include <boost/histogram/make_profile.hpp>
#include <boost/histogram/storage_adaptor.hpp>
#include <boost/histogram/unlimited_storage.hpp>

#endif
