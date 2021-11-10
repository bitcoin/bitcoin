/*=============================================================================
  Copyright (c) 2001-2008 Joel de Guzman
  Copyright (c) 2001-2008 Hartmut Kaiser
  http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_NAMESPACE_HPP
#define BOOST_SPIRIT_CLASSIC_NAMESPACE_HPP

#if defined(BOOST_SPIRIT_USE_OLD_NAMESPACE)

// Use the old namespace for Spirit.Classic, everything is located in the 
// namespace boost::spirit.
// This is in place for backwards compatibility with Spirit V1.8.x. Don't use
// it when combining Spirit.Classic with other parts of the library

#define BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN /*namespace classic {*/
#define BOOST_SPIRIT_CLASSIC_NS              boost::spirit/*::classic*/
#define BOOST_SPIRIT_CLASSIC_NAMESPACE_END   /*}*/

#else

// This is the normal (and suggested) mode of operation when using 
// Spirit.Classic. Everything will be located in the namespace 
// boost::spirit::classic, avoiding name clashes with other parts of Spirit.

#define BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN namespace classic {
#define BOOST_SPIRIT_CLASSIC_NS              boost::spirit::classic
#define BOOST_SPIRIT_CLASSIC_NAMESPACE_END   }

#endif

#endif
