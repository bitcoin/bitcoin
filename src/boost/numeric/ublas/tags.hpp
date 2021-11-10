/**
 * -*- c++ -*-
 *
 * \file tags.hpp
 *
 * \brief Tags.
 *
 * Copyright (c) 2009, Marco Guazzone
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * \author Marco Guazzone, marco.guazzone@gmail.com
 */

#ifndef BOOST_NUMERIC_UBLAS_TAG_HPP
#define BOOST_NUMERIC_UBLAS_TAG_HPP


namespace boost { namespace numeric { namespace ublas { namespace tag {

/// \brief Tag for the major dimension.
struct major {};


/// \brief Tag for the minor dimension.
struct minor {};


/// \brief Tag for the leading dimension.
struct leading {};

}}}} // Namespace boost::numeric::ublas::tag


#endif // BOOST_NUMERIC_UBLAS_TAG_HPP
