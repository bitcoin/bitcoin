/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
Copyright (c) 1999-2006: Cortex Software GmbH, Kantstrasse 57, Berlin
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_VALUE_SIZE_HPP_JOFA_081004
#define BOOST_ICL_VALUE_SIZE_HPP_JOFA_081004

namespace boost{ namespace icl
{

template <typename Type>
Type abs(Type val) { return val < 0 ? -val : val; }

/// static class template for the size of a type's value
/** This function is needed to be able to order values according
    to their size. This is used to e.g. prefer simple test
    instances and to express this simplicity independent of the
    type of the test case.

    @author  Joachim Faulhaber
*/
template <class Type>
struct value_size
{
    /** The size of a value is used to be able to order values according to
        their simplicity */
    static std::size_t apply(const Type& val);
};


template<> inline std::size_t value_size<int>::apply(const int& value)
{ return abs(value); }

template<> inline std::size_t value_size<double>::apply(const double& value)
{ return static_cast<int>(abs(value)); }

template <typename Type>
inline std::size_t value_size<Type>::apply(const Type& value)
{ return icl::iterative_size(value); }



}} // namespace boost icl

#endif
