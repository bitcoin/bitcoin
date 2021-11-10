////////////////////////////////////////////////////////////////////////////
// lazy_list.hpp
//
// Build lazy operations for Phoenix equivalents for FC++
//
// These are equivalents of the Boost FC++ functoids in list.hpp
//
// Implemented so far:
//
// head tail null
//
// strict_list<T> and associated iterator.
//
// list<T> and odd_list<T>
//
// cons cat
//
// Comparisons between list and odd_list types and separately for strict_list.
//
// NOTES: There is a fix at the moment as I have not yet sorted out
//        how to get back the return type of a functor returning a list type.
//        For the moment I have fixed this as odd_list<T> at two locations,
//        one in list<T> and one in Cons. I am going to leave it like this
//        for now as reading the code, odd_list<T> seems to be correct.
//
//        I am also not happy at the details needed to detect types in Cons.
//
// I think the structure of this file is now complete.
// John Fletcher  February 2015.
//
////////////////////////////////////////////////////////////////////////////
/*=============================================================================
    Copyright (c) 2000-2003 Brian McNamara and Yannis Smaragdakis
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2015 John Fletcher

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

///////////////////////////////////////////////////////////////////////
// This is from Boost FC++ list.hpp reimplemented without Fun0 or Full0
///////////////////////////////////////////////////////////////////////

/*
concept ListLike: Given a list representation type L

L<T> inherits ListLike and has
   // typedefs just show typical values
   typedef T value_type
   typedef L<T> force_result_type
   typedef L<T> delay_result_type
   typedef L<T> tail_result_type
   template <class UU> struct cons_rebind {
      typedef L<UU> type;        // force type
      typedef L<UU> delay_type;  // delay type
   };

   L()
   L( a_unique_type_for_nil )
   template <class F> L(F)       // F :: ()->L

   constructor: force_result_type( T, L<T> )
   template <class F>
   constructor: force_result_type( T, F )  // F :: ()->L

   template <class It>
   L( It, It )

   // FIX THIS instead of operator bool(), does Boost have something better?
   operator bool() const
   force_result_type force() const
   delay_result_type delay() const
   T head() const
   tail_result_type tail() const

   static const bool is_lazy;   // true if can represent infinite lists

   typedef const_iterator;
   typedef const_iterator iterator;  // ListLikes are immutable
   iterator begin() const
   iterator end() const
*/

//////////////////////////////////////////////////////////////////////
// End of section from Boost FC++ list.hpp
//////////////////////////////////////////////////////////////////////

#ifndef BOOST_PHOENIX_FUNCTION_LAZY_LIST
#define BOOST_PHOENIX_FUNCTION_LAZY_LIST

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/function.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
//#include "lazy_reuse.hpp"

namespace boost {

  namespace phoenix {

//////////////////////////////////////////////////////////////////////
// These are the list types being declared.
//////////////////////////////////////////////////////////////////////

   template <class T> class strict_list;
    namespace impl {
   template <class T> class list;
   template <class T> class odd_list;
    }
   // in ref_count.hpp in BoostFC++ now in lazy_operator.hpp
   //typedef unsigned int RefCountType;

//////////////////////////////////////////////////////////////////////
// a_unique_type_for_nil moved to lazy_operator.hpp.
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Distinguish lazy lists (list and odd_list) from strict_list.
//////////////////////////////////////////////////////////////////////

    namespace lazy {
      // Copied from Boost FC++ list.hpp
      template <class L, bool is_lazy> struct ensure_lazy_helper {};
      template <class L> struct ensure_lazy_helper<L,true> {
         static void requires_lazy_list_to_prevent_infinite_recursion() {}
      };
      template <class L>
      void ensure_lazy() {
        ensure_lazy_helper<L,L::is_lazy>::
        requires_lazy_list_to_prevent_infinite_recursion();
      }

    }

//////////////////////////////////////////////////////////////////////
// Provide remove reference for types defined for list types.
//////////////////////////////////////////////////////////////////////

    namespace result_of {

      template < typename L >
      class ListType
      {
      public:
        typedef typename boost::remove_reference<L>::type LType;
        typedef typename LType::value_type value_type;
        typedef typename LType::tail_result_type tail_result_type;
        typedef typename LType::force_result_type force_result_type;
        typedef typename LType::delay_result_type delay_result_type;
      };

      template <>
      class ListType<a_unique_type_for_nil>
      {
      public:
        typedef a_unique_type_for_nil LType;
        //typedef a_unique_type_for_nil value_type;
      };

      template <typename F, typename T>
      struct ResultType {
        typedef typename impl::odd_list<T> type;
      };
  
    }

//////////////////////////////////////////////////////////////////////
// ListLike is a property inherited by any list type to enable it to
// work with the functions being implemented in this file.
// It provides the check for the structure described above.
//////////////////////////////////////////////////////////////////////

   namespace listlike {

       struct ListLike {};   // This lets us use is_base_and_derived() to see
       // (at compile-time) what classes are user-defined lists.

 
       template <class L, bool is_lazy> struct ensure_lazy_helper {};
       template <class L> struct ensure_lazy_helper<L,true> {
           static void requires_lazy_list_to_prevent_infinite_recursion() {}
       };
       template <class L>
       void ensure_lazy() {
         ensure_lazy_helper<L,L::is_lazy>::
         requires_lazy_list_to_prevent_infinite_recursion();
       }

       template <class L, bool b>
       struct EnsureListLikeHelp {
           static void trying_to_call_a_list_function_on_a_non_list() {}
       };
       template <class L> struct EnsureListLikeHelp<L,false> { };
       template <class L>
       void EnsureListLike() {
          typedef typename result_of::ListType<L>::LType LType;
          EnsureListLikeHelp<L,boost::is_base_and_derived<ListLike,LType>::value>::
             trying_to_call_a_list_function_on_a_non_list();
       }

