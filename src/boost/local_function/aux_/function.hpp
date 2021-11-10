
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#if !BOOST_PP_IS_ITERATING
#   ifndef BOOST_LOCAL_FUNCTION_AUX_FUNCTION_HPP_
#       define BOOST_LOCAL_FUNCTION_AUX_FUNCTION_HPP_

#       include <boost/local_function/config.hpp>
#       include <boost/local_function/aux_/member.hpp>
#       include <boost/call_traits.hpp>
#       include <boost/typeof/typeof.hpp>
#       include <boost/config.hpp>
#       include <boost/preprocessor/iteration/iterate.hpp>
#       include <boost/preprocessor/repetition/repeat.hpp>
#       include <boost/preprocessor/repetition/enum.hpp>
#       include <boost/preprocessor/punctuation/comma_if.hpp>
#       include <boost/preprocessor/arithmetic/add.hpp>
#       include <boost/preprocessor/arithmetic/sub.hpp>
#       include <boost/preprocessor/arithmetic/inc.hpp>
#       include <boost/preprocessor/control/iif.hpp>
#       include <boost/preprocessor/cat.hpp>

// PRIVATE //

#define BOOST_LOCAL_FUNCTION_AUX_FUNCTION_THIS_FILE_ \
    "boost/local_function/aux_/function.hpp"

// PUBLIC //

#define BOOST_LOCAL_FUNCTION_AUX_FUNCTION_INIT_CALL_FUNC \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (init_call) )

#define BOOST_LOCAL_FUNCTION_AUX_typename_seq(z, n, unused) \
    (typename)

#define BOOST_LOCAL_FUNCTION_AUX_arg_type(z, arg_n, unused) \
    BOOST_PP_CAT(Arg, arg_n)

#define BOOST_LOCAL_FUNCTION_AUX_arg_typedef(z, arg_n, unused) \
    typedef \
        BOOST_LOCAL_FUNCTION_AUX_arg_type(z, arg_n, ~) \
        /* name must follow Boost.FunctionTraits arg1_type, arg2_type, ... */ \
        BOOST_PP_CAT(BOOST_PP_CAT(arg, BOOST_PP_INC(arg_n)), _type) \
    ;

#define BOOST_LOCAL_FUNCTION_AUX_comma_arg_tparam(z, arg_n, unused) \
    , typename BOOST_LOCAL_FUNCTION_AUX_arg_type(z, arg_n, ~)

#define BOOST_LOCAL_FUNCTION_AUX_arg_param_type(z, arg_n, comma01) \
    BOOST_PP_COMMA_IF(comma01) \
    typename ::boost::call_traits< \
        BOOST_LOCAL_FUNCTION_AUX_arg_type(z, arg_n, ~) \
    >::param_type

#define BOOST_LOCAL_FUNCTION_AUX_arg_name(z, arg_n, comma01) \
    BOOST_PP_COMMA_IF(comma01) \
    BOOST_PP_CAT(arg, arg_n)

#define BOOST_LOCAL_FUNCTION_AUX_arg_param_decl(z, arg_n, unused) \
    BOOST_LOCAL_FUNCTION_AUX_arg_param_type(z, arg_n, 0 /* no leading comma */)\
    BOOST_LOCAL_FUNCTION_AUX_arg_name(z, arg_n, 0 /* no leading comma */)

#define BOOST_LOCAL_FUNCTION_AUX_bind_type(z, bind_n, unused) \
    BOOST_PP_CAT(Bind, bind_n)

#define BOOST_LOCAL_FUNCTION_AUX_comma_bind_type(z, bind_n, unused) \
    , BOOST_LOCAL_FUNCTION_AUX_bind_type(z, bind_n, ~)

#define BOOST_LOCAL_FUNCTION_AUX_comma_bind_ref(z, bind_n, unused) \
    , BOOST_LOCAL_FUNCTION_AUX_bind_type(z, bind_n, ~) &

#define BOOST_LOCAL_FUNCTION_AUX_comma_bind_tparam(z, bind_n, unused) \
    , typename BOOST_LOCAL_FUNCTION_AUX_bind_type(z, bind_n, ~)

#define BOOST_LOCAL_FUNCTION_AUX_bind_name(z, bind_n, unused) \
    BOOST_PP_CAT(bing, bind_n)

#define BOOST_LOCAL_FUNCTION_AUX_comma_bind_param_decl(z, bind_n, unused) \
    , \
    BOOST_LOCAL_FUNCTION_AUX_bind_type(z, bind_n, ~) & \
    BOOST_LOCAL_FUNCTION_AUX_bind_name(z, bind_n, ~)
    
