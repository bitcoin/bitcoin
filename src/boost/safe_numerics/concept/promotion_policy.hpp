#ifndef BOOST_NUMERIC_CONCEPT_PROMOTION_POLICY_HPP
#define BOOST_NUMERIC_CONCEPT_PROMOTION_POLICY_HPP

//  Copyright (c) 2015 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

namespace boost {
namespace safe_numerics {

template<class PP>
struct PromotionPolicy {
    using T = int;
    using U = int;
    using a_type = typename PP::template addition_result<T, U>;
    using s_type = typename PP::template subtraction_result<T, U>;
    using m_type = typename PP::template multiplication_result<T, U>;
    using d_type = typename PP::template division_result<T, U>;
    using mod_type = typename PP::template modulus_result<T, U>;
    using ls_type = typename PP::template left_shift_result<T, U>;
    using rs_type = typename PP::template right_shift_result<T, U>;
    using cc_type = typename PP::template comparison_result<T, U>;
    using baw_type = typename PP::template bitwise_and_result<T, U>;
    using bow_type = typename PP::template bitwise_or_result<T, U>;
    using bxw_type = typename PP::template bitwise_xor_result<T, U>;
};

} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_CONCEPT_EXCEPTION_POLICY_HPP