      template <class L>
      bool is_a_unique_type_for_nil(const L& /*l*/) {
         return false;
      }

      template <>
      bool is_a_unique_type_for_nil<a_unique_type_for_nil>
      (const a_unique_type_for_nil& /* n */) {
         return true;
      }

      template <class L>
      struct detect_nil {
        static const bool is_nil = false;
      };

      template <>
      struct detect_nil<a_unique_type_for_nil> {
       static const bool is_nil = true;
      };

      template <>
      struct detect_nil<a_unique_type_for_nil&> {
       static const bool is_nil = true;
      };

      template <>
      struct detect_nil<const a_unique_type_for_nil&> {
       static const bool is_nil = true;
      };
          
    }

//////////////////////////////////////////////////////////////////////
// Implement lazy functions for list types. cat and cons come later.
//////////////////////////////////////////////////////////////////////

#ifndef BOOST_PHOENIX_FUNCTION_MAX_LAZY_LIST_LENGTH
#define BOOST_PHOENIX_FUNCTION_MAX_LAZY_LIST_LENGTH 1000
#endif

    namespace impl {

      struct Head
      {
        template <typename Sig>
        struct result;

        template <typename This, typename L>
        struct result<This(L)>
        {
          typedef typename result_of::ListType<L>::value_type type;
        };

        template <typename L>
        typename result<Head(L)>::type
        operator()(const L & l) const
        {
          listlike::EnsureListLike<L>();
          return l.head();
        }

      };

      struct Tail
      {
        template <typename Sig>
        struct result;

        template <typename This, typename L>
        struct result<This(L)>
        {
          typedef typename result_of::ListType<L>::tail_result_type type;
        };

        template <typename L>
        typename result<Tail(L)>::type
        operator()(const L & l) const
        {
          listlike::EnsureListLike<L>();
          return l.tail();
        }

      };

      struct Null
      {
        template <typename Sig>
        struct result;

        template <typename This, typename L>
        struct result<This(L)>
        {
            typedef bool type;
        };

        template <typename L>
        typename result<Null(L)>::type
        //bool
        operator()(const L& l) const
        {
          listlike::EnsureListLike<L>();
          return !l;
        }

      };

      struct Delay {
        template <typename Sig>
        struct result;

        template <typename This, typename L>
        struct result<This(L)>
        {
          typedef typename result_of::ListType<L>::delay_result_type type;
        };

        template <typename L>
        typename result<Delay(L)>::type
        operator()(const L & l) const
        {
          listlike::EnsureListLike<L>();
          return l.delay();
        }

      };

      struct Force {
        template <typename Sig>
        struct result;

        template <typename This, typename L>
        struct result<This(L)>
        {
          typedef typename result_of::ListType<L>::force_result_type type;
        };

        template <typename L>
        typename result<Force(L)>::type
        operator()(const L & l) const
        {
          listlike::EnsureListLike<L>();
          return l.force();
        }

      };

    }
    //BOOST_PHOENIX_ADAPT_CALLABLE(head, impl::head, 1)
    //BOOST_PHOENIX_ADAPT_CALLABLE(tail, impl::tail, 1)
    //BOOST_PHOENIX_ADAPT_CALLABLE(null, impl::null, 1)
    typedef boost::phoenix::function<impl::Head> Head;
    typedef boost::phoenix::function<impl::Tail> Tail;
    typedef boost::phoenix::function<impl::Null> Null;
    typedef boost::phoenix::function<impl::Delay> Delay;
    typedef boost::phoenix::function<impl::Force> Force;
    Head head;
    Tail tail;
    Null null;
    Delay delay;
    Force force;

//////////////////////////////////////////////////////////////////////
// These definitions used for strict_list are imported from BoostFC++
// unchanged.
//////////////////////////////////////////////////////////////////////

namespace impl {
template <class T>
struct strict_cons : public boost::noncopyable {
   mutable RefCountType refC;
   T head;
   typedef boost::intrusive_ptr<strict_cons> tail_type;
   tail_type tail;
   strict_cons( const T& h, const tail_type& t ) : refC(0), head(h), tail(t) {}

};
template <class T>
void intrusive_ptr_add_ref( const strict_cons<T>* p ) {
   ++ (p->refC);
}
template <class T>
void intrusive_ptr_release( const strict_cons<T>* p ) {
   if( !--(p->refC) ) delete p;
}

template <class T>
class strict_list_iterator {
   typedef boost::intrusive_ptr<strict_cons<T> > rep_type;
   rep_type l;
   bool is_nil;
   void advance() {
      l = l->tail;
      if( !l )
         is_nil = true;
   }
   class Proxy {  // needed for operator->
      const T x;
      friend class strict_list_iterator;
      Proxy( const T& xx ) : x(xx) {}
   public:
      const T* operator->() const { return &x; }
   };
public:
   typedef std::input_iterator_tag iterator_category;
   typedef T value_type;
   typedef ptrdiff_t difference_type;
   typedef T* pointer;
   typedef T& reference;

   strict_list_iterator() : l(), is_nil(true) {}
   explicit strict_list_iterator( const rep_type& ll ) : l(ll), is_nil(!ll) {}
   
   const T operator*() const { return l->head; }
   const Proxy operator->() const { return Proxy(l->head); }
   strict_list_iterator<T>& operator++() {
      advance();
      return *this;
   }
   const strict_list_iterator<T> operator++(int) {
      strict_list_iterator<T> i( *this );
      advance();
      return i;
   }
   bool operator==( const strict_list_iterator<T>& i ) const {
      return is_nil && i.is_nil;
   }
   bool operator!=( const strict_list_iterator<T>& i ) const {
      return ! this->operator==(i);
   }
};
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

