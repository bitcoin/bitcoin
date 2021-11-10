//
// Copyright 2012-2021 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_TYPE_INDEX_HPP
#define BOOST_TYPE_INDEX_HPP

/// \file boost/type_index.hpp
/// \brief Includes minimal set of headers required to use the Boost.TypeIndex library.
///
/// By inclusion of this file most optimal type index classes will be included and used 
/// as a boost::typeindex::type_index and boost::typeindex::type_info.

#include <boost/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

#if defined(BOOST_TYPE_INDEX_USER_TYPEINDEX)
#   include BOOST_TYPE_INDEX_USER_TYPEINDEX
#   ifdef BOOST_HAS_PRAGMA_DETECT_MISMATCH
#       pragma detect_mismatch( "boost__type_index__abi", "user defined type_index class is used: " BOOST_STRINGIZE(BOOST_TYPE_INDEX_USER_TYPEINDEX))
#   endif
#elif (!defined(BOOST_NO_RTTI) && !defined(BOOST_TYPE_INDEX_FORCE_NO_RTTI_COMPATIBILITY)) || defined(BOOST_MSVC)
#   include <boost/type_index/stl_type_index.hpp>
#   if defined(BOOST_NO_RTTI) || defined(BOOST_TYPE_INDEX_FORCE_NO_RTTI_COMPATIBILITY)
#       include <boost/type_index/detail/stl_register_class.hpp>
#       ifdef BOOST_HAS_PRAGMA_DETECT_MISMATCH
#           pragma detect_mismatch( "boost__type_index__abi", "RTTI is off - typeid() is used only for templates")
#       endif
#   else
#       ifdef BOOST_HAS_PRAGMA_DETECT_MISMATCH
#           pragma detect_mismatch( "boost__type_index__abi", "RTTI is used")
#       endif
#   endif
#else
#   include <boost/type_index/ctti_type_index.hpp>
#   include <boost/type_index/detail/ctti_register_class.hpp>
#   ifdef BOOST_HAS_PRAGMA_DETECT_MISMATCH
#       pragma detect_mismatch( "boost__type_index__abi", "RTTI is off - using CTTI")
#   endif
#endif

#ifndef BOOST_TYPE_INDEX_REGISTER_CLASS
#define BOOST_TYPE_INDEX_REGISTER_CLASS
#endif

