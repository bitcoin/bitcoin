//  boost polymorphic_pointer_cast.hpp header file  ----------------------------------------------//
//  (C) Copyright Boris Rasin and Antony Polukhin 2014-2021.
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/conversion for Documentation.

#ifndef BOOST_CONVERSION_POLYMORPHIC_POINTER_CAST_HPP
#define BOOST_CONVERSION_POLYMORPHIC_POINTER_CAST_HPP

# include <boost/config.hpp>
# include <boost/assert.hpp>
# include <boost/pointer_cast.hpp>
# include <boost/throw_exception.hpp>
# include <boost/utility/declval.hpp>
# ifdef BOOST_NO_CXX11_DECLTYPE
#   include <boost/typeof/typeof.hpp>
# endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

namespace boost
{
//  See the documentation for descriptions of how to choose between
//  static_pointer_cast<>, dynamic_pointer_cast<>, polymorphic_pointer_cast<> and polymorphic_pointer_downcast<>

//  polymorphic_pointer_downcast  --------------------------------------------//

    //  BOOST_ASSERT() checked polymorphic downcast.  Crosscasts prohibited.
    //  Supports any type with static_pointer_cast/dynamic_pointer_cast functions:
    //  built-in pointers, std::shared_ptr, boost::shared_ptr, boost::intrusive_ptr, etc.

    //  WARNING: Because this cast uses BOOST_ASSERT(), it violates
    //  the One Definition Rule if used in multiple translation units
    //  where BOOST_DISABLE_ASSERTS, BOOST_ENABLE_ASSERT_HANDLER
    //  NDEBUG are defined inconsistently.

    //  Contributed by Boris Rasin

    namespace detail
    {
        template <typename Target, typename Source>
        struct dynamic_pointer_cast_result
        {
#ifdef BOOST_NO_CXX11_DECLTYPE
            BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, dynamic_pointer_cast<Target>(boost::declval<Source>()))
            typedef typename nested::type type;
#else
            typedef decltype(dynamic_pointer_cast<Target>(boost::declval<Source>())) type;
#endif
        };
    }

    template <typename Target, typename Source>
    inline typename detail::dynamic_pointer_cast_result<Target, Source>::type
    polymorphic_pointer_downcast (const Source& x)
    {
        BOOST_ASSERT(dynamic_pointer_cast<Target> (x) == x);
        return static_pointer_cast<Target> (x);
    }

    template <typename Target, typename Source>
    inline typename detail::dynamic_pointer_cast_result<Target, Source>::type
    polymorphic_pointer_cast (const Source& x)
    {
        typename detail::dynamic_pointer_cast_result<Target, Source>::type tmp
            = dynamic_pointer_cast<Target> (x);
        if ( !tmp ) boost::throw_exception( std::bad_cast() );

        return tmp;
    }

} // namespace boost

#endif  // BOOST_CONVERSION_POLYMORPHIC_POINTER_CAST_HPP