   template <class T>
   class strict_list : public listlike::ListLike
   {
       typedef boost::intrusive_ptr<impl::strict_cons<T> > rep_type;
       rep_type rep;
       struct Make {};

       template <class Iter>
       static rep_type help( Iter a, const Iter& b ) {
           rep_type r;
           while( a != b ) {
              T x( *a );
              r = rep_type( new impl::strict_cons<T>( x, r ) );
              ++a;
           }
           return r;
       }

   public:
       static const bool is_lazy = false;

       typedef T value_type;
       typedef strict_list force_result_type;
       typedef strict_list delay_result_type;
       typedef strict_list tail_result_type;
       template <class UU> struct cons_rebind {
           typedef strict_list<UU> type;
           typedef strict_list<UU> delay_type;
       };
 

   strict_list( Make, const rep_type& r ) : rep(r) {}

   strict_list() : rep() {}

   strict_list( a_unique_type_for_nil ) : rep() {}

   template <class F>
   strict_list( const F& f ) : rep( f().rep ) {
     // I cannot do this yet.
     //functoid_traits<F>::template ensure_accepts<0>::args();
   }

   strict_list( const T& x, const strict_list& y )
      : rep( new impl::strict_cons<T>(x,y.rep) ) {}

   template <class F>
   strict_list( const T& x, const F& f )
      : rep( new impl::strict_cons<T>(x,f().rep) ) {}
   
     operator bool() const { return (bool)rep; }
   force_result_type force() const { return *this; }
   delay_result_type delay() const { return *this; }
   T head() const {
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
      if( !*this )
         throw lazy_exception("Tried to take head() of empty strict_list");
#endif
      return rep->head;
   }
   tail_result_type tail() const {
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
      if( !*this )
         throw lazy_exception("Tried to take tail() of empty strict_list");
#endif
      return strict_list(Make(),rep->tail);
   }

   template <class Iter>
   strict_list( const Iter& a, const Iter& b ) : rep( rep_type() ) {
      // How ironic.  We need to reverse the iterator range in order to
      // non-recursively build this!
      std::vector<T> tmp(a,b);
      rep = help( tmp.rbegin(), tmp.rend() );
   }

   // Since the strict_cons destructor can't call the strict_list
   // destructor, the "simple" iterative destructor is correct and
   // efficient.  Hurray.
   ~strict_list() { while(rep && (rep->refC == 1)) rep = rep->tail; }

   // The following helps makes strict_list almost an STL "container"
   typedef impl::strict_list_iterator<T> const_iterator;
   typedef const_iterator iterator;         // strict_list is immutable
   iterator begin() const { return impl::strict_list_iterator<T>( rep ); }
   iterator end() const   { return impl::strict_list_iterator<T>(); }

   };

    // All of these null head and tail are now non lazy using e.g. null(a)().
    // They need an extra () e.g. null(a)().
    template <class T>
    bool operator==( const strict_list<T>& a, a_unique_type_for_nil ) {
      return null(a)();
    }
    template <class T>
    bool operator==( a_unique_type_for_nil, const strict_list<T>& a ) {
      return null(a)();
    }
    template <class T>
    bool operator==( const strict_list<T>& a, const strict_list<T>& b ) {
        if( null(a)() && null(b)() )
            return true;
        if( null(a)() || null(b)() )
            return false;
        return (head(a)()==head(b)()) &&
            (tail(a)()==tail(b)());
    }

    template <class T>
    bool operator<( const strict_list<T>& a, const strict_list<T>& b ) {
      if( null(a)() && !null(b)() )  return true;
        if( null(b)() )                      return false;
        if( head(b)() < head(a)() )    return false;
        if( head(a)() < head(b)() )    return true;
        return (tail(a)() < tail(b)());
    }
    template <class T>
    bool operator<( const strict_list<T>&, a_unique_type_for_nil ) {
        return false;
    }
    template <class T>
    bool operator<( a_unique_type_for_nil, const strict_list<T>& b ) {
      return !(null(b)());
    }

//////////////////////////////////////////////////////////////////////
// Class list<T> is the primary interface to the user for lazy lists.
//////////////////////////////////////////////////////////////////////{
    namespace impl {
      using fcpp::INV;
      using fcpp::VAR;
      using fcpp::reuser2;

      struct CacheEmpty {};

      template <class T> class Cache;
      template <class T> class odd_list;
      template <class T> class list_iterator;
      template <class It, class T>
      struct ListItHelp2 /*: public c_fun_type<It,It,odd_list<T> >*/ {
        // This will need a return type.
        typedef odd_list<T> return_type;
        odd_list<T> operator()( It begin, const It& end,
          reuser2<INV,VAR,INV,ListItHelp2<It,T>,It,It> r = NIL ) const;
      };
      template <class U,class F> struct cvt;
      template <class T, class F, class R> struct ListHelp;
      template <class T> Cache<T>* xempty_helper();
      template <class T, class F, class L, bool b> struct ConsHelp2;

      struct ListRaw {};

      template <class T>
      class list : public listlike::ListLike
      {
        // never NIL, unless an empty odd_list
        boost::intrusive_ptr<Cache<T> > rep;

        template <class U> friend class Cache;
        template <class U> friend class odd_list;
        template <class TT, class F, class L, bool b> friend struct ConsHelp2;
        template <class U,class F> friend struct cvt;

        list( const boost::intrusive_ptr<Cache<T> >& p ) : rep(p) { }
        list( ListRaw, Cache<T>* p ) : rep(p) { }

        bool priv_isEmpty() const {
          return rep->cache().second.rep == Cache<T>::XNIL();
        }
        T priv_head() const {
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
          if( priv_isEmpty() )
             throw lazy_exception("Tried to take head() of empty list");
#endif
          return rep->cache().first();
        }
        list<T> priv_tail() const {
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
          if( priv_isEmpty() )
            throw lazy_exception("Tried to take tail() of empty list");
#endif
          return rep->cache().second;
        }


