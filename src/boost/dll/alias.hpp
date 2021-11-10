// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright 2015-2021 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_ALIAS_HPP
#define BOOST_DLL_ALIAS_HPP

#include <boost/dll/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/predef/compiler.h>
#include <boost/predef/os.h>
#include <boost/dll/detail/aggressive_ptr_cast.hpp>

#if BOOST_COMP_GNUC // MSVC does not have <stdint.h> and defines it in some other header, MinGW requires that header.
#include <stdint.h> // intptr_t
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

/// \file boost/dll/alias.hpp
/// \brief Includes alias methods and macro. You can include this header or
/// boost/dll/shared_library.hpp to reduce dependencies
/// in case you do not use the refcountable functions.

namespace boost { namespace dll {

#ifdef BOOST_DLL_DOXYGEN
/// Define this macro to explicitly specify translation unit in which alias must be instantiated.
/// See section 'Limitations' for more info. You may find usage examples in source codes of almost each tutorial.
/// Must be used in code, when \forcedmacrolink{BOOST_DLL_FORCE_NO_WEAK_EXPORTS} is defined
#define BOOST_DLL_FORCE_ALIAS_INSTANTIATION

/// Define this macro to disable exporting weak symbols and start using the \forcedmacrolink{BOOST_DLL_FORCE_ALIAS_INSTANTIATION}.
/// This may be useful for working around linker problems or to test your program for compatibility with linkers that do not support export of weak symbols.
#define BOOST_DLL_FORCE_NO_WEAK_EXPORTS
#endif

#if defined(_MSC_VER) // MSVC, Clang-cl, and ICC on Windows

#define BOOST_DLL_SELECTANY __declspec(selectany)

#define BOOST_DLL_SECTION(SectionName, Permissions)                                             \
    BOOST_STATIC_ASSERT_MSG(                                                                    \
        sizeof(#SectionName) < 10,                                                              \
        "Some platforms require section names to be at most 8 bytes"                            \
    );                                                                                          \
    __pragma(section(#SectionName, Permissions)) __declspec(allocate(#SectionName))             \
    /**/

#else // #if BOOST_COMP_MSVC


#if BOOST_OS_WINDOWS || BOOST_OS_ANDROID || BOOST_COMP_IBM
// There are some problems with mixing `__dllexport__` and `weak` using MinGW
// See https://sourceware.org/bugzilla/show_bug.cgi?id=17480
//
// Android had an issue with exporting weak symbols
// https://code.google.com/p/android/issues/detail?id=70206
#define BOOST_DLL_SELECTANY
#else // #if BOOST_OS_WINDOWS
/*!
* \brief Macro that allows linker to select any occurrence of this symbol instead of
* failing with 'multiple definitions' error at linktime.
*
* This macro does not work on Android, IBM XL C/C++ and MinGW+Windows
* because of linker problems with exporting weak symbols
* (See https://code.google.com/p/android/issues/detail?id=70206, https://sourceware.org/bugzilla/show_bug.cgi?id=17480)
*/
#define BOOST_DLL_SELECTANY __attribute__((weak))
#endif // #if BOOST_OS_WINDOWS

// TODO: improve section permissions using following info:
// http://stackoverflow.com/questions/6252812/what-does-the-aw-flag-in-the-section-attribute-mean

#if !BOOST_OS_MACOS && !BOOST_OS_IOS
/*!
* \brief Macro that puts symbol to a specific section. On MacOS all the sections are put into "__DATA" segment.
* \param SectionName Name of the section. Must be a valid C identifier without quotes not longer than 8 bytes.
* \param Permissions Can be "read" or "write" (without quotes!).
*/
#define BOOST_DLL_SECTION(SectionName, Permissions)                                             \
    BOOST_STATIC_ASSERT_MSG(                                                                    \
        sizeof(#SectionName) < 10,                                                              \
        "Some platforms require section names to be at most 8 bytes"                            \
    );                                                                                          \
    __attribute__ ((section (#SectionName)))                                                    \
    /**/
#else // #if !BOOST_OS_MACOS && !BOOST_OS_IOS

#define BOOST_DLL_SECTION(SectionName, Permissions)                                             \
    BOOST_STATIC_ASSERT_MSG(                                                                    \
        sizeof(#SectionName) < 10,                                                              \
        "Some platforms require section names to be at most 8 bytes"                            \
    );                                                                                          \
    __attribute__ ((section ( "__DATA," #SectionName)))                                         \
    /**/

#endif // #if #if !BOOST_OS_MACOS && !BOOST_OS_IOS

#endif // #if BOOST_COMP_MSVC


// Alias - is just a variable that pointers to original data
//
// A few attempts were made to avoid additional indirection:
// 1) 
//          // Does not work on Windows, work on Linux
//          extern "C" BOOST_SYMBOL_EXPORT void AliasName() {
//              reinterpret_cast<void (*)()>(Function)();
//          }
//
// 2) 
//          // Does not work on Linux (changes permissions of .text section and produces incorrect DSO)
//          extern "C" BOOST_SYMBOL_EXPORT void* __attribute__ ((section(".text#"))) 
//                  func_ptr = *reinterpret_cast<std::ptrdiff_t**>(&foo::bar);
//
// 3)       // requires mangled name of `Function` 
//          //  AliasName() __attribute__ ((weak, alias ("Function")))  
//
//          // hard to use
//          `#pragma comment(linker, "/alternatename:_pWeakValue=_pDefaultWeakValue")`

/*!
* \brief Makes an alias name for exported function or variable.
*
* This macro is useful in cases of long mangled C++ names. For example some `void boost::foo(std::string)`
* function name will change to something like `N5boostN3foosE` after mangling.
* Importing function by `N5boostN3foosE` name does not looks user friendly, especially assuming the fact
* that different compilers have different mangling schemes. AliasName is the name that won't be mangled
* and can be used as a portable import name.
*
*
* Can be used in any namespace, including global. FunctionOrVar must be fully qualified,
* so that address of it could be taken. Multiple different aliases for a single variable/function
* are allowed.
*
* Make sure that AliasNames are unique per library/executable. Functions or variables
* in global namespace must not have names same as AliasNames.
*
* Same AliasName in different translation units must point to the same FunctionOrVar.
*
* Puts all the aliases into the \b "boostdll" read only section of the binary. Equal to
* \forcedmacrolink{BOOST_DLL_ALIAS_SECTIONED}(FunctionOrVar, AliasName, boostdll).
*
* \param FunctionOrVar Function or variable for which an alias must be made.
* \param AliasName Name of the alias. Must be a valid C identifier.
*
* \b Example:
* \code
* namespace foo {
*   void bar(std::string&);
*
*   BOOST_DLL_ALIAS(foo::bar, foo_bar)
* }
*
* BOOST_DLL_ALIAS(foo::bar, foo_bar_another_alias_name)
* \endcode
*
* \b See: \forcedmacrolink{BOOST_DLL_ALIAS_SECTIONED} for making alias in a specific section.
*/
#define BOOST_DLL_ALIAS(FunctionOrVar, AliasName)                       \
    BOOST_DLL_ALIAS_SECTIONED(FunctionOrVar, AliasName, boostdll)       \
    /**/


#if ((BOOST_COMP_GNUC && BOOST_OS_WINDOWS) || BOOST_OS_ANDROID || BOOST_COMP_IBM || defined(BOOST_DLL_FORCE_NO_WEAK_EXPORTS)) \
    && !defined(BOOST_DLL_FORCE_ALIAS_INSTANTIATION) && !defined(BOOST_DLL_DOXYGEN)

#define BOOST_DLL_ALIAS_SECTIONED(FunctionOrVar, AliasName, SectionName)                        \
    namespace _autoaliases {                                                                    \
        extern "C" BOOST_SYMBOL_EXPORT const void *AliasName;                                   \
    } /* namespace _autoaliases */                                                              \
    /**/

#define BOOST_DLL_AUTO_ALIAS(FunctionOrVar)                                                     \
    namespace _autoaliases {                                                                    \
        extern "C" BOOST_SYMBOL_EXPORT const void *FunctionOrVar;                               \
    } /* namespace _autoaliases */                                                              \
    /**/
#else    
// Note: we can not use `aggressive_ptr_cast` here, because in that case GCC applies
// different permissions to the section and it causes Segmentation fault.
// Note: we can not use `boost::addressof()` here, because in that case GCC 
// may optimize away the FunctionOrVar instance and we'll get a pointer to unexisting symbol.
/*!
* \brief Same as \forcedmacrolink{BOOST_DLL_ALIAS} but puts alias name into the user specified section.
*
* \param FunctionOrVar Function or variable for which an alias must be made.
* \param AliasName Name of the alias. Must be a valid C identifier.
* \param SectionName Name of the section. Must be a valid C identifier without quotes not longer than 8 bytes.
*
* \b Example:
* \code
* namespace foo {
*   void bar(std::string&);
*
*   BOOST_DLL_ALIAS_SECTIONED(foo::bar, foo_bar, sect_1) // section "sect_1" now exports "foo_bar"
* }
* \endcode
*
*/
#define BOOST_DLL_ALIAS_SECTIONED(FunctionOrVar, AliasName, SectionName)                        \
    namespace _autoaliases {                                                                    \
        extern "C" BOOST_SYMBOL_EXPORT const void *AliasName;                                   \
        BOOST_DLL_SECTION(SectionName, read) BOOST_DLL_SELECTANY                                \
        const void * AliasName = reinterpret_cast<const void*>(reinterpret_cast<intptr_t>(      \
            &FunctionOrVar                                                                      \
        ));                                                                                     \
    } /* namespace _autoaliases */                                                              \
    /**/

/*!
* \brief Exports variable or function with unmangled alias name.
*
* This macro is useful in cases of long mangled C++ names. For example some `void boost::foo(std::string)`
* function name will change to something like `N5boostN3foosE` after mangling.
* Importing function by `N5boostN3foosE` name does not looks user friendly, especially assuming the fact
* that different compilers have different mangling schemes.*
*
* Must be used in scope where FunctionOrVar declared. FunctionOrVar must be a valid C name, which means that
* it must not contain `::`.
*
* Functions or variables
* in global namespace must not have names same as FunctionOrVar.
*
* Puts all the aliases into the \b "boostdll" read only section of the binary. Almost same as
* \forcedmacrolink{BOOST_DLL_ALIAS}(FunctionOrVar, FunctionOrVar).
*
* \param FunctionOrVar Function or variable for which an unmangled alias must be made.
*
* \b Example:
* \code
* namespace foo {
*   void bar(std::string&);
*   BOOST_DLL_AUTO_ALIAS(bar)
* }
*
* \endcode
*
* \b See: \forcedmacrolink{BOOST_DLL_ALIAS} for making an alias with different names.
*/

#define BOOST_DLL_AUTO_ALIAS(FunctionOrVar)                                                     \
    namespace _autoaliases {                                                                    \
        BOOST_DLL_SELECTANY const void * dummy_ ## FunctionOrVar                                \
            = reinterpret_cast<const void*>(reinterpret_cast<intptr_t>(                         \
                &FunctionOrVar                                                                  \
            ));                                                                                 \
        extern "C" BOOST_SYMBOL_EXPORT const void *FunctionOrVar;                               \
        BOOST_DLL_SECTION(boostdll, read) BOOST_DLL_SELECTANY                                   \
        const void * FunctionOrVar = dummy_ ## FunctionOrVar;                                   \
    } /* namespace _autoaliases */                                                              \
    /**/


#endif


}} // namespace boost::dll


#endif // BOOST_DLL_ALIAS_HPP

