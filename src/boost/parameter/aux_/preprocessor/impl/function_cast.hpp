// Copyright Daniel Wallin 2006.
// Copyright Cromwell D. Enage 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_CAST_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_CAST_HPP

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

namespace boost { namespace parameter { namespace aux {

    // Handles possible implicit casts.  Used by preprocessor.hpp
    // to normalize user input.
    //
    // cast<void*>::execute() is identity
    // cast<void*(X)>::execute() is identity
    // cast<void(X)>::execute() casts to X
    //
    // preprocessor.hpp uses this like this:
    //
    //     #define X(value, predicate)
    //         cast<void predicate>::execute(value)
    //
    //     X(something, *)
    //     X(something, *(predicate))
    //     X(something, (int))
    template <typename VoidExpr, typename Args>
    struct cast;
}}} // namespace boost::parameter::aux

#include <boost/parameter/aux_/use_default_tag.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename T, typename B>
    inline ::boost::parameter::aux::use_default_tag
        forward(::boost::parameter::aux::use_default_tag)
    {
        return ::boost::parameter::aux::use_default_tag();
    }
}}} // namespace boost::parameter::aux

#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mp11/integral.hpp>
#include <boost/mp11/utility.hpp>
#endif

namespace boost { namespace parameter { namespace aux {

    template <typename Args>
    struct cast<void*,Args>
    {
        template <typename T, typename B>
        struct apply
        {
            typedef typename ::boost::mpl
            ::if_<B,T,::boost::mpl::true_>::type type;
        };

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        template <typename T, typename B>
        using fn = ::boost::mp11::mp_if<B,T,::boost::mp11::mp_true>;
#endif
    };
}}} // namespace boost::parameter::aux

#include <boost/parameter/aux_/void.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename Predicate, typename Args>
    struct cast<void*(Predicate),Args>
      : ::boost::parameter::aux::cast<void*,Args>
    {
    };
}}} // namespace boost::parameter::aux

#include <boost/mpl/placeholders.hpp>

namespace boost { namespace parameter { namespace aux {

    // This is a hack used in cast<> to turn the user supplied type,
    // which may or may not be a placeholder expression, into one,
    // so that it will be properly evaluated by mpl::apply.
    template <typename T, typename Dummy = ::boost::mpl::_1>
    struct as_placeholder_expr
    {
        typedef T type;
    };
}}} // namespace boost::parameter::aux

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mp11/list.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename Target, typename Source, typename Args>
    struct apply_target_fn
    {
        using type = ::boost::mp11
        ::mp_apply_q<Target,::boost::mp11::mp_list<Source,Args> >;
    };
}}} // namespace boost::parameter::aux

#endif

#include <boost/mpl/apply.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/parameter/aux_/has_nested_template_fn.hpp>
#include <type_traits>
#else
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#endif

namespace boost { namespace parameter { namespace aux {

    template <typename Target, typename Source, typename Args>
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
    using is_target_same_as_source = ::std::is_same<
        typename ::std::remove_const<
            typename ::std::remove_reference<
                typename ::boost::mp11::mp_if<
                    ::boost::parameter::aux::has_nested_template_fn<Target>
                  , ::boost::parameter::aux
                    ::apply_target_fn<Target,Source,Args>
                  , ::boost::mpl::apply2<
                        ::boost::parameter::aux::as_placeholder_expr<Target>
                      , Source
                      , Args
                    >
                >::type
            >::type
        >::type
      , typename ::std::remove_const<Source>::type
    >;
#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11)
    struct is_target_same_as_source
      : ::boost::mpl::if_<
            ::boost::is_same<
                typename ::boost::remove_const<
                    typename ::boost::remove_reference<
                        typename ::boost::mpl::apply2<
                            ::boost::parameter::aux
                            ::as_placeholder_expr<Target>
                          , Source
                          , Args
                        >::type
                    >::type
                >::type
              , typename ::boost::remove_const<Source>::type
            >
          , ::boost::mpl::true_
          , ::boost::mpl::false_
        >::type
    {
    };
#endif  // BOOST_PARAMETER_CAN_USE_MP11
}}} // namespace boost::parameter::aux

