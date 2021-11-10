/*=============================================================================
    Copyright (c) 2003 Giovanni Bajo
    Copyright (c) 2003 Martin Wille
    Copyright (c) 2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#ifndef BOOST_SPIRIT_FILE_ITERATOR_IPP
#define BOOST_SPIRIT_FILE_ITERATOR_IPP

#ifdef BOOST_SPIRIT_FILEITERATOR_WINDOWS
#  include <windows.h>
#endif

#include <cstdio>
#include <boost/shared_ptr.hpp>

#ifdef BOOST_SPIRIT_FILEITERATOR_WINDOWS
#  include <boost/type_traits/remove_pointer.hpp>
#endif

#ifdef BOOST_SPIRIT_FILEITERATOR_POSIX
#  include <sys/types.h> // open, stat, mmap, munmap
#  include <sys/stat.h>  // stat
#  include <fcntl.h>     // open
#  include <unistd.h>    // stat, mmap, munmap
#  include <sys/mman.h>  // mmap, mmunmap
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
namespace fileiter_impl {

///////////////////////////////////////////////////////////////////////////////
//
//  std_file_iterator
//
//  Base class that implements iteration through a file using standard C
//  stream library (fopen and friends). This class and the following are
//  the base components on which the iterator is built (through the
//  iterator adaptor library).
//
//  The opened file stream (FILE) is held with a shared_ptr<>, whose
//  custom deleter invokes fcose(). This makes the syntax of the class
//  very easy, especially everything related to copying.
//
///////////////////////////////////////////////////////////////////////////////

template <typename CharT>
class std_file_iterator
{
public:
    typedef CharT value_type;

    std_file_iterator()
    {}

    explicit std_file_iterator(std::string const& fileName)
    {
        using namespace std;
        FILE* f = fopen(fileName.c_str(), "rb");

        // If the file was opened, store it into
        //  the smart pointer.
        if (f)
        {
            m_file.reset(f, fclose);
            m_pos = 0;
            m_eof = false;
            update_char();
        }
    }

    std_file_iterator(const std_file_iterator& iter)
    { *this = iter; }

    std_file_iterator& operator=(const std_file_iterator& iter)
    {
        m_file = iter.m_file;
        m_curChar = iter.m_curChar;
        m_eof = iter.m_eof;
        m_pos = iter.m_pos;

        return *this;
    }

    // Nasty bug in Comeau up to 4.3.0.1, we need explicit boolean context
    //  for shared_ptr to evaluate correctly
    operator bool() const
    { return m_file ? true : false; }

    bool operator==(const std_file_iterator& iter) const
    {
        return (m_file == iter.m_file) && (m_eof == iter.m_eof) &&
            (m_pos == iter.m_pos);
    }

    const CharT& get_cur_char(void) const
    {
        return m_curChar;
    }

    void prev_char(void)
    {
        m_pos -= sizeof(CharT);
        update_char();
    }

    void next_char(void)
    {
        m_pos += sizeof(CharT);
        update_char();
    }

    void seek_end(void)
    {
        using namespace std;
        fseek(m_file.get(), 0, SEEK_END);
        m_pos = ftell(m_file.get()) / sizeof(CharT);
        m_eof = true;
    }

    void advance(std::ptrdiff_t n)
    {
        m_pos += static_cast<long>(n) * sizeof(CharT);
        update_char();
    }

    std::ptrdiff_t distance(const std_file_iterator& iter) const
    {
        return (std::ptrdiff_t)(m_pos - iter.m_pos) / sizeof(CharT);
    }

private:
    boost::shared_ptr<std::FILE> m_file;
    long m_pos;
    CharT m_curChar;
    bool m_eof;

    void update_char(void)
    {
        using namespace std;
        if (ftell(m_file.get()) != m_pos)
            fseek(m_file.get(), m_pos, SEEK_SET);

        m_eof = (fread(&m_curChar, sizeof(CharT), 1, m_file.get()) < 1);
    }
};


///////////////////////////////////////////////////////////////////////////////
//
//  mmap_file_iterator
//
//  File iterator for memory mapped files, for now implemented on Windows and
//  POSIX  platforms. This class has the same interface of std_file_iterator,
//  and can be used in its place (in fact, it's the default for Windows and
//  POSIX).
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// mmap_file_iterator, Windows version
#ifdef BOOST_SPIRIT_FILEITERATOR_WINDOWS
template <typename CharT>
class mmap_file_iterator
{
public:
    typedef CharT value_type;

    mmap_file_iterator()
      : m_filesize(0), m_curChar(0)
    {}

    explicit mmap_file_iterator(std::string const& fileName)
      : m_filesize(0), m_curChar(0)
    {
        HANDLE hFile = ::CreateFileA(
            fileName.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN,
            NULL
        );

        if (hFile == INVALID_HANDLE_VALUE)
            return;

        // Store the size of the file, it's used to construct
        //  the end iterator
        m_filesize = ::GetFileSize(hFile, NULL);

        HANDLE hMap = ::CreateFileMapping(
            hFile,
            NULL,
            PAGE_READONLY,
            0, 0,
            NULL
        );

        if (hMap == NULL)
        {
            ::CloseHandle(hFile);
            return;
        }

        LPVOID pMem = ::MapViewOfFile(
            hMap,
            FILE_MAP_READ,
            0, 0, 0
        );

        if (pMem == NULL)
        {
            ::CloseHandle(hMap);
            ::CloseHandle(hFile);
            return;
        }

        // We hold both the file handle and the memory pointer.
        // We can close the hMap handle now because Windows holds internally
        //  a reference to it since there is a view mapped.
        ::CloseHandle(hMap);

        // It seems like we can close the file handle as well (because
        //  a reference is hold by the filemap object).
        ::CloseHandle(hFile);

        // Store the handles inside the shared_ptr (with the custom destructors)
        m_mem.reset(static_cast<CharT*>(pMem), ::UnmapViewOfFile);

        // Start of the file
        m_curChar = m_mem.get();
    }

    mmap_file_iterator(const mmap_file_iterator& iter)
    { *this = iter; }

    mmap_file_iterator& operator=(const mmap_file_iterator& iter)
    {
        m_curChar = iter.m_curChar;
        m_mem = iter.m_mem;
        m_filesize = iter.m_filesize;

        return *this;
    }

    // Nasty bug in Comeau up to 4.3.0.1, we need explicit boolean context
    //  for shared_ptr to evaluate correctly
    operator bool() const
    { return m_mem ? true : false; }

    bool operator==(const mmap_file_iterator& iter) const
    { return m_curChar == iter.m_curChar; }

    const CharT& get_cur_char(void) const
    { return *m_curChar; }

    void next_char(void)
    { m_curChar++; }

    void prev_char(void)
    { m_curChar--; }

    void advance(std::ptrdiff_t n)
    { m_curChar += n; }

    std::ptrdiff_t distance(const mmap_file_iterator& iter) const
    { return m_curChar - iter.m_curChar; }

    void seek_end(void)
    {
        m_curChar = m_mem.get() +
            (m_filesize / sizeof(CharT));
    }

private:
    typedef boost::remove_pointer<HANDLE>::type handle_t;

    boost::shared_ptr<CharT> m_mem;
    std::size_t m_filesize;
    CharT* m_curChar;
};

#endif // BOOST_SPIRIT_FILEITERATOR_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// mmap_file_iterator, POSIX version
#ifdef BOOST_SPIRIT_FILEITERATOR_POSIX
template <typename CharT>
class mmap_file_iterator
{
private:
    struct mapping
    {
        mapping(void *p, off_t len)
            : data(p)
            , size(len)
        { }

        CharT const *begin() const
        {
            return static_cast<CharT *>(data);
        }

        CharT const *end() const
        {
            return static_cast<CharT *>(data) + size/sizeof(CharT);
        }

        ~mapping()
        {
            munmap(static_cast<char*>(data), size);
        }

    private:
        void *data;
        off_t size;
    };

public:
    typedef CharT value_type;

    mmap_file_iterator()
      : m_curChar(0)
    {}

    explicit mmap_file_iterator(std::string const& file_name)
      : m_curChar(0)
    {
        // open the file
       int fd = open(file_name.c_str(),
#ifdef O_NOCTTY
            O_NOCTTY | // if stdin was closed then opening a file
                       // would cause the file to become the controlling
                       // terminal if the filename refers to a tty. Setting
                       // O_NOCTTY inhibits this.
#endif
            O_RDONLY);

        if (fd == -1)
            return;

        // call fstat to find get information about the file just
        // opened (size and file type)
        struct stat stat_buf;
        if ((fstat(fd, &stat_buf) != 0) || !S_ISREG(stat_buf.st_mode))
        {   // if fstat returns an error or if the file isn't a
            // regular file we give up.
            close(fd);
            return;
        }

        // perform the actual mapping
        void *p = mmap(0, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
        // it is safe to close() here. POSIX requires that the OS keeps a
        // second handle to the file while the file is mmapped.
        close(fd);

        if (p == MAP_FAILED)
            return;

        mapping *m = 0;
        try
        {
            m = new mapping(p, stat_buf.st_size);
        }
        catch(...)
        {
            munmap(static_cast<char*>(p), stat_buf.st_size);
            throw;
        }

        m_mem.reset(m);

        // Start of the file
        m_curChar = m_mem->begin();
    }

    mmap_file_iterator(const mmap_file_iterator& iter)
    { *this = iter; }

    mmap_file_iterator& operator=(const mmap_file_iterator& iter)
    {
        m_curChar = iter.m_curChar;
        m_mem = iter.m_mem;

        return *this;
    }

    // Nasty bug in Comeau up to 4.3.0.1, we need explicit boolean context
    //  for shared_ptr to evaluate correctly
    operator bool() const
    { return m_mem ? true : false; }

    bool operator==(const mmap_file_iterator& iter) const
    { return m_curChar == iter.m_curChar; }

    const CharT& get_cur_char(void) const
    { return *m_curChar; }

    void next_char(void)
    { m_curChar++; }

    void prev_char(void)
    { m_curChar--; }

    void advance(signed long n)
    { m_curChar += n; }

    long distance(const mmap_file_iterator& iter) const
    { return m_curChar - iter.m_curChar; }

    void seek_end(void)
    {
        m_curChar = m_mem->end();
    }

private:

    boost::shared_ptr<mapping> m_mem;
    CharT const* m_curChar;
};

#endif // BOOST_SPIRIT_FILEITERATOR_POSIX

///////////////////////////////////////////////////////////////////////////////
} /* namespace boost::spirit::fileiter_impl */

template <typename CharT, typename BaseIteratorT>
file_iterator<CharT,BaseIteratorT>
file_iterator<CharT,BaseIteratorT>::make_end(void)
{
    file_iterator iter(*this);
    iter.base_reference().seek_end();
    return iter;
}

template <typename CharT, typename BaseIteratorT>
file_iterator<CharT,BaseIteratorT>&
file_iterator<CharT,BaseIteratorT>::operator=(const base_t& iter)
{
    base_t::operator=(iter);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} /* namespace boost::spirit */


#endif /* BOOST_SPIRIT_FILE_ITERATOR_IPP */
