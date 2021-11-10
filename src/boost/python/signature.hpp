///////////////////////////////////////////////////////////////////////////////
//
// Copyright David Abrahams 2002, Joel de Guzman, 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_PP_IS_ITERATING)

# ifndef SIGNATURE_JDG20020813_HPP
#  define SIGNATURE_JDG20020813_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/mpl/if.hpp>

#  include <boost/python/detail/preprocessor.hpp>
#  include <boost/python/detail/type_traits.hpp>
#  include <boost/preprocessor/repeat.hpp>
#  include <boost/preprocessor/enum.hpp>
#  include <boost/preprocessor/enum_params.hpp>
#  include <boost/preprocessor/empty.hpp>
#  include <boost/preprocessor/arithmetic/sub.hpp>
#  include <boost/preprocessor/iterate.hpp>
#  include <boost/python/detail/type_list.hpp>

#  include <boost/preprocessor/debug/line.hpp>
#  include <boost/preprocessor/arithmetic/sub.hpp>
#  include <boost/preprocessor/arithmetic/inc.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_params.hpp>

# define BOOST_PYTHON_LIST_INC(n)        \
   BOOST_PP_CAT(mpl::vector, BOOST_PP_INC(n))

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace python { namespace detail {

// A metafunction returning C1 if C1 is derived from C2, and C2
// otherwise
template <class C1, class C2>
struct most_derived
{
    typedef typename mpl::if_<
        detail::is_convertible<C1*,C2*>
      , C1
      , C2
    >::type type;
};

//  The following macros generate expansions for::
//
//      template <class RT, class T0... class TN>
//      inline mpl::vector<RT, T0...TN>
//      get_signature(RT(BOOST_PYTHON_FN_CC *)(T0...TN), void* = 0)
//      {
//          return mpl::list<RT, T0...TN>();
//      }
//
//    where BOOST_PYTHON_FN_CC is a calling convention keyword, can be
//
//        empty, for default calling convention
//        __cdecl (if BOOST_PYTHON_ENABLE_CDECL is defined)
//        __stdcall (if BOOST_PYTHON_ENABLE_STDCALL is defined)
//        __fastcall (if BOOST_PYTHON_ENABLE_FASTCALL is defined)
//
//   And, for an appropriate assortment of cv-qualifications::
//
//      template <class RT, class ClassT, class T0... class TN>
//      inline mpl::vector<RT, ClassT&, T0...TN>
//      get_signature(RT(BOOST_PYTHON_FN_CC ClassT::*)(T0...TN) cv))
//      {
//          return mpl::list<RT, ClassT&, T0...TN>();
//      }
//
//      template <class Target, class RT, class ClassT, class T0... class TN>
//      inline mpl::vector<
//          RT
//        , typename most_derived<Target, ClassT>::type&
//        , T0...TN
//      >
//      get_signature(RT(BOOST_PYTHON_FN_CC ClassT::*)(T0...TN) cv), Target*)
//      {
//          return mpl::list<RT, ClassT&, T0...TN>();
//      }
//
//  There are two forms for invoking get_signature::
//
//      get_signature(f)
//
//  and ::
//
//      get_signature(f,(Target*)0)
//
//  These functions extract the return type, class (for member
//  functions) and arguments of the input signature and stuff them in
//  an mpl type sequence (the calling convention is dropped).
//  Note that cv-qualification is dropped from
//  the "hidden this" argument of member functions; that is a
//  necessary sacrifice to ensure that an lvalue from_python converter
//  is used.  A pointer is not used so that None will be rejected for
//  overload resolution.
//
//  The second form of get_signature essentially downcasts the "hidden
//  this" argument of member functions to Target, because the function
//  may actually be a member of a base class which is not wrapped, and
//  in that case conversion from python would fail.
//
// @group {

// 'default' calling convention

#  define BOOST_PYTHON_FN_CC

#  define BOOST_PP_ITERATION_PARAMS_1                                   \
    (3, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/signature.hpp>))

#  include BOOST_PP_ITERATE()

#  undef BOOST_PYTHON_FN_CC

// __cdecl calling convention

#  if defined(BOOST_PYTHON_ENABLE_CDECL)

#   define BOOST_PYTHON_FN_CC __cdecl
#   define BOOST_PYTHON_FN_CC_IS_CDECL

#   define BOOST_PP_ITERATION_PARAMS_1                                   \
     (3, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/signature.hpp>))

