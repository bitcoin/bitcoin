#ifndef BOOST_METAPARSE_V1_ONE_CHAR_HPP
#define BOOST_METAPARSE_V1_ONE_CHAR_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2009 - 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/error/unexpected_end_of_input.hpp>
#include <boost/metaparse/v1/next_char.hpp>
#include <boost/metaparse/v1/next_line.hpp>
#include <boost/metaparse/v1/accept.hpp>
#include <boost/metaparse/v1/reject.hpp>
#include <boost/metaparse/v1/get_prev_char.hpp>

#include <boost/mpl/empty.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/bool.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      struct one_char
      {
      private:
        template <class C, class Pos>
        struct next_pos :
          boost::mpl::eval_if<
            boost::mpl::bool_<
              C::type::value == '\r'
              || (
                C::type::value == '\n'
                && get_prev_char<Pos>::type::value != '\r'
              )
            >,
            next_line<Pos, C>,
            next_char<Pos, C>
          >
        {};

        template <class S, class NextPos>
        struct unchecked :
          accept<
            typename boost::mpl::front<S>::type,
            boost::mpl::pop_front<S>,
            NextPos
          >
        {};
      public:
        typedef one_char type;
        
        template <class S, class Pos>
        struct apply :
          boost::mpl::eval_if<
            typename boost::mpl::empty<S>::type,
            reject<error::unexpected_end_of_input, Pos>,
            unchecked<S, next_pos<boost::mpl::front<S>, Pos> >
          >
        {};
      };
    }
  }
}

#endif