#define BOOST_LOCAL_FUNCTION_AUX_bind_member(z, bind_n, unsued) \
    BOOST_PP_CAT(BOOST_LOCAL_FUNCTION_AUX_bind_name(z, bind_n, ~), _)

#define BOOST_LOCAL_FUNCTION_AUX_comma_bind_member_deref(z, bind_n, unsued) \
    , member_deref< BOOST_LOCAL_FUNCTION_AUX_bind_type(z, bind_n, ~) >( \
            BOOST_LOCAL_FUNCTION_AUX_bind_member(z, bind_n, ~))

#define BOOST_LOCAL_FUNCTION_AUX_bind_member_init(z, bind_n, unused) \
    BOOST_LOCAL_FUNCTION_AUX_bind_member(z, bind_n, ~) = member_addr( \
            BOOST_LOCAL_FUNCTION_AUX_bind_name(z, bind_n, ~));

#define BOOST_LOCAL_FUNCTION_AUX_bind_member_decl(z, bind_n, unused) \
    /* must be ptr (not ref) so can use default constr */ \
    typename member_type< \
        BOOST_LOCAL_FUNCTION_AUX_bind_type(z, bind_n, ~) \
    >::pointer BOOST_LOCAL_FUNCTION_AUX_bind_member(z, bind_n, ~) ;

#define BOOST_LOCAL_FUNCTION_AUX_call_ptr(z, n, unused) \
    BOOST_PP_CAT(call_ptr, n)

#define BOOST_LOCAL_FUNCTION_AUX_call_name(z, n, unused) \
    BOOST_PP_CAT(call, n)

#define BOOST_LOCAL_FUNCTION_AUX_call_member(z, n, unused) \
    BOOST_PP_CAT(BOOST_LOCAL_FUNCTION_AUX_call_name(z, n, unused), _)

#define BOOST_LOCAL_FUNCTION_AUX_call_typedef(z, n, arity) \
    typedef R (*BOOST_LOCAL_FUNCTION_AUX_call_ptr(z, n, ~))( \
        object_ptr \
        BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS, \
            BOOST_PP_TUPLE_EAT(3) \
        , \
            BOOST_PP_REPEAT_ ## z \
        )(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX, \
                BOOST_LOCAL_FUNCTION_AUX_comma_bind_ref, ~) \
        BOOST_PP_REPEAT_ ## z(BOOST_PP_SUB(arity, n), \
                BOOST_LOCAL_FUNCTION_AUX_arg_param_type, 1 /* leading comma */)\
    );

#define BOOST_LOCAL_FUNCTION_AUX_comma_call_param_decl(z, n, unused) \
    , \
    BOOST_LOCAL_FUNCTION_AUX_call_ptr(z, n, ~) \
    BOOST_LOCAL_FUNCTION_AUX_call_name(z, n, ~)

#define BOOST_LOCAL_FUNCTION_AUX_call_decl(z, n, unused) \
    BOOST_LOCAL_FUNCTION_AUX_call_ptr(z, n, ~) \
    BOOST_LOCAL_FUNCTION_AUX_call_member(z, n, ~);

#define BOOST_LOCAL_FUNCTION_AUX_call_init(z, n, unused) \
    BOOST_LOCAL_FUNCTION_AUX_call_member(z, n, ~) = \
            BOOST_LOCAL_FUNCTION_AUX_call_name(z, n, ~);
                
#define BOOST_LOCAL_FUNCTION_AUX_operator_call(z, defaults_n, arity) \
    /* precondition: object_ && call_function_ */ \
    inline R operator()( \
        BOOST_PP_ENUM_ ## z(BOOST_PP_SUB(arity, defaults_n), \
                BOOST_LOCAL_FUNCTION_AUX_arg_param_decl, ~) \
    ) /* cannot be const because of binds (same as for local ftor) */ { \
        /* run-time: do not assert preconditions here for efficiency */ \
        /* run-time: this function call is done via a function pointer */ \
        /* so unfortunately does not allow for compiler inlining */ \
        /* optimizations (an alternative using virtual function was also */ \
        /* investigated but also virtual functions cannot be optimized */ \
        /* plus they require virtual table lookups to the alternative */ \
        /* performed worst) */ \
        return BOOST_LOCAL_FUNCTION_AUX_call_member(z, defaults_n, ~)( \
            object_ \
            BOOST_PP_IIF( \
                    BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS,\
                BOOST_PP_TUPLE_EAT(3) \
            , \
                BOOST_PP_REPEAT_ ## z \
            )(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX, \
                    BOOST_LOCAL_FUNCTION_AUX_comma_bind_member_deref, ~) \
            BOOST_PP_REPEAT_ ## z(BOOST_PP_SUB(arity, defaults_n), \
                    BOOST_LOCAL_FUNCTION_AUX_arg_name, 1 /* leading comma */) \
        ); \
    }

