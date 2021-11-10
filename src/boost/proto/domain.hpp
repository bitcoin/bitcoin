///////////////////////////////////////////////////////////////////////////////
/// \file domain.hpp
/// Contains definition of domain\<\> class template and helpers for
/// defining domains with a generator and a grammar for controlling
/// operator overloading.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_DOMAIN_HPP_EAN_02_13_2007
#define BOOST_PROTO_DOMAIN_HPP_EAN_02_13_2007

#include <boost/ref.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/generate.hpp>
#include <boost/proto/detail/as_expr.hpp>
#include <boost/proto/detail/deduce_domain.hpp>

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable : 4714) // function 'xxx' marked as __forceinline not inlined
#endif

namespace boost { namespace proto
{

    namespace detail
    {
        struct not_a_generator
        {};

        struct not_a_grammar
        {};

        struct not_a_domain
        {};
    }

    namespace domainns_
    {
        /// \brief For use in defining domain tags to be used
        /// with \c proto::extends\<\>. A \e Domain associates
        /// an expression type with a \e Generator, and optionally
        /// a \e Grammar.
        ///
        /// The Generator determines how new expressions in the
        /// domain are constructed. Typically, a generator wraps
        /// all new expressions in a wrapper that imparts
        /// domain-specific behaviors to expressions within its
        /// domain. (See \c proto::extends\<\>.)
        ///
        /// The Grammar determines whether a given expression is
        /// valid within the domain, and automatically disables
        /// any operator overloads which would cause an invalid
        /// expression to be created. By default, the Grammar
        /// parameter defaults to the wildcard, \c proto::_, which
        /// makes all expressions valid within the domain.
        ///
        /// The Super declares the domain currently being defined
        /// to be a sub-domain of Super. Expressions in sub-domains
        /// can be freely combined with expressions in its super-
        /// domain (and <I>its</I> super-domain, etc.).
        ///
        /// Example:
        /// \code
        /// template<typename Expr>
        /// struct MyExpr;
        ///
        /// struct MyGrammar
        ///   : or_< terminal<_>, plus<MyGrammar, MyGrammar> >
        /// {};
        ///
        /// // Define MyDomain, in which all expressions are
        /// // wrapped in MyExpr<> and only expressions that
        /// // conform to MyGrammar are allowed.
        /// struct MyDomain
        ///   : domain<generator<MyExpr>, MyGrammar>
        /// {};
        ///
        /// // Use MyDomain to define MyExpr
        /// template<typename Expr>
        /// struct MyExpr
        ///   : extends<Expr, MyExpr<Expr>, MyDomain>
        /// {
        ///     // ...
        /// };
        /// \endcode
        ///
        template<
            typename Generator // = default_generator
          , typename Grammar   // = proto::_
          , typename Super     // = no_super_domain
        >
        struct domain
          : Generator
        {
            typedef Generator proto_generator;
            typedef Grammar   proto_grammar;
            typedef Super     proto_super_domain;
            typedef domain    proto_base_domain;

            /// INTERNAL ONLY
            typedef void proto_is_domain_;

            /// \brief A unary MonomorphicFunctionObject that turns objects into Proto
            /// expression objects in this domain.
            ///
            /// The <tt>as_expr\<\></tt> function object turns objects into Proto expressions, if
            /// they are not already, by making them Proto terminals held by value if
            /// possible. Objects that are already Proto expressions are left alone.
            ///
            /// If <tt>wants_basic_expr\<Generator\>::value</tt> is true, then let \c E be \c basic_expr;
            /// otherwise, let \t E be \c expr. Given an lvalue \c t of type \c T:
            ///
            /// If \c T is not a Proto expression type the resulting terminal is
            /// calculated as follows:
            ///
            ///   If \c T is a function type, an abstract type, or a type derived from
            ///   \c std::ios_base, let \c A be <tt>T &</tt>.
            ///   Otherwise, let \c A be the type \c T stripped of cv-qualifiers.
            ///   Then, the result of applying <tt>as_expr\<T\>()(t)</tt> is
            ///   <tt>Generator()(E\<tag::terminal, term\<A\> \>::make(t))</tt>.
            ///
            /// If \c T is a Proto expression type and its generator type is different from
            /// \c Generator, the result is <tt>Generator()(t)</tt>.
            ///
            /// Otherwise, the result is \c t converted to an (un-const) rvalue.
            ///
            template<typename T, typename IsExpr = void, typename Callable = proto::callable>
            struct as_expr
              : detail::as_expr<
                    T
                  , typename detail::base_generator<Generator>::type
                  , wants_basic_expr<Generator>::value
                >
            {
                BOOST_PROTO_CALLABLE()
            };

            /// INTERNAL ONLY
            ///
            template<typename T>
            struct as_expr<T, typename T::proto_is_expr_, proto::callable>
            {
                BOOST_PROTO_CALLABLE()
                typedef typename remove_const<T>::type result_type;

                BOOST_FORCEINLINE
                result_type operator()(T &e) const
                {
                    return e;
                }
            };

            /// \brief A unary MonomorphicFunctionObject that turns objects into Proto
            /// expression objects in this domain.
            ///
            /// The <tt>as_child\<\></tt> function object turns objects into Proto expressions, if
            /// they are not already, by making them Proto terminals held by reference.
            /// Objects that are already Proto expressions are simply returned by reference.
            ///
            /// If <tt>wants_basic_expr\<Generator\>::value</tt> is true, then let \c E be \c basic_expr;
            /// otherwise, let \t E be \c expr. Given an lvalue \c t of type \c T:
            ///
            /// If \c T is not a Proto expression type the resulting terminal is
            /// <tt>Generator()(E\<tag::terminal, term\<T &\> \>::make(t))</tt>.
            ///
            /// If \c T is a Proto expression type and its generator type is different from
            /// \c Generator, the result is <tt>Generator()(t)</tt>.
            ///
            /// Otherwise, the result is the lvalue \c t.
            ///
            template<typename T, typename IsExpr = void, typename Callable = proto::callable>
            struct as_child
              : detail::as_child<
                    T
                  , typename detail::base_generator<Generator>::type
                  , wants_basic_expr<Generator>::value
                >
            {
                BOOST_PROTO_CALLABLE()
            };

            /// INTERNAL ONLY
            ///
            template<typename T>
            struct as_child<T, typename T::proto_is_expr_, proto::callable>
            {
                BOOST_PROTO_CALLABLE()
                typedef T &result_type;

                BOOST_FORCEINLINE
                result_type operator()(T &e) const
                {
                    return e;
                }
            };
        };

