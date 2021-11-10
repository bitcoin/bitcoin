///////////////////////////////////////////////////////////////////////////////
/// \file make_expr.hpp
/// Definition of the \c make_expr() and \c unpack_expr() utilities for
/// building Proto expression nodes from child nodes or from a Fusion
/// sequence of child nodes, respectively.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_MAKE_EXPR_HPP_EAN_04_01_2005
#define BOOST_PROTO_MAKE_EXPR_HPP_EAN_04_01_2005

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/ref.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/domain.hpp>
#include <boost/proto/generate.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/begin.hpp>
#include <boost/fusion/include/next.hpp>
#include <boost/fusion/include/value_of.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/proto/detail/poly_function.hpp>
#include <boost/proto/detail/deprecated.hpp>

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable : 4180) // qualifier applied to function type has no meaning; ignored
# pragma warning(disable : 4714) // function 'xxx' marked as __forceinline not inlined
#endif

namespace boost { namespace proto
{
/// INTERNAL ONLY
///
#define BOOST_PROTO_AS_CHILD_TYPE(Z, N, DATA)                                                   \
    typename boost::proto::detail::protoify<                                                    \
        BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 0, DATA), N)                                        \
      , BOOST_PP_TUPLE_ELEM(3, 2, DATA)                                                         \
    >::result_type                                                                              \
    /**/

