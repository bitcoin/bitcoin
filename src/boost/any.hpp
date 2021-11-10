// See http://www.boost.org/libs/any for Documentation.

#ifndef BOOST_ANY_INCLUDED
#define BOOST_ANY_INCLUDED

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

// what:  variant type boost::any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Antony Polukhin, Ed Brey, Mark Rodgers,
//        Peter Dimov, and James Curran
// when:  July 2001, April 2013 - 2020

#include <boost/any/bad_any_cast.hpp>
#include <boost/any/fwd.hpp>
#include <boost/type_index.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/core/addressof.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/conditional.hpp>

namespace boost
{
    class any
    {
    public: // structors

        BOOST_CONSTEXPR any() BOOST_NOEXCEPT
          : content(0)
        {
        }

        template<typename ValueType>
        any(const ValueType & value)
          : content(new holder<
                BOOST_DEDUCED_TYPENAME remove_cv<BOOST_DEDUCED_TYPENAME decay<const ValueType>::type>::type
            >(value))
        {
            BOOST_STATIC_ASSERT_MSG(
                !anys::detail::is_basic_any<ValueType>::value,
                "boost::any shall not be constructed from boost::anys::basic_any"
            );
        }

        any(const any & other)
          : content(other.content ? other.content->clone() : 0)
        {
        }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        // Move constructor
        any(any&& other) BOOST_NOEXCEPT
          : content(other.content)
        {
            other.content = 0;
        }

        // Perfect forwarding of ValueType
        template<typename ValueType>
        any(ValueType&& value
            , typename boost::disable_if<boost::is_same<any&, ValueType> >::type* = 0 // disable if value has type `any&`
            , typename boost::disable_if<boost::is_const<ValueType> >::type* = 0) // disable if value has type `const ValueType&&`
          : content(new holder< typename decay<ValueType>::type >(static_cast<ValueType&&>(value)))
        {
            BOOST_STATIC_ASSERT_MSG(
                !anys::detail::is_basic_any<typename boost::decay<ValueType>::type>::value,
                "boost::any shall not be constructed from boost::anys::basic_any"
            );
        }
#endif

        ~any() BOOST_NOEXCEPT
        {
            delete content;
        }

    public: // modifiers

        any & swap(any & rhs) BOOST_NOEXCEPT
        {
            placeholder* tmp = content;
            content = rhs.content;
            rhs.content = tmp;
            return *this;
        }


#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        template<typename ValueType>
        any & operator=(const ValueType & rhs)
        {
            BOOST_STATIC_ASSERT_MSG(
                !anys::detail::is_basic_any<ValueType>::value,
                "boost::anys::basic_any shall not be assigned into boost::any"
            );
            any(rhs).swap(*this);
            return *this;
        }

        any & operator=(any rhs)
        {
            rhs.swap(*this);
            return *this;
        }

#else
        any & operator=(const any& rhs)
        {
            any(rhs).swap(*this);
            return *this;
        }

        // move assignment
        any & operator=(any&& rhs) BOOST_NOEXCEPT
        {
            rhs.swap(*this);
            any().swap(rhs);
            return *this;
        }

        // Perfect forwarding of ValueType
        template <class ValueType>
        any & operator=(ValueType&& rhs)
        {
            BOOST_STATIC_ASSERT_MSG(
                !anys::detail::is_basic_any<typename boost::decay<ValueType>::type>::value,
                "boost::anys::basic_any shall not be assigned into boost::any"
            );
            any(static_cast<ValueType&&>(rhs)).swap(*this);
            return *this;
        }
#endif

    public: // queries

        bool empty() const BOOST_NOEXCEPT
        {
            return !content;
        }

        void clear() BOOST_NOEXCEPT
        {
            any().swap(*this);
        }

        const boost::typeindex::type_info& type() const BOOST_NOEXCEPT
        {
            return content ? content->type() : boost::typeindex::type_id<void>().type_info();
        }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
    private: // types
#else
    public: // types (public so any_cast can be non-friend)
#endif

        class BOOST_SYMBOL_VISIBLE placeholder
        {
        public: // structors

            virtual ~placeholder()
            {
            }

        public: // queries

            virtual const boost::typeindex::type_info& type() const BOOST_NOEXCEPT = 0;

