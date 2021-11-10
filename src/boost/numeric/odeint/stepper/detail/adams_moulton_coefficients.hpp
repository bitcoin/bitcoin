/*
 [auto_generated]
 boost/numeric/odeint/stepper/detail/adams_moulton_coefficients.hpp

 [begin_description]
 Coefficients for the Adams Moulton method.
 [end_description]

 Copyright 2011-2012 Karsten Ahnert
 Copyright 2011-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_DETAIL_ADAMS_MOULTON_COEFFICIENTS_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_DETAIL_ADAMS_MOULTON_COEFFICIENTS_HPP_INCLUDED


#include <boost/array.hpp>


namespace boost {
namespace numeric {
namespace odeint {
namespace detail {

template< class Value , size_t Steps >
class adams_moulton_coefficients ;

template< class Value >
class adams_moulton_coefficients< Value , 1 > : public boost::array< Value , 1 >
{
public:
    adams_moulton_coefficients( void )
    : boost::array< Value , 1 >()
      {
        (*this)[0] = static_cast< Value >( 1 );
      }
};


template< class Value >
class adams_moulton_coefficients< Value , 2 > : public boost::array< Value , 2 >
{
public:
    adams_moulton_coefficients( void )
    : boost::array< Value , 2 >()
      {
        (*this)[0] = static_cast< Value >( 1 ) / static_cast< Value >( 2 );
        (*this)[1] = static_cast< Value >( 1 ) / static_cast< Value >( 2 );
      }
};


template< class Value >
class adams_moulton_coefficients< Value , 3 > : public boost::array< Value , 3 >
{
public:
    adams_moulton_coefficients( void )
    : boost::array< Value , 3 >()
      {
        (*this)[0] = static_cast< Value >( 5 ) / static_cast< Value >( 12 );
        (*this)[1] = static_cast< Value >( 2 ) / static_cast< Value >( 3 );
        (*this)[2] = -static_cast< Value >( 1 ) / static_cast< Value >( 12 );
      }
};


template< class Value >
class adams_moulton_coefficients< Value , 4 > : public boost::array< Value , 4 >
{
public:
    adams_moulton_coefficients( void )
    : boost::array< Value , 4 >()
      {
        (*this)[0] = static_cast< Value >( 3 ) / static_cast< Value >( 8 );
        (*this)[1] = static_cast< Value >( 19 ) / static_cast< Value >( 24 );
        (*this)[2] = -static_cast< Value >( 5 ) / static_cast< Value >( 24 );
        (*this)[3] = static_cast< Value >( 1 ) / static_cast< Value >( 24 );
      }
};


template< class Value >
class adams_moulton_coefficients< Value , 5 > : public boost::array< Value , 5 >
{
public:
    adams_moulton_coefficients( void )
    : boost::array< Value , 5 >()
      {
        (*this)[0] = static_cast< Value >( 251 ) / static_cast< Value >( 720 );
        (*this)[1] = static_cast< Value >( 323 ) / static_cast< Value >( 360 );
        (*this)[2] = -static_cast< Value >( 11 ) / static_cast< Value >( 30 );
        (*this)[3] = static_cast< Value >( 53 ) / static_cast< Value >( 360 );
        (*this)[4] = -static_cast< Value >( 19 ) / static_cast< Value >( 720 );
      }
};


template< class Value >
class adams_moulton_coefficients< Value , 6 > : public boost::array< Value , 6 >
{
public:
    adams_moulton_coefficients( void )
    : boost::array< Value , 6 >()
      {
        (*this)[0] = static_cast< Value >( 95 ) / static_cast< Value >( 288 );
        (*this)[1] = static_cast< Value >( 1427 ) / static_cast< Value >( 1440 );
        (*this)[2] = -static_cast< Value >( 133 ) / static_cast< Value >( 240 );
        (*this)[3] = static_cast< Value >( 241 ) / static_cast< Value >( 720 );
        (*this)[4] = -static_cast< Value >( 173 ) / static_cast< Value >( 1440 );
        (*this)[5] = static_cast< Value >( 3 ) / static_cast< Value >( 160 );
      }
};

template< class Value >
class adams_moulton_coefficients< Value , 7 > : public boost::array< Value , 7 >
{
public:
    adams_moulton_coefficients( void )
    : boost::array< Value , 7 >()
      {
        (*this)[0] = static_cast< Value >( 19087 ) / static_cast< Value >( 60480 );
        (*this)[1] = static_cast< Value >( 2713 ) / static_cast< Value >( 2520 );
        (*this)[2] = -static_cast< Value >( 15487 ) / static_cast< Value >( 20160 );
        (*this)[3] = static_cast< Value >( 586 ) / static_cast< Value >( 945 );
        (*this)[4] = -static_cast< Value >( 6737 ) / static_cast< Value >( 20160 );
        (*this)[5] = static_cast< Value >( 263 ) / static_cast< Value >( 2520 );
        (*this)[6] = -static_cast< Value >( 863 ) / static_cast< Value >( 60480 );
      }
};


template< class Value >
class adams_moulton_coefficients< Value , 8 > : public boost::array< Value , 8 >
{
public:
    adams_moulton_coefficients( void )
    : boost::array< Value , 8 >()
      {
        (*this)[0] = static_cast< Value >( 5257 ) / static_cast< Value >( 17280 );
        (*this)[1] = static_cast< Value >( 139849 ) / static_cast< Value >( 120960 );
        (*this)[2] = -static_cast< Value >( 4511 ) / static_cast< Value >( 4480 );
        (*this)[3] = static_cast< Value >( 123133 ) / static_cast< Value >( 120960 );
        (*this)[4] = -static_cast< Value >( 88547 ) / static_cast< Value >( 120960 );
        (*this)[5] = static_cast< Value >( 1537 ) / static_cast< Value >( 4480 );
        (*this)[6] = -static_cast< Value >( 11351 ) / static_cast< Value >( 120960 );
        (*this)[7] = static_cast< Value >( 275 ) / static_cast< Value >( 24192 );
      }
};







} // detail
} // odeint
} // numeric
} // boost



#endif // BOOST_NUMERIC_ODEINT_STEPPER_DETAIL_ADAMS_MOULTON_COEFFICIENTS_HPP_INCLUDED
