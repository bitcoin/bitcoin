/* Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_SCOPE_GUARD_HPP
#define BOOST_MULTI_INDEX_DETAIL_SCOPE_GUARD_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/core/no_exceptions_support.hpp>
#include <boost/mpl/if.hpp>

namespace boost{

namespace multi_index{

namespace detail{

/* Until some official version of the ScopeGuard idiom makes it into Boost,
 * we locally define our own. This is a merely reformated version of
 * ScopeGuard.h as defined in:
 *   Alexandrescu, A., Marginean, P.:"Generic<Programming>: Change the Way You
 *     Write Exception-Safe Code - Forever", C/C++ Users Jornal, Dec 2000,
 *     http://www.drdobbs.com/184403758
 * with the following modifications:
 *   - General pretty formatting (pretty to my taste at least.)
 *   - Naming style changed to standard C++ library requirements.
 *   - Added scope_guard_impl4 and obj_scope_guard_impl3, (Boost.MultiIndex
 *     needs them). A better design would provide guards for many more
 *     arguments through the Boost Preprocessor Library.
 *   - Added scope_guard_impl_base::touch (see below.)
 *   - Removed RefHolder and ByRef, whose functionality is provided
 *     already by Boost.Ref.
 *   - Removed static make_guard's and make_obj_guard's, so that the code
 *     will work even if BOOST_NO_MEMBER_TEMPLATES is defined. This forces
 *     us to move some private ctors to public, though.
 *
 * NB: CodeWarrior Pro 8 seems to have problems looking up safe_execute
 * without an explicit qualification.
 * 
 * We also define the following variants of the idiom:
 * 
 *   - make_guard_if_c<bool>( ... )
 *   - make_guard_if<IntegralConstant>( ... )
 *   - make_obj_guard_if_c<bool>( ... )
 *   - make_obj_guard_if<IntegralConstant>( ... )
 * which may be used with a compile-time constant to yield
 * a "null_guard" if the boolean compile-time parameter is false,
 * or conversely, the guard is only constructed if the constant is true.
 * This is useful to avoid extra tagging, because the returned
 * null_guard can be optimzed comlpetely away by the compiler.
 */

class scope_guard_impl_base
{
public:
  scope_guard_impl_base():dismissed_(false){}
  void dismiss()const{dismissed_=true;}

  /* This helps prevent some "unused variable" warnings under, for instance,
   * GCC 3.2.
   */
  void touch()const{}

protected:
  ~scope_guard_impl_base(){}

  scope_guard_impl_base(const scope_guard_impl_base& other):
    dismissed_(other.dismissed_)
  {
    other.dismiss();
  }

  template<typename J>
  static void safe_execute(J& j){
    BOOST_TRY{
      if(!j.dismissed_)j.execute();
    }
    BOOST_CATCH(...){}
    BOOST_CATCH_END
  }
  
  mutable bool dismissed_;

private:
  scope_guard_impl_base& operator=(const scope_guard_impl_base&);
};

typedef const scope_guard_impl_base& scope_guard;

struct null_guard : public scope_guard_impl_base
{
    template< class T1 >
    null_guard( const T1& )
    { }

    template< class T1, class T2 >
    null_guard( const T1&, const T2& )
    { }

    template< class T1, class T2, class T3 >
    null_guard( const T1&, const T2&, const T3& )
    { }

    template< class T1, class T2, class T3, class T4 >
    null_guard( const T1&, const T2&, const T3&, const T4& )
    { }

    template< class T1, class T2, class T3, class T4, class T5 >
    null_guard( const T1&, const T2&, const T3&, const T4&, const T5& )
    { }
};

template< bool cond, class T >
struct null_guard_return
{
    typedef typename boost::mpl::if_c<cond,T,null_guard>::type type;
};

template<typename F>
class scope_guard_impl0:public scope_guard_impl_base
{
public:
  scope_guard_impl0(F fun):fun_(fun){}
  ~scope_guard_impl0(){scope_guard_impl_base::safe_execute(*this);}
  void execute(){fun_();}

protected:

  F fun_;
};

template<typename F> 
inline scope_guard_impl0<F> make_guard(F fun)
{
  return scope_guard_impl0<F>(fun);
}

template<bool cond, typename F> 
inline typename null_guard_return<cond,scope_guard_impl0<F> >::type  
make_guard_if_c(F fun)
{
  return typename null_guard_return<cond,scope_guard_impl0<F> >::type(fun);
}

