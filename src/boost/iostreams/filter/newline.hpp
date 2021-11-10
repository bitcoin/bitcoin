// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// NOTE: I hope to replace the current implementation with a much simpler
// one.

#ifndef BOOST_IOSTREAMS_NEWLINE_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_NEWLINE_FILTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assert.hpp>
#include <cstdio>
#include <stdexcept>                       // logic_error.
#include <boost/config.hpp>                // BOOST_STATIC_CONSTANT.
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/detail/char_traits.hpp>
#include <boost/iostreams/detail/ios.hpp>  // BOOST_IOSTREAMS_FAILURE 
#include <boost/iostreams/read.hpp>        // get 
#include <boost/iostreams/write.hpp>       // put 
#include <boost/iostreams/pipeline.hpp>
#include <boost/iostreams/putback.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_convertible.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp>

#define BOOST_IOSTREAMS_ASSERT_UNREACHABLE(val) \
    (BOOST_ASSERT("unreachable code" == 0), val) \
    /**/

namespace boost { namespace iostreams {

namespace newline {

const char CR                   = 0x0D;
const char LF                   = 0x0A;

    // Flags for configuring newline_filter.

// Exactly one of the following three flags must be present.

const int posix             = 1;    // Use CR as line separator.
const int mac               = 2;    // Use LF as line separator.
const int dos               = 4;    // Use CRLF as line separator.
const int mixed             = 8;    // Mixed line endings.
const int final_newline     = 16;
const int platform_mask     = posix | dos | mac;

} // End namespace newline.

namespace detail {

class newline_base {
public:
    bool is_posix() const
    {
        return !is_mixed() && (flags_ & newline::posix) != 0;
    }
    bool is_dos() const
    {
        return !is_mixed() && (flags_ & newline::dos) != 0;
    }
    bool is_mac() const
    {
        return !is_mixed() && (flags_ & newline::mac) != 0;
    }
    bool is_mixed_posix() const { return (flags_ & newline::posix) != 0; }
    bool is_mixed_dos() const { return (flags_ & newline::dos) != 0; }
    bool is_mixed_mac() const { return (flags_ & newline::mac) != 0; }
    bool is_mixed() const
    {
        int platform =
            (flags_ & newline::posix) != 0 ?
                newline::posix :
                (flags_ & newline::dos) != 0 ?
                    newline::dos :
                    (flags_ & newline::mac) != 0 ?
                        newline::mac :
                        0;
        return (flags_ & ~platform & newline::platform_mask) != 0;
    }
    bool has_final_newline() const
    {
        return (flags_ & newline::final_newline) != 0;
    }
protected:
    newline_base(int flags) : flags_(flags) { }
    int flags_;
};

} // End namespace detail.

class newline_error
    : public BOOST_IOSTREAMS_FAILURE, public detail::newline_base
{
private:
    friend class newline_checker;
    newline_error(int flags)
        : BOOST_IOSTREAMS_FAILURE("bad line endings"),
          detail::newline_base(flags)
        { }
};

class newline_filter {
public:
    typedef char char_type;
    struct category
        : dual_use,
          filter_tag,
          closable_tag
        { };

    explicit newline_filter(int target) : flags_(target)
    {
        if ( target != iostreams::newline::posix &&
             target != iostreams::newline::dos &&
             target != iostreams::newline::mac )
        {
            boost::throw_exception(std::logic_error("bad flags"));
        }
    }

    template<typename Source>
    int get(Source& src)
    {
        using iostreams::newline::CR;
        using iostreams::newline::LF;

        BOOST_ASSERT((flags_ & f_write) == 0);
        flags_ |= f_read;

        if (flags_ & (f_has_LF | f_has_EOF)) {
            if (flags_ & f_has_LF)
                return newline();
            else
                return EOF;
        }

        int c =
            (flags_ & f_has_CR) == 0 ?
                iostreams::get(src) :
                CR;

        if (c == WOULD_BLOCK )
            return WOULD_BLOCK;

        if (c == CR) {
            flags_ |= f_has_CR;

            int d;
            if ((d = iostreams::get(src)) == WOULD_BLOCK)
                return WOULD_BLOCK;

            if (d == LF) {
                flags_ &= ~f_has_CR;
                return newline();
            }

            if (d == EOF) {
                flags_ |= f_has_EOF;
            } else {
                iostreams::putback(src, d);
            }

            flags_ &= ~f_has_CR;
            return newline();
        }

        if (c == LF)
            return newline();

        return c;
    }

    template<typename Sink>
    bool put(Sink& dest, char c)
    {
        using iostreams::newline::CR;
        using iostreams::newline::LF;

        BOOST_ASSERT((flags_ & f_read) == 0);
        flags_ |= f_write;

        if ((flags_ & f_has_LF) != 0)
            return c == LF ?
                newline(dest) :
                newline(dest) && this->put(dest, c);

        if (c == LF)
           return newline(dest);

        if ((flags_ & f_has_CR) != 0)
            return newline(dest) ?
                this->put(dest, c) :
                false;

        if (c == CR) {
            flags_ |= f_has_CR;
            return true;
        }

        return iostreams::put(dest, c);
    }

    template<typename Sink>
    void close(Sink& dest, BOOST_IOS::openmode)
    {
        if ((flags_ & f_write) != 0 && (flags_ & f_has_CR) != 0)
            newline_if_sink(dest);
        flags_ &= ~f_has_LF; // Restore original flags.
    }
private:

    // Returns the appropriate element of a newline sequence.
    int newline()
    {
        using iostreams::newline::CR;
        using iostreams::newline::LF;

        switch (flags_ & iostreams::newline::platform_mask) {
        case iostreams::newline::posix:
            return LF;
        case iostreams::newline::mac:
            return CR;
        case iostreams::newline::dos:
            if (flags_ & f_has_LF) {
                flags_ &= ~f_has_LF;
                return LF;
            } else {
                flags_ |= f_has_LF;
                return CR;
            }
        }
        return BOOST_IOSTREAMS_ASSERT_UNREACHABLE(0);
    }

    // Writes a newline sequence.
    template<typename Sink>
    bool newline(Sink& dest)
    {
        using iostreams::newline::CR;
        using iostreams::newline::LF;

        bool success = false;
        switch (flags_ & iostreams::newline::platform_mask) {
        case iostreams::newline::posix:
            success = boost::iostreams::put(dest, LF);
            break;
        case iostreams::newline::mac:
            success = boost::iostreams::put(dest, CR);
            break;
        case iostreams::newline::dos:
            if ((flags_ & f_has_LF) != 0) {
                if ((success = boost::iostreams::put(dest, LF)))
                    flags_ &= ~f_has_LF;
            } else if (boost::iostreams::put(dest, CR)) {
                if (!(success = boost::iostreams::put(dest, LF)))
                    flags_ |= f_has_LF;
            }
            break;
        }
        if (success)
            flags_ &= ~f_has_CR;
        return success;
    }

    // Writes a newline sequence if the given device is a Sink.
    template<typename Device>
    void newline_if_sink(Device& dest) 
    { 
        typedef typename iostreams::category_of<Device>::type category;
        newline_if_sink(dest, is_convertible<category, output>()); 
    }

    template<typename Sink>
    void newline_if_sink(Sink& dest, mpl::true_) { newline(dest); }

    template<typename Source>
    void newline_if_sink(Source&, mpl::false_) { }

    enum flags {
        f_has_LF         = 32768,
        f_has_CR         = f_has_LF << 1,
        f_has_newline    = f_has_CR << 1,
        f_has_EOF        = f_has_newline << 1,
        f_read           = f_has_EOF << 1,
        f_write          = f_read << 1
    };
    int       flags_;
};
BOOST_IOSTREAMS_PIPABLE(newline_filter, 0)

class newline_checker : public detail::newline_base {
public:
    typedef char                 char_type;
    struct category
        : dual_use_filter_tag,
          closable_tag
        { };
    explicit newline_checker(int target = newline::mixed)
        : detail::newline_base(0), target_(target), open_(false)
        { }
    template<typename Source>
    int get(Source& src)
    {
        using newline::CR;
        using newline::LF;

        if (!open_) {
            open_ = true;
            source() = 0;
        }

        int c;
        if ((c = iostreams::get(src)) == WOULD_BLOCK)
            return WOULD_BLOCK;

        // Update source flags.
        if (c != EOF)
            source() &= ~f_line_complete;
        if ((source() & f_has_CR) != 0) {
            if (c == LF) {
                source() |= newline::dos;
                source() |= f_line_complete;
            } else {
                source() |= newline::mac;
                if (c == EOF)
                    source() |= f_line_complete;
            }
        } else if (c == LF) {
            source() |= newline::posix;
            source() |= f_line_complete;
        }
        source() = (source() & ~f_has_CR) | (c == CR ? f_has_CR : 0);

        // Check for errors.
        if ( c == EOF &&
            (target_ & newline::final_newline) != 0 &&
            (source() & f_line_complete) == 0 )
        {
            fail();
        }
        if ( (target_ & newline::platform_mask) != 0 &&
             (source() & ~target_ & newline::platform_mask) != 0 )
        {
            fail();
        }

        return c;
    }

    template<typename Sink>
    bool put(Sink& dest, int c)
    {
        using iostreams::newline::CR;
        using iostreams::newline::LF;

        if (!open_) {
            open_ = true;
            source() = 0;
        }

        if (!iostreams::put(dest, c))
            return false;

         // Update source flags.
        source() &= ~f_line_complete;
        if ((source() & f_has_CR) != 0) {
            if (c == LF) {
                source() |= newline::dos;
                source() |= f_line_complete;
            } else {
                source() |= newline::mac;
            }
        } else if (c == LF) {
            source() |= newline::posix;
            source() |= f_line_complete;
        }
        source() = (source() & ~f_has_CR) | (c == CR ? f_has_CR : 0);

        // Check for errors.
        if ( (target_ & newline::platform_mask) != 0 &&
             (source() & ~target_ & newline::platform_mask) != 0 )
        {
            fail();
        }

        return true;
    }

    template<typename Sink>
    void close(Sink&, BOOST_IOS::openmode)
    {
        using iostreams::newline::final_newline;

        // Update final_newline flag.
        if ( (source() & f_has_CR) != 0 ||
             (source() & f_line_complete) != 0 )
        {
            source() |= final_newline;
        }

        // Clear non-sticky flags.
        source() &= ~(f_has_CR | f_line_complete);

        // Check for errors.
        if ( (target_ & final_newline) != 0 &&
             (source() & final_newline) == 0 )
        {
            fail();
        }
    }
private:
    void fail() { boost::throw_exception(newline_error(source())); }
    int& source() { return flags_; }
    int source() const { return flags_; }

    enum flags {
        f_has_CR = 32768,
        f_line_complete = f_has_CR << 1
    };

    int   target_;  // Represents expected input.
    bool  open_;
};
BOOST_IOSTREAMS_PIPABLE(newline_checker, 0)

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>

#endif // #ifndef BOOST_IOSTREAMS_NEWLINE_FILTER_HPP_INCLUDED
