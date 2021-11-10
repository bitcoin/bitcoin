/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    returns.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_RETURNS_H
#define BOOST_HOF_GUARD_RETURNS_H

/// BOOST_HOF_RETURNS
/// ===========
/// 
/// Description
/// -----------
/// 
/// The `BOOST_HOF_RETURNS` macro defines the function as the expression
/// equivalence. It does this by deducing `noexcept` and the return type by
/// using a trailing `decltype`. Instead of repeating the expression for the
/// return type, `noexcept` clause and the function body, this macro will
/// reduce the code duplication from that.
/// 
/// Note: The expression used to deduce the return the type will also
/// constrain the template function and deduce `noexcept` as well, which is
/// different behaviour than using C++14's return type deduction.
/// 
/// Synopsis
/// --------
/// 
///     #define BOOST_HOF_RETURNS(...) 
/// 
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
/// 
///     template<class T, class U>
///     auto sum(T x, U y) BOOST_HOF_RETURNS(x+y);
/// 
///     int main() {
///         assert(3 == sum(1, 2));
///     }
/// 
/// 
/// Incomplete this
/// ---------------
/// 
/// Description
/// -----------
/// 
/// On some non-conformant compilers, such as gcc, the `this` variable cannot
/// be used inside the `BOOST_HOF_RETURNS` macro because it is considered an
/// incomplete type. So the following macros are provided to help workaround
/// the issue.
/// 
/// 
/// Synopsis
/// --------
/// 
///     // Declares the type of the `this` variable
///     #define BOOST_HOF_RETURNS_CLASS(...) 
///     // Used to refer to the `this` variable in the BOOST_HOF_RETURNS macro
///     #define BOOST_HOF_THIS
///     // Used to refer to the const `this` variable in the BOOST_HOF_RETURNS macro
///     #define BOOST_HOF_CONST_THIS
/// 
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
/// 
///     struct add_1
///     {
///         int a;
///         add_1() : a(1) {}
///         
///         BOOST_HOF_RETURNS_CLASS(add_1);
///         
///         template<class T>
///         auto operator()(T x) const 
///         BOOST_HOF_RETURNS(x+BOOST_HOF_CONST_THIS->a);
///     };
/// 
///     int main() {
///         assert(3 == add_1()(2));
///     }
/// 
/// 
/// Mangling overloads
/// ------------------
/// 
/// Description
/// -----------
/// 
/// On older compilers some operations done in the expressions cannot be
/// properly mangled. These macros help provide workarounds for these
/// operations on older compilers.
/// 
/// 
/// Synopsis
/// --------
/// 
///     // Explicitly defines the type for name mangling
///     #define BOOST_HOF_MANGLE_CAST(...) 
///     // C cast for name mangling
///     #define BOOST_HOF_RETURNS_C_CAST(...) 
///     // Reinterpret cast for name mangling
///     #define BOOST_HOF_RETURNS_REINTERPRET_CAST(...) 
///     // Static cast for name mangling
///     #define BOOST_HOF_RETURNS_STATIC_CAST(...) 
///     // Construction for name mangling
///     #define BOOST_HOF_RETURNS_CONSTRUCT(...) 
/// 


#include <boost/hof/config.hpp>
#include <utility>
#include <boost/hof/detail/forward.hpp>
#include <boost/hof/detail/noexcept.hpp>

#define BOOST_HOF_EAT(...)
#define BOOST_HOF_REM(...) __VA_ARGS__

#if BOOST_HOF_HAS_COMPLETE_DECLTYPE && BOOST_HOF_HAS_MANGLE_OVERLOAD
#ifdef _MSC_VER
// Since decltype can't be used in noexcept on MSVC, we can't check for noexcept
// move constructors.
#define BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(...) BOOST_HOF_NOEXCEPT(noexcept(__VA_ARGS__))
#else
#define BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(...) BOOST_HOF_NOEXCEPT(noexcept(static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)))
#endif
#define BOOST_HOF_RETURNS(...) \
BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(__VA_ARGS__) \
-> decltype(__VA_ARGS__) { return __VA_ARGS__; }
#define BOOST_HOF_THIS this
#define BOOST_HOF_CONST_THIS this
#define BOOST_HOF_RETURNS_CLASS(...) \
void fit_returns_class_check() \
{ \
    static_assert(std::is_same<__VA_ARGS__*, decltype(this)>::value, \
        "Returns class " #__VA_ARGS__ " type doesn't match"); \
}

#define BOOST_HOF_MANGLE_CAST(...) BOOST_HOF_REM

#define BOOST_HOF_RETURNS_C_CAST(...) (__VA_ARGS__) BOOST_HOF_REM
#define BOOST_HOF_RETURNS_REINTERPRET_CAST(...) reinterpret_cast<__VA_ARGS__>
#define BOOST_HOF_RETURNS_STATIC_CAST(...) static_cast<__VA_ARGS__>
#define BOOST_HOF_RETURNS_CONSTRUCT(...) __VA_ARGS__
#else
#include <boost/hof/detail/pp.hpp>

#define BOOST_HOF_RETURNS_RETURN(...) return BOOST_HOF_RETURNS_RETURN_X(BOOST_HOF_PP_WALL(__VA_ARGS__))
#define BOOST_HOF_RETURNS_RETURN_X(...) __VA_ARGS__

#ifdef _MSC_VER
// Since decltype can't be used in noexcept on MSVC, we can't check for noexcept
// move constructors.
#define BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(...) BOOST_HOF_NOEXCEPT(noexcept(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(__VA_ARGS__)))
#else
#define BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(...) BOOST_HOF_NOEXCEPT(noexcept(static_cast<decltype(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(__VA_ARGS__))>(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(__VA_ARGS__))))
#endif

#define BOOST_HOF_RETURNS_DECLTYPE(...) \
-> decltype(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(__VA_ARGS__))

#define BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(...) BOOST_HOF_RETURNS_DECLTYPE_CONTEXT_X(BOOST_HOF_PP_WALL(__VA_ARGS__))
#define BOOST_HOF_RETURNS_DECLTYPE_CONTEXT_X(...) __VA_ARGS__

#define BOOST_HOF_RETURNS_THAT(...) BOOST_HOF_PP_IIF(BOOST_HOF_PP_IS_PAREN(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(())))(\
    (boost::hof::detail::check_this<__VA_ARGS__, decltype(this)>(), this), \
    std::declval<__VA_ARGS__>() \
)

