#ifndef BOOST_METAPARSE_V1_TRANSFORM_ERROR_MESSAGE_HPP
#define BOOST_METAPARSE_V1_TRANSFORM_ERROR_MESSAGE_HPP

//    Copyright Abel Sinkovics (abel@sinkovics.hu) 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/is_error.hpp>
#include <boost/metaparse/v1/reject.hpp>
#include <boost/metaparse/v1/get_position.hpp>
#include <boost/metaparse/v1/get_message.hpp>

#include <boost/mpl/eval_if.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class P, class F>
      struct transform_error_message
      {
        template <class R>
        struct rejection :
          reject<
            typename F::template apply<typename get_message<R>::type>::type,
            get_position<R>
          >
        {};

        template <class S, class Pos>
        struct apply :
          boost::mpl::eval_if<
            typename is_error<typename P::template apply<S, Pos> >::type,
            rejection<typename P::template apply<S, Pos> >,
            typename P::template apply<S, Pos>
          >
        {};
        
        typedef transform_error_message type;
      };
    }
  }
}

#endif

