// (C) Copyright David Abrahams 2002.
// (C) Copyright Jeremy Siek    2002.
// (C) Copyright Thomas Witt    2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// no include guard multiple inclusion intended

//
// This is a temporary workaround until the bulk of this is
// available in boost config.
// 23/02/03 thw
//

#include <boost/config.hpp> // for prior
#include <boost/detail/workaround.hpp>

#ifdef BOOST_ITERATOR_CONFIG_DEF
# error you have nested config_def #inclusion.
#else 
# define BOOST_ITERATOR_CONFIG_DEF
#endif 

// We enable this always now.  Otherwise, the simple case in
// libs/iterator/test/constant_iterator_arrow.cpp fails to compile
// because the operator-> return is improperly deduced as a non-const
// pointer.
#if 1 || defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)           \
    || BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x531))

// Recall that in general, compilers without partial specialization
// can't strip constness.  Consider counting_iterator, which normally
// passes a const Value to iterator_facade.  As a result, any code
// which makes a std::vector of the iterator's value_type will fail
// when its allocator declares functions overloaded on reference and
// const_reference (the same type).
//
// Furthermore, Borland 5.5.1 drops constness in enough ways that we
// end up using a proxy for operator[] when we otherwise shouldn't.
// Using reference constness gives it an extra hint that it can
// return the value_type from operator[] directly, but is not
// strictly necessary.  Not sure how best to resolve this one.

# define BOOST_ITERATOR_REF_CONSTNESS_KILLS_WRITABILITY 1

#endif

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x5A0))                      \
    || (BOOST_WORKAROUND(BOOST_INTEL_CXX_VERSION, <= 700) && defined(_MSC_VER)) \
    || BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042))                \
    || BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))
    
# define BOOST_NO_LVALUE_RETURN_DETECTION

# if 0 // test code
  struct v  {};

  typedef  char (&no)[3];

  template <class T>
  no foo(T const&, ...);

  template <class T>
  char foo(T&, int);


  struct value_iterator
  {
      v operator*() const;
  };

  template <class T>
  struct lvalue_deref_helper
  {
      static T& x;
      enum { value = (sizeof(foo(*x,0)) == 1) };
  };

  int z2[(lvalue_deref_helper<v*>::value == 1) ? 1 : -1];
  int z[(lvalue_deref_helper<value_iterator>::value) == 1 ? -1 : 1 ];
# endif 

#endif

#if BOOST_WORKAROUND(__MWERKS__, <=0x2407)
#  define BOOST_NO_IS_CONVERTIBLE // "is_convertible doesn't work for simple types"
#endif

#if BOOST_WORKAROUND(__GNUC__, == 3) && BOOST_WORKAROUND(__GNUC_MINOR__, < 4) && !defined(__EDG_VERSION__)   \
    || BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x551))
#  define BOOST_NO_IS_CONVERTIBLE_TEMPLATE // The following program fails to compile:

#  if 0 // test code
    #include <boost/type_traits/is_convertible.hpp>
    template <class T>
    struct foo
    {
        foo(T);

        template <class U>
        foo(foo<U> const& other) : p(other.p) { }

        T p;
    };

    bool x = boost::is_convertible<foo<int const*>, foo<int*> >::value;
#  endif

#endif


#if !defined(BOOST_MSVC) && (defined(BOOST_NO_SFINAE) || defined(BOOST_NO_IS_CONVERTIBLE) || defined(BOOST_NO_IS_CONVERTIBLE_TEMPLATE))
# define BOOST_NO_STRICT_ITERATOR_INTEROPERABILITY
#endif 

# if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))

// GCC-2.95 (obsolete) eagerly instantiates templated constructors and conversion
// operators in convertibility checks, causing premature errors.
//
// Borland's problems are harder to diagnose due to lack of an
// instantiation stack backtrace.  They may be due in part to the fact
// that it drops cv-qualification willy-nilly in templates.
#  define BOOST_NO_ONE_WAY_ITERATOR_INTEROP
# endif 

// no include guard; multiple inclusion intended