template<typename C, typename F> 
inline typename null_guard_return<C::value,scope_guard_impl0<F> >::type  
make_guard_if(F fun)
{
  return make_guard_if<C::value>(fun);
}

template<typename F,typename P1>
class scope_guard_impl1:public scope_guard_impl_base
{
public:
  scope_guard_impl1(F fun,P1 p1):fun_(fun),p1_(p1){}
  ~scope_guard_impl1(){scope_guard_impl_base::safe_execute(*this);}
  void execute(){fun_(p1_);}

protected:
  F        fun_;
  const P1 p1_;
};

template<typename F,typename P1> 
inline scope_guard_impl1<F,P1> make_guard(F fun,P1 p1)
{
  return scope_guard_impl1<F,P1>(fun,p1);
}

template<bool cond, typename F,typename P1> 
inline typename null_guard_return<cond,scope_guard_impl1<F,P1> >::type 
make_guard_if_c(F fun,P1 p1)
{
  return typename null_guard_return<cond,scope_guard_impl1<F,P1> >::type(fun,p1);
}

template<typename C, typename F,typename P1> 
inline typename null_guard_return<C::value,scope_guard_impl1<F,P1> >::type 
make_guard_if(F fun,P1 p1)
{
  return make_guard_if_c<C::value>(fun,p1);
}

template<typename F,typename P1,typename P2>
class scope_guard_impl2:public scope_guard_impl_base
{
public:
  scope_guard_impl2(F fun,P1 p1,P2 p2):fun_(fun),p1_(p1),p2_(p2){}
  ~scope_guard_impl2(){scope_guard_impl_base::safe_execute(*this);}
  void execute(){fun_(p1_,p2_);}

protected:
  F        fun_;
  const P1 p1_;
  const P2 p2_;
};

template<typename F,typename P1,typename P2>
inline scope_guard_impl2<F,P1,P2> make_guard(F fun,P1 p1,P2 p2)
{
  return scope_guard_impl2<F,P1,P2>(fun,p1,p2);
}

template<bool cond, typename F,typename P1,typename P2>
inline typename null_guard_return<cond,scope_guard_impl2<F,P1,P2> >::type
make_guard_if_c(F fun,P1 p1,P2 p2)
{
  return typename null_guard_return<cond,scope_guard_impl2<F,P1,P2> >::type(fun,p1,p2);
}

template<typename C, typename F,typename P1,typename P2>
inline typename null_guard_return<C::value,scope_guard_impl2<F,P1,P2> >::type
make_guard_if(F fun,P1 p1,P2 p2)
{
  return make_guard_if_c<C::value>(fun,p1,p2);
}

template<typename F,typename P1,typename P2,typename P3>
class scope_guard_impl3:public scope_guard_impl_base
{
public:
  scope_guard_impl3(F fun,P1 p1,P2 p2,P3 p3):fun_(fun),p1_(p1),p2_(p2),p3_(p3){}
  ~scope_guard_impl3(){scope_guard_impl_base::safe_execute(*this);}
  void execute(){fun_(p1_,p2_,p3_);}

protected:
  F        fun_;
  const P1 p1_;
  const P2 p2_;
  const P3 p3_;
};

template<typename F,typename P1,typename P2,typename P3>
inline scope_guard_impl3<F,P1,P2,P3> make_guard(F fun,P1 p1,P2 p2,P3 p3)
{
  return scope_guard_impl3<F,P1,P2,P3>(fun,p1,p2,p3);
}

template<bool cond,typename F,typename P1,typename P2,typename P3>
inline typename null_guard_return<cond,scope_guard_impl3<F,P1,P2,P3> >::type 
make_guard_if_c(F fun,P1 p1,P2 p2,P3 p3)
{
  return typename null_guard_return<cond,scope_guard_impl3<F,P1,P2,P3> >::type(fun,p1,p2,p3);
}

template<typename C,typename F,typename P1,typename P2,typename P3>
inline typename null_guard_return< C::value,scope_guard_impl3<F,P1,P2,P3> >::type 
make_guard_if(F fun,P1 p1,P2 p2,P3 p3)
{
  return make_guard_if_c<C::value>(fun,p1,p2,p3);
}

