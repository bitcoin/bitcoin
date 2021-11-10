// (C) Copyright Jorge Lodos 2008.
// (C) Copyright Jonathan Turkanis 2003.
// (C) Copyright Craig Henderson 2002.   'boost/memmap.hpp' from sandbox
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

#ifndef BOOST_IOSTREAMS_MAPPED_FILE_HPP_INCLUDED
#define BOOST_IOSTREAMS_MAPPED_FILE_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>                   // make sure size_t is in std.
#include <cstddef>                            // size_t.
#include <string>                             // pathnames.
#include <utility>                            // pair.
#include <boost/config.hpp>                   // BOOST_MSVC.
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/close.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/detail/config/auto_link.hpp>
#include <boost/iostreams/detail/config/dyn_link.hpp>
#include <boost/iostreams/detail/config/wide_streams.hpp>
#include <boost/iostreams/detail/ios.hpp>     // openmode, failure
#include <boost/iostreams/detail/path.hpp>
#include <boost/iostreams/operations_fwd.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_same.hpp>

// Must come last.
#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251)  // Missing DLL interface for shared_ptr
#endif
#include <boost/config/abi_prefix.hpp>

namespace boost { namespace iostreams {

//------------------Definition of mapped_file_base and mapped_file_params-----//

// Forward declarations
class mapped_file_source;
class mapped_file_sink;
class mapped_file;
namespace detail { class mapped_file_impl; }

class mapped_file_base {
public:
    enum mapmode {
        readonly = 1,
        readwrite = 2,
        priv = 4
    };
};

// Bitmask operations for mapped_file_base::mapmode
mapped_file_base::mapmode 
operator|(mapped_file_base::mapmode a, mapped_file_base::mapmode b);

mapped_file_base::mapmode 
operator&(mapped_file_base::mapmode a, mapped_file_base::mapmode b);

mapped_file_base::mapmode 
operator^(mapped_file_base::mapmode a, mapped_file_base::mapmode b);

mapped_file_base::mapmode 
operator~(mapped_file_base::mapmode a);

mapped_file_base::mapmode 
operator|=(mapped_file_base::mapmode& a, mapped_file_base::mapmode b);

mapped_file_base::mapmode 
operator&=(mapped_file_base::mapmode& a, mapped_file_base::mapmode b);

mapped_file_base::mapmode 
operator^=(mapped_file_base::mapmode& a, mapped_file_base::mapmode b);

//------------------Definition of mapped_file_params--------------------------//

namespace detail {

struct mapped_file_params_base {
    mapped_file_params_base()
        : flags(static_cast<mapped_file_base::mapmode>(0)), 
          mode(), offset(0), length(static_cast<std::size_t>(-1)), 
          new_file_size(0), hint(0)
        { }
private:
    friend class mapped_file_impl;
    void normalize();
public:
    mapped_file_base::mapmode   flags;
    BOOST_IOS::openmode         mode;  // Deprecated
    stream_offset               offset;
    std::size_t                 length;
    stream_offset               new_file_size;
    const char*                 hint;
};

} // End namespace detail.

// This template allows Boost.Filesystem paths to be specified when creating or
// reopening a memory mapped file, without creating a dependence on
// Boost.Filesystem. Possible values of Path include std::string,
// boost::filesystem::path, boost::filesystem::wpath, 
// and boost::iostreams::detail::path (used to store either a std::string or a
// std::wstring).
template<typename Path>
struct basic_mapped_file_params 
    : detail::mapped_file_params_base 
{
    typedef detail::mapped_file_params_base base_type;

    // For wide paths, instantiate basic_mapped_file_params 
    // with boost::filesystem::wpath
#ifndef BOOST_IOSTREAMS_NO_WIDE_STREAMS
    BOOST_STATIC_ASSERT((!is_same<Path, std::wstring>::value));
#endif

    // Default constructor
    basic_mapped_file_params() { }

    // Construction from a Path
    explicit basic_mapped_file_params(const Path& p) : path(p) { }

    // Construction from a path of a different type
    template<typename PathT>
    explicit basic_mapped_file_params(const PathT& p) : path(p) { }

    // Copy constructor
    basic_mapped_file_params(const basic_mapped_file_params& other)
        : base_type(other), path(other.path)
        { }

    // Templated copy constructor
    template<typename PathT>
    basic_mapped_file_params(const basic_mapped_file_params<PathT>& other)
        : base_type(other), path(other.path)
        { }

    typedef Path  path_type;
    Path          path;
};

typedef basic_mapped_file_params<std::string> mapped_file_params;

//------------------Definition of mapped_file_source--------------------------//

class BOOST_IOSTREAMS_DECL mapped_file_source : public mapped_file_base {
private:
    struct safe_bool_helper { int x; };
    typedef int safe_bool_helper::*                 safe_bool;
    typedef detail::mapped_file_impl                impl_type;
    typedef basic_mapped_file_params<detail::path>  param_type;
    friend class mapped_file;
    friend class detail::mapped_file_impl;
    friend struct boost::iostreams::operations<mapped_file_source>;
public:
    typedef char                                    char_type;
    struct category
        : public source_tag,
          public direct_tag,
          public closable_tag
        { };
    typedef std::size_t                             size_type;
    typedef const char*                             iterator;
    BOOST_STATIC_CONSTANT(size_type, max_length = static_cast<size_type>(-1));

