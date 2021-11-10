#ifndef BOOST_BIND_BIND_HPP_INCLUDED
#define BOOST_BIND_BIND_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  bind.hpp - binds function objects to arguments
//
//  Copyright (c) 2001-2004 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2001 David Abrahams
//  Copyright (c) 2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/bind/bind.html for documentation.
//

#include <boost/config.hpp>
#include <boost/ref.hpp>
#include <boost/mem_fn.hpp>
#include <boost/type.hpp>
#include <boost/is_placeholder.hpp>
#include <boost/bind/arg.hpp>
#include <boost/bind/detail/result_traits.hpp>
#include <boost/bind/std_placeholders.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/visit_each.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/core/is_same.hpp>

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )
#include <utility> // std::forward
#endif

// Borland-specific bug, visit_each() silently fails to produce code

#if defined(BOOST_BORLANDC)
#  define BOOST_BIND_VISIT_EACH boost::visit_each
#else
#  define BOOST_BIND_VISIT_EACH visit_each
#endif

#include <boost/bind/storage.hpp>

#ifdef BOOST_MSVC
# pragma warning(push)
# pragma warning(disable: 4512) // assignment operator could not be generated
#endif

namespace boost
{

template<class T> class weak_ptr;

namespace _bi // implementation details
{

// ref_compare

template<class T> bool ref_compare( T const & a, T const & b, long )
{
    return a == b;
}

template<int I> bool ref_compare( arg<I> const &, arg<I> const &, int )
{
    return true;
}

template<int I> bool ref_compare( arg<I> (*) (), arg<I> (*) (), int )
{
    return true;
}

template<class T> bool ref_compare( reference_wrapper<T> const & a, reference_wrapper<T> const & b, int )
{
    return a.get_pointer() == b.get_pointer();
}

// bind_t forward declaration for listN

template<class R, class F, class L> class bind_t;

template<class R, class F, class L> bool ref_compare( bind_t<R, F, L> const & a, bind_t<R, F, L> const & b, int )
{
    return a.compare( b );
}

// value

template<class T> class value
{
public:

    value(T const & t): t_(t) {}

    T & get() { return t_; }
    T const & get() const { return t_; }

    bool operator==(value const & rhs) const
    {
        return t_ == rhs.t_;
    }

private:

    T t_;
};

// ref_compare for weak_ptr

template<class T> bool ref_compare( value< weak_ptr<T> > const & a, value< weak_ptr<T> > const & b, int )
{
    return !(a.get() < b.get()) && !(b.get() < a.get());
}

// type

template<class T> class type {};

// unwrap

template<class F> struct unwrapper
{
    static inline F & unwrap( F & f, long )
    {
        return f;
    }

    template<class F2> static inline F2 & unwrap( reference_wrapper<F2> rf, int )
    {
        return rf.get();
    }

    template<class R, class T> static inline _mfi::dm<R, T> unwrap( R T::* pm, int )
    {
        return _mfi::dm<R, T>( pm );
    }
};

// listN

class list0
{
public:

    list0() {}

    template<class T> T & operator[] (_bi::value<T> & v) const { return v.get(); }

    template<class T> T const & operator[] (_bi::value<T> const & v) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A &, long)
    {
        return unwrapper<F>::unwrap(f, 0)();
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A &, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)();
    }

    template<class F, class A> void operator()(type<void>, F & f, A &, int)
    {
        unwrapper<F>::unwrap(f, 0)();
    }

    template<class F, class A> void operator()(type<void>, F const & f, A &, int) const
    {
        unwrapper<F const>::unwrap(f, 0)();
    }

    template<class V> void accept(V &) const
    {
    }

    bool operator==(list0 const &) const
    {
        return true;
    }
};

#ifdef BOOST_MSVC
// MSVC is bright enough to realise that the parameter rhs 
// in operator==may be unused for some template argument types:
#pragma warning(push)
#pragma warning(disable:4100)
#endif

template< class A1 > class list1: private storage1< A1 >
{
private:

    typedef storage1< A1 > base_type;

public:

    explicit list1( A1 a1 ): base_type( a1 ) {}

    A1 operator[] (boost::arg<1>) const { return base_type::a1_; }

    A1 operator[] (boost::arg<1> (*) ()) const { return base_type::a1_; }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A & a, long)
    {
        return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_]);
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A & a, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_]);
    }

    template<class F, class A> void operator()(type<void>, F & f, A & a, int)
    {
        unwrapper<F>::unwrap(f, 0)(a[base_type::a1_]);
    }

    template<class F, class A> void operator()(type<void>, F const & f, A & a, int) const
    {
        unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_]);
    }

    template<class V> void accept(V & v) const
    {
        base_type::accept(v);
    }

    bool operator==(list1 const & rhs) const
    {
        return ref_compare(base_type::a1_, rhs.a1_, 0);
    }
};

struct logical_and;
struct logical_or;

template< class A1, class A2 > class list2: private storage2< A1, A2 >
{
private:

    typedef storage2< A1, A2 > base_type;

public:

    list2( A1 a1, A2 a2 ): base_type( a1, a2 ) {}

    A1 operator[] (boost::arg<1>) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2>) const { return base_type::a2_; }

    A1 operator[] (boost::arg<1> (*) ()) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2> (*) ()) const { return base_type::a2_; }

    template<class T> T & operator[] (_bi::value<T> & v) const { return v.get(); }

    template<class T> T const & operator[] (_bi::value<T> const & v) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A & a, long)
    {
        return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_]);
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A & a, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_]);
    }

    template<class F, class A> void operator()(type<void>, F & f, A & a, int)
    {
        unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_]);
    }

    template<class F, class A> void operator()(type<void>, F const & f, A & a, int) const
    {
        unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_]);
    }

    template<class A> bool operator()( type<bool>, logical_and & /*f*/, A & a, int )
    {
        return a[ base_type::a1_ ] && a[ base_type::a2_ ];
    }

    template<class A> bool operator()( type<bool>, logical_and const & /*f*/, A & a, int ) const
    {
        return a[ base_type::a1_ ] && a[ base_type::a2_ ];
    }

    template<class A> bool operator()( type<bool>, logical_or & /*f*/, A & a, int )
    {
        return a[ base_type::a1_ ] || a[ base_type::a2_ ];
    }

    template<class A> bool operator()( type<bool>, logical_or const & /*f*/, A & a, int ) const
    {
        return a[ base_type::a1_ ] || a[ base_type::a2_ ];
    }

    template<class V> void accept(V & v) const
    {
        base_type::accept(v);
    }

    bool operator==(list2 const & rhs) const
    {
        return ref_compare(base_type::a1_, rhs.a1_, 0) && ref_compare(base_type::a2_, rhs.a2_, 0);
    }
};

template< class A1, class A2, class A3 > class list3: private storage3< A1, A2, A3 >
{
private:

    typedef storage3< A1, A2, A3 > base_type;

public:

    list3( A1 a1, A2 a2, A3 a3 ): base_type( a1, a2, a3 ) {}

    A1 operator[] (boost::arg<1>) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2>) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3>) const { return base_type::a3_; }

    A1 operator[] (boost::arg<1> (*) ()) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2> (*) ()) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3> (*) ()) const { return base_type::a3_; }

    template<class T> T & operator[] (_bi::value<T> & v) const { return v.get(); }

    template<class T> T const & operator[] (_bi::value<T> const & v) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A & a, long)
    {
        return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_]);
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A & a, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_]);
    }

    template<class F, class A> void operator()(type<void>, F & f, A & a, int)
    {
        unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_]);
    }

    template<class F, class A> void operator()(type<void>, F const & f, A & a, int) const
    {
        unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_]);
    }

    template<class V> void accept(V & v) const
    {
        base_type::accept(v);
    }

    bool operator==(list3 const & rhs) const
    {
        return
            
            ref_compare( base_type::a1_, rhs.a1_, 0 ) &&
            ref_compare( base_type::a2_, rhs.a2_, 0 ) &&
            ref_compare( base_type::a3_, rhs.a3_, 0 );
    }
};

