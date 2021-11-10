#ifndef BOOST_METAPARSE_V1_LOOK_AHEAD_HPP
#define BOOST_METAPARSE_V1_LOOK_AHEAD_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)   2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/is_error.hpp>
#include <boost/metaparse/v1/accept.hpp>
#include <boost/metaparse/v1/get_result.hpp>

#include <boost/mpl/eval_if.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class P>
      struct look_ahead
      {
      private:
        template <class S, class Pos>
        struct no_error :
          accept<
            typename get_result<typename P::template apply<S, Pos> >::type,
            S,
            Pos
          >
        {};
      public:
        typedef look_ahead type;
        
        template <class S, class Pos>
        struct apply :
          boost::mpl::eval_if<
            typename is_error<typename P::template apply<S, Pos> >::type,
            typename P::template apply<S, Pos>,
            no_error<S, Pos>
          >
        {};
      };
    }
  }
}

#endif

