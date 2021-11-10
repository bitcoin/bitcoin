/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden
    Copyright (c) 2009-2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STD_PAIR_HPP
#define BOOST_FUSION_ADAPTED_STD_PAIR_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <utility>

BOOST_FUSION_ADAPT_TPL_STRUCT(
    (T1)(T2),(std::pair)(T1)(T2),(T1, first)(T2, second))

#endif