template< class A1, class A2, class A3, class A4 > class list4: private storage4< A1, A2, A3, A4 >
{
private:

    typedef storage4< A1, A2, A3, A4 > base_type;

public:

    list4( A1 a1, A2 a2, A3 a3, A4 a4 ): base_type( a1, a2, a3, a4 ) {}

    A1 operator[] (boost::arg<1>) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2>) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3>) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4>) const { return base_type::a4_; }

    A1 operator[] (boost::arg<1> (*) ()) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2> (*) ()) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3> (*) ()) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4> (*) ()) const { return base_type::a4_; }

    template<class T> T & operator[] (_bi::value<T> & v) const { return v.get(); }

    template<class T> T const & operator[] (_bi::value<T> const & v) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A & a, long)
    {
        return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_]);
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A & a, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_]);
    }

    template<class F, class A> void operator()(type<void>, F & f, A & a, int)
    {
        unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_]);
    }

    template<class F, class A> void operator()(type<void>, F const & f, A & a, int) const
    {
        unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_]);
    }

    template<class V> void accept(V & v) const
    {
        base_type::accept(v);
    }

    bool operator==(list4 const & rhs) const
    {
        return

            ref_compare( base_type::a1_, rhs.a1_, 0 ) &&
            ref_compare( base_type::a2_, rhs.a2_, 0 ) &&
            ref_compare( base_type::a3_, rhs.a3_, 0 ) &&
            ref_compare( base_type::a4_, rhs.a4_, 0 );
    }
};

template< class A1, class A2, class A3, class A4, class A5 > class list5: private storage5< A1, A2, A3, A4, A5 >
{
private:

    typedef storage5< A1, A2, A3, A4, A5 > base_type;

public:

    list5( A1 a1, A2 a2, A3 a3, A4 a4, A5 a5 ): base_type( a1, a2, a3, a4, a5 ) {}

    A1 operator[] (boost::arg<1>) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2>) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3>) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4>) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5>) const { return base_type::a5_; }

    A1 operator[] (boost::arg<1> (*) ()) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2> (*) ()) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3> (*) ()) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4> (*) ()) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5> (*) ()) const { return base_type::a5_; }

    template<class T> T & operator[] (_bi::value<T> & v) const { return v.get(); }

    template<class T> T const & operator[] (_bi::value<T> const & v) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A & a, long)
    {
        return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_]);
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A & a, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_]);
    }

    template<class F, class A> void operator()(type<void>, F & f, A & a, int)
    {
        unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_]);
    }

    template<class F, class A> void operator()(type<void>, F const & f, A & a, int) const
    {
        unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_]);
    }

    template<class V> void accept(V & v) const
    {
        base_type::accept(v);
    }

    bool operator==(list5 const & rhs) const
    {
        return

            ref_compare( base_type::a1_, rhs.a1_, 0 ) &&
            ref_compare( base_type::a2_, rhs.a2_, 0 ) &&
            ref_compare( base_type::a3_, rhs.a3_, 0 ) &&
            ref_compare( base_type::a4_, rhs.a4_, 0 ) &&
            ref_compare( base_type::a5_, rhs.a5_, 0 );
    }
};

template<class A1, class A2, class A3, class A4, class A5, class A6> class list6: private storage6< A1, A2, A3, A4, A5, A6 >
{
private:

    typedef storage6< A1, A2, A3, A4, A5, A6 > base_type;

public:

    list6( A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6 ): base_type( a1, a2, a3, a4, a5, a6 ) {}

    A1 operator[] (boost::arg<1>) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2>) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3>) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4>) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5>) const { return base_type::a5_; }
    A6 operator[] (boost::arg<6>) const { return base_type::a6_; }

    A1 operator[] (boost::arg<1> (*) ()) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2> (*) ()) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3> (*) ()) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4> (*) ()) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5> (*) ()) const { return base_type::a5_; }
    A6 operator[] (boost::arg<6> (*) ()) const { return base_type::a6_; }

    template<class T> T & operator[] (_bi::value<T> & v) const { return v.get(); }

    template<class T> T const & operator[] (_bi::value<T> const & v) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A & a, long)
    {
        return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_]);
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A & a, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_]);
    }

    template<class F, class A> void operator()(type<void>, F & f, A & a, int)
    {
        unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_]);
    }

    template<class F, class A> void operator()(type<void>, F const & f, A & a, int) const
    {
        unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_]);
    }

    template<class V> void accept(V & v) const
    {
        base_type::accept(v);
    }

    bool operator==(list6 const & rhs) const
    {
        return

            ref_compare( base_type::a1_, rhs.a1_, 0 ) &&
            ref_compare( base_type::a2_, rhs.a2_, 0 ) &&
            ref_compare( base_type::a3_, rhs.a3_, 0 ) &&
            ref_compare( base_type::a4_, rhs.a4_, 0 ) &&
            ref_compare( base_type::a5_, rhs.a5_, 0 ) &&
            ref_compare( base_type::a6_, rhs.a6_, 0 );
    }
};

template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> class list7: private storage7< A1, A2, A3, A4, A5, A6, A7 >
{
private:

    typedef storage7< A1, A2, A3, A4, A5, A6, A7 > base_type;

public:

    list7( A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7 ): base_type( a1, a2, a3, a4, a5, a6, a7 ) {}

    A1 operator[] (boost::arg<1>) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2>) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3>) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4>) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5>) const { return base_type::a5_; }
    A6 operator[] (boost::arg<6>) const { return base_type::a6_; }
    A7 operator[] (boost::arg<7>) const { return base_type::a7_; }

    A1 operator[] (boost::arg<1> (*) ()) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2> (*) ()) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3> (*) ()) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4> (*) ()) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5> (*) ()) const { return base_type::a5_; }
    A6 operator[] (boost::arg<6> (*) ()) const { return base_type::a6_; }
    A7 operator[] (boost::arg<7> (*) ()) const { return base_type::a7_; }

    template<class T> T & operator[] (_bi::value<T> & v) const { return v.get(); }

    template<class T> T const & operator[] (_bi::value<T> const & v) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A & a, long)
    {
        return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_]);
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A & a, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_]);
    }

    template<class F, class A> void operator()(type<void>, F & f, A & a, int)
    {
        unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_]);
    }

    template<class F, class A> void operator()(type<void>, F const & f, A & a, int) const
    {
        unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_]);
    }

    template<class V> void accept(V & v) const
    {
        base_type::accept(v);
    }

    bool operator==(list7 const & rhs) const
    {
        return

            ref_compare( base_type::a1_, rhs.a1_, 0 ) &&
            ref_compare( base_type::a2_, rhs.a2_, 0 ) &&
            ref_compare( base_type::a3_, rhs.a3_, 0 ) &&
            ref_compare( base_type::a4_, rhs.a4_, 0 ) &&
            ref_compare( base_type::a5_, rhs.a5_, 0 ) &&
            ref_compare( base_type::a6_, rhs.a6_, 0 ) &&
            ref_compare( base_type::a7_, rhs.a7_, 0 );
    }
};

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 > class list8: private storage8< A1, A2, A3, A4, A5, A6, A7, A8 >
{
private:

    typedef storage8< A1, A2, A3, A4, A5, A6, A7, A8 > base_type;

public:

    list8( A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8 ): base_type( a1, a2, a3, a4, a5, a6, a7, a8 ) {}

    A1 operator[] (boost::arg<1>) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2>) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3>) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4>) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5>) const { return base_type::a5_; }
    A6 operator[] (boost::arg<6>) const { return base_type::a6_; }
    A7 operator[] (boost::arg<7>) const { return base_type::a7_; }
    A8 operator[] (boost::arg<8>) const { return base_type::a8_; }

    A1 operator[] (boost::arg<1> (*) ()) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2> (*) ()) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3> (*) ()) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4> (*) ()) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5> (*) ()) const { return base_type::a5_; }
    A6 operator[] (boost::arg<6> (*) ()) const { return base_type::a6_; }
    A7 operator[] (boost::arg<7> (*) ()) const { return base_type::a7_; }
    A8 operator[] (boost::arg<8> (*) ()) const { return base_type::a8_; }

    template<class T> T & operator[] (_bi::value<T> & v) const { return v.get(); }

    template<class T> T const & operator[] (_bi::value<T> const & v) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A & a, long)
    {
        return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_]);
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A & a, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_]);
    }

    template<class F, class A> void operator()(type<void>, F & f, A & a, int)
    {
        unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_]);
    }

    template<class F, class A> void operator()(type<void>, F const & f, A & a, int) const
    {
        unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_]);
    }

    template<class V> void accept(V & v) const
    {
        base_type::accept(v);
    }

    bool operator==(list8 const & rhs) const
    {
        return
            
            ref_compare( base_type::a1_, rhs.a1_, 0 ) &&
            ref_compare( base_type::a2_, rhs.a2_, 0 ) &&
            ref_compare( base_type::a3_, rhs.a3_, 0 ) &&
            ref_compare( base_type::a4_, rhs.a4_, 0 ) &&
            ref_compare( base_type::a5_, rhs.a5_, 0 ) &&
            ref_compare( base_type::a6_, rhs.a6_, 0 ) &&
            ref_compare( base_type::a7_, rhs.a7_, 0 ) &&
            ref_compare( base_type::a8_, rhs.a8_, 0 );
    }
};

