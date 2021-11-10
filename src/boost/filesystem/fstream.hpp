//  boost/filesystem/fstream.hpp  ------------------------------------------------------//

//  Copyright Beman Dawes 2002
//  Copyright Andrey Semashev 2021

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#ifndef BOOST_FILESYSTEM_FSTREAM_HPP
#define BOOST_FILESYSTEM_FSTREAM_HPP

#include <boost/filesystem/config.hpp>
#include <boost/filesystem/path.hpp>
#include <iosfwd>
#include <fstream>

#include <boost/filesystem/detail/header.hpp> // must be the last #include

#if defined(BOOST_WINDOWS_API)
// On Windows, except for standard libaries known to have wchar_t overloads for
// file stream I/O, use path::string() to get a narrow character c_str()
#if (defined(_CPPLIB_VER) && _CPPLIB_VER >= 405 && !defined(_STLPORT_VERSION)) || \
    (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 7000 && defined(_LIBCPP_HAS_OPEN_WITH_WCHAR))
// Use wide characters directly
// Note: We don't use C++17 std::filesystem::path as a means to pass wide paths
// to file streams because of various problems:
// - std::filesystem is available in gcc 8 but it is broken there (fails to compile path definition
//   on Windows). Compilation errors seem to be fixed since gcc 9.
// - In gcc 10.2 and clang 8.0.1 on Cygwin64, the path attempts to convert the wide string to narrow
//   and fails in runtime. This may be system locale dependent, and performing character code conversion
//   is against the purpose of using std::filesystem::path anyway.
// - Other std::filesystem implementations were not tested, so it is not known if they actually work
//   with wide paths.
#define BOOST_FILESYSTEM_C_STR(p) p.c_str()
#else
// Use narrow characters, since wide not available
#define BOOST_FILESYSTEM_C_STR(p) p.string().c_str()
#endif
#endif // defined(BOOST_WINDOWS_API)

#if !defined(BOOST_FILESYSTEM_C_STR)
#define BOOST_FILESYSTEM_C_STR(p) p.c_str()
#endif

#if defined(BOOST_MSVC)
#pragma warning(push)
// 'boost::filesystem::basic_fstream<charT>' : inherits 'std::basic_istream<_Elem,_Traits>::std::basic_istream<_Elem,_Traits>::_Add_vtordisp1' via dominance
#pragma warning(disable : 4250)
#endif

