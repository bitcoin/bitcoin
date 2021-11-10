// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Contains machinery for performing code conversion.

#ifndef BOOST_IOSTREAMS_CODE_CONVERTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_CODE_CONVERTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/iostreams/detail/config/wide_streams.hpp>
#if defined(BOOST_IOSTREAMS_NO_WIDE_STREAMS) || \
    defined(BOOST_IOSTREAMS_NO_LOCALE) \
    /**/
# error code conversion not supported on this platform
#endif

#include <algorithm>                       // max.
#include <cstring>                         // memcpy.
#include <exception>
#include <boost/config.hpp>                // DEDUCED_TYPENAME, 
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/constants.hpp>   // default_filter_buffer_size.
#include <boost/iostreams/detail/adapter/concept_adapter.hpp>
#include <boost/iostreams/detail/adapter/direct_adapter.hpp>
#include <boost/iostreams/detail/buffer.hpp>
#include <boost/iostreams/detail/call_traits.hpp>
#include <boost/iostreams/detail/codecvt_holder.hpp>
#include <boost/iostreams/detail/codecvt_helper.hpp>
#include <boost/iostreams/detail/double_object.hpp>
#include <boost/iostreams/detail/execute.hpp>
#include <boost/iostreams/detail/forward.hpp>
#include <boost/iostreams/detail/functional.hpp>
#include <boost/iostreams/detail/ios.hpp> // failure, openmode, int types, streamsize.
#include <boost/iostreams/detail/optional.hpp>
#include <boost/iostreams/detail/select.hpp>
#include <boost/iostreams/traits.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp> // Borland 5.x

namespace boost { namespace iostreams {

struct code_conversion_error : BOOST_IOSTREAMS_FAILURE {
    code_conversion_error() 
        : BOOST_IOSTREAMS_FAILURE("code conversion error")
        { }
};

namespace detail {

//--------------Definition of strncpy_if_same---------------------------------//

// Helper template for strncpy_if_same, below.
template<bool B>
struct strncpy_if_same_impl;

template<>
struct strncpy_if_same_impl<true> {
    template<typename Ch>
    static Ch* copy(Ch* tgt, const Ch* src, std::streamsize n)
    { return BOOST_IOSTREAMS_CHAR_TRAITS(Ch)::copy(tgt, src, n); }
};

template<>
struct strncpy_if_same_impl<false> {
    template<typename Src, typename Tgt>
    static Tgt* copy(Tgt* tgt, const Src*, std::streamsize) { return tgt; }
};

template<typename Src, typename Tgt>
Tgt* strncpy_if_same(Tgt* tgt, const Src* src, std::streamsize n)
{
    typedef strncpy_if_same_impl<is_same<Src, Tgt>::value> impl;
    return impl::copy(tgt, src, n);
}

//--------------Definition of conversion_buffer-------------------------------//

// Buffer and conversion state for reading.
template<typename Codecvt, typename Alloc>
class conversion_buffer 
    : public buffer<
                 BOOST_DEDUCED_TYPENAME detail::codecvt_extern<Codecvt>::type,
                 Alloc
             > 
{
public:
    typedef typename Codecvt::state_type state_type;
    conversion_buffer() 
        : buffer<
              BOOST_DEDUCED_TYPENAME detail::codecvt_extern<Codecvt>::type,
              Alloc
          >(0) 
    { 
        reset(); 
    }
    state_type& state() { return state_; }
    void reset() 
    { 
        if (this->size()) 
            this->set(0, 0);
        state_ = state_type(); 
    }
private:
    state_type state_;
};

//--------------Definition of converter_impl----------------------------------//

// Contains member data, open/is_open/close and buffer management functions.
template<typename Device, typename Codecvt, typename Alloc>
struct code_converter_impl {
    typedef typename codecvt_extern<Codecvt>::type          extern_type;
    typedef typename category_of<Device>::type              device_category;
    typedef is_convertible<device_category, input>          can_read;
    typedef is_convertible<device_category, output>         can_write;
    typedef is_convertible<device_category, bidirectional>  is_bidir;
    typedef typename 
            iostreams::select<  // Disambiguation for Tru64.
                is_bidir, bidirectional,
                can_read, input,
                can_write, output
            >::type                                         mode;      
    typedef typename
            mpl::if_<
                is_direct<Device>,
                direct_adapter<Device>,
                Device
            >::type                                         device_type;
    typedef optional< concept_adapter<device_type> >        storage_type;
    typedef is_convertible<device_category, two_sequence>   is_double;
    typedef conversion_buffer<Codecvt, Alloc>               buffer_type;

