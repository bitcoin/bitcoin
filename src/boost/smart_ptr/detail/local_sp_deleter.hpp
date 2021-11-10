#ifndef BOOST_SMART_PTR_DETAIL_LOCAL_SP_DELETER_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_LOCAL_SP_DELETER_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/local_sp_deleter.hpp
//
//  Copyright 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#include <boost/smart_ptr/detail/local_counted_base.hpp>
#include <boost/config.hpp>

namespace boost
{

namespace detail
{

template<class D> class local_sp_deleter: public local_counted_impl_em
{
private:

    D d_;

public:

    local_sp_deleter(): d_()
    {
    }

    explicit local_sp_deleter( D const& d ) BOOST_SP_NOEXCEPT: d_( d )
    {
    }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    explicit local_sp_deleter( D&& d ) BOOST_SP_NOEXCEPT: d_( std::move(d) )
    {
    }

#endif

    D& deleter() BOOST_SP_NOEXCEPT
    {
        return d_;
    }

    template<class Y> void operator()( Y* p ) BOOST_SP_NOEXCEPT
    {
        d_( p );
    }

#if !defined( BOOST_NO_CXX11_NULLPTR )

    void operator()( boost::detail::sp_nullptr_t p ) BOOST_SP_NOEXCEPT
    {
        d_( p );
    }

#endif
};

template<> class local_sp_deleter<void>
{
};

template<class D> D * get_local_deleter( local_sp_deleter<D> * p ) BOOST_SP_NOEXCEPT
{
    return &p->deleter();
}

inline void * get_local_deleter( local_sp_deleter<void> * /*p*/ ) BOOST_SP_NOEXCEPT
{
    return 0;
}

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_LOCAL_SP_DELETER_HPP_INCLUDED