      public:
        static const bool is_lazy = true;

        typedef T value_type;
        typedef list tail_result_type;
        typedef odd_list<T> force_result_type;
        typedef list delay_result_type;
        template <class UU> struct cons_rebind {
           typedef odd_list<UU> type;
           typedef list<UU> delay_type;
        };

        list( a_unique_type_for_nil ) : rep( Cache<T>::XEMPTY() ) { }
        list() : rep( Cache<T>::XEMPTY() ) { }

        template <class F>  // works on both ()->odd_list and ()->list
        // At the moment this is fixed for odd_list<T>.
        // I need to do more work to get the general result.
        list( const F& f )
          : rep( ListHelp<T,F,odd_list<T> >()(f) ) { }
        //: rep( ListHelp<T,F,typename F::result_type>()(f) ) { }

        operator bool() const { return !priv_isEmpty(); }
        const force_result_type& force() const { return rep->cache(); }
        const delay_result_type& delay() const { return *this; }
        // Note: force returns a reference;
        // implicit conversion now returns a copy.
        operator odd_list<T>() const { return force(); }

        T head() const { return priv_head(); }
        tail_result_type tail() const { return priv_tail(); }

        // The following helps makes list almost an STL "container"
        typedef list_iterator<T> const_iterator;
        typedef const_iterator iterator;         // list is immutable
        iterator begin() const { return list_iterator<T>( *this ); }
        iterator end() const   { return list_iterator<T>(); }

        // end of list<T>
      };

//////////////////////////////////////////////////////////////////////
// Class odd_list<T> is not normally accessed by the user.
//////////////////////////////////////////////////////////////////////

      struct OddListDummyY {};

      template <class T>
      class odd_list : public listlike::ListLike
      {
      public:
        typedef
        typename boost::type_with_alignment<boost::alignment_of<T>::value>::type
        xfst_type;
      private:
        union { xfst_type fst; unsigned char dummy[sizeof(T)]; };

        const T& first() const {
           return *static_cast<const T*>(static_cast<const void*>(&fst));
        }
        T& first() {
           return *static_cast<T*>(static_cast<void*>(&fst));
        }
        list<T>  second;   // If XNIL, then this odd_list is NIL

        template <class U> friend class list;
        template <class U> friend class Cache;

        odd_list( OddListDummyY )
          : second( Cache<T>::XBAD() ) { }

        void init( const T& x ) {
           new (static_cast<void*>(&fst)) T(x);
        }

        bool fst_is_valid() const {
           if( second.rep != Cache<T>::XNIL() )
              if( second.rep != Cache<T>::XBAD() )
                 return true;
           return false;
        }

        bool priv_isEmpty() const { return second.rep == Cache<T>::XNIL(); }
        T priv_head() const {
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
           if( priv_isEmpty() )
             throw lazy_exception("Tried to take head() of empty odd_list");
#endif
           return first();
        }

        list<T> priv_tail() const {
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
           if( priv_isEmpty() )
             throw lazy_exception("Tried to take tail() of empty odd_list");
#endif
           return second;
        }

      public:
        static const bool is_lazy = true;

        typedef T value_type;
        typedef list<T> tail_result_type;
        typedef odd_list<T> force_result_type;
        typedef list<T> delay_result_type;
        template <class UU> struct cons_rebind {
          typedef odd_list<UU> type;
          typedef list<UU> delay_type;
        };

        odd_list() : second( Cache<T>::XNIL() ) { }
        odd_list( a_unique_type_for_nil ) : second( Cache<T>::XNIL() ) { }
        odd_list( const T& x, const list<T>& y ) : second(y) { init(x); }
        odd_list( const T& x, a_unique_type_for_nil ) : second(NIL) { init(x); }

        odd_list( const odd_list<T>& x ) : second(x.second) {
           if( fst_is_valid() ) {
              init( x.first() );
           }
        }

        template <class It>
        odd_list( It begin, const It& end )
          : second( begin==end ? Cache<T>::XNIL() :
             ( init(*begin++), list<T>( begin, end ) ) ) {}

        odd_list<T>& operator=( const odd_list<T>& x ) {
           if( this == &x ) return *this;
           if( fst_is_valid() ) {
             if( x.fst_is_valid() )
               first() = x.first();
             else
               first().~T();
           }
           else {
              if( x.fst_is_valid() )
                 init( x.first() );
           }
           second = x.second;
           return *this;
        }
      
       ~odd_list() {
          if( fst_is_valid() ) {
            first().~T();
          }
        }

        operator bool() const { return !priv_isEmpty(); }
        const force_result_type& force() const { return *this; }
        delay_result_type delay() const { return list<T>(*this); }

        T head() const { return priv_head(); }
        tail_result_type tail() const { return priv_tail(); }

        // The following helps makes odd_list almost an STL "container"
        typedef list_iterator<T> const_iterator;
        typedef const_iterator iterator;         // odd_list is immutable
        iterator begin() const { return list_iterator<T>( this->delay() ); }
        iterator end() const   { return list_iterator<T>(); }

        // end of odd_list<T>
      };

//////////////////////////////////////////////////////////////////////
// struct cvt
//////////////////////////////////////////////////////////////////////

      // This converts ()->list<T> to ()->odd_list<T>.
      // In other words, here is the 'extra work' done when using the
      // unoptimized interface.
      template <class U,class F>
      struct cvt /*: public c_fun_type<odd_list<U> >*/ {
        typedef odd_list<U> return_type;
        F f;
        cvt( const F& ff ) : f(ff) {}
        odd_list<U> operator()() const {
           list<U> l = f();
           return l.force();
        }
      };


//////////////////////////////////////////////////////////////////////
// Cache<T> and associated functions.
//////////////////////////////////////////////////////////////////////

// I malloc a RefCountType to hold the refCount and init it to 1 to ensure the
// refCount will never get to 0, so the destructor-of-global-object
// order at the end of the program is a non-issue.  In other words, the
// memory allocated here is only reclaimed by the operating system.
    template <class T>
    Cache<T>* xnil_helper() {
       void *p = std::malloc( sizeof(RefCountType) );
       *((RefCountType*)p) = 1;
       return static_cast<Cache<T>*>( p );
    }

