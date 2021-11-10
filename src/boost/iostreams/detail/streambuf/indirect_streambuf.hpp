// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
// See http://www.boost.org/libs/iostreams for documentation.

// This material is heavily indebted to the discussion and code samples in
// A. Langer and K. Kreft, "Standard C++ IOStreams and Locales",
// Addison-Wesley, 2000, pp. 228-43.

// User "GMSB" provided an optimization for small seeks.

#ifndef BOOST_IOSTREAMS_DETAIL_INDIRECT_STREAMBUF_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_INDIRECT_STREAMBUF_HPP_INCLUDED

#include <algorithm>                             // min, max.
#include <cassert>
#include <exception>
#include <boost/config.hpp>                      // Member template friends.
#include <boost/detail/workaround.hpp>
#include <boost/core/typeinfo.hpp>
#include <boost/iostreams/constants.hpp>
#include <boost/iostreams/detail/adapter/concept_adapter.hpp>
#include <boost/iostreams/detail/buffer.hpp>
#include <boost/iostreams/detail/config/wide_streams.hpp>
#include <boost/iostreams/detail/double_object.hpp> 
#include <boost/iostreams/detail/execute.hpp>
#include <boost/iostreams/detail/functional.hpp>
#include <boost/iostreams/detail/ios.hpp>
#include <boost/iostreams/detail/optional.hpp>
#include <boost/iostreams/detail/push.hpp>
#include <boost/iostreams/detail/streambuf/linked_streambuf.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/traits.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/mpl/if.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_convertible.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp>  // MSVC, BCC 5.x

