/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_INTERVAL_BOUNDS_HPP_JOFA_100330
#define BOOST_ICL_INTERVAL_BOUNDS_HPP_JOFA_100330

#include <boost/utility/enable_if.hpp>
#include <boost/icl/detail/design_config.hpp>

namespace boost{namespace icl
{

typedef unsigned char bound_type;

class interval_bounds
{
public:
    BOOST_STATIC_CONSTANT(bound_type, static_open       = 0);
    BOOST_STATIC_CONSTANT(bound_type, static_left_open  = 1);
    BOOST_STATIC_CONSTANT(bound_type, static_right_open = 2);
    BOOST_STATIC_CONSTANT(bound_type, static_closed     = 3);
    BOOST_STATIC_CONSTANT(bound_type, dynamic           = 4);
    BOOST_STATIC_CONSTANT(bound_type, undefined         = 5);

    BOOST_STATIC_CONSTANT(bound_type, _open       = 0);
    BOOST_STATIC_CONSTANT(bound_type, _left_open  = 1);
    BOOST_STATIC_CONSTANT(bound_type, _right_open = 2);
    BOOST_STATIC_CONSTANT(bound_type, _closed     = 3);

    BOOST_STATIC_CONSTANT(bound_type, _right     = 1);
    BOOST_STATIC_CONSTANT(bound_type, _left      = 2);
    BOOST_STATIC_CONSTANT(bound_type, _all       = 3);

public:
    interval_bounds():_bits(){}
    explicit interval_bounds(bound_type bounds): _bits(bounds){}
    interval_bounds all  ()const { return interval_bounds(_bits & _all  ); }
    interval_bounds left ()const { return interval_bounds(_bits & _left ); }
    interval_bounds right()const { return interval_bounds(_bits & _right); }
    interval_bounds reverse_left ()const { return interval_bounds((bound_type(~_bits)>>1) & _right); }
    interval_bounds reverse_right()const { return interval_bounds((bound_type(~_bits)<<1) & _left ); }

    bound_type bits()const{ return _bits; }

    static interval_bounds open()      { return interval_bounds(_open);     }
    static interval_bounds left_open() { return interval_bounds(_left_open); }
    static interval_bounds right_open(){ return interval_bounds(_right_open);}
    static interval_bounds closed()    { return interval_bounds(_closed);   }

public:
    bound_type _bits;
};


template<class DomainT>
class bounded_value
{
public:
    typedef DomainT domain_type;
    typedef bounded_value<DomainT> type;
public:
    bounded_value(const domain_type& value, interval_bounds bound)
        : _value(value), _bound(bound) {}

    domain_type     value()const { return _value; }
    interval_bounds bound()const { return _bound; }

private:
    domain_type     _value;
    interval_bounds _bound;
};

}} // namespace icl boost

#endif