#if !defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/is_const.hpp>
#endif

namespace boost { namespace parameter { namespace aux {

    // Covers the case where is_convertible<Source,Target> but not
    // is_same<Source,Target>.  Use cases are covered
    // by test/normalize_argument_types.cpp
    template <typename Source, typename Target>
    class cast_convert
    {
        typedef ::boost::parameter::aux::cast_convert<Source,Target> _self;

     public:
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        using type = typename ::boost::mp11::mp_if<
            ::std::is_const<Source>
          , ::std::add_const<Target>
          , ::std::remove_const<Target>
        >::type;
#else
        typedef typename boost::mpl::eval_if<
            ::boost::is_const<Source>
          , ::boost::add_const<Target>
          , ::boost::remove_const<Target>
        >::type type;
#endif

     private:
        inline static typename _self::type
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
            _copy(typename ::std::remove_const<Target>::type value)
#else
            _copy(typename ::boost::remove_const<Target>::type value)
#endif
        {
            return value;
        }

     public:
        inline static typename _self::type evaluate(Source&& source)
        {
            return _self::_copy(source);
        }
    };

    template <typename Target, typename Source, typename Args>
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
    using cast_impl = ::std::remove_reference<
        typename ::boost::mp11::mp_if<
            ::boost::parameter::aux::has_nested_template_fn<Target>
          , ::boost::parameter::aux
            ::is_target_same_as_source<Target,Source,Args>
          , ::boost::mpl::apply2<
                ::boost::parameter::aux::as_placeholder_expr<Target>
              , Source
              , Args
            >
        >::type
    >;
#else
    struct cast_impl
      : ::boost::remove_reference<
            typename ::boost::mpl::apply2<
                ::boost::parameter::aux::as_placeholder_expr<Target>
              , Source
              , Args
            >::type
        >
    {
    };
#endif  // BOOST_PARAMETER_CAN_USE_MP11
}}} // namespace boost::parameter::aux

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename Target, typename Args>
    struct cast<void(Target),Args>
    {
        template <typename T, typename B>
        struct apply
        {
            typedef typename ::boost::mpl::eval_if<
                B
              , ::boost::mpl::eval_if<
                    ::boost::parameter::aux
                    ::is_target_same_as_source<Target,T,Args>
                  , ::boost::mpl::identity<T>
                  , ::boost::parameter::aux::cast_impl<Target,T,Args>
                >
              , ::boost::parameter::aux
                ::is_target_same_as_source<Target,T,Args>
            >::type type;
        };

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        template <typename T, typename B>
        using fn = typename ::boost::mp11::mp_if<
            B
          , ::boost::mp11::mp_if<
                ::boost::parameter::aux
                ::is_target_same_as_source<Target,T,Args>
              , ::boost::mp11::mp_identity<T>
              , ::boost::parameter::aux::cast_impl<Target,T,Args>
            >
          , ::boost::parameter::aux
            ::is_target_same_as_source<Target,T,Args>
        >::type;
#endif
    };
}}} // namespace boost::parameter::aux

#include <boost/parameter/value_type.hpp>

#if !defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mpl/apply_wrap.hpp>
#endif

// Expands to the target type of the argument as indicated by the predicate.
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#define BOOST_PARAMETER_FUNCTION_CAST_T(tag, predicate, args)                \
    ::boost::mp11::mp_apply_q<                                               \
        ::boost::parameter::aux::cast<void predicate, args>                  \
      , ::boost::mp11::mp_list<                                              \
            typename ::boost::parameter::value_type<                         \
                args                                                         \
              , tag                                                          \
              , ::boost::parameter::aux::use_default_tag                     \
            >::type                                                          \
          , ::boost::mp11::mp_true                                           \
        >                                                                    \
    >