    // Default constructor
    mapped_file_source();

    // Constructor taking a parameters object
    template<typename Path>
    explicit mapped_file_source(const basic_mapped_file_params<Path>& p);

    // Constructor taking a list of parameters
    template<typename Path>
    explicit mapped_file_source( const Path& path,
                                 size_type length = max_length,
                                 boost::intmax_t offset = 0 );

    // Copy Constructor
    mapped_file_source(const mapped_file_source& other);

    //--------------Stream interface------------------------------------------//

    template<typename Path>
    void open(const basic_mapped_file_params<Path>& p);

    template<typename Path>
    void open( const Path& path,
               size_type length = max_length,
               boost::intmax_t offset = 0 );

    bool is_open() const;
    void close();
    operator safe_bool() const;
    bool operator!() const;
    mapmode flags() const;

    //--------------Container interface---------------------------------------//

    size_type size() const;
    const char* data() const;
    iterator begin() const;
    iterator end() const;

    //--------------Query admissible offsets----------------------------------//

    // Returns the allocation granularity for virtual memory. Values passed
    // as offsets must be multiples of this value.
    static int alignment();

private:
    void init();
    void open_impl(const param_type& p);

    boost::shared_ptr<impl_type> pimpl_;
};

//------------------Definition of mapped_file---------------------------------//

class BOOST_IOSTREAMS_DECL mapped_file : public mapped_file_base {
private:
    typedef mapped_file_source                      delegate_type;
    typedef delegate_type::safe_bool                safe_bool;
    typedef basic_mapped_file_params<detail::path>  param_type;
    friend struct boost::iostreams::operations<mapped_file >;
    friend class mapped_file_sink;
public:
    typedef char                                    char_type;
    struct category
        : public seekable_device_tag,
          public direct_tag,
          public closable_tag
        { };
    typedef mapped_file_source::size_type           size_type;
    typedef char*                                   iterator;
    typedef const char*                             const_iterator;
    BOOST_STATIC_CONSTANT(size_type, max_length = delegate_type::max_length);

    // Default constructor
    mapped_file() { }

    // Construstor taking a parameters object
    template<typename Path>
    explicit mapped_file(const basic_mapped_file_params<Path>& p);

    // Constructor taking a list of parameters
    template<typename Path>
    mapped_file( const Path& path,
                 mapmode flags,
                 size_type length = max_length,
                 stream_offset offset = 0 );

    // Constructor taking a list of parameters, including a 
    // std::ios_base::openmode (deprecated)
    template<typename Path>
    explicit mapped_file( const Path& path,
                          BOOST_IOS::openmode mode =
                              BOOST_IOS::in | BOOST_IOS::out,
                          size_type length = max_length,
                          stream_offset offset = 0 );

    // Copy Constructor
    mapped_file(const mapped_file& other);

    //--------------Conversion to mapped_file_source (deprecated)-------------//

    operator mapped_file_source&() { return delegate_; }
    operator const mapped_file_source&() const { return delegate_; }

    //--------------Stream interface------------------------------------------//

    // open overload taking a parameters object
    template<typename Path>
    void open(const basic_mapped_file_params<Path>& p);

    // open overload taking a list of parameters
    template<typename Path>
    void open( const Path& path,
               mapmode mode,
               size_type length = max_length,
               stream_offset offset = 0 );

    // open overload taking a list of parameters, including a 
    // std::ios_base::openmode (deprecated)
    template<typename Path>
    void open( const Path& path,
               BOOST_IOS::openmode mode =
                   BOOST_IOS::in | BOOST_IOS::out,
               size_type length = max_length,
               stream_offset offset = 0 );

