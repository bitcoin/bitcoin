/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2010 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
================================================_==============================*/
#if !defined(BOOST_SPIRIT_X3_STRING_TRAITS_OCTOBER_2008_1252PM)
#define BOOST_SPIRIT_X3_STRING_TRAITS_OCTOBER_2008_1252PM

#include <string>
#include <boost/mpl/bool.hpp>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Get the C string from a string
    ///////////////////////////////////////////////////////////////////////////
    template <typename String>
    struct extract_c_string;

    template <typename String>
    struct extract_c_string
    {
        template <typename T>
        static T const* call (T* str)
        {
            return (T const*)str;
        }

        template <typename T>
        static T const* call (T const* str)
        {
            return str;
        }
    };

    // Forwarder that strips const
    template <typename T>
    struct extract_c_string<T const>
    {
        static decltype(auto) call(T const str)
        {
            return extract_c_string<T>::call(str);
        }
    };

    // Forwarder that strips references
    template <typename T>
    struct extract_c_string<T&>
    {
        static decltype(auto) call(T& str)
        {
            return extract_c_string<T>::call(str);
        }
    };

    // Forwarder that strips const references
    template <typename T>
    struct extract_c_string<T const&>
    {
        static decltype(auto) call(T const& str)
        {
            return extract_c_string<T>::call(str);
        }
    };

    template <typename T, typename Traits, typename Allocator>
    struct extract_c_string<std::basic_string<T, Traits, Allocator> >
    {
        typedef std::basic_string<T, Traits, Allocator> string;

        static T const* call (string const& str)
        {
            return str.c_str();
        }
    };

    template <typename T>
    decltype(auto) get_c_string(T* str)
    {
        return extract_c_string<T*>::call(str);
    }

    template <typename T>
    decltype(auto) get_c_string(T const* str)
    {
        return extract_c_string<T const*>::call(str);
    }

    template <typename String>
    decltype(auto) get_c_string(String& str)
    {
        return extract_c_string<String>::call(str);
    }

    template <typename String>
    decltype(auto) get_c_string(String const& str)
    {
        return extract_c_string<String>::call(str);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Get the begin/end iterators from a string
    ///////////////////////////////////////////////////////////////////////////

    // Implementation for C-style strings.

    template <typename T>
    inline T const* get_string_begin(T const* str) { return str; }

    template <typename T>
    inline T* get_string_begin(T* str) { return str; }

    template <typename T>
    inline T const* get_string_end(T const* str)
    {
        T const* last = str;
        while (*last)
            last++;
        return last;
    }

    template <typename T>
    inline T* get_string_end(T* str)
    {
        T* last = str;
        while (*last)
            last++;
        return last;
    }

    // Implementation for containers (includes basic_string).
    template <typename T, typename Str>
    inline typename Str::const_iterator get_string_begin(Str const& str)
    { return str.begin(); }

    template <typename T, typename Str>
    inline typename Str::iterator
    get_string_begin(Str& str)
    { return str.begin(); }

    template <typename T, typename Str>
    inline typename Str::const_iterator get_string_end(Str const& str)
    { return str.end(); }

    template <typename T, typename Str>
    inline typename Str::iterator
    get_string_end(Str& str)
    { return str.end(); }
}}}}

#endif
