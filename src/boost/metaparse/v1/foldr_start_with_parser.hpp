#ifndef BOOST_METAPARSE_V1_FOLDR_START_WITH_PARSER_HPP
#define BOOST_METAPARSE_V1_FOLDR_START_WITH_PARSER_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/accept.hpp>
#include <boost/metaparse/v1/is_error.hpp>
#include <boost/metaparse/v1/get_position.hpp>
#include <boost/metaparse/v1/get_result.hpp>
#include <boost/metaparse/v1/get_remaining.hpp>

#include <boost/mpl/eval_if.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class P, class StateP, class BackwardOp>
      struct foldr_start_with_parser
      {
      private:
        template <class Res, class Rem>
        struct apply_unchecked1 :
          accept<
            typename BackwardOp::template apply<
              typename get_result<Rem>::type,
              typename get_result<Res>::type
            >::type,
            typename get_remaining<Rem>::type,
            typename get_position<Rem>::type
          >
        {};

        template <class Res>
        struct apply_unchecked;
      public:
        typedef foldr_start_with_parser type;
      
        template <class S, class Pos>
        struct apply :
          boost::mpl::eval_if<
            typename is_error<typename P::template apply<S, Pos> >::type,
            typename StateP::template apply<S, Pos>,
            apply_unchecked<typename P::template apply<S, Pos> >
          >
        {};
      private:
        template <class Res>
        struct apply_unchecked
        {
        private:
          typedef
            typename foldr_start_with_parser::template apply<
              typename get_remaining<Res>::type,
              typename get_position<Res>::type
            >
            parsed_remaining;
        public:
          typedef
            typename boost::mpl::eval_if<
              typename is_error<parsed_remaining>::type,
              parsed_remaining,
              apply_unchecked1<Res, parsed_remaining>
            >::type
            type;
        };
      };
    }
  }
}

#endif

