/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2010 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
================================================_==============================*/
#if !defined(BOOST_SPIRIT_STRING_TRAITS_OCTOBER_2008_1252PM)
#define BOOST_SPIRIT_STRING_TRAITS_OCTOBER_2008_1252PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/container.hpp>
#include <string>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/if.hpp>
#include <boost/proto/proto_fwd.hpp> // for BOOST_PROTO_DISABLE_IF_IS_CONST
#include <boost/type_traits/is_const.hpp>
#if defined(__GNUC__) && (__GNUC__ < 4)
#include <boost/type_traits/add_const.hpp>
#endif

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Determine if T is a character type
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_char : mpl::false_ {};

    template <typename T>
    struct is_char<T const> : is_char<T> {};

    template <>
    struct is_char<char> : mpl::true_ {};

    template <>
    struct is_char<wchar_t> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // Determine if T is a string
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_string : mpl::false_ {};

    template <typename T>
    struct is_string<T const> : is_string<T> {};

    template <>
    struct is_string<char const*> : mpl::true_ {};

    template <>
    struct is_string<wchar_t const*> : mpl::true_ {};

    template <>
    struct is_string<char*> : mpl::true_ {};

    template <>
    struct is_string<wchar_t*> : mpl::true_ {};

    template <std::size_t N>
    struct is_string<char[N]> : mpl::true_ {};

    template <std::size_t N>
    struct is_string<wchar_t[N]> : mpl::true_ {};

    template <std::size_t N>
    struct is_string<char const[N]> : mpl::true_ {};

    template <std::size_t N>
    struct is_string<wchar_t const[N]> : mpl::true_ {};

    template <std::size_t N>
    struct is_string<char(&)[N]> : mpl::true_ {};

    template <std::size_t N>
    struct is_string<wchar_t(&)[N]> : mpl::true_ {};

    template <std::size_t N>
    struct is_string<char const(&)[N]> : mpl::true_ {};

    template <std::size_t N>
    struct is_string<wchar_t const(&)[N]> : mpl::true_ {};

    template <typename T, typename Traits, typename Allocator>
    struct is_string<std::basic_string<T, Traits, Allocator> > : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // Get the underlying char type of a string
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct char_type_of;

    template <typename T>
    struct char_type_of<T const> : char_type_of<T> {};

    template <>
    struct char_type_of<char> : mpl::identity<char> {};

    template <>
    struct char_type_of<wchar_t> : mpl::identity<wchar_t> {};

    template <>
    struct char_type_of<char const*> : mpl::identity<char const> {};

    template <>
    struct char_type_of<wchar_t const*> : mpl::identity<wchar_t const> {};

    template <>
    struct char_type_of<char*> : mpl::identity<char> {};

    template <>
    struct char_type_of<wchar_t*> : mpl::identity<wchar_t> {};

    template <std::size_t N>
    struct char_type_of<char[N]> : mpl::identity<char> {};

    template <std::size_t N>
    struct char_type_of<wchar_t[N]> : mpl::identity<wchar_t> {};

    template <std::size_t N>
    struct char_type_of<char const[N]> : mpl::identity<char const> {};

    template <std::size_t N>
    struct char_type_of<wchar_t const[N]> : mpl::identity<wchar_t const> {};

    template <std::size_t N>
    struct char_type_of<char(&)[N]> : mpl::identity<char> {};

    template <std::size_t N>
    struct char_type_of<wchar_t(&)[N]> : mpl::identity<wchar_t> {};

    template <std::size_t N>
    struct char_type_of<char const(&)[N]> : mpl::identity<char const> {};

    template <std::size_t N>
    struct char_type_of<wchar_t const(&)[N]> : mpl::identity<wchar_t const> {};

    template <typename T, typename Traits, typename Allocator>
    struct char_type_of<std::basic_string<T, Traits, Allocator> >
      : mpl::identity<T> {};

    ///////////////////////////////////////////////////////////////////////////
    // Get the C string from a string
    ///////////////////////////////////////////////////////////////////////////
    template <typename String>
    struct extract_c_string;

    template <typename String>
    struct extract_c_string
    {
        typedef typename char_type_of<String>::type char_type;

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
        typedef typename extract_c_string<T>::char_type char_type;

        static typename extract_c_string<T>::char_type const* call (T const str)
        {
            return extract_c_string<T>::call(str);
        }
    };

    // Forwarder that strips references
    template <typename T>
    struct extract_c_string<T&>
    {
        typedef typename extract_c_string<T>::char_type char_type;

        static typename extract_c_string<T>::char_type const* call (T& str)
        {
            return extract_c_string<T>::call(str);
        }
    };

    // Forwarder that strips const references
    template <typename T>
    struct extract_c_string<T const&>
    {
        typedef typename extract_c_string<T>::char_type char_type;

        static typename extract_c_string<T>::char_type const* call (T const& str)
        {
            return extract_c_string<T>::call(str);
        }
    };

    template <typename T, typename Traits, typename Allocator>
    struct extract_c_string<std::basic_string<T, Traits, Allocator> >
    {
        typedef T char_type;

        typedef std::basic_string<T, Traits, Allocator> string;

        static T const* call (string const& str)
        {
            return str.c_str();
        }
    };
    
    template <typename T>
    typename extract_c_string<T*>::char_type const*
    get_c_string (T* str)
    {
        return extract_c_string<T*>::call(str);
    }

    template <typename T>
    typename extract_c_string<T const*>::char_type const*
    get_c_string (T const* str)
    {
        return extract_c_string<T const*>::call(str);
    }
    
    template <typename String>
    typename extract_c_string<String>::char_type const*
    get_c_string (String& str)
    {
        return extract_c_string<String>::call(str);
    }

    template <typename String>
    typename extract_c_string<String>::char_type const*
    get_c_string (String const& str)
    {
        return extract_c_string<String>::call(str);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Get the begin/end iterators from a string
    ///////////////////////////////////////////////////////////////////////////

    // Implementation for C-style strings.

// gcc 3.x.x has problems resolving ambiguities here
#if defined(__GNUC__) && (__GNUC__ < 4)
    template <typename T>
    inline typename add_const<T>::type * get_begin(T* str) { return str; }

    template <typename T>
    inline typename add_const<T>::type* get_end(T* str)
    {
        T* last = str;
        while (*last)
            last++;
        return last;
    }
#else
    template <typename T>
    inline T const* get_begin(T const* str) { return str; }

    template <typename T>
    inline T* get_begin(T* str) { return str; }

    template <typename T>
    inline T const* get_end(T const* str)
    {
        T const* last = str;
        while (*last)
            last++;
        return last;
    }

    template <typename T>
    inline T* get_end(T* str)
    {
        T* last = str;
        while (*last)
            last++;
        return last;
    }
#endif

    // Implementation for containers (includes basic_string).
    template <typename T, typename Str>
    inline typename Str::const_iterator get_begin(Str const& str)
    { return str.begin(); }

    template <typename T, typename Str>
    inline typename Str::iterator
    get_begin(Str& str BOOST_PROTO_DISABLE_IF_IS_CONST(Str))
    { return str.begin(); }

    template <typename T, typename Str>
    inline typename Str::const_iterator get_end(Str const& str)
    { return str.end(); }

    template <typename T, typename Str>
    inline typename Str::iterator
    get_end(Str& str BOOST_PROTO_DISABLE_IF_IS_CONST(Str))
    { return str.end(); }

    // Default implementation for other types: try a C-style string
    // conversion.
    // These overloads are explicitly disabled for containers,
    // as they would be ambiguous with the previous ones.
    template <typename T, typename Str>
    inline typename disable_if<is_container<Str>
      , T const*>::type get_begin(Str const& str)
    { return str; }

    template <typename T, typename Str>
    inline typename disable_if<is_container<Str>
      , T const*>::type get_end(Str const& str)
    { return get_end(get_begin<T>(str)); }
}

namespace result_of
{
    template <typename Char, typename T, typename Enable = void>
    struct get_begin
    {
        typedef typename traits::char_type_of<T>::type char_type;

        typedef typename mpl::if_<
            is_const<char_type>
          , char_type const 
          , char_type
        >::type* type;
    };

    template <typename Char, typename Str>
    struct get_begin<Char, Str
      , typename enable_if<traits::is_container<Str> >::type>
    {
        typedef typename mpl::if_<
            is_const<Str>
          , typename Str::const_iterator
          , typename Str::iterator
        >::type type;
    };

    template <typename Char, typename T>
    struct get_end : get_begin<Char, T> {};
}

}}

#endif