namespace boost { namespace local_function { namespace aux {

template<
      typename F
    , size_t defaults
#if !BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS
    BOOST_PP_REPEAT(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX,
            BOOST_LOCAL_FUNCTION_AUX_comma_bind_tparam, ~)
#endif
>
class function {}; // Empty template, only use its specializations.

// Iterate within namespace.
#       define BOOST_PP_ITERATION_PARAMS_1 \
                (3, (0, BOOST_LOCAL_FUNCTION_CONFIG_FUNCTION_ARITY_MAX, \
                BOOST_LOCAL_FUNCTION_AUX_FUNCTION_THIS_FILE_))
#       include BOOST_PP_ITERATE() // Iterate over function arity.

} } } // namespace

// Register type for type-of emu (NAME use TYPEOF to deduce this fctor type).
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::local_function::aux::function,
    (typename) // For `F` tparam.
    (size_t) // For `defaults` tparam.
    // MSVC error if using #if instead of PP_IIF here.
    BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS,
        BOOST_PP_TUPLE_EAT(3) // Nothing.
    ,
        BOOST_PP_REPEAT // For bind tparams.
    )(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX,
            BOOST_LOCAL_FUNCTION_AUX_typename_seq, ~)
)

#undef BOOST_LOCAL_FUNCTION_AUX_typename_seq
#undef BOOST_LOCAL_FUNCTION_AUX_arg_type
#undef BOOST_LOCAL_FUNCTION_AUX_arg_typedef
#undef BOOST_LOCAL_FUNCTION_AUX_comma_arg_tparam
#undef BOOST_LOCAL_FUNCTION_AUX_arg_param_type
#undef BOOST_LOCAL_FUNCTION_AUX_arg_name
#undef BOOST_LOCAL_FUNCTION_AUX_arg_param_decl
#undef BOOST_LOCAL_FUNCTION_AUX_bind_type
#undef BOOST_LOCAL_FUNCTION_AUX_comma_bind_type
#undef BOOST_LOCAL_FUNCTION_AUX_comma_bind_ref
#undef BOOST_LOCAL_FUNCTION_AUX_comma_bind_tparam
#undef BOOST_LOCAL_FUNCTION_AUX_bind_name
#undef BOOST_LOCAL_FUNCTION_AUX_comma_bind_param_decl
#undef BOOST_LOCAL_FUNCTION_AUX_bind_member
#undef BOOST_LOCAL_FUNCTION_AUX_comma_bind_member_deref
#undef BOOST_LOCAL_FUNCTION_AUX_bind_member_init
#undef BOOST_LOCAL_FUNCTION_AUX_bind_member_decl
#undef BOOST_LOCAL_FUNCTION_AUX_call_ptr
#undef BOOST_LOCAL_FUNCTION_AUX_call_name
#undef BOOST_LOCAL_FUNCTION_AUX_call_member
#undef BOOST_LOCAL_FUNCTION_AUX_call_typedef
#undef BOOST_LOCAL_FUNCTION_AUX_comma_call_param_decl
#undef BOOST_LOCAL_FUNCTION_AUX_call_decl
#undef BOOST_LOCAL_FUNCTION_AUX_call_init
#undef BOOST_LOCAL_FUNCTION_AUX_operator_call

#   endif // #include guard

#elif BOOST_PP_ITERATION_DEPTH() == 1
#   define BOOST_LOCAL_FUNCTION_AUX_arity BOOST_PP_FRAME_ITERATION(1)
#   define BOOST_PP_ITERATION_PARAMS_2 \
            (3, (0, BOOST_LOCAL_FUNCTION_AUX_arity, \
            BOOST_LOCAL_FUNCTION_AUX_FUNCTION_THIS_FILE_))
#   include BOOST_PP_ITERATE() // Iterate over default params count.
#   undef BOOST_LOCAL_FUNCTION_AUX_arity

#elif BOOST_PP_ITERATION_DEPTH() == 2
#   define BOOST_LOCAL_FUNCTION_AUX_defaults BOOST_PP_FRAME_ITERATION(2)

