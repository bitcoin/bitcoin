/*
 [auto_generated]
 boost/numeric/odeint/util/same_instance.hpp

 [begin_description]
 Basic check if two variables are the same instance
 [end_description]

 Copyright 2012 Karsten Ahnert
 Copyright 2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_UTIL_SAME_INSTANCE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_UTIL_SAME_INSTANCE_HPP_INCLUDED

namespace boost {
namespace numeric {
namespace odeint {

template< class T1 , class T2 , class Enabler=void >
struct same_instance_impl
{ 
    static bool same_instance( const T1& /* x1 */ , const T2& /* x2 */ )
    {
        return false;
    }
};

template< class T >
struct same_instance_impl< T , T >
{ 
    static bool same_instance( const T &x1 , const T &x2 )
    {
        // check pointers
        return (&x1 == &x2);
    }
};


template< class T1 , class T2 >
bool same_instance( const T1 &x1 , const T2 &x2 )
{
    return same_instance_impl< T1 , T2 >::same_instance( x1 , x2 );
}


} // namespace odeint
} // namespace numeric
} // namespace boost

#endif