    code_converter_impl() : cvt_(), flags_(0) { }

    ~code_converter_impl()
    { 
        try { 
            if (flags_ & f_open) close(); 
        } catch (...) { /* */ } 
    }

    template <class T>
    void open(const T& dev, std::streamsize buffer_size)
    {
        if (flags_ & f_open)
            boost::throw_exception(BOOST_IOSTREAMS_FAILURE("already open"));
        if (buffer_size == -1)
            buffer_size = default_filter_buffer_size;
        std::streamsize max_length = cvt_.get().max_length();
        buffer_size = (std::max)(buffer_size, 2 * max_length);
        if (can_read::value) {
            buf_.first().resize(buffer_size);
            buf_.first().set(0, 0);
        }
        if (can_write::value && !is_double::value) {
            buf_.second().resize(buffer_size);
            buf_.second().set(0, 0);
        }
        dev_.reset(concept_adapter<device_type>(dev));
        flags_ = f_open;
    }

    void close()
    {
        detail::execute_all(
            detail::call_member_close(*this, BOOST_IOS::in),
            detail::call_member_close(*this, BOOST_IOS::out)
        );
    }

    void close(BOOST_IOS::openmode which)
    {
        if (which == BOOST_IOS::in && (flags_ & f_input_closed) == 0) {
            flags_ |= f_input_closed;
            iostreams::close(dev(), BOOST_IOS::in);
        }
        if (which == BOOST_IOS::out && (flags_ & f_output_closed) == 0) {
            flags_ |= f_output_closed;
            detail::execute_all(
                detail::flush_buffer(buf_.second(), dev(), can_write::value),
                detail::call_close(dev(), BOOST_IOS::out),
                detail::call_reset(dev_),
                detail::call_reset(buf_.first()),
                detail::call_reset(buf_.second())
            );
        }
    }

    bool is_open() const { return (flags_ & f_open) != 0;}

    device_type& dev() { return **dev_; }

    enum flag_type {
        f_open             = 1,
        f_input_closed     = f_open << 1,
        f_output_closed    = f_input_closed << 1
    };

