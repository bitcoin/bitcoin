///////////////////////////////////////////////////////////////////////////////
//
// Copyright David Abrahams 2002, Joel de Guzman, 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_PP_IS_ITERATING)

#ifndef DEFAULTS_DEF_JDG20020811_HPP
#define DEFAULTS_DEF_JDG20020811_HPP

#include <boost/python/detail/defaults_gen.hpp>
#include <boost/python/detail/type_traits.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/static_assert.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/python/class_fwd.hpp>
#include <boost/python/scope.hpp>
#include <boost/preprocessor/debug/line.hpp>
#include <boost/python/detail/scope.hpp>
#include <boost/python/detail/make_keyword_range_fn.hpp>
#include <boost/python/object/add_to_namespace.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace python {

struct module;

namespace objects
{
  struct class_base;
}

namespace detail
{
  // Called as::
  //
  //    name_space_def(ns, "func", func, kw, policies, docstring, &ns)
  //
  // Dispatch to properly add f to namespace ns.
  //
  // @group define_stub_function helpers { 
  template <class Func, class CallPolicies, class NameSpaceT>
  static void name_space_def(
      NameSpaceT& name_space
      , char const* name
      , Func f
      , keyword_range const& kw
      , CallPolicies const& policies
      , char const* doc
      , objects::class_base*
      )
  {
      typedef typename NameSpaceT::wrapped_type wrapped_type;
      
      objects::add_to_namespace(
          name_space, name,
          detail::make_keyword_range_function(
              f, policies, kw, get_signature(f, (wrapped_type*)0))
        , doc
      );
  }

  template <class Func, class CallPolicies>
  static void name_space_def(
      object& name_space
      , char const* name
      , Func f
      , keyword_range const& kw
      , CallPolicies const& policies
      , char const* doc
      , ...
      )
  {
      scope within(name_space);

      detail::scope_setattr_doc(
          name
          , detail::make_keyword_range_function(f, policies, kw)
          , doc);
  }

  // For backward compatibility -- is this obsolete?
  template <class Func, class CallPolicies, class NameSpaceT>
  static void name_space_def(
      NameSpaceT& name_space
      , char const* name
      , Func f
      , keyword_range const& kw // ignored
      , CallPolicies const& policies
      , char const* doc
      , module*
      )
  {
      name_space.def(name, f, policies, doc);
  }
  // }


  //  Expansions of ::
  //
  //      template <typename OverloadsT, typename NameSpaceT>
  //      inline void
  //      define_stub_function(
  //          char const* name, OverloadsT s, NameSpaceT& name_space, mpl::int_<N>)
  //      {
  //          name_space.def(name, &OverloadsT::func_N);
  //      }
  //
  //  where N runs from 0 to BOOST_PYTHON_MAX_ARITY.
  //
  //  The set of overloaded functions (define_stub_function) expects:
  //
  //      1. char const* name:        function name that will be visible to python
  //      2. OverloadsT:              a function overloads struct (see defaults_gen.hpp)
  //      3. NameSpaceT& name_space:  a python::class_ or python::module instance
  //      4. int_t<N>:                the Nth overloaded function (OverloadsT::func_N)
  //                                  (see defaults_gen.hpp)
  //      5. char const* name:        doc string
  //
  // @group define_stub_function<N> {
  template <int N>
  struct define_stub_function {};

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (3, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/detail/defaults_def.hpp>))

