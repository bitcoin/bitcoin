//  boost/timer/config.hpp  -----------------------------------------------------------//

//  Copyright Beman Dawes 2003, 2006, 2011

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/timer for documentation.

#ifndef BOOST_TIMER_CONFIG_HPP
#define BOOST_TIMER_CONFIG_HPP

#include <boost/config.hpp>

#include <boost/system/api_config.hpp> 

// This header implements separate compilation features as described in
// http://www.boost.org/more/separate_compilation.html

//  enable dynamic or static linking as requested --------------------------------------//

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TIMER_DYN_LINK)
# if defined(BOOST_TIMER_SOURCE)
#   define BOOST_TIMER_DECL BOOST_SYMBOL_EXPORT
# else
#   define BOOST_TIMER_DECL BOOST_SYMBOL_IMPORT
# endif
#else
# define BOOST_TIMER_DECL
#endif

//  enable automatic library variant selection  ----------------------------------------//

#if !defined(BOOST_TIMER_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_TIMER_NO_LIB)
//
// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define BOOST_LIB_NAME boost_timer
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TIMER_DYN_LINK)
#  define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>

// We also need to autolink to the Chrono library; even though
// it's not used in the interface, and no Chrono header is included,
// it's used in the implementation and is necessary in order to link

#if !defined(BOOST_CHRONO_NO_LIB)

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_CHRONO_DYN_LINK)
#  define BOOST_DYN_LINK
#endif

#define BOOST_LIB_NAME boost_chrono

#include <boost/config/auto_link.hpp>

#endif // !defined(BOOST_CHRONO_NO_LIB)

#endif  // auto-linking disabled

#endif // BOOST_TIMER_CONFIG_HPP