    template <class T>
    Cache<T>* xnil_helper_nil() {
       Cache<T>* p = xnil_helper<T>();
       return p;
    }

    template <class T>
    Cache<T>* xnil_helper_bad() {
       Cache<T>* p = xnil_helper<T>();
       return p;
    }

    template <class T>
    Cache<T>* xempty_helper() {
       Cache<T>* p = new Cache<T>( CacheEmpty() );
       return p;
    }

    // This makes a boost phoenix function type with return type
    // odd_list<T>
    template <class T>
    struct fun0_type_helper{
       typedef boost::function0<odd_list<T> > fun_type;
       typedef boost::phoenix::function<fun_type> phx_type;
    };


      template <class T>
      struct make_fun0_odd_list {

        typedef typename fun0_type_helper<T>::fun_type fun_type;
        typedef typename fun0_type_helper<T>::phx_type phx_type;
        typedef phx_type result_type;

        template <class F>
        result_type operator()(const F& f) const
        {
            fun_type ff(f);
            phx_type g(ff);
            return g;
        }

        // Overload for the case where it is a boost phoenix function already.
        template <class F>
        typename boost::phoenix::function<F> operator()
          (const boost::phoenix::function<F>& f) const
        {
            return f;
        }

    };

    template <class T>
    class Cache : boost::noncopyable {
       mutable RefCountType refC;
       // This is the boost::function type
       typedef typename fun0_type_helper<T>::fun_type fun_odd_list_T;
       // This is the boost::phoenix::function type;
       typedef typename fun0_type_helper<T>::phx_type fun0_odd_list_T;
       mutable fun0_odd_list_T fxn;
       mutable odd_list<T>     val;
       // val.second.rep can be XBAD, XNIL, or a valid ptr
       //  - XBAD: val is invalid (fxn is valid)
       //  - XNIL: this is the empty list
       //  - anything else: val.first() is head, val.second is tail()

       // This functoid should never be called; it represents a
       // self-referent Cache, which should be impossible under the current
       // implementation.  Nonetheless, we need a 'dummy' function object to
       // represent invalid 'fxn's (val.second.rep!=XBAD), and this
       // implementation seems to be among the most reasonable.
       struct blackhole_helper /*: c_fun_type< odd_list<T> >*/ {
          typedef odd_list<T> return_type;
          odd_list<T> operator()() const {
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
            throw lazy_exception("You have entered a black hole.");
#else
            return odd_list<T>();
#endif
          }
       };

       // Don't get rid of these XFOO() functions; they impose no overhead,
       // and provide a useful place to add debugging code for tracking down
       // before-main()-order-of-initialization problems.
       static const boost::intrusive_ptr<Cache<T> >& XEMPTY() {
          static boost::intrusive_ptr<Cache<T> > xempty( xempty_helper<T>() );
          return xempty;
       }
       static const boost::intrusive_ptr<Cache<T> >& XNIL() {
       // this list is nil
          static boost::intrusive_ptr<Cache<T> > xnil( xnil_helper_nil<T>() );
          return xnil;
       }

       static const boost::intrusive_ptr<Cache<T> >& XBAD() {
       // the pair is invalid; use fxn
          static boost::intrusive_ptr<Cache<T> > xbad( xnil_helper_bad<T>() );
          return xbad;
       }

       static fun0_odd_list_T /*<odd_list<T> >*/ the_blackhole;
       static fun0_odd_list_T& blackhole() {
         static fun0_odd_list_T the_blackhole;
         //( make_fun0_odd_list<T>()( blackhole_helper() ) );
         return the_blackhole;
       }

       odd_list<T>& cache() const {
         if( val.second.rep == XBAD() ) {
            val = fxn()();
            fxn = blackhole();
         }
         return val;
       }

       template <class U> friend class list;
       template <class U> friend class odd_list;
       template <class TT, class F, class L, bool b> friend struct ConsHelp2;
       template <class U,class F> friend struct cvt;
       template <class U, class F, class R> friend struct ListHelp;
       template <class U> friend Cache<U>* xempty_helper();

       Cache( CacheEmpty ) : refC(0), fxn(blackhole()), val() {}
       Cache( const odd_list<T>& x ) : refC(0), fxn(blackhole()), val(x) {}
       Cache( const T& x, const list<T>& l ) : refC(0),fxn(blackhole()),val(x,l)
          {}

       Cache( const fun0_odd_list_T& f )
         : refC(0), fxn(f), val( OddListDummyY() ) {}

       // f must be a boost phoenix function object?
       template <class F>
       Cache( const F& f )    // ()->odd_list
         : refC(0), fxn(make_fun0_odd_list<T>()(f)), val( OddListDummyY() ) {}

       // This is for ()->list<T> to ()->odd_list<T>
       struct CvtFxn {};
       template <class F>
       Cache( CvtFxn, const F& f )    // ()->list
         :  refC(0), fxn(make_fun0_odd_list<T>()(cvt<T,F>(f))), val( OddListDummyY() ) {}

       template <class X>
       friend void intrusive_ptr_add_ref( const Cache<X>* p );
       template <class X>
       friend void intrusive_ptr_release( const Cache<X>* p );
    };

