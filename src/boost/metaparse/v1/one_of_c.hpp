#ifndef BOOST_METAPARSE_V1_ONE_OF_C_HPP
#define BOOST_METAPARSE_V1_ONE_OF_C_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/config.hpp>

#if BOOST_METAPARSE_STD >= 2014
#  include <boost/metaparse/v1/cpp14/one_of_c.hpp>
#elif BOOST_METAPARSE_STD >= 2011
#  include <boost/metaparse/v1/cpp11/one_of_c.hpp>
#else
#  include <boost/metaparse/v1/cpp98/one_of_c.hpp>
#endif

#endif

