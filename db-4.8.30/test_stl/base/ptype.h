/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#ifndef _DB_STL_PTYPE_H
#define _DB_STL_PTYPE_H

#include "dbstl_common.h"
#include "dbstl_element_ref.h"
#include <iostream>

using std::istream;
using std::ostream;
using namespace dbstl;
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// ptype class template definition
//
// ptype<> is a primitive types wrapper, must use this wrapper to 
// store/retrieve primitive types to/from db via db stl interface
//
template <Typename T>
class ptype 
{
public:
    T v;
    ptype(){v = 0;}
    ptype(T vv){v = vv;}
    typedef ptype<T> tptype;
    typedef db_vector_iterator<tptype> dvit; 

    operator T()
    { 
        return v;
    }

    ptype(const ElementRef<ptype<T> >&rval) 
    {   
        if ( rval._DB_STL_GetData(*this) != INVALID_KEY_DATA) {
            
        } else {    
            throw new InvalidDbtException();    
        }   
    }

    ptype(const ptype<T>&p2)
    {
        v = p2.v;
    }

    template <Typename T2>  
    ptype(const ptype<T2>&p2)
    {
        v = p2.v;
    }
    template <Typename T2>  
    const ptype<T>& operator=(const ptype<T2>&p2)
    {
        v = p2.v;
        return *this;
    }
    template <Typename T2>  
    const ptype<T> operator +(const ptype<T2> &p2) const 
    { 
        return ptype<T>(v + p2.v );
    }
    template <Typename T2>  
    const ptype<T> operator -(const ptype<T2> &p2) const 
    { 
        return ptype<T>(v - p2.v );
    }
    template <Typename T2>  
    const ptype<T> operator *(const ptype<T2> &p2) const 
    { 
        return ptype<T>(v * p2.v );
    }
    template <Typename T2>  
    const ptype<T> operator /(const ptype<T2> &p2) const 
    { 
        return ptype<T>(v / p2.v );
    }
    template <Typename T2>  
    const ptype<T> operator %(const ptype<T2> &p2) const 
    { 
        return ptype<T>(v % p2.v );
    }

    template <Typename T2>  
    const ptype<T>& operator +=(const ptype<T2> &p2) const 
    { 
        v += p2.v;
        return *this;
    }

    template <Typename T2>  
    const ptype<T>& operator -=(const ptype<T2> &p2) const 
    { 
        v -= p2.v;
        return *this;
    }
    template <Typename T2>  
    const ptype<T>& operator *=(const ptype<T2> &p2) const 
    { 
        v *= p2.v;
        return *this;
    }
    template <Typename T2>  
    const ptype<T>& operator /=(const ptype<T2> &p2) const 
    { 
        v /= p2.v;
        return *this;
    }
    template <Typename T2>  
    const ptype<T>& operator %=(const ptype<T2> &p2) const 
    { 
        v %= p2.v;
        return *this;
    }

    template <Typename T2>
    bool operator==(const ptype<T2>&p2) const 
    {
        return v == p2.v;
    }
    template <Typename T2>
    bool operator!=(const ptype<T2>&p2)const 
    {
        return v != p2.v;
    }

    template <Typename T2>
    bool operator<(const ptype<T2>&p2) const 
    {
        return v < p2.v;
    }

    template <Typename T2>
    bool operator>(const ptype<T2>&p2) const 
    {
        return v > p2.v;
    }

    template <Typename T2>
    bool operator<=(const ptype<T2>&p2) const 
    {
        return v <= p2.v;
    }
    template <Typename T2>
    bool operator>=(const ptype<T2>&p2) const 
    {
        return v >= p2.v;
    }

    const ptype<T>& operator=(const ptype<T>&p2)
    {
        v = p2.v;
        return p2;
    }

    bool operator>(const ptype<T>&p2) const 
    {
        return v > p2.v;
    }
    bool operator<(const ptype<T>&p2) const 
    {
        return v < p2.v;
    }

    bool operator>=(const ptype<T>&p2) const 
    {
        return v >= p2.v;
    }
    bool operator<=(const ptype<T>&p2) const 
    {
        return v <= p2.v;
    }
    const ptype<T> operator-() const 
    {
        return ptype<T>(-v);
    }
    // althought the definitionos of following arithmetic operators are quite
    // alike, we can't define them using macros, seems macro's parameter can only
    // be symbols or literals, not operators
    const ptype<T> operator +(const ptype<T> &p2) const 
    { 
        return ptype<T>(v + p2.v );
    }
    const ptype<T> operator -(const ptype<T> &p2) const
    { 
        return ptype<T>(v - p2.v );
    }
    const ptype<T> operator *(const ptype<T> &p2) const 
    { 
        return ptype<T>(v * p2.v );
    }
    const ptype<T> operator /(const ptype<T> &p2) const 
    { 
        return ptype<T>(v / p2.v );
    }
    const ptype<T> operator %(const ptype<T> &p2) const 
    { 
        return ptype<T>(v % p2.v );
    }
    const ptype<T>& operator %=(const ptype<T> &p2) const 
    { 
        v %= p2.v;
        return *this;
    }

    const ptype<T>& operator +=(const ptype<T> &p2) const 
    { 
        v += p2.v;
        return *this;
    }
    const ptype<T>& operator -=(const ptype<T> &p2) const 
    { 
        v -= p2.v;
        return *this;
    }
    const ptype<T>& operator /=(const ptype<T> &p2) const 
    { 
        v /= p2.v;
        return *this;
    }
    const ptype<T>& operator *=(const ptype<T> &p2) const 
    { 
        v *= p2.v;
        return *this;
    }
}; // ptype

// the following two functions help io ptype objs to/from any streams
template<Typename _CharT, Typename _Traits, Typename T>
basic_istream<_CharT,_Traits>&
operator>>( basic_istream<_CharT,_Traits> & in, ptype<T>&p)
{
    in>>p.v;
    return in;
}

template<Typename _CharT, Typename _Traits, Typename T>
basic_ostream<_CharT,_Traits>&
operator<<( basic_ostream<_CharT,_Traits> & out, const ptype<T>&p)
{
    out<<p.v;
    return out;
}

template <Typename T>
T operator * (T t, const ptype<T> &pt)
{
    return t * pt.v;
}

#endif // !_DB_STL_PTYPE_H
