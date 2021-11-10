////////////////////////////////////////////////////////////////////////////
// lazy_reuse.hpp
//
// Build lazy operations for Phoenix equivalents for FC++
//
// These are equivalents of the Boost FC++ functoids in reuse.hpp
//
// Implemented so far:
//
// reuser1
// reuser2
// reuser3
// reuser4 (not yet tested)
//
// NOTE: It has been possible to simplify the operation of this code.
//       It now makes no use of boost::function or old FC++ code.
//
//       The functor type F must be an operator defined with
//       boost::phoenix::function.
//       See the example Apply in lazy_prelude.hpp
//
////////////////////////////////////////////////////////////////////////////
/*=============================================================================
    Copyright (c) 2000-2003 Brian McNamara and Yannis Smaragdakis
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2015 John Fletcher

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_PHOENIX_FUNCTION_LAZY_REUSE
#define BOOST_PHOENIX_FUNCTION_LAZY_REUSE

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/intrusive_ptr.hpp>


namespace boost {

  namespace phoenix {

    namespace fcpp {

//////////////////////////////////////////////////////////////////////
// Original FC++ comment:
// "Reuser"s are effectively special-purpose versions of curry() that
// enable recursive list functoids to reuse the thunk of the curried
// recursive call.  See
//    http://www.cc.gatech.edu/~yannis/fc++/New/reusers.html
// for a more detailed description.
//////////////////////////////////////////////////////////////////////

// For efficiency, we mark parameters as either "VAR"iant or "INV"ariant.
struct INV {};
struct VAR {};

template <class V, class X> struct Maybe_Var_Inv;
template <class X>
struct Maybe_Var_Inv<VAR,X> {
   static void remake( X& x, const X& val ) {
      x.~X();
      new (&x) X(val);
   }
   static X clone( const X& x ) { return X(x); }
};
template <class X>
struct Maybe_Var_Inv<INV,X> {
   static void remake( X&, const X& ) {}
   static const X& clone( const X& x ) { return x; }
};

/////////////////////////////////////////////////////////////////////
// ThunkImpl is an implementation of Fun0Impl for this use.
/////////////////////////////////////////////////////////////////////

template <class Result>
class ThunkImpl
{
   mutable RefCountType refC;
public:
   ThunkImpl() : refC(0) {}
   virtual Result operator()() const =0;
   virtual ~ThunkImpl() {}
   template <class X>
   friend void intrusive_ptr_add_ref( const ThunkImpl<X>* p );
   template <class X>
   friend void intrusive_ptr_release( const ThunkImpl<X>* p );
};

template <class T>
void intrusive_ptr_add_ref( const ThunkImpl<T>* p ) {
   ++ (p->refC);
}
template <class T>
void intrusive_ptr_release( const ThunkImpl<T>* p ) {
   if( !--(p->refC) ) delete p;
}

//////////////////////////////////////////////////////////////////////
// reuser1 is needed in list<T> operations
//////////////////////////////////////////////////////////////////////

template <class V1, class V2, class F, class X>
struct reuser1;

template <class V1, class V2, class F, class X, class R>
struct Thunk1 : public ThunkImpl<R> {
   mutable F f;
   mutable X x;
   Thunk1( const F& ff, const X& xx ) : f(ff), x(xx) {}
   void init( const F& ff, const X& xx ) const {
      Maybe_Var_Inv<V1,F>::remake( f, ff );
      Maybe_Var_Inv<V2,X>::remake( x, xx );
   }
   R operator()() const {
      return Maybe_Var_Inv<V1,F>::clone(f)(
         Maybe_Var_Inv<V2,X>::clone(x),
         reuser1<V1,V2,F,X>(this) );
   }
};

template <class V1, class V2, class F, class X>
struct reuser1 {
   typedef typename F::template result<F(X)>::type R;
   typedef typename boost::phoenix::function<R> fun0_type;
   typedef Thunk1<V1,V2,F,X,R> M;
   typedef M result_type;
   boost::intrusive_ptr<const M> ref;
   reuser1(a_unique_type_for_nil) {}
   reuser1(const M* m) : ref(m) {}
   M operator()( const F& f, const X& x ) {
      if( !ref )   ref = boost::intrusive_ptr<const M>( new M(f,x) );
      else         ref->init(f,x);
      return *ref;
   }
   void iter( const F& f, const X& x ) {
      if( ref )    ref->init(f,x);
   }
};

//////////////////////////////////////////////////////////////////////
// reuser2 is needed in list<T>
//////////////////////////////////////////////////////////////////////

template <class V1, class V2, class V3, class F, class X, class Y>
struct reuser2;

template <class V1, class V2, class V3, class F, class X, class Y, class R>
struct Thunk2 : public ThunkImpl<R> {
   mutable F f;
   mutable X x;
   mutable Y y;
   Thunk2( const F& ff, const X& xx, const Y& yy ) : f(ff), x(xx), y(yy) {}
   void init( const F& ff, const X& xx, const Y& yy ) const {
      Maybe_Var_Inv<V1,F>::remake( f, ff );
      Maybe_Var_Inv<V2,X>::remake( x, xx );
      Maybe_Var_Inv<V3,Y>::remake( y, yy );
   }
   R operator()() const {
      return Maybe_Var_Inv<V1,F>::clone(f)(
         Maybe_Var_Inv<V2,X>::clone(x),
         Maybe_Var_Inv<V3,Y>::clone(y),
         reuser2<V1,V2,V3,F,X,Y>(this) );
   }
};

template <class V1, class V2, class V3, class F, class X, class Y>
struct reuser2 {
   typedef typename F::template result<F(X,Y)>::type R;
   typedef Thunk2<V1,V2,V3,F,X,Y,R> M;
   typedef M result_type;
   boost::intrusive_ptr<const M> ref;
   reuser2(a_unique_type_for_nil) {}
   reuser2(const M* m) : ref(m) {}
   M operator()( const F& f, const X& x, const Y& y ) {
      if( !ref )   ref = boost::intrusive_ptr<const M>( new M(f,x,y) );
      else         ref->init(f,x,y);
      return *ref;
   }
   void iter( const F& f, const X& x, const Y& y ) {
      if( ref )    ref->init(f,x,y);
   }
};

//////////////////////////////////////////////////////////////////////
// reuser3
//////////////////////////////////////////////////////////////////////

template <class V1, class V2, class V3, class V4,
          class F, class X, class Y, class Z>
struct reuser3;

template <class V1, class V2, class V3, class V4,
          class F, class X, class Y, class Z, class R>
struct Thunk3 : public ThunkImpl<R> {
   mutable F f;
   mutable X x;
   mutable Y y;
   mutable Z z;
   Thunk3( const F& ff, const X& xx, const Y& yy, const Z& zz )
      : f(ff), x(xx), y(yy), z(zz) {}
   void init( const F& ff, const X& xx, const Y& yy, const Z& zz ) const {
      Maybe_Var_Inv<V1,F>::remake( f, ff );
      Maybe_Var_Inv<V2,X>::remake( x, xx );
      Maybe_Var_Inv<V3,Y>::remake( y, yy );
      Maybe_Var_Inv<V4,Z>::remake( z, zz );
   }
   R operator()() const {
      return Maybe_Var_Inv<V1,F>::clone(f)(
         Maybe_Var_Inv<V2,X>::clone(x),
         Maybe_Var_Inv<V3,Y>::clone(y),
         Maybe_Var_Inv<V4,Z>::clone(z),
         reuser3<V1,V2,V3,V4,F,X,Y,Z>(this) );
   }
};

template <class V1, class V2, class V3, class V4,
          class F, class X, class Y, class Z>
struct reuser3 {
   typedef typename F::template result<F(X,Y,Z)>::type R;
   typedef Thunk3<V1,V2,V3,V4,F,X,Y,Z,R> M;
   typedef M result_type;
   boost::intrusive_ptr<const M> ref;
   reuser3(a_unique_type_for_nil) {}
   reuser3(const M* m) : ref(m) {}
   M operator()( const F& f, const X& x, const Y& y, const Z& z ) {
      if( !ref )   ref = boost::intrusive_ptr<const M>( new M(f,x,y,z) );
      else         ref->init(f,x,y,z);
      return *ref;
   }
   void iter( const F& f, const X& x, const Y& y, const Z& z ) {
      if( ref )    ref->init(f,x,y,z);
   }
};
//////////////////////////////////////////////////////////////////////
// reuser4
//////////////////////////////////////////////////////////////////////

      template <class V1, class V2, class V3, class V4, class V5,
                class F, class W, class X, class Y, class Z>
struct reuser4;

      template <class V1, class V2, class V3, class V4, class V5,
                class F, class W, class X, class Y, class Z, class R>
struct Thunk4 : public ThunkImpl<R> {
      mutable F f;
      mutable W w;
      mutable X x;
      mutable Y y;
      mutable Z z;
      Thunk4( const F& ff, const W& ww, const X& xx, const Y& yy, const Z& zz )
            : f(ff), w(ww), x(xx), y(yy), z(zz) {}
   void init( const F& ff, const W& ww, const X& xx, const Y& yy, const Z& zz ) const {
      Maybe_Var_Inv<V1,F>::remake( f, ff );
      Maybe_Var_Inv<V2,W>::remake( w, ww );
      Maybe_Var_Inv<V3,X>::remake( x, xx );
      Maybe_Var_Inv<V4,Y>::remake( y, yy );
      Maybe_Var_Inv<V5,Z>::remake( z, zz );
   }
   R operator()() const {
      return Maybe_Var_Inv<V1,F>::clone(f)(
         Maybe_Var_Inv<V2,W>::clone(w),
         Maybe_Var_Inv<V3,X>::clone(x),
         Maybe_Var_Inv<V4,Y>::clone(y),
         Maybe_Var_Inv<V5,Z>::clone(z),
         reuser4<V1,V2,V3,V4,V5,F,W,X,Y,Z>(this) );
   }
};

      template <class V1, class V2, class V3, class V4, class V5,
                class F, class W, class X, class Y, class Z>
      struct reuser4 {
        typedef typename F::template result<F(W,X,Y,Z)>::type R;
        typedef Thunk4<V1,V2,V3,V4,V5,F,W,X,Y,Z,R> M;
        typedef M result_type;
        boost::intrusive_ptr<const M> ref;
        reuser4(a_unique_type_for_nil) {}
        reuser4(const M* m) : ref(m) {}
   M operator()( const F& f, const W& w, const X& x, const Y& y, const Z& z ) {
      if( !ref )   ref = boost::intrusive_ptr<const M>( new M(f,w,x,y,z) );
      else         ref->init(f,w,x,y,z);
      return *ref;
   }
   void iter( const F& f, const W& w, const X& x, const Y& y, const Z& z ) {
      if( ref )    ref->init(f,w,x,y,z);
   }
      };

    }

  }
}

#endif