    bool is_open() const { return delegate_.is_open(); }
    void close() { delegate_.close(); }
    operator safe_bool() const { return delegate_; }
    bool operator!() const { return !delegate_; }
    mapmode flags() const { return delegate_.flags(); }

    //--------------Container interface---------------------------------------//

    size_type size() const { return delegate_.size(); }
    char* data() const;
    const char* const_data() const { return delegate_.data(); }
    iterator begin() const { return data(); }
    const_iterator const_begin() const { return const_data(); }
    iterator end() const;
    const_iterator const_end() const { return const_data() + size(); }

    //--------------Query admissible offsets----------------------------------//

    // Returns the allocation granularity for virtual memory. Values passed
    // as offsets must be multiples of this value.
    static int alignment() { return mapped_file_source::alignment(); }

    //--------------File access----------------------------------------------//

    void resize(stream_offset new_size);
private:
    delegate_type delegate_;
};

//------------------Definition of mapped_file_sink----------------------------//

class BOOST_IOSTREAMS_DECL mapped_file_sink : private mapped_file {
public:
    friend struct boost::iostreams::operations<mapped_file_sink>;
    using mapped_file::mapmode;
    using mapped_file::readonly;
    using mapped_file::readwrite;
    using mapped_file::priv;
    using mapped_file::char_type;
    struct category
        : public sink_tag,
          public direct_tag,
          public closable_tag
        { };
    using mapped_file::size_type;
    using mapped_file::iterator;
    using mapped_file::max_length;
    using mapped_file::is_open;
    using mapped_file::close;
    using mapped_file::operator safe_bool;
    using mapped_file::operator !;
    using mapped_file::flags;
    using mapped_file::size;
    using mapped_file::data;
    using mapped_file::begin;
    using mapped_file::end;
    using mapped_file::alignment;
    using mapped_file::resize;

    // Default constructor
    mapped_file_sink() { }

    // Constructor taking a parameters object
    template<typename Path>
    explicit mapped_file_sink(const basic_mapped_file_params<Path>& p);

    // Constructor taking a list of parameters
    template<typename Path>
    explicit mapped_file_sink( const Path& path,
                               size_type length = max_length,
                               boost::intmax_t offset = 0,
                               mapmode flags = readwrite );

    // Copy Constructor
    mapped_file_sink(const mapped_file_sink& other);

    // open overload taking a parameters object
    template<typename Path>
    void open(const basic_mapped_file_params<Path>& p);

