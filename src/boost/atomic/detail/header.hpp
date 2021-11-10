/*
 *             Copyright Andrey Semashev 2020.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/config.hpp>

#if !defined(BOOST_ATOMIC_ENABLE_WARNINGS)

#if defined(BOOST_MSVC)

#pragma warning(push, 3)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
// 'this' : used in base member initializer list
#pragma warning(disable: 4355)
// 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4800)
// unreferenced formal parameter
#pragma warning(disable: 4100)
// conditional expression is constant
#pragma warning(disable: 4127)
// default constructor could not be generated
#pragma warning(disable: 4510)
// copy constructor could not be generated
#pragma warning(disable: 4511)
// assignment operator could not be generated
#pragma warning(disable: 4512)
// function marked as __forceinline not inlined
#pragma warning(disable: 4714)
// decorated name length exceeded, name was truncated
#pragma warning(disable: 4503)
// declaration of 'A' hides previous local declaration
#pragma warning(disable: 4456)
// declaration of 'A' hides global declaration
#pragma warning(disable: 4459)
// 'X': This function or variable may be unsafe. Consider using Y instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#pragma warning(disable: 4996)
// 'A' : multiple assignment operators specified
#pragma warning(disable: 4522)
// unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable: 4146)
// frame pointer register 'ebx' modified by inline assembly code
#pragma warning(disable: 4731)
// alignment is sensitive to packing
#pragma warning(disable: 4121)
// 'struct_name' : structure was padded due to __declspec(align())
#pragma warning(disable: 4324)

#elif defined(BOOST_GCC) && BOOST_GCC >= 40600

#pragma GCC diagnostic push
// unused parameter 'arg'
#pragma GCC diagnostic ignored "-Wunused-parameter"
// missing initializer for member var
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#elif defined(BOOST_CLANG)

#pragma clang diagnostic push
// unused parameter 'arg'
#pragma clang diagnostic ignored "-Wunused-parameter"
// missing initializer for member var
#pragma clang diagnostic ignored "-Wmissing-field-initializers"

#endif

#endif // !defined(BOOST_ATOMIC_ENABLE_WARNINGS)
