/*
 [auto_generated]
 boost/numeric/odeint/external/viennacl/viennacl_resize.hpp

 [begin_description]
 Enable resizing for viennacl vector.
 [end_description]

 Copyright 2012 Denis Demidov
 Copyright 2012 Karsten Ahnert
 Copyright 2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_VIENNACL_VIENNACL_RESIZE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_VIENNACL_VIENNACL_RESIZE_HPP_INCLUDED

#include <viennacl/vector.hpp>

#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resize.hpp>
#include <boost/numeric/odeint/util/same_size.hpp>

namespace boost {
namespace numeric {
namespace odeint {



/*
 * specializations for viennacl::vector< T >
 */
template< typename T >
struct is_resizeable< viennacl::vector< T > > : boost::true_type { };

template< typename T >
struct resize_impl< viennacl::vector< T > , viennacl::vector< T > >
{
    static void resize( viennacl::vector< T > &x1 , const viennacl::vector< T > &x2 )
    {
        x1.resize( x2.size() , false );
    }
};

template< typename T >
struct same_size_impl< viennacl::vector< T > , viennacl::vector< T > >
{
    static bool same_size( const viennacl::vector< T > &x1 , const viennacl::vector< T > &x2 )
    {
        return x1.size() == x2.size();
    }
};



} // namespace odeint
} // namespace numeric
} // namespace boost



#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_VIENNACL_VIENNACL_RESIZE_HPP_INCLUDED