template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> class list9: private storage9< A1, A2, A3, A4, A5, A6, A7, A8, A9 >
{
private:

    typedef storage9< A1, A2, A3, A4, A5, A6, A7, A8, A9 > base_type;

public:

    list9( A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9 ): base_type( a1, a2, a3, a4, a5, a6, a7, a8, a9 ) {}

    A1 operator[] (boost::arg<1>) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2>) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3>) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4>) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5>) const { return base_type::a5_; }
    A6 operator[] (boost::arg<6>) const { return base_type::a6_; }
    A7 operator[] (boost::arg<7>) const { return base_type::a7_; }
    A8 operator[] (boost::arg<8>) const { return base_type::a8_; }
    A9 operator[] (boost::arg<9>) const { return base_type::a9_; }

    A1 operator[] (boost::arg<1> (*) ()) const { return base_type::a1_; }
    A2 operator[] (boost::arg<2> (*) ()) const { return base_type::a2_; }
    A3 operator[] (boost::arg<3> (*) ()) const { return base_type::a3_; }
    A4 operator[] (boost::arg<4> (*) ()) const { return base_type::a4_; }
    A5 operator[] (boost::arg<5> (*) ()) const { return base_type::a5_; }
    A6 operator[] (boost::arg<6> (*) ()) const { return base_type::a6_; }
    A7 operator[] (boost::arg<7> (*) ()) const { return base_type::a7_; }
    A8 operator[] (boost::arg<8> (*) ()) const { return base_type::a8_; }
    A9 operator[] (boost::arg<9> (*) ()) const { return base_type::a9_; }

    template<class T> T & operator[] (_bi::value<T> & v) const { return v.get(); }

    template<class T> T const & operator[] (_bi::value<T> const & v) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const { return b.eval(*this); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const { return b.eval(*this); }

    template<class R, class F, class A> R operator()(type<R>, F & f, A & a, long)
    {
        return unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_], a[base_type::a9_]);
    }

    template<class R, class F, class A> R operator()(type<R>, F const & f, A & a, long) const
    {
        return unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_], a[base_type::a9_]);
    }

    template<class F, class A> void operator()(type<void>, F & f, A & a, int)
    {
        unwrapper<F>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_], a[base_type::a9_]);
    }

    template<class F, class A> void operator()(type<void>, F const & f, A & a, int) const
    {
        unwrapper<F const>::unwrap(f, 0)(a[base_type::a1_], a[base_type::a2_], a[base_type::a3_], a[base_type::a4_], a[base_type::a5_], a[base_type::a6_], a[base_type::a7_], a[base_type::a8_], a[base_type::a9_]);
    }

    template<class V> void accept(V & v) const
    {
        base_type::accept(v);
    }

    bool operator==(list9 const & rhs) const
    {
        return

            ref_compare( base_type::a1_, rhs.a1_, 0 ) &&
            ref_compare( base_type::a2_, rhs.a2_, 0 ) &&
            ref_compare( base_type::a3_, rhs.a3_, 0 ) &&
            ref_compare( base_type::a4_, rhs.a4_, 0 ) &&
            ref_compare( base_type::a5_, rhs.a5_, 0 ) &&
            ref_compare( base_type::a6_, rhs.a6_, 0 ) &&
            ref_compare( base_type::a7_, rhs.a7_, 0 ) &&
            ref_compare( base_type::a8_, rhs.a8_, 0 ) &&
            ref_compare( base_type::a9_, rhs.a9_, 0 );
    }
};

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

// bind_t

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

template< class A1 > class rrlist1
{
private:

    A1 & a1_; // not A1&& because of msvc-10.0

public:

    explicit rrlist1( A1 & a1 ): a1_( a1 ) {}

    A1 && operator[] (boost::arg<1>) const { return std::forward<A1>( a1_ ); } // not static_cast because of g++ 4.9

    A1 && operator[] (boost::arg<1> (*) ()) const { return std::forward<A1>( a1_ ); }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const
    {
        rrlist1<A1&> a( a1_ );
        return b.eval( a );
    }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const
    {
        rrlist1<A1&> a( a1_ );
        return b.eval( a );
    }
};

template< class A1, class A2 > class rrlist2
{
private:

    A1 & a1_;
    A2 & a2_;

public:

    rrlist2( A1 & a1, A2 & a2 ): a1_( a1 ), a2_( a2 ) {}

    A1 && operator[] (boost::arg<1>) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2>) const { return std::forward<A2>( a2_ ); }

    A1 && operator[] (boost::arg<1> (*) ()) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2> (*) ()) const { return std::forward<A2>( a2_ ); }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const
    {
        rrlist2<A1&, A2&> a( a1_, a2_ );
        return b.eval( a );
    }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const
    {
        rrlist2<A1&, A2&> a( a1_, a2_ );
        return b.eval( a );
    }
};

template< class A1, class A2, class A3 > class rrlist3
{
private:

    A1 & a1_;
    A2 & a2_;
    A3 & a3_;

public:

    rrlist3( A1 & a1, A2 & a2, A3 & a3 ): a1_( a1 ), a2_( a2 ), a3_( a3 ) {}

    A1 && operator[] (boost::arg<1>) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2>) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3>) const { return std::forward<A3>( a3_ ); }

    A1 && operator[] (boost::arg<1> (*) ()) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2> (*) ()) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3> (*) ()) const { return std::forward<A3>( a3_ ); }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const
    {
        rrlist3<A1&, A2&, A3&> a( a1_, a2_, a3_ );
        return b.eval( a );
    }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const
    {
        rrlist3<A1&, A2&, A3&> a( a1_, a2_, a3_ );
        return b.eval( a );
    }
};