/**/
#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11)
#define BOOST_PARAMETER_FUNCTION_CAST_T(tag, predicate, args)                \
    typename ::boost::mpl::apply_wrap2<                                      \
        ::boost::parameter::aux::cast<void predicate, args>                  \
      , typename ::boost::parameter::value_type<                             \
            args                                                             \
          , tag                                                              \
          , ::boost::parameter::aux::use_default_tag                         \
        >::type                                                              \
      , ::boost::mpl::true_                                                  \
    >::type
/**/
#endif  // BOOST_PARAMETER_CAN_USE_MP11

// Expands to boost::mpl::true_ if and only if the argument's source and
// target types are the same.
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#define BOOST_PARAMETER_FUNCTION_CAST_B(tag, predicate, args)                \
    ::boost::mp11::mp_apply_q<                                               \
        ::boost::parameter::aux::cast<void predicate, args>                  \
      , ::boost::mp11::mp_list<                                              \
            typename ::boost::parameter::value_type<                         \
                args                                                         \
              , tag                                                          \
              , ::boost::parameter::aux::use_default_tag                     \
            >::type                                                          \
          , ::boost::mp11::mp_false                                          \
        >                                                                    \
    >
/**/
#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11)
#define BOOST_PARAMETER_FUNCTION_CAST_B(tag, predicate, args)                \
    typename ::boost::mpl::apply_wrap2<                                      \
        ::boost::parameter::aux::cast<void predicate, args>                  \
      , typename ::boost::parameter::value_type<                             \
            args                                                             \
          , tag                                                              \
          , ::boost::parameter::aux::use_default_tag                         \
        >::type                                                              \
      , ::boost::mpl::false_                                                 \
    >::type
/**/
#endif  // BOOST_PARAMETER_CAN_USE_MP11

#include <boost/core/enable_if.hpp>
#include <utility>

namespace boost { namespace parameter { namespace aux {

    // If the source and target types are not the same,
    // then perform an implicit conversion.
    template <typename Target, typename B, typename Source>
    inline typename ::boost::lazy_disable_if<
        B
      , ::boost::parameter::aux::cast_convert<Source,Target>
    >::type
        forward(Source&& source)
    {
        return ::boost::parameter::aux::cast_convert<Source,Target>
        ::evaluate(::std::forward<Source>(source));
    }

    // If the source and target types are the same,
    // then simply forward the argument.
    // However, treat rvalue references to scalars as const lvalue references.
    template <typename T, typename B>
    inline typename ::boost::enable_if<B,T const&>::type forward(T const& t)
    {
        return t;
    }

    template <typename T, typename B>
    inline typename ::boost::enable_if<
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        ::boost::mp11::mp_if<
            B
          , ::boost::mp11::mp_if<
                ::std::is_const<T>
              , ::boost::mp11::mp_false
              , ::boost::mp11::mp_true
            >
          , ::boost::mp11::mp_false
        >
#else
        typename ::boost::mpl::eval_if<
            B
          , ::boost::mpl::if_<
                ::boost::is_const<T>
              , ::boost::mpl::false_
              , ::boost::mpl::true_
            >
          , ::boost::mpl::false_
        >::type
#endif  // BOOST_PARAMETER_CAN_USE_MP11
      , T&
    >::type
        forward(T& t)
    {
        return t;
    }
}}} // namespace boost::parameter::aux

#include <boost/type_traits/is_scalar.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename T, typename B>
    inline typename ::boost::enable_if<
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        ::boost::mp11::mp_if<
            B
          , ::boost::mp11::mp_if<
                ::std::is_scalar<T>
              , ::boost::mp11::mp_false
              , ::boost::mp11::mp_true
            >
          , ::boost::mp11::mp_false
        >
