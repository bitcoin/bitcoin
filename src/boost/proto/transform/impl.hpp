///////////////////////////////////////////////////////////////////////////////
/// \file impl.hpp
/// Contains definition of transform<> and transform_impl<> helpers.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TRANSFORM_IMPL_HPP_EAN_04_03_2008
#define BOOST_PROTO_TRANSFORM_IMPL_HPP_EAN_04_03_2008

#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/detail/any.hpp>
#include <boost/proto/detail/static_const.hpp>

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable : 4714) // function 'xxx' marked as __forceinline not inlined
#endif

namespace boost { namespace proto
{
    namespace envns_
    {
        ////////////////////////////////////////////////////////////////////////////////////////////
        struct key_not_found
        {};

        ////////////////////////////////////////////////////////////////////////////////////////////
        // empty_env
        struct empty_env
        {
            typedef void proto_environment_;

            template<typename OtherTag, typename OtherValue = key_not_found>
            struct lookup
            {
                typedef OtherValue type;
                typedef
                    typename add_reference<typename add_const<OtherValue>::type>::type
                const_reference;
            };

            key_not_found operator[](detail::any) const
            {
                return key_not_found();
            }

            template<typename T>
            T const &at(detail::any, T const &t) const
            {
                return t;
            }
        };
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // is_env
    template<typename T, typename Void>
    struct is_env
      : mpl::false_
    {};

    template<typename T>
    struct is_env<T, typename T::proto_environment_>
      : mpl::true_
    {};

    template<typename T>
    struct is_env<T &, void>
      : is_env<T>
    {};

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_TRANSFORM_(PrimitiveTransform, X)                                                       \
    BOOST_PROTO_CALLABLE()                                                                                      \
    typedef X proto_is_transform_;                                                                              \
    typedef PrimitiveTransform transform_type;                                                                  \
                                                                                                                \
    template<typename Sig>                                                                                      \
    struct result                                                                                               \
    {                                                                                                           \
        typedef typename boost::proto::detail::apply_transform<Sig>::result_type type;                          \
    };                                                                                                          \
                                                                                                                \
    template<typename Expr>                                                                                     \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr &)>::result_type                         \
    operator ()(Expr &e) const                                                                                  \
    {                                                                                                           \
        boost::proto::empty_state s = 0;                                                                        \
        boost::proto::empty_env d;                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr &)>()(e, s, d);                        \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr>                                                                                     \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr const &)>::result_type                   \
    operator ()(Expr const &e) const                                                                            \
    {                                                                                                           \
        boost::proto::empty_state s = 0;                                                                        \
        boost::proto::empty_env d;                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr const &)>()(e, s, d);                  \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State>                                                                     \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr &, State &)>::result_type                \
    operator ()(Expr &e, State &s) const                                                                        \
    {                                                                                                           \
        boost::proto::empty_env d;                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr &, State &)>()(e, s, d);               \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State>                                                                     \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr const &, State &)>::result_type          \
    operator ()(Expr const &e, State &s) const                                                                  \
    {                                                                                                           \
        boost::proto::empty_env d;                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr const &, State &)>()(e, s, d);         \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State>                                                                     \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr &, State const &)>::result_type          \
    operator ()(Expr &e, State const &s) const                                                                  \
    {                                                                                                           \
        boost::proto::empty_env d;                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr &, State const &)>()(e, s, d);         \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State>                                                                     \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr const &, State const &)>::result_type    \
    operator ()(Expr const &e, State const &s) const                                                            \
    {                                                                                                           \
        boost::proto::empty_env d;                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr const &, State const &)>()(e, s, d);   \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State, typename Data>                                                      \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr &, State &, Data &)>::result_type        \
    operator ()(Expr &e, State &s, Data &d) const                                                               \
    {                                                                                                           \
        return boost::proto::detail::apply_transform<transform_type(Expr &, State &, Data &)>()(e, s, d);       \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State, typename Data>                                                      \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr const &, State &, Data &)>::result_type  \
    operator ()(Expr const &e, State &s, Data &d) const                                                         \
    {                                                                                                           \
        return boost::proto::detail::apply_transform<transform_type(Expr const &, State &, Data &)>()(e, s, d); \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State, typename Data>                                                      \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr &, State const &, Data &)>::result_type  \
    operator ()(Expr &e, State const &s, Data &d) const                                                         \
    {                                                                                                           \
        return boost::proto::detail::apply_transform<transform_type(Expr &, State const &, Data &)>()(e, s, d); \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State, typename Data>                                                      \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr const &, State const &, Data &)>::result_type  \
    operator ()(Expr const &e, State const &s, Data &d) const                                                   \
    {                                                                                                           \
        return boost::proto::detail::apply_transform<transform_type(Expr const &, State const &, Data &)>()(e, s, d); \
    }                                                                                                           \
    /**/