template< class A1, class A2, class A3, class A4 > class rrlist4
{
private:

    A1 & a1_;
    A2 & a2_;
    A3 & a3_;
    A4 & a4_;

public:

    rrlist4( A1 & a1, A2 & a2, A3 & a3, A4 & a4 ): a1_( a1 ), a2_( a2 ), a3_( a3 ), a4_( a4 ) {}

    A1 && operator[] (boost::arg<1>) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2>) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3>) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4>) const { return std::forward<A4>( a4_ ); }

    A1 && operator[] (boost::arg<1> (*) ()) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2> (*) ()) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3> (*) ()) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4> (*) ()) const { return std::forward<A4>( a4_ ); }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const
    {
        rrlist4<A1&, A2&, A3&, A4&> a( a1_, a2_, a3_, a4_ );
        return b.eval( a );
    }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const
    {
        rrlist4<A1&, A2&, A3&, A4&> a( a1_, a2_, a3_, a4_ );
        return b.eval( a );
    }
};

template< class A1, class A2, class A3, class A4, class A5 > class rrlist5
{
private:

    A1 & a1_;
    A2 & a2_;
    A3 & a3_;
    A4 & a4_;
    A5 & a5_;

public:

    rrlist5( A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5 ): a1_( a1 ), a2_( a2 ), a3_( a3 ), a4_( a4 ), a5_( a5 ) {}

    A1 && operator[] (boost::arg<1>) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2>) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3>) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4>) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5>) const { return std::forward<A5>( a5_ ); }

    A1 && operator[] (boost::arg<1> (*) ()) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2> (*) ()) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3> (*) ()) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4> (*) ()) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5> (*) ()) const { return std::forward<A5>( a5_ ); }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const
    {
        rrlist5<A1&, A2&, A3&, A4&, A5&> a( a1_, a2_, a3_, a4_, a5_ );
        return b.eval( a );
    }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const
    {
        rrlist5<A1&, A2&, A3&, A4&, A5&> a( a1_, a2_, a3_, a4_, a5_ );
        return b.eval( a );
    }
};

template< class A1, class A2, class A3, class A4, class A5, class A6 > class rrlist6
{
private:

    A1 & a1_;
    A2 & a2_;
    A3 & a3_;
    A4 & a4_;
    A5 & a5_;
    A6 & a6_;

public:

    rrlist6( A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6 ): a1_( a1 ), a2_( a2 ), a3_( a3 ), a4_( a4 ), a5_( a5 ), a6_( a6 ) {}

    A1 && operator[] (boost::arg<1>) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2>) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3>) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4>) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5>) const { return std::forward<A5>( a5_ ); }
    A6 && operator[] (boost::arg<6>) const { return std::forward<A6>( a6_ ); }

    A1 && operator[] (boost::arg<1> (*) ()) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2> (*) ()) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3> (*) ()) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4> (*) ()) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5> (*) ()) const { return std::forward<A5>( a5_ ); }
    A6 && operator[] (boost::arg<6> (*) ()) const { return std::forward<A6>( a6_ ); }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const
    {
        rrlist6<A1&, A2&, A3&, A4&, A5&, A6&> a( a1_, a2_, a3_, a4_, a5_, a6_ );
        return b.eval( a );
    }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const
    {
        rrlist6<A1&, A2&, A3&, A4&, A5&, A6&> a( a1_, a2_, a3_, a4_, a5_, a6_ );
        return b.eval( a );
    }
};

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7 > class rrlist7
{
private:

    A1 & a1_;
    A2 & a2_;
    A3 & a3_;
    A4 & a4_;
    A5 & a5_;
    A6 & a6_;
    A7 & a7_;

public:

    rrlist7( A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7 ): a1_( a1 ), a2_( a2 ), a3_( a3 ), a4_( a4 ), a5_( a5 ), a6_( a6 ), a7_( a7 ) {}

    A1 && operator[] (boost::arg<1>) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2>) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3>) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4>) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5>) const { return std::forward<A5>( a5_ ); }
    A6 && operator[] (boost::arg<6>) const { return std::forward<A6>( a6_ ); }
    A7 && operator[] (boost::arg<7>) const { return std::forward<A7>( a7_ ); }

    A1 && operator[] (boost::arg<1> (*) ()) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2> (*) ()) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3> (*) ()) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4> (*) ()) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5> (*) ()) const { return std::forward<A5>( a5_ ); }
    A6 && operator[] (boost::arg<6> (*) ()) const { return std::forward<A6>( a6_ ); }
    A7 && operator[] (boost::arg<7> (*) ()) const { return std::forward<A7>( a7_ ); }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const
    {
        rrlist7<A1&, A2&, A3&, A4&, A5&, A6&, A7&> a( a1_, a2_, a3_, a4_, a5_, a6_, a7_ );
        return b.eval( a );
    }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const
    {
        rrlist7<A1&, A2&, A3&, A4&, A5&, A6&, A7&> a( a1_, a2_, a3_, a4_, a5_, a6_, a7_ );
        return b.eval( a );
    }
};

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 > class rrlist8
{
private:

    A1 & a1_;
    A2 & a2_;
    A3 & a3_;
    A4 & a4_;
    A5 & a5_;
    A6 & a6_;
    A7 & a7_;
    A8 & a8_;

public:

    rrlist8( A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8 ): a1_( a1 ), a2_( a2 ), a3_( a3 ), a4_( a4 ), a5_( a5 ), a6_( a6 ), a7_( a7 ), a8_( a8 ) {}

    A1 && operator[] (boost::arg<1>) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2>) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3>) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4>) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5>) const { return std::forward<A5>( a5_ ); }
    A6 && operator[] (boost::arg<6>) const { return std::forward<A6>( a6_ ); }
    A7 && operator[] (boost::arg<7>) const { return std::forward<A7>( a7_ ); }
    A8 && operator[] (boost::arg<8>) const { return std::forward<A8>( a8_ ); }

    A1 && operator[] (boost::arg<1> (*) ()) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2> (*) ()) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3> (*) ()) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4> (*) ()) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5> (*) ()) const { return std::forward<A5>( a5_ ); }
    A6 && operator[] (boost::arg<6> (*) ()) const { return std::forward<A6>( a6_ ); }
    A7 && operator[] (boost::arg<7> (*) ()) const { return std::forward<A7>( a7_ ); }
    A8 && operator[] (boost::arg<8> (*) ()) const { return std::forward<A8>( a8_ ); }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const
    {
        rrlist8<A1&, A2&, A3&, A4&, A5&, A6&, A7&, A8&> a( a1_, a2_, a3_, a4_, a5_, a6_, a7_, a8_ );
        return b.eval( a );
    }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const
    {
        rrlist8<A1&, A2&, A3&, A4&, A5&, A6&, A7&, A8&> a( a1_, a2_, a3_, a4_, a5_, a6_, a7_, a8_ );
        return b.eval( a );
    }
};

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9 > class rrlist9
{
private:

    A1 & a1_;
    A2 & a2_;
    A3 & a3_;
    A4 & a4_;
    A5 & a5_;
    A6 & a6_;
    A7 & a7_;
    A8 & a8_;
    A9 & a9_;

public:

    rrlist9( A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8, A9 & a9 ): a1_( a1 ), a2_( a2 ), a3_( a3 ), a4_( a4 ), a5_( a5 ), a6_( a6 ), a7_( a7 ), a8_( a8 ), a9_( a9 ) {}

    A1 && operator[] (boost::arg<1>) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2>) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3>) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4>) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5>) const { return std::forward<A5>( a5_ ); }
    A6 && operator[] (boost::arg<6>) const { return std::forward<A6>( a6_ ); }
    A7 && operator[] (boost::arg<7>) const { return std::forward<A7>( a7_ ); }
    A8 && operator[] (boost::arg<8>) const { return std::forward<A8>( a8_ ); }
    A9 && operator[] (boost::arg<9>) const { return std::forward<A9>( a9_ ); }

    A1 && operator[] (boost::arg<1> (*) ()) const { return std::forward<A1>( a1_ ); }
    A2 && operator[] (boost::arg<2> (*) ()) const { return std::forward<A2>( a2_ ); }
    A3 && operator[] (boost::arg<3> (*) ()) const { return std::forward<A3>( a3_ ); }
    A4 && operator[] (boost::arg<4> (*) ()) const { return std::forward<A4>( a4_ ); }
    A5 && operator[] (boost::arg<5> (*) ()) const { return std::forward<A5>( a5_ ); }
    A6 && operator[] (boost::arg<6> (*) ()) const { return std::forward<A6>( a6_ ); }
    A7 && operator[] (boost::arg<7> (*) ()) const { return std::forward<A7>( a7_ ); }
    A8 && operator[] (boost::arg<8> (*) ()) const { return std::forward<A8>( a8_ ); }
    A9 && operator[] (boost::arg<9> (*) ()) const { return std::forward<A9>( a9_ ); }

    template<class T> T & operator[] ( _bi::value<T> & v ) const { return v.get(); }

    template<class T> T const & operator[] ( _bi::value<T> const & v ) const { return v.get(); }

    template<class T> T & operator[] (reference_wrapper<T> const & v) const { return v.get(); }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> & b) const
    {
        rrlist9<A1&, A2&, A3&, A4&, A5&, A6&, A7&, A8&, A9&> a( a1_, a2_, a3_, a4_, a5_, a6_, a7_, a8_, a9_ );
        return b.eval( a );
    }

    template<class R, class F, class L> typename result_traits<R, F>::type operator[] (bind_t<R, F, L> const & b) const
    {
        rrlist9<A1&, A2&, A3&, A4&, A5&, A6&, A7&, A8&, A9&> a( a1_, a2_, a3_, a4_, a5_, a6_, a7_, a8_, a9_ );
        return b.eval( a );
    }
};

