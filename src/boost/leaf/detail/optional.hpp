#ifndef BOOST_LEAF_DETAIL_OPTIONAL_HPP_INCLUDED
#define BOOST_LEAF_DETAIL_OPTIONAL_HPP_INCLUDED

/// Copyright (c) 2018-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_LEAF_ENABLE_WARNINGS ///
#   if defined(_MSC_VER) ///
#       pragma warning(push,1) ///
#   elif defined(__clang__) ///
#       pragma clang system_header ///
#   elif (__GNUC__*100+__GNUC_MINOR__>301) ///
#       pragma GCC system_header ///
#   endif ///
#endif ///

#include <boost/leaf/detail/config.hpp>
#include <utility>
#include <new>

namespace boost { namespace leaf {

namespace leaf_detail
{
    template <class T>
    class optional
    {
        int key_;
        union { T value_; };

    public:

        typedef T value_type;

        BOOST_LEAF_CONSTEXPR optional() noexcept:
            key_(0)
        {
        }

        BOOST_LEAF_CONSTEXPR optional( optional const & x ):
            key_(x.key_)
        {
            if( x.key_ )
                (void) new (&value_) T( x.value_ );
        }

        BOOST_LEAF_CONSTEXPR optional( optional && x ) noexcept:
            key_(x.key_)
        {
            if( x.key_ )
            {
                (void) new (&value_) T( std::move(x.value_) );
                x.reset();
            }
        }

        BOOST_LEAF_CONSTEXPR optional( int key, T const & v ):
            key_(key),
            value_(v)
        {
            BOOST_LEAF_ASSERT(!empty());
        }

        BOOST_LEAF_CONSTEXPR optional( int key, T && v ) noexcept:
            key_(key),
            value_(std::move(v))
        {
            BOOST_LEAF_ASSERT(!empty());
        }

        BOOST_LEAF_CONSTEXPR optional & operator=( optional const & x )
        {
            reset();
            if( int key = x.key() )
            {
                put(key, x.value_);
                key_ = key;
            }
            return *this;
        }

        BOOST_LEAF_CONSTEXPR optional & operator=( optional && x ) noexcept
        {
            reset();
            if( int key = x.key() )
            {
                put(key, std::move(x.value_));
                x.reset();
            }
            return *this;
        }

        ~optional() noexcept
        {
            reset();
        }

        BOOST_LEAF_CONSTEXPR bool empty() const noexcept
        {
            return key_==0;
        }

        BOOST_LEAF_CONSTEXPR int key() const noexcept
        {
            return key_;
        }

        BOOST_LEAF_CONSTEXPR void reset() noexcept
        {
            if( key_ )
            {
                value_.~T();
                key_=0;
            }
        }

        BOOST_LEAF_CONSTEXPR T & put( int key, T const & v )
        {
            BOOST_LEAF_ASSERT(key);
            reset();
            (void) new(&value_) T(v);
            key_=key;
            return value_;
        }

        BOOST_LEAF_CONSTEXPR T & put( int key, T && v ) noexcept
        {
            BOOST_LEAF_ASSERT(key);
            reset();
            (void) new(&value_) T(std::move(v));
            key_=key;
            return value_;
        }

        BOOST_LEAF_CONSTEXPR T const * has_value(int key) const noexcept
        {
            BOOST_LEAF_ASSERT(key);
            return key_==key ? &value_ : 0;
        }

        BOOST_LEAF_CONSTEXPR T * has_value(int key) noexcept
        {
            BOOST_LEAF_ASSERT(key);
            return key_==key ? &value_ : 0;
        }

        BOOST_LEAF_CONSTEXPR T const & value(int key) const & noexcept
        {
            BOOST_LEAF_ASSERT(has_value(key) != 0);
            return value_;
        }

        BOOST_LEAF_CONSTEXPR T & value(int key) & noexcept
        {
            BOOST_LEAF_ASSERT(has_value(key) != 0);
            return value_;
        }

        BOOST_LEAF_CONSTEXPR T const && value(int key) const && noexcept
        {
            BOOST_LEAF_ASSERT(has_value(key) != 0);
            return value_;
        }

        BOOST_LEAF_CONSTEXPR T value(int key) && noexcept
        {
            BOOST_LEAF_ASSERT(has_value(key) != 0);
            T tmp(std::move(value_));
            reset();
            return tmp;
        }
    };

}

} }

#if defined(_MSC_VER) && !defined(BOOST_LEAF_ENABLE_WARNINGS) ///
#pragma warning(pop) ///
#endif ///

#endif
