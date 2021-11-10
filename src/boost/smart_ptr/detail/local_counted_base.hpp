#ifndef BOOST_SMART_PTR_DETAIL_LOCAL_COUNTED_BASE_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_LOCAL_COUNTED_BASE_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/local_counted_base.hpp
//
//  Copyright 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.

#include <boost/smart_ptr/detail/shared_count.hpp>
#include <boost/config.hpp>
#include <utility>

namespace boost
{

namespace detail
{

class BOOST_SYMBOL_VISIBLE local_counted_base
{
private:

    local_counted_base & operator= ( local_counted_base const & );

private:

    // not 'int' or 'unsigned' to avoid aliasing and enable optimizations
    enum count_type { min_ = 0, initial_ = 1, max_ = 2147483647 };

    count_type local_use_count_;

public:

    BOOST_CONSTEXPR local_counted_base() BOOST_SP_NOEXCEPT: local_use_count_( initial_ )
    {
    }

    BOOST_CONSTEXPR local_counted_base( local_counted_base const & ) BOOST_SP_NOEXCEPT: local_use_count_( initial_ )
    {
    }

    virtual ~local_counted_base() /*BOOST_SP_NOEXCEPT*/
    {
    }

    virtual void local_cb_destroy() BOOST_SP_NOEXCEPT = 0;

    virtual boost::detail::shared_count local_cb_get_shared_count() const BOOST_SP_NOEXCEPT = 0;

    void add_ref() BOOST_SP_NOEXCEPT
    {
#if !defined(__NVCC__)
#if defined( __has_builtin )
# if __has_builtin( __builtin_assume )

        __builtin_assume( local_use_count_ >= 1 );

# endif
#endif
#endif

        local_use_count_ = static_cast<count_type>( local_use_count_ + 1 );
    }

    void release() BOOST_SP_NOEXCEPT
    {
        local_use_count_ = static_cast<count_type>( local_use_count_ - 1 );

        if( local_use_count_ == 0 )
        {
            local_cb_destroy();
        }
    }

    long local_use_count() const BOOST_SP_NOEXCEPT
    {
        return local_use_count_;
    }
};

class BOOST_SYMBOL_VISIBLE local_counted_impl: public local_counted_base
{
private:

    local_counted_impl( local_counted_impl const & );

private:

    shared_count pn_;

public:

    explicit local_counted_impl( shared_count const& pn ) BOOST_SP_NOEXCEPT: pn_( pn )
    {
    }

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    explicit local_counted_impl( shared_count && pn ) BOOST_SP_NOEXCEPT: pn_( std::move(pn) )
    {
    }

#endif

    void local_cb_destroy() BOOST_SP_NOEXCEPT BOOST_OVERRIDE
    {
        delete this;
    }

    boost::detail::shared_count local_cb_get_shared_count() const BOOST_SP_NOEXCEPT BOOST_OVERRIDE
    {
        return pn_;
    }
};

class BOOST_SYMBOL_VISIBLE local_counted_impl_em: public local_counted_base
{
public:

    shared_count pn_;

    void local_cb_destroy() BOOST_SP_NOEXCEPT BOOST_OVERRIDE
    {
        shared_count().swap( pn_ );
    }

    boost::detail::shared_count local_cb_get_shared_count() const BOOST_SP_NOEXCEPT BOOST_OVERRIDE
    {
        return pn_;
    }
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_LOCAL_COUNTED_BASE_HPP_INCLUDED
