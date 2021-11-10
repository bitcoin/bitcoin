// -------------- Boost static_log2.hpp header file  ----------------------- //
//
//                 Copyright (C) 2001 Daryle Walker.
//                 Copyright (C) 2003 Vesa Karvonen.
//                 Copyright (C) 2003 Gennaro Prota.
//
//     Distributed under the Boost Software License, Version 1.0.
//        (See accompanying file LICENSE_1_0.txt or copy at
//              https://www.boost.org/LICENSE_1_0.txt)
//
//         ---------------------------------------------------
//       See https://www.boost.org/libs/integer for documentation.
// ------------------------------------------------------------------------- //


#ifndef BOOST_INTEGER_STATIC_LOG2_HPP
#define BOOST_INTEGER_STATIC_LOG2_HPP

#include <boost/config.hpp>
#include <boost/integer_fwd.hpp>

namespace boost {

 namespace detail {

     namespace static_log2_impl {

     // choose_initial_n<>
     //
     // Recursively doubles its integer argument, until it
     // becomes >= of the "width" (C99, 6.2.6.2p4) of
     // static_log2_argument_type.
     //
     // Used to get the maximum power of two less then the width.
     //
     // Example: if on your platform argument_type has 48 value
     //          bits it yields n=32.
     //
     // It's easy to prove that, starting from such a value
     // of n, the core algorithm works correctly for any width
     // of static_log2_argument_type and that recursion always
     // terminates with x = 1 and n = 0 (see the algorithm's
     // invariant).

     typedef boost::static_log2_argument_type argument_type;
     typedef boost::static_log2_result_type result_type;

     template <result_type n>
     struct choose_initial_n {

         BOOST_STATIC_CONSTANT(bool, c = (argument_type(1) << n << n) != 0);
         BOOST_STATIC_CONSTANT(
             result_type,
             value = !c*n + choose_initial_n<2*c*n>::value
         );

     };

     template <>
     struct choose_initial_n<0> {
         BOOST_STATIC_CONSTANT(result_type, value = 0);
     };



     // start computing from n_zero - must be a power of two
     const result_type n_zero = 16;
     const result_type initial_n = choose_initial_n<n_zero>::value;

     // static_log2_impl<>
     //
     // * Invariant:
     //                 2n
     //  1 <= x && x < 2    at the start of each recursion
     //                     (see also choose_initial_n<>)
     //
     // * Type requirements:
     //
     //   argument_type maybe any unsigned type with at least n_zero + 1
     //   value bits. (Note: If larger types will be standardized -e.g.
     //   unsigned long long- then the argument_type typedef can be
     //   changed without affecting the rest of the code.)
     //

     template <argument_type x, result_type n = initial_n>
     struct static_log2_impl {

         BOOST_STATIC_CONSTANT(bool, c = (x >> n) > 0); // x >= 2**n ?
         BOOST_STATIC_CONSTANT(
             result_type,
             value = c*n + (static_log2_impl< (x>>c*n), n/2 >::value)
         );

     };

     template <>
     struct static_log2_impl<1, 0> {
        BOOST_STATIC_CONSTANT(result_type, value = 0);
     };

     }
 } // detail



 // --------------------------------------
 // static_log2<x>
 // ----------------------------------------

 template <static_log2_argument_type x>
 struct static_log2 {

     BOOST_STATIC_CONSTANT(
         static_log2_result_type,
         value = detail::static_log2_impl::static_log2_impl<x>::value
     );

 };


 template <>
 struct static_log2<0> { };

}

#endif // include guard