    template <class T>
    void intrusive_ptr_add_ref( const Cache<T>* p ) {
        ++ (p->refC);
    }
    template <class T>
    void intrusive_ptr_release( const Cache<T>* p ) {
        if( !--(p->refC) ) delete p;
    }

//////////////////////////////////////////////////////////////////////
// Rest of list's stuff
//////////////////////////////////////////////////////////////////////

template <class T, class F> struct ListHelp<T,F,list<T> > {
   boost::intrusive_ptr<Cache<T> > operator()( const F& f ) const {
      return boost::intrusive_ptr<Cache<T> >
         (new Cache<T>(typename Cache<T>::CvtFxn(),f));
   }
};
template <class T, class F> struct ListHelp<T,F,odd_list<T> > {
   boost::intrusive_ptr<Cache<T> > operator()( const F& f ) const {
      return boost::intrusive_ptr<Cache<T> >(new Cache<T>(f));
   }
};

template <class T>
class list_iterator {
   list<T> l;
   bool is_nil;
   void advance() {
      l = l.tail();
      if( !l )
         is_nil = true;
   }
   class Proxy {  // needed for operator->
      const T x;
      friend class list_iterator;
      Proxy( const T& xx ) : x(xx) {}
   public:
      const T* operator->() const { return &x; }
   };
public:
   typedef std::input_iterator_tag iterator_category;
   typedef T value_type;
   typedef ptrdiff_t difference_type;
   typedef T* pointer;
   typedef T& reference;

   list_iterator() : l(), is_nil(true) {}
   explicit list_iterator( const list<T>& ll ) : l(ll), is_nil(!ll) {}
   
   const T operator*() const { return l.head(); }
   const Proxy operator->() const { return Proxy(l.head()); }
   list_iterator<T>& operator++() {
      advance();
      return *this;
   }
   const list_iterator<T> operator++(int) {
      list_iterator<T> i( *this );
      advance();
      return i;
   }
   bool operator==( const list_iterator<T>& i ) const {
      return is_nil && i.is_nil;
   }
   bool operator!=( const list_iterator<T>& i ) const {
      return ! this->operator==(i);
   }
};


    } // namespace impl

  using impl::list;
  using impl::odd_list;
  using impl::list_iterator;

//////////////////////////////////////////////////////////////////////
// op== and op<, overloaded for all combos of list, odd_list, and NIL
//////////////////////////////////////////////////////////////////////
// All of these null head and tail are now non lazy using e.g. null(a)().
// They need an extra () e.g. null(a)().

// FIX THIS comparison operators can be implemented simpler with enable_if
template <class T>
bool operator==( const odd_list<T>& a, a_unique_type_for_nil ) {
  return null(a)();
}
template <class T>
bool operator==( const list<T>& a, a_unique_type_for_nil ) {
  return null(a)();
}
template <class T>
bool operator==( a_unique_type_for_nil, const odd_list<T>& a ) {
  return null(a)();
}
template <class T>
bool operator==( a_unique_type_for_nil, const list<T>& a ) {
  return null(a)();
}
template <class T>
bool operator==( const list<T>& a, const list<T>& b ) {
  if( null(a)() && null(b)() )
      return true;
  if( null(a)() || null(b)() )
      return false;
  return (head(a)()==head(b)()) && (tail(a)()==tail(b)());
}
template <class T>
bool operator==( const odd_list<T>& a, const odd_list<T>& b ) {
  if( null(a)() && null(b)() )
      return true;
  if( null(a)() || null(b)() )
      return false;
  return (head(a)()==head(b)()) && (tail(a)()==tail(b)());
}
template <class T>
bool operator==( const list<T>& a, const odd_list<T>& b ) {
  if( null(a)() && null(b)() )
      return true;
  if( null(a)() || null(b)() )
      return false;
  return (head(a)()==head(b)()) && (tail(a)()==tail(b)());
}
template <class T>
bool operator==( const odd_list<T>& a, const list<T>& b ) {
  if( null(a)() && null(b)() )
      return true;
  if( null(a)() || null(b)() )
      return false;
  return (head(a)()==head(b)()) && (tail(a)()==tail(b)());
}

template <class T>
bool operator<( const list<T>& a, const list<T>& b ) {
  if( null(a)() && !null(b)() )  return true;
  if( null(b)() )              return false;
  if( head(b)() < head(a)() )    return false;
  if( head(a)() < head(b)() )    return true;
  return (tail(a)() < tail(b)());
}
template <class T>
bool operator<( const odd_list<T>& a, const list<T>& b ) {
  if( null(a)() && !null(b)() )  return true;
  if( null(b)() )              return false;
  if( head(b)() < head(a)() )    return false;
  if( head(a)() < head(b)() )    return true;
  return (tail(a)() < tail(b)());
}
template <class T>
bool operator<( const list<T>& a, const odd_list<T>& b ) {
   if( null(a) && !null(b) )  return true;
   if( null(b) )              return false;
   if( head(b) < head(a) )    return false;
   if( head(a) < head(b) )    return true;
   return (tail(a) < tail(b));
}
template <class T>
bool operator<( const odd_list<T>& a, const odd_list<T>& b ) {
  if( null(a)() && !null(b)() )  return true;
  if( null(b)() )              return false;
  if( head(b)() < head(a)() )    return false;
  if( head(a)() < head(b)() )    return true;
  return (tail(a)() < tail(b)());
}
template <class T>
bool operator<( const odd_list<T>&, a_unique_type_for_nil ) {
   return false;
}
template <class T>
bool operator<( const list<T>&, a_unique_type_for_nil ) {
   return false;
}
template <class T>
bool operator<( a_unique_type_for_nil, const odd_list<T>& b ) {
  return !null(b)();
}
template <class T>
bool operator<( a_unique_type_for_nil, const list<T>& b ) {
  return !null(b)();
}

//////////////////////////////////////////////////////////////////////
// Implement cat and cons after the list types are defined.
//////////////////////////////////////////////////////////////////////
    namespace impl {
      using listlike::ListLike;