#else
        typename ::boost::mpl::eval_if<
            B
          , ::boost::mpl::if_<
                ::boost::is_scalar<T>
              , ::boost::mpl::false_
              , ::boost::mpl::true_
            >
          , ::boost::mpl::false_
        >::type
#endif  // BOOST_PARAMETER_CAN_USE_MP11
      , T const&&
    >::type
        forward(T const&& t)
    {
        return static_cast<T const&&>(t);
    }

    template <typename T, typename B>
    inline typename ::boost::enable_if<
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        ::boost::mp11::mp_if<
            B
          , ::boost::mp11::mp_if<
                ::std::is_scalar<T>
              , ::boost::mp11::mp_false
              , ::boost::mp11::mp_true
            >
          , ::boost::mp11::mp_false
        >
#else
        typename ::boost::mpl::eval_if<
            B
          , ::boost::mpl::if_<
                ::boost::is_scalar<T>
              , ::boost::mpl::false_
              , ::boost::mpl::true_
            >
          , ::boost::mpl::false_
        >::type
#endif  // BOOST_PARAMETER_CAN_USE_MP11
      , T&&
    >::type
        forward(T&& t)
    {
        return ::std::forward<T>(t);
    }
}}} // namespace boost::parameter::aux

#elif BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
#define BOOST_PARAMETER_FUNCTION_CAST_T(value_t, predicate, args) value_t
#define BOOST_PARAMETER_FUNCTION_CAST_B(value, predicate, args) value
#else   // no perfect forwarding support and no Borland workarounds needed

namespace boost { namespace parameter { namespace aux {

    // Handles possible implicit casts.  Used by preprocessor.hpp
    // to normalize user input.
    //
    // cast<void*>::execute() is identity
    // cast<void*(X)>::execute() is identity
    // cast<void(X)>::execute() casts to X
    //
    // preprocessor.hpp uses this like this:
    //
    //     #define X(value, predicate)
    //         cast<void predicate>::execute(value)
    //
    //     X(something, *)
    //     X(something, *(predicate))
    //     X(something, (int))
    template <typename VoidExpr, typename Args>
    struct cast;
}}} // namespace boost::parameter::aux

#include <boost/parameter/aux_/use_default_tag.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename Args>
    struct cast<void*,Args>
    {
        template <typename T>
        struct apply
        {
            typedef T& type;
        };

        inline static ::boost::parameter::aux::use_default_tag
            execute(::boost::parameter::aux::use_default_tag)
        {
            return ::boost::parameter::aux::use_default_tag();
        }

        template <typename U>
        inline static U& execute(U& value)
        {
            return value;
        }
    };
}}} // namespace boost::parameter::aux

#include <boost/parameter/aux_/void.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename Predicate, typename Args>
#if BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x580))
    struct cast< ::boost::parameter::aux::voidstar(Predicate),Args>
#else
    struct cast<void*(Predicate),Args>
#endif
      : ::boost::parameter::aux::cast<void*,Args>
    {
    };
}}} // namespace boost::parameter::aux

#include <boost/mpl/placeholders.hpp>

namespace boost { namespace parameter { namespace aux {

    // This is a hack used in cast<> to turn the user supplied type,
    // which may or may not be a placeholder expression, into one,
    // so that it will be properly evaluated by mpl::apply.
    template <typename T, typename Dummy = ::boost::mpl::_1>
    struct as_placeholder_expr
    {
        typedef T type;
    };
}}} // namespace boost::parameter::aux

#include <boost/mpl/apply.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename Target, typename Source, typename Args>
    struct is_target_same_as_source
      : ::boost::mpl::if_<
            ::boost::is_same<
                typename ::boost::remove_const<
                    typename ::boost::remove_reference<
                        typename ::boost::mpl::apply2<
                            ::boost::parameter::aux
                            ::as_placeholder_expr<Target>
                          , Source
                          , Args
                        >::type
                    >::type
                >::type
              , typename ::boost::remove_const<Source>::type
            >
          , ::boost::mpl::true_
          , ::boost::mpl::false_
        >::type
    {
    };

    template <
        typename Target
      , typename Source
      , typename Args
      , typename Enable = ::boost::parameter::aux
        ::is_target_same_as_source<Target,Source,Args>
    >
    struct cast_impl
    {
        typedef Source& type;

        inline static Source& evaluate(Source& value)
        {
            return value;
        }
    };
}}} // namespace boost::parameter::aux