namespace boost {
namespace filesystem {

//--------------------------------------------------------------------------------------//
//                                  basic_filebuf                                       //
//--------------------------------------------------------------------------------------//

template< class charT, class traits = std::char_traits< charT > >
class basic_filebuf :
    public std::basic_filebuf< charT, traits >
{
public:
    BOOST_DEFAULTED_FUNCTION(basic_filebuf(), {})
    BOOST_DELETED_FUNCTION(basic_filebuf(basic_filebuf const&))
    BOOST_DELETED_FUNCTION(basic_filebuf const& operator=(basic_filebuf const&))

public:
    basic_filebuf< charT, traits >* open(path const& p, std::ios_base::openmode mode)
    {
        return std::basic_filebuf< charT, traits >::open(BOOST_FILESYSTEM_C_STR(p), mode) ? this : 0;
    }
};

//--------------------------------------------------------------------------------------//
//                                 basic_ifstream                                       //
//--------------------------------------------------------------------------------------//

template< class charT, class traits = std::char_traits< charT > >
class basic_ifstream :
    public std::basic_ifstream< charT, traits >
{
public:
    BOOST_DEFAULTED_FUNCTION(basic_ifstream(), {})

    // use two signatures, rather than one signature with default second
    // argument, to workaround VC++ 7.1 bug (ID VSWhidbey 38416)

    explicit basic_ifstream(path const& p) :
        std::basic_ifstream< charT, traits >(BOOST_FILESYSTEM_C_STR(p), std::ios_base::in) {}

    basic_ifstream(path const& p, std::ios_base::openmode mode) :
        std::basic_ifstream< charT, traits >(BOOST_FILESYSTEM_C_STR(p), mode) {}

    BOOST_DELETED_FUNCTION(basic_ifstream(basic_ifstream const&))
    BOOST_DELETED_FUNCTION(basic_ifstream const& operator=(basic_ifstream const&))

public:
    void open(path const& p)
    {
        std::basic_ifstream< charT, traits >::open(BOOST_FILESYSTEM_C_STR(p), std::ios_base::in);
    }

    void open(path const& p, std::ios_base::openmode mode)
    {
        std::basic_ifstream< charT, traits >::open(BOOST_FILESYSTEM_C_STR(p), mode);
    }
};

//--------------------------------------------------------------------------------------//
//                                 basic_ofstream                                       //
//--------------------------------------------------------------------------------------//

template< class charT, class traits = std::char_traits< charT > >
class basic_ofstream :
    public std::basic_ofstream< charT, traits >
{
public:
    BOOST_DEFAULTED_FUNCTION(basic_ofstream(), {})

    // use two signatures, rather than one signature with default second
    // argument, to workaround VC++ 7.1 bug (ID VSWhidbey 38416)

    explicit basic_ofstream(path const& p) :
        std::basic_ofstream< charT, traits >(BOOST_FILESYSTEM_C_STR(p), std::ios_base::out) {}

    basic_ofstream(path const& p, std::ios_base::openmode mode) :
        std::basic_ofstream< charT, traits >(BOOST_FILESYSTEM_C_STR(p), mode) {}

    BOOST_DELETED_FUNCTION(basic_ofstream(basic_ofstream const&))
    BOOST_DELETED_FUNCTION(basic_ofstream const& operator=(basic_ofstream const&))

public:
    void open(path const& p)
    {
        std::basic_ofstream< charT, traits >::open(BOOST_FILESYSTEM_C_STR(p), std::ios_base::out);
    }

    void open(path const& p, std::ios_base::openmode mode)
    {
        std::basic_ofstream< charT, traits >::open(BOOST_FILESYSTEM_C_STR(p), mode);
    }
};

//--------------------------------------------------------------------------------------//
//                                  basic_fstream                                       //
//--------------------------------------------------------------------------------------//

template< class charT, class traits = std::char_traits< charT > >
class basic_fstream :
    public std::basic_fstream< charT, traits >
{
public:
    BOOST_DEFAULTED_FUNCTION(basic_fstream(), {})

    // use two signatures, rather than one signature with default second
    // argument, to workaround VC++ 7.1 bug (ID VSWhidbey 38416)

    explicit basic_fstream(path const& p) :
        std::basic_fstream< charT, traits >(BOOST_FILESYSTEM_C_STR(p), std::ios_base::in | std::ios_base::out) {}

    basic_fstream(path const& p, std::ios_base::openmode mode) :
        std::basic_fstream< charT, traits >(BOOST_FILESYSTEM_C_STR(p), mode) {}

    BOOST_DELETED_FUNCTION(basic_fstream(basic_fstream const&))
    BOOST_DELETED_FUNCTION(basic_fstream const& operator=(basic_fstream const&))

public:
    void open(path const& p)
    {
        std::basic_fstream< charT, traits >::open(BOOST_FILESYSTEM_C_STR(p), std::ios_base::in | std::ios_base::out);
    }

    void open(path const& p, std::ios_base::openmode mode)
    {
        std::basic_fstream< charT, traits >::open(BOOST_FILESYSTEM_C_STR(p), mode);
    }
};

//--------------------------------------------------------------------------------------//
//                                    typedefs                                          //
//--------------------------------------------------------------------------------------//

typedef basic_filebuf< char > filebuf;
typedef basic_ifstream< char > ifstream;
typedef basic_ofstream< char > ofstream;
typedef basic_fstream< char > fstream;

typedef basic_filebuf< wchar_t > wfilebuf;
typedef basic_ifstream< wchar_t > wifstream;
typedef basic_ofstream< wchar_t > wofstream;
typedef basic_fstream< wchar_t > wfstream;

} // namespace filesystem
} // namespace boost

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#include <boost/filesystem/detail/footer.hpp>

#endif // BOOST_FILESYSTEM_FSTREAM_HPP
