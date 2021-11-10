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


#ifndef BOOST_NUMERIC_ODEINT_UTIL_SAME_SIZE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_UTIL_SAME_SIZE_HPP_INCLUDED

#include <boost/numeric/odeint/util/is_resizeable.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/zip_view.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/make_fused.hpp>
#include <boost/fusion/include/all.hpp>

#include <boost/range.hpp>


namespace boost {
namespace numeric {
namespace odeint {
    
template< typename State1 , typename State2 , class Enabler = void >
struct same_size_impl_sfinae
{
    static bool same_size( const State1 &x1 , const State2 &x2 )
    {
        return ( boost::size( x1 ) == boost::size( x2 ) );
    }

};

// same_size function
// standard implementation relies on boost.range
template< class State1 , class State2 >
struct same_size_impl
{
    static bool same_size( const State1 &x1 , const State2 &x2 )
    {
        return same_size_impl_sfinae< State1 , State2 >::same_size( x1 , x2 );
    }
};


// do not overload or specialize this function, specialize resize_impl<> instead
template< class State1 , class State2 >
bool same_size( const State1 &x1 , const State2 &x2 )
{
    return same_size_impl< State1 , State2 >::same_size( x1 , x2 );
}

namespace detail {

struct same_size_fusion
{
    typedef bool result_type;

    template< class S1 , class S2 >
    bool operator()( const S1 &x1 , const S2 &x2 ) const
    {
        return same_size_op( x1 , x2 , typename is_resizeable< S1 >::type() );
    }

    template< class S1 , class S2 >
    bool same_size_op( const S1 &x1 , const S2 &x2 , boost::true_type ) const
    {
        return same_size( x1 , x2 );
    }

    template< class S1 , class S2 >
    bool same_size_op( const S1 &/*x1*/ , const S2 &/*x2*/ , boost::false_type ) const
    {
        return true;
    }
};

} // namespace detail



template< class FusionSeq >
struct same_size_impl_sfinae< FusionSeq , FusionSeq , typename boost::enable_if< typename boost::fusion::traits::is_sequence< FusionSeq >::type >::type >
{
    static bool same_size( const FusionSeq &x1 , const FusionSeq &x2 )
    {
        typedef boost::fusion::vector< const FusionSeq& , const FusionSeq& > Sequences;
        Sequences sequences( x1 , x2 );
        return boost::fusion::all( boost::fusion::zip_view< Sequences >( sequences ) ,
                                   boost::fusion::make_fused( detail::same_size_fusion() ) );
    }
};


}
}
}



#endif // BOOST_NUMERIC_ODEINT_UTIL_SAME_SIZE_HPP_INCLUDED
