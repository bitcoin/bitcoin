//  This header intentionally has no include guards.
//
//  Copyright (c) 2001-2009, 2012 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#if !defined( BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS ) && !defined( BOOST_NO_CXX11_NULLPTR )\
    && !(defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, <= 0x5130))

    explicit operator bool () const BOOST_SP_NOEXCEPT
    {
        return px != 0;
    }

#elif ( defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, < 0x570) ) || defined(__CINT__)

    operator bool () const BOOST_SP_NOEXCEPT
    {
        return px != 0;
    }

#elif defined( _MANAGED )

    static void unspecified_bool( this_type*** )
    {
    }

    typedef void (*unspecified_bool_type)( this_type*** );

    operator unspecified_bool_type() const BOOST_SP_NOEXCEPT
    {
        return px == 0? 0: unspecified_bool;
    }

#elif \
    ( defined(__MWERKS__) && BOOST_WORKAROUND(__MWERKS__, < 0x3200) ) || \
    ( defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ < 304) ) || \
    ( defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, <= 0x590) )

    typedef element_type * (this_type::*unspecified_bool_type)() const;

    operator unspecified_bool_type() const BOOST_SP_NOEXCEPT
    {
        return px == 0? 0: &this_type::get;
    }

#else

    typedef element_type * this_type::*unspecified_bool_type;

    operator unspecified_bool_type() const BOOST_SP_NOEXCEPT
    {
        return px == 0? 0: &this_type::px;
    }

#endif

    // operator! is redundant, but some compilers need it
    bool operator! () const BOOST_SP_NOEXCEPT
    {
        return px == 0;
    }
