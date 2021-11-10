//  filesystem path.hpp  ---------------------------------------------------------------//

//  Copyright Vladimir Prus 2002
//  Copyright Beman Dawes 2002-2005, 2009
//  Copyright Andrey Semashev 2021

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//  path::stem(), extension(), and replace_extension() are based on
//  basename(), extension(), and change_extension() from the original
//  filesystem/convenience.hpp header by Vladimir Prus.

#ifndef BOOST_FILESYSTEM_PATH_HPP
#define BOOST_FILESYSTEM_PATH_HPP

#include <boost/assert.hpp>
#include <boost/filesystem/config.hpp>
#include <boost/filesystem/path_traits.hpp> // includes <cwchar>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/io/quoted.hpp>
#include <boost/functional/hash_fwd.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <cstddef>
#include <cwchar> // for mbstate_t
#include <string>
#include <iosfwd>
#include <iterator>
#include <locale>
#include <utility>

#include <boost/filesystem/detail/header.hpp> // must be the last #include

namespace boost {
namespace filesystem {
namespace path_detail // intentionally don't use filesystem::detail to not bring internal Boost.Filesystem functions into ADL via path_constants
{

template< typename Char, Char Separator, Char PreferredSeparator, Char Dot >
struct path_constants
{
    typedef path_constants< Char, Separator, PreferredSeparator, Dot > path_constants_base;
    typedef Char value_type;
    static BOOST_CONSTEXPR_OR_CONST value_type separator = Separator;
    static BOOST_CONSTEXPR_OR_CONST value_type preferred_separator = PreferredSeparator;
    static BOOST_CONSTEXPR_OR_CONST value_type dot = Dot;
};

#if defined(BOOST_NO_CXX17_INLINE_VARIABLES)
template< typename Char, Char Separator, Char PreferredSeparator, Char Dot >
BOOST_CONSTEXPR_OR_CONST typename path_constants< Char, Separator, PreferredSeparator, Dot >::value_type
path_constants< Char, Separator, PreferredSeparator, Dot >::separator;
template< typename Char, Char Separator, Char PreferredSeparator, Char Dot >
BOOST_CONSTEXPR_OR_CONST typename path_constants< Char, Separator, PreferredSeparator, Dot >::value_type
path_constants< Char, Separator, PreferredSeparator, Dot >::preferred_separator;
template< typename Char, Char Separator, Char PreferredSeparator, Char Dot >
BOOST_CONSTEXPR_OR_CONST typename path_constants< Char, Separator, PreferredSeparator, Dot >::value_type
path_constants< Char, Separator, PreferredSeparator, Dot >::dot;
#endif

// A struct that denotes a contiguous range of characters in a string. A lightweight alternative to string_view.
struct substring
{
    std::size_t pos;
    std::size_t size;
};

} // namespace path_detail

//------------------------------------------------------------------------------------//
//                                                                                    //
//                                    class path                                      //
//                                                                                    //
//------------------------------------------------------------------------------------//

class path :
    public filesystem::path_detail::path_constants<
#ifdef BOOST_WINDOWS_API
        wchar_t, L'/', L'\\', L'.'
#else
        char, '/', '/', '.'
#endif
    >
{
public:
    //  value_type is the character type used by the operating system API to
    //  represent paths.

    typedef path_constants_base::value_type value_type;
    typedef std::basic_string< value_type > string_type;
    typedef std::codecvt< wchar_t, char, std::mbstate_t > codecvt_type;

    //  ----- character encoding conversions -----

    //  Following the principle of least astonishment, path input arguments
    //  passed to or obtained from the operating system via objects of
    //  class path behave as if they were directly passed to or
    //  obtained from the O/S API, unless conversion is explicitly requested.
    //
    //  POSIX specfies that path strings are passed unchanged to and from the
    //  API. Note that this is different from the POSIX command line utilities,
    //  which convert according to a locale.
    //
    //  Thus for POSIX, char strings do not undergo conversion.  wchar_t strings
    //  are converted to/from char using the path locale or, if a conversion
    //  argument is given, using a conversion object modeled on
    //  std::wstring_convert.
    //
    //  The path locale, which is global to the thread, can be changed by the
    //  imbue() function. It is initialized to an implementation defined locale.
    //
    //  For Windows, wchar_t strings do not undergo conversion. char strings
    //  are converted using the "ANSI" or "OEM" code pages, as determined by
    //  the AreFileApisANSI() function, or, if a conversion argument is given,
    //  using a conversion object modeled on std::wstring_convert.
    //
    //  See m_pathname comments for further important rationale.

    //  TODO: rules needed for operating systems that use / or .
    //  differently, or format directory paths differently from file paths.
    //
    //  **********************************************************************************
    //
    //  More work needed: How to handle an operating system that may have
    //  slash characters or dot characters in valid filenames, either because
    //  it doesn't follow the POSIX standard, or because it allows MBCS
    //  filename encodings that may contain slash or dot characters. For
    //  example, ISO/IEC 2022 (JIS) encoding which allows switching to
    //  JIS x0208-1983 encoding. A valid filename in this set of encodings is
    //  0x1B 0x24 0x42 [switch to X0208-1983] 0x24 0x2F [U+304F Kiragana letter KU]
    //                                             ^^^^
    //  Note that 0x2F is the ASCII slash character
    //
    //  **********************************************************************************

    //  Supported source arguments: half-open iterator range, container, c-array,
    //  and single pointer to null terminated string.

    //  All source arguments except pointers to null terminated byte strings support
    //  multi-byte character strings which may have embedded nulls. Embedded null
    //  support is required for some Asian languages on Windows.

    //  "const codecvt_type& cvt=codecvt()" default arguments are not used because this
    //  limits the impact of locale("") initialization failures on POSIX systems to programs
    //  that actually depend on locale(""). It further ensures that exceptions thrown
    //  as a result of such failues occur after main() has started, so can be caught.

    //  -----  constructors  -----

    path() BOOST_NOEXCEPT {}
    path(path const& p) : m_pathname(p.m_pathname) {}

    template< class Source >
    path(Source const& source, typename boost::enable_if< path_traits::is_pathable< typename boost::decay< Source >::type > >::type* = 0)
    {
        path_traits::dispatch(source, m_pathname);
    }

    path(const value_type* s) : m_pathname(s) {}
    path(value_type* s) : m_pathname(s) {}
    path(string_type const& s) : m_pathname(s) {}
    path(string_type& s) : m_pathname(s) {}

    //  As of October 2015 the interaction between noexcept and =default is so troublesome
    //  for VC++, GCC, and probably other compilers, that =default is not used with noexcept
    //  functions. GCC is not even consistent for the same release on different platforms.

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    path(path&& p) BOOST_NOEXCEPT : m_pathname(std::move(p.m_pathname))
    {
    }
    path& operator=(path&& p) BOOST_NOEXCEPT
    {
        m_pathname = std::move(p.m_pathname);
        return *this;
    }

    path(string_type&& s) BOOST_NOEXCEPT : m_pathname(std::move(s))
    {
    }
    path& operator=(string_type&& p) BOOST_NOEXCEPT
    {
        m_pathname = std::move(p);
        return *this;
    }
#endif

    template< class Source >
    path(Source const& source, codecvt_type const& cvt)
    {
        path_traits::dispatch(source, m_pathname, cvt);
    }

    template< class InputIterator >
    path(InputIterator begin, InputIterator end)
    {
        if (begin != end)
        {
            // convert requires contiguous string, so copy
            std::basic_string< typename std::iterator_traits< InputIterator >::value_type > seq(begin, end);
            path_traits::convert(seq.c_str(), seq.c_str() + seq.size(), m_pathname);
        }
    }

    template< class InputIterator >
    path(InputIterator begin, InputIterator end, codecvt_type const& cvt)
    {
        if (begin != end)
        {
            // convert requires contiguous string, so copy
            std::basic_string< typename std::iterator_traits< InputIterator >::value_type > seq(begin, end);
            path_traits::convert(seq.c_str(), seq.c_str() + seq.size(), m_pathname, cvt);
        }
    }

    //  -----  assignments  -----

    path& operator=(path const& p)
    {
        m_pathname = p.m_pathname;
        return *this;
    }

    template< class Source >
    typename boost::enable_if< path_traits::is_pathable< typename boost::decay< Source >::type >, path& >::type
    operator=(Source const& source)
    {
        m_pathname.clear();
        path_traits::dispatch(source, m_pathname);
        return *this;
    }

    //  value_type overloads

    path& operator=(const value_type* ptr) // required in case ptr overlaps *this
    {
        m_pathname = ptr;
        return *this;
    }

    path& operator=(value_type* ptr) // required in case ptr overlaps *this
    {
        m_pathname = ptr;
        return *this;
    }

    path& operator=(string_type const& s)
    {
        m_pathname = s;
        return *this;
    }

    path& operator=(string_type& s)
    {
        m_pathname = s;
        return *this;
    }

    path& assign(const value_type* ptr, codecvt_type const&) // required in case ptr overlaps *this
    {
        m_pathname = ptr;
        return *this;
    }

    template< class Source >
    path& assign(Source const& source, codecvt_type const& cvt)
    {
        m_pathname.clear();
        path_traits::dispatch(source, m_pathname, cvt);
        return *this;
    }

    template< class InputIterator >
    path& assign(InputIterator begin, InputIterator end)
    {
        m_pathname.clear();
        if (begin != end)
        {
            std::basic_string< typename std::iterator_traits< InputIterator >::value_type > seq(begin, end);
            path_traits::convert(seq.c_str(), seq.c_str() + seq.size(), m_pathname);
        }
        return *this;
    }

    template< class InputIterator >
    path& assign(InputIterator begin, InputIterator end, codecvt_type const& cvt)
    {
        m_pathname.clear();
        if (begin != end)
        {
            std::basic_string< typename std::iterator_traits< InputIterator >::value_type > seq(begin, end);
            path_traits::convert(seq.c_str(), seq.c_str() + seq.size(), m_pathname, cvt);
        }
        return *this;
    }

    //  -----  concatenation  -----

    template< class Source >
    typename boost::enable_if< path_traits::is_pathable< typename boost::decay< Source >::type >, path& >::type
    operator+=(Source const& source)
    {
        return concat(source);
    }

    //  value_type overloads. Same rationale as for constructors above
    path& operator+=(path const& p)
    {
        m_pathname += p.m_pathname;
        return *this;
    }

    path& operator+=(const value_type* ptr)
    {
        m_pathname += ptr;
        return *this;
    }

    path& operator+=(value_type* ptr)
    {
        m_pathname += ptr;
        return *this;
    }

    path& operator+=(string_type const& s)
    {
        m_pathname += s;
        return *this;
    }
    path& operator+=(string_type& s)
    {
        m_pathname += s;
        return *this;
    }

    path& operator+=(value_type c)
    {
        m_pathname += c;
        return *this;
    }

    template< class CharT >
    typename boost::enable_if< boost::is_integral< CharT >, path& >::type
    operator+=(CharT c)
    {
        CharT tmp[2];
        tmp[0] = c;
        tmp[1] = 0;
        return concat(tmp);
    }

    template< class Source >
    path& concat(Source const& source)
    {
        path_traits::dispatch(source, m_pathname);
        return *this;
    }

    template< class Source >
    path& concat(Source const& source, codecvt_type const& cvt)
    {
        path_traits::dispatch(source, m_pathname, cvt);
        return *this;
    }

    template< class InputIterator >
    path& concat(InputIterator begin, InputIterator end)
    {
        if (begin == end)
            return *this;
        std::basic_string< typename std::iterator_traits< InputIterator >::value_type > seq(begin, end);
        path_traits::convert(seq.c_str(), seq.c_str() + seq.size(), m_pathname);
        return *this;
    }

    template< class InputIterator >
    path& concat(InputIterator begin, InputIterator end, codecvt_type const& cvt)
    {
        if (begin == end)
            return *this;
        std::basic_string< typename std::iterator_traits< InputIterator >::value_type > seq(begin, end);
        path_traits::convert(seq.c_str(), seq.c_str() + seq.size(), m_pathname, cvt);
        return *this;
    }

    //  -----  appends  -----

    //  if a separator is added, it is the preferred separator for the platform;
    //  slash for POSIX, backslash for Windows

    BOOST_FILESYSTEM_DECL path& operator/=(path const& p);

    template< class Source >
    typename boost::enable_if< path_traits::is_pathable< typename boost::decay< Source >::type >, path& >::type
    operator/=(Source const& source)
    {
        return append(source);
    }

    BOOST_FILESYSTEM_DECL path& operator/=(const value_type* ptr);
    path& operator/=(value_type* ptr)
    {
        return this->operator/=(const_cast< const value_type* >(ptr));
    }
    path& operator/=(string_type const& s) { return this->operator/=(path(s)); }
    path& operator/=(string_type& s) { return this->operator/=(path(s)); }

    path& append(const value_type* ptr) // required in case ptr overlaps *this
    {
        this->operator/=(ptr);
        return *this;
    }

    path& append(const value_type* ptr, codecvt_type const&) // required in case ptr overlaps *this
    {
        this->operator/=(ptr);
        return *this;
    }

    template< class Source >
    path& append(Source const& source);

    template< class Source >
    path& append(Source const& source, codecvt_type const& cvt);

    template< class InputIterator >
    path& append(InputIterator begin, InputIterator end);

    template< class InputIterator >
    path& append(InputIterator begin, InputIterator end, const codecvt_type& cvt);

    //  -----  modifiers  -----

    void clear() BOOST_NOEXCEPT { m_pathname.clear(); }
#ifdef BOOST_POSIX_API
    path& make_preferred()
    {
        // No effect on POSIX
        return *this;
    }
#else // BOOST_WINDOWS_API
    BOOST_FILESYSTEM_DECL path& make_preferred(); // change slashes to backslashes
#endif
    BOOST_FILESYSTEM_DECL path& remove_filename();
    BOOST_FILESYSTEM_DECL path& remove_trailing_separator();
    BOOST_FILESYSTEM_DECL path& replace_extension(path const& new_extension = path());
    void swap(path& rhs) BOOST_NOEXCEPT { m_pathname.swap(rhs.m_pathname); }

    //  -----  observers  -----

    //  For operating systems that format file paths differently than directory
    //  paths, return values from observers are formatted as file names unless there
    //  is a trailing separator, in which case returns are formatted as directory
    //  paths. POSIX and Windows make no such distinction.

    //  Implementations are permitted to return const values or const references.

    //  The string or path returned by an observer are specified as being formatted
    //  as "native" or "generic".
    //
    //  For POSIX, these are all the same format; slashes and backslashes are as input and
    //  are not modified.
    //
    //  For Windows,   native:    as input; slashes and backslashes are not modified;
    //                            this is the format of the internally stored string.
    //                 generic:   backslashes are converted to slashes

    //  -----  native format observers  -----

    string_type const& native() const BOOST_NOEXCEPT { return m_pathname; }
    const value_type* c_str() const BOOST_NOEXCEPT { return m_pathname.c_str(); }
    string_type::size_type size() const BOOST_NOEXCEPT { return m_pathname.size(); }

    template< class String >
    String string() const;

    template< class String >
    String string(codecvt_type const& cvt) const;

#ifdef BOOST_WINDOWS_API
    std::string string() const
    {
        std::string tmp;
        if (!m_pathname.empty())
            path_traits::convert(m_pathname.c_str(), m_pathname.c_str() + m_pathname.size(), tmp);
        return tmp;
    }
    std::string string(codecvt_type const& cvt) const
    {
        std::string tmp;
        if (!m_pathname.empty())
            path_traits::convert(m_pathname.c_str(), m_pathname.c_str() + m_pathname.size(), tmp, cvt);
        return tmp;
    }

    //  string_type is std::wstring, so there is no conversion
    std::wstring const& wstring() const { return m_pathname; }
    std::wstring const& wstring(codecvt_type const&) const { return m_pathname; }
#else // BOOST_POSIX_API
    //  string_type is std::string, so there is no conversion
    std::string const& string() const { return m_pathname; }
    std::string const& string(codecvt_type const&) const { return m_pathname; }

    std::wstring wstring() const
    {
        std::wstring tmp;
        if (!m_pathname.empty())
            path_traits::convert(m_pathname.c_str(), m_pathname.c_str() + m_pathname.size(), tmp);
        return tmp;
    }
    std::wstring wstring(codecvt_type const& cvt) const
    {
        std::wstring tmp;
        if (!m_pathname.empty())
            path_traits::convert(m_pathname.c_str(), m_pathname.c_str() + m_pathname.size(), tmp, cvt);
        return tmp;
    }
#endif

    //  -----  generic format observers  -----

    //  Experimental generic function returning generic formatted path (i.e. separators
    //  are forward slashes). Motivation: simpler than a family of generic_*string
    //  functions.
#ifdef BOOST_WINDOWS_API
    BOOST_FILESYSTEM_DECL path generic_path() const;
#else
    path generic_path() const { return path(*this); }
#endif

    template< class String >
    String generic_string() const;

    template< class String >
    String generic_string(codecvt_type const& cvt) const;

#ifdef BOOST_WINDOWS_API
    std::string generic_string() const { return generic_path().string(); }
    std::string generic_string(codecvt_type const& cvt) const { return generic_path().string(cvt); }
    std::wstring generic_wstring() const { return generic_path().wstring(); }
    std::wstring generic_wstring(codecvt_type const&) const { return generic_wstring(); }
#else // BOOST_POSIX_API
    //  On POSIX-like systems, the generic format is the same as the native format
    std::string const& generic_string() const { return m_pathname; }
    std::string const& generic_string(codecvt_type const&) const { return m_pathname; }
    std::wstring generic_wstring() const { return this->wstring(); }
    std::wstring generic_wstring(codecvt_type const& cvt) const { return this->wstring(cvt); }
#endif

    //  -----  compare  -----

    BOOST_FILESYSTEM_DECL int compare(path const& p) const BOOST_NOEXCEPT; // generic, lexicographical
    int compare(std::string const& s) const { return compare(path(s)); }
    int compare(const value_type* s) const { return compare(path(s)); }

    //  -----  decomposition  -----

    path root_path() const { return path(m_pathname.c_str(), m_pathname.c_str() + find_root_path_size()); }
    // returns 0 or 1 element path even on POSIX, root_name() is non-empty() for network paths
    path root_name() const { return path(m_pathname.c_str(), m_pathname.c_str() + find_root_name_size()); }

    // returns 0 or 1 element path
    path root_directory() const
    {
        path_detail::substring root_dir = find_root_directory();
        const value_type* p = m_pathname.c_str() + root_dir.pos;
        return path(p, p + root_dir.size);
    }

    path relative_path() const
    {
        path_detail::substring root_dir = find_relative_path();
        const value_type* p = m_pathname.c_str() + root_dir.pos;
        return path(p, p + root_dir.size);
    }

    path parent_path() const { return path(m_pathname.c_str(), m_pathname.c_str() + find_parent_path_size()); }

    BOOST_FORCEINLINE path filename() const { return BOOST_FILESYSTEM_VERSIONED_SYM(filename)(); }   // returns 0 or 1 element path
    BOOST_FORCEINLINE path stem() const { return BOOST_FILESYSTEM_VERSIONED_SYM(stem)(); }           // returns 0 or 1 element path
    BOOST_FORCEINLINE path extension() const { return BOOST_FILESYSTEM_VERSIONED_SYM(extension)(); } // returns 0 or 1 element path

    //  -----  query  -----

    bool empty() const BOOST_NOEXCEPT { return m_pathname.empty(); }
    bool filename_is_dot() const;
    bool filename_is_dot_dot() const;
    bool has_root_path() const { return find_root_path_size() > 0; }
    bool has_root_name() const { return find_root_name_size() > 0; }
    bool has_root_directory() const { return find_root_directory().size > 0; }
    bool has_relative_path() const { return find_relative_path().size > 0; }
    bool has_parent_path() const { return find_parent_path_size() > 0; }
    BOOST_FORCEINLINE bool has_filename() const { return BOOST_FILESYSTEM_VERSIONED_SYM(has_filename)(); }
    bool has_stem() const { return !stem().empty(); }
    bool has_extension() const { return !extension().empty(); }
    bool is_relative() const { return !is_absolute(); }
    bool is_absolute() const
    {
        // Windows CE has no root name (aka drive letters)
#if defined(BOOST_WINDOWS_API) && !defined(UNDER_CE)
        return has_root_name() && has_root_directory();
#else
        return has_root_directory();
#endif
    }

    //  -----  lexical operations  -----

    BOOST_FILESYSTEM_DECL path lexically_normal() const;
    BOOST_FILESYSTEM_DECL path lexically_relative(path const& base) const;
    path lexically_proximate(path const& base) const
    {
        path tmp(lexically_relative(base));
        return tmp.empty() ? *this : tmp;
    }

    //  -----  iterators  -----

    class iterator;
    friend class iterator;
    typedef iterator const_iterator;
    class reverse_iterator;
    typedef reverse_iterator const_reverse_iterator;

    BOOST_FILESYSTEM_DECL iterator begin() const;
    BOOST_FILESYSTEM_DECL iterator end() const;
    reverse_iterator rbegin() const;
    reverse_iterator rend() const;

    //  -----  static member functions  -----

    static BOOST_FILESYSTEM_DECL std::locale imbue(std::locale const& loc);
    static BOOST_FILESYSTEM_DECL codecvt_type const& codecvt();

    //  -----  deprecated functions  -----

#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
    //  recently deprecated functions supplied by default
    path& normalize()
    {
        path tmp(lexically_normal());
        m_pathname.swap(tmp.m_pathname);
        return *this;
    }
    path& remove_leaf() { return remove_filename(); }
    path leaf() const { return filename(); }
    path branch_path() const { return parent_path(); }
    path generic() const { return generic_path(); }
    bool has_leaf() const { return !m_pathname.empty(); }
    bool has_branch_path() const { return !parent_path().empty(); }
    bool is_complete() const { return is_absolute(); }
#endif

#if defined(BOOST_FILESYSTEM_DEPRECATED)
    //  deprecated functions with enough signature or semantic changes that they are
    //  not supplied by default
    std::string file_string() const { return string(); }
    std::string directory_string() const { return string(); }
    std::string native_file_string() const { return string(); }
    std::string native_directory_string() const { return string(); }
    string_type external_file_string() const { return native(); }
    string_type external_directory_string() const { return native(); }
#endif

    //--------------------------------------------------------------------------------------//
    //                            class path private members                                //
    //--------------------------------------------------------------------------------------//
private:
    bool has_filename_v3() const { return !m_pathname.empty(); }
    BOOST_FILESYSTEM_DECL bool has_filename_v4() const;
    BOOST_FILESYSTEM_DECL path filename_v3() const;
    BOOST_FILESYSTEM_DECL path filename_v4() const;
    BOOST_FILESYSTEM_DECL path stem_v3() const;
    BOOST_FILESYSTEM_DECL path stem_v4() const;
    BOOST_FILESYSTEM_DECL path extension_v3() const;
    BOOST_FILESYSTEM_DECL path extension_v4() const;

    //  Returns: If separator is to be appended, m_pathname.size() before append. Otherwise 0.
    //  Note: An append is never performed if size()==0, so a returned 0 is unambiguous.
    BOOST_FILESYSTEM_DECL string_type::size_type append_separator_if_needed();
    BOOST_FILESYSTEM_DECL void erase_redundant_separator(string_type::size_type sep_pos);

    BOOST_FILESYSTEM_DECL string_type::size_type find_root_name_size() const;
    BOOST_FILESYSTEM_DECL string_type::size_type find_root_path_size() const;
    BOOST_FILESYSTEM_DECL path_detail::substring find_root_directory() const;
    BOOST_FILESYSTEM_DECL path_detail::substring find_relative_path() const;
    BOOST_FILESYSTEM_DECL string_type::size_type find_parent_path_size() const;

private:
    /*
     * m_pathname has the type, encoding, and format required by the native
     * operating system. Thus for POSIX and Windows there is no conversion for
     * passing m_pathname.c_str() to the O/S API or when obtaining a path from the
     * O/S API. POSIX encoding is unspecified other than for dot and slash
     * characters; POSIX just treats paths as a sequence of bytes. Windows
     * encoding is UCS-2 or UTF-16 depending on the version.
     */
    string_type m_pathname;     // Windows: as input; backslashes NOT converted to slashes,
                                // slashes NOT converted to backslashes
};

namespace detail {
BOOST_FILESYSTEM_DECL int lex_compare(path::iterator first1, path::iterator last1, path::iterator first2, path::iterator last2);
BOOST_FILESYSTEM_DECL path const& dot_path();
BOOST_FILESYSTEM_DECL path const& dot_dot_path();
} // namespace detail

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
typedef path wpath;
#endif

//------------------------------------------------------------------------------------//
//                             class path::iterator                                   //
//------------------------------------------------------------------------------------//

class path::iterator :
    public boost::iterator_facade<
        path::iterator,
        const path,
        boost::bidirectional_traversal_tag
    >
{
private:
    friend class boost::iterator_core_access;
    friend class boost::filesystem::path;
    friend class boost::filesystem::path::reverse_iterator;

    path const& dereference() const { return m_element; }

    bool equal(iterator const& rhs) const
    {
        return m_path_ptr == rhs.m_path_ptr && m_pos == rhs.m_pos;
    }

    BOOST_FILESYSTEM_DECL void increment();
    BOOST_FILESYSTEM_DECL void decrement();

private:
    // current element
    path m_element;
    // path being iterated over
    const path* m_path_ptr;
    // position of m_element in m_path_ptr->m_pathname.
    // if m_element is implicit dot, m_pos is the
    // position of the last separator in the path.
    // end() iterator is indicated by
    // m_pos == m_path_ptr->m_pathname.size()
    string_type::size_type m_pos;
};

//------------------------------------------------------------------------------------//
//                         class path::reverse_iterator                               //
//------------------------------------------------------------------------------------//

class path::reverse_iterator :
    public boost::iterator_facade<
        path::reverse_iterator,
        const path,
        boost::bidirectional_traversal_tag
    >
{
public:
    explicit reverse_iterator(iterator itr) :
        m_itr(itr)
    {
        if (itr != itr.m_path_ptr->begin())
            m_element = *--itr;
    }

private:
    friend class boost::iterator_core_access;
    friend class boost::filesystem::path;

    path const& dereference() const { return m_element; }
    bool equal(reverse_iterator const& rhs) const { return m_itr == rhs.m_itr; }

    void increment()
    {
        --m_itr;
        if (m_itr != m_itr.m_path_ptr->begin())
        {
            iterator tmp = m_itr;
            m_element = *--tmp;
        }
    }

    void decrement()
    {
        m_element = *m_itr;
        ++m_itr;
    }

private:
    iterator m_itr;
    path m_element;
};

//------------------------------------------------------------------------------------//
//                                                                                    //
//                              non-member functions                                  //
//                                                                                    //
//------------------------------------------------------------------------------------//

//  std::lexicographical_compare would infinitely recurse because path iterators
//  yield paths, so provide a path aware version
inline bool lexicographical_compare(path::iterator first1, path::iterator last1, path::iterator first2, path::iterator last2)
{
    return detail::lex_compare(first1, last1, first2, last2) < 0;
}

inline bool operator==(path const& lhs, path const& rhs)
{
    return lhs.compare(rhs) == 0;
}

inline bool operator==(path const& lhs, path::string_type const& rhs)
{
    return lhs.compare(rhs) == 0;
}

inline bool operator==(path::string_type const& lhs, path const& rhs)
{
    return rhs.compare(lhs) == 0;
}

inline bool operator==(path const& lhs, const path::value_type* rhs)
{
    return lhs.compare(rhs) == 0;
}

inline bool operator==(const path::value_type* lhs, path const& rhs)
{
    return rhs.compare(lhs) == 0;
}

inline bool operator!=(path const& lhs, path const& rhs)
{
    return lhs.compare(rhs) != 0;
}

inline bool operator!=(path const& lhs, path::string_type const& rhs)
{
    return lhs.compare(rhs) != 0;
}

inline bool operator!=(path::string_type const& lhs, path const& rhs)
{
    return rhs.compare(lhs) != 0;
}

inline bool operator!=(path const& lhs, const path::value_type* rhs)
{
    return lhs.compare(rhs) != 0;
}

inline bool operator!=(const path::value_type* lhs, path const& rhs)
{
    return rhs.compare(lhs) != 0;
}

// TODO: why do == and != have additional overloads, but the others don't?

inline bool operator<(path const& lhs, path const& rhs)
{
    return lhs.compare(rhs) < 0;
}
inline bool operator<=(path const& lhs, path const& rhs)
{
    return !(rhs < lhs);
}
inline bool operator>(path const& lhs, path const& rhs)
{
    return rhs < lhs;
}
inline bool operator>=(path const& lhs, path const& rhs)
{
    return !(lhs < rhs);
}

inline std::size_t hash_value(path const& x) BOOST_NOEXCEPT
{
#ifdef BOOST_WINDOWS_API
    std::size_t seed = 0;
    for (const path::value_type* it = x.c_str(); *it; ++it)
        hash_combine(seed, *it == L'/' ? L'\\' : *it);
    return seed;
#else // BOOST_POSIX_API
    return hash_range(x.native().begin(), x.native().end());
#endif
}

inline void swap(path& lhs, path& rhs) BOOST_NOEXCEPT
{
    lhs.swap(rhs);
}

inline path operator/(path const& lhs, path const& rhs)
{
    path p = lhs;
    p /= rhs;
    return p;
}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
inline path operator/(path&& lhs, path const& rhs)
{
    lhs /= rhs;
    return std::move(lhs);
}
#endif

//  inserters and extractors
//    use boost::io::quoted() to handle spaces in paths
//    use '&' as escape character to ease use for Windows paths

template< class Char, class Traits >
inline std::basic_ostream< Char, Traits >&
operator<<(std::basic_ostream< Char, Traits >& os, path const& p)
{
    return os << boost::io::quoted(p.template string< std::basic_string< Char > >(), static_cast< Char >('&'));
}

template< class Char, class Traits >
inline std::basic_istream< Char, Traits >&
operator>>(std::basic_istream< Char, Traits >& is, path& p)
{
    std::basic_string< Char > str;
    is >> boost::io::quoted(str, static_cast< Char >('&'));
    p = str;
    return is;
}

//  name_checks

//  These functions are holdovers from version 1. It isn't clear they have much
//  usefulness, or how to generalize them for later versions.

BOOST_FILESYSTEM_DECL bool portable_posix_name(std::string const& name);
BOOST_FILESYSTEM_DECL bool windows_name(std::string const& name);
BOOST_FILESYSTEM_DECL bool portable_name(std::string const& name);
BOOST_FILESYSTEM_DECL bool portable_directory_name(std::string const& name);
BOOST_FILESYSTEM_DECL bool portable_file_name(std::string const& name);
BOOST_FILESYSTEM_DECL bool native(std::string const& name);

namespace detail {

//  For POSIX, is_directory_separator() and is_element_separator() are identical since
//  a forward slash is the only valid directory separator and also the only valid
//  element separator. For Windows, forward slash and back slash are the possible
//  directory separators, but colon (example: "c:foo") is also an element separator.
inline bool is_directory_separator(path::value_type c) BOOST_NOEXCEPT
{
    return c == path::separator
#ifdef BOOST_WINDOWS_API
        || c == path::preferred_separator
#endif
        ;
}

inline bool is_element_separator(path::value_type c) BOOST_NOEXCEPT
{
    return c == path::separator
#ifdef BOOST_WINDOWS_API
        || c == path::preferred_separator || c == L':'
#endif
        ;
}

} // namespace detail

//------------------------------------------------------------------------------------//
//                  class path miscellaneous function implementations                 //
//------------------------------------------------------------------------------------//

inline path::reverse_iterator path::rbegin() const
{
    return reverse_iterator(end());
}
inline path::reverse_iterator path::rend() const
{
    return reverse_iterator(begin());
}

inline bool path::filename_is_dot() const
{
    // implicit dot is tricky, so actually call filename(); see path::filename() example
    // in reference.html
    path p(filename());
    return p.size() == 1 && *p.c_str() == dot;
}

inline bool path::filename_is_dot_dot() const
{
    return size() >= 2 && m_pathname[size() - 1] == dot && m_pathname[size() - 2] == dot && (m_pathname.size() == 2 || detail::is_element_separator(m_pathname[size() - 3]));
    // use detail::is_element_separator() rather than detail::is_directory_separator
    // to deal with "c:.." edge case on Windows when ':' acts as a separator
}

//--------------------------------------------------------------------------------------//
//                     class path member template implementation                        //
//--------------------------------------------------------------------------------------//

template< class InputIterator >
path& path::append(InputIterator begin, InputIterator end)
{
    if (begin == end)
        return *this;
    string_type::size_type sep_pos = append_separator_if_needed();
    std::basic_string< typename std::iterator_traits< InputIterator >::value_type > seq(begin, end);
    path_traits::convert(seq.c_str(), seq.c_str() + seq.size(), m_pathname);
    if (sep_pos)
        erase_redundant_separator(sep_pos);
    return *this;
}

template< class InputIterator >
path& path::append(InputIterator begin, InputIterator end, codecvt_type const& cvt)
{
    if (begin == end)
        return *this;
    string_type::size_type sep_pos = append_separator_if_needed();
    std::basic_string< typename std::iterator_traits< InputIterator >::value_type > seq(begin, end);
    path_traits::convert(seq.c_str(), seq.c_str() + seq.size(), m_pathname, cvt);
    if (sep_pos)
        erase_redundant_separator(sep_pos);
    return *this;
}

template< class Source >
path& path::append(Source const& source)
{
    if (path_traits::empty(source))
        return *this;
    string_type::size_type sep_pos = append_separator_if_needed();
    path_traits::dispatch(source, m_pathname);
    if (sep_pos)
        erase_redundant_separator(sep_pos);
    return *this;
}

template< class Source >
path& path::append(Source const& source, codecvt_type const& cvt)
{
    if (path_traits::empty(source))
        return *this;
    string_type::size_type sep_pos = append_separator_if_needed();
    path_traits::dispatch(source, m_pathname, cvt);
    if (sep_pos)
        erase_redundant_separator(sep_pos);
    return *this;
}

//--------------------------------------------------------------------------------------//
//                     class path member template specializations                       //
//--------------------------------------------------------------------------------------//

template<>
inline std::string path::string< std::string >() const
{
    return string();
}

template<>
inline std::wstring path::string< std::wstring >() const
{
    return wstring();
}

template<>
inline std::string path::string< std::string >(const codecvt_type& cvt) const
{
    return string(cvt);
}

template<>
inline std::wstring path::string< std::wstring >(const codecvt_type& cvt) const
{
    return wstring(cvt);
}

template<>
inline std::string path::generic_string< std::string >() const
{
    return generic_string();
}

template<>
inline std::wstring path::generic_string< std::wstring >() const
{
    return generic_wstring();
}

template<>
inline std::string path::generic_string< std::string >(codecvt_type const& cvt) const
{
    return generic_string(cvt);
}

template<>
inline std::wstring path::generic_string< std::wstring >(codecvt_type const& cvt) const
{
    return generic_wstring(cvt);
}

//--------------------------------------------------------------------------------------//
//                     path_traits convert function implementations                     //
//                        requiring path::codecvt() be visable                          //
//--------------------------------------------------------------------------------------//

namespace path_traits { //  without codecvt

inline void convert(const char* from,
                    const char* from_end, // 0 for null terminated MBCS
                    std::wstring& to)
{
    convert(from, from_end, to, path::codecvt());
}

inline void convert(const wchar_t* from,
                    const wchar_t* from_end, // 0 for null terminated MBCS
                    std::string& to)
{
    convert(from, from_end, to, path::codecvt());
}

inline void convert(const char* from, std::wstring& to)
{
    BOOST_ASSERT(!!from);
    convert(from, 0, to, path::codecvt());
}

inline void convert(const wchar_t* from, std::string& to)
{
    BOOST_ASSERT(!!from);
    convert(from, 0, to, path::codecvt());
}

} // namespace path_traits
} // namespace filesystem
} // namespace boost

//----------------------------------------------------------------------------//

#include <boost/filesystem/detail/footer.hpp>

#endif // BOOST_FILESYSTEM_PATH_HPP
