//  Copyright 2015 Klemens Morgenstern
//
// This file provides a demangling for function names, i.e. entry points of a dll.
//
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DLL_DEMANGLE_SYMBOL_HPP_
#define BOOST_DLL_DEMANGLE_SYMBOL_HPP_

#include <boost/dll/config.hpp>
#include <string>
#include <algorithm>
#include <memory>

#if defined(_MSC_VER) // MSVC, Clang-cl, and ICC on Windows

namespace boost
{
namespace dll
{
namespace detail
{

typedef void * (__cdecl * allocation_function)(std::size_t);
typedef void   (__cdecl * free_function)(void *);

extern "C" char* __unDName( char* outputString,
        const char* name,
        int maxStringLength,    // Note, COMMA is leading following optional arguments
        allocation_function pAlloc,
        free_function pFree,
        unsigned short disableFlags
        );


inline std::string demangle_symbol(const char *mangled_name)
{

    allocation_function alloc =  [](std::size_t size){return static_cast<void*>(new char[size]);};
    free_function free_f      = [](void* p){delete [] static_cast<char*>(p);};



    std::unique_ptr<char> name { __unDName(
            nullptr,
            mangled_name,
            0,
            alloc,
            free_f,
            static_cast<unsigned short>(0))};

    return std::string(name.get());
}
inline std::string demangle_symbol(const std::string& mangled_name)
{
    return demangle_symbol(mangled_name.c_str());
}


}}}
#else

#include <boost/core/demangle.hpp>

namespace boost
{
namespace dll
{
namespace detail
{

inline std::string demangle_symbol(const char *mangled_name)
{

    if (*mangled_name == '_')
    {
        //because it start's with an underline _
        auto dm = boost::core::demangle(mangled_name);
        if (!dm.empty())
            return dm;
        else
            return (mangled_name);
    }

    //could not demangled
    return "";


}

//for my personal convenience
inline std::string demangle_symbol(const std::string& mangled_name)
{
    return demangle_symbol(mangled_name.c_str());
}


}
namespace experimental
{
using ::boost::dll::detail::demangle_symbol;
}

}}

#endif

#endif /* BOOST_DEMANGLE_HPP_ */