      template <class T, class F, class L>
      struct ConsHelp2<T,F,L,true>
      {
         typedef typename boost::remove_reference<T>::type TT;
         typedef typename L::force_result_type type;
         static type go( const TT& x, const F& f ) {
            return type( x, f );
         }
      };
      template <class T, class F>
      struct ConsHelp2<T,F,list<T>,true>
      {
         typedef typename boost::remove_reference<T>::type TT;
         typedef list<TT> L;
         typedef typename L::force_result_type type;
         static type go( const TT& x, const F& f ) {
            return odd_list<TT>(x, list<TT>(
            boost::intrusive_ptr<Cache<TT> >(new Cache<T>(
            typename Cache<TT>::CvtFxn(),f))));
         }
       };
       template <class T, class F>
       struct ConsHelp2<T,F,odd_list<T>,true>
       {
          typedef typename boost::remove_reference<T>::type TT;
          typedef odd_list<TT> L;
          typedef typename L::force_result_type type;
          static type go( const TT& x, const F& f ) {
              return odd_list<TT>(x, list<TT>( ListRaw(), new Cache<T>(f) ));
          }
       };
       template <class T, class F>
       struct ConsHelp2<T,F,a_unique_type_for_nil,false>
       {
          typedef typename boost::remove_reference<T>::type TT;
          typedef odd_list<TT> type;
          static type go( const TT& x, const F& f ) {
             return odd_list<TT>(x, list<TT>( ListRaw(), new Cache<T>(f) ));
          }
       };

       template <class T, class L, bool b> struct ConsHelp1 {
          typedef typename boost::remove_reference<T>::type TT;
          typedef typename L::force_result_type type;
          static type go( const TT& x, const L& l ) {
             return type(x,l);
          }
       };
      template <class T> struct ConsHelp1<T,a_unique_type_for_nil,false> {
        typedef typename boost::remove_reference<T>::type TT;
        typedef odd_list<TT> type;
        static type go( const TT& x, const a_unique_type_for_nil& n ) {
        return type(x,n);
        }
      };
      template <class T, class F> struct ConsHelp1<T,F,false> {
        // It's a function returning a list
        // This is the one I have not fixed yet....
        // typedef typename F::result_type L;
        // typedef typename result_of::template ListType<F>::result_type L;
        typedef odd_list<T> L;
        typedef ConsHelp2<T,F,L,boost::is_base_and_derived<ListLike,L>::value> help;
        typedef typename help::type type;
        static type go( const T& x, const F& f ) {
           return help::go(x,f);
        }
      };

      template <class T, class L, bool b>
      struct ConsHelp0;

      template <class T>
      struct ConsHelp0<T,a_unique_type_for_nil,true> {
        typedef typename boost::remove_reference<T>::type TT;
        typedef odd_list<T> type;
      };

      template <class T>
      struct ConsHelp0<const T &,const a_unique_type_for_nil &,true> {
        typedef typename boost::remove_reference<T>::type TT;
        typedef odd_list<TT> type;
      };

      template <class T>
      struct ConsHelp0<T &,a_unique_type_for_nil &,true> {
        typedef typename boost::remove_reference<T>::type TT;
        typedef odd_list<TT> type;
      };

      template <class T, class L>
      struct ConsHelp0<T,L,false> {
          // This removes any references from L for correct return type
          // identification.
           typedef typename boost::remove_reference<L>::type LType;
           typedef typename ConsHelp1<T,LType,
           boost::is_base_and_derived<ListLike,LType>::value>::type type;
      };

      /////////////////////////////////////////////////////////////////////
      // cons (t,l) - cons a value to the front of a list.
      // Note: The first arg,  t, must be a value.
      //       The second arg, l, can be a list or NIL
      //       or a function that returns a list.
      /////////////////////////////////////////////////////////////////////
      struct Cons
      {
        /* template <class T, class L> struct sig : public fun_type<
        typename ConsHelp1<T,L,
      boost::is_base_and_derived<ListLike,L>::value>::type> {};
        */
        template <typename Sig> struct result;

        template <typename This, typename T, typename L>
        struct result<This(T, L)>
        {
          typedef typename ConsHelp0<T,L,
          listlike::detect_nil<L>::is_nil>::type type;
        };

        template <typename This, typename T>
        struct result<This(T,a_unique_type_for_nil)>
        {
          typedef typename boost::remove_reference<T>::type TT;
          typedef odd_list<TT> type;
        };

        template <typename T, typename L>
        typename result<Cons(T,L)>::type
        operator()( const T& x, const L& l ) const {
           typedef typename result_of::ListType<L>::LType LType;
           typedef ConsHelp1<T,LType,
           boost::is_base_and_derived<ListLike,LType>::value> help;
           return help::go(x,l);
          }
      
        template <typename T>
        typename result<Cons(T,a_unique_type_for_nil)>::type
        operator()( const T& x, const a_unique_type_for_nil &n ) const {
           typedef typename result<Cons(T,a_unique_type_for_nil)>::type LL;
           typedef ConsHelp1<T,LL,
           boost::is_base_and_derived<ListLike,LL>::value> help;
           return help::go(x,n);
          }

      };
    }

    typedef boost::phoenix::function<impl::Cons> Cons;
    Cons cons;

    namespace impl {

      template <class L, class M, bool b>
      struct CatHelp0;

      template <class LL>
      struct CatHelp0<LL,a_unique_type_for_nil,true> {
        typedef typename result_of::template ListType<LL>::LType type;
      };

      template <class LL>
      struct CatHelp0<const LL &,const a_unique_type_for_nil &,true> {
        typedef typename result_of::template ListType<LL>::LType type;
        //typedef L type;
      };

