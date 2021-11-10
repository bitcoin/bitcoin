// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Contains the definitions of the class templates symmetric_filter,
// which models DualUseFilter based on a model of the Symmetric Filter.

//
// Roughly, a Symmetric Filter is a class type with the following interface:
//
//   struct symmetric_filter {
//       typedef xxx char_type;
//
//       bool filter( const char*& begin_in, const char* end_in,
//                    char*& begin_out, char* end_out, bool flush )
//       {
//          // Consume as many characters as possible from the interval
//          // [begin_in, end_in), without exhausting the output range
//          // [begin_out, end_out). If flush is true, write as mush output
//          // as possible. 
//          // A return value of true indicates that filter should be called 
//          // again. More precisely, if flush is false, a return value of 
//          // false indicates that the natural end of stream has been reached
//          // and that all filtered data has been forwarded; if flush is
//          // true, a return value of false indicates that all filtered data 
//          // has been forwarded.
//       }
//       void close() { /* Reset filter's state. */ }
//   };
//
// Symmetric Filter filters need not be CopyConstructable.
//

#ifndef BOOST_IOSTREAMS_SYMMETRIC_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_SYMMETRIC_FILTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assert.hpp>
#include <memory>                               // allocator.
#include <boost/config.hpp>                     // BOOST_DEDUCED_TYPENAME.
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/constants.hpp>        // buffer size.
#include <boost/iostreams/detail/buffer.hpp>
#include <boost/iostreams/detail/char_traits.hpp>
#include <boost/iostreams/detail/config/limits.hpp>
#include <boost/iostreams/detail/ios.hpp>  // streamsize.
#include <boost/iostreams/detail/template_params.hpp>
#include <boost/iostreams/traits.hpp>
#include <boost/iostreams/operations.hpp>       // read, write.
#include <boost/iostreams/pipeline.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/shared_ptr.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp>  // MSVC.

namespace boost { namespace iostreams {

template< typename SymmetricFilter,
          typename Alloc =
              std::allocator<
                  BOOST_DEDUCED_TYPENAME char_type_of<SymmetricFilter>::type
              > >
class symmetric_filter {
public:
    typedef typename char_type_of<SymmetricFilter>::type      char_type;
    typedef BOOST_IOSTREAMS_CHAR_TRAITS(char_type)            traits_type;
    typedef std::basic_string<char_type, traits_type, Alloc>  string_type;
    struct category
        : dual_use,
          filter_tag,
          multichar_tag,
          closable_tag
        { };

    // Expands to a sequence of ctors which forward to impl.
    #define BOOST_PP_LOCAL_MACRO(n) \
        BOOST_IOSTREAMS_TEMPLATE_PARAMS(n, T) \
        explicit symmetric_filter( \
              std::streamsize buffer_size BOOST_PP_COMMA_IF(n) \
              BOOST_PP_ENUM_BINARY_PARAMS(n, const T, &t) ) \
            : pimpl_(new impl(buffer_size BOOST_PP_COMMA_IF(n) \
                     BOOST_PP_ENUM_PARAMS(n, t))) \
            { BOOST_ASSERT(buffer_size > 0); } \
        /**/
    #define BOOST_PP_LOCAL_LIMITS (0, BOOST_IOSTREAMS_MAX_FORWARDING_ARITY)
    #include BOOST_PP_LOCAL_ITERATE()
    #undef BOOST_PP_LOCAL_MACRO

    template<typename Source>
    std::streamsize read(Source& src, char_type* s, std::streamsize n)
    {
        using namespace std;
        if (!(state() & f_read))
            begin_read();

        buffer_type&  buf = pimpl_->buf_;
        int           status = (state() & f_eof) != 0 ? f_eof : f_good;
        char_type    *next_s = s,
                     *end_s = s + n;
        while (true)
        {
            // Invoke filter if there are unconsumed characters in buffer or if
            // filter must be flushed.
            bool flush = status == f_eof;
            if (buf.ptr() != buf.eptr() || flush) {
                const char_type* next = buf.ptr();
                bool done =
                    !filter().filter(next, buf.eptr(), next_s, end_s, flush);
                buf.ptr() = buf.data() + (next - buf.data());
                if (done)
                    return detail::check_eof(
                               static_cast<std::streamsize>(next_s - s)
                           );
            }

            // If no more characters are available without blocking, or
            // if read request has been satisfied, return.
            if ( (status == f_would_block && buf.ptr() == buf.eptr()) ||
                 next_s == end_s )
            {
                return static_cast<std::streamsize>(next_s - s);
            }

            // Fill buffer.
            if (status == f_good)
                status = fill(src);
        }
    }

    template<typename Sink>
    std::streamsize write(Sink& snk, const char_type* s, std::streamsize n)
    {
        if (!(state() & f_write))
            begin_write();

        buffer_type&     buf = pimpl_->buf_;
        const char_type *next_s, *end_s;
        for (next_s = s, end_s = s + n; next_s != end_s; ) {
            if (buf.ptr() == buf.eptr() && !flush(snk))
                break;
            if(!filter().filter(next_s, end_s, buf.ptr(), buf.eptr(), false)) {
                flush(snk);
                break;
            }
        }
        return static_cast<std::streamsize>(next_s - s);
    }

