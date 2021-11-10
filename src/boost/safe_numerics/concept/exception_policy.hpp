#ifndef BOOST_NUMERIC_CONCEPT_EXCEPTION_POLICY_HPP
#define BOOST_NUMERIC_CONCEPT_EXCEPTION_POLICY_HPP

//  Copyright (c) 2015 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

namespace boost {
namespace safe_numerics {

template<class EP>
struct ExceptionPolicy {
    const char * message;
    /*
    BOOST_CONCEPT_USAGE(ExceptionPolicy){
        EP::on_arithmetic_error(e, message);
        EP::on_undefined_behavior(e, message)
        EP::on_implementation_defined_behavior(e, message)
        EP::on_uninitialized_value(e, message)
    }
    */
};

} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_CONCEPT_EXCEPTION_POLICY_HPP