            virtual placeholder * clone() const = 0;

        };

        template<typename ValueType>
        class holder
#ifndef BOOST_NO_CXX11_FINAL
          final
#endif
          : public placeholder
        {
        public: // structors

            holder(const ValueType & value)
              : held(value)
            {
            }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
            holder(ValueType&& value)
              : held(static_cast< ValueType&& >(value))
            {
            }
#endif
        public: // queries

            const boost::typeindex::type_info& type() const BOOST_NOEXCEPT BOOST_OVERRIDE
            {
                return boost::typeindex::type_id<ValueType>().type_info();
            }

            placeholder * clone() const BOOST_OVERRIDE
            {
                return new holder(held);
            }

        public: // representation

            ValueType held;

        private: // intentionally left unimplemented
            holder & operator=(const holder &);
        };

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS

    private: // representation

        template<typename ValueType>
        friend ValueType * any_cast(any *) BOOST_NOEXCEPT;

        template<typename ValueType>
        friend ValueType * unsafe_any_cast(any *) BOOST_NOEXCEPT;

#else

    public: // representation (public so any_cast can be non-friend)

#endif

        placeholder * content;

    };

    inline void swap(any & lhs, any & rhs) BOOST_NOEXCEPT
    {
        lhs.swap(rhs);
    }

    template<typename ValueType>
    ValueType * any_cast(any * operand) BOOST_NOEXCEPT
    {
        return operand && operand->type() == boost::typeindex::type_id<ValueType>()
            ? boost::addressof(
                static_cast<any::holder<BOOST_DEDUCED_TYPENAME remove_cv<ValueType>::type> *>(operand->content)->held
              )
            : 0;
    }

    template<typename ValueType>
    inline const ValueType * any_cast(const any * operand) BOOST_NOEXCEPT
    {
        return any_cast<ValueType>(const_cast<any *>(operand));
    }

    template<typename ValueType>
    ValueType any_cast(any & operand)
    {
        typedef BOOST_DEDUCED_TYPENAME remove_reference<ValueType>::type nonref;


        nonref * result = any_cast<nonref>(boost::addressof(operand));
        if(!result)
            boost::throw_exception(bad_any_cast());

        // Attempt to avoid construction of a temporary object in cases when
        // `ValueType` is not a reference. Example:
        // `static_cast<std::string>(*result);`
        // which is equal to `std::string(*result);`
        typedef BOOST_DEDUCED_TYPENAME boost::conditional<
            boost::is_reference<ValueType>::value,
            ValueType,
            BOOST_DEDUCED_TYPENAME boost::add_reference<ValueType>::type
        >::type ref_type;

#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable: 4172) // "returning address of local variable or temporary" but *result is not local!
#endif
        return static_cast<ref_type>(*result);
#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif
    }

    template<typename ValueType>
    inline ValueType any_cast(const any & operand)
    {
        typedef BOOST_DEDUCED_TYPENAME remove_reference<ValueType>::type nonref;
        return any_cast<const nonref &>(const_cast<any &>(operand));
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template<typename ValueType>
    inline ValueType any_cast(any&& operand)
    {
        BOOST_STATIC_ASSERT_MSG(
            boost::is_rvalue_reference<ValueType&&>::value /*true if ValueType is rvalue or just a value*/
            || boost::is_const< typename boost::remove_reference<ValueType>::type >::value,
            "boost::any_cast shall not be used for getting nonconst references to temporary objects"
        );
        return any_cast<ValueType>(operand);
    }
#endif


    // Note: The "unsafe" versions of any_cast are not part of the
    // public interface and may be removed at any time. They are
    // required where we know what type is stored in the any and can't
    // use typeid() comparison, e.g., when our types may travel across
    // different shared libraries.
    template<typename ValueType>
    inline ValueType * unsafe_any_cast(any * operand) BOOST_NOEXCEPT
    {
        return boost::addressof(
            static_cast<any::holder<ValueType> *>(operand->content)->held
        );
    }

    template<typename ValueType>
    inline const ValueType * unsafe_any_cast(const any * operand) BOOST_NOEXCEPT
    {
        return unsafe_any_cast<ValueType>(const_cast<any *>(operand));
    }
}

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
// Copyright Antony Polukhin, 2013-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#endif
