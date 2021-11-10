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


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_MTL4_RESIZE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_MTL4_RESIZE_HPP_INCLUDED

#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resize.hpp>
#include <boost/numeric/odeint/util/same_size.hpp>

#include <boost/numeric/mtl/vector/dense_vector.hpp>
#include <boost/numeric/mtl/matrix/dense2D.hpp>
#include <boost/numeric/mtl/matrix/compressed2D.hpp>


namespace boost {
namespace numeric {
namespace odeint {


template< class Value , class Parameters >
struct is_resizeable< mtl::dense_vector< Value , Parameters > >
{ 
    typedef boost::true_type type;
    const static bool value = type::value;
};

template< class Value , class Parameters >
struct is_resizeable< mtl::dense2D< Value , Parameters > >
{
    typedef boost::true_type type;
    const static bool value = type::value;
};

template< class Value , class Parameters >
struct is_resizeable< mtl::compressed2D< Value , Parameters > >
{
    typedef boost::true_type type;
    const static bool value = type::value;
};




template< class Value , class Parameters >
struct same_size_impl< mtl::dense_vector< Value , Parameters > , mtl::dense_vector< Value , Parameters > >
{
    static bool same_size( const mtl::dense_vector< Value , Parameters > &v1 ,
                           const mtl::dense_vector< Value , Parameters > &v2 )
    {
        return mtl::size( v1 ) == mtl::size( v2 );
    }
};

template< class Value , class Parameters >
struct resize_impl< mtl::dense_vector< Value , Parameters > , mtl::dense_vector< Value , Parameters > >
{
    static void resize( mtl::dense_vector< Value , Parameters > &v1 ,
                        const mtl::dense_vector< Value , Parameters > &v2 )
    {
        v1.change_dim( mtl::size( v2 ) );
    }
};



template< class Value , class MatrixParameters , class VectorParameters >
struct same_size_impl< mtl::dense2D< Value , MatrixParameters > , mtl::dense_vector< Value , VectorParameters > >
{
    static bool same_size( const mtl::dense2D< Value , MatrixParameters > &m , 
                           const mtl::dense_vector< Value , VectorParameters > &v )
    {
        return ( ( mtl::size( v ) == m.num_cols() ) && ( mtl::size( v ) == m.num_rows() ) );
    }
};

template< class Value , class MatrixParameters , class VectorParameters >
struct resize_impl< mtl::dense2D< Value , MatrixParameters > , mtl::dense_vector< Value , VectorParameters > >
{
    static void resize( mtl::dense2D< Value , MatrixParameters > &m , 
                        const mtl::dense_vector< Value , VectorParameters > &v )
    {
        m.change_dim( mtl::size( v ) , mtl::size( v ) , false );
    }
};




template< class Value , class MatrixParameters , class VectorParameters >
struct same_size_impl< mtl::compressed2D< Value , MatrixParameters > , mtl::dense_vector< Value , VectorParameters > >
{
    static bool same_size( const mtl::compressed2D< Value , MatrixParameters > &m , 
                           const mtl::dense_vector< Value , VectorParameters > &v )
    {
        return ( ( mtl::size( v ) == m.num_cols() ) && ( mtl::size( v ) == m.num_rows() ) );
    }
};

template< class Value , class MatrixParameters , class VectorParameters >
struct resize_impl< mtl::compressed2D< Value , MatrixParameters > , mtl::dense_vector< Value , VectorParameters > >
{
    static void resize( mtl::compressed2D< Value , MatrixParameters > &m , 
                        const mtl::dense_vector< Value , VectorParameters > &v )
    {
        m.change_dim( mtl::size( v ) , mtl::size( v ) );
    }
};








} // namespace odeint
} // namespace numeric
} // namespace boost

#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_MTL4_RESIZE_HPP_INCLUDED
