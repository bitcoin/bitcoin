// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_DETAIL_LINKED_STREAMBUF_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_LINKED_STREAMBUF_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>                        // member template friends.
#include <boost/core/typeinfo.hpp>
#include <boost/iostreams/detail/char_traits.hpp>
#include <boost/iostreams/detail/ios.hpp>          // openmode.
#include <boost/iostreams/detail/streambuf.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp> // MSVC.

namespace boost { namespace iostreams { namespace detail {

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
class chain_base;

template<typename Chain, typename Access, typename Mode> class chainbuf;

#define BOOST_IOSTREAMS_USING_PROTECTED_STREAMBUF_MEMBERS(base) \
    using base::eback; using base::gptr; using base::egptr; \
    using base::setg; using base::gbump; using base::pbase; \
    using base::pptr; using base::epptr; using base::setp; \
    using base::pbump; using base::underflow; using base::pbackfail; \
    using base::xsgetn; using base::overflow; using base::xsputn; \
    using base::sync; using base::seekoff; using base::seekpos; \
    /**/

template<typename Ch, typename Tr = BOOST_IOSTREAMS_CHAR_TRAITS(Ch) >
class linked_streambuf : public BOOST_IOSTREAMS_BASIC_STREAMBUF(Ch, Tr) {
protected:
    linked_streambuf() : flags_(0) { }
    void set_true_eof(bool eof) 
    { 
        flags_ = (flags_ & ~f_true_eof) | (eof ? f_true_eof : 0); 
    }
public:

    // Should be called only after receiving an ordinary EOF indication,
    // to confirm that it represents EOF rather than WOULD_BLOCK.
    bool true_eof() const { return (flags_ & f_true_eof) != 0; }
protected:

    //----------grant friendship to chain_base and chainbuf-------------------//

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
    template< typename Self, typename ChT, typename TrT,
              typename Alloc, typename Mode >
    friend class chain_base;
    template<typename Chain, typename Mode, typename Access>
    friend class chainbuf;
    template<typename U>
    friend class member_close_operation; 
#else
    public:
        typedef BOOST_IOSTREAMS_BASIC_STREAMBUF(Ch, Tr) base;
        BOOST_IOSTREAMS_USING_PROTECTED_STREAMBUF_MEMBERS(base)
#endif
    void close(BOOST_IOS::openmode which)
    {
        if ( which == BOOST_IOS::in && 
            (flags_ & f_input_closed) == 0 )
        {
            flags_ |= f_input_closed;
            close_impl(which);
        }
        if ( which == BOOST_IOS::out && 
            (flags_ & f_output_closed) == 0 )
        {
            flags_ |= f_output_closed;
            close_impl(which);
        }
    }
    void set_needs_close()
    {
        flags_ &= ~(f_input_closed | f_output_closed);
    }
    virtual void set_next(linked_streambuf<Ch, Tr>* /* next */) { }
    virtual void close_impl(BOOST_IOS::openmode) = 0;
    virtual bool auto_close() const = 0;
    virtual void set_auto_close(bool) = 0;
    virtual bool strict_sync() = 0;
    virtual const boost::core::typeinfo& component_type() const = 0;
    virtual void* component_impl() = 0;
#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
    private:
#else
    public:
#endif
private:
    enum flag_type {
        f_true_eof       = 1,
        f_input_closed   = f_true_eof << 1,
        f_output_closed  = f_input_closed << 1
    };
    int flags_;
};

} } } // End namespaces detail, iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp> // MSVC.

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_LINKED_STREAMBUF_HPP_INCLUDED
