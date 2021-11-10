//Copyright (c) 2006-2010 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_EXCEPTION_CE6983AC753411DDA764247956D89593
#define BOOST_EXCEPTION_CE6983AC753411DDA764247956D89593

#include <boost/config.hpp>
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#endif
#include <utility>
#include <string>

#ifndef BOOST_EXCEPTION_ENABLE_WARNINGS
#if __GNUC__*100+__GNUC_MINOR__>301
#pragma GCC system_header
#endif
#ifdef __clang__
#pragma clang system_header
#endif
#ifdef _MSC_VER
#pragma warning(push,1)
#endif
#endif

namespace
boost
    {
    namespace
    exception_detail
        {
        class
        error_info_base
            {
            public:

            virtual std::string name_value_string() const = 0;
            virtual error_info_base * clone() const = 0;

            virtual
            ~error_info_base() BOOST_NOEXCEPT_OR_NOTHROW
                {
                }
            };
        }

    template <class Tag,class T>
    class
    error_info:
        public exception_detail::error_info_base
        {
        exception_detail::error_info_base *
        clone() const
            {
            return new error_info<Tag,T>(*this);
            }
        public:
        typedef T value_type;
        error_info( value_type const & v ):
            v_(v)
            {
            }
#if (__GNUC__*100+__GNUC_MINOR__!=406) //workaround for g++ bug
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        error_info( error_info const & x ):
            v_(x.v_)
            {
            }
        error_info( T && v ) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible<T>::value):
            v_(std::move(v))
            {
            }
        error_info( error_info && x ) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible<T>::value):
            v_(std::move(x.v_))
            {
            }
#endif
#endif
        ~error_info() BOOST_NOEXCEPT_OR_NOTHROW
            {
            }
        value_type const &
        value() const
            {
            return v_;
            }
        value_type &
        value()
            {
            return v_;
            }
        private:
        error_info & operator=( error_info const & );
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        error_info & operator=( error_info && x );
#endif
        std::string name_value_string() const;
        value_type v_;
        };
    }

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif
