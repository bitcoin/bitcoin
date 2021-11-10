// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_DETAIL_BUFFERS_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_BUFFERS_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif              

#include <algorithm>                           // swap.
#include <memory>                              // allocator.
#include <boost/config.hpp>                    // member templates.
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/detail/ios.hpp>      // streamsize.
#include <boost/iostreams/read.hpp>
#include <boost/iostreams/traits.hpp>          // int_type_of.
#include <boost/iostreams/checked_operations.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace iostreams { namespace detail {

//----------------Buffers-----------------------------------------------------//

//
// Template name: buffer
// Description: Character buffer.
// Template parameters:
//     Ch - The character type.
//     Alloc - The Allocator type.
//
template< typename Ch,
          typename Alloc = std::allocator<Ch> >
class basic_buffer {
private:
#ifndef BOOST_NO_STD_ALLOCATOR
#if defined(BOOST_NO_CXX11_ALLOCATOR)
    typedef typename Alloc::template rebind<Ch>::other allocator_type;
#else
    typedef typename std::allocator_traits<Alloc>::template rebind_alloc<Ch> allocator_type;
    typedef std::allocator_traits<allocator_type> allocator_traits;
#endif
#else
    typedef std::allocator<Ch> allocator_type;
#endif
    static Ch* allocate(std::streamsize buffer_size);
public:
    basic_buffer();
    basic_buffer(std::streamsize buffer_size);
    ~basic_buffer();
    void resize(std::streamsize buffer_size);
    Ch* begin() const { return buf_; }
    Ch* end() const { return buf_ + size_; }
    Ch* data() const { return buf_; }
    std::streamsize size() const { return size_; }
    void swap(basic_buffer& rhs);
private:
    // Disallow copying and assignment.
    basic_buffer(const basic_buffer&);
    basic_buffer& operator=(const basic_buffer&);
    Ch*              buf_;
    std::streamsize  size_;
};

template<typename Ch, typename Alloc>
void swap(basic_buffer<Ch, Alloc>& lhs, basic_buffer<Ch, Alloc>& rhs)
{ lhs.swap(rhs); }

//
// Template name: buffer
// Description: Character buffer with two pointers accessible via ptr() and
//      eptr().
// Template parameters:
//     Ch - A character type.
//
template< typename Ch,
          typename Alloc = std::allocator<Ch> >
class buffer : public basic_buffer<Ch, Alloc> {
private:
    typedef basic_buffer<Ch, Alloc> base;
public:
    typedef iostreams::char_traits<Ch> traits_type;
    using base::resize; 
    using base::data; 
    using base::size;
    typedef Ch* const const_pointer;
    buffer(std::streamsize buffer_size);
    Ch* & ptr() { return ptr_; }
    const_pointer& ptr() const { return ptr_; }
    Ch* & eptr() { return eptr_; }
    const_pointer& eptr() const { return eptr_; }
    void set(std::streamsize ptr, std::streamsize end);
    void swap(buffer& rhs);

    // Returns an int_type as a status code.
    template<typename Source>
    typename int_type_of<Source>::type fill(Source& src) 
    {
        using namespace std;
        std::streamsize keep;
        if ((keep = static_cast<std::streamsize>(eptr_ - ptr_)) > 0)
            traits_type::move(
                this->data(),
                ptr_, 
                static_cast<size_t>(keep)
            );
        set(0, keep);
        std::streamsize result = 
            iostreams::read(src, this->data() + keep, this->size() - keep);
        if (result != -1)
            this->set(0, keep + result);
        return result == -1 ?
            traits_type::eof() :
                result == 0 ?
                    traits_type::would_block() :
                    traits_type::good();

    }

    // Returns true if one or more characters were written.
    template<typename Sink>
    bool flush(Sink& dest) 
    {
        using namespace std;
        std::streamsize amt = static_cast<std::streamsize>(eptr_ - ptr_);
        std::streamsize result = iostreams::write_if(dest, ptr_, amt);
        if (result < amt) {
            traits_type::move( this->data(), 
                               ptr_ + static_cast<size_t>(result), 
                               static_cast<size_t>(amt - result) );
        }
        this->set(0, amt - result);
        return result != 0;
    }
private:
    Ch *ptr_, *eptr_;
};

template<typename Ch, typename Alloc>
void swap(buffer<Ch, Alloc>& lhs, buffer<Ch, Alloc>& rhs)
{ lhs.swap(rhs); }

//--------------Implementation of basic_buffer--------------------------------//

template<typename Ch, typename Alloc>
basic_buffer<Ch, Alloc>::basic_buffer() : buf_(0), size_(0) { }

template<typename Ch, typename Alloc>
inline Ch* basic_buffer<Ch, Alloc>::allocate(std::streamsize buffer_size)
{
#if defined(BOOST_NO_CXX11_ALLOCATOR) || defined(BOOST_NO_STD_ALLOCATOR)
    return static_cast<Ch*>(allocator_type().allocate(
           static_cast<BOOST_DEDUCED_TYPENAME Alloc::size_type>(buffer_size), 0));
#else
    allocator_type alloc;
    return static_cast<Ch*>(allocator_traits::allocate(alloc,
           static_cast<BOOST_DEDUCED_TYPENAME allocator_traits::size_type>(buffer_size)));
#endif
}

template<typename Ch, typename Alloc>
basic_buffer<Ch, Alloc>::basic_buffer(std::streamsize buffer_size)
    : buf_(allocate(buffer_size)),
      size_(buffer_size) // Cast for SunPro 5.3.
    { }

template<typename Ch, typename Alloc>
inline basic_buffer<Ch, Alloc>::~basic_buffer()
{
    if (buf_) {
#if defined(BOOST_NO_CXX11_ALLOCATOR) || defined(BOOST_NO_STD_ALLOCATOR)
        allocator_type().deallocate(buf_,
            static_cast<BOOST_DEDUCED_TYPENAME Alloc::size_type>(size_));
#else
        allocator_type alloc;
        allocator_traits::deallocate(alloc, buf_,
            static_cast<BOOST_DEDUCED_TYPENAME allocator_traits::size_type>(size_));
#endif
    }
}

template<typename Ch, typename Alloc>
inline void basic_buffer<Ch, Alloc>::resize(std::streamsize buffer_size)
{
    if (size_ != buffer_size) {
        basic_buffer<Ch, Alloc> temp(buffer_size);
        std::swap(size_, temp.size_);
        std::swap(buf_, temp.buf_);
    }
}

template<typename Ch, typename Alloc>
void basic_buffer<Ch, Alloc>::swap(basic_buffer& rhs) 
{ 
    std::swap(buf_, rhs.buf_); 
    std::swap(size_, rhs.size_); 
}

//--------------Implementation of buffer--------------------------------------//

template<typename Ch, typename Alloc>
buffer<Ch, Alloc>::buffer(std::streamsize buffer_size)
    : basic_buffer<Ch, Alloc>(buffer_size), ptr_(data()), eptr_(data() + buffer_size) { }

template<typename Ch, typename Alloc>
inline void buffer<Ch, Alloc>::set(std::streamsize ptr, std::streamsize end)
{ 
    ptr_ = data() + ptr; 
    eptr_ = data() + end; 
}

template<typename Ch, typename Alloc>
inline void buffer<Ch, Alloc>::swap(buffer& rhs) 
{ 
    base::swap(rhs); 
    std::swap(ptr_, rhs.ptr_); 
    std::swap(eptr_, rhs.eptr_); 
}

//----------------------------------------------------------------------------//

} } } // End namespaces detail, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_BUFFERS_HPP_INCLUDED
