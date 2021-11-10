// Boost.Range library
//
//  Copyright Neil Groves 2014. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#define BOOST_RANGE_combined_exp_pred(d, data) BOOST_PP_TUPLE_ELEM(3, 0, data)

#define BOOST_RANGE_combined_exp_op(d, data) \
 ( \
    BOOST_PP_DEC( \
       BOOST_PP_TUPLE_ELEM(3, 0, data) \
    ), \
    BOOST_PP_TUPLE_ELEM(3, 1, data), \
    BOOST_PP_MUL_D( \
       d, \
       BOOST_PP_TUPLE_ELEM(3, 2, data), \
       BOOST_PP_TUPLE_ELEM(3, 1, data) \
    ) \
 )

#define BOOST_RANGE_combined_exp(x, n) \
  BOOST_PP_TUPLE_ELEM(3, 2, \
  BOOST_PP_WHILE(BOOST_RANGE_combined_exp_pred, \
                 BOOST_RANGE_combined_exp_op, (n, x, 1)))

#define BOOST_RANGE_combined_bitset_pred(n, state) \
    BOOST_PP_TUPLE_ELEM(2,1,state)

#define BOOST_RANGE_combined_bitset_op(d, state) \
    (BOOST_PP_DIV_D(d, BOOST_PP_TUPLE_ELEM(2,0,state), 2), \
     BOOST_PP_DEC(BOOST_PP_TUPLE_ELEM(2,1,state)))

#define BOOST_RANGE_combined_bitset(i, n) \
BOOST_PP_MOD(BOOST_PP_TUPLE_ELEM(2, 0, \
      BOOST_PP_WHILE(BOOST_RANGE_combined_bitset_pred, \
                     BOOST_RANGE_combined_bitset_op, (i,n))), 2)

#define BOOST_RANGE_combined_range_iterator(z, n, i) \
  typename range_iterator< \
      BOOST_PP_CAT(R,n)          \
      BOOST_PP_IF( \
          BOOST_RANGE_combined_bitset(i,n), \
          BOOST_PP_IDENTITY(const), \
          BOOST_PP_EMPTY)() \
  >::type

#define BOOST_RANGE_combined_args(z, n, i) \
  BOOST_PP_CAT(R, n) \
  BOOST_PP_IF(BOOST_RANGE_combined_bitset(i,n), const&, &)  \
  BOOST_PP_CAT(r, n)

#define BOOST_RANGE_combine_impl(z, i, n)\
    template<BOOST_PP_ENUM_PARAMS(n, typename R)> \
    inline range::combined_range< \
        boost::tuple<BOOST_PP_ENUM(n, BOOST_RANGE_combined_range_iterator, i)> \
    > \
    combine(BOOST_PP_ENUM(n, BOOST_RANGE_combined_args, i)) \
    { \
        typedef tuple< \
            BOOST_PP_ENUM(n, BOOST_RANGE_combined_range_iterator, i) \
        > rng_tuple_t;   \
        return range::combined_range<rng_tuple_t>( \
            rng_tuple_t(BOOST_PP_ENUM(n, BOOST_RANGE_combined_seq, begin)), \
            rng_tuple_t(BOOST_PP_ENUM(n, BOOST_RANGE_combined_seq, end))); \
    }


#define BOOST_RANGE_combine(z, n, data) \
  BOOST_PP_REPEAT(BOOST_RANGE_combined_exp(2,n), BOOST_RANGE_combine_impl, n)