template<typename F,typename P1,typename P2,typename P3,typename P4>
class scope_guard_impl4:public scope_guard_impl_base
{
public:
  scope_guard_impl4(F fun,P1 p1,P2 p2,P3 p3,P4 p4):
    fun_(fun),p1_(p1),p2_(p2),p3_(p3),p4_(p4){}
  ~scope_guard_impl4(){scope_guard_impl_base::safe_execute(*this);}
  void execute(){fun_(p1_,p2_,p3_,p4_);}

protected:
  F        fun_;
  const P1 p1_;
  const P2 p2_;
  const P3 p3_;
  const P4 p4_;
};

template<typename F,typename P1,typename P2,typename P3,typename P4>
inline scope_guard_impl4<F,P1,P2,P3,P4> make_guard(
  F fun,P1 p1,P2 p2,P3 p3,P4 p4)
{
  return scope_guard_impl4<F,P1,P2,P3,P4>(fun,p1,p2,p3,p4);
}

template<bool cond, typename F,typename P1,typename P2,typename P3,typename P4>
inline typename null_guard_return<cond,scope_guard_impl4<F,P1,P2,P3,P4> >::type 
make_guard_if_c(
  F fun,P1 p1,P2 p2,P3 p3,P4 p4)
{
  return typename null_guard_return<cond,scope_guard_impl4<F,P1,P2,P3,P4> >::type(fun,p1,p2,p3,p4);
}

template<typename C, typename F,typename P1,typename P2,typename P3,typename P4>
inline typename null_guard_return<C::value,scope_guard_impl4<F,P1,P2,P3,P4> >::type 
make_guard_if(
  F fun,P1 p1,P2 p2,P3 p3,P4 p4)
{
  return make_guard_if_c<C::value>(fun,p1,p2,p3,p4);
}

template<class Obj,typename MemFun>
class obj_scope_guard_impl0:public scope_guard_impl_base
{
public:
  obj_scope_guard_impl0(Obj& obj,MemFun mem_fun):obj_(obj),mem_fun_(mem_fun){}
  ~obj_scope_guard_impl0(){scope_guard_impl_base::safe_execute(*this);}
  void execute(){(obj_.*mem_fun_)();}

protected:
  Obj&   obj_;
  MemFun mem_fun_;
};

template<class Obj,typename MemFun>
inline obj_scope_guard_impl0<Obj,MemFun> make_obj_guard(Obj& obj,MemFun mem_fun)
{
  return obj_scope_guard_impl0<Obj,MemFun>(obj,mem_fun);
}

template<bool cond, class Obj,typename MemFun>
inline typename null_guard_return<cond,obj_scope_guard_impl0<Obj,MemFun> >::type 
make_obj_guard_if_c(Obj& obj,MemFun mem_fun)
{
  return typename null_guard_return<cond,obj_scope_guard_impl0<Obj,MemFun> >::type(obj,mem_fun);
}

template<typename C, class Obj,typename MemFun>
inline typename null_guard_return<C::value,obj_scope_guard_impl0<Obj,MemFun> >::type 
make_obj_guard_if(Obj& obj,MemFun mem_fun)
{
  return make_obj_guard_if_c<C::value>(obj,mem_fun);
}

template<class Obj,typename MemFun,typename P1>
class obj_scope_guard_impl1:public scope_guard_impl_base
{
public:
  obj_scope_guard_impl1(Obj& obj,MemFun mem_fun,P1 p1):
    obj_(obj),mem_fun_(mem_fun),p1_(p1){}
  ~obj_scope_guard_impl1(){scope_guard_impl_base::safe_execute(*this);}
  void execute(){(obj_.*mem_fun_)(p1_);}

protected:
  Obj&     obj_;
  MemFun   mem_fun_;
  const P1 p1_;
};

template<class Obj,typename MemFun,typename P1>
inline obj_scope_guard_impl1<Obj,MemFun,P1> make_obj_guard(
  Obj& obj,MemFun mem_fun,P1 p1)
{
  return obj_scope_guard_impl1<Obj,MemFun,P1>(obj,mem_fun,p1);
}

template<bool cond, class Obj,typename MemFun,typename P1>
inline typename null_guard_return<cond,obj_scope_guard_impl1<Obj,MemFun,P1> >::type 
make_obj_guard_if_c(  Obj& obj,MemFun mem_fun,P1 p1)
{
  return typename null_guard_return<cond,obj_scope_guard_impl1<Obj,MemFun,P1> >::type(obj,mem_fun,p1);
}

