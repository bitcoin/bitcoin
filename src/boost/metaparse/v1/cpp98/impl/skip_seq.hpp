#ifndef BOOST_METAPARSE_V1_CPP98_IMPL_SKIP_SEQ_HPP
#define BOOST_METAPARSE_V1_CPP98_IMPL_SKIP_SEQ_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/is_error.hpp>
#include <boost/metaparse/v1/accept.hpp>
#include <boost/metaparse/v1/get_remaining.hpp>
#include <boost/metaparse/v1/get_position.hpp>
#include <boost/metaparse/v1/get_result.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        struct skip_seq
        {
        private:
          template <class ParsingResult, class NewResultValue>
          struct change_result :
            accept<
              NewResultValue,
              typename get_remaining<ParsingResult>::type,
              typename get_position<ParsingResult>::type
            >
          {};
        
          template <class Result, class P>
          struct apply_unchecked :
            boost::mpl::eval_if<
              typename is_error<
                typename P::template apply<
                  typename get_remaining<Result>::type,
                  typename get_position<Result>::type
                >
              >::type,
              typename P::template apply<
                typename get_remaining<Result>::type,
                typename get_position<Result>::type
              >,
              change_result<
                typename P::template apply<
                  typename get_remaining<Result>::type,
                  typename get_position<Result>::type
                >,
                typename get_result<Result>::type
              >
            >
          {};
          
        public:
          template <class Result, class P>
          struct apply :
            boost::mpl::eval_if<
              is_error<Result>,
              Result,
              apply_unchecked<Result, P>
            >
          {};
        };
      }
    }
  }
}

#endif