        /// \brief The domain expressions have by default, if
        /// \c proto::extends\<\> has not been used to associate
        /// a domain with an expression.
        ///
        struct default_domain
          : domain<>
        {};

        /// \brief A domain to use when you prefer the use of
        /// \c proto::basic_expr\<\> over \c proto::expr\<\>.
        ///
        struct basic_default_domain
          : domain<basic_default_generator>
        {};

        /// \brief A pseudo-domain for use in functions and
        /// metafunctions that require a domain parameter. It
        /// indicates that the domain of the parent node should
        /// be inferred from the domains of the child nodes.
        ///
        /// \attention \c deduce_domain is not itself a valid domain.
        ///
        struct deduce_domain
          : domain<detail::not_a_generator, detail::not_a_grammar, detail::not_a_domain>
        {};

        /// \brief Given a domain, a tag type and an argument list,
        /// compute the type of the expression to generate. This is
        /// either an instance of \c proto::expr\<\> or
        /// \c proto::basic_expr\<\>.
        ///
        template<typename Domain, typename Tag, typename Args, bool WantsBasicExpr>
        struct base_expr
        {
            typedef proto::expr<Tag, Args, Args::arity> type;
        };

        /// INTERNAL ONLY
        ///
        template<typename Domain, typename Tag, typename Args>
        struct base_expr<Domain, Tag, Args, true>
        {
            typedef proto::basic_expr<Tag, Args, Args::arity> type;
        };

    }

    /// A metafunction that returns \c mpl::true_
    /// if the type \c T is the type of a Proto domain;
    /// \c mpl::false_ otherwise. If \c T inherits from
    /// \c proto::domain\<\>, \c is_domain\<T\> is
    /// \c mpl::true_.
    template<typename T, typename Void  /* = void*/>
    struct is_domain
      : mpl::false_
    {};

    /// INTERNAL ONLY
    ///
    template<typename T>
    struct is_domain<T, typename T::proto_is_domain_>
      : mpl::true_
    {};

    /// A metafunction that returns the domain of
    /// a given type. If \c T is a Proto expression
    /// type, it returns that expression's associated
    /// domain. If not, it returns
    /// \c proto::default_domain.
    template<typename T, typename Void /* = void*/>
    struct domain_of
    {
        typedef default_domain type;
    };

    /// INTERNAL ONLY
    ///
    template<typename T>
    struct domain_of<T, typename T::proto_is_expr_>
    {
        typedef typename T::proto_domain type;
    };

    /// INTERNAL ONLY
    ///
    template<typename T>
    struct domain_of<T &, void>
    {
        typedef typename domain_of<T>::type type;
    };

    /// INTERNAL ONLY
    ///
    template<typename T>
    struct domain_of<boost::reference_wrapper<T>, void>
    {
        typedef typename domain_of<T>::type type;
    };

    /// INTERNAL ONLY
    ///
    template<typename T>
    struct domain_of<boost::reference_wrapper<T> const, void>
    {
        typedef typename domain_of<T>::type type;
    };

    /// A metafunction that returns \c mpl::true_
    /// if the type \c SubDomain is a sub-domain of
    /// \c SuperDomain; \c mpl::false_ otherwise.
    template<typename SubDomain, typename SuperDomain>
    struct is_sub_domain_of
      : is_sub_domain_of<typename SubDomain::proto_super_domain, SuperDomain>
    {};

    /// INTERNAL ONLY
    ///
    template<typename SuperDomain>
    struct is_sub_domain_of<proto::no_super_domain, SuperDomain>
      : mpl::false_
    {};

    /// INTERNAL ONLY
    ///
    template<typename SuperDomain>
    struct is_sub_domain_of<SuperDomain, SuperDomain>
      : mpl::true_
    {};

}}

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif
