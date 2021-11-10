# ifndef BOOST_PYTHON_SYNOPSIS 
# // Copyright David Abrahams 2002.
# // Distributed under the Boost Software License, Version 1.0. (See
# // accompanying file LICENSE_1_0.txt or copy at
# // http://www.boost.org/LICENSE_1_0.txt)

#  if !defined(BOOST_PP_IS_ITERATING)
#   error Boost.Python - do not include this file!
#  endif

#  define N BOOST_PP_ITERATION()

#  define BOOST_PYTHON_MAKE_TUPLE_ARG(z, N, ignored)    \
    PyTuple_SET_ITEM(                                   \
        result.ptr()                                    \
        , N                                             \
        , python::incref(python::object(a##N).ptr())    \
        );

    template <BOOST_PP_ENUM_PARAMS_Z(1, N, class A)>
    tuple
    make_tuple(BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, A, const& a))
    {
        tuple result((detail::new_reference)::PyTuple_New(N));
        BOOST_PP_REPEAT_1ST(N, BOOST_PYTHON_MAKE_TUPLE_ARG, _)
        return result;
    }

#  undef BOOST_PYTHON_MAKE_TUPLE_ARG

#  undef N
# endif // BOOST_PYTHON_SYNOPSIS 
