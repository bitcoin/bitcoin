/*
 boost/numeric/odeint/stepper/detail/pid_step_adjuster_coefficients.hpp

 [begin_description]
 Coefficients for the PID stepsize controller.
 [end_description]

 Copyright 2017 Valentin Noah Hartmann

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_ODEINT_STEPPER_DETAIL_PID_STEP_ADJUSTER_COEFFICIENTS_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_DETAIL_PID_STEP_ADJUSTER_COEFFICIENTS_HPP_INCLUDED

#include <boost/array.hpp>

namespace boost {
namespace numeric {
namespace odeint {
namespace detail {

enum adjuster_type{
    BASIC,
    H0211,
    H211b,
    H211PI,
    H0312,
    H312b,
    H312PID,
    H0321,
    H321
};

template<int Type>
class pid_step_adjuster_coefficients;

template<>
class pid_step_adjuster_coefficients<BASIC> : public boost::array<double, 5>
{
public:
    pid_step_adjuster_coefficients()
    : boost::array<double, 5>()
    {
        (*this)[0] = 1.0;
        (*this)[1] = 0.0;
        (*this)[2] = 0.0;
        (*this)[3] = 0.0;
        (*this)[4] = 0.0;
    }
};

template<>
class pid_step_adjuster_coefficients<H0211> : public boost::array<double, 5>
{
public:
    pid_step_adjuster_coefficients()
    : boost::array<double, 5>()
    {
        (*this)[0] = 1.0 / 2.0;
        (*this)[1] = 1.0 / 2.0;
        (*this)[2] = 0.0;
        (*this)[3] = 1.0 / 2.0;
        (*this)[4] = 0.0;
    }
};

template<>
class pid_step_adjuster_coefficients<H211b> : public boost::array<double, 5>
{
public:
    pid_step_adjuster_coefficients()
    : boost::array<double, 5>()
    {
        (*this)[0] = 1.0 / 5.0;
        (*this)[1] = 2.0 / 5.0;
        (*this)[2] = 0.0;
        (*this)[3] = 1.0 / 5.0;
        (*this)[4] = 0.0;
    }
};

template<>
class pid_step_adjuster_coefficients<H211PI> : public boost::array<double, 5>
{
public:
    pid_step_adjuster_coefficients()
    : boost::array<double, 5>()
    {
        (*this)[0] = 1.0 / 6.0;
        (*this)[1] = 2.0 / 6.0;
        (*this)[2] = 0.0;
        (*this)[3] = 0.0;
        (*this)[4] = 0.0;
    }
};

template<>
class pid_step_adjuster_coefficients<H0312> : public boost::array<double, 5>
{
public:
    pid_step_adjuster_coefficients()
    : boost::array<double, 5>()
    {
        (*this)[0] = 1.0 / 4.0;
        (*this)[1] = 2.0 / 2.0;
        (*this)[2] = 1.0 / 4.0;
        (*this)[3] = 3.0 / 4.0;
        (*this)[4] = 1.0 / 4.0;
    }
};

template<>
class pid_step_adjuster_coefficients<H312b> : public boost::array<double, 5>
{
public:
    pid_step_adjuster_coefficients()
    : boost::array<double, 5>()
    {
        (*this)[0] = 1.0 / 6.0;
        (*this)[1] = 2.0 / 6.0;
        (*this)[2] = 1.0 / 6.0;
        (*this)[3] = 3.0 / 6.0;
        (*this)[4] = 1.0 / 6.0;
    }
};

template<>
class pid_step_adjuster_coefficients<H312PID> : public boost::array<double, 5>
{
public:
    pid_step_adjuster_coefficients()
    : boost::array<double, 5>()
    {
        (*this)[0] = 1.0 / 18.0;
        (*this)[1] = 2.0 / 9.0;
        (*this)[2] = 1.0 / 18.0;
        (*this)[3] = 0.0;
        (*this)[4] = 0.0;
    }
};

template<>
class pid_step_adjuster_coefficients<H0321> : public boost::array<double, 5>
{
public:
    pid_step_adjuster_coefficients()
    : boost::array<double, 5>()
    {
        (*this)[0] =  5.0 / 4.0;
        (*this)[1] =  1.0 / 2.0;
        (*this)[2] = -3.0 / 4.0;
        (*this)[3] = -1.0 / 4.0;
        (*this)[4] = -3.0 / 4.0;
    }
};

template<>
class pid_step_adjuster_coefficients<H321> : public boost::array<double, 5>
{
public:
    pid_step_adjuster_coefficients()
    : boost::array<double, 5>()
    {
        (*this)[0] =  1.0 / 3.0;
        (*this)[1] =  1.0 / 18.0;
        (*this)[2] = -5.0 / 18.0;
        (*this)[3] = -5.0 / 16.0;
        (*this)[4] = -1.0 / 6.0;
    }
};

} // detail
} // odeint
} // numeric
} // boost

#endif