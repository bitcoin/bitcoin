#ifndef BOOST_METAPARSE_V1_CPP98_IMPL_NTH_OF_C_IMPL_HPP
#define BOOST_METAPARSE_V1_CPP98_IMPL_NTH_OF_C_IMPL_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp98/impl/skip_seq.hpp>

#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/fold.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <int N, class Seq>
        struct nth_of_c_impl
        {
        private:
          template <class NextResult>
          struct apply_unchecked :
            nth_of_c_impl<
              N - 1,
              typename boost::mpl::pop_front<Seq>::type
            >::template apply<
              typename get_remaining<NextResult>::type,
              typename get_position<NextResult>::type
            >
          {};
        public:
          typedef nth_of_c_impl type;
          
          template <class S, class Pos>
          struct apply :
            boost::mpl::eval_if<
              typename is_error<
                typename boost::mpl::front<Seq>::type::template apply<S, Pos>
              >::type,
              typename boost::mpl::front<Seq>::type::template apply<S, Pos>,
              apply_unchecked<
                typename boost::mpl::front<Seq>::type::template apply<S, Pos>
              >
            >
          {};
        };
        
        template <class Seq>
        struct nth_of_c_impl<0, Seq>
        {
          typedef nth_of_c_impl type;
          
          template <class S, class Pos>
          struct apply :
            boost::mpl::fold<
              typename boost::mpl::pop_front<Seq>::type,
              typename boost::mpl::front<Seq>::type::template apply<
                S,
                Pos
              >::type,
              skip_seq
            >
          {};
        };
      }
    }
  }
}

#endif

