//  boost/filesystem/directory.hpp  ---------------------------------------------------//

//  Copyright Beman Dawes 2002-2009
//  Copyright Jan Langer 2002
//  Copyright Dietmar Kuehl 2001
//  Copyright Vladimir Prus 2002
//  Copyright Andrey Semashev 2019

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#ifndef BOOST_FILESYSTEM_DIRECTORY_HPP
#define BOOST_FILESYSTEM_DIRECTORY_HPP

#include <boost/filesystem/config.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/file_status.hpp>

#include <string>
#include <vector>
#include <utility> // std::move

#include <boost/assert.hpp>
#include <boost/core/scoped_enum.hpp>
#include <boost/detail/bitmask.hpp>
#include <boost/system/error_code.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_categories.hpp>

#include <boost/filesystem/detail/header.hpp> // must be the last #include

//--------------------------------------------------------------------------------------//

namespace boost {
namespace filesystem {

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                 directory_entry                                      //
//                                                                                      //
//--------------------------------------------------------------------------------------//

//  GCC has a problem with a member function named path within a namespace or
//  sub-namespace that also has a class named path. The workaround is to always
//  fully qualify the name path when it refers to the class name.

class directory_entry
{
public:
    typedef boost::filesystem::path::value_type value_type; // enables class path ctor taking directory_entry

    directory_entry() BOOST_NOEXCEPT {}

    explicit directory_entry(boost::filesystem::path const& p) :
        m_path(p), m_status(file_status()), m_symlink_status(file_status())
    {
    }

    directory_entry(boost::filesystem::path const& p, file_status st, file_status symlink_st = file_status()) :
        m_path(p), m_status(st), m_symlink_status(symlink_st)
    {
    }

    directory_entry(directory_entry const& rhs) :
        m_path(rhs.m_path), m_status(rhs.m_status), m_symlink_status(rhs.m_symlink_status)
    {
    }

    directory_entry& operator=(directory_entry const& rhs)
    {
        m_path = rhs.m_path;
        m_status = rhs.m_status;
        m_symlink_status = rhs.m_symlink_status;
        return *this;
    }

    //  As of October 2015 the interaction between noexcept and =default is so troublesome
    //  for VC++, GCC, and probably other compilers, that =default is not used with noexcept
    //  functions. GCC is not even consistent for the same release on different platforms.

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    directory_entry(directory_entry&& rhs) BOOST_NOEXCEPT :
        m_path(std::move(rhs.m_path)),
        m_status(std::move(rhs.m_status)),
        m_symlink_status(std::move(rhs.m_symlink_status))
    {
    }

    directory_entry& operator=(directory_entry&& rhs) BOOST_NOEXCEPT
    {
        m_path = std::move(rhs.m_path);
        m_status = std::move(rhs.m_status);
        m_symlink_status = std::move(rhs.m_symlink_status);
        return *this;
    }
#endif

    void assign(boost::filesystem::path const& p, file_status st = file_status(), file_status symlink_st = file_status())
    {
        m_path = p;
        m_status = st;
        m_symlink_status = symlink_st;
    }

    void replace_filename(boost::filesystem::path const& p, file_status st = file_status(), file_status symlink_st = file_status())
    {
        m_path.remove_filename();
        m_path /= p;
        m_status = st;
        m_symlink_status = symlink_st;
    }

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
    void replace_leaf(boost::filesystem::path const& p, file_status st, file_status symlink_st)
    {
        replace_filename(p, st, symlink_st);
    }
#endif

    boost::filesystem::path const& path() const BOOST_NOEXCEPT
    {
        return m_path;
    }
    operator boost::filesystem::path const&() const BOOST_NOEXCEPT { return m_path; }
    file_status status() const { return get_status(); }
    file_status status(system::error_code& ec) const BOOST_NOEXCEPT { return get_status(&ec); }
    file_status symlink_status() const { return get_symlink_status(); }
    file_status symlink_status(system::error_code& ec) const BOOST_NOEXCEPT { return get_symlink_status(&ec); }