namespace boost { namespace iostreams { namespace detail {

//
// Description: The implementation of basic_streambuf used by chains.
//
template<typename T, typename Tr, typename Alloc, typename Mode>
class indirect_streambuf
    : public linked_streambuf<BOOST_DEDUCED_TYPENAME char_type_of<T>::type, Tr>
{
public:
    typedef typename char_type_of<T>::type                    char_type;
    BOOST_IOSTREAMS_STREAMBUF_TYPEDEFS(Tr)
private:
    typedef typename category_of<T>::type                     category;
    typedef concept_adapter<T>                                wrapper;
    typedef detail::basic_buffer<char_type, Alloc>            buffer_type;
    typedef indirect_streambuf<T, Tr, Alloc, Mode>            my_type;
    typedef detail::linked_streambuf<char_type, traits_type>  base_type;
    typedef linked_streambuf<char_type, Tr>                   streambuf_type;
public:
    indirect_streambuf();

    void open(const T& t BOOST_IOSTREAMS_PUSH_PARAMS());
    bool is_open() const;
    void close();
    bool auto_close() const;
    void set_auto_close(bool close);
    bool strict_sync();

    // Declared in linked_streambuf.
    T* component() { return &*obj(); }
protected:
    BOOST_IOSTREAMS_USING_PROTECTED_STREAMBUF_MEMBERS(base_type)

    //----------virtual functions---------------------------------------------//

#ifndef BOOST_IOSTREAMS_NO_LOCALE
    void imbue(const std::locale& loc);
#endif
#ifdef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES
    public:
#endif
    int_type underflow();
    int_type pbackfail(int_type c);
    int_type overflow(int_type c);
    int sync();
    pos_type seekoff( off_type off, BOOST_IOS::seekdir way,
                      BOOST_IOS::openmode which );
    pos_type seekpos(pos_type sp, BOOST_IOS::openmode which);

    // Declared in linked_streambuf.
    void set_next(streambuf_type* next);
    void close_impl(BOOST_IOS::openmode m);
    const boost::core::typeinfo& component_type() const { return BOOST_CORE_TYPEID(T); }
    void* component_impl() { return component(); }
private:

    //----------Accessor functions--------------------------------------------//

    wrapper& obj() { return *storage_; }
    streambuf_type* next() const { return next_; }
    buffer_type& in() { return buffer_.first(); }
    buffer_type& out() { return buffer_.second(); }
    bool can_read() const { return is_convertible<Mode, input>::value; }
    bool can_write() const { return is_convertible<Mode, output>::value; }
    bool output_buffered() const { return (flags_ & f_output_buffered) != 0; }
    bool shared_buffer() const { return is_convertible<Mode, seekable>::value || is_convertible<Mode, dual_seekable>::value; }
    void set_flags(int f) { flags_ = f; }

    //----------State changing functions--------------------------------------//

    virtual void init_get_area();
    virtual void init_put_area();

    //----------Utility function----------------------------------------------//

    pos_type seek_impl( stream_offset off, BOOST_IOS::seekdir way,
                        BOOST_IOS::openmode which );
    void sync_impl();

    enum flag_type {
        f_open             = 1,
        f_output_buffered  = f_open << 1,
        f_auto_close       = f_output_buffered << 1
    };

    optional<wrapper>           storage_;
    streambuf_type*             next_;
    double_object<
        buffer_type,
        is_convertible<
            Mode,
            two_sequence
        >
    >                           buffer_;
    std::streamsize             pback_size_;
    int                         flags_;
};

//--------------Implementation of indirect_streambuf--------------------------//

template<typename T, typename Tr, typename Alloc, typename Mode>
indirect_streambuf<T, Tr, Alloc, Mode>::indirect_streambuf()
    : next_(0), pback_size_(0), flags_(f_auto_close) { }

//--------------Implementation of open, is_open and close---------------------//

template<typename T, typename Tr, typename Alloc, typename Mode>
void indirect_streambuf<T, Tr, Alloc, Mode>::open
    (const T& t, std::streamsize buffer_size, std::streamsize pback_size)
{
    using namespace std;

    // Normalize buffer sizes.
    buffer_size =
        (buffer_size != -1) ?
        buffer_size :
        iostreams::optimal_buffer_size(t);
    pback_size =
        (pback_size != -1) ?
        pback_size :
        default_pback_buffer_size;

    // Construct input buffer.
    if (can_read()) {
        pback_size_ = (std::max)(std::streamsize(2), pback_size); // STLPort needs 2.
        std::streamsize size =
            pback_size_ +
            ( buffer_size ? buffer_size: std::streamsize(1) );
        in().resize(static_cast<int>(size));
        if (!shared_buffer())
            init_get_area();
    }

    // Construct output buffer.
    if (can_write() && !shared_buffer()) {
        if (buffer_size != std::streamsize(0))
            out().resize(static_cast<int>(buffer_size));
        init_put_area();
    }

    storage_.reset(wrapper(t));
    flags_ |= f_open;
    if (can_write() && buffer_size > 1)
        flags_ |= f_output_buffered;
    this->set_true_eof(false);
    this->set_needs_close();
}

template<typename T, typename Tr, typename Alloc, typename Mode>
inline bool indirect_streambuf<T, Tr, Alloc, Mode>::is_open() const
{ return (flags_ & f_open) != 0; }

template<typename T, typename Tr, typename Alloc, typename Mode>
void indirect_streambuf<T, Tr, Alloc, Mode>::close()
{
    using namespace std;
    base_type* self = this;
    detail::execute_all(
        detail::call_member_close(*self, BOOST_IOS::in),
        detail::call_member_close(*self, BOOST_IOS::out),
        detail::call_reset(storage_),
        detail::clear_flags(flags_)
    );
}

template<typename T, typename Tr, typename Alloc, typename Mode>
bool indirect_streambuf<T, Tr, Alloc, Mode>::auto_close() const
{ return (flags_ & f_auto_close) != 0; }

template<typename T, typename Tr, typename Alloc, typename Mode>
void indirect_streambuf<T, Tr, Alloc, Mode>::set_auto_close(bool close)
{ flags_ = (flags_ & ~f_auto_close) | (close ? f_auto_close : 0); }

//--------------Implementation virtual functions------------------------------//

#ifndef BOOST_IOSTREAMS_NO_LOCALE
template<typename T, typename Tr, typename Alloc, typename Mode>
void indirect_streambuf<T, Tr, Alloc, Mode>::imbue(const std::locale& loc)
{
    if (is_open()) {
        obj().imbue(loc);
        if (next_)
            next_->pubimbue(loc);
    }
}
#endif

template<typename T, typename Tr, typename Alloc, typename Mode>
typename indirect_streambuf<T, Tr, Alloc, Mode>::int_type
indirect_streambuf<T, Tr, Alloc, Mode>::underflow()
{
    using namespace std;
    if (!gptr()) init_get_area();
    buffer_type& buf = in();
    if (gptr() < egptr()) return traits_type::to_int_type(*gptr());

    // Fill putback buffer.
    std::streamsize keep = 
        (std::min)( static_cast<std::streamsize>(gptr() - eback()),
                    pback_size_ );
    if (keep)
        traits_type::move( buf.data() + (pback_size_ - keep),
                           gptr() - keep, keep );

    // Set pointers to reasonable values in case read throws.
    setg( buf.data() + pback_size_ - keep,
          buf.data() + pback_size_,
          buf.data() + pback_size_ );

    // Read from source.
    std::streamsize chars =
        obj().read(buf.data() + pback_size_, buf.size() - pback_size_, next_);
    if (chars == -1) {
        this->set_true_eof(true);
        chars = 0;
    }
    setg(eback(), gptr(), buf.data() + pback_size_ + chars);
    return chars != 0 ?
        traits_type::to_int_type(*gptr()) :
        traits_type::eof();
}

template<typename T, typename Tr, typename Alloc, typename Mode>
typename indirect_streambuf<T, Tr, Alloc, Mode>::int_type
indirect_streambuf<T, Tr, Alloc, Mode>::pbackfail(int_type c)
{
    if (gptr() != eback()) {
        gbump(-1);
        if (!traits_type::eq_int_type(c, traits_type::eof()))
            *gptr() = traits_type::to_char_type(c);
        return traits_type::not_eof(c);
    } else {
        boost::throw_exception(bad_putback());
    }
}

template<typename T, typename Tr, typename Alloc, typename Mode>
typename indirect_streambuf<T, Tr, Alloc, Mode>::int_type
indirect_streambuf<T, Tr, Alloc, Mode>::overflow(int_type c)
{
    if ( (output_buffered() && pptr() == 0) ||
         (shared_buffer() && gptr() != 0) )
    {
        init_put_area();
    }
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
        if (output_buffered()) {
            if (pptr() == epptr()) {
                sync_impl();
                if (pptr() == epptr())
                    return traits_type::eof();
            }
            *pptr() = traits_type::to_char_type(c);
            pbump(1);
        } else {
            char_type d = traits_type::to_char_type(c);
            if (obj().write(&d, 1, next_) != 1)
                return traits_type::eof();
        }
    }
    return traits_type::not_eof(c);
}

template<typename T, typename Tr, typename Alloc, typename Mode>
int indirect_streambuf<T, Tr, Alloc, Mode>::sync()
{
    try { // sync() is no-throw.
        sync_impl();
        obj().flush(next_);
        return 0;
    } catch (...) { return -1; }
}

template<typename T, typename Tr, typename Alloc, typename Mode>
bool indirect_streambuf<T, Tr, Alloc, Mode>::strict_sync()
{
    try { // sync() is no-throw.
        sync_impl();
        return obj().flush(next_);
    } catch (...) { return false; }
}

template<typename T, typename Tr, typename Alloc, typename Mode>
inline typename indirect_streambuf<T, Tr, Alloc, Mode>::pos_type
indirect_streambuf<T, Tr, Alloc, Mode>::seekoff
    (off_type off, BOOST_IOS::seekdir way, BOOST_IOS::openmode which)
{ return seek_impl(off, way, which); }

template<typename T, typename Tr, typename Alloc, typename Mode>
inline typename indirect_streambuf<T, Tr, Alloc, Mode>::pos_type
indirect_streambuf<T, Tr, Alloc, Mode>::seekpos
    (pos_type sp, BOOST_IOS::openmode which)
{ 
    return seek_impl(position_to_offset(sp), BOOST_IOS::beg, which); 
}

template<typename T, typename Tr, typename Alloc, typename Mode>
typename indirect_streambuf<T, Tr, Alloc, Mode>::pos_type
indirect_streambuf<T, Tr, Alloc, Mode>::seek_impl
    (stream_offset off, BOOST_IOS::seekdir way, BOOST_IOS::openmode which)
{
    if ( gptr() != 0 && way == BOOST_IOS::cur && which == BOOST_IOS::in && 
         eback() - gptr() <= off && off <= egptr() - gptr() ) 
    {   // Small seek optimization
        gbump(static_cast<int>(off));
        return obj().seek(stream_offset(0), BOOST_IOS::cur, BOOST_IOS::in, next_) -
               static_cast<off_type>(egptr() - gptr());
    }
    if (pptr() != 0) 
        this->BOOST_IOSTREAMS_PUBSYNC(); // sync() confuses VisualAge 6.
    if (way == BOOST_IOS::cur && gptr())
        off -= static_cast<off_type>(egptr() - gptr());
    bool two_head = is_convertible<category, dual_seekable>::value ||
                    is_convertible<category, bidirectional_seekable>::value;
    if (two_head) {
        BOOST_IOS::openmode both = BOOST_IOS::in | BOOST_IOS::out;
        if ((which & both) == both)
            boost::throw_exception(bad_seek());
        if (which & BOOST_IOS::in) {
            setg(0, 0, 0);
        }
        if (which & BOOST_IOS::out) {
            setp(0, 0);
        }
    }
    else {
        setg(0, 0, 0);
        setp(0, 0);
    }
    return obj().seek(off, way, which, next_);
}

template<typename T, typename Tr, typename Alloc, typename Mode>
inline void indirect_streambuf<T, Tr, Alloc, Mode>::set_next
    (streambuf_type* next)
{ next_ = next; }

template<typename T, typename Tr, typename Alloc, typename Mode>
inline void indirect_streambuf<T, Tr, Alloc, Mode>::close_impl
    (BOOST_IOS::openmode which)
{
    if (which == BOOST_IOS::in && is_convertible<Mode, input>::value) {
        setg(0, 0, 0);
    }
    if (which == BOOST_IOS::out && is_convertible<Mode, output>::value) {
        sync();
        setp(0, 0);
    }
    #if defined(BOOST_MSVC)
      #pragma warning(push)
      #pragma warning(disable: 4127) // conditional expression is constant
    #endif
    if ( !is_convertible<category, dual_use>::value ||
         is_convertible<Mode, input>::value == (which == BOOST_IOS::in) )
    {
        obj().close(which, next_);
    }
    #if defined(BOOST_MSVC)
      #pragma warning(pop)
    #endif
}

//----------State changing functions------------------------------------------//

template<typename T, typename Tr, typename Alloc, typename Mode>
void indirect_streambuf<T, Tr, Alloc, Mode>::sync_impl()
{
    std::streamsize avail, amt;
    if ((avail = static_cast<std::streamsize>(pptr() - pbase())) > 0) {
        if ((amt = obj().write(pbase(), avail, next())) == avail)
            setp(out().begin(), out().end());
        else {
            const char_type* ptr = pptr();
            setp(out().begin() + amt, out().end());
            pbump(static_cast<int>(ptr - pptr()));
        }
    }
}

template<typename T, typename Tr, typename Alloc, typename Mode>
void indirect_streambuf<T, Tr, Alloc, Mode>::init_get_area()
{
    if (shared_buffer() && pptr() != 0) {
        sync_impl();
        setp(0, 0);
    }
    setg(in().begin(), in().begin(), in().begin());
}

template<typename T, typename Tr, typename Alloc, typename Mode>
void indirect_streambuf<T, Tr, Alloc, Mode>::init_put_area()
{
    using namespace std;
    if (shared_buffer() && gptr() != 0) {
        obj().seek(static_cast<off_type>(gptr() - egptr()), BOOST_IOS::cur, BOOST_IOS::in, next_);
        setg(0, 0, 0);
    }
    if (output_buffered())
        setp(out().begin(), out().end());
    else
        setp(0, 0);
}

//----------------------------------------------------------------------------//

} } } // End namespaces detail, iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp> // MSVC, BCC 5.x

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_INDIRECT_STREAMBUF_HPP_INCLUDED
