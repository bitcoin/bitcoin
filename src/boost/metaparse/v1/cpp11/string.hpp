#ifndef BOOST_METAPARSE_V1_CPP11_STRING_HPP
#define BOOST_METAPARSE_V1_CPP11_STRING_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp11/fwd/string.hpp>
#include <boost/metaparse/v1/string_tag.hpp>
#include <boost/metaparse/v1/impl/string_iterator.hpp>
#include <boost/metaparse/v1/cpp11/impl/empty_string.hpp>
#include <boost/metaparse/v1/cpp11/impl/size.hpp>
#include <boost/metaparse/v1/cpp11/impl/pop_front.hpp>
#include <boost/metaparse/v1/cpp11/impl/push_front_c.hpp>
#include <boost/metaparse/v1/cpp11/impl/push_back_c.hpp>
#include <boost/metaparse/v1/cpp11/impl/pop_back.hpp>
#include <boost/metaparse/v1/cpp11/impl/string_at.hpp>

#include <type_traits>

/*
 * The string type
 */

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <char... Cs>
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
      struct apply : std::is_same<typename A::type, typename B::type> {};
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

    template <char... Cs>
    struct c_str<boost::metaparse::v1::string<Cs...>>
    {
      typedef c_str type;
      static constexpr char value[sizeof...(Cs) + 1] = {Cs..., 0};
    };

    template <>
    struct c_str<boost::metaparse::v1::string<>> :
      boost::metaparse::v1::impl::empty_string<>
    {};

    template <char... Cs>
    constexpr char c_str<boost::metaparse::v1::string<Cs...>>::value[];
  }
}

#if __clang__
#  if __has_extension(cxx_string_literal_templates)
#    define BOOST_METAPARSE_V1_STRING(...) ::boost::metaparse::string<__VA_ARGS__>
#  else
#    include <boost/metaparse/v1/cpp11/impl/string.hpp>
#  endif
#else
#  include <boost/metaparse/v1/cpp11/impl/string.hpp>
#endif

#endif

