/*
 [auto_generated]
 boost/numeric/odeint/config.hpp

 [begin_description]
 Sets configurations for odeint and used libraries. Should be included before any other odeint library
 [end_description]

 Copyright 2011-2012 Mario Mulansky
 Copyright 2011-2012 Karsten Ahnert

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_ODEINT_CONFIG_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_CONFIG_HPP_INCLUDED


//increase macro variable to allow rk78 scheme
#ifndef FUSION_MAX_VECTOR_SIZE
#define FUSION_MAX_VECTOR_SIZE 15
#endif


/*
 * the following definitions are only required if fusion vectors are used as state types
 * in the rk78 scheme
 * they should be defined by the user if required, see e.g. libs/numeric/examples/harmonic_oscillator_units.cpp
 */
#ifndef BOOST_FUSION_INVOKE_MAX_ARITY
#define BOOST_FUSION_INVOKE_MAX_ARITY 15
#endif

#ifndef BOOST_RESULT_OF_NUM_ARGS
#define BOOST_RESULT_OF_NUM_ARGS 15
#endif





#include <boost/config.hpp>

#if __cplusplus >= 201103L
#define BOOST_NUMERIC_ODEINT_CXX11 1
#endif




#endif // BOOST_NUMERIC_ODEINT_CONFIG_HPP_INCLUDED