namespace boost { namespace typeindex {

#if defined(BOOST_TYPE_INDEX_DOXYGEN_INVOKED)

/// \def BOOST_TYPE_INDEX_FUNCTION_SIGNATURE
/// BOOST_TYPE_INDEX_FUNCTION_SIGNATURE is used by boost::typeindex::ctti_type_index class to
/// deduce the name of a type. If your compiler is not recognized
/// by the TypeIndex library and you wish to work with boost::typeindex::ctti_type_index, you may
/// define this macro by yourself.
///
/// BOOST_TYPE_INDEX_FUNCTION_SIGNATURE must be defined to a compiler specific macro
/// that outputs the \b whole function signature \b including \b template \b parameters.
///
/// If your compiler is not recognised and BOOST_TYPE_INDEX_FUNCTION_SIGNATURE is not defined,
/// then a compile-time error will arise at any attempt to use boost::typeindex::ctti_type_index classes.
///
/// See BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS and BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING
/// for an information of how to tune the implementation to make a nice pretty_name() output.
#define BOOST_TYPE_INDEX_FUNCTION_SIGNATURE BOOST_CURRENT_FUNCTION

/// \def BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING
/// This is a helper macro for making correct pretty_names() with RTTI off.
///
/// BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING macro may be defined to
/// '(begin_skip, end_skip, runtime_skip, runtime_skip_until)' with parameters for adding a
/// support for compilers, that by default are not recognized by TypeIndex library.
///
/// \b Example:
///
/// Imagine the situation when
/// \code boost::typeindex::ctti_type_index::type_id<int>().pretty_name() \endcode
/// returns the following string:
/// \code "static const char *boost::detail::ctti<int>::n() [T = int]" \endcode
/// and \code boost::typeindex::ctti_type_index::type_id<short>().pretty_name() \endcode returns the following:
/// \code "static const char *boost::detail::ctti<short>::n() [T = short]" \endcode
///
/// As we may see first 39 characters are "static const char *boost::detail::ctti<" and they do not depend on
/// the type T. After first 39 characters we have a human readable type name which is duplicated at the end
/// of a string. String always ends on ']', which consumes 1 character.
///
/// Now if we define `BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING` to
/// `(39, 1, false, "")` we'll be getting \code "int>::n() [T = int" \endcode
/// for `boost::typeindex::ctti_type_index::type_id<int>().pretty_name()` and \code "short>::n() [T = short" \endcode
/// for `boost::typeindex::ctti_type_index::type_id<short>().pretty_name()`.
///
/// Now we need to take additional care of the characters that go before the last mention of our type. We'll
/// do that by telling the macro that we need to cut off everything that goes before the "T = " including the "T = "
/// itself:
///
/// \code (39, 1, true, "T = ") \endcode
///
/// In case of GCC or Clang command line we need to add the following line while compiling all the sources:
///
/// \code
/// -DBOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING='(39, 1, true, "T = ")'
/// \endcode
/// \param begin_skip How many characters must be skipped at the beginning of the type holding string.
/// Must be a compile time constant.
/// \param end_skip How many characters must be skipped at the end of the type holding string.
/// Must be a compile time constant.
/// \param runtime_skip Do we need additional checks at runtime to cut off the more characters.
/// Must be `true` or `false`.
/// \param runtime_skip_until Skip all the characters before the following string (including the string itself).
/// Must be a compile time array of characters.
///
/// See [RTTI emulation limitations](boost_typeindex/rtti_emulation_limitations.html) for more info.
#define BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING (0, 0, false, "")


