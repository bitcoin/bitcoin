#if !defined(BOOST_PP_IS_ITERATING)

// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef CALLER_DWA20021121_HPP
#  define CALLER_DWA20021121_HPP

#  include <boost/python/type_id.hpp>
#  include <boost/python/handle.hpp>

#  include <boost/detail/indirect_traits.hpp>

#  include <boost/python/detail/invoke.hpp>
#  include <boost/python/detail/signature.hpp>
#  include <boost/python/detail/preprocessor.hpp>
#  include <boost/python/detail/type_traits.hpp>

#  include <boost/python/arg_from_python.hpp>
#  include <boost/python/converter/context_result_converter.hpp>
#  include <boost/python/converter/builtin_converters.hpp>

#  include <boost/preprocessor/iterate.hpp>
#  include <boost/preprocessor/cat.hpp>
#  include <boost/preprocessor/dec.hpp>
#  include <boost/preprocessor/if.hpp>
#  include <boost/preprocessor/iteration/local.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#  include <boost/preprocessor/repetition/repeat.hpp>

#  include <boost/compressed_pair.hpp>

#  include <boost/mpl/apply.hpp>
#  include <boost/mpl/eval_if.hpp>
#  include <boost/mpl/identity.hpp>
#  include <boost/mpl/size.hpp>
#  include <boost/mpl/at.hpp>
#  include <boost/mpl/int.hpp>
#  include <boost/mpl/next.hpp>

namespace boost { namespace python { namespace detail { 

template <int N>
inline PyObject* get(mpl::int_<N>, PyObject* const& args_)
{
    return PyTuple_GET_ITEM(args_,N);
}

inline Py_ssize_t arity(PyObject* const& args_)
{
    return PyTuple_GET_SIZE(args_);
}

// This "result converter" is really just used as
// a dispatch tag to invoke(...), selecting the appropriate
// implementation
typedef int void_result_to_python;

// Given a model of CallPolicies and a C++ result type, this
// metafunction selects the appropriate converter to use for
// converting the result to python.
template <class Policies, class Result>
struct select_result_converter
  : mpl::eval_if<
        is_same<Result,void>
      , mpl::identity<void_result_to_python>
      , mpl::apply1<typename Policies::result_converter,Result>
    >
{
};

template <class ArgPackage, class ResultConverter>
inline ResultConverter create_result_converter(
    ArgPackage const& args_
  , ResultConverter*
  , converter::context_result_converter*
)
{
    return ResultConverter(args_);
}
    
template <class ArgPackage, class ResultConverter>
inline ResultConverter create_result_converter(
    ArgPackage const&
  , ResultConverter*
  , ...
)
{
    return ResultConverter();
}

#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
template <class ResultConverter>
struct converter_target_type 
{
    static PyTypeObject const *get_pytype()
    {
        return create_result_converter((PyObject*)0, (ResultConverter *)0, (ResultConverter *)0).get_pytype();
    }
};

template < >
struct converter_target_type <void_result_to_python >
{
    static PyTypeObject const *get_pytype()
    {
        return 0;
    }
};

// Generation of ret moved from caller_arity<N>::impl::signature to here due to "feature" in MSVC 15.7.2 with /O2
// which left the ret uninitialized and caused segfaults in Python interpreter.
template<class Policies, class Sig> const signature_element* get_ret()
{
    typedef BOOST_DEDUCED_TYPENAME Policies::template extract_return_type<Sig>::type rtype;
    typedef typename select_result_converter<Policies, rtype>::type result_converter;

    static const signature_element ret = {
        (is_void<rtype>::value ? "void" : type_id<rtype>().name())
        , &detail::converter_target_type<result_converter>::get_pytype
        , boost::detail::indirect_traits::is_reference_to_non_const<rtype>::value 
    };

    return &ret;
}

#endif

    
template <unsigned> struct caller_arity;

template <class F, class CallPolicies, class Sig>
struct caller;

#  define BOOST_PYTHON_NEXT(init,name,n)                                                        \
    typedef BOOST_PP_IF(n,typename mpl::next< BOOST_PP_CAT(name,BOOST_PP_DEC(n)) >::type, init) name##n;

#  define BOOST_PYTHON_ARG_CONVERTER(n)                                         \
     BOOST_PYTHON_NEXT(typename mpl::next<first>::type, arg_iter,n)             \
     typedef arg_from_python<BOOST_DEDUCED_TYPENAME arg_iter##n::type> c_t##n;  \
     c_t##n c##n(get(mpl::int_<n>(), inner_args));                              \
     if (!c##n.convertible())                                                   \
          return 0;

#  define BOOST_PP_ITERATION_PARAMS_1                                            \
        (3, (0, BOOST_PYTHON_MAX_ARITY + 1, <boost/python/detail/caller.hpp>))
#  include BOOST_PP_ITERATE()

#  undef BOOST_PYTHON_ARG_CONVERTER
#  undef BOOST_PYTHON_NEXT

// A metafunction returning the base class used for caller<class F,
// class ConverterGenerators, class CallPolicies, class Sig>.
template <class F, class CallPolicies, class Sig>
struct caller_base_select
{
    enum { arity = mpl::size<Sig>::value - 1 };
    typedef typename caller_arity<arity>::template impl<F,CallPolicies,Sig> type;
};

// A function object type which wraps C++ objects as Python callable
// objects.
//
// Template Arguments:
//
//   F -
//      the C++ `function object' that will be called. Might
//      actually be any data for which an appropriate invoke_tag() can
//      be generated. invoke(...) takes care of the actual invocation syntax.
//
//   CallPolicies -
//      The precall, postcall, and what kind of resultconverter to
//      generate for mpl::front<Sig>::type
//
//   Sig -
//      The `intended signature' of the function. An MPL sequence
//      beginning with a result type and continuing with a list of
//      argument types.
template <class F, class CallPolicies, class Sig>
struct caller
    : caller_base_select<F,CallPolicies,Sig>::type
{
    typedef typename caller_base_select<
        F,CallPolicies,Sig
        >::type base;

    typedef PyObject* result_type;
    
    caller(F f, CallPolicies p) : base(f,p) {}

};

}}} // namespace boost::python::detail

# endif // CALLER_DWA20021121_HPP

#else

# define N BOOST_PP_ITERATION()

template <>
struct caller_arity<N>
{
    template <class F, class Policies, class Sig>
    struct impl
    {
        impl(F f, Policies p) : m_data(f,p) {}