#include BOOST_PP_ITERATE()
  
  // }
  
  //  This helper template struct does the actual recursive
  //  definition.  There's a generic version
  //  define_with_defaults_helper<N> and a terminal case
  //  define_with_defaults_helper<0>. The struct and its
  //  specialization has a sole static member function def that
  //  expects:
  //
  //    1. char const* name:        function name that will be
  //                                visible to python
  //
  //    2. OverloadsT:              a function overloads struct
  //                                (see defaults_gen.hpp)
  //
  //    3. NameSpaceT& name_space:  a python::class_ or
  //                                python::module instance
  //
  //    4. char const* name:        doc string
  //
  //  The def static member function calls a corresponding
  //  define_stub_function<N>. The general case recursively calls
  //  define_with_defaults_helper<N-1>::def until it reaches the
  //  terminal case case define_with_defaults_helper<0>.
  template <int N>
  struct define_with_defaults_helper {

      template <class StubsT, class CallPolicies, class NameSpaceT>
      static void
      def(
          char const* name,
          StubsT stubs,
          keyword_range kw,
          CallPolicies const& policies,
          NameSpaceT& name_space,
          char const* doc)
      {
          //  define the NTH stub function of stubs
          define_stub_function<N>::define(name, stubs, kw, policies, name_space, doc);

          if (kw.second > kw.first)
              --kw.second;

          //  call the next define_with_defaults_helper
          define_with_defaults_helper<N-1>::def(name, stubs, kw, policies, name_space, doc);
      }
  };

  template <>
  struct define_with_defaults_helper<0> {

      template <class StubsT, class CallPolicies, class NameSpaceT>
      static void
      def(
          char const* name,
          StubsT stubs,
          keyword_range const& kw,
          CallPolicies const& policies,
          NameSpaceT& name_space,
          char const* doc)
      {
          //  define the Oth stub function of stubs
          define_stub_function<0>::define(name, stubs, kw, policies, name_space, doc);
          //  return
      }
  };

  //  define_with_defaults
  //
  //      1. char const* name:        function name that will be
  //                                  visible to python
  //
  //      2. OverloadsT:              a function overloads struct
  //                                  (see defaults_gen.hpp)
  //
  //      3. CallPolicies& policies:  Call policies
  //      4. NameSpaceT& name_space:  a python::class_ or
  //                                  python::module instance
  //
  //      5. SigT sig:                Function signature typelist
  //                                  (see defaults_gen.hpp)
  //
  //      6. char const* name:        doc string
  //
  //  This is the main entry point. This function recursively
  //  defines all stub functions of StubT (see defaults_gen.hpp) in
  //  NameSpaceT name_space which can be either a python::class_ or
  //  a python::module. The sig argument is a typelist that
  //  specifies the return type, the class (for member functions,
  //  and the arguments. Here are some SigT examples:
  //
  //      int foo(int)        mpl::vector<int, int>
  //      void bar(int, int)  mpl::vector<void, int, int>
  //      void C::foo(int)    mpl::vector<void, C, int>
  //
  template <class OverloadsT, class NameSpaceT, class SigT>
  inline void
  define_with_defaults(
      char const* name,
      OverloadsT const& overloads,
      NameSpaceT& name_space,
      SigT const&)
  {
      typedef typename mpl::front<SigT>::type return_type;
      typedef typename OverloadsT::void_return_type void_return_type;
      typedef typename OverloadsT::non_void_return_type non_void_return_type;

      typedef typename mpl::if_c<
          is_same<void, return_type>::value
          , void_return_type
          , non_void_return_type
      >::type stubs_type;

      BOOST_STATIC_ASSERT(
          (stubs_type::max_args) <= mpl::size<SigT>::value);

      typedef typename stubs_type::template gen<SigT> gen_type;
      define_with_defaults_helper<stubs_type::n_funcs-1>::def(
          name
          , gen_type()
          , overloads.keywords()
          , overloads.call_policies()
          , name_space
          , overloads.doc_string());
  }

} // namespace detail

}} // namespace boost::python

#endif // DEFAULTS_DEF_JDG20020811_HPP

#else // defined(BOOST_PP_IS_ITERATING)
// PP vertical iteration code


template <>
struct define_stub_function<BOOST_PP_ITERATION()> {
    template <class StubsT, class CallPolicies, class NameSpaceT>
    static void define(
        char const* name
        , StubsT const&
        , keyword_range const& kw
        , CallPolicies const& policies
        , NameSpaceT& name_space
        , char const* doc)
    {
        detail::name_space_def(
            name_space
            , name
            , &StubsT::BOOST_PP_CAT(func_, BOOST_PP_ITERATION())
            , kw
            , policies
            , doc
            , &name_space);
    }
};

#endif // !defined(BOOST_PP_IS_ITERATING)
