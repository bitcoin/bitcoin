/*
  [auto_generated]
  boost/numeric/odeint/external/blaze/blaze_resize.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2009-2012 Karsten Ahnert
  Copyright 2009-2012 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_BLAZE_BLAZE_RESIZE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_BLAZE_BLAZE_RESIZE_HPP_INCLUDED

#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resize.hpp>
#include <boost/numeric/odeint/util/same_size.hpp>

#include <blaze/math/dense/DynamicVector.h>

namespace boost {
namespace numeric {
namespace odeint {

template< typename T , bool TF >
struct is_resizeable< blaze::DynamicVector< T , TF > > 
{ 
    typedef boost::true_type type;
    const static bool value = type::value;
};

template< typename T1 , bool TF1, typename T2 , bool TF2 >
struct same_size_impl< blaze::DynamicVector< T1 , TF1 > , blaze::DynamicVector< T2 , TF2 > >
{
    static bool same_size( const blaze::DynamicVector< T1 , TF1 > &x1 , const blaze::DynamicVector< T2 , TF2 > &x2 )
    {
        return x1.size() == x2.size();
    }
};

template< typename T1 , bool TF1, typename T2 , bool TF2 >
struct resize_impl< blaze::DynamicVector< T1 , TF1 > , blaze::DynamicVector< T2 , TF2 > >
{
    static void resize( blaze::DynamicVector< T1 , TF1 > &x1 , const blaze::DynamicVector< T2 , TF2 > &x2 )
    {
        x1.resize( x2.size() );
    }
};


} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_BLAZE_BLAZE_RESIZE_HPP_INCLUDED
