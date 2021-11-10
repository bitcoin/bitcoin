//  boost/filesystem/detail/macro_value.hpp  -------------------------------------------//

//  (C) Copyright John Maddock 2001 - 2003
//  (C) Copyright Jens Maurer 2001
//  (C) Copyright Peter Dimov 2001
//  (C) Copyright Darin Adler 2001
//  (C) Copyright Beman Dawes 2002

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#ifndef BOOST_FILESYSTEM_MACRO_VALUE_HPP
#define BOOST_FILESYSTEM_MACRO_VALUE_HPP

#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <cstdlib>

namespace boost {
namespace detail {

inline const char* macro_value(const char* name, const char* value)
{
    static const char* no_value = "[no value]";
    static const char* not_defined = "[not defined]";

    BOOST_ASSERT_MSG(name, "name argument must not be a null pointer");
    BOOST_ASSERT_MSG(value, "value argument must not be a null pointer");

    return strcmp(name, value + 1) ? ((*value && *(value + 1)) ? (value + 1) : no_value) : not_defined; // name == value+1 so the macro is not defined
}

} // namespace detail
} // namespace boost

#define BOOST_MACRO_VALUE(X) boost::detail::macro_value(#X, BOOST_STRINGIZE(=X))

#endif // BOOST_FILESYSTEM_MACRO_VALUE_HPP
