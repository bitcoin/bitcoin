/*
 [auto_generated]
 boost/numeric/odeint/util/split.hpp

 [begin_description]
 Split abstraction for parallel backends.
 [end_description]

 Copyright 2013 Karsten Ahnert
 Copyright 2013 Mario Mulansky
 Copyright 2013 Pascal Germroth

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_UTIL_SPLIT_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_UTIL_SPLIT_HPP_INCLUDED

namespace boost {
namespace numeric {
namespace odeint {

/*
 * No default implementation of the split operation
 */
template< class Container1, class Container2 , class Enabler = void >
struct split_impl
{
    static void split( const Container1 &from , Container2 &to );
};

template< class Container1 , class Container2 >
void split( const Container1 &from , Container2 &to )
{
    split_impl< Container1 , Container2 >::split( from , to );
}


/*
 * No default implementation of the unsplit operation
 */
template< class Container1, class Container2 , class Enabler = void >
struct unsplit_impl
{
    static void unsplit( const Container1 &from , Container2 &to );
};

template< class Container1 , class Container2 >
void unsplit( const Container1 &from , Container2 &to )
{
    unsplit_impl< Container1 , Container2 >::unsplit( from , to );
}


} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_UTIL_COPY_HPP_INCLUDED

