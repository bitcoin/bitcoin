#ifndef BOOST_SMART_PTR_OWNER_HASH_HPP_INCLUDED
#define BOOST_SMART_PTR_OWNER_HASH_HPP_INCLUDED

// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>
#include <cstddef>

namespace boost
{

template<class T> struct owner_hash
{
    typedef std::size_t result_type;
    typedef T argument_type;

    std::size_t operator()( T const & t ) const BOOST_NOEXCEPT
    {
        return t.owner_hash_value();
    }
};

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_OWNER_HASH_HPP_INCLUDED
