/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2013 Agustin Berge

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_PARSER_OCTOBER_16_2008_0254PM)
#define BOOST_SPIRIT_X3_PARSER_OCTOBER_16_2008_0254PM

#include <boost/mpl/bool.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/declval.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/support/traits/has_attribute.hpp>
#include <boost/spirit/home/x3/support/utility/sfinae.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/assert.hpp>
#include <string>

#if !defined(BOOST_SPIRIT_X3_NO_RTTI)
#include <typeinfo>
#endif

namespace boost { namespace spirit { namespace x3
{
    template <typename Subject, typename Action>
    struct action;

    template <typename Subject, typename Handler>
    struct guard;

    struct parser_base {};
    struct parser_id;

    template <typename Derived>
    struct parser : parser_base
    {
        typedef Derived derived_type;
        static bool const handles_container = false;
        static bool const is_pass_through_unary = false;
        static bool const has_action = false;

        constexpr Derived const& derived() const
        {
            return *static_cast<Derived const*>(this);
        }

        template <typename Action>
        constexpr action<Derived, Action> operator[](Action f) const
        {
            return { this->derived(), f };
        }

        template <typename Handler>
        constexpr guard<Derived, Handler> on_error(Handler f) const
        {
            return { this->derived(), f };
        }
    };

    struct unary_category;
    struct binary_category;

    template <typename Subject, typename Derived>
    struct unary_parser : parser<Derived>
    {
        typedef unary_category category;
        typedef Subject subject_type;
        static bool const has_action = Subject::has_action;

        constexpr unary_parser(Subject const& subject)
            : subject(subject) {}

        unary_parser const& get_unary() const { return *this; }

        Subject subject;
    };

    template <typename Left, typename Right, typename Derived>
    struct binary_parser : parser<Derived>
    {
        typedef binary_category category;
        typedef Left left_type;
        typedef Right right_type;
        static bool const has_action =
            left_type::has_action || right_type::has_action;

        constexpr binary_parser(Left const& left, Right const& right)
            : left(left), right(right) {}

        binary_parser const& get_binary() const { return *this; }

        Left left;
        Right right;
    };

    ///////////////////////////////////////////////////////////////////////////
    // as_parser: convert a type, T, into a parser.
    ///////////////////////////////////////////////////////////////////////////
    namespace extension
    {
        namespace detail
        {
            namespace as_parser_guard
            {
                void as_spirit_parser(...);

                template<typename T, typename R =
                    decltype(as_spirit_parser(boost::declval<T const&>()))>
                struct deduce_as_parser
                {
                    typedef R type;
                    typedef typename
                        boost::remove_cv<
                            typename boost::remove_reference<R>::type
                        >::type
                    value_type;

                    static type call(T const& v)
                    {
                        return as_spirit_parser(v);
                    }
                };
                template<typename T>
                struct deduce_as_parser<T, void>
                {};
            }
            using as_parser_guard::deduce_as_parser;
        }

        template <typename T, typename Enable = void>
        struct as_parser : detail::deduce_as_parser<T> {};

        template <>
        struct as_parser<unused_type>
        {
            typedef unused_type type;
            typedef unused_type value_type;
            static constexpr type call(unused_type)
            {
                return unused;
            }
        };

        template <typename Derived>
        struct as_parser<Derived
            , typename enable_if<is_base_of<parser_base, Derived>>::type>
        {
            typedef Derived const& type;
            typedef Derived value_type;
            static constexpr type call(Derived const& p)
            {
                return p;
            }
        };

        template <typename Derived>
        struct as_parser<parser<Derived>>
        {
            typedef Derived const& type;
            typedef Derived value_type;
            static constexpr type call(parser<Derived> const& p)
            {
                return p.derived();
            }
        };
    }

    template <typename T>
    constexpr typename extension::as_parser<T>::type
    as_parser(T const& x)
    {
        return extension::as_parser<T>::call(x);
    }

    template <typename Derived>
    constexpr Derived const&
    as_parser(parser<Derived> const& p)
    {
        return p.derived();
    }

    ///////////////////////////////////////////////////////////////////////////
    // The main what function
    //
    // Note: unlike Spirit2, spirit parsers are no longer required to have a
    // "what" member function. In X3, we specialize the get_info struct
    // below where needed. If a specialization is not provided, the default
    // below will be used. The default "what" result will be the typeid
    // name of the parser if BOOST_SPIRIT_X3_NO_RTTI is not defined, otherwise
    // "undefined"
    ///////////////////////////////////////////////////////////////////////////
    template <typename Parser, typename Enable = void>
    struct get_info
    {
        typedef std::string result_type;
        std::string operator()(Parser const&) const
        {
#if !defined(BOOST_SPIRIT_X3_NO_RTTI)
            return typeid(Parser).name();
#else
            return "undefined";
#endif
        }
    };

    template <typename Parser>
    std::string what(Parser const& p)
    {
        return get_info<Parser>()(p);
    }
}}}

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Subject, typename Derived, typename Context>
    struct has_attribute<x3::unary_parser<Subject, Derived>, Context>
        : has_attribute<Subject, Context> {};

    template <typename Left, typename Right, typename Derived, typename Context>
    struct has_attribute<x3::binary_parser<Left, Right, Derived>, Context>
        : mpl::bool_<has_attribute<Left, Context>::value ||
                has_attribute<Right, Context>::value> {};
}}}}

#endif