template<class R, class F, class L> class bind_t
{
private:

    F f_;
    L l_;

public:

    typedef typename result_traits<R, F>::type result_type;
    typedef bind_t this_type;

    bind_t( F f, L const & l ): f_( f ), l_( l ) {}

    //

    result_type operator()()
    {
        list0 a;
        return l_( type<result_type>(), f_, a, 0 );
    }

    result_type operator()() const
    {
        list0 a;
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1> result_type operator()( A1 && a1 )
    {
        rrlist1< A1 > a( a1 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1> result_type operator()( A1 && a1 ) const
    {
        rrlist1< A1 > a( a1 );
        return l_(type<result_type>(), f_, a, 0);
    }

    template<class A1, class A2> result_type operator()( A1 && a1, A2 && a2 )
    {
        rrlist2< A1, A2 > a( a1, a2 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2> result_type operator()( A1 && a1, A2 && a2 ) const
    {
        rrlist2< A1, A2 > a( a1, a2 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3> result_type operator()( A1 && a1, A2 && a2, A3 && a3 )
    {
        rrlist3< A1, A2, A3 > a( a1, a2, a3 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3> result_type operator()( A1 && a1, A2 && a2, A3 && a3 ) const
    {
        rrlist3< A1, A2, A3 > a( a1, a2, a3 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4 )
    {
        rrlist4< A1, A2, A3, A4 > a( a1, a2, a3, a4 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4 ) const
    {
        rrlist4< A1, A2, A3, A4 > a( a1, a2, a3, a4 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5 )
    {
        rrlist5< A1, A2, A3, A4, A5 > a( a1, a2, a3, a4, a5 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5 ) const
    {
        rrlist5< A1, A2, A3, A4, A5 > a( a1, a2, a3, a4, a5 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5, A6 && a6 )
    {
        rrlist6< A1, A2, A3, A4, A5, A6 > a( a1, a2, a3, a4, a5, a6 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5, A6 && a6 ) const
    {
        rrlist6< A1, A2, A3, A4, A5, A6 > a( a1, a2, a3, a4, a5, a6 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5, A6 && a6, A7 && a7 )
    {
        rrlist7< A1, A2, A3, A4, A5, A6, A7 > a( a1, a2, a3, a4, a5, a6, a7 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5, A6 && a6, A7 && a7 ) const
    {
        rrlist7< A1, A2, A3, A4, A5, A6, A7 > a( a1, a2, a3, a4, a5, a6, a7 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5, A6 && a6, A7 && a7, A8 && a8 )
    {
        rrlist8< A1, A2, A3, A4, A5, A6, A7, A8 > a( a1, a2, a3, a4, a5, a6, a7, a8 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5, A6 && a6, A7 && a7, A8 && a8 ) const
    {
        rrlist8< A1, A2, A3, A4, A5, A6, A7, A8 > a( a1, a2, a3, a4, a5, a6, a7, a8 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5, A6 && a6, A7 && a7, A8 && a8, A9 && a9 )
    {
        rrlist9< A1, A2, A3, A4, A5, A6, A7, A8, A9 > a( a1, a2, a3, a4, a5, a6, a7, a8, a9 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()( A1 && a1, A2 && a2, A3 && a3, A4 && a4, A5 && a5, A6 && a6, A7 && a7, A8 && a8, A9 && a9 ) const
    {
        rrlist9< A1, A2, A3, A4, A5, A6, A7, A8, A9 > a( a1, a2, a3, a4, a5, a6, a7, a8, a9 );
        return l_( type<result_type>(), f_, a, 0 );
    }

    //

    template<class A> result_type eval( A & a )
    {
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class A> result_type eval( A & a ) const
    {
        return l_( type<result_type>(), f_, a, 0 );
    }

    template<class V> void accept( V & v ) const
    {
#if !defined( BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP ) && !defined( BOOST_BORLANDC )
        using boost::visit_each;
#endif

        BOOST_BIND_VISIT_EACH( v, f_, 0 );
        l_.accept( v );
    }

    bool compare( this_type const & rhs ) const
    {
        return ref_compare( f_, rhs.f_, 0 ) && l_ == rhs.l_;
    }
};

#elif !defined( BOOST_NO_VOID_RETURNS )

template<class R, class F, class L> class bind_t
{
public:

    typedef bind_t this_type;

    bind_t(F f, L const & l): f_(f), l_(l) {}

#define BOOST_BIND_RETURN return
#include <boost/bind/bind_template.hpp>
#undef BOOST_BIND_RETURN

};

#else // no void returns

template<class R> struct bind_t_generator
{

template<class F, class L> class implementation
{
public:

    typedef implementation this_type;

    implementation(F f, L const & l): f_(f), l_(l) {}

#define BOOST_BIND_RETURN return
#include <boost/bind/bind_template.hpp>
#undef BOOST_BIND_RETURN

};

};

template<> struct bind_t_generator<void>
{

template<class F, class L> class implementation
{
private:

    typedef void R;

public:

    typedef implementation this_type;

    implementation(F f, L const & l): f_(f), l_(l) {}

#define BOOST_BIND_RETURN
#include <boost/bind/bind_template.hpp>
#undef BOOST_BIND_RETURN

};

};

template<class R2, class F, class L> class bind_t: public bind_t_generator<R2>::BOOST_NESTED_TEMPLATE implementation<F, L>
{
public:

    bind_t(F f, L const & l): bind_t_generator<R2>::BOOST_NESTED_TEMPLATE implementation<F, L>(f, l) {}

};

#endif

// function_equal

#ifndef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP

// put overloads in _bi, rely on ADL

# ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

template<class R, class F, class L> bool function_equal( bind_t<R, F, L> const & a, bind_t<R, F, L> const & b )
{
    return a.compare(b);
}

# else

template<class R, class F, class L> bool function_equal_impl( bind_t<R, F, L> const & a, bind_t<R, F, L> const & b, int )
{
    return a.compare(b);
}

# endif // #ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

#else // BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP

// put overloads in boost

} // namespace _bi

# ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

template<class R, class F, class L> bool function_equal( _bi::bind_t<R, F, L> const & a, _bi::bind_t<R, F, L> const & b )
{
    return a.compare(b);
}

# else

template<class R, class F, class L> bool function_equal_impl( _bi::bind_t<R, F, L> const & a, _bi::bind_t<R, F, L> const & b, int )
{
    return a.compare(b);
}

# endif // #ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

namespace _bi
{

#endif // BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP

// add_value

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) || (__SUNPRO_CC >= 0x530)

#if defined( BOOST_BORLANDC ) && BOOST_WORKAROUND( BOOST_BORLANDC, BOOST_TESTED_AT(0x582) )

template<class T> struct add_value
{
    typedef _bi::value<T> type;
};

#else

template< class T, int I > struct add_value_2
{
    typedef boost::arg<I> type;
};

template< class T > struct add_value_2< T, 0 >
{
    typedef _bi::value< T > type;
};

template<class T> struct add_value
{
    typedef typename add_value_2< T, boost::is_placeholder< T >::value >::type type;
};

#endif

template<class T> struct add_value< value<T> >
{
    typedef _bi::value<T> type;
};

template<class T> struct add_value< reference_wrapper<T> >
{
    typedef reference_wrapper<T> type;
};

template<int I> struct add_value< arg<I> >
{
    typedef boost::arg<I> type;
};

template<int I> struct add_value< arg<I> (*) () >
{
    typedef boost::arg<I> (*type) ();
};

template<class R, class F, class L> struct add_value< bind_t<R, F, L> >
{
    typedef bind_t<R, F, L> type;
};

#else

template<int I> struct _avt_0;

template<> struct _avt_0<1>
{
    template<class T> struct inner
    {
        typedef T type;
    };
};

template<> struct _avt_0<2>
{
    template<class T> struct inner
    {
        typedef value<T> type;
    };
};

typedef char (&_avt_r1) [1];
typedef char (&_avt_r2) [2];

template<class T> _avt_r1 _avt_f(value<T>);
template<class T> _avt_r1 _avt_f(reference_wrapper<T>);
template<int I> _avt_r1 _avt_f(arg<I>);
template<int I> _avt_r1 _avt_f(arg<I> (*) ());
template<class R, class F, class L> _avt_r1 _avt_f(bind_t<R, F, L>);

_avt_r2 _avt_f(...);

template<class T> struct add_value
{
    static T t();
    typedef typename _avt_0<sizeof(_avt_f(t()))>::template inner<T>::type type;
};

#endif

// list_av_N

template<class A1> struct list_av_1
{
    typedef typename add_value<A1>::type B1;
    typedef list1<B1> type;
};

template<class A1, class A2> struct list_av_2
{
    typedef typename add_value<A1>::type B1;
    typedef typename add_value<A2>::type B2;
    typedef list2<B1, B2> type;
};

template<class A1, class A2, class A3> struct list_av_3
{
    typedef typename add_value<A1>::type B1;
    typedef typename add_value<A2>::type B2;
    typedef typename add_value<A3>::type B3;
    typedef list3<B1, B2, B3> type;
};

template<class A1, class A2, class A3, class A4> struct list_av_4
{
    typedef typename add_value<A1>::type B1;
    typedef typename add_value<A2>::type B2;
    typedef typename add_value<A3>::type B3;
    typedef typename add_value<A4>::type B4;
    typedef list4<B1, B2, B3, B4> type;
};

template<class A1, class A2, class A3, class A4, class A5> struct list_av_5
{
    typedef typename add_value<A1>::type B1;
    typedef typename add_value<A2>::type B2;
    typedef typename add_value<A3>::type B3;
    typedef typename add_value<A4>::type B4;
    typedef typename add_value<A5>::type B5;
    typedef list5<B1, B2, B3, B4, B5> type;
};

template<class A1, class A2, class A3, class A4, class A5, class A6> struct list_av_6
{
    typedef typename add_value<A1>::type B1;
    typedef typename add_value<A2>::type B2;
    typedef typename add_value<A3>::type B3;
    typedef typename add_value<A4>::type B4;
    typedef typename add_value<A5>::type B5;
    typedef typename add_value<A6>::type B6;
    typedef list6<B1, B2, B3, B4, B5, B6> type;
};

template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> struct list_av_7
{
    typedef typename add_value<A1>::type B1;
    typedef typename add_value<A2>::type B2;
    typedef typename add_value<A3>::type B3;
    typedef typename add_value<A4>::type B4;
    typedef typename add_value<A5>::type B5;
    typedef typename add_value<A6>::type B6;
    typedef typename add_value<A7>::type B7;
    typedef list7<B1, B2, B3, B4, B5, B6, B7> type;
};

template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> struct list_av_8
{
    typedef typename add_value<A1>::type B1;
    typedef typename add_value<A2>::type B2;
    typedef typename add_value<A3>::type B3;
    typedef typename add_value<A4>::type B4;
    typedef typename add_value<A5>::type B5;
    typedef typename add_value<A6>::type B6;
    typedef typename add_value<A7>::type B7;
    typedef typename add_value<A8>::type B8;
    typedef list8<B1, B2, B3, B4, B5, B6, B7, B8> type;
};

template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> struct list_av_9
{
    typedef typename add_value<A1>::type B1;
    typedef typename add_value<A2>::type B2;
    typedef typename add_value<A3>::type B3;
    typedef typename add_value<A4>::type B4;
    typedef typename add_value<A5>::type B5;
    typedef typename add_value<A6>::type B6;
    typedef typename add_value<A7>::type B7;
    typedef typename add_value<A8>::type B8;
    typedef typename add_value<A9>::type B9;
    typedef list9<B1, B2, B3, B4, B5, B6, B7, B8, B9> type;
};

// operator!

struct logical_not
{
    template<class V> bool operator()(V const & v) const { return !v; }
};

template<class R, class F, class L>
    bind_t< bool, logical_not, list1< bind_t<R, F, L> > >
    operator! (bind_t<R, F, L> const & f)
{
    typedef list1< bind_t<R, F, L> > list_type;
    return bind_t<bool, logical_not, list_type> ( logical_not(), list_type(f) );
}

// relational operators

#define BOOST_BIND_OPERATOR( op, name ) \
\
struct name \
{ \
    template<class V, class W> bool operator()(V const & v, W const & w) const { return v op w; } \
}; \
 \
template<class R, class F, class L, class A2> \
    bind_t< bool, name, list2< bind_t<R, F, L>, typename add_value<A2>::type > > \
    operator op (bind_t<R, F, L> const & f, A2 a2) \
{ \
    typedef typename add_value<A2>::type B2; \
    typedef list2< bind_t<R, F, L>, B2> list_type; \
    return bind_t<bool, name, list_type> ( name(), list_type(f, a2) ); \
}

BOOST_BIND_OPERATOR( ==, equal )
BOOST_BIND_OPERATOR( !=, not_equal )

BOOST_BIND_OPERATOR( <, less )
BOOST_BIND_OPERATOR( <=, less_equal )

BOOST_BIND_OPERATOR( >, greater )
BOOST_BIND_OPERATOR( >=, greater_equal )

BOOST_BIND_OPERATOR( &&, logical_and )
BOOST_BIND_OPERATOR( ||, logical_or )

#undef BOOST_BIND_OPERATOR

#if defined(__GNUC__) && BOOST_WORKAROUND(__GNUC__, < 3)

// resolve ambiguity with rel_ops

#define BOOST_BIND_OPERATOR( op, name ) \
\
template<class R, class F, class L> \
    bind_t< bool, name, list2< bind_t<R, F, L>, bind_t<R, F, L> > > \
    operator op (bind_t<R, F, L> const & f, bind_t<R, F, L> const & g) \
{ \
    typedef list2< bind_t<R, F, L>, bind_t<R, F, L> > list_type; \
    return bind_t<bool, name, list_type> ( name(), list_type(f, g) ); \
}

BOOST_BIND_OPERATOR( !=, not_equal )
BOOST_BIND_OPERATOR( <=, less_equal )
BOOST_BIND_OPERATOR( >, greater )
BOOST_BIND_OPERATOR( >=, greater_equal )

#endif

// visit_each, ADL

#if !defined( BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP ) && !defined( BOOST_BORLANDC ) \
   && !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)

template<class V, class T> void visit_each( V & v, value<T> const & t, int )
{
    using boost::visit_each;
    BOOST_BIND_VISIT_EACH( v, t.get(), 0 );
}

template<class V, class R, class F, class L> void visit_each( V & v, bind_t<R, F, L> const & t, int )
{
    t.accept( v );
}

#endif

} // namespace _bi

// visit_each, no ADL

#if defined( BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP ) || defined( BOOST_BORLANDC ) \
  || (defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)

template<class V, class T> void visit_each( V & v, _bi::value<T> const & t, int )
{
    BOOST_BIND_VISIT_EACH( v, t.get(), 0 );
}

template<class V, class R, class F, class L> void visit_each( V & v, _bi::bind_t<R, F, L> const & t, int )
{
    t.accept( v );
}

#endif

// is_bind_expression

template< class T > struct is_bind_expression
{
    enum _vt { value = 0 };
};

#if !defined( BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION )

template< class R, class F, class L > struct is_bind_expression< _bi::bind_t< R, F, L > >
{
    enum _vt { value = 1 };
};

#endif

// bind

#ifndef BOOST_BIND
#define BOOST_BIND bind
#endif

// generic function objects

template<class R, class F>
    _bi::bind_t<R, F, _bi::list0>
    BOOST_BIND(F f)
{
    typedef _bi::list0 list_type;
    return _bi::bind_t<R, F, list_type> (f, list_type());
}

template<class R, class F, class A1>
    _bi::bind_t<R, F, typename _bi::list_av_1<A1>::type>
    BOOST_BIND(F f, A1 a1)
{
    typedef typename _bi::list_av_1<A1>::type list_type;
    return _bi::bind_t<R, F, list_type> (f, list_type(a1));
}

template<class R, class F, class A1, class A2>
    _bi::bind_t<R, F, typename _bi::list_av_2<A1, A2>::type>
    BOOST_BIND(F f, A1 a1, A2 a2)
{
    typedef typename _bi::list_av_2<A1, A2>::type list_type;
    return _bi::bind_t<R, F, list_type> (f, list_type(a1, a2));
}

template<class R, class F, class A1, class A2, class A3>
    _bi::bind_t<R, F, typename _bi::list_av_3<A1, A2, A3>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3)
{
    typedef typename _bi::list_av_3<A1, A2, A3>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3));
}

template<class R, class F, class A1, class A2, class A3, class A4>
    _bi::bind_t<R, F, typename _bi::list_av_4<A1, A2, A3, A4>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef typename _bi::list_av_4<A1, A2, A3, A4>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5>
    _bi::bind_t<R, F, typename _bi::list_av_5<A1, A2, A3, A4, A5>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef typename _bi::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6>
    _bi::bind_t<R, F, typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    _bi::bind_t<R, F, typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    _bi::bind_t<R, F, typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    _bi::bind_t<R, F, typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}

// generic function objects, alternative syntax

template<class R, class F>
    _bi::bind_t<R, F, _bi::list0>
    BOOST_BIND(boost::type<R>, F f)
{
    typedef _bi::list0 list_type;
    return _bi::bind_t<R, F, list_type> (f, list_type());
}

template<class R, class F, class A1>
    _bi::bind_t<R, F, typename _bi::list_av_1<A1>::type>
    BOOST_BIND(boost::type<R>, F f, A1 a1)
{
    typedef typename _bi::list_av_1<A1>::type list_type;
    return _bi::bind_t<R, F, list_type> (f, list_type(a1));
}

template<class R, class F, class A1, class A2>
    _bi::bind_t<R, F, typename _bi::list_av_2<A1, A2>::type>
    BOOST_BIND(boost::type<R>, F f, A1 a1, A2 a2)
{
    typedef typename _bi::list_av_2<A1, A2>::type list_type;
    return _bi::bind_t<R, F, list_type> (f, list_type(a1, a2));
}

template<class R, class F, class A1, class A2, class A3>
    _bi::bind_t<R, F, typename _bi::list_av_3<A1, A2, A3>::type>
    BOOST_BIND(boost::type<R>, F f, A1 a1, A2 a2, A3 a3)
{
    typedef typename _bi::list_av_3<A1, A2, A3>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3));
}

template<class R, class F, class A1, class A2, class A3, class A4>
    _bi::bind_t<R, F, typename _bi::list_av_4<A1, A2, A3, A4>::type>
    BOOST_BIND(boost::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef typename _bi::list_av_4<A1, A2, A3, A4>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5>
    _bi::bind_t<R, F, typename _bi::list_av_5<A1, A2, A3, A4, A5>::type>
    BOOST_BIND(boost::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef typename _bi::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6>
    _bi::bind_t<R, F, typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    BOOST_BIND(boost::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    _bi::bind_t<R, F, typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    BOOST_BIND(boost::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    _bi::bind_t<R, F, typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    BOOST_BIND(boost::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

template<class R, class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    _bi::bind_t<R, F, typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    BOOST_BIND(boost::type<R>, F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return _bi::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)

// adaptable function objects

template<class F>
    _bi::bind_t<_bi::unspecified, F, _bi::list0>
    BOOST_BIND(F f)
{
    typedef _bi::list0 list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type> (f, list_type());
}

template<class F, class A1>
    _bi::bind_t<_bi::unspecified, F, typename _bi::list_av_1<A1>::type>
    BOOST_BIND(F f, A1 a1)
{
    typedef typename _bi::list_av_1<A1>::type list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type> (f, list_type(a1));
}

template<class F, class A1, class A2>
    _bi::bind_t<_bi::unspecified, F, typename _bi::list_av_2<A1, A2>::type>
    BOOST_BIND(F f, A1 a1, A2 a2)
{
    typedef typename _bi::list_av_2<A1, A2>::type list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type> (f, list_type(a1, a2));
}

template<class F, class A1, class A2, class A3>
    _bi::bind_t<_bi::unspecified, F, typename _bi::list_av_3<A1, A2, A3>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3)
{
    typedef typename _bi::list_av_3<A1, A2, A3>::type list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type>(f, list_type(a1, a2, a3));
}

template<class F, class A1, class A2, class A3, class A4>
    _bi::bind_t<_bi::unspecified, F, typename _bi::list_av_4<A1, A2, A3, A4>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef typename _bi::list_av_4<A1, A2, A3, A4>::type list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4));
}

template<class F, class A1, class A2, class A3, class A4, class A5>
    _bi::bind_t<_bi::unspecified, F, typename _bi::list_av_5<A1, A2, A3, A4, A5>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef typename _bi::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5));
}

template<class F, class A1, class A2, class A3, class A4, class A5, class A6>
    _bi::bind_t<_bi::unspecified, F, typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6));
}

template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    _bi::bind_t<_bi::unspecified, F, typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7));
}

template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    _bi::bind_t<_bi::unspecified, F, typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    _bi::bind_t<_bi::unspecified, F, typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    BOOST_BIND(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return _bi::bind_t<_bi::unspecified, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}

#endif // !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)

// function pointers

#define BOOST_BIND_CC
#define BOOST_BIND_ST
#define BOOST_BIND_NOEXCEPT

#include <boost/bind/bind_cc.hpp>

# if defined( __cpp_noexcept_function_type ) || defined( _NOEXCEPT_TYPES_SUPPORTED )
#   undef BOOST_BIND_NOEXCEPT
#   define BOOST_BIND_NOEXCEPT noexcept
#   include <boost/bind/bind_cc.hpp>
# endif

#undef BOOST_BIND_CC
#undef BOOST_BIND_ST
#undef BOOST_BIND_NOEXCEPT

#if defined(BOOST_BIND_ENABLE_STDCALL) && !defined(_M_X64)

#define BOOST_BIND_CC __stdcall
#define BOOST_BIND_ST
#define BOOST_BIND_NOEXCEPT

#include <boost/bind/bind_cc.hpp>

#undef BOOST_BIND_CC
#undef BOOST_BIND_ST
#undef BOOST_BIND_NOEXCEPT

#endif

#if defined(BOOST_BIND_ENABLE_FASTCALL) && !defined(_M_X64)

#define BOOST_BIND_CC __fastcall
#define BOOST_BIND_ST
#define BOOST_BIND_NOEXCEPT

#include <boost/bind/bind_cc.hpp>

#undef BOOST_BIND_CC
#undef BOOST_BIND_ST
#undef BOOST_BIND_NOEXCEPT

#endif

#ifdef BOOST_BIND_ENABLE_PASCAL

#define BOOST_BIND_ST pascal
#define BOOST_BIND_CC
#define BOOST_BIND_NOEXCEPT

#include <boost/bind/bind_cc.hpp>

#undef BOOST_BIND_ST
#undef BOOST_BIND_CC
#undef BOOST_BIND_NOEXCEPT

#endif

// member function pointers

#define BOOST_BIND_MF_NAME(X) X
#define BOOST_BIND_MF_CC
#define BOOST_BIND_MF_NOEXCEPT

#include <boost/bind/bind_mf_cc.hpp>
#include <boost/bind/bind_mf2_cc.hpp>

# if defined( __cpp_noexcept_function_type ) || defined( _NOEXCEPT_TYPES_SUPPORTED )
#   undef BOOST_BIND_MF_NOEXCEPT
#   define BOOST_BIND_MF_NOEXCEPT noexcept
#   include <boost/bind/bind_mf_cc.hpp>
#   include <boost/bind/bind_mf2_cc.hpp>
# endif

#undef BOOST_BIND_MF_NAME
#undef BOOST_BIND_MF_CC
#undef BOOST_BIND_MF_NOEXCEPT

#if defined(BOOST_MEM_FN_ENABLE_CDECL) && !defined(_M_X64)

#define BOOST_BIND_MF_NAME(X) X##_cdecl
#define BOOST_BIND_MF_CC __cdecl
#define BOOST_BIND_MF_NOEXCEPT

#include <boost/bind/bind_mf_cc.hpp>
#include <boost/bind/bind_mf2_cc.hpp>

#undef BOOST_BIND_MF_NAME
#undef BOOST_BIND_MF_CC
#undef BOOST_BIND_MF_NOEXCEPT

#endif

#if defined(BOOST_MEM_FN_ENABLE_STDCALL) && !defined(_M_X64)

#define BOOST_BIND_MF_NAME(X) X##_stdcall
#define BOOST_BIND_MF_CC __stdcall
#define BOOST_BIND_MF_NOEXCEPT

#include <boost/bind/bind_mf_cc.hpp>
#include <boost/bind/bind_mf2_cc.hpp>

#undef BOOST_BIND_MF_NAME
#undef BOOST_BIND_MF_CC
#undef BOOST_BIND_MF_NOEXCEPT

#endif

#if defined(BOOST_MEM_FN_ENABLE_FASTCALL) && !defined(_M_X64)

#define BOOST_BIND_MF_NAME(X) X##_fastcall
#define BOOST_BIND_MF_CC __fastcall
#define BOOST_BIND_MF_NOEXCEPT

#include <boost/bind/bind_mf_cc.hpp>
#include <boost/bind/bind_mf2_cc.hpp>

#undef BOOST_BIND_MF_NAME
#undef BOOST_BIND_MF_CC
#undef BOOST_BIND_MF_NOEXCEPT

#endif

// data member pointers

#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) \
    || ( defined(BOOST_BORLANDC) && BOOST_WORKAROUND( BOOST_BORLANDC, BOOST_TESTED_AT( 0x620 ) ) )

template<class R, class T, class A1>
_bi::bind_t< R, _mfi::dm<R, T>, typename _bi::list_av_1<A1>::type >
    BOOST_BIND(R T::*f, A1 a1)
{
    typedef _mfi::dm<R, T> F;
    typedef typename _bi::list_av_1<A1>::type list_type;
    return _bi::bind_t<R, F, list_type>( F(f), list_type(a1) );
}

#else

namespace _bi
{

template< class Pm, int I > struct add_cref;

template< class M, class T > struct add_cref< M T::*, 0 >
{
    typedef M type;
};

template< class M, class T > struct add_cref< M T::*, 1 >
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4180)
#endif
    typedef M const & type;
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
};

template< class R, class T > struct add_cref< R (T::*) (), 1 >
{
    typedef void type;
};

#if !defined(__IBMCPP__) || __IBMCPP_FUNC_CV_TMPL_ARG_DEDUCTION

template< class R, class T > struct add_cref< R (T::*) () const, 1 >
{
    typedef void type;
};

#if defined( __cpp_noexcept_function_type ) || defined( _NOEXCEPT_TYPES_SUPPORTED )

template< class R, class T > struct add_cref< R (T::*) () const noexcept, 1 >
{
    typedef void type;
};

#endif // __cpp_noexcept_function_type

#endif // __IBMCPP__

template<class R> struct isref
{
    enum value_type { value = 0 };
};

template<class R> struct isref< R& >
{
    enum value_type { value = 1 };
};

template<class R> struct isref< R* >
{
    enum value_type { value = 1 };
};

template<class Pm, class A1> struct dm_result
{
    typedef typename add_cref< Pm, 1 >::type type;
};

template<class Pm, class R, class F, class L> struct dm_result< Pm, bind_t<R, F, L> >
{
    typedef typename bind_t<R, F, L>::result_type result_type;
    typedef typename add_cref< Pm, isref< result_type >::value >::type type;
};

} // namespace _bi

template< class A1, class M, class T >

_bi::bind_t<
    typename _bi::dm_result< M T::*, A1 >::type,
    _mfi::dm<M, T>,
    typename _bi::list_av_1<A1>::type
>

BOOST_BIND( M T::*f, A1 a1 )
{
    typedef typename _bi::dm_result< M T::*, A1 >::type result_type;
    typedef _mfi::dm<M, T> F;
    typedef typename _bi::list_av_1<A1>::type list_type;
    return _bi::bind_t< result_type, F, list_type >( F( f ), list_type( a1 ) );
}

#endif

} // namespace boost

#ifndef BOOST_BIND_NO_PLACEHOLDERS

# include <boost/bind/placeholders.hpp>

#endif

#ifdef BOOST_MSVC
# pragma warning(default: 4512) // assignment operator could not be generated
# pragma warning(pop)
#endif

#endif // #ifndef BOOST_BIND_BIND_HPP_INCLUDED
