/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_CONCEPT_INTERVAL_BOUNDS_HPP_JOFA_100927
#define BOOST_ICL_CONCEPT_INTERVAL_BOUNDS_HPP_JOFA_100927

#include <boost/icl/interval_bounds.hpp>
#include <boost/icl/type_traits/is_discrete.hpp>
#include <boost/icl/type_traits/is_numeric.hpp>

namespace boost{namespace icl
{

inline interval_bounds left(interval_bounds x1)
{ return interval_bounds(x1._bits & interval_bounds::_left); }

inline interval_bounds right(interval_bounds x1)
{ return interval_bounds(x1._bits & interval_bounds::_right); }

inline interval_bounds all(interval_bounds x1)
{ return interval_bounds(x1._bits & interval_bounds::_all); }

inline bool operator == (const interval_bounds x1, const interval_bounds x2)
{ return x1._bits == x2._bits; }

inline bool operator != (const interval_bounds x1, const interval_bounds x2)
{ return x1._bits != x2._bits; }

inline interval_bounds operator & (interval_bounds x1, interval_bounds x2)
{ return interval_bounds(x1._bits & x2._bits); }

inline interval_bounds operator | (interval_bounds x1, interval_bounds x2)
{ return interval_bounds(x1._bits | x2._bits); }

// left shift (multiplies by 2^shift)
inline interval_bounds operator << (interval_bounds bounds, unsigned int shift)
{ return interval_bounds(bounds._bits << shift); }

// right shift (divides by 2^shift)
inline interval_bounds operator >> (interval_bounds bounds, unsigned int shift)
{ return interval_bounds(bounds._bits >> shift); }

inline interval_bounds operator ~ (interval_bounds x1)
{ return all(interval_bounds(~(x1._bits))); }

inline interval_bounds outer_bounds(interval_bounds x1, interval_bounds x2)
{ return left(x1) | right(x2); }

inline interval_bounds inner_bounds(interval_bounds x1, interval_bounds x2)
{ return interval_bounds(x1.reverse_right() | x2.reverse_left()); }

inline interval_bounds left_bounds(interval_bounds x1, interval_bounds x2)
{ return left(x1) | (left(x2) >> 1); }

inline interval_bounds right_bounds(interval_bounds x1, interval_bounds x2)
{ return (right(x1) <<1 ) | right(x2); }

inline interval_bounds left_subtract_bounds(interval_bounds x1, interval_bounds x2)
{ return right(x1) | ~(right(x2) << 1); }

inline interval_bounds right_subtract_bounds(interval_bounds x1, interval_bounds x2)
{ return left(x1) | ~(left(x2) >> 1); }

inline bool is_complementary(interval_bounds x1)
{ return x1 == interval_bounds::right_open() || x1 == interval_bounds::left_open(); }

inline bool is_left_closed(interval_bounds bounds)
{ return bounds.left().bits()==2; }

inline bool is_right_closed(interval_bounds bounds)
{ return bounds.right().bits()==1; }

inline std::string left_bracket(interval_bounds bounds)
{ return is_left_closed(bounds) ? "[" : "("; }

inline std::string right_bracket(interval_bounds bounds)
{ return is_right_closed(bounds) ? "]" : ")"; }

template <class Type>
inline typename enable_if<is_discrete<Type>, Type>::type
shift_lower(interval_bounds decl, interval_bounds repr, const Type& low)
{
    if(is_left_closed(decl) && !is_left_closed(repr))
        return icl::pred(low);
    else if(!is_left_closed(decl) && is_left_closed(repr)) 
        return icl::succ(low);
    else 
        return low;
}

template <class Type>
inline typename enable_if<is_discrete<Type>, Type>::type
shift_upper(interval_bounds decl, interval_bounds repr, const Type& up)
{
    if(!is_right_closed(decl) && is_right_closed(repr))
        return icl::pred(up);
    else if(is_right_closed(decl) && !is_right_closed(repr)) 
        return icl::succ(up);
    else 
        return up;
}

template<class CharType, class CharTraits>
std::basic_ostream<CharType, CharTraits>& operator <<
  (std::basic_ostream<CharType, CharTraits> &stream, 
   interval_bounds const& object)
{
    return stream << left_bracket(object) << right_bracket(object);
}



template<class IntervalT>
inline typename 
boost::enable_if<has_dynamic_bounds<IntervalT>, interval_bounds>::type
outer_bounds(const IntervalT& x1, const IntervalT& x2)
{ return outer_bounds(x1.bounds(), x2.bounds()); }

template<class IntervalT>
inline typename 
boost::enable_if<has_dynamic_bounds<IntervalT>, interval_bounds>::type
inner_bounds(const IntervalT& x1, const IntervalT& x2)
{ return inner_bounds(x1.bounds(), x2.bounds()); }

template<class IntervalT>
inline typename 
boost::enable_if<has_dynamic_bounds<IntervalT>, interval_bounds>::type
left_bounds(const IntervalT& x1, const IntervalT& x2)
{ return left_bounds(x1.bounds(), x2.bounds()); }

template<class IntervalT>
inline typename 
boost::enable_if<has_dynamic_bounds<IntervalT>, interval_bounds>::type
right_bounds(const IntervalT& x1, const IntervalT& x2)
{ return right_bounds(x1.bounds(), x2.bounds()); }

template<class IntervalT>
inline typename 
boost::enable_if<has_dynamic_bounds<IntervalT>, interval_bounds>::type
left_subtract_bounds(const IntervalT& x1, const IntervalT& x2)
{ return left_subtract_bounds(x1.bounds(), x2.bounds()); }

template<class IntervalT>
inline typename 
boost::enable_if<has_dynamic_bounds<IntervalT>, interval_bounds>::type
right_subtract_bounds(const IntervalT& x1, const IntervalT& x2)
{ return right_subtract_bounds(x1.bounds(), x2.bounds()); }


}} // namespace icl boost

#endif