#   include BOOST_PP_ITERATE()

#   undef BOOST_PYTHON_FN_CC
#   undef BOOST_PYTHON_FN_CC_IS_CDECL

#  endif // defined(BOOST_PYTHON_ENABLE_CDECL)

// __stdcall calling convention

#  if defined(BOOST_PYTHON_ENABLE_STDCALL)

#   define BOOST_PYTHON_FN_CC __stdcall

#   define BOOST_PP_ITERATION_PARAMS_1                                   \
     (3, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/signature.hpp>))

#   include BOOST_PP_ITERATE()

#   undef BOOST_PYTHON_FN_CC

#  endif // defined(BOOST_PYTHON_ENABLE_STDCALL)

// __fastcall calling convention

#  if defined(BOOST_PYTHON_ENABLE_FASTCALL)

#   define BOOST_PYTHON_FN_CC __fastcall

#   define BOOST_PP_ITERATION_PARAMS_1                                   \
     (3, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/signature.hpp>))

#   include BOOST_PP_ITERATE()

#   undef BOOST_PYTHON_FN_CC

#  endif // defined(BOOST_PYTHON_ENABLE_FASTCALL)

#  undef BOOST_PYTHON_LIST_INC

// }

}}} // namespace boost::python::detail


# endif // SIGNATURE_JDG20020813_HPP

// For gcc 4.4 compatability, we must include the
// BOOST_PP_ITERATION_DEPTH test inside an #else clause.
#else // BOOST_PP_IS_ITERATING
#if BOOST_PP_ITERATION_DEPTH() == 1 // defined(BOOST_PP_IS_ITERATING)

# define N BOOST_PP_ITERATION()

   // as 'get_signature(RT(*)(T0...TN), void* = 0)' is the same
   // function as 'get_signature(RT(__cdecl *)(T0...TN), void* = 0)',
   // we don't define it twice
#  if !defined(BOOST_PYTHON_FN_CC_IS_CDECL)

template <
    class RT BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class T)>
inline BOOST_PYTHON_LIST_INC(N)<
    RT BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)>
get_signature(RT(BOOST_PYTHON_FN_CC *)(BOOST_PP_ENUM_PARAMS_Z(1, N, T)), void* = 0)
{
    return BOOST_PYTHON_LIST_INC(N)<
            RT BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)
        >();
}

#  endif // !defined(BOOST_PYTHON_FN_CC_IS_CDECL)

# undef N

# define BOOST_PP_ITERATION_PARAMS_2 \
    (3, (0, 3, <boost/python/signature.hpp>))
# include BOOST_PP_ITERATE()

#else

# define N BOOST_PP_RELATIVE_ITERATION(1)
# define Q BOOST_PYTHON_CV_QUALIFIER(BOOST_PP_ITERATION())

template <
    class RT, class ClassT BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class T)>
inline BOOST_PYTHON_LIST_INC(BOOST_PP_INC(N))<
    RT, ClassT& BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)>
get_signature(RT(BOOST_PYTHON_FN_CC ClassT::*)(BOOST_PP_ENUM_PARAMS_Z(1, N, T)) Q)
{
    return BOOST_PYTHON_LIST_INC(BOOST_PP_INC(N))<
            RT, ClassT& BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)
        >();
}

template <
    class Target
  , class RT
  , class ClassT
    BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class T)
>
inline BOOST_PYTHON_LIST_INC(BOOST_PP_INC(N))<
    RT
  , typename most_derived<Target, ClassT>::type&
    BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)
>
get_signature(
    RT(BOOST_PYTHON_FN_CC ClassT::*)(BOOST_PP_ENUM_PARAMS_Z(1, N, T)) Q
  , Target*
)
{
    return BOOST_PYTHON_LIST_INC(BOOST_PP_INC(N))<
        RT
      , BOOST_DEDUCED_TYPENAME most_derived<Target, ClassT>::type&
        BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)
    >();
}

# undef Q
# undef N

#endif // BOOST_PP_ITERATION_DEPTH()
#endif // !defined(BOOST_PP_IS_ITERATING)
