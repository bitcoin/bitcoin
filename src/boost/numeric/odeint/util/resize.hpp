/*
 [auto_generated]
 boost/numeric/odeint/util/state_wrapper.hpp

 [begin_description]
 State wrapper for the state type in all stepper. The state wrappers are responsible for construction,
 destruction, copying construction, assignment and resizing.
 [end_description]

 Copyright 2011-2013 Karsten Ahnert
 Copyright 2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_UTIL_RESIZE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_UTIL_RESIZE_HPP_INCLUDED

#include <boost/range.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/zip_view.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/make_fused.hpp>
#include <boost/fusion/include/for_each.hpp>

#include <boost/numeric/odeint/util/is_resizeable.hpp>

namespace boost {
namespace numeric {
namespace odeint {


template< class StateOut , class StateIn , class Enabler = void >
struct resize_impl_sfinae
{
    static void resize( StateOut &x1 , const StateIn &x2 )
    {
        x1.resize( boost::size( x2 ) );
    }
};

// resize function
// standard implementation relies on boost.range and resize member function
template< class StateOut , class StateIn >
struct resize_impl
{
    static void resize( StateOut &x1 , const StateIn &x2 )
    {
        resize_impl_sfinae< StateOut , StateIn >::resize( x1 , x2 );
    }
};


// do not overload or specialize this function, specialize resize_impl<> instead
template< class StateOut , class StateIn >
void resize( StateOut &x1 , const StateIn &x2 )
{
    resize_impl< StateOut , StateIn >::resize( x1 , x2 );
}


namespace detail {

    struct resizer
    {
        typedef void result_type;

        template< class StateOut , class StateIn >
        void operator()( StateOut &x1 , const StateIn &x2 ) const
        {
            resize_op( x1 , x2 , typename is_resizeable< StateOut >::type() );
        }

        template< class StateOut , class StateIn >
        void resize_op( StateOut &x1 , const StateIn &x2 , boost::true_type ) const
        {
            resize( x1 , x2 );
        }

        template< class StateOut , class StateIn >
        void resize_op( StateOut &/*x1*/ , const StateIn &/*x2*/ , boost::false_type ) const
        {
        }

    };
} // namespace detail


/*
 * specialization for fusion sequences
 */
template< class FusionSeq >
struct resize_impl_sfinae< FusionSeq , FusionSeq ,
    typename boost::enable_if< typename boost::fusion::traits::is_sequence< FusionSeq >::type >::type >
{
    static void resize( FusionSeq &x1 , const FusionSeq &x2 )
    {
        typedef boost::fusion::vector< FusionSeq& , const FusionSeq& > Sequences;
        Sequences sequences( x1 , x2 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( detail::resizer() ) );
    }
};




}
}
}



#endif // BOOST_NUMERIC_ODEINT_UTIL_RESIZE_HPP_INCLUDED
