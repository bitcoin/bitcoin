#ifndef BOOST_METAPARSE_V1_BUILD_PARSER_HPP
#define BOOST_METAPARSE_V1_BUILD_PARSER_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2009 - 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/fwd/build_parser.hpp>
#include <boost/metaparse/v1/start.hpp>
#include <boost/metaparse/v1/get_result.hpp>
#include <boost/metaparse/v1/get_position.hpp>
#include <boost/metaparse/v1/get_message.hpp>
#include <boost/metaparse/v1/get_line.hpp>
#include <boost/metaparse/v1/get_col.hpp>
#include <boost/metaparse/v1/is_error.hpp>

#include <boost/mpl/eval_if.hpp>

#include <boost/static_assert.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <int Line, int Col, class Msg>
      struct x__________________PARSING_FAILED__________________x
      {
        BOOST_STATIC_ASSERT(Line == Line + 1);
      };

      template <class P, class S>
      struct parsing_failed :
        x__________________PARSING_FAILED__________________x<
          get_line<
            get_position<typename P::template apply<S, start> >
          >::type::value,
          get_col<
            get_position<typename P::template apply<S, start> >
          >::type::value,
          typename get_message<typename P::template apply<S, start> >::type
        >
      {};

      template <class P>
      struct build_parser
      {
        typedef build_parser type;
        
        template <class S>
        struct apply :
          boost::mpl::eval_if<
            typename is_error<typename P::template apply<S, start> >::type,
            parsing_failed<P, S>,
            get_result<typename P::template apply<S, start> >
          >
        {};
      };
    }
  }
}

#endif

