#ifndef BOOST_METAPARSE_V1_SOURCE_POSITION_HPP
#define BOOST_METAPARSE_V1_SOURCE_POSITION_HPP

//    Copyright Abel Sinkovics (abel@sinkovics.hu) 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/fwd/source_position.hpp>
#include <boost/metaparse/v1/source_position_tag.hpp>


#include <boost/mpl/bool.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/less.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class Line, class Col, class PrevChar>
      struct source_position
      {
        typedef source_position_tag tag;
        typedef source_position type;
        
        typedef Line line;
        typedef Col col;
        typedef PrevChar prev_char;
      };
    }
  }
}

namespace boost
{
  namespace mpl
  {
    template <class TagA, class TagB>
    struct equal_to_impl;

    template <>
    struct equal_to_impl<
      boost::metaparse::v1::source_position_tag,
      boost::metaparse::v1::source_position_tag
    >
    {
      typedef equal_to_impl type;

      template <class A, class B>
      struct apply :
        bool_<
          A::type::line::value == B::type::line::value
          && A::type::col::value == B::type::col::value
          && A::type::prev_char::value == B::type::prev_char::value
        >
      {};
    };

    template <class TagA, class TagB>
    struct not_equal_to_impl;

    template <>
    struct not_equal_to_impl<
      boost::metaparse::v1::source_position_tag,
      boost::metaparse::v1::source_position_tag
    >
    {
      typedef not_equal_to_impl type;

      template <class A, class B>
      struct apply : bool_<!equal_to<A, B>::type::value> {};
    };

    template <class TagA, class TagB>
    struct less_impl;

    template <>
    struct less_impl<
      boost::metaparse::v1::source_position_tag,
      boost::metaparse::v1::source_position_tag
    >
    {
      typedef less_impl type;

      template <class A, class B>
      struct apply :
        bool_<(
          (A::type::line::value) < (B::type::line::value) || (
            (A::type::line::value) == (B::type::line::value) && (
              (A::type::col::value) < (B::type::col::value) || (
                (A::type::col::value) == (B::type::col::value) &&
                (A::type::prev_char::value) < (B::type::prev_char::value)
              )
            )
          )
        )>
      {};
    };

    template <class TagA, class TagB>
    struct greater_impl;

    template <>
    struct greater_impl<
      boost::metaparse::v1::source_position_tag,
      boost::metaparse::v1::source_position_tag
    >
    {
      typedef greater_impl type;

      template <class A, class B>
      struct apply :
        bool_<!(less<A, B>::type::value || equal_to<A, B>::type::value)>
      {};
    };

    template <class TagA, class TagB>
    struct greater_equal_impl;

    template <>
    struct greater_equal_impl<
      boost::metaparse::v1::source_position_tag,
      boost::metaparse::v1::source_position_tag
    >
    {
      typedef greater_equal_impl type;

      template <class A, class B>
      struct apply : bool_<!less<A, B>::type::value> {};
    };

    template <class TagA, class TagB>
    struct less_equal_impl;

    template <>
    struct less_equal_impl<
      boost::metaparse::v1::source_position_tag,
      boost::metaparse::v1::source_position_tag
    >
    {
      typedef less_equal_impl type;

      template <class A, class B>
      struct apply :
        bool_<less<A, B>::type::value || equal_to<A, B>::type::value>
      {};
    };

  }
}

#endif

