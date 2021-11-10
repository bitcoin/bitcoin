///////////////////////////////////////////////////////////////////////////////
/// \file generate.hpp
/// Contains definition of generate\<\> class template, which end users can
/// specialize for generating domain-specific expression wrappers.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_GENERATE_HPP_EAN_02_13_2007
#define BOOST_PROTO_GENERATE_HPP_EAN_02_13_2007

#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/args.hpp>

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable : 4714) // function 'xxx' marked as __forceinline not inlined
#endif

namespace boost { namespace proto
{

    namespace detail
    {
        template<typename Expr>
        struct by_value_generator_;

        template<typename Tag, typename Arg>
        struct by_value_generator_<proto::expr<Tag, term<Arg>, 0> >
        {
            typedef
                proto::expr<
                    Tag
                  , term<typename detail::term_traits<Arg>::value_type>
                  , 0
                >
            type;

            BOOST_FORCEINLINE
            static type const call(proto::expr<Tag, term<Arg>, 0> const &e)
            {
                type that = {e.child0};
                return that;
            }
        };

        template<typename Tag, typename Arg>
        struct by_value_generator_<proto::basic_expr<Tag, term<Arg>, 0> >
        {
            typedef
                proto::basic_expr<
                    Tag
                  , term<typename detail::term_traits<Arg>::value_type>
                  , 0
                >
            type;

            BOOST_FORCEINLINE
            static type const call(proto::basic_expr<Tag, term<Arg>, 0> const &e)
            {
                type that = {e.child0};
                return that;
            }
        };

        // Include the other specializations of by_value_generator_
        #include <boost/proto/detail/generate_by_value.hpp>
    }

    /// \brief Annotate a generator to indicate that it would
    /// prefer to be passed instances of \c proto::basic_expr\<\> rather
    /// than \c proto::expr\<\>. <tt>use_basic_expr\<Generator\></tt> is
    /// itself a generator.
    ///
    template<typename Generator>
    struct use_basic_expr
      : Generator
    {
        BOOST_PROTO_USE_BASIC_EXPR()
    };

    /// \brief A simple generator that passes an expression
    /// through unchanged.
    ///
    /// Generators are intended for use as the first template parameter
    /// to the \c domain\<\> class template and control if and how
    /// expressions within that domain are to be customized.
    /// The \c default_generator makes no modifications to the expressions
    /// passed to it.
    struct default_generator
    {
        BOOST_PROTO_CALLABLE()

        template<typename Sig>
        struct result;

        template<typename This, typename Expr>
        struct result<This(Expr)>
        {
            typedef Expr type;
        };

        /// \param expr A Proto expression
        /// \return expr
        template<typename Expr>
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(Expr, Expr const &)
        operator ()(Expr const &e) const
        {
            return e;
        }
    };

    /// \brief A simple generator that passes an expression
    /// through unchanged and specifies a preference for
    /// \c proto::basic_expr\<\> over \c proto::expr\<\>.
    ///
    /// Generators are intended for use as the first template parameter
    /// to the \c domain\<\> class template and control if and how
    /// expressions within that domain are to be customized.
    /// The \c default_generator makes no modifications to the expressions
    /// passed to it.
    struct basic_default_generator
      : proto::use_basic_expr<default_generator>
    {};
    
    /// \brief A generator that wraps expressions passed
    /// to it in the specified extension wrapper.
    ///
    /// Generators are intended for use as the first template parameter
    /// to the \c domain\<\> class template and control if and how
    /// expressions within that domain are to be customized.
    /// \c generator\<\> wraps each expression passed to it in
    /// the \c Extends\<\> wrapper.
    template<template<typename> class Extends>
    struct generator
    {
        BOOST_PROTO_CALLABLE()
        BOOST_PROTO_USE_BASIC_EXPR()

        template<typename Sig>
        struct result;

        template<typename This, typename Expr>
        struct result<This(Expr)>
        {
            typedef Extends<Expr> type;
        };

        template<typename This, typename Expr>
        struct result<This(Expr &)>
        {
            typedef Extends<Expr> type;
        };