#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_lvalue_reference.hpp>

namespace boost { namespace parameter { namespace aux {

    // Covers the case where is_convertible<Source,Target> but not
    // is_same<Source,Target>.  Use cases are covered
    // by test/normalize_argument_types.cpp
    template <typename Source, typename Target>
    class cast_convert
    {
        typedef ::boost::parameter::aux::cast_convert<Source,Target> _self;

     public:
        typedef typename ::boost::add_lvalue_reference<
            typename ::boost::add_const<Target>::type
        >::type type;

     private:
        template <typename U>
        inline static typename _self::type _mod_const(U const& u)
        {
            return u;
        }

        inline static Target _copy(Target value)
        {
            return value;
        }

     public:
        inline static typename _self::type evaluate(Source& source)
        {
            return _self::_mod_const(_self::_copy(source));
        }
    };

    template <typename Target, typename Source, typename Args>
    struct cast_impl<Target,Source,Args,::boost::mpl::false_>
      : ::boost::parameter::aux::cast_convert<
            Source,
            typename ::boost::mpl::apply2<
                ::boost::parameter::aux::as_placeholder_expr<Target>
              , Source
              , Args
            >::type
        >
    {
    };
}}} // namespace boost::parameter::aux

#include <boost/mpl/eval_if.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename Target, typename Args>
    struct cast<void(Target),Args>
    {
        template <typename T>
        struct apply
        {
            typedef typename ::boost::mpl::eval_if<
                    ::boost::parameter::aux
                    ::is_target_same_as_source<Target,T,Args>
                  , ::boost::add_lvalue_reference<T>
                  , ::boost::parameter::aux::cast_impl<
                        Target
                      , T
                      , Args
                      , ::boost::mpl::false_
                    >
            >::type type;
        };

        inline static ::boost::parameter::aux::use_default_tag
            execute(::boost::parameter::aux::use_default_tag)
        {
            return ::boost::parameter::aux::use_default_tag();
        }

        template <typename U>
        inline static typename ::boost::parameter::aux
        ::cast_impl<Target,U const,Args>::type
            execute(U const& value)
        {
            return ::boost::parameter::aux
            ::cast_impl<Target,U const,Args>::evaluate(value);
        }

        template <typename U>
        inline static typename ::boost::parameter::aux
        ::cast_impl<Target,U,Args>::type
            execute(U& value)
        {
            return ::boost::parameter::aux
            ::cast_impl<Target,U,Args>::evaluate(value);
        }
    };
}}} // namespace boost::parameter::aux

#include <boost/mpl/apply_wrap.hpp>
#include <boost/parameter/value_type.hpp>

// Expands to the reference-qualified target type of the argument
// as indicated by the predicate.
#define BOOST_PARAMETER_FUNCTION_CAST_T(tag, predicate, args)                \
    typename ::boost::mpl::apply_wrap1<                                      \
        ::boost::parameter::aux::cast<void predicate, args>                  \
      , typename ::boost::parameter::value_type<                             \
            args                                                             \
          , tag                                                              \
          , ::boost::parameter::aux::use_default_tag                         \
        >::type                                                              \
    >::type
/**/

// Expands to the converted or passed-through value
// as indicated by the predicate.
#define BOOST_PARAMETER_FUNCTION_CAST_B(value, predicate, args)              \
    ::boost::parameter::aux::cast<void predicate, args>::execute(value)
/**/

#endif  // perfect forwarding support, or Borland workarounds needed
#endif  // include guard