    bool operator==(directory_entry const& rhs) const BOOST_NOEXCEPT { return m_path == rhs.m_path; }
    bool operator!=(directory_entry const& rhs) const BOOST_NOEXCEPT { return m_path != rhs.m_path; }
    bool operator<(directory_entry const& rhs) const BOOST_NOEXCEPT { return m_path < rhs.m_path; }
    bool operator<=(directory_entry const& rhs) const BOOST_NOEXCEPT { return m_path <= rhs.m_path; }
    bool operator>(directory_entry const& rhs) const BOOST_NOEXCEPT { return m_path > rhs.m_path; }
    bool operator>=(directory_entry const& rhs) const BOOST_NOEXCEPT { return m_path >= rhs.m_path; }

private:
    BOOST_FILESYSTEM_DECL file_status get_status(system::error_code* ec = 0) const;
    BOOST_FILESYSTEM_DECL file_status get_symlink_status(system::error_code* ec = 0) const;

private:
    boost::filesystem::path m_path;
    mutable file_status m_status;         // stat()-like
    mutable file_status m_symlink_status; // lstat()-like
};                                        // directory_entry

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                            directory_entry overloads                                 //
//                                                                                      //
//--------------------------------------------------------------------------------------//

//  Without these functions, calling (for example) 'is_directory' with a 'directory_entry' results in:
//  - a conversion to 'path' using 'operator boost::filesystem::path const&()',
//  - then a call to 'is_directory(path const& p)' which recomputes the status with 'detail::status(p)'.
//
//  These functions avoid a costly recomputation of the status if one calls 'is_directory(e)' instead of 'is_directory(e.status())'

inline file_status status(directory_entry const& e)
{
    return e.status();
}
inline file_status status(directory_entry const& e, system::error_code& ec) BOOST_NOEXCEPT
{
    return e.status(ec);
}
inline bool type_present(directory_entry const& e)
{
    return filesystem::type_present(e.status());
}
inline bool type_present(directory_entry const& e, system::error_code& ec) BOOST_NOEXCEPT
{
    return filesystem::type_present(e.status(ec));
}
inline bool status_known(directory_entry const& e)
{
    return filesystem::status_known(e.status());
}
inline bool status_known(directory_entry const& e, system::error_code& ec) BOOST_NOEXCEPT
{
    return filesystem::status_known(e.status(ec));
}
inline bool exists(directory_entry const& e)
{
    return filesystem::exists(e.status());
}
inline bool exists(directory_entry const& e, system::error_code& ec) BOOST_NOEXCEPT
{
    return filesystem::exists(e.status(ec));
}
inline bool is_regular_file(directory_entry const& e)
{
    return filesystem::is_regular_file(e.status());
}
inline bool is_regular_file(directory_entry const& e, system::error_code& ec) BOOST_NOEXCEPT
{
    return filesystem::is_regular_file(e.status(ec));
}
inline bool is_directory(directory_entry const& e)
{
    return filesystem::is_directory(e.status());
}
inline bool is_directory(directory_entry const& e, system::error_code& ec) BOOST_NOEXCEPT
{
    return filesystem::is_directory(e.status(ec));
}
inline bool is_symlink(directory_entry const& e)
{
    return filesystem::is_symlink(e.symlink_status());
}
inline bool is_symlink(directory_entry const& e, system::error_code& ec) BOOST_NOEXCEPT
{
    return filesystem::is_symlink(e.symlink_status(ec));
}
inline bool is_other(directory_entry const& e)
{
    return filesystem::is_other(e.status());
}
inline bool is_other(directory_entry const& e, system::error_code& ec) BOOST_NOEXCEPT
{
    return filesystem::is_other(e.status(ec));
}
#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
inline bool is_regular(directory_entry const& e)
{
    return filesystem::is_regular(e.status());
}
#endif

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                            directory_iterator helpers                                //
//                                                                                      //
//--------------------------------------------------------------------------------------//

BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(directory_options, unsigned int)
{
    none = 0u,
    skip_permission_denied = 1u,        // if a directory cannot be opened because of insufficient permissions, pretend that the directory is empty
    follow_directory_symlink = 1u << 1, // recursive_directory_iterator: follow directory symlinks
    skip_dangling_symlinks = 1u << 2,   // non-standard extension for recursive_directory_iterator: don't follow dangling directory symlinks,
    pop_on_error = 1u << 3,             // non-standard extension for recursive_directory_iterator: instead of producing an end iterator on errors,
                                        // repeatedly invoke pop() until it succeeds or the iterator becomes equal to end iterator
    _detail_no_push = 1u << 4           // internal use only
}
BOOST_SCOPED_ENUM_DECLARE_END(directory_options)

BOOST_BITMASK(BOOST_SCOPED_ENUM_NATIVE(directory_options))

class directory_iterator;

namespace detail {

BOOST_FILESYSTEM_DECL
system::error_code dir_itr_close( // never throws()
    void*& handle
#if defined(BOOST_POSIX_API)
    , void*& buffer
#endif
    ) BOOST_NOEXCEPT;

struct dir_itr_imp :
    public boost::intrusive_ref_counter< dir_itr_imp >
{
    directory_entry dir_entry;
    void* handle;

#if defined(BOOST_POSIX_API)
    void* buffer; // see dir_itr_increment implementation
#endif

    dir_itr_imp() BOOST_NOEXCEPT :
        handle(0)
#if defined(BOOST_POSIX_API)
        , buffer(0)
#endif
    {
    }

    ~dir_itr_imp() BOOST_NOEXCEPT
    {
        dir_itr_close(handle
#if defined(BOOST_POSIX_API)
            , buffer
#endif
        );
    }
};

BOOST_FILESYSTEM_DECL void directory_iterator_construct(directory_iterator& it, path const& p, unsigned int opts, system::error_code* ec);
BOOST_FILESYSTEM_DECL void directory_iterator_increment(directory_iterator& it, system::error_code* ec);

} // namespace detail

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                directory_iterator                                    //
//                                                                                      //
//--------------------------------------------------------------------------------------//

class directory_iterator :
    public boost::iterator_facade<
        directory_iterator,
        directory_entry,
        boost::single_pass_traversal_tag
    >
{
    friend class boost::iterator_core_access;

    friend BOOST_FILESYSTEM_DECL void detail::directory_iterator_construct(directory_iterator& it, path const& p, unsigned int opts, system::error_code* ec);
    friend BOOST_FILESYSTEM_DECL void detail::directory_iterator_increment(directory_iterator& it, system::error_code* ec);

public:
    directory_iterator() BOOST_NOEXCEPT {} // creates the "end" iterator

    // iterator_facade derived classes don't seem to like implementations in
    // separate translation unit dll's, so forward to detail functions
    explicit directory_iterator(path const& p, BOOST_SCOPED_ENUM_NATIVE(directory_options) opts = directory_options::none)
    {
        detail::directory_iterator_construct(*this, p, static_cast< unsigned int >(opts), 0);
    }

    directory_iterator(path const& p, system::error_code& ec) BOOST_NOEXCEPT
    {
        detail::directory_iterator_construct(*this, p, static_cast< unsigned int >(directory_options::none), &ec);
    }

    directory_iterator(path const& p, BOOST_SCOPED_ENUM_NATIVE(directory_options) opts, system::error_code& ec) BOOST_NOEXCEPT
    {
        detail::directory_iterator_construct(*this, p, static_cast< unsigned int >(opts), &ec);
    }

    BOOST_DEFAULTED_FUNCTION(directory_iterator(directory_iterator const& that), : m_imp(that.m_imp) {})
    BOOST_DEFAULTED_FUNCTION(directory_iterator& operator=(directory_iterator const& that), { m_imp = that.m_imp; return *this; })

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    directory_iterator(directory_iterator&& that) BOOST_NOEXCEPT :
        m_imp(std::move(that.m_imp))
    {
    }

    directory_iterator& operator=(directory_iterator&& that) BOOST_NOEXCEPT
    {
        m_imp = std::move(that.m_imp);
        return *this;
    }
#endif // !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

    directory_iterator& increment(system::error_code& ec) BOOST_NOEXCEPT
    {
        detail::directory_iterator_increment(*this, &ec);
        return *this;
    }

private:
    boost::iterator_facade<
        directory_iterator,
        directory_entry,
        boost::single_pass_traversal_tag
    >::reference dereference() const
    {
        BOOST_ASSERT_MSG(!is_end(), "attempt to dereference end directory iterator");
        return m_imp->dir_entry;
    }

    void increment() { detail::directory_iterator_increment(*this, 0); }

    bool equal(directory_iterator const& rhs) const BOOST_NOEXCEPT
    {
        return m_imp == rhs.m_imp || (is_end() && rhs.is_end());
    }

    bool is_end() const BOOST_NOEXCEPT
    {
        // Note: The check for handle is needed because the iterator can be copied and the copy
        // can be incremented to end while the original iterator still refers to the same dir_itr_imp.
        return !m_imp || !m_imp->handle;
    }

private:
    // intrusive_ptr provides the shallow-copy semantics required for single pass iterators
    // (i.e. InputIterators). The end iterator is indicated by is_end().
    boost::intrusive_ptr< detail::dir_itr_imp > m_imp;
};

//  enable directory_iterator C++11 range-based for statement use  --------------------//

// begin() and end() are only used by a range-based for statement in the context of
// auto - thus the top-level const is stripped - so returning const is harmless and
// emphasizes begin() is just a pass through.
inline directory_iterator const& begin(directory_iterator const& iter) BOOST_NOEXCEPT
{
    return iter;
}

inline directory_iterator end(directory_iterator const&) BOOST_NOEXCEPT
{
    return directory_iterator();
}

// enable C++14 generic accessors for range const iterators
inline directory_iterator const& cbegin(directory_iterator const& iter) BOOST_NOEXCEPT
{
    return iter;
}

inline directory_iterator cend(directory_iterator const&) BOOST_NOEXCEPT
{
    return directory_iterator();
}

//  enable directory_iterator BOOST_FOREACH  -----------------------------------------//

inline directory_iterator& range_begin(directory_iterator& iter) BOOST_NOEXCEPT
{
    return iter;
}

inline directory_iterator range_begin(directory_iterator const& iter) BOOST_NOEXCEPT
{
    return iter;
}

inline directory_iterator range_end(directory_iterator&) BOOST_NOEXCEPT
{
    return directory_iterator();
}

inline directory_iterator range_end(directory_iterator const&) BOOST_NOEXCEPT
{
    return directory_iterator();
}

} // namespace filesystem

//  namespace boost template specializations
template< typename C, typename Enabler >
struct range_mutable_iterator;

template<>
struct range_mutable_iterator< boost::filesystem::directory_iterator, void >
{
    typedef boost::filesystem::directory_iterator type;
};

template< typename C, typename Enabler >
struct range_const_iterator;

template<>
struct range_const_iterator< boost::filesystem::directory_iterator, void >
{
    typedef boost::filesystem::directory_iterator type;
};

namespace filesystem {

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                      recursive_directory_iterator helpers                            //
//                                                                                      //
//--------------------------------------------------------------------------------------//

#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
// Deprecated enum, use directory_options instead
BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(symlink_option, unsigned int)
{
    none = static_cast< unsigned int >(directory_options::none),
    no_recurse = none,                                                                  // don't follow directory symlinks (default behavior)
    recurse = static_cast< unsigned int >(directory_options::follow_directory_symlink), // follow directory symlinks
    _detail_no_push = static_cast< unsigned int >(directory_options::_detail_no_push)   // internal use only
}
BOOST_SCOPED_ENUM_DECLARE_END(symlink_option)

BOOST_BITMASK(BOOST_SCOPED_ENUM_NATIVE(symlink_option))
#endif // BOOST_FILESYSTEM_NO_DEPRECATED

class recursive_directory_iterator;

namespace detail {

struct recur_dir_itr_imp :
    public boost::intrusive_ref_counter< recur_dir_itr_imp >
{
    typedef directory_iterator element_type;
    std::vector< element_type > m_stack;
    // directory_options values, declared as unsigned int for ABI compatibility
    unsigned int m_options;

    explicit recur_dir_itr_imp(unsigned int opts) BOOST_NOEXCEPT : m_options(opts) {}
};

BOOST_FILESYSTEM_DECL void recursive_directory_iterator_construct(recursive_directory_iterator& it, path const& dir_path, unsigned int opts, system::error_code* ec);
BOOST_FILESYSTEM_DECL void recursive_directory_iterator_increment(recursive_directory_iterator& it, system::error_code* ec);
BOOST_FILESYSTEM_DECL void recursive_directory_iterator_pop(recursive_directory_iterator& it, system::error_code* ec);

} // namespace detail

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                           recursive_directory_iterator                               //
//                                                                                      //
//--------------------------------------------------------------------------------------//

class recursive_directory_iterator :
    public boost::iterator_facade<
        recursive_directory_iterator,
        directory_entry,
        boost::single_pass_traversal_tag
    >
{
    friend class boost::iterator_core_access;

    friend BOOST_FILESYSTEM_DECL void detail::recursive_directory_iterator_construct(recursive_directory_iterator& it, path const& dir_path, unsigned int opts, system::error_code* ec);
    friend BOOST_FILESYSTEM_DECL void detail::recursive_directory_iterator_increment(recursive_directory_iterator& it, system::error_code* ec);
    friend BOOST_FILESYSTEM_DECL void detail::recursive_directory_iterator_pop(recursive_directory_iterator& it, system::error_code* ec);

public:
    recursive_directory_iterator() BOOST_NOEXCEPT {} // creates the "end" iterator

    explicit recursive_directory_iterator(path const& dir_path)
    {
        detail::recursive_directory_iterator_construct(*this, dir_path, static_cast< unsigned int >(directory_options::none), 0);
    }

    recursive_directory_iterator(path const& dir_path, system::error_code& ec)
    {
        detail::recursive_directory_iterator_construct(*this, dir_path, static_cast< unsigned int >(directory_options::none), &ec);
    }

    recursive_directory_iterator(path const& dir_path, BOOST_SCOPED_ENUM_NATIVE(directory_options) opts)
    {
        detail::recursive_directory_iterator_construct(*this, dir_path, static_cast< unsigned int >(opts), 0);
    }

    recursive_directory_iterator(path const& dir_path, BOOST_SCOPED_ENUM_NATIVE(directory_options) opts, system::error_code& ec)
    {
        detail::recursive_directory_iterator_construct(*this, dir_path, static_cast< unsigned int >(opts), &ec);
    }

#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
    // Deprecated constructors
    recursive_directory_iterator(path const& dir_path, BOOST_SCOPED_ENUM_NATIVE(symlink_option) opts)
    {
        detail::recursive_directory_iterator_construct(*this, dir_path, static_cast< unsigned int >(opts), 0);
    }

    recursive_directory_iterator(path const& dir_path, BOOST_SCOPED_ENUM_NATIVE(symlink_option) opts, system::error_code& ec) BOOST_NOEXCEPT
    {
        detail::recursive_directory_iterator_construct(*this, dir_path, static_cast< unsigned int >(opts), &ec);
    }
#endif // BOOST_FILESYSTEM_NO_DEPRECATED

    BOOST_DEFAULTED_FUNCTION(recursive_directory_iterator(recursive_directory_iterator const& that), : m_imp(that.m_imp) {})
    BOOST_DEFAULTED_FUNCTION(recursive_directory_iterator& operator=(recursive_directory_iterator const& that), { m_imp = that.m_imp; return *this; })

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    recursive_directory_iterator(recursive_directory_iterator&& that) BOOST_NOEXCEPT :
        m_imp(std::move(that.m_imp))
    {
    }

    recursive_directory_iterator& operator=(recursive_directory_iterator&& that) BOOST_NOEXCEPT
    {
        m_imp = std::move(that.m_imp);
        return *this;
    }
#endif // !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

    recursive_directory_iterator& increment(system::error_code& ec) BOOST_NOEXCEPT
    {
        detail::recursive_directory_iterator_increment(*this, &ec);
        return *this;
    }

    int depth() const BOOST_NOEXCEPT
    {
        BOOST_ASSERT_MSG(!is_end(), "depth() on end recursive_directory_iterator");
        return static_cast< int >(m_imp->m_stack.size() - 1u);
    }

    bool recursion_pending() const BOOST_NOEXCEPT
    {
        BOOST_ASSERT_MSG(!is_end(), "recursion_pending() on end recursive_directory_iterator");
        return (m_imp->m_options & static_cast< unsigned int >(directory_options::_detail_no_push)) == 0u;
    }

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
    int level() const BOOST_NOEXCEPT
    {
        return depth();
    }
    bool no_push_pending() const BOOST_NOEXCEPT { return !recursion_pending(); }
    bool no_push_request() const BOOST_NOEXCEPT { return !recursion_pending(); }
#endif

    void pop()
    {
        detail::recursive_directory_iterator_pop(*this, 0);
    }

    void pop(system::error_code& ec) BOOST_NOEXCEPT
    {
        detail::recursive_directory_iterator_pop(*this, &ec);
    }

    void disable_recursion_pending(bool value = true) BOOST_NOEXCEPT
    {
        BOOST_ASSERT_MSG(!is_end(), "disable_recursion_pending() on end recursive_directory_iterator");
        if (value)
            m_imp->m_options |= static_cast< unsigned int >(directory_options::_detail_no_push);
        else
            m_imp->m_options &= ~static_cast< unsigned int >(directory_options::_detail_no_push);
    }

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
    void no_push(bool value = true) BOOST_NOEXCEPT
    {
        disable_recursion_pending(value);
    }
#endif

    file_status status() const
    {
        BOOST_ASSERT_MSG(!is_end(), "status() on end recursive_directory_iterator");
        return m_imp->m_stack.back()->status();
    }

    file_status symlink_status() const
    {
        BOOST_ASSERT_MSG(!is_end(), "symlink_status() on end recursive_directory_iterator");
        return m_imp->m_stack.back()->symlink_status();
    }

private:
    boost::iterator_facade<
        recursive_directory_iterator,
        directory_entry,
        boost::single_pass_traversal_tag
    >::reference dereference() const
    {
        BOOST_ASSERT_MSG(!is_end(), "dereference of end recursive_directory_iterator");
        return *m_imp->m_stack.back();
    }

    void increment() { detail::recursive_directory_iterator_increment(*this, 0); }

    bool equal(recursive_directory_iterator const& rhs) const BOOST_NOEXCEPT
    {
        return m_imp == rhs.m_imp || (is_end() && rhs.is_end());
    }

    bool is_end() const BOOST_NOEXCEPT
    {
        // Note: The check for m_stack.empty() is needed because the iterator can be copied and the copy
        // can be incremented to end while the original iterator still refers to the same recur_dir_itr_imp.
        return !m_imp || m_imp->m_stack.empty();
    }

private:
    // intrusive_ptr provides the shallow-copy semantics required for single pass iterators
    // (i.e. InputIterators). The end iterator is indicated by is_end().
    boost::intrusive_ptr< detail::recur_dir_itr_imp > m_imp;
};

#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
typedef recursive_directory_iterator wrecursive_directory_iterator;
#endif

//  enable recursive directory iterator C++11 range-base for statement use  ----------//

// begin() and end() are only used by a range-based for statement in the context of
// auto - thus the top-level const is stripped - so returning const is harmless and
// emphasizes begin() is just a pass through.
inline recursive_directory_iterator const& begin(recursive_directory_iterator const& iter) BOOST_NOEXCEPT
{
    return iter;
}

inline recursive_directory_iterator end(recursive_directory_iterator const&) BOOST_NOEXCEPT
{
    return recursive_directory_iterator();
}

// enable C++14 generic accessors for range const iterators
inline recursive_directory_iterator const& cbegin(recursive_directory_iterator const& iter) BOOST_NOEXCEPT
{
    return iter;
}

inline recursive_directory_iterator cend(recursive_directory_iterator const&) BOOST_NOEXCEPT
{
    return recursive_directory_iterator();
}

//  enable recursive directory iterator BOOST_FOREACH  -------------------------------//

inline recursive_directory_iterator& range_begin(recursive_directory_iterator& iter) BOOST_NOEXCEPT
{
    return iter;
}

inline recursive_directory_iterator range_begin(recursive_directory_iterator const& iter) BOOST_NOEXCEPT
{
    return iter;
}

inline recursive_directory_iterator range_end(recursive_directory_iterator&) BOOST_NOEXCEPT
{
    return recursive_directory_iterator();
}

inline recursive_directory_iterator range_end(recursive_directory_iterator const&) BOOST_NOEXCEPT
{
    return recursive_directory_iterator();
}

} // namespace filesystem

//  namespace boost template specializations
template<>
struct range_mutable_iterator< boost::filesystem::recursive_directory_iterator, void >
{
    typedef boost::filesystem::recursive_directory_iterator type;
};

template<>
struct range_const_iterator< boost::filesystem::recursive_directory_iterator, void >
{
    typedef boost::filesystem::recursive_directory_iterator type;
};

} // namespace boost

#include <boost/filesystem/detail/footer.hpp>

#endif // BOOST_FILESYSTEM_DIRECTORY_HPP