        template<typename This, typename Expr>
        struct result<This(Expr const &)>
        {
            typedef Extends<Expr> type;
        };

        /// \param expr A Proto expression
        /// \return Extends<Expr>(expr)
        template<typename Expr>
        BOOST_FORCEINLINE
        Extends<Expr> operator ()(Expr const &e) const
        {
            return Extends<Expr>(e);
        }
    };

    /// \brief A generator that wraps expressions passed
    /// to it in the specified extension wrapper and uses
    /// aggregate initialization for the wrapper.
    ///
    /// Generators are intended for use as the first template parameter
    /// to the \c domain\<\> class template and control if and how
    /// expressions within that domain are to be customized.
    /// \c pod_generator\<\> wraps each expression passed to it in
    /// the \c Extends\<\> wrapper, and uses aggregate initialzation
    /// for the wrapped object.
    template<template<typename> class Extends>
    struct pod_generator
    {
        BOOST_PROTO_CALLABLE()
        BOOST_PROTO_USE_BASIC_EXPR()

        template<typename Sig>
        struct result;

        template<typename This, typename Expr>
        struct result<This(Expr)>
        {
            typedef Extends<Expr> type;
        };

        template<typename This, typename Expr>
        struct result<This(Expr &)>
        {
            typedef Extends<Expr> type;
        };

        template<typename This, typename Expr>
        struct result<This(Expr const &)>
        {
            typedef Extends<Expr> type;
        };

        /// \param expr The expression to wrap
        /// \return <tt>Extends\<Expr\> that = {expr}; return that;</tt>
        template<typename Expr>
        BOOST_FORCEINLINE
        Extends<Expr> operator ()(Expr const &e) const
        {
            Extends<Expr> that = {e};
            return that;
        }

        // Work-around for:
        // https://connect.microsoft.com/VisualStudio/feedback/details/765449/codegen-stack-corruption-using-runtime-checks-when-aggregate-initializing-struct
    #if BOOST_WORKAROUND(BOOST_MSVC, < 1800)
        template<typename Class, typename Member>
        BOOST_FORCEINLINE
        Extends<expr<tag::terminal, proto::term<Member Class::*> > > operator ()(expr<tag::terminal, proto::term<Member Class::*> > const &e) const
        {
            Extends<expr<tag::terminal, proto::term<Member Class::*> > > that;
            proto::value(that.proto_expr_) = proto::value(e);
            return that;
        }

        template<typename Class, typename Member>
        BOOST_FORCEINLINE
        Extends<basic_expr<tag::terminal, proto::term<Member Class::*> > > operator ()(basic_expr<tag::terminal, proto::term<Member Class::*> > const &e) const
        {
            Extends<basic_expr<tag::terminal, proto::term<Member Class::*> > > that;
            proto::value(that.proto_expr_) = proto::value(e);
            return that;
        }
    #endif
    };

    /// \brief A generator that replaces child nodes held by
    /// reference with ones held by value. Use with
    /// \c compose_generators to forward that result to another
    /// generator.
    ///
    /// Generators are intended for use as the first template parameter
    /// to the \c domain\<\> class template and control if and how
    /// expressions within that domain are to be customized.
    /// \c by_value_generator ensures all child nodes are
    /// held by value. This generator is typically composed with a
    /// second generator for further processing, as
    /// <tt>compose_generators\<by_value_generator, MyGenerator\></tt>.
    struct by_value_generator
    {
        BOOST_PROTO_CALLABLE()

        template<typename Sig>
        struct result;

        template<typename This, typename Expr>
        struct result<This(Expr)>
        {
            typedef
                typename detail::by_value_generator_<Expr>::type
            type;
        };

        template<typename This, typename Expr>
        struct result<This(Expr &)>
        {
            typedef
                typename detail::by_value_generator_<Expr>::type
            type;
        };

        template<typename This, typename Expr>
        struct result<This(Expr const &)>
        {
            typedef
                typename detail::by_value_generator_<Expr>::type
            type;
        };

