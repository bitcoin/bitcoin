/*
[begin_description]
Modification of the implicit Euler method, works with the MTL4 matrix library only. 
[end_description]

Copyright 2012-2013 Andreas Angelopoulos
Copyright 2012-2013 Karsten Ahnert
Copyright 2012-2013 Mario Mulansky

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_IMPLICIT_EULER_MTL4_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_IMPLICIT_EULER_MTL4_HPP_INCLUDED


#include <utility>

#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

#include <boost/numeric/odeint/external/mtl4/mtl4_resize.hpp>

#include <boost/numeric/mtl/mtl.hpp>
#include <boost/numeric/itl/itl.hpp>




namespace boost {
namespace numeric {
namespace odeint {


template< class ValueType , class Resizer = initially_resizer >
class implicit_euler_mtl4
{

public:

    typedef ValueType value_type;
    typedef value_type time_type;
    typedef mtl::dense_vector<value_type> state_type;

    typedef state_wrapper< state_type > wrapped_state_type;
    typedef state_type deriv_type;
    typedef state_wrapper< deriv_type > wrapped_deriv_type;
    typedef mtl::compressed2D< value_type > matrix_type;
    typedef state_wrapper< matrix_type > wrapped_matrix_type;

    typedef Resizer resizer_type;
    typedef stepper_tag stepper_category;

    typedef implicit_euler_mtl4< ValueType , Resizer > stepper_type;


    implicit_euler_mtl4( const value_type epsilon = 1E-6 )
        : m_epsilon( epsilon ) , m_resizer() ,
          m_dxdt() , m_x() ,
          m_identity() , m_jacobi()
    { }


    template< class System >
    void do_step( System system , state_type &x , time_type t , time_type dt )
    {
        typedef typename odeint::unwrap_reference< System >::type system_type;
        typedef typename odeint::unwrap_reference< typename system_type::first_type >::type deriv_func_type;
        typedef typename odeint::unwrap_reference< typename system_type::second_type >::type jacobi_func_type;
        system_type &sys = system;
        deriv_func_type &deriv_func = sys.first;
        jacobi_func_type &jacobi_func = sys.second;

        m_resizer.adjust_size( x , detail::bind(
                                   &stepper_type::template resize_impl< state_type > , detail::ref( *this ) , detail::_1 ) );

        m_identity.m_v = 1;

        t += dt;
        m_x.m_v = x;

        deriv_func( x , m_dxdt.m_v , t );
        jacobi_func( x , m_jacobi.m_v , t );


        m_dxdt.m_v *= -dt;

        m_jacobi.m_v *= dt;
        m_jacobi.m_v -= m_identity.m_v ;



        // using ilu_0 preconditioning -incomplete LU factorisation
        // itl::pc::diagonal<matrix_type,double> L(m_jacobi.m_v);
        itl::pc::ilu_0<matrix_type> L( m_jacobi.m_v );

        solve( m_jacobi.m_v , m_x.m_v , m_dxdt.m_v , L );
        x+= m_x.m_v;


    }


    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_impl( x );
    }


private:


    /*
      Applying approximate iterative linear solvers
      default solver is Biconjugate gradient stabilized method
      itl::bicgstab(A, x, b, L, iter);
    */
    template < class LinearOperator, class HilbertSpaceX, class HilbertSpaceB, class Preconditioner>
    void solve(const LinearOperator& A, HilbertSpaceX& x, const HilbertSpaceB& b,
               const Preconditioner& L, int max_iteractions =500)
    {
        // Termination criterion: r < 1e-6 * b or N iterations
        itl::basic_iteration< double > iter( b , max_iteractions , 1e-6 );
        itl::bicgstab( A , x , b , L , iter );

    }


    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        bool resized = false;
        resized |= adjust_size_by_resizeability( m_dxdt , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_x , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_identity , x , typename is_resizeable<matrix_type>::type() );
        resized |= adjust_size_by_resizeability( m_jacobi , x , typename is_resizeable<matrix_type>::type() );
        return resized;
    }


private:

    value_type m_epsilon;
    resizer_type m_resizer;
    wrapped_deriv_type m_dxdt;
    wrapped_state_type m_x;
    wrapped_matrix_type m_identity;
    wrapped_matrix_type m_jacobi;
};


} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_IMPLICIT_EULER_MTL4_HPP_INCLUDED
