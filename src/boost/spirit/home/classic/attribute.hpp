/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ATTRIBUTE_MAIN_HPP)
#define BOOST_SPIRIT_ATTRIBUTE_MAIN_HPP

#include <boost/spirit/home/classic/version.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  Master header for Spirit.Attributes
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  Phoenix predefined maximum limit. This limit defines the maximum
//  number of elements a tuple can hold. This number defaults to 3. The
//  actual maximum is rounded up in multiples of 3. Thus, if this value
//  is 4, the actual limit is 6. The ultimate maximum limit in this
//  implementation is 15.
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(PHOENIX_LIMIT)
#define PHOENIX_LIMIT 6
#endif // !defined(PHOENIX_LIMIT)

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/attribute/parametric.hpp>
#include <boost/spirit/home/classic/attribute/closure.hpp>

#endif // !defined(BOOST_SPIRIT_ATTRIBUTE_MAIN_HPP)