template<
    typename R
    BOOST_PP_REPEAT(BOOST_LOCAL_FUNCTION_AUX_arity,
            BOOST_LOCAL_FUNCTION_AUX_comma_arg_tparam, ~)
#if !BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS
    BOOST_PP_REPEAT(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX,
            BOOST_LOCAL_FUNCTION_AUX_comma_bind_tparam, ~)
#endif
>
class function<
      R (
        BOOST_PP_ENUM(BOOST_LOCAL_FUNCTION_AUX_arity,
                BOOST_LOCAL_FUNCTION_AUX_arg_type, ~)
      )
    , BOOST_LOCAL_FUNCTION_AUX_defaults
#if !BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS
    BOOST_PP_REPEAT(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX,
            BOOST_LOCAL_FUNCTION_AUX_comma_bind_type, ~)
#endif
> {
    // The object type will actually be a local class which cannot be passed as
    // a template parameter so a generic `void*` pointer is used to hold the
    // object (this pointer will then be cased by the call-function implemented
    // by the local class itself). This is the trick used to pass a local
    // function as a template parameter. This trick uses function pointers for
    // the call-functions and function pointers cannot always be optimized by
    // the compiler (they cannot be inlined) thus this trick increased run-time
    // (another trick using virtual functions for the local class was also
    // investigated but also virtual functions cannot be inlined plus they
    // require virtual tables lookups so the virtual functions trick measured
    // worst run-time performance than the function pointer trick).
    typedef void* object_ptr;
    BOOST_PP_REPEAT(BOOST_PP_INC(BOOST_LOCAL_FUNCTION_AUX_defaults),
            BOOST_LOCAL_FUNCTION_AUX_call_typedef, // INC for no defaults.
            BOOST_LOCAL_FUNCTION_AUX_arity)

public:
    // Provide public type interface following Boost.Function names
    // (traits must be defined in both this and the local functor).
    BOOST_STATIC_CONSTANT(size_t, arity = BOOST_LOCAL_FUNCTION_AUX_arity);
    typedef R result_type;
    BOOST_PP_REPEAT(BOOST_LOCAL_FUNCTION_AUX_arity,
            BOOST_LOCAL_FUNCTION_AUX_arg_typedef, ~)

    // NOTE: Must have default constructor for init without function name in
    // function macro expansion.

    // Cannot be private but it should never be used by programmers directly
    // so used internal symbol.
    inline void BOOST_LOCAL_FUNCTION_AUX_FUNCTION_INIT_CALL_FUNC(
        object_ptr object
#if !BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS
        BOOST_PP_REPEAT(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX,
                BOOST_LOCAL_FUNCTION_AUX_comma_bind_param_decl, ~)
#endif
        BOOST_PP_REPEAT(BOOST_PP_INC(BOOST_LOCAL_FUNCTION_AUX_defaults),
                BOOST_LOCAL_FUNCTION_AUX_comma_call_param_decl, ~)
    ) {
        object_ = object;
#if !BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS
        BOOST_PP_REPEAT(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX,
                BOOST_LOCAL_FUNCTION_AUX_bind_member_init, ~)
#endif
        BOOST_PP_REPEAT(BOOST_PP_INC(BOOST_LOCAL_FUNCTION_AUX_defaults),
                BOOST_LOCAL_FUNCTION_AUX_call_init, ~) // INC for no defaults.
        unused_ = 0; // To avoid a GCC uninitialized warning.
    }
    
    // Result operator(Arg1, ..., ArgN-1, ArgN) -- iff defaults >= 0
    // Result operator(Arg1, ..., ArgN-1)       -- iff defaults >= 1
    // ...                                      -- etc
    BOOST_PP_REPEAT(BOOST_PP_INC(BOOST_LOCAL_FUNCTION_AUX_defaults),
            BOOST_LOCAL_FUNCTION_AUX_operator_call, // INC for no defaults.
            BOOST_LOCAL_FUNCTION_AUX_arity)

private:
    object_ptr object_;
#if !BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS
    BOOST_PP_REPEAT(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX,
            BOOST_LOCAL_FUNCTION_AUX_bind_member_decl, ~)
#endif
    BOOST_PP_REPEAT(BOOST_PP_INC(BOOST_LOCAL_FUNCTION_AUX_defaults),
            BOOST_LOCAL_FUNCTION_AUX_call_decl, ~) // INC for no defaults.

    // run-time: this unused void* member variable allows for compiler
    // optimizations (at least on MSVC it reduces invocation time of about 50%)
    void* unused_;
};

#   undef BOOST_LOCAL_FUNCTION_AUX_defaults
#endif // iteration

