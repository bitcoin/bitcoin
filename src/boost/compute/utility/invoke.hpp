//---------------------------------------------------------------------------//
// Copyright (c) 2013-2015 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://kylelutz.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_UTILITY_INVOKE_HPP
#define BOOST_COMPUTE_UTILITY_INVOKE_HPP

#include <boost/preprocessor/enum.hpp>
#include <boost/preprocessor/repetition.hpp>

#include <boost/compute/config.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/container/detail/scalar.hpp>
#include <boost/compute/type_traits/result_of.hpp>

namespace boost {
namespace compute {

#define BOOST_COMPUTE_DETAIL_INVOKE_ARG(z, n, unused) \
    BOOST_PP_COMMA_IF(n) k.var<BOOST_PP_CAT(T, n)>("arg" BOOST_PP_STRINGIZE(n))

#define BOOST_COMPUTE_DETAIL_INVOKE_ADD_ARG(z, n, unused) \
    k.add_set_arg("arg" BOOST_PP_STRINGIZE(n), BOOST_PP_CAT(arg, n));

#define BOOST_COMPUTE_DETAIL_DEFINE_INVOKE(z, n, unused) \
template<class Function, BOOST_PP_ENUM_PARAMS(n, class T)> \
inline typename result_of<Function(BOOST_PP_ENUM_PARAMS(n, T))>::type \
invoke(const Function& function, command_queue& queue, BOOST_PP_ENUM_BINARY_PARAMS(n, const T, &arg)) \
{ \
    typedef typename result_of<Function(BOOST_PP_ENUM_PARAMS(n, T))>::type result_type; \
    detail::meta_kernel k("invoke"); \
    detail::scalar<result_type> result(queue.get_context()); \
    const size_t result_arg = k.add_arg<result_type *>(memory_object::global_memory, "result"); \
    BOOST_PP_REPEAT(n, BOOST_COMPUTE_DETAIL_INVOKE_ADD_ARG, ~) \
    k << "*result = " << function( \
        BOOST_PP_REPEAT(n, BOOST_COMPUTE_DETAIL_INVOKE_ARG, ~) \
    ) << ";"; \
    k.set_arg(result_arg, result.get_buffer()); \
    k.exec(queue); \
    return result.read(queue); \
}

BOOST_PP_REPEAT_FROM_TO(1, BOOST_COMPUTE_MAX_ARITY, BOOST_COMPUTE_DETAIL_DEFINE_INVOKE, ~)

#undef BOOST_COMPUTE_DETAIL_INVOKE_ARG
#undef BOOST_COMPUTE_DETAIL_INVOKE_ADD_ARG
#undef BOOST_COMPUTE_DETAIL_DEFINE_INVOKE

#ifdef BOOST_COMPUTE_DOXYGEN_INVOKED
/// Invokes \p function with \p args on \p queue.
///
/// For example, to invoke the builtin abs() function:
/// \code
/// int result = invoke(abs<int>(), queue, -10); // returns 10
/// \endcode
template<class Function, class... Args>
inline typename result_of<Function(Args...)>::type
invoke(const Function& function, command_queue& queue, const Args&... args);
#endif // BOOST_COMPUTE_DOXYGEN_INVOKED

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_UTILITY_INVOKE_HPP