    /// Depending on a compiler flags, optimal implementation of type_index will be used 
    /// as a default boost::typeindex::type_index.
    ///
    /// Could be a boost::typeindex::stl_type_index, boost::typeindex::ctti_type_index or 
    /// user defined type_index class.
    ///
    /// \b See boost::typeindex::type_index_facade for a full description of type_index functions.
    typedef platform_specific type_index;
#elif defined(BOOST_TYPE_INDEX_USER_TYPEINDEX)
    // Nothing to do
#elif (!defined(BOOST_NO_RTTI) && !defined(BOOST_TYPE_INDEX_FORCE_NO_RTTI_COMPATIBILITY)) || defined(BOOST_MSVC)
    typedef boost::typeindex::stl_type_index type_index;
#else 
    typedef boost::typeindex::ctti_type_index type_index;
#endif

/// Depending on a compiler flags, optimal implementation of type_info will be used 
/// as a default boost::typeindex::type_info.
///
/// Could be a std::type_info, boost::typeindex::detail::ctti_data or 
/// some user defined class.
///
/// type_info \b is \b not copyable or default constructible. It is \b not assignable too!
typedef type_index::type_info_t type_info;

#if defined(BOOST_TYPE_INDEX_DOXYGEN_INVOKED)

/// \def BOOST_TYPE_INDEX_USER_TYPEINDEX
/// BOOST_TYPE_INDEX_USER_TYPEINDEX can be defined to the path to header file
/// with user provided implementation of type_index.
///
/// See [Making a custom type_index](boost_typeindex/making_a_custom_type_index.html) section
/// of documentation for usage example.
#define BOOST_TYPE_INDEX_USER_TYPEINDEX <full/absolute/path/to/header/with/type_index.hpp>


/// \def BOOST_TYPE_INDEX_REGISTER_CLASS
/// BOOST_TYPE_INDEX_REGISTER_CLASS is used to help to emulate RTTI.
/// Put this macro into the public section of polymorphic class to allow runtime type detection.
///
/// Depending on the typeid() availability this macro will expand to nothing or to virtual helper function
/// `virtual const type_info& boost_type_info_type_id_runtime_() const noexcept`.
///
/// \b Example:
/// \code
/// class A {
/// public:
///     BOOST_TYPE_INDEX_REGISTER_CLASS
///     virtual ~A(){}
/// };
///
/// struct B: public A {
///     BOOST_TYPE_INDEX_REGISTER_CLASS
/// };
///
/// struct C: public B {
///     BOOST_TYPE_INDEX_REGISTER_CLASS
/// };
///
/// ...
///
/// C c1;
/// A* pc1 = &c1;
/// assert(boost::typeindex::type_id<C>() == boost::typeindex::type_id_runtime(*pc1));
/// \endcode
#define BOOST_TYPE_INDEX_REGISTER_CLASS nothing-or-some-virtual-functions

/// \def BOOST_TYPE_INDEX_FORCE_NO_RTTI_COMPATIBILITY
/// BOOST_TYPE_INDEX_FORCE_NO_RTTI_COMPATIBILITY is a helper macro that must be defined if mixing
/// RTTI on/off modules. See
/// [Mixing sources with RTTI on and RTTI off](boost_typeindex/mixing_sources_with_rtti_on_and_.html)
/// section of documentation for more info.
#define BOOST_TYPE_INDEX_FORCE_NO_RTTI_COMPATIBILITY

#endif // defined(BOOST_TYPE_INDEX_DOXYGEN_INVOKED)


/// Function to get boost::typeindex::type_index for a type T.
/// Removes const, volatile && and & modifiers from T.
///
/// \b Example:
/// \code
/// type_index ti = type_id<int&>();
/// std::cout << ti.pretty_name();  // Outputs 'int'
/// \endcode
///
/// \tparam T Type for which type_index must be created.
/// \throw Nothing.
/// \return boost::typeindex::type_index with information about the specified type T.
template <class T>
inline type_index type_id() BOOST_NOEXCEPT {
    return type_index::type_id<T>();
}

/// Function for constructing boost::typeindex::type_index instance for type T. 
/// Does not remove const, volatile, & and && modifiers from T.
///
/// If T has no const, volatile, & and && modifiers, then returns exactly 
/// the same result as in case of calling `type_id<T>()`.
///
/// \b Example:
/// \code
/// type_index ti = type_id_with_cvr<int&>();
/// std::cout << ti.pretty_name();  // Outputs 'int&'
/// \endcode
///
/// \tparam T Type for which type_index must be created.
/// \throw Nothing.
/// \return boost::typeindex::type_index with information about the specified type T.
template <class T>
inline type_index type_id_with_cvr() BOOST_NOEXCEPT {
    return type_index::type_id_with_cvr<T>();
}

/// Function that works exactly like C++ typeid(rtti_val) call, but returns boost::type_index.
///
/// Returns runtime information about specified type.
///
/// \b Requirements: RTTI available or Base and Derived classes must be marked with BOOST_TYPE_INDEX_REGISTER_CLASS.
///
/// \b Example:
/// \code
/// struct Base { virtual ~Base(){} };
/// struct Derived: public Base  {};
/// ...
/// Derived d;
/// Base& b = d;
/// type_index ti = type_id_runtime(b);
/// std::cout << ti.pretty_name();  // Outputs 'Derived'
/// \endcode
///
/// \param runtime_val Variable which runtime type must be returned.
/// \throw Nothing.
/// \return boost::typeindex::type_index with information about the specified variable.
template <class T>
inline type_index type_id_runtime(const T& runtime_val) BOOST_NOEXCEPT {
    return type_index::type_id_runtime(runtime_val);
}

}} // namespace boost::typeindex



#endif // BOOST_TYPE_INDEX_HPP

