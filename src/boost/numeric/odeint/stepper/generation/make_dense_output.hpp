/*
 [auto_generated]
 boost/numeric/odeint/stepper/generation/make_dense_output.hpp

 [begin_description]
 Factory function to simplify the creation of dense output steppers from error steppers.
 [end_description]

 Copyright 2011-2012 Karsten Ahnert
 Copyright 2011-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_MAKE_DENSE_OUTPUT_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_MAKE_DENSE_OUTPUT_HPP_INCLUDED

namespace boost {
namespace numeric {
namespace odeint {


// default template for the dense output
template< class Stepper > struct get_dense_output { };



// default dense output factory
template< class Stepper , class DenseOutput >
struct dense_output_factory
{
    DenseOutput operator()(
            typename Stepper::value_type abs_error ,
            typename Stepper::value_type rel_error ,
            const Stepper &stepper )
    {
        return DenseOutput( abs_error , rel_error , stepper );
    }

    DenseOutput operator()(
            typename Stepper::value_type abs_error ,
            typename Stepper::value_type rel_error ,
            typename Stepper::time_type max_dt ,
            const Stepper &stepper )
    {
        return DenseOutput( abs_error , rel_error , max_dt , stepper );
    }
};



namespace result_of
{
    template< class Stepper >
    struct make_dense_output
    {
        typedef typename get_dense_output< Stepper >::type type;
    };
}



template< class Stepper >
typename result_of::make_dense_output< Stepper >::type make_dense_output(
        typename Stepper::value_type abs_error ,
        typename Stepper::value_type rel_error ,
        const Stepper &stepper = Stepper() )
{
    typedef Stepper stepper_type;
    typedef typename result_of::make_dense_output< stepper_type >::type dense_output_type;
    typedef dense_output_factory< stepper_type , dense_output_type > factory_type;
    factory_type factory;
    return factory( abs_error , rel_error , stepper );
}


template< class Stepper >
typename result_of::make_dense_output< Stepper >::type make_dense_output(
        typename Stepper::value_type abs_error ,
        typename Stepper::value_type rel_error ,
        typename Stepper::time_type max_dt ,
        const Stepper &stepper = Stepper() )
{
    typedef Stepper stepper_type;
    typedef typename result_of::make_dense_output< stepper_type >::type dense_output_type;
    typedef dense_output_factory< stepper_type , dense_output_type > factory_type;
    factory_type factory;
    return factory( abs_error , rel_error , max_dt, stepper );
}


} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_MAKE_DENSE_OUTPUT_HPP_INCLUDED
