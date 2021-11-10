// Copyright (C) 2014, Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  akrzemi1@gmail.com
//
#ifndef BOOST_BAD_OPTIONAL_ACCESS_22MAY2014_HPP
#define BOOST_BAD_OPTIONAL_ACCESS_22MAY2014_HPP

#include <stdexcept>
#if __cplusplus < 201103L
#include <string> // to make converting-ctor std::string(char const*) visible
#endif

namespace boost {

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wweak-vtables"
#endif

class bad_optional_access : public std::logic_error
{
public:
  bad_optional_access()
    : std::logic_error("Attempted to access the value of an uninitialized optional object.")
    {}
};

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

} // namespace boost

#endif
