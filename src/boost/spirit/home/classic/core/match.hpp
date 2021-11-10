/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_MATCH_HPP)
#define BOOST_SPIRIT_MATCH_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/config.hpp>
#include <boost/spirit/home/classic/core/nil.hpp>
#include <boost/call_traits.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/home/classic/core/assert.hpp>
#include <boost/spirit/home/classic/core/safe_bool.hpp>
#include <boost/spirit/home/classic/core/impl/match_attr_traits.ipp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/is_reference.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  match class
    //
    //      The match holds the result of a parser. A match object evaluates
    //      to true when a successful match is found, otherwise false. The
    //      length of the match is the number of characters (or tokens) that
    //      is successfully matched. This can be queried through its length()
    //      member function. A negative value means that the match is
    //      unsuccessful.
    //
    //      Each parser may have an associated attribute. This attribute is
    //      also returned back to the client on a successful parse through
    //      the match object. The match's value() member function returns the
    //      match's attribute.
    //
    //      A match attribute is valid:
    //
    //          * on a successful match
    //          * when its value is set through the value(val) member function
    //          * if it is assigned or copied from a compatible match object
    //            (e.g. match<double> from match<int>) with a valid attribute.
    //
    //      The match attribute is undefined:
    //
    //          * on an unsuccessful match
    //          * when an attempt to copy or assign from another match object
    //            with an incompatible attribute type (e.g. match<std::string>
    //            from match<int>).
    //
    //      The member function has_valid_attribute() can be queried to know if
    //      it is safe to get the match's attribute. The attribute may be set
    //      through the member function value(v) where v is the new attribute
    //      value.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename T = nil_t>
    class match : public safe_bool<match<T> >
    {
        typedef typename
            conditional<
                is_reference<T>::value
              , T
              , typename add_reference<
                    typename add_const<T>::type
                >::type
            >::type attr_ref_t;

    public:

        typedef typename boost::optional<T> optional_type;
        typedef attr_ref_t ctor_param_t;
        typedef attr_ref_t return_t;
        typedef T attr_t;

                                match();
        explicit                match(std::size_t length);
                                match(std::size_t length, ctor_param_t val);

        bool                    operator!() const;
        std::ptrdiff_t          length() const;
        bool                    has_valid_attribute() const;
        return_t                value() const;
        void                    swap(match& other);

        template <typename T2>
        match(match<T2> const& other)
        : len(other.length()), val()
        {
            impl::match_attr_traits<T>::copy(val, other);
        }

        template <typename T2>
        match&
        operator=(match<T2> const& other)
        {
            impl::match_attr_traits<T>::assign(val, other);
            len = other.length();
            return *this;
        }

        template <typename MatchT>
        void
        concat(MatchT const& other)
        {
            BOOST_SPIRIT_ASSERT(*this && other);
            len += other.length();
        }

        template <typename ValueT>
        void
        value(ValueT const& val_)
        {
            impl::match_attr_traits<T>::set_value(val, val_, is_reference<T>());
        }

        bool operator_bool() const
        {
            return len >= 0;
        }

    private:

        std::ptrdiff_t len;
        optional_type val;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  match class specialization for nil_t values
    //
    ///////////////////////////////////////////////////////////////////////////
    template <>
    class match<nil_t> : public safe_bool<match<nil_t> >
    {
    public:

        typedef nil_t attr_t;
        typedef nil_t return_t;

                                match();
        explicit                match(std::size_t length);
                                match(std::size_t length, nil_t);

        bool                    operator!() const;
        bool                    has_valid_attribute() const;
        std::ptrdiff_t          length() const;
        nil_t                   value() const;
        void                    value(nil_t);
        void                    swap(match& other);

        template <typename T>
        match(match<T> const& other)
        : len(other.length()) {}

        template <typename T>
        match<>&
        operator=(match<T> const& other)
        {
            len = other.length();
            return *this;
        }

        template <typename T>
        void
        concat(match<T> const& other)
        {
            BOOST_SPIRIT_ASSERT(*this && other);
            len += other.length();
        }

        bool operator_bool() const
        {
            return len >= 0;
        }

    private:

        std::ptrdiff_t len;
    };

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
#include <boost/spirit/home/classic/core/impl/match.ipp>