    codecvt_holder<Codecvt>  cvt_;
    storage_type             dev_;
    double_object<
        buffer_type, 
        is_double
    >                        buf_;
    int                      flags_;
};

} // End namespace detail.

//--------------Definition of converter---------------------------------------//

#define BOOST_IOSTREAMS_CONVERTER_PARAMS() , std::streamsize buffer_size = -1
#define BOOST_IOSTREAMS_CONVERTER_ARGS() , buffer_size

template<typename Device, typename Codecvt, typename Alloc>
struct code_converter_base {
    typedef detail::code_converter_impl<
                Device, Codecvt, Alloc
            > impl_type;
    code_converter_base() : pimpl_(new impl_type) { }
    shared_ptr<impl_type> pimpl_;
};

template< typename Device, 
          typename Codecvt = detail::default_codecvt, 
          typename Alloc = std::allocator<char> >
class code_converter 
    : protected code_converter_base<Device, Codecvt, Alloc>
{
private:
    typedef detail::code_converter_impl<
                Device, Codecvt, Alloc
            >                                                       impl_type;
    typedef typename impl_type::device_type                         device_type;
    typedef typename impl_type::buffer_type                         buffer_type;
    typedef typename detail::codecvt_holder<Codecvt>::codecvt_type  codecvt_type;
    typedef typename detail::codecvt_intern<Codecvt>::type          intern_type;
    typedef typename detail::codecvt_extern<Codecvt>::type          extern_type;
    typedef typename detail::codecvt_state<Codecvt>::type           state_type;
public:
    typedef intern_type                                             char_type;    
    struct category 
        : impl_type::mode, device_tag, closable_tag, localizable_tag
        { };
    BOOST_STATIC_ASSERT((
        is_same<
            extern_type, 
            BOOST_DEDUCED_TYPENAME char_type_of<Device>::type
        >::value
    ));
public:
    code_converter() { }
    BOOST_IOSTREAMS_FORWARD( code_converter, open_impl, Device,
                             BOOST_IOSTREAMS_CONVERTER_PARAMS, 
                             BOOST_IOSTREAMS_CONVERTER_ARGS )

        // fstream-like interface.

    bool is_open() const { return this->pimpl_->is_open(); }
    void close(BOOST_IOS::openmode which = BOOST_IOS::in | BOOST_IOS::out )
    { impl().close(which); }

        // Device interface.

    std::streamsize read(char_type*, std::streamsize);
    std::streamsize write(const char_type*, std::streamsize);
    void imbue(const std::locale& loc) { impl().cvt_.imbue(loc); }

        // Direct device access.

    Device& operator*() { return detail::unwrap_direct(dev()); }
    Device* operator->() { return &detail::unwrap_direct(dev()); }
private:
    template<typename T> // Used for forwarding.
    void open_impl(const T& t BOOST_IOSTREAMS_CONVERTER_PARAMS()) 
    { 
        impl().open(t BOOST_IOSTREAMS_CONVERTER_ARGS()); 
    }

    const codecvt_type& cvt() { return impl().cvt_.get(); }
    device_type& dev() { return impl().dev(); }
    buffer_type& in() { return impl().buf_.first(); }
    buffer_type& out() { return impl().buf_.second(); }
    impl_type& impl() { return *this->pimpl_; }
};

//--------------Implementation of converter-----------------------------------//

// Implementation note: if end of stream contains a partial character,
// it is ignored.
template<typename Device, typename Codevt, typename Alloc>
std::streamsize code_converter<Device, Codevt, Alloc>::read
    (char_type* s, std::streamsize n)
{
    const extern_type*   next;        // Next external char.
    intern_type*         nint;        // Next internal char.
    std::streamsize      total = 0;   // Characters read.
    int                  status = iostreams::char_traits<char>::good();
    bool                 partial = false;
    buffer_type&         buf = in();

    do {

        // Fill buffer.
        if (buf.ptr() == buf.eptr() || partial) {
            status = buf.fill(dev());
            if (buf.ptr() == buf.eptr())
                break;
            partial = false;
        }

        // Convert.
        std::codecvt_base::result result =
            cvt().in( buf.state(),
                      buf.ptr(), buf.eptr(), next,
                      s + total, s + n, nint );
        buf.ptr() += next - buf.ptr();
        total = static_cast<std::streamsize>(nint - s);

        switch (result) {
        case std::codecvt_base::partial:
            partial = true;
            break;
        case std::codecvt_base::ok:
            break;
        case std::codecvt_base::noconv:
            {
                std::streamsize amt = 
                    std::min<std::streamsize>(next - buf.ptr(), n - total);
                detail::strncpy_if_same(s + total, buf.ptr(), amt);
                total += amt;
            }
            break;
        case std::codecvt_base::error:
        default:
            buf.state() = state_type();
            boost::throw_exception(code_conversion_error());
        }

    } while (total < n && status != EOF && status != WOULD_BLOCK);

    return total == 0 && status == EOF ? -1 : total;
}

template<typename Device, typename Codevt, typename Alloc>
std::streamsize code_converter<Device, Codevt, Alloc>::write
    (const char_type* s, std::streamsize n)
{
    buffer_type&        buf = out();
    extern_type*        next;              // Next external char.
    const intern_type*  nint;              // Next internal char.
    std::streamsize     total = 0;         // Characters written.
    bool                partial = false;

    while (total < n) {

        // Empty buffer.
        if (buf.eptr() == buf.end() || partial) {
            if (!buf.flush(dev()))
                break;
            partial = false;
        }
       
        // Convert.
        std::codecvt_base::result result =
            cvt().out( buf.state(),
                       s + total, s + n, nint,
                       buf.eptr(), buf.end(), next );
        int progress = (int) (next - buf.eptr());
        buf.eptr() += progress;

        switch (result) {
        case std::codecvt_base::partial:
            partial = true;
            BOOST_FALLTHROUGH;
        case std::codecvt_base::ok:
            total = static_cast<std::streamsize>(nint - s);
            break;
        case std::codecvt_base::noconv:
            {
                std::streamsize amt = 
                    std::min<std::streamsize>( nint - total - s, 
                                               buf.end() - buf.eptr() );
                detail::strncpy_if_same(buf.eptr(), s + total, amt);
                total += amt;
            }
            break;
        case std::codecvt_base::error:
        default:
            buf.state() = state_type();
            boost::throw_exception(code_conversion_error());
        }
    }
    return total;
}

//----------------------------------------------------------------------------//

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp> // Borland 5.x

#endif // #ifndef BOOST_IOSTREAMS_CODE_CONVERTER_HPP_INCLUDED
