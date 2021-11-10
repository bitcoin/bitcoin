// Copyright David Abrahams 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_CONCEPT_CHECK_MSVC_DWA2006429_HPP
# define BOOST_CONCEPT_CHECK_MSVC_DWA2006429_HPP

# ifdef BOOST_OLD_CONCEPT_SUPPORT
#  include <boost/concept_check/has_constraints.hpp>
#  include <boost/type_traits/conditional.hpp>
# endif


// This implementation works on GCC and Comeau, but has actually been
// fairly carefully tuned to work on GCC versions starting with
// gcc-2.95.x.  If you're trying to get an additional compiler to pass
// the tests you might consider breaking out a separate gcc.hpp and
// starting over on the general case.
namespace boost
{
  namespace concept_checking
  {
    template <void(*)()> struct instantiate {};
  }
  
  template <class ModelFn> struct concept_check_;

  template <class Model>
  void concept_check_failed()
  {
      ((Model*)0)->~Model();
  }

  template <class Model>
  struct concept_check
  {
      concept_checking::instantiate<concept_check_failed<Model> > x;
      enum { instantiate = 1 };
  };

# ifdef BOOST_OLD_CONCEPT_SUPPORT
  
  template <class Model>
  void constraint_check_failed()
  {
      ((Model*)0)->constraints();
  }

  template <class Model>
  struct constraint_check
  {
      concept_checking::instantiate<constraint_check_failed<Model> > x;
      enum { instantiate = 1 };
  };
  
  template <class Model>
  struct concept_check_<void(*)(Model)>
    : conditional<
          concept_checking::has_constraints<Model>::value
        , constraint_check<Model>
        , concept_check<Model>
      >::type
  {};
  
# else
  
  template <class Model>
  struct concept_check_<void(*)(Model)>
    : concept_check<Model>
  {};
  
# endif
  
  // Usage, in class or function context:
  //
  //     BOOST_CONCEPT_ASSERT((UnaryFunctionConcept<F,bool,int>));
#  define BOOST_CONCEPT_ASSERT( ModelInParens )                             \
  enum { BOOST_PP_CAT(boost_concept_check,__LINE__) =                       \
         ::boost::concept_check_<void(*) ModelInParens>::instantiate        \
  }
}

#endif // BOOST_CONCEPT_CHECK_MSVC_DWA2006429_HPP
