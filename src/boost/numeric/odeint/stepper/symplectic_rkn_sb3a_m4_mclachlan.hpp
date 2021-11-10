/*
  [auto_generated]
  boost/numeric/odeint/stepper/symplectic_rkn_sb3a_m4_mclachlan.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2012-2013 Karsten Ahnert
  Copyright 2012-2013 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_SYMPLECTIC_RKN_SB3A_M4_MCLACHLAN_HPP_DEFINED
#define BOOST_NUMERIC_ODEINT_STEPPER_SYMPLECTIC_RKN_SB3A_M4_MCLACHLAN_HPP_DEFINED

#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>

#include <boost/numeric/odeint/util/resizer.hpp>


namespace boost {
namespace numeric {
namespace odeint {

#ifndef DOXYGEN_SKIP
namespace detail {
namespace symplectic_rkn_sb3a_m4_mclachlan {

    /*
      exp( a1 t A ) exp( b1 t B )
      exp( a2 t A ) exp( b2 t B )
      exp( a3 t A )
      exp( b2 t B ) exp( a2 t A )
      exp( b1 t B ) exp( a1 t A )
    */



    template< class Value >
    struct coef_a_type : public boost::array< Value , 5 >
    {
        coef_a_type( void )
        {
            using std::sqrt;

            Value z = sqrt( static_cast< Value >( 7 ) / static_cast< Value >( 8 ) ) / static_cast< Value >( 3 );
            (*this)[0] = static_cast< Value >( 1 ) / static_cast< Value >( 2 ) - z ;
            (*this)[1] = static_cast< Value >( -1 ) / static_cast< Value >( 3 ) + z ;
            (*this)[2] = static_cast< Value >( 2 ) / static_cast< Value >( 3 );
            (*this)[3] = (*this)[1];
            (*this)[4] = (*this)[0];
        }
    };

    template< class Value >
    struct coef_b_type : public boost::array< Value , 5 >
    {
        coef_b_type( void )
        {
            (*this)[0] = static_cast< Value >( 1 );
            (*this)[1] = static_cast< Value >( -1 ) / static_cast< Value >( 2 );
            (*this)[2] = (*this)[1];
            (*this)[3] = (*this)[0];
            (*this)[4] = static_cast< Value >( 0 );
        }
    };

} // namespace symplectic_rkn_sb3a_m4_mclachlan
} // namespace detail
#endif // DOXYGEN_SKIP




template<
    class Coor ,
    class Momentum = Coor ,
    class Value = double ,
    class CoorDeriv = Coor ,
    class MomentumDeriv = Coor ,
    class Time = Value ,
    class Algebra = typename algebra_dispatcher< Coor >::algebra_type ,
    class Operations = typename operations_dispatcher< Coor >::operations_type ,
    class Resizer = initially_resizer
    >
#ifndef DOXYGEN_SKIP
class symplectic_rkn_sb3a_m4_mclachlan :
        public symplectic_nystroem_stepper_base
<
    5 , 4 ,
    Coor , Momentum , Value , CoorDeriv , MomentumDeriv , Time , Algebra , Operations , Resizer
    >
#else
class symplectic_rkn_sb3a_m4_mclachlan : public symplectic_nystroem_stepper_base
#endif
{
public:
#ifndef DOXYGEN_SKIP
    typedef symplectic_nystroem_stepper_base
    <
    5 , 4 ,
    Coor , Momentum , Value , CoorDeriv , MomentumDeriv , Time , Algebra , Operations , Resizer
    > stepper_base_type;
#endif
    typedef typename stepper_base_type::algebra_type algebra_type;
    typedef typename stepper_base_type::value_type value_type;


    symplectic_rkn_sb3a_m4_mclachlan( const algebra_type &algebra = algebra_type() )
        : stepper_base_type(
            detail::symplectic_rkn_sb3a_m4_mclachlan::coef_a_type< value_type >() ,
            detail::symplectic_rkn_sb3a_m4_mclachlan::coef_b_type< value_type >() ,
            algebra )
    { }
};


/***************** DOXYGEN ***************/

/**
 * \class symplectic_rkn_sb3a_m4_mclachlan
 * \brief Implementation of the symmetric B3A Runge-Kutta Nystroem method of fifth order.
 *
 * The method is of fourth order and has five stages. It is described HERE. This method can be used
 * with multiprecision types since the coefficients are defined analytically.
 *
 * ToDo: add reference to paper.
 *
 * \tparam Order The order of the stepper.
 * \tparam Coor The type representing the coordinates q.
 * \tparam Momentum The type representing the coordinates p.
 * \tparam Value The basic value type. Should be something like float, double or a high-precision type.
 * \tparam CoorDeriv The type representing the time derivative of the coordinate dq/dt.
 * \tparam MomemtnumDeriv The type representing the time derivative of the momentum dp/dt.
 * \tparam Time The type representing the time t.
 * \tparam Algebra The algebra.
 * \tparam Operations The operations.
 * \tparam Resizer The resizer policy.
 */

    /**
     * \fn symplectic_rkn_sb3a_m4_mclachlan::symplectic_rkn_sb3a_m4_mclachlan( const algebra_type &algebra )
     * \brief Constructs the symplectic_rkn_sb3a_m4_mclachlan. This constructor can be used as a default
     * constructor if the algebra has a default constructor.
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */

} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_SYMPLECTIC_RKN_SB3A_M4_MCLACHLAN_HPP_DEFINED