/// INTERNAL ONLY
///
#define BOOST_PROTO_AS_CHILD(Z, N, DATA)                                                        \
    boost::proto::detail::protoify<                                                             \
        BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 0, DATA), N)                                        \
      , BOOST_PP_TUPLE_ELEM(3, 2, DATA)                                                         \
    >()(BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 1, DATA), N))                                       \
    /**/

    namespace detail
    {
        template<typename T, typename Domain>
        struct protoify
          : Domain::template as_expr<T>
        {};

        template<typename T, typename Domain>
        struct protoify<T &, Domain>
          : Domain::template as_child<T>
        {};

        template<typename T, typename Domain>
        struct protoify<boost::reference_wrapper<T>, Domain>
          : Domain::template as_child<T>
        {};

        template<typename T, typename Domain>
        struct protoify<boost::reference_wrapper<T> const, Domain>
          : Domain::template as_child<T>
        {};

        // Definition of detail::unpack_expr_
        #include <boost/proto/detail/unpack_expr_.hpp>

        // Definition of detail::make_expr_
        #include <boost/proto/detail/make_expr_.hpp>
    }

    namespace result_of
    {
        /// \brief Metafunction that computes the return type of the
        /// \c make_expr() function, with a domain deduced from the
        /// domains of the children.
        ///
        /// Use the <tt>result_of::make_expr\<\></tt> metafunction to
        /// compute the return type of the \c make_expr() function.
        ///
        /// In this specialization, the domain is deduced from the
        /// domains of the child types. (If
        /// <tt>is_domain\<A0\>::value</tt> is \c true, then another
        /// specialization is selected.)
        template<
            typename Tag
          , BOOST_PP_ENUM_PARAMS(BOOST_PROTO_MAX_ARITY, typename A)
          , typename Void1  // = void
          , typename Void2  // = void
        >
        struct make_expr
        {
            /// Same as <tt>result_of::make_expr\<Tag, D, A0, ... AN\>::type</tt>
            /// where \c D is the deduced domain, which is calculated as follows:
            ///
            /// For each \c x in <tt>[0,N)</tt> (proceeding in order beginning with
            /// <tt>x=0</tt>), if <tt>domain_of\<Ax\>::type</tt> is not
            /// \c default_domain, then \c D is <tt>domain_of\<Ax\>::type</tt>.
            /// Otherwise, \c D is \c default_domain.
            typedef
                typename detail::make_expr_<
                    Tag
                  , deduce_domain
                    BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, A)
                >::result_type
            type;
        };

        /// \brief Metafunction that computes the return type of the
        /// \c make_expr() function, within the specified domain.
        ///
        /// Use the <tt>result_of::make_expr\<\></tt> metafunction to compute
        /// the return type of the \c make_expr() function.
        template<
            typename Tag
          , typename Domain
            BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, typename A)
        >
        struct make_expr<
            Tag
          , Domain
            BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, A)
          , typename Domain::proto_is_domain_
        >
        {
            /// If \c Tag is <tt>tag::terminal</tt>, then \c type is a
            /// typedef for <tt>boost::result_of\<Domain(expr\<tag::terminal,
            /// term\<A0\> \>)\>::type</tt>.
            ///
            /// Otherwise, \c type is a typedef for <tt>boost::result_of\<Domain(expr\<Tag,
            /// listN\< as_child\<A0\>::type, ... as_child\<AN\>::type\>)
            /// \>::type</tt>, where \c N is the number of non-void template
            /// arguments, and <tt>as_child\<A\>::type</tt> is evaluated as
            /// follows:
            ///
            /// \li If <tt>is_expr\<A\>::value</tt> is \c true, then the
            /// child type is \c A.
            /// \li If \c A is <tt>B &</tt> or <tt>cv boost::reference_wrapper\<B\></tt>,
            /// and <tt>is_expr\<B\>::value</tt> is \c true, then the
            /// child type is <tt>B &</tt>.
            /// \li If <tt>is_expr\<A\>::value</tt> is \c false, then the
            /// child type is <tt>boost::result_of\<Domain(expr\<tag::terminal, term\<A\> \>
            /// )\>::type</tt>.
            /// \li If \c A is <tt>B &</tt> or <tt>cv boost::reference_wrapper\<B\></tt>,
            /// and <tt>is_expr\<B\>::value</tt> is \c false, then the
            /// child type is <tt>boost::result_of\<Domain(expr\<tag::terminal, term\<B &\> \>
            /// )\>::type</tt>.
            typedef
                typename detail::make_expr_<
                    Tag
                  , Domain
                    BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, A)
                >::result_type
            type;
        };

        /// \brief Metafunction that computes the return type of the
        /// \c unpack_expr() function, with a domain deduced from the
        /// domains of the children.
        ///
        /// Use the <tt>result_of::unpack_expr\<\></tt> metafunction to
        /// compute the return type of the \c unpack_expr() function.
        ///
        /// \c Sequence is a Fusion Forward Sequence.
        ///
        /// In this specialization, the domain is deduced from the
        /// domains of the child types. (If
        /// <tt>is_domain\<Sequence>::value</tt> is \c true, then another
        /// specialization is selected.)
        template<
            typename Tag
          , typename Sequence
          , typename Void1  // = void
          , typename Void2  // = void
        >
        struct unpack_expr
        {
            /// Let \c S be the type of a Fusion Random Access Sequence
            /// equivalent to \c Sequence. Then \c type is the
            /// same as <tt>result_of::make_expr\<Tag,
            /// fusion::result_of::value_at_c\<S, 0\>::type, ...
            /// fusion::result_of::value_at_c\<S, N-1\>::type\>::type</tt>,
            /// where \c N is the size of \c S.
            typedef
                typename detail::unpack_expr_<
                    Tag
                  , deduce_domain
                  , Sequence
                  , fusion::result_of::size<Sequence>::type::value
                >::type
            type;
        };

        /// \brief Metafunction that computes the return type of the
        /// \c unpack_expr() function, within the specified domain.
        ///
        /// Use the <tt>result_of::make_expr\<\></tt> metafunction to compute
        /// the return type of the \c make_expr() function.
        template<typename Tag, typename Domain, typename Sequence>
        struct unpack_expr<Tag, Domain, Sequence, typename Domain::proto_is_domain_>
        {
            /// Let \c S be the type of a Fusion Random Access Sequence
            /// equivalent to \c Sequence. Then \c type is the
            /// same as <tt>result_of::make_expr\<Tag, Domain,
            /// fusion::result_of::value_at_c\<S, 0\>::type, ...
            /// fusion::result_of::value_at_c\<S, N-1\>::type\>::type</tt>,
            /// where \c N is the size of \c S.
            typedef
                typename detail::unpack_expr_<
                    Tag
                  , Domain
                  , Sequence
                  , fusion::result_of::size<Sequence>::type::value
                >::type
            type;
        };
    }

    namespace functional
    {
        /// \brief A callable function object equivalent to the
        /// \c proto::make_expr() function.
        ///
        /// In all cases, <tt>functional::make_expr\<Tag, Domain\>()(a0, ... aN)</tt>
        /// is equivalent to <tt>proto::make_expr\<Tag, Domain\>(a0, ... aN)</tt>.
        ///
        /// <tt>functional::make_expr\<Tag\>()(a0, ... aN)</tt>
        /// is equivalent to <tt>proto::make_expr\<Tag\>(a0, ... aN)</tt>.
        template<typename Tag, typename Domain  /* = deduce_domain*/>
        struct make_expr
        {
            BOOST_PROTO_CALLABLE()
            BOOST_PROTO_POLY_FUNCTION()

            template<typename Sig>
            struct result;

            template<typename This, typename A0>
            struct result<This(A0)>
            {
                typedef
                    typename result_of::make_expr<
                        Tag
                      , Domain
                      , A0
                    >::type
                type;
            };

            /// Construct an expression node with tag type \c Tag
            /// and in the domain \c Domain.
            ///
            /// \return <tt>proto::make_expr\<Tag, Domain\>(a0,...aN)</tt>
            template<typename A0>
            BOOST_FORCEINLINE
            typename result_of::make_expr<
                Tag
              , Domain
              , A0 const
            >::type const
            operator ()(A0 const &a0) const
            {
                return proto::detail::make_expr_<
                    Tag
                  , Domain
                  , A0 const
                >()(a0);
            }

            // Additional overloads generated by the preprocessor ...
            #include <boost/proto/detail/make_expr_funop.hpp>

            /// INTERNAL ONLY
            ///
            template<
                BOOST_PP_ENUM_BINARY_PARAMS(
                    BOOST_PROTO_MAX_ARITY
                  , typename A
                  , = void BOOST_PP_INTERCEPT
                )
            >
            struct impl
              : detail::make_expr_<
                    Tag
                  , Domain
                    BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, A)
                >
            {};
        };

        /// \brief A callable function object equivalent to the
        /// \c proto::unpack_expr() function.
        ///
        /// In all cases, <tt>functional::unpack_expr\<Tag, Domain\>()(seq)</tt>
        /// is equivalent to <tt>proto::unpack_expr\<Tag, Domain\>(seq)</tt>.
        ///
        /// <tt>functional::unpack_expr\<Tag\>()(seq)</tt>
        /// is equivalent to <tt>proto::unpack_expr\<Tag\>(seq)</tt>.
        template<typename Tag, typename Domain /* = deduce_domain*/>
        struct unpack_expr
        {
            BOOST_PROTO_CALLABLE()

            template<typename Sig>
            struct result;

            template<typename This, typename Sequence>
            struct result<This(Sequence)>
            {
                typedef
                    typename result_of::unpack_expr<
                        Tag
                      , Domain
                      , typename remove_reference<Sequence>::type
                    >::type
                type;
            };

            /// Construct an expression node with tag type \c Tag
            /// and in the domain \c Domain.
            ///
            /// \param sequence A Fusion Forward Sequence
            /// \return <tt>proto::unpack_expr\<Tag, Domain\>(sequence)</tt>
            template<typename Sequence>
            BOOST_FORCEINLINE
            typename result_of::unpack_expr<Tag, Domain, Sequence const>::type const
            operator ()(Sequence const &sequence) const
            {
                return proto::detail::unpack_expr_<
                    Tag
                  , Domain
                  , Sequence const
                  , fusion::result_of::size<Sequence>::type::value
                >::call(sequence);
            }
        };

    } // namespace functional

    /// \brief Construct an expression of the requested tag type
    /// with a domain and with the specified arguments as children.
    ///
    /// This function template may be invoked either with or without
    /// specifying a \c Domain argument. If no domain is specified,
    /// the domain is deduced by examining in order the domains of
    /// the given arguments and taking the first that is not
    /// \c default_domain, if any such domain exists, or
    /// \c default_domain otherwise.
    ///
    /// Let \c wrap_(x) be defined such that:
    /// \li If \c x is a <tt>boost::reference_wrapper\<\></tt>,
    /// \c wrap_(x) is equivalent to <tt>as_child\<Domain\>(x.get())</tt>.
    /// \li Otherwise, \c wrap_(x) is equivalent to
    /// <tt>as_expr\<Domain\>(x)</tt>.
    ///
    /// Let <tt>make_\<Tag\>(b0,...bN)</tt> be defined as
    /// <tt>expr\<Tag, listN\<C0,...CN\> \>::make(c0,...cN)</tt>
    /// where \c Bx is the type of \c bx.
    ///
    /// \return <tt>Domain()(make_\<Tag\>(wrap_(a0),...wrap_(aN)))</tt>.
    template<typename Tag, typename A0>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
          , A0 const
        >
    >::type const
    make_expr(A0 const &a0)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
          , A0 const
        >()(a0);
    }

    /// \overload
    ///
    template<typename Tag, typename Domain, typename C0>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
      , C0 const
    >::type const
    make_expr(C0 const &c0)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
          , C0 const
        >()(c0);
    }

    // Additional overloads generated by the preprocessor...
    #include <boost/proto/detail/make_expr.hpp>

    /// \brief Construct an expression of the requested tag type
    /// with a domain and with childres from the specified Fusion
    /// Forward Sequence.
    ///
    /// This function template may be invoked either with or without
    /// specifying a \c Domain argument. If no domain is specified,
    /// the domain is deduced by examining in order the domains of the
    /// elements of \c sequence and taking the first that is not
    /// \c default_domain, if any such domain exists, or
    /// \c default_domain otherwise.
    ///
    /// Let \c s be a Fusion Random Access Sequence equivalent to \c sequence.
    /// Let <tt>wrap_\<N\>(s)</tt>, where \c s has type \c S, be defined
    /// such that:
    /// \li If <tt>fusion::result_of::value_at_c\<S,N\>::type</tt> is a reference,
    /// <tt>wrap_\<N\>(s)</tt> is equivalent to
    /// <tt>as_child\<Domain\>(fusion::at_c\<N\>(s))</tt>.
    /// \li Otherwise, <tt>wrap_\<N\>(s)</tt> is equivalent to
    /// <tt>as_expr\<Domain\>(fusion::at_c\<N\>(s))</tt>.
    ///
    /// Let <tt>make_\<Tag\>(b0,...bN)</tt> be defined as
    /// <tt>expr\<Tag, listN\<B0,...BN\> \>::make(b0,...bN)</tt>
    /// where \c Bx is the type of \c bx.
    ///
    /// \param sequence a Fusion Forward Sequence.
    /// \return <tt>Domain()(make_\<Tag\>(wrap_\<0\>(s),...wrap_\<N-1\>(s)))</tt>,
    /// where N is the size of \c Sequence.
    template<typename Tag, typename Sequence>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<Sequence>
      , result_of::unpack_expr<Tag, Sequence const>
    >::type const
    unpack_expr(Sequence const &sequence)
    {
        return proto::detail::unpack_expr_<
            Tag
          , deduce_domain
          , Sequence const
          , fusion::result_of::size<Sequence>::type::value
        >::call(sequence);
    }

    /// \overload
    ///
    template<typename Tag, typename Domain, typename Sequence2>
    BOOST_FORCEINLINE
    typename result_of::unpack_expr<Tag, Domain, Sequence2 const>::type const
    unpack_expr(Sequence2 const &sequence2)
    {
        return proto::detail::unpack_expr_<
            Tag
          , Domain
          , Sequence2 const
          , fusion::result_of::size<Sequence2>::type::value
        >::call(sequence2);
    }

    /// INTERNAL ONLY
    ///
    template<typename Tag, typename Domain>
    struct is_callable<functional::make_expr<Tag, Domain> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    ///
    template<typename Tag, typename Domain>
    struct is_callable<functional::unpack_expr<Tag, Domain> >
      : mpl::true_
    {};

}}

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif // BOOST_PROTO_MAKE_EXPR_HPP_EAN_04_01_2005
