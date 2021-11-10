#ifndef BOOST_DESCRIBE_DETAIL_CONFIG_HPP_INCLUDED
#define BOOST_DESCRIBE_DETAIL_CONFIG_HPP_INCLUDED

// Copyright 2021 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#if __cplusplus >= 201402L

# define BOOST_DESCRIBE_CXX14
# define BOOST_DESCRIBE_CXX11

#elif defined(_MSC_VER) && _MSC_VER >= 1900

# define BOOST_DESCRIBE_CXX14
# define BOOST_DESCRIBE_CXX11

#elif __cplusplus >= 201103L

# define BOOST_DESCRIBE_CXX11

#endif

#if defined(BOOST_DESCRIBE_CXX11)
# define BOOST_DESCRIBE_CONSTEXPR_OR_CONST constexpr
#else
# define BOOST_DESCRIBE_CONSTEXPR_OR_CONST const
#endif

#endif // #ifndef BOOST_DESCRIBE_DETAIL_CONFIG_HPP_INCLUDED