template<typename C, class Obj,typename MemFun,typename P1>
inline typename null_guard_return<C::value,obj_scope_guard_impl1<Obj,MemFun,P1> >::type 
make_obj_guard_if( Obj& obj,MemFun mem_fun,P1 p1)
{
  return make_obj_guard_if_c<C::value>(obj,mem_fun,p1);
}

template<class Obj,typename MemFun,typename P1,typename P2>
class obj_scope_guard_impl2:public scope_guard_impl_base
{
public:
  obj_scope_guard_impl2(Obj& obj,MemFun mem_fun,P1 p1,P2 p2):
    obj_(obj),mem_fun_(mem_fun),p1_(p1),p2_(p2)
  {}
  ~obj_scope_guard_impl2(){scope_guard_impl_base::safe_execute(*this);}
  void execute(){(obj_.*mem_fun_)(p1_,p2_);}

protected:
  Obj&     obj_;
  MemFun   mem_fun_;
  const P1 p1_;
  const P2 p2_;
};

template<class Obj,typename MemFun,typename P1,typename P2>
inline obj_scope_guard_impl2<Obj,MemFun,P1,P2>
make_obj_guard(Obj& obj,MemFun mem_fun,P1 p1,P2 p2)
{
  return obj_scope_guard_impl2<Obj,MemFun,P1,P2>(obj,mem_fun,p1,p2);
}

template<bool cond, class Obj,typename MemFun,typename P1,typename P2>
inline typename null_guard_return<cond,obj_scope_guard_impl2<Obj,MemFun,P1,P2> >::type
make_obj_guard_if_c(Obj& obj,MemFun mem_fun,P1 p1,P2 p2)
{
  return typename null_guard_return<cond,obj_scope_guard_impl2<Obj,MemFun,P1,P2> >::type(obj,mem_fun,p1,p2);
}

template<typename C, class Obj,typename MemFun,typename P1,typename P2>
inline typename null_guard_return<C::value,obj_scope_guard_impl2<Obj,MemFun,P1,P2> >::type
make_obj_guard_if(Obj& obj,MemFun mem_fun,P1 p1,P2 p2)
{
  return make_obj_guard_if_c<C::value>(obj,mem_fun,p1,p2);
}

template<class Obj,typename MemFun,typename P1,typename P2,typename P3>
class obj_scope_guard_impl3:public scope_guard_impl_base
{
public:
  obj_scope_guard_impl3(Obj& obj,MemFun mem_fun,P1 p1,P2 p2,P3 p3):
    obj_(obj),mem_fun_(mem_fun),p1_(p1),p2_(p2),p3_(p3)
  {}
  ~obj_scope_guard_impl3(){scope_guard_impl_base::safe_execute(*this);}
  void execute(){(obj_.*mem_fun_)(p1_,p2_,p3_);}

protected:
  Obj&     obj_;
  MemFun   mem_fun_;
  const P1 p1_;
  const P2 p2_;
  const P3 p3_;
};

template<class Obj,typename MemFun,typename P1,typename P2,typename P3>
inline obj_scope_guard_impl3<Obj,MemFun,P1,P2,P3>
make_obj_guard(Obj& obj,MemFun mem_fun,P1 p1,P2 p2,P3 p3)
{
  return obj_scope_guard_impl3<Obj,MemFun,P1,P2,P3>(obj,mem_fun,p1,p2,p3);
}

template<bool cond, class Obj,typename MemFun,typename P1,typename P2,typename P3>
inline typename null_guard_return<cond,obj_scope_guard_impl3<Obj,MemFun,P1,P2,P3> >::type
make_obj_guard_if_c(Obj& obj,MemFun mem_fun,P1 p1,P2 p2,P3 p3)
{
  return typename null_guard_return<cond,obj_scope_guard_impl3<Obj,MemFun,P1,P2,P3> >::type(obj,mem_fun,p1,p2,p3);
}

template<typename C, class Obj,typename MemFun,typename P1,typename P2,typename P3>
inline typename null_guard_return<C::value,obj_scope_guard_impl3<Obj,MemFun,P1,P2,P3> >::type
make_obj_guard_if(Obj& obj,MemFun mem_fun,P1 p1,P2 p2,P3 p3)
{
  return make_obj_guard_if_c<C::value>(obj,mem_fun,p1,p2,p3);
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
