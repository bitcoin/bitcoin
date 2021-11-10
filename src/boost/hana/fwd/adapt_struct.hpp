/*!
@file
Documents the `BOOST_HANA_ADAPT_STRUCT` macro.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_ADAPT_STRUCT_HPP
#define BOOST_HANA_FWD_ADAPT_STRUCT_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    // Note:
    // The weird definition as a variable seems to exploit a glitch in Doxygen
    // which makes the macro appear in the related objects of Struct (as we
    // want it to).

    //! Defines a model of `Struct` with the given members.
    //! @ingroup group-Struct
    //!
    //! Using this macro at _global scope_ will define a model of the `Struct`
    //! concept for the given type. This can be used to easily adapt existing
    //! user-defined types in a ad-hoc manner. Unlike the
    //! `BOOST_HANA_DEFINE_STRUCT` macro, this macro does not
    //! require the types of the members to be specified.
    //!
    //! @note
    //! This macro only works if the tag of the user-defined type `T` is `T`
    //! itself. This is the case unless you specifically asked for something
    //! different; see `tag_of`'s documentation.
    //!
    //!
    //! Example
    //! -------
    //! @include example/adapt_struct.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    auto BOOST_HANA_ADAPT_STRUCT(...) = ;
    #define BOOST_HANA_ADAPT_STRUCT(Name, ...) see documentation
#else
    // defined in <boost/hana/adapt_struct.hpp>
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_ADAPT_STRUCT_HPP