        /// \param expr The expression to modify.
        /// \return <tt>deep_copy(expr)</tt>
        template<typename Expr>
        BOOST_FORCEINLINE
        typename result<by_value_generator(Expr)>::type operator ()(Expr const &e) const
        {
            return detail::by_value_generator_<Expr>::call(e);
        }
    };

    /// \brief A composite generator that first applies one
    /// transform to an expression and then forwards the result
    /// on to another generator for further transformation.
    ///
    /// Generators are intended for use as the first template parameter
    /// to the \c domain\<\> class template and control if and how
    /// expressions within that domain are to be customized.
    /// \c compose_generators\<\> is a composite generator that first
    /// applies one transform to an expression and then forwards the
    /// result on to another generator for further transformation.
    template<typename First, typename Second>
    struct compose_generators
    {
        BOOST_PROTO_CALLABLE()

        template<typename Sig>
        struct result;

        template<typename This, typename Expr>
        struct result<This(Expr)>
        {
            typedef
                typename Second::template result<
                    Second(typename First::template result<First(Expr)>::type)
                >::type
            type;
        };

        template<typename This, typename Expr>
        struct result<This(Expr &)>
        {
            typedef
                typename Second::template result<
                    Second(typename First::template result<First(Expr)>::type)
                >::type
            type;
        };

        template<typename This, typename Expr>
        struct result<This(Expr const &)>
        {
            typedef
                typename Second::template result<
                    Second(typename First::template result<First(Expr)>::type)
                >::type
            type;
        };

        /// \param expr The expression to modify.
        /// \return Second()(First()(expr))
        template<typename Expr>
        BOOST_FORCEINLINE
        typename result<compose_generators(Expr)>::type operator ()(Expr const &e) const
        {
            return Second()(First()(e));
        }
    };

    /// \brief Tests a generator to see whether it would prefer
    /// to be passed instances of \c proto::basic_expr\<\> rather than
    /// \c proto::expr\<\>.
    ///
    template<typename Generator, typename Void>
    struct wants_basic_expr
      : mpl::false_
    {};

    template<typename Generator>
    struct wants_basic_expr<Generator, typename Generator::proto_use_basic_expr_>
      : mpl::true_
    {};

    /// INTERNAL ONLY
    template<>
    struct is_callable<default_generator>
      : mpl::true_
    {};

    /// INTERNAL ONLY
    template<template<typename> class Extends>
    struct is_callable<generator<Extends> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    template<template<typename> class Extends>
    struct is_callable<pod_generator<Extends> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    template<>
    struct is_callable<by_value_generator>
      : mpl::true_
    {};

    /// INTERNAL ONLY
    template<typename First, typename Second>
    struct is_callable<compose_generators<First, Second> >
      : mpl::true_
    {};

}}

// Specializations of boost::result_of and boost::tr1_result_of to eliminate
// some unnecessary template instantiations
namespace boost
{
    template<typename Expr>
    struct result_of<proto::default_domain(Expr)>
    {
        typedef Expr type;
    };

    template<typename Expr>
    struct result_of<proto::basic_default_domain(Expr)>
    {
        typedef Expr type;
    };

    template<typename Expr>
    struct result_of<proto::default_generator(Expr)>
    {
        typedef Expr type;
    };

    template<typename Expr>
    struct result_of<proto::basic_default_generator(Expr)>
    {
        typedef Expr type;
    };

    #if BOOST_VERSION >= 104400
    template<typename Expr>
    struct tr1_result_of<proto::default_domain(Expr)>
    {
        typedef Expr type;
    };

    template<typename Expr>
    struct tr1_result_of<proto::basic_default_domain(Expr)>
    {
        typedef Expr type;
    };

    template<typename Expr>
    struct tr1_result_of<proto::default_generator(Expr)>
    {
        typedef Expr type;
    };

    template<typename Expr>
    struct tr1_result_of<proto::basic_default_generator(Expr)>
    {
        typedef Expr type;
    };
    #endif
}

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif // BOOST_PROTO_GENERATE_HPP_EAN_02_13_2007
