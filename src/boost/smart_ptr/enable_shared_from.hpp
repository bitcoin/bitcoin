#ifndef BOOST_SMART_PTR_ENABLE_SHARED_FROM_HPP_INCLUDED
#define BOOST_SMART_PTR_ENABLE_SHARED_FROM_HPP_INCLUDED

//  enable_shared_from.hpp
//
//  Copyright 2019, 2020 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/smart_ptr/detail/sp_noexcept.hpp>

namespace boost
{

class enable_shared_from: public enable_shared_from_this<enable_shared_from>
{
private:

    using enable_shared_from_this<enable_shared_from>::shared_from_this;
    using enable_shared_from_this<enable_shared_from>::weak_from_this;
};


template<class T> shared_ptr<T> shared_from( T * p )
{
    return shared_ptr<T>( p->enable_shared_from_this<enable_shared_from>::shared_from_this(), p );
}

template<class T> weak_ptr<T> weak_from( T * p ) BOOST_SP_NOEXCEPT
{
    return weak_ptr<T>( p->enable_shared_from_this<enable_shared_from>::weak_from_this(), p );
}

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_ENABLE_SHARED_FROM_HPP_INCLUDED
