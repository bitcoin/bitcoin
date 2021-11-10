/*=============================================================================
    Copyright (c) 2003 Giovanni Bajo
    Copyright (c) 2003 Thomas Witt
    Copyright (c) 2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

///////////////////////////////////////////////////////////////////////////////
//
//  File Iterator structure
//
//  The new structure is designed on layers. The top class (used by the user)
//  is file_iterator, which implements a full random access iterator through
//  the file, and some specific member functions (constructor that opens
//  the file, make_end() to generate the end iterator, operator bool to check
//  if the file was opened correctly).
//
//  file_iterator implements the random access iterator interface by the means
//  of boost::iterator_adaptor, that is inhering an object created with it.
//  iterator_adaptor gets a low-level file iterator implementation (with just
//  a few member functions) and a policy (that basically describes to it how
//  the low-level file iterator interface is). The advantage is that
//  with boost::iterator_adaptor only 5 functions are needed to implement
//  a fully conformant random access iterator, instead of dozens of functions
//  and operators.
//
//  There are two low-level file iterators implemented in this module. The
//  first (std_file_iterator) uses cstdio stream functions (fopen/fread), which
//  support full buffering, and is available everywhere (it's standard C++).
//  The second (mmap_file_iterator) is currently available only on Windows
//  platforms, and uses memory mapped files, which gives a decent speed boost.
//
///////////////////////////////////////////////////////////////////////////////
//
//  TODO LIST:
//
//  - In the Win32 mmap iterator, we could check if keeping a handle to the
//    opened file is really required. If it's not, we can just store the file
//    length (for make_end()) and save performance. Notice that this should be
//    tested under different Windows versions, the behaviour might change.
//  - Add some error support (by the means of some exceptions) in case of
//    low-level I/O failure.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_SPIRIT_FILE_ITERATOR_HPP
#define BOOST_SPIRIT_FILE_ITERATOR_HPP

#include <string>
#include <boost/config.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/safe_bool.hpp>

#include <boost/spirit/home/classic/iterator/file_iterator_fwd.hpp>

#if !defined(BOOST_SPIRIT_FILEITERATOR_STD)
#  if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__)) \
      && !defined(BOOST_DISABLE_WIN32)
#    define BOOST_SPIRIT_FILEITERATOR_WINDOWS
#  elif defined(BOOST_HAS_UNISTD_H)
extern "C"
{
#    include <unistd.h>
}
#    ifdef _POSIX_MAPPED_FILES
#      define BOOST_SPIRIT_FILEITERATOR_POSIX
#    endif // _POSIX_MAPPED_FILES
#  endif // BOOST_HAS_UNISTD_H

#  if !defined(BOOST_SPIRIT_FILEITERATOR_WINDOWS) && \
      !defined(BOOST_SPIRIT_FILEITERATOR_POSIX)
#    define BOOST_SPIRIT_FILEITERATOR_STD
#  endif
#endif // BOOST_SPIRIT_FILEITERATOR_STD

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

template <
    typename CharT = char,
    typename BaseIterator =
#ifdef BOOST_SPIRIT_FILEITERATOR_STD
        fileiter_impl::std_file_iterator<CharT>
#else
        fileiter_impl::mmap_file_iterator<CharT>
#endif
> class file_iterator;

///////////////////////////////////////////////////////////////////////////////
namespace fileiter_impl {

    /////////////////////////////////////////////////////////////////////////
    //
    //  file_iter_generator
    //
    //  Template meta-function to invoke boost::iterator_adaptor
    //  NOTE: This cannot be moved into the implementation file because of
    //  a bug of MSVC 7.0 and previous versions (base classes types are
    //  looked up at compilation time, not instantion types, and
    //  file_iterator would break).
    //
    /////////////////////////////////////////////////////////////////////////

#if !defined(BOOST_ITERATOR_ADAPTORS_VERSION) || \
     BOOST_ITERATOR_ADAPTORS_VERSION < 0x0200
#error "Please use at least Boost V1.31.0 while compiling the file_iterator class!"
#else // BOOST_ITERATOR_ADAPTORS_VERSION < 0x0200

    template <typename CharT, typename BaseIteratorT>
    struct file_iter_generator
    {
    public:
        typedef BaseIteratorT adapted_t;
        typedef typename adapted_t::value_type value_type;

        typedef boost::iterator_adaptor <
            file_iterator<CharT, BaseIteratorT>,
            adapted_t,
            value_type const,
            std::random_access_iterator_tag,
            boost::use_default,
            std::ptrdiff_t
        > type;
    };

#endif // BOOST_ITERATOR_ADAPTORS_VERSION < 0x0200

///////////////////////////////////////////////////////////////////////////////
} /* namespace impl */


///////////////////////////////////////////////////////////////////////////////
//
//  file_iterator
//
//  Iterates through an opened file.
//
//  The main iterator interface is implemented by the iterator_adaptors
//  library, which wraps a conforming iterator interface around the
//  impl::BaseIterator class. This class merely derives the iterator_adaptors
//  generated class to implement the custom constructors and make_end()
//  member function.
//
///////////////////////////////////////////////////////////////////////////////

template<typename CharT, typename BaseIteratorT>
class file_iterator
    : public fileiter_impl::file_iter_generator<CharT, BaseIteratorT>::type,
      public safe_bool<file_iterator<CharT, BaseIteratorT> >
{
private:
    typedef typename
        fileiter_impl::file_iter_generator<CharT, BaseIteratorT>::type
        base_t;
    typedef typename
        fileiter_impl::file_iter_generator<CharT, BaseIteratorT>::adapted_t
        adapted_t;

public:
    file_iterator()
    {}

    file_iterator(std::string const& fileName)
    :   base_t(adapted_t(fileName))
    {}

    file_iterator(const base_t& iter)
    :   base_t(iter)
    {}

    inline file_iterator& operator=(const base_t& iter);
    file_iterator make_end(void);

    // operator bool. This borrows a trick from boost::shared_ptr to avoid
    //   to interfere with arithmetic operations.
    bool operator_bool(void) const
    { return this->base(); }

private:
    friend class ::boost::iterator_core_access;

    typename base_t::reference dereference() const
    {
        return this->base_reference().get_cur_char();
    }

    void increment()
    {
        this->base_reference().next_char();
    }

    void decrement()
    {
        this->base_reference().prev_char();
    }

    void advance(typename base_t::difference_type n)
    {
        this->base_reference().advance(n);
    }

    template <
        typename OtherDerivedT, typename OtherIteratorT,
        typename V, typename C, typename R, typename D
    >
    typename base_t::difference_type distance_to(
        iterator_adaptor<OtherDerivedT, OtherIteratorT, V, C, R, D>
        const &x) const
    {
        return x.base().distance(this->base_reference());
    }
};

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} /* namespace BOOST_SPIRIT_CLASSIC_NS */

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/iterator/impl/file_iterator.ipp> /* implementation */

#endif /* BOOST_SPIRIT_FILE_ITERATOR_HPP */

