#if !defined(BOOST_PP_IS_ITERATING)

// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef RESULT_DWA2002521_HPP
#  define RESULT_DWA2002521_HPP

#  include <boost/type.hpp>

#  include <boost/python/detail/preprocessor.hpp>
#  include <boost/python/detail/type_traits.hpp>

#  include <boost/mpl/if.hpp>

#  include <boost/preprocessor/comma_if.hpp>
#  include <boost/preprocessor/iterate.hpp>
#  include <boost/preprocessor/debug/line.hpp>
#  include <boost/preprocessor/enum_params.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_params.hpp>

namespace boost { namespace python { namespace detail {

// Defines a family of overloaded function which, given x, a function
// pointer, member [function] pointer, or an AdaptableFunction object,
// returns a pointer to type<R>*, where R is the result type of
// invoking the result of bind(x).
//
// In order to work around bugs in deficient compilers, if x might be
// an AdaptableFunction object, you must pass OL as a second argument
// to get this to work portably.

#  define BOOST_PP_ITERATION_PARAMS_1                                                                   \
    (4, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/detail/result.hpp>, BOOST_PYTHON_FUNCTION_POINTER))
#  include BOOST_PP_ITERATE()

#  define BOOST_PP_ITERATION_PARAMS_1                                                                     \
    (4, (0, BOOST_PYTHON_CV_COUNT - 1, <boost/python/detail/result.hpp>, BOOST_PYTHON_POINTER_TO_MEMBER))
#  include BOOST_PP_ITERATE()

template <class R, class T>
boost::type<R>* result(R (T::*), int = 0) { return 0; }

#  if (defined(__MWERKS__) && __MWERKS__ < 0x3000)
// This code actually works on all implementations, but why use it when we don't have to?
template <class T>
struct get_result_type
{
    typedef boost::type<typename T::result_type> type;
};

struct void_type
{
    typedef void type;
};

template <class T>
struct result_result
{
    typedef typename mpl::if_c<
        is_class<T>::value
        , get_result_type<T>
        , void_type
        >::type t1;

    typedef typename t1::type* type;
};

template <class X>
typename result_result<X>::type
result(X const&, short) { return 0; }

#  else // Simpler code for more-capable compilers
template <class X>
boost::type<typename X::result_type>*
result(X const&, short = 0) { return 0; }

#  endif

}}} // namespace boost::python::detail

# endif // RESULT_DWA2002521_HPP

/* --------------- function pointers --------------- */
// For gcc 4.4 compatability, we must include the
// BOOST_PP_ITERATION_DEPTH test inside an #else clause.
#else // BOOST_PP_IS_ITERATING
#if BOOST_PP_ITERATION_DEPTH() == 1 && BOOST_PP_ITERATION_FLAGS() == BOOST_PYTHON_FUNCTION_POINTER
# if !(BOOST_WORKAROUND(__MWERKS__, > 0x3100)                      \
        && BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3201)))
#  line BOOST_PP_LINE(__LINE__, result.hpp(function pointers))
# endif

# define N BOOST_PP_ITERATION()

template <class R BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class A)>
boost::type<R>* result(R (*)(BOOST_PP_ENUM_PARAMS_Z(1, N, A)), int = 0)
{
    return 0;
}

# undef N

/* --------------- pointers-to-members --------------- */
#elif BOOST_PP_ITERATION_DEPTH() == 1 && BOOST_PP_ITERATION_FLAGS() == BOOST_PYTHON_POINTER_TO_MEMBER
// Outer over cv-qualifiers

# define BOOST_PP_ITERATION_PARAMS_2 (3, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/detail/result.hpp>))
# include BOOST_PP_ITERATE()

#elif BOOST_PP_ITERATION_DEPTH() == 2
# if !(BOOST_WORKAROUND(__MWERKS__, > 0x3100)                      \
        && BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3201)))
#  line BOOST_PP_LINE(__LINE__, result.hpp(pointers-to-members))
# endif 
// Inner over arities

# define N BOOST_PP_ITERATION()
# define Q BOOST_PYTHON_CV_QUALIFIER(BOOST_PP_RELATIVE_ITERATION(1))

template <class R, class T BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class A)>
boost::type<R>* result(R (T::*)(BOOST_PP_ENUM_PARAMS_Z(1, N, A)) Q, int = 0)
{
    return 0;
}

# undef N
# undef Q

#endif // BOOST_PP_ITERATION_DEPTH()
#endif