        PyObject* operator()(PyObject* args_, PyObject*) // eliminate
                                                         // this
                                                         // trailing
                                                         // keyword dict
        {
            typedef typename mpl::begin<Sig>::type first;
            typedef typename first::type result_t;
            typedef typename select_result_converter<Policies, result_t>::type result_converter;
            typedef typename Policies::argument_package argument_package;
            
            argument_package inner_args(args_);

# if N
#  define BOOST_PP_LOCAL_MACRO(i) BOOST_PYTHON_ARG_CONVERTER(i)
#  define BOOST_PP_LOCAL_LIMITS (0, N-1)
#  include BOOST_PP_LOCAL_ITERATE()
# endif 
            // all converters have been checked. Now we can do the
            // precall part of the policy
            if (!m_data.second().precall(inner_args))
                return 0;

            PyObject* result = detail::invoke(
                detail::invoke_tag<result_t,F>()
              , create_result_converter(args_, (result_converter*)0, (result_converter*)0)
              , m_data.first()
                BOOST_PP_ENUM_TRAILING_PARAMS(N, c)
            );
            
            return m_data.second().postcall(inner_args, result);
        }

        static unsigned min_arity() { return N; }
        
        static py_func_sig_info  signature()
        {
            const signature_element * sig = detail::signature<Sig>::elements();
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
            // MSVC 15.7.2, when compiling to /O2 left the static const signature_element ret, 
            // originally defined here, uninitialized. This in turn led to SegFault in Python interpreter.
            // Issue is resolved by moving the generation of ret to separate function in detail namespace (see above).
            const signature_element * ret = detail::get_ret<Policies, Sig>();

            py_func_sig_info res = {sig, ret };
#else
            py_func_sig_info res = {sig, sig };
#endif

            return  res;
        }
     private:
        compressed_pair<F,Policies> m_data;
    };
};



#endif // BOOST_PP_IS_ITERATING 


