/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2001-2003 Hartmut Kaiser
    Copyright (c) 2002-2003 Martin Wille
    Copyright (c) 2002 Juan Carlos Arevalo-Baeza
    Copyright (c) 2002 Raghavendra Satish
    Copyright (c) 2002 Jeff Westfahl
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_UTILITY_MAIN_HPP)
#define BOOST_SPIRIT_UTILITY_MAIN_HPP

#include <boost/spirit/home/classic/version.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  Master header for Spirit.Utilities
//
///////////////////////////////////////////////////////////////////////////////

// Utility.Parsers
#include <boost/spirit/home/classic/utility/chset.hpp>
#include <boost/spirit/home/classic/utility/chset_operators.hpp>
#include <boost/spirit/home/classic/utility/escape_char.hpp>
#include <boost/spirit/home/classic/utility/functor_parser.hpp>
#include <boost/spirit/home/classic/utility/loops.hpp>
#include <boost/spirit/home/classic/utility/confix.hpp>
#include <boost/spirit/home/classic/utility/lists.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>

// Utility.Support
#include <boost/spirit/home/classic/utility/flush_multi_pass.hpp>
#ifdef BOOST_SPIRIT_THREADSAFE
#include <boost/spirit/home/classic/utility/scoped_lock.hpp>
#endif


#endif // !defined(BOOST_SPIRIT_UTILITY_MAIN_HPP)