    // open overload taking a list of parameters
    template<typename Path>
    void open( const Path& path,
               size_type length = max_length,
               boost::intmax_t offset = 0,
               mapmode flags = readwrite );
};

//------------------Implementation of mapped_file_source----------------------//

template<typename Path>
mapped_file_source::mapped_file_source(const basic_mapped_file_params<Path>& p)
{ init(); open(p); }

template<typename Path>
mapped_file_source::mapped_file_source( 
    const Path& path, size_type length, boost::intmax_t offset)
{ init(); open(path, length, offset); }

template<typename Path>
void mapped_file_source::open(const basic_mapped_file_params<Path>& p)
{
    param_type params(p);
    if (params.flags) {
        if (params.flags != mapped_file::readonly)
            boost::throw_exception(BOOST_IOSTREAMS_FAILURE("invalid flags"));
    } else {
        if (params.mode & BOOST_IOS::out)
            boost::throw_exception(BOOST_IOSTREAMS_FAILURE("invalid mode"));
        params.mode |= BOOST_IOS::in;
    }
    open_impl(params);
}

template<typename Path>
void mapped_file_source::open(
    const Path& path, size_type length, boost::intmax_t offset)
{
    param_type p(path);
    p.length = length;
    p.offset = offset;
    open(p);
}

//------------------Implementation of mapped_file-----------------------------//

template<typename Path>
mapped_file::mapped_file(const basic_mapped_file_params<Path>& p)
{ open(p); }

template<typename Path>
mapped_file::mapped_file( 
    const Path& path, mapmode flags, 
    size_type length, stream_offset offset )
{ open(path, flags, length, offset); }

template<typename Path>
mapped_file::mapped_file( 
    const Path& path, BOOST_IOS::openmode mode, 
    size_type length, stream_offset offset )
{ open(path, mode, length, offset); }

template<typename Path>
void mapped_file::open(const basic_mapped_file_params<Path>& p)
{ delegate_.open_impl(p); }

template<typename Path>
void mapped_file::open( 
    const Path& path, mapmode flags, 
    size_type length, stream_offset offset )
{
    param_type p(path);
    p.flags = flags;
    p.length = length;
    p.offset = offset;
    open(p);
}

template<typename Path>
void mapped_file::open( 
    const Path& path, BOOST_IOS::openmode mode, 
    size_type length, stream_offset offset )
{
    param_type p(path);
    p.mode = mode;
    p.length = length;
    p.offset = offset;
    open(p);
}

inline char* mapped_file::data() const 
{ return (flags() != readonly) ? const_cast<char*>(delegate_.data()) : 0; }

inline mapped_file::iterator mapped_file::end() const 
{ return (flags() != readonly) ? data() + size() : 0; }

//------------------Implementation of mapped_file_sink------------------------//

template<typename Path>
mapped_file_sink::mapped_file_sink(const basic_mapped_file_params<Path>& p)
{ open(p); }

template<typename Path>
mapped_file_sink::mapped_file_sink(
    const Path& path, size_type length,
    boost::intmax_t offset, mapmode flags )
{ open(path, length, offset, flags); }

template<typename Path>
void mapped_file_sink::open(const basic_mapped_file_params<Path>& p)
{
    param_type params(p);
    if (params.flags) {
        if (params.flags & mapped_file::readonly)
            boost::throw_exception(BOOST_IOSTREAMS_FAILURE("invalid flags"));
    } else {
        if (params.mode & BOOST_IOS::in)
            boost::throw_exception(BOOST_IOSTREAMS_FAILURE("invalid mode"));
        params.mode |= BOOST_IOS::out;
    }
    mapped_file::open(params);
}

template<typename Path>
void mapped_file_sink::open(
    const Path& path, size_type length,
    boost::intmax_t offset, mapmode flags )
{
    param_type p(path);
    p.flags = flags;
    p.length = length;
    p.offset = offset;
    open(p);
}

//------------------Specialization of direct_impl-----------------------------//

template<>
struct operations<mapped_file_source>
    : boost::iostreams::detail::close_impl<closable_tag>
{
    static std::pair<char*, char*>
    input_sequence(mapped_file_source& src)
    {
        return std::make_pair( const_cast<char*>(src.begin()),
                               const_cast<char*>(src.end()) );
    }
};

template<>
struct operations<mapped_file>
    : boost::iostreams::detail::close_impl<closable_tag>
{
    static std::pair<char*, char*>
    input_sequence(mapped_file& file)
    { 
        return std::make_pair(file.begin(), file.end()); 
    }
    static std::pair<char*, char*>
    output_sequence(mapped_file& file)
    { 
        return std::make_pair(file.begin(), file.end()); 
    }
};

template<>
struct operations<mapped_file_sink>
    : boost::iostreams::detail::close_impl<closable_tag>
{
    static std::pair<char*, char*>
    output_sequence(mapped_file_sink& sink)
    { 
        return std::make_pair(sink.begin(), sink.end()); 
    }
};
                    
//------------------Definition of mapmode operators---------------------------//

inline mapped_file::mapmode 
operator|(mapped_file::mapmode a, mapped_file::mapmode b)
{
    return static_cast<mapped_file::mapmode>
        (static_cast<int>(a) | static_cast<int>(b));
}

inline mapped_file::mapmode 
operator&(mapped_file::mapmode a, mapped_file::mapmode b)
{
    return static_cast<mapped_file::mapmode>
        (static_cast<int>(a) & static_cast<int>(b));
}

inline mapped_file::mapmode 
operator^(mapped_file::mapmode a, mapped_file::mapmode b)
{
    return static_cast<mapped_file::mapmode>
        (static_cast<int>(a) ^ static_cast<int>(b));
}

inline mapped_file::mapmode
operator~(mapped_file::mapmode a)
{
    return static_cast<mapped_file::mapmode>(~static_cast<int>(a));
}

inline mapped_file::mapmode 
operator|=(mapped_file::mapmode& a, mapped_file::mapmode b)
{
    return a = a | b;
}

inline mapped_file::mapmode 
operator&=(mapped_file::mapmode& a, mapped_file::mapmode b)
{
    return a = a & b;
}

inline mapped_file::mapmode 
operator^=(mapped_file::mapmode& a, mapped_file::mapmode b)
{
    return a = a ^ b;
}

} } // End namespaces iostreams, boost.

#include <boost/config/abi_suffix.hpp> // pops abi_suffix.hpp pragmas
#if defined(BOOST_MSVC)
# pragma warning(pop)  // pops #pragma warning(disable:4251)
#endif

#endif // #ifndef BOOST_IOSTREAMS_MAPPED_FILE_HPP_INCLUDED