      template <class LL>
      struct CatHelp0<LL &,a_unique_type_for_nil &,true> {
        typedef typename result_of::template ListType<LL>::LType type;
        //typedef L type;
      };

      template <class LL, class MM>
      struct CatHelp0<LL,MM,false> {
          // This removes any references from L for correct return type
          // identification.
        typedef typename result_of::template ListType<LL>::LType type;
        //    typedef typename ConsHelp1<T,LType,
        //   boost::is_base_and_derived<ListLike,LType>::value>::type type;
      };

      /////////////////////////////////////////////////////////////////////
      // cat (l,m) - concatenate lists.
      // Note: The first arg,  l, must be a list or NIL.
      //       The second arg, m, can be a list or NIL
      //       or a function that returns a list.
      /////////////////////////////////////////////////////////////////////
      struct Cat
      {
         template <class L, class M, bool b, class R>
         struct Helper /*: public c_fun_type<L,M,R>*/ {
           template <typename Sig> struct result;
           
           template <typename This>
           struct result<This(L,M)>
          {
             typedef typename result_of::ListType<L>::tail_result_type type;
          };

           typedef R return_type;
           R operator()( const L& l, const M& m,
             reuser2<INV,VAR,INV,Helper,
             typename result_of::template ListType<L>::tail_result_type,M>
             r = NIL ) const {
             if( null(l)() )
                return m().force();
             else
                return cons( head(l)(), r( Helper<L,M,b,R>(), tail(l), m )() );
         }
      };
          template <class L, class M, class R>
          struct Helper<L,M,true,R> /*: public c_fun_type<L,M,R>*/ {
           template <typename Sig> struct result;
           
           template <typename This>
           struct result<This(L,M)>
          {
             typedef typename result_of::ListType<L>::tail_result_type type;
          };
          typedef R return_type;
          R operator()( const L& l, const M& m,
             reuser2<INV,VAR,INV,Helper,
             typename result_of::template ListType<L>::tail_result_type,M>
             r = NIL ) const {
             if( null(l)() )
                return m.force();
             else
                return cons( head(l)(), r(Helper<L,M,true,R>(), tail(l), m )());
         }
      };
      template <class L, class R>
      struct Helper<L,a_unique_type_for_nil,false,R>
      /*: public c_fun_type<L,
        a_unique_type_for_nil,odd_list<typename L::value_type> > */
      {
        typedef odd_list<typename result_of::template ListType<L>::value_type> type;
        odd_list<typename result_of::template ListType<L>::value_type>
        operator()( const L& l, const a_unique_type_for_nil& ) const {
         return l;
        }
      };
   public:
        /*template <class L, class M> struct sig : public fun_type<
        typename RT<cons_type,typename L::value_type,M>::result_type>
      {}; */
   // Need to work out the return type here.
      template <typename Sig> struct result;

      template <typename This, typename L, typename M>
      struct result<This(L,M)>
      {
        typedef typename CatHelp0<L,M,
          listlike::detect_nil<M>::is_nil>::type type;
        // typedef typename result_of::ListType<L>::tail_result_type type;
      };
      
      template <typename This, typename L>
      struct result<This(L,a_unique_type_for_nil)>
      {
         typedef typename result_of::ListType<L>::tail_result_type type;
      };
      template <class L, class M>
      typename result<Cat(L,M)>::type operator()( const L& l, const M& m ) const
      {
         listlike::EnsureListLike<L>();
         return Helper<L,M,
         boost::is_base_and_derived<typename listlike::ListLike,M>::value,
                typename result<Cat(L,M)>::type>()(l,m);
      }

      template <class L>
      typename result<Cat(L,a_unique_type_for_nil)>::type operator()( const L& l, const a_unique_type_for_nil& /* n */ ) const
      {
         listlike::EnsureListLike<L>();
         return l;
      }
       
      };


    }

    typedef boost::phoenix::function<impl::Cat> Cat;
    Cat cat;


//////////////////////////////////////////////////////////////////////
// Handy functions for making list literals
//////////////////////////////////////////////////////////////////////
// Yes, these aren't functoids, they're just template functions.  I'm
// lazy and created these mostly to make it easily to make little lists
// in the sample code snippets that appear in papers.

struct UseList {
   template <class T> struct List { typedef list<T> type; };
};
struct UseOddList {
   template <class T> struct List { typedef odd_list<T> type; };
};
struct UseStrictList {
   template <class T> struct List { typedef strict_list<T> type; };
};

template <class Kind = UseList>
struct list_with {
   template <class T>
   typename Kind::template List<T>::type
   operator()( const T& a ) const {
      typename Kind::template List<T>::type l;
      l = cons( a, l );
      return l;
   }
   
   template <class T>
   typename Kind::template List<T>::type
   operator()( const T& a, const T& b ) const {
      typename Kind::template List<T>::type l;
      l = cons( b, l );
      l = cons( a, l );
      return l;
   }
   
   template <class T>
   typename Kind::template List<T>::type
   operator()( const T& a, const T& b, const T& c ) const {
      typename Kind::template List<T>::type l;
      l = cons( c, l );
      l = cons( b, l );
      l = cons( a, l );
      return l;
   }
   
   template <class T>
   typename Kind::template List<T>::type
   operator()( const T& a, const T& b, const T& c, const T& d ) const {
      typename Kind::template List<T>::type l;
      l = cons( d, l );
      l = cons( c, l );
      l = cons( b, l );
      l = cons( a, l );
      return l;
   }
   
   template <class T>
   typename Kind::template List<T>::type
   operator()( const T& a, const T& b, const T& c, const T& d,
               const T& e ) const {
      typename Kind::template List<T>::type l;
      l = cons( e, l );
      l = cons( d, l );
      l = cons( c, l );
      l = cons( b, l );
      l = cons( a, l );
      return l;
   }
};
//////////////////////////////////////////////////////////////////////

  }

}

#endif
