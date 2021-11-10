//  filesystem path_traits.hpp  --------------------------------------------------------//

//  Copyright Beman Dawes 2009

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

#ifndef BOOST_FILESYSTEM_PATH_TRAITS_HPP
#define BOOST_FILESYSTEM_PATH_TRAITS_HPP

#include <boost/filesystem/config.hpp>
#include <boost/system/error_category.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/core/enable_if.hpp>
#include <cstddef>
#include <cwchar> // for mbstate_t
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include <locale>
#include <boost/assert.hpp>

#include <boost/filesystem/detail/header.hpp> // must be the last #include

namespace boost {
namespace filesystem {

BOOST_FILESYSTEM_DECL const system::error_category& codecvt_error_category();
//  uses std::codecvt_base::result used for error codes:
//
//    ok:       Conversion successful.
//    partial:  Not all source characters converted; one or more additional source
//              characters are needed to produce the final target character, or the
//              size of the target intermediate buffer was too small to hold the result.
//    error:    A character in the source could not be converted to the target encoding.
//    noconv:   The source and target characters have the same type and encoding, so no
//              conversion was necessary.

class directory_entry;

namespace path_traits {

typedef std::codecvt< wchar_t, char, std::mbstate_t > codecvt_type;

//  is_pathable type trait; allows disabling over-agressive class path member templates

template< class T >
struct is_pathable
{
    static const bool value = false;
};

template<>
struct is_pathable< char* >
{
    static const bool value = true;
};

template<>
struct is_pathable< const char* >
{
    static const bool value = true;
};

template<>
struct is_pathable< wchar_t* >
{
    static const bool value = true;
};

template<>
struct is_pathable< const wchar_t* >
{
    static const bool value = true;
};

template<>
struct is_pathable< std::string >
{
    static const bool value = true;
};

template<>
struct is_pathable< std::wstring >
{
    static const bool value = true;
};

template<>
struct is_pathable< std::vector< char > >
{
    static const bool value = true;
};

template<>
struct is_pathable< std::vector< wchar_t > >
{
    static const bool value = true;
};

template<>
struct is_pathable< std::list< char > >
{
    static const bool value = true;
};

template<>
struct is_pathable< std::list< wchar_t > >
{
    static const bool value = true;
};

template<>
struct is_pathable< directory_entry >
{
    static const bool value = true;
};

//  Pathable empty

template< class Container >
inline
    // disable_if aids broken compilers (IBM, old GCC, etc.) and is harmless for
    // conforming compilers. Replace by plain "bool" at some future date (2012?)
    typename boost::disable_if< boost::is_array< Container >, bool >::type
    empty(Container const& c)
{
    return c.begin() == c.end();
}

template< class T >
inline bool empty(T* const& c_str)
{
    BOOST_ASSERT(c_str);
    return !*c_str;
}

template< typename T, std::size_t N >
inline bool empty(T (&x)[N])
{
    return !x[0];
}

// value types differ  ---------------------------------------------------------------//
//
//   A from_end argument of 0 is less efficient than a known end, so use only if needed

//  with codecvt

BOOST_FILESYSTEM_DECL
void convert(const char* from,
             const char* from_end, // 0 for null terminated MBCS
             std::wstring& to, codecvt_type const& cvt);

BOOST_FILESYSTEM_DECL
void convert(const wchar_t* from,
             const wchar_t* from_end, // 0 for null terminated MBCS
             std::string& to, codecvt_type const& cvt);

inline void convert(const char* from, std::wstring& to, codecvt_type const& cvt)
{
    BOOST_ASSERT(from);
    convert(from, 0, to, cvt);
}

inline void convert(const wchar_t* from, std::string& to, codecvt_type const& cvt)
{
    BOOST_ASSERT(from);
    convert(from, 0, to, cvt);
}

//  without codecvt

inline void convert(const char* from,
                    const char* from_end, // 0 for null terminated MBCS
                    std::wstring& to);

inline void convert(const wchar_t* from,
                    const wchar_t* from_end, // 0 for null terminated MBCS
                    std::string& to);

inline void convert(const char* from, std::wstring& to);

inline void convert(const wchar_t* from, std::string& to);

// value types same  -----------------------------------------------------------------//

// char with codecvt

inline void convert(const char* from, const char* from_end, std::string& to, codecvt_type const&)
{
    BOOST_ASSERT(from);
    BOOST_ASSERT(from_end);
    to.append(from, from_end);
}

inline void convert(const char* from, std::string& to, codecvt_type const&)
{
    BOOST_ASSERT(from);
    to += from;
}

// wchar_t with codecvt

inline void convert(const wchar_t* from, const wchar_t* from_end, std::wstring& to, codecvt_type const&)
{
    BOOST_ASSERT(from);
    BOOST_ASSERT(from_end);
    to.append(from, from_end);
}

inline void convert(const wchar_t* from, std::wstring& to, codecvt_type const&)
{
    BOOST_ASSERT(from);
    to += from;
}

// char without codecvt

inline void convert(const char* from, const char* from_end, std::string& to)
{
    BOOST_ASSERT(from);
    BOOST_ASSERT(from_end);
    to.append(from, from_end);
}

inline void convert(const char* from, std::string& to)
{
    BOOST_ASSERT(from);
    to += from;
}

// wchar_t without codecvt

inline void convert(const wchar_t* from, const wchar_t* from_end, std::wstring& to)
{
    BOOST_ASSERT(from);
    BOOST_ASSERT(from_end);
    to.append(from, from_end);
}

inline void convert(const wchar_t* from, std::wstring& to)
{
    BOOST_ASSERT(from);
    to += from;
}

//  Source dispatch  -----------------------------------------------------------------//

//  contiguous containers with codecvt
template< class U >
inline void dispatch(std::string const& c, U& to, codecvt_type const& cvt)
{
    if (!c.empty())
        convert(&*c.begin(), &*c.begin() + c.size(), to, cvt);
}
template< class U >
inline void dispatch(std::wstring const& c, U& to, codecvt_type const& cvt)
{
    if (!c.empty())
        convert(&*c.begin(), &*c.begin() + c.size(), to, cvt);
}
template< class U >
inline void dispatch(std::vector< char > const& c, U& to, codecvt_type const& cvt)
{
    if (!c.empty())
        convert(&*c.begin(), &*c.begin() + c.size(), to, cvt);
}
template< class U >
inline void dispatch(std::vector< wchar_t > const& c, U& to, codecvt_type const& cvt)
{
    if (!c.empty())
        convert(&*c.begin(), &*c.begin() + c.size(), to, cvt);
}

//  contiguous containers without codecvt
template< class U >
inline void dispatch(std::string const& c, U& to)
{
    if (!c.empty())
        convert(&*c.begin(), &*c.begin() + c.size(), to);
}
template< class U >
inline void dispatch(std::wstring const& c, U& to)
{
    if (!c.empty())
        convert(&*c.begin(), &*c.begin() + c.size(), to);
}
template< class U >
inline void dispatch(std::vector< char > const& c, U& to)
{
    if (!c.empty())
        convert(&*c.begin(), &*c.begin() + c.size(), to);
}
template< class U >
inline void dispatch(std::vector< wchar_t > const& c, U& to)
{
    if (!c.empty())
        convert(&*c.begin(), &*c.begin() + c.size(), to);
}

//  non-contiguous containers with codecvt
template< class Container, class U >
inline
    // disable_if aids broken compilers (IBM, old GCC, etc.) and is harmless for
    // conforming compilers. Replace by plain "void" at some future date (2012?)
    typename boost::disable_if< boost::is_array< Container >, void >::type
    dispatch(Container const& c, U& to, codecvt_type const& cvt)
{
    if (!c.empty())
    {
        std::basic_string< typename Container::value_type > s(c.begin(), c.end());
        convert(s.c_str(), s.c_str() + s.size(), to, cvt);
    }
}

//  c_str
template< class T, class U >
inline void dispatch(T* const& c_str, U& to, codecvt_type const& cvt)
{
    //    std::cout << "dispatch() const T *\n";
    BOOST_ASSERT(c_str);
    convert(c_str, to, cvt);
}

//  Note: there is no dispatch on C-style arrays because the array may
//  contain a string smaller than the array size.

BOOST_FILESYSTEM_DECL
void dispatch(directory_entry const& de,
#ifdef BOOST_WINDOWS_API
              std::wstring& to,
#else
              std::string& to,
#endif
              codecvt_type const&);

//  non-contiguous containers without codecvt
template< class Container, class U >
inline
    // disable_if aids broken compilers (IBM, old GCC, etc.) and is harmless for
    // conforming compilers. Replace by plain "void" at some future date (2012?)
    typename boost::disable_if< boost::is_array< Container >, void >::type
    dispatch(Container const& c, U& to)
{
    if (!c.empty())
    {
        std::basic_string< typename Container::value_type > seq(c.begin(), c.end());
        convert(seq.c_str(), seq.c_str() + seq.size(), to);
    }
}

//  c_str
template< class T, class U >
inline void dispatch(T* const& c_str, U& to)
{
    //    std::cout << "dispatch() const T *\n";
    BOOST_ASSERT(c_str);
    convert(c_str, to);
}

//  Note: there is no dispatch on C-style arrays because the array may
//  contain a string smaller than the array size.

BOOST_FILESYSTEM_DECL
void dispatch(directory_entry const& de,
#ifdef BOOST_WINDOWS_API
              std::wstring& to
#else
              std::string& to
#endif
);

} // namespace path_traits
} // namespace filesystem
} // namespace boost

#include <boost/filesystem/detail/footer.hpp>

#endif // BOOST_FILESYSTEM_PATH_TRAITS_HPP
