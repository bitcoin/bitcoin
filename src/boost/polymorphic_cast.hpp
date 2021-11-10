//  boost polymorphic_cast.hpp header file  ----------------------------------------------//

//  (C) Copyright Kevlin Henney and Dave Abrahams 1999.
//  (C) Copyright Boris Rasin 2014.
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/conversion for Documentation.

//  Revision History
//  10 Nov 14  polymorphic_pointer_downcast moved to a separate header,
//             minor improvements to stisfy latest Boost coding style
//  08 Nov 14  Add polymorphic_pointer_downcast (Boris Rasin)
//  09 Jun 14  "cast.hpp" was renamed to "polymorphic_cast.hpp" and
//             inclusion of numeric_cast was removed (Antony Polukhin)
//  23 Jun 05  numeric_cast removed and redirected to the new verion (Fernando Cacciola)
//  02 Apr 01  Removed BOOST_NO_LIMITS workarounds and included
//             <boost/limits.hpp> instead (the workaround did not
//             actually compile when BOOST_NO_LIMITS was defined in
//             any case, so we loose nothing). (John Maddock)
//  21 Jan 01  Undid a bug I introduced yesterday. numeric_cast<> never
//             worked with stock GCC; trying to get it to do that broke
//             vc-stlport.
//  20 Jan 01  Moved BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS to config.hpp.
//             Removed unused BOOST_EXPLICIT_TARGET macro. Moved
//             boost::detail::type to boost/type.hpp. Made it compile with
//             stock gcc again (Dave Abrahams)
//  29 Nov 00  Remove nested namespace cast, cleanup spacing before Formal
//             Review (Beman Dawes)
//  19 Oct 00  Fix numeric_cast for floating-point types (Dave Abrahams)
//  15 Jul 00  Suppress numeric_cast warnings for GCC, Borland and MSVC
//             (Dave Abrahams)
//  30 Jun 00  More MSVC6 wordarounds.  See comments below.  (Dave Abrahams)
//  28 Jun 00  Removed implicit_cast<>.  See comment below. (Beman Dawes)
//  27 Jun 00  More MSVC6 workarounds
//  15 Jun 00  Add workarounds for MSVC6
//   2 Feb 00  Remove bad_numeric_cast ";" syntax error (Doncho Angelov)
//  26 Jan 00  Add missing throw() to bad_numeric_cast::what(0 (Adam Levar)
//  29 Dec 99  Change using declarations so usages in other namespaces work
//             correctly (Dave Abrahams)
//  23 Sep 99  Change polymorphic_downcast assert to also detect M.I. errors
//             as suggested Darin Adler and improved by Valentin Bonnard.
//   2 Sep 99  Remove controversial asserts, simplify, rename.
//  30 Aug 99  Move to cast.hpp, replace value_cast with numeric_cast,
//             place in nested namespace.
//   3 Aug 99  Initial version

#ifndef BOOST_POLYMORPHIC_CAST_HPP
#define BOOST_POLYMORPHIC_CAST_HPP

# include <boost/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

# include <boost/assert.hpp>
# include <boost/core/addressof.hpp>
# include <boost/core/enable_if.hpp>
# include <boost/throw_exception.hpp>
# include <boost/type_traits/is_reference.hpp> 
# include <boost/type_traits/remove_reference.hpp>

# include <typeinfo>

namespace boost
{
//  See the documentation for descriptions of how to choose between
//  static_cast<>, dynamic_cast<>, polymorphic_cast<> and polymorphic_downcast<>

//  polymorphic_cast  --------------------------------------------------------//

    //  Runtime checked polymorphic downcasts and crosscasts.
    //  Suggested in The C++ Programming Language, 3rd Ed, Bjarne Stroustrup,
    //  section 15.8 exercise 1, page 425.

    template <class Target, class Source>
    inline Target polymorphic_cast(Source* x)
    {
        Target tmp = dynamic_cast<Target>(x);
        if ( tmp == 0 ) boost::throw_exception( std::bad_cast() );
        return tmp;
    }

//  polymorphic_downcast  ----------------------------------------------------//

    //  BOOST_ASSERT() checked raw pointer polymorphic downcast.  Crosscasts prohibited.

    //  WARNING: Because this cast uses BOOST_ASSERT(), it violates
    //  the One Definition Rule if used in multiple translation units
    //  where BOOST_DISABLE_ASSERTS, BOOST_ENABLE_ASSERT_HANDLER
    //  NDEBUG are defined inconsistently.

    //  Contributed by Dave Abrahams

    template <class Target, class Source>
    inline Target polymorphic_downcast(Source* x)
    {
        BOOST_ASSERT( dynamic_cast<Target>(x) == x );  // detect logic error
        return static_cast<Target>(x);
    }

    //  BOOST_ASSERT() checked reference polymorphic downcast.  Crosscasts prohibited.

    //  WARNING: Because this cast uses BOOST_ASSERT(), it violates
    //  the One Definition Rule if used in multiple translation units
    //  where BOOST_DISABLE_ASSERTS, BOOST_ENABLE_ASSERT_HANDLER
    //  NDEBUG are defined inconsistently.

    //  Contributed by Julien Delacroix

    template <class Target, class Source>
    inline typename boost::enable_if_c<
        boost::is_reference<Target>::value, Target
    >::type polymorphic_downcast(Source& x)
    {
        typedef typename boost::remove_reference<Target>::type* target_pointer_type;
        return *boost::polymorphic_downcast<target_pointer_type>(
            boost::addressof(x)
        );
    }

} // namespace boost

#endif  // BOOST_POLYMORPHIC_CAST_HPP