#define BOOST_HOF_THIS BOOST_HOF_PP_RAIL(BOOST_HOF_RETURNS_THAT)(fit_this_type)
#define BOOST_HOF_CONST_THIS BOOST_HOF_PP_RAIL(BOOST_HOF_RETURNS_THAT)(fit_const_this_type)

#define BOOST_HOF_RETURNS_CLASS(...) typedef __VA_ARGS__* fit_this_type; typedef const __VA_ARGS__* fit_const_this_type

#define BOOST_HOF_RETURNS(...) \
BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(__VA_ARGS__) \
BOOST_HOF_RETURNS_DECLTYPE(__VA_ARGS__) \
{ BOOST_HOF_RETURNS_RETURN(__VA_ARGS__); }


namespace boost { namespace hof { namespace detail {
template<class Assumed, class T>
struct check_this
{
    static_assert(std::is_same<T, Assumed>::value, "Incorret BOOST_HOF_THIS or BOOST_HOF_CONST_THIS used.");
};

}}} // namespace boost::hof

#endif


#if BOOST_HOF_HAS_MANGLE_OVERLOAD

#define BOOST_HOF_MANGLE_CAST(...) BOOST_HOF_REM

#define BOOST_HOF_RETURNS_C_CAST(...) (__VA_ARGS__) BOOST_HOF_REM
#define BOOST_HOF_RETURNS_REINTERPRET_CAST(...) reinterpret_cast<__VA_ARGS__>
#define BOOST_HOF_RETURNS_STATIC_CAST(...) static_cast<__VA_ARGS__>
#define BOOST_HOF_RETURNS_CONSTRUCT(...) __VA_ARGS__

#else

#define BOOST_HOF_RETURNS_DERAIL_MANGLE_CAST(...) BOOST_HOF_PP_IIF(BOOST_HOF_PP_IS_PAREN(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(())))(\
    BOOST_HOF_REM, \
    std::declval<__VA_ARGS__>() BOOST_HOF_EAT \
)
#define BOOST_HOF_MANGLE_CAST BOOST_HOF_PP_RAIL(BOOST_HOF_RETURNS_DERAIL_MANGLE_CAST)


#define BOOST_HOF_RETURNS_DERAIL_C_CAST(...) BOOST_HOF_PP_IIF(BOOST_HOF_PP_IS_PAREN(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(())))(\
    (__VA_ARGS__) BOOST_HOF_REM, \
    std::declval<__VA_ARGS__>() BOOST_HOF_EAT \
)
#define BOOST_HOF_RETURNS_C_CAST BOOST_HOF_PP_RAIL(BOOST_HOF_RETURNS_DERAIL_C_CAST)


#define BOOST_HOF_RETURNS_DERAIL_REINTERPRET_CAST(...) BOOST_HOF_PP_IIF(BOOST_HOF_PP_IS_PAREN(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(())))(\
    reinterpret_cast<__VA_ARGS__>, \
    std::declval<__VA_ARGS__>() BOOST_HOF_EAT \
)
#define BOOST_HOF_RETURNS_REINTERPRET_CAST BOOST_HOF_PP_RAIL(BOOST_HOF_RETURNS_DERAIL_REINTERPRET_CAST)

#define BOOST_HOF_RETURNS_DERAIL_STATIC_CAST(...) BOOST_HOF_PP_IIF(BOOST_HOF_PP_IS_PAREN(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(())))(\
    static_cast<__VA_ARGS__>, \
    std::declval<__VA_ARGS__>() BOOST_HOF_EAT \
)
#define BOOST_HOF_RETURNS_STATIC_CAST BOOST_HOF_PP_RAIL(BOOST_HOF_RETURNS_DERAIL_STATIC_CAST)

#define BOOST_HOF_RETURNS_DERAIL_CONSTRUCT(...) BOOST_HOF_PP_IIF(BOOST_HOF_PP_IS_PAREN(BOOST_HOF_RETURNS_DECLTYPE_CONTEXT(())))(\
    __VA_ARGS__, \
    std::declval<__VA_ARGS__>() BOOST_HOF_EAT \
)
#define BOOST_HOF_RETURNS_CONSTRUCT BOOST_HOF_PP_RAIL(BOOST_HOF_RETURNS_DERAIL_CONSTRUCT)

#endif

#define BOOST_HOF_AUTO_FORWARD(...) BOOST_HOF_RETURNS_STATIC_CAST(decltype(__VA_ARGS__))(__VA_ARGS__)

#endif