#else

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_TRANSFORM_(PrimitiveTransform, X)                                                       \
    BOOST_PROTO_CALLABLE()                                                                                      \
    typedef X proto_is_transform_;                                                                              \
    typedef PrimitiveTransform transform_type;                                                                  \
                                                                                                                \
    template<typename Sig>                                                                                      \
    struct result                                                                                               \
    {                                                                                                           \
        typedef typename boost::proto::detail::apply_transform<Sig>::result_type type;                          \
    };                                                                                                          \
                                                                                                                \
    template<typename Expr>                                                                                     \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr const &)>::result_type                   \
    operator ()(Expr &&e) const                                                                                 \
    {                                                                                                           \
        boost::proto::empty_state s = 0;                                                                        \
        boost::proto::empty_env d;                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr const &)>()(e, s, d);                  \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State>                                                                     \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr const &, State const &)>::result_type    \
    operator ()(Expr &&e, State &&s) const                                                                      \
    {                                                                                                           \
        boost::proto::empty_env d;                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr const &, State const &)>()(e, s, d);   \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State, typename Data>                                                      \
    BOOST_FORCEINLINE                                                                                           \
    typename boost::proto::detail::apply_transform<transform_type(Expr const &, State const &, Data const &)>::result_type \
    operator ()(Expr &&e, State &&s, Data &&d) const                                                            \
    {                                                                                                           \
        return boost::proto::detail::apply_transform<transform_type(Expr const &, State const &, Data const &)>()(e, s, d); \
    }                                                                                                           \
    /**/

#endif

    #define BOOST_PROTO_TRANSFORM(PrimitiveTransform)                                                           \
        BOOST_PROTO_TRANSFORM_(PrimitiveTransform, void)                                                        \
        /**/

    namespace detail
    {
        template<typename Sig>
        struct apply_transform;

        template<typename PrimitiveTransform, typename Expr>
        struct apply_transform<PrimitiveTransform(Expr)>
          : PrimitiveTransform::template impl<Expr, empty_state, empty_env>
        {};

        template<typename PrimitiveTransform, typename Expr, typename State>
        struct apply_transform<PrimitiveTransform(Expr, State)>
          : PrimitiveTransform::template impl<Expr, State, empty_env>
        {};

        template<typename PrimitiveTransform, typename Expr, typename State, typename Data>
        struct apply_transform<PrimitiveTransform(Expr, State, Data)>
          : PrimitiveTransform::template impl<Expr, State, Data>
        {};
    }

    template<typename PrimitiveTransform, typename X>
    struct transform
    {
        BOOST_PROTO_TRANSFORM_(PrimitiveTransform, X)
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl
    {
        typedef Expr const expr;
        typedef Expr const &expr_param;
        typedef State const state;
        typedef State const &state_param;
        typedef Data const data;
        typedef Data const &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr &, State, Data>
    {
        typedef Expr expr;
        typedef Expr &expr_param;
        typedef State const state;
        typedef State const &state_param;
        typedef Data const data;
        typedef Data const &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr, State &, Data>
    {
        typedef Expr const expr;
        typedef Expr const &expr_param;
        typedef State state;
        typedef State &state_param;
        typedef Data const data;
        typedef Data const &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr, State, Data &>
    {
        typedef Expr const expr;
        typedef Expr const &expr_param;
        typedef State const state;
        typedef State const &state_param;
        typedef Data data;
        typedef Data &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr &, State &, Data>
    {
        typedef Expr expr;
        typedef Expr &expr_param;
        typedef State state;
        typedef State &state_param;
        typedef Data const data;
        typedef Data const &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr &, State, Data &>
    {
        typedef Expr expr;
        typedef Expr &expr_param;
        typedef State const state;
        typedef State const &state_param;
        typedef Data data;
        typedef Data &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr, State &, Data &>
    {
        typedef Expr const expr;
        typedef Expr const &expr_param;
        typedef State state;
        typedef State &state_param;
        typedef Data data;
        typedef Data &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr &, State &, Data &>
    {
        typedef Expr expr;
        typedef Expr &expr_param;
        typedef State state;
        typedef State &state_param;
        typedef Data data;
        typedef Data &data_param;
    };

}} // namespace boost::proto

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif
