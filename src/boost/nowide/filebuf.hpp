//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//  Copyright (c) 2019-2020 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NOWIDE_FILEBUF_HPP_INCLUDED
#define BOOST_NOWIDE_FILEBUF_HPP_INCLUDED

#include <boost/nowide/config.hpp>
#if BOOST_NOWIDE_USE_FILEBUF_REPLACEMENT
#include <boost/nowide/cstdio.hpp>
#include <boost/nowide/stackstring.hpp>
#include <cassert>
#include <cstdio>
#include <ios>
#include <limits>
#include <locale>
#include <stdexcept>
#include <streambuf>
#else
#include <fstream>
#endif

namespace boost {
namespace nowide {
    namespace detail {
        /// Same as std::ftell but potentially with Large File Support
        BOOST_NOWIDE_DECL std::streampos ftell(FILE* file);
        /// Same as std::fseek but potentially with Large File Support
        BOOST_NOWIDE_DECL int fseek(FILE* file, std::streamoff offset, int origin);
    } // namespace detail

#if !BOOST_NOWIDE_USE_FILEBUF_REPLACEMENT && !defined(BOOST_NOWIDE_DOXYGEN)
    using std::basic_filebuf;
    using std::filebuf;
#else // Windows
    ///
    /// \brief This forward declaration defines the basic_filebuf type.
    ///
    /// it is implemented and specialized for CharType = char, it
    /// implements std::filebuf over standard C I/O
    ///
    template<typename CharType, typename Traits = std::char_traits<CharType>>
    class basic_filebuf;

    ///
    /// \brief This is the implementation of std::filebuf
    ///
    /// it is implemented and specialized for CharType = char, it
    /// implements std::filebuf over standard C I/O
    ///
    template<>
    class basic_filebuf<char> : public std::basic_streambuf<char>
    {
        using Traits = std::char_traits<char>;

    public:
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4351) // new behavior : elements of array will be default initialized
#endif
        ///
        /// Creates new filebuf
        ///
        basic_filebuf() :
            buffer_size_(BUFSIZ), buffer_(0), file_(0), owns_buffer_(false), last_char_(),
            mode_(std::ios_base::openmode(0))
        {
            setg(0, 0, 0);
            setp(0, 0);
        }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
        basic_filebuf(const basic_filebuf&) = delete;
        basic_filebuf& operator=(const basic_filebuf&) = delete;
        basic_filebuf(basic_filebuf&& other) noexcept : basic_filebuf()
        {
            swap(other);
        }
        basic_filebuf& operator=(basic_filebuf&& other) noexcept
        {
            close();
            swap(other);
            return *this;
        }
        void swap(basic_filebuf& rhs)
        {
            std::basic_streambuf<char>::swap(rhs);
            using std::swap;
            swap(buffer_size_, rhs.buffer_size_);
            swap(buffer_, rhs.buffer_);
            swap(file_, rhs.file_);
            swap(owns_buffer_, rhs.owns_buffer_);
            swap(last_char_[0], rhs.last_char_[0]);
            swap(mode_, rhs.mode_);

            // Fixup last_char references
            if(pbase() == rhs.last_char_)
                setp(last_char_, (pptr() == epptr()) ? last_char_ : last_char_ + 1);
            if(eback() == rhs.last_char_)
                setg(last_char_, (gptr() == rhs.last_char_) ? last_char_ : last_char_ + 1, last_char_ + 1);

            if(rhs.pbase() == last_char_)
                rhs.setp(rhs.last_char_, (rhs.pptr() == rhs.epptr()) ? rhs.last_char_ : rhs.last_char_ + 1);
            if(rhs.eback() == last_char_)
            {
                rhs.setg(rhs.last_char_,
                         (rhs.gptr() == last_char_) ? rhs.last_char_ : rhs.last_char_ + 1,
                         rhs.last_char_ + 1);
            }
        }

        virtual ~basic_filebuf()
        {
            close();
        }

        ///
        /// Same as std::filebuf::open but s is UTF-8 string
        ///
        basic_filebuf* open(const std::string& s, std::ios_base::openmode mode)
        {
            return open(s.c_str(), mode);
        }
        ///
        /// Same as std::filebuf::open but s is UTF-8 string
        ///
        basic_filebuf* open(const char* s, std::ios_base::openmode mode)
        {
            const wstackstring name(s);
            return open(name.get(), mode);
        }
        /// Opens the file with the given name, see std::filebuf::open
        basic_filebuf* open(const wchar_t* s, std::ios_base::openmode mode)
        {
            if(is_open())
                return NULL;
            validate_cvt(this->getloc());
            const bool ate = (mode & std::ios_base::ate) != 0;
            if(ate)
                mode &= ~std::ios_base::ate;
            const wchar_t* smode = get_mode(mode);
            if(!smode)
                return 0;
            file_ = detail::wfopen(s, smode);
            if(!file_)
                return 0;
            if(ate && detail::fseek(file_, 0, SEEK_END) != 0)
            {
                close();
                return 0;
            }
            mode_ = mode;
            return this;
        }
        ///
        /// Same as std::filebuf::close()
        ///
        basic_filebuf* close()
        {
            if(!is_open())
                return NULL;
            bool res = sync() == 0;
            if(std::fclose(file_) != 0)
                res = false;
            file_ = NULL;
            mode_ = std::ios_base::openmode(0);
            if(owns_buffer_)
            {
                delete[] buffer_;
                buffer_ = NULL;
                owns_buffer_ = false;
            }
            setg(0, 0, 0);
            setp(0, 0);
            return res ? this : NULL;
        }
        ///
        /// Same as std::filebuf::is_open()
        ///
        bool is_open() const
        {
            return file_ != NULL;
        }

    private:
        void make_buffer()
        {
            if(buffer_)
                return;
            if(buffer_size_ > 0)
            {
                buffer_ = new char[buffer_size_];
                owns_buffer_ = true;
            }
        }
        void validate_cvt(const std::locale& loc)
        {
            if(!std::use_facet<std::codecvt<char, char, std::mbstate_t>>(loc).always_noconv())
                throw std::runtime_error("Converting codecvts are not supported");
        }

    protected:
        std::streambuf* setbuf(char* s, std::streamsize n) override
        {
            assert(n >= 0);
            // Maximum compatibility: Discard all local buffers and use user-provided values
            // Users should call sync() before or better use it before any IO is done or any file is opened
            setg(NULL, NULL, NULL);
            setp(NULL, NULL);
            if(owns_buffer_)
                delete[] buffer_;
            buffer_ = s;
            buffer_size_ = (n >= 0) ? static_cast<size_t>(n) : 0;
            return this;
        }

        int overflow(int c = EOF) override
        {
            if(!(mode_ & (std::ios_base::out | std::ios_base::app)))
                return EOF;

            if(!stop_reading())
                return EOF;

            size_t n = pptr() - pbase();
            if(n > 0)
            {
                if(std::fwrite(pbase(), 1, n, file_) != n)
                    return EOF;
                setp(buffer_, buffer_ + buffer_size_);
                if(c != EOF)
                {
                    *buffer_ = Traits::to_char_type(c);
                    pbump(1);
                }
            } else if(c != EOF)
            {
                if(buffer_size_ > 0)
                {
                    make_buffer();
                    setp(buffer_, buffer_ + buffer_size_);
                    *buffer_ = Traits::to_char_type(c);
                    pbump(1);
                } else if(std::fputc(c, file_) == EOF)
                {
                    return EOF;
                } else if(!pptr())
                {
                    // Set to dummy value so we know we have written something
                    setp(last_char_, last_char_);
                }
            }
            return Traits::not_eof(c);
        }

        int sync() override
        {
            if(!file_)
                return 0;
            bool result;
            if(pptr())
            {
                result = overflow() != EOF;
                // Only flush if anything was written, otherwise behavior of fflush is undefined
                if(std::fflush(file_) != 0)
                    return result = false;
            } else
                result = stop_reading();
            return result ? 0 : -1;
        }

        int underflow() override
        {
            if(!(mode_ & std::ios_base::in))
                return EOF;
            if(!stop_writing())
                return EOF;
            // In text mode we cannot use a buffer size of more than 1 (i.e. single char only)
            // This is due to the need to seek back in case of a sync to "put back" unread chars.
            // However determining the number of chars to seek back is impossible in case there are newlines
            // as we cannot know if those were converted.
            if(buffer_size_ == 0 || !(mode_ & std::ios_base::binary))
            {
                const int c = std::fgetc(file_);
                if(c == EOF)
                    return EOF;
                last_char_[0] = Traits::to_char_type(c);
                setg(last_char_, last_char_, last_char_ + 1);
            } else
            {
                make_buffer();
                const size_t n = std::fread(buffer_, 1, buffer_size_, file_);
                setg(buffer_, buffer_, buffer_ + n);
                if(n == 0)
                    return EOF;
            }
            return Traits::to_int_type(*gptr());
        }

        int pbackfail(int c = EOF) override
        {
            if(!(mode_ & std::ios_base::in))
                return EOF;
            if(!stop_writing())
                return EOF;
            if(gptr() > eback())
                gbump(-1);
            else if(seekoff(-1, std::ios_base::cur) != std::streampos(std::streamoff(-1)))
            {
                if(underflow() == EOF)
                    return EOF;
            } else
                return EOF;

            // Case 1: Caller just wanted space for 1 char
            if(c == EOF)
                return Traits::not_eof(c);
            // Case 2: Caller wants to put back different char
            // gptr now points to the (potentially newly read) previous char
            if(*gptr() != c)
                *gptr() = Traits::to_char_type(c);
            return Traits::not_eof(c);
        }

        std::streampos seekoff(std::streamoff off,
                               std::ios_base::seekdir seekdir,
                               std::ios_base::openmode = std::ios_base::in | std::ios_base::out) override
        {
            if(!file_)
                return EOF;
            // Switching between input<->output requires a seek
            // So do NOT optimize for seekoff(0, cur) as No-OP

            // On some implementations a seek also flushes, so do a full sync
            if(sync() != 0)
                return EOF;
            int whence;
            switch(seekdir)
            {
            case std::ios_base::beg: whence = SEEK_SET; break;
            case std::ios_base::cur: whence = SEEK_CUR; break;
            case std::ios_base::end: whence = SEEK_END; break;
            default: assert(false); return EOF;
            }
            if(detail::fseek(file_, off, whence) != 0)
                return EOF;
            return detail::ftell(file_);
        }
        std::streampos seekpos(std::streampos pos,
                               std::ios_base::openmode m = std::ios_base::in | std::ios_base::out) override
        {
            // Standard mandates "as-if fsetpos", but assume the effect is the same as fseek
            return seekoff(pos, std::ios_base::beg, m);
        }
        void imbue(const std::locale& loc) override
        {
            validate_cvt(loc);
        }

    private:
        /// Stop reading adjusting the file pointer if necessary
        /// Postcondition: gptr() == NULL
        bool stop_reading()
        {
            if(!gptr())
                return true;
            const auto off = gptr() - egptr();
            setg(0, 0, 0);
            if(!off)
                return true;
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#endif
            // coverity[result_independent_of_operands]
            if(off > std::numeric_limits<std::streamoff>::max())
                return false;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
            return detail::fseek(file_, static_cast<std::streamoff>(off), SEEK_CUR) == 0;
        }

        /// Stop writing. If any bytes are to be written, writes them to file
        /// Postcondition: pptr() == NULL
        bool stop_writing()
        {
            if(pptr())
            {
                const char* const base = pbase();
                const size_t n = pptr() - base;
                setp(0, 0);
                if(n && std::fwrite(base, 1, n, file_) != n)
                    return false;
            }
            return true;
        }

        void reset(FILE* f = 0)
        {
            sync();
            if(file_)
            {
                fclose(file_);
                file_ = 0;
            }
            file_ = f;
        }

        static const wchar_t* get_mode(std::ios_base::openmode mode)
        {
            //
            // done according to n2914 table 106 27.9.1.4
            //

            // note can't use switch case as overload operator can't be used
            // in constant expression
            if(mode == (std::ios_base::out))
                return L"w";
            if(mode == (std::ios_base::out | std::ios_base::app))
                return L"a";
            if(mode == (std::ios_base::app))
                return L"a";
            if(mode == (std::ios_base::out | std::ios_base::trunc))
                return L"w";
            if(mode == (std::ios_base::in))
                return L"r";
            if(mode == (std::ios_base::in | std::ios_base::out))
                return L"r+";
            if(mode == (std::ios_base::in | std::ios_base::out | std::ios_base::trunc))
                return L"w+";
            if(mode == (std::ios_base::in | std::ios_base::out | std::ios_base::app))
                return L"a+";
            if(mode == (std::ios_base::in | std::ios_base::app))
                return L"a+";
            if(mode == (std::ios_base::binary | std::ios_base::out))
                return L"wb";
            if(mode == (std::ios_base::binary | std::ios_base::out | std::ios_base::app))
                return L"ab";
            if(mode == (std::ios_base::binary | std::ios_base::app))
                return L"ab";
            if(mode == (std::ios_base::binary | std::ios_base::out | std::ios_base::trunc))
                return L"wb";
            if(mode == (std::ios_base::binary | std::ios_base::in))
                return L"rb";
            if(mode == (std::ios_base::binary | std::ios_base::in | std::ios_base::out))
                return L"r+b";
            if(mode == (std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc))
                return L"w+b";
            if(mode == (std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app))
                return L"a+b";
            if(mode == (std::ios_base::binary | std::ios_base::in | std::ios_base::app))
                return L"a+b";
            return 0;
        }

        size_t buffer_size_;
        char* buffer_;
        FILE* file_;
        bool owns_buffer_;
        char last_char_[1];
        std::ios::openmode mode_;
    };

    ///
    /// \brief Convenience typedef
    ///
    using filebuf = basic_filebuf<char>;

    /// Swap the basic_filebuf instances
    template<typename CharType, typename Traits>
    void swap(basic_filebuf<CharType, Traits>& lhs, basic_filebuf<CharType, Traits>& rhs)
    {
        lhs.swap(rhs);
    }

#endif // windows

} // namespace nowide
} // namespace boost

#endif
