#ifndef BOOST_METAPARSE_V1_CPP98_STRING_HPP
#define BOOST_METAPARSE_V1_CPP98_STRING_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp98/fwd/string.hpp>
#include <boost/metaparse/v1/string_tag.hpp>
#include <boost/metaparse/v1/impl/string_iterator.hpp>
#include <boost/metaparse/v1/cpp98/impl/empty_string.hpp>
#include <boost/metaparse/v1/cpp98/impl/size.hpp>
#include <boost/metaparse/v1/cpp98/impl/pop_front.hpp>
#include <boost/metaparse/v1/cpp98/impl/push_front_c.hpp>
#include <boost/metaparse/v1/cpp98/impl/push_back_c.hpp>
#include <boost/metaparse/v1/cpp98/impl/pop_back.hpp>

#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

#include <boost/type_traits/is_same.hpp>

/*
 * The string type
 */

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <BOOST_PP_ENUM_PARAMS(BOOST_METAPARSE_LIMIT_STRING_SIZE, int C)>
      struct string
      {
        typedef string type;
        typedef string_tag tag;
      };
    }
  }
}

/*
 * Boost.MPL overloads
 */

namespace boost
{
  namespace mpl
  {
    // push_back
    template <class S>
    struct push_back_impl;

    template <>
    struct push_back_impl<boost::metaparse::v1::string_tag>
    {
      typedef push_back_impl type;

      template <class S, class C>
      struct apply :
        boost::metaparse::v1::impl::push_back_c<
          typename S::type,
          C::type::value
        >
      {};
    };

    // pop_back
    template <class S>
    struct pop_back_impl;

    template <>
    struct pop_back_impl<boost::metaparse::v1::string_tag>
    {
      typedef pop_back_impl type;

      template <class S>
      struct apply : boost::metaparse::v1::impl::pop_back<S> {};
    };

    // push_front
    template <class S>
    struct push_front_impl;

    template <>
    struct push_front_impl<boost::metaparse::v1::string_tag>
    {
      typedef push_front_impl type;

      template <class S, class C>
      struct apply :
        boost::metaparse::v1::impl::push_front_c<
          typename S::type,
          C::type::value
        >
      {};
    };

    // pop_front
    template <class S>
    struct pop_front_impl;

    template <>
    struct pop_front_impl<boost::metaparse::v1::string_tag>
    {
      typedef pop_front_impl type;

      template <class S>
      struct apply : boost::metaparse::v1::impl::pop_front<S> {};
    };

    // clear
    template <class S>
    struct clear_impl;

    template <>
    struct clear_impl<boost::metaparse::v1::string_tag>
    {
      typedef clear_impl type;

      template <class S>
      struct apply : boost::metaparse::v1::string<> {};
    };

    // begin
    template <class S>
    struct begin_impl;

    template <>
    struct begin_impl<boost::metaparse::v1::string_tag>
    {
      typedef begin_impl type;

      template <class S>
      struct apply :
        boost::metaparse::v1::impl::string_iterator<typename S::type, 0>
      {};
    };

    // end
    template <class S>
    struct end_impl;

    template <>
    struct end_impl<boost::metaparse::v1::string_tag>
    {
      typedef end_impl type;

      template <class S>
      struct apply :
        boost::metaparse::v1::impl::string_iterator<
          typename S::type,
          boost::metaparse::v1::impl::size<typename S::type>::type::value
        >
      {};
    };

    // equal_to
    template <class A, class B>
    struct equal_to_impl;

    template <>
    struct equal_to_impl<
      boost::metaparse::v1::string_tag,
      boost::metaparse::v1::string_tag
    >
    {
      typedef equal_to_impl type;

      template <class A, class B>
      struct apply : boost::is_same<typename A::type, typename B::type> {};
    };

    template <class T>
    struct equal_to_impl<boost::metaparse::v1::string_tag, T>
    {
      typedef equal_to_impl type;
      
      template <class, class>
      struct apply : false_ {};
    };
    
    template <class T>
    struct equal_to_impl<T, boost::metaparse::v1::string_tag> :
      equal_to_impl<boost::metaparse::v1::string_tag, T>
    {};

    // c_str
    template <class S>
    struct c_str;

    template <BOOST_PP_ENUM_PARAMS(BOOST_METAPARSE_LIMIT_STRING_SIZE, int C)>
    struct c_str<
      boost::metaparse::v1::string<
        BOOST_PP_ENUM_PARAMS(BOOST_METAPARSE_LIMIT_STRING_SIZE, C)
      >
    >
    {
      typedef c_str type;
      static const char value[BOOST_METAPARSE_LIMIT_STRING_SIZE + 1];
    };

    template <BOOST_PP_ENUM_PARAMS(BOOST_METAPARSE_LIMIT_STRING_SIZE, int C)>
    const char
      c_str<
        boost::metaparse::v1::string<
          BOOST_PP_ENUM_PARAMS(BOOST_METAPARSE_LIMIT_STRING_SIZE, C)
        >
      >::value[BOOST_METAPARSE_LIMIT_STRING_SIZE + 1]
        = {BOOST_PP_ENUM_PARAMS(BOOST_METAPARSE_LIMIT_STRING_SIZE, C), 0};

    template <>
    struct c_str<boost::metaparse::v1::string<> > :
      boost::metaparse::v1::impl::empty_string<>
    {
      typedef c_str type;
    };

    #ifdef BOOST_METAPARSE_STRING_CASE
    #  error BOOST_METAPARSE_STRING_CASE is already defined
    #endif
    #define BOOST_METAPARSE_STRING_CASE(z, n, unused) \
      template <BOOST_PP_ENUM_PARAMS(n, int C)> \
      struct \
        c_str< \
          boost::metaparse::v1::string< \
            BOOST_PP_ENUM_PARAMS(n, C) BOOST_PP_COMMA_IF(n) \
            BOOST_PP_ENUM( \
              BOOST_PP_SUB(BOOST_METAPARSE_LIMIT_STRING_SIZE, n), \
              BOOST_NO_CHAR BOOST_PP_TUPLE_EAT(3), \
              ~ \
            ) \
          > \
        > \
      { \
        typedef c_str type; \
        static const char value[n + 1]; \
      }; \
      \
      template <BOOST_PP_ENUM_PARAMS(n, int C)> \
      const char c_str< \
        boost::metaparse::v1::string< \
          BOOST_PP_ENUM_PARAMS(n, C) BOOST_PP_COMMA_IF(n) \
          BOOST_PP_ENUM( \
            BOOST_PP_SUB(BOOST_METAPARSE_LIMIT_STRING_SIZE, n), \
            BOOST_NO_CHAR BOOST_PP_TUPLE_EAT(3), \
            ~ \
          ) \
        > \
      >::value[n + 1] = {BOOST_PP_ENUM_PARAMS(n, C) BOOST_PP_COMMA_IF(n) 0};

    BOOST_PP_REPEAT_FROM_TO(
      1,
      BOOST_METAPARSE_LIMIT_STRING_SIZE,
      BOOST_METAPARSE_STRING_CASE,
      ~
    )

    #undef BOOST_METAPARSE_STRING_CASE
  }
}

#define BOOST_METAPARSE_V1_CONFIG_NO_BOOST_METAPARSE_STRING 1

#endif

