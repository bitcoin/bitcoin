//  (C) Copyright 2009-2011 Frederic Bron.
//
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_HAS_BIT_XOR_ASSIGN_HPP_INCLUDED
#define BOOST_TT_HAS_BIT_XOR_ASSIGN_HPP_INCLUDED

#define BOOST_TT_TRAIT_NAME has_bit_xor_assign
#define BOOST_TT_TRAIT_OP ^=
#define BOOST_TT_FORBIDDEN_IF\
   (\
      /* Lhs==fundamental and Rhs==fundamental and (Lhs!=integral or Rhs!=integral) */\
      (\
         ::boost::is_fundamental< Lhs_nocv >::value && \
         ::boost::is_fundamental< Rhs_nocv >::value && \
         ( \
            (!  ::boost::is_integral< Lhs_noref >::value ) || \
            (!  ::boost::is_integral< Rhs_noref >::value )\
          )\
      )||\
      /* Lhs==fundamental and Rhs==pointer */\
      (\
         ::boost::is_fundamental< Lhs_nocv >::value && \
         ::boost::is_pointer< Rhs_noref >::value\
      )||\
      /* Rhs==fundamental and Lhs==pointer */\
      (\
         ::boost::is_fundamental< Rhs_nocv >::value && \
         ::boost::is_pointer< Lhs_noref >::value\
      )||\
      /* Lhs==pointer and Rhs==pointer */\
      (\
         ::boost::is_pointer< Lhs_noref >::value && \
         ::boost::is_pointer< Rhs_noref >::value\
      )||\
      /* Lhs==fundamental and Rhs==fundamental and Lhs==const */\
      (\
         ::boost::is_fundamental< Lhs_nocv >::value && \
         ::boost::is_fundamental< Rhs_nocv >::value && \
         ::boost::is_const< Lhs_noref >::value\
      )\
      )


#include <boost/type_traits/detail/has_binary_operator.hpp>

#undef BOOST_TT_TRAIT_NAME
#undef BOOST_TT_TRAIT_OP
#undef BOOST_TT_FORBIDDEN_IF

#endif