    template<typename Sink>
    void close(Sink& snk, BOOST_IOS::openmode mode)
    {
        if (mode == BOOST_IOS::out) {

            if (!(state() & f_write))
                begin_write();

            // Repeatedly invoke filter() with no input.
            try {
                buffer_type&     buf = pimpl_->buf_;
                char_type        dummy;
                const char_type* end = &dummy;
                bool             again = true;
                while (again) {
                    if (buf.ptr() != buf.eptr())
                        again = filter().filter( end, end, buf.ptr(),
                                                 buf.eptr(), true );
                    flush(snk);
                }
            } catch (...) {
                try { close_impl(); } catch (...) { }
                throw;
            }
            close_impl();
        } else {
            close_impl();
        }
    }
    SymmetricFilter& filter() { return *pimpl_; }
    string_type unconsumed_input() const;

// Give impl access to buffer_type on Tru64
#if !BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042)) 
    private:
#endif
    typedef detail::buffer<char_type, Alloc> buffer_type;
private:
    buffer_type& buf() { return pimpl_->buf_; }
    const buffer_type& buf() const { return pimpl_->buf_; }
    int& state() { return pimpl_->state_; }
    void begin_read();
    void begin_write();

    template<typename Source>
    int fill(Source& src)
    {
        std::streamsize amt = iostreams::read(src, buf().data(), buf().size());
        if (amt == -1) {
            state() |= f_eof;
            return f_eof;
        }
        buf().set(0, amt);
        return amt != 0 ? f_good : f_would_block;
    }

    // Attempts to write the contents of the buffer the given Sink.
    // Returns true if at least on character was written.
    template<typename Sink>
    bool flush(Sink& snk)
    {
        typedef typename iostreams::category_of<Sink>::type  category;
        typedef is_convertible<category, output>             can_write;
        return flush(snk, can_write());
    }

    template<typename Sink>
    bool flush(Sink& snk, mpl::true_)
    {
        std::streamsize amt =
            static_cast<std::streamsize>(buf().ptr() - buf().data());
        std::streamsize result =
            boost::iostreams::write(snk, buf().data(), amt);
        if (result < amt && result > 0)
            traits_type::move(buf().data(), buf().data() + result, amt - result);
        buf().set(amt - result, buf().size());
        return result != 0;
    }

    template<typename Sink>
    bool flush(Sink&, mpl::false_) { return true;}

    void close_impl();

    enum flag_type {
        f_read   = 1,
        f_write  = f_read << 1,
        f_eof    = f_write << 1,
        f_good,
        f_would_block
    };

    struct impl : SymmetricFilter {

    // Expands to a sequence of ctors which forward to SymmetricFilter.
    #define BOOST_PP_LOCAL_MACRO(n) \
        BOOST_IOSTREAMS_TEMPLATE_PARAMS(n, T) \
        impl( std::streamsize buffer_size BOOST_PP_COMMA_IF(n) \
              BOOST_PP_ENUM_BINARY_PARAMS(n, const T, &t) ) \
            : SymmetricFilter(BOOST_PP_ENUM_PARAMS(n, t)), \
              buf_(buffer_size), state_(0) \
            { } \
        /**/
    #define BOOST_PP_LOCAL_LIMITS (0, BOOST_IOSTREAMS_MAX_FORWARDING_ARITY)
    #include BOOST_PP_LOCAL_ITERATE()
    #undef BOOST_PP_LOCAL_MACRO

        buffer_type  buf_;
        int          state_;
    };

    shared_ptr<impl> pimpl_;
};
BOOST_IOSTREAMS_PIPABLE(symmetric_filter, 2)

//------------------Implementation of symmetric_filter----------------//

template<typename SymmetricFilter, typename Alloc>
void symmetric_filter<SymmetricFilter, Alloc>::begin_read()
{
    BOOST_ASSERT(!(state() & f_write));
    state() |= f_read;
    buf().set(0, 0);
}

template<typename SymmetricFilter, typename Alloc>
void symmetric_filter<SymmetricFilter, Alloc>::begin_write()
{
    BOOST_ASSERT(!(state() & f_read));
    state() |= f_write;
    buf().set(0, buf().size());
}

template<typename SymmetricFilter, typename Alloc>
void symmetric_filter<SymmetricFilter, Alloc>::close_impl()
{
    state() = 0;
    buf().set(0, 0);
    filter().close();
}

template<typename SymmetricFilter, typename Alloc>
typename symmetric_filter<SymmetricFilter, Alloc>::string_type
symmetric_filter<SymmetricFilter, Alloc>::unconsumed_input() const
{ return string_type(buf().ptr(), buf().eptr()); }

//----------------------------------------------------------------------------//

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>  // MSVC.

#endif // #ifndef BOOST_IOSTREAMS_SYMMETRIC_FILTER_HPP_INCLUDED
