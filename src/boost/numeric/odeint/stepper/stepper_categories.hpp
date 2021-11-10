/*
 [auto_generated]
 boost/numeric/odeint/stepper/stepper_categories.hpp

 [begin_description]
 Definition of all stepper categories.
 [end_description]

 Copyright 2010-2011 Mario Mulansky
 Copyright 2010-2012 Karsten Ahnert

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_STEPPER_CATEGORIES_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_STEPPER_CATEGORIES_HPP_INCLUDED

#include <boost/type_traits/integral_constant.hpp>

namespace boost {
namespace numeric {
namespace odeint {


/*
 * Tags to specify stepper types
 *
 * These tags are used by integrate() to choose which integration method is used
 */

struct stepper_tag {};
// struct explicit_stepper_tag : stepper_tag {};
// struct implicit_stepper_tag : stepper_tag {};


struct error_stepper_tag : stepper_tag {};
struct explicit_error_stepper_tag : error_stepper_tag {};
struct explicit_error_stepper_fsal_tag : error_stepper_tag {};

struct controlled_stepper_tag {};
struct explicit_controlled_stepper_tag : controlled_stepper_tag {};
struct explicit_controlled_stepper_fsal_tag : controlled_stepper_tag {};

struct dense_output_stepper_tag {};


template< class tag > struct base_tag ;
template< > struct base_tag< stepper_tag > { typedef stepper_tag type; };
template< > struct base_tag< error_stepper_tag > { typedef stepper_tag type; };
template< > struct base_tag< explicit_error_stepper_tag > { typedef stepper_tag type; };
template< > struct base_tag< explicit_error_stepper_fsal_tag > { typedef stepper_tag type; };

template< > struct base_tag< controlled_stepper_tag > { typedef controlled_stepper_tag type; };
template< > struct base_tag< explicit_controlled_stepper_tag > { typedef controlled_stepper_tag type; };
template< > struct base_tag< explicit_controlled_stepper_fsal_tag > { typedef controlled_stepper_tag type; };

template< > struct base_tag< dense_output_stepper_tag > { typedef dense_output_stepper_tag type; };


} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_STEPPER_CATEGORIES_HPP_INCLUDED
