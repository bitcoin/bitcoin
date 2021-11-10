// ----------------------------------------------------------------------------
//  alt_sstream_impl.hpp : alternative stringstream, templates implementation 
// ----------------------------------------------------------------------------

//  Copyright Samuel Krempp 2003. Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/format for library home page

// ----------------------------------------------------------------------------

#ifndef BOOST_SK_ALT_SSTREAM_IMPL_HPP
#define BOOST_SK_ALT_SSTREAM_IMPL_HPP

namespace boost {
    namespace io {
// --- Implementation  ------------------------------------------------------//

        template<class Ch, class Tr, class Alloc>
        void basic_altstringbuf<Ch, Tr, Alloc>:: 
        clear_buffer () {
            const Ch * p = pptr();
            const Ch * b = pbase();
            if(p != NULL && p != b) {
                seekpos(0, ::std::ios_base::out); 
            }
            p = gptr();
            b = eback();
            if(p != NULL && p != b) {
                seekpos(0, ::std::ios_base::in); 
            }
        }

        template<class Ch, class Tr, class Alloc>
        void basic_altstringbuf<Ch, Tr, Alloc>:: 
        str (const string_type& s) {
            size_type sz=s.size();
            if(sz != 0 && mode_ & (::std::ios_base::in | ::std::ios_base::out) ) {
#ifdef _RWSTD_NO_CLASS_PARTIAL_SPEC
                void *vd_ptr = boost::allocator_allocate(alloc_, sz, is_allocated_? eback() : 0);
                Ch *new_ptr = static_cast<Ch *>(vd_ptr);
#else
                Ch *new_ptr = boost::allocator_allocate(alloc_, sz, is_allocated_? eback() : 0);
#endif
                // if this didnt throw, we're safe, update the buffer
                dealloc();
                sz = s.copy(new_ptr, sz);
                putend_ = new_ptr + sz;
                if(mode_ & ::std::ios_base::in)
                    streambuf_t::setg(new_ptr, new_ptr, new_ptr + sz);
                if(mode_ & ::std::ios_base::out) {
                    streambuf_t::setp(new_ptr, new_ptr + sz);
                    if(mode_ & (::std::ios_base::app | ::std::ios_base::ate))
                        streambuf_t::pbump(static_cast<int>(sz));
                    if(gptr() == NULL)
                        streambuf_t::setg(new_ptr, NULL, new_ptr);
                }
                is_allocated_ = true;
            }
            else 
                dealloc();
        }
        template<class Ch, class Tr, class Alloc>
        Ch*   basic_altstringbuf<Ch, Tr, Alloc>:: 
        begin () const {
            if(mode_ & ::std::ios_base::out && pptr() != NULL)
                return pbase();
            else if(mode_ & ::std::ios_base::in && gptr() != NULL)
                return eback();
            return NULL;
        }

        template<class Ch, class Tr, class Alloc>
        typename std::basic_string<Ch,Tr,Alloc>::size_type
        basic_altstringbuf<Ch, Tr, Alloc>:: 
        size () const { 
            if(mode_ & ::std::ios_base::out && pptr())
                return static_cast<size_type>(pend() - pbase());
            else if(mode_ & ::std::ios_base::in && gptr())
                return static_cast<size_type>(egptr() - eback());
            else 
                return 0;
        }

        template<class Ch, class Tr, class Alloc>
        typename std::basic_string<Ch,Tr,Alloc>::size_type
        basic_altstringbuf<Ch, Tr, Alloc>:: 
        cur_size () const { 
            if(mode_ & ::std::ios_base::out && pptr())
                return static_cast<streamsize>( pptr() - pbase());
            else if(mode_ & ::std::ios_base::in && gptr())
                return static_cast<streamsize>( gptr() - eback());
            else 
                return 0;
        }

        template<class Ch, class Tr, class Alloc>
        typename basic_altstringbuf<Ch, Tr, Alloc>::pos_type  
        basic_altstringbuf<Ch, Tr, Alloc>:: 
        seekoff (off_type off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which) {
            if(pptr() != NULL && putend_ < pptr())
                putend_ = pptr();
            if(which & ::std::ios_base::in && gptr() != NULL) {
                // get area
                if(way == ::std::ios_base::end)
                    off += static_cast<off_type>(putend_ - gptr());
                else if(way == ::std::ios_base::beg)
                    off += static_cast<off_type>(eback() - gptr());
                else if(way != ::std::ios_base::cur || (which & ::std::ios_base::out) )
                    // (altering in&out is only supported if way is beg or end, not cur)
                    return pos_type(off_type(-1));
                if(eback() <= off+gptr() && off+gptr() <= putend_ ) {
                    // set gptr
                    streambuf_t::gbump(static_cast<int>(off));
                    if(which & ::std::ios_base::out && pptr() != NULL)
                        // update pptr to match gptr
                        streambuf_t::pbump(static_cast<int>(gptr()-pptr()));
                }
                else
                    off = off_type(-1);
            }
            else if(which & ::std::ios_base::out && pptr() != NULL) {
                // put area
                if(way == ::std::ios_base::end)
                    off += static_cast<off_type>(putend_ - pptr());
                else if(way == ::std::ios_base::beg)
                    off += static_cast<off_type>(pbase() - pptr());
                else if(way != ::std::ios_base::beg)
                    return pos_type(off_type(-1));                    
                if(pbase() <= off+pptr() && off+pptr() <= putend_)
                    // set pptr
                    streambuf_t::pbump(static_cast<int>(off)); 
                else
                    off = off_type(-1);
            }
            else // neither in nor out
                off = off_type(-1);
            return (pos_type(off));
        }
        //- end seekoff(..)

        
        template<class Ch, class Tr, class Alloc>
        typename basic_altstringbuf<Ch, Tr, Alloc>::pos_type 
        basic_altstringbuf<Ch, Tr, Alloc>:: 
        seekpos (pos_type pos, ::std::ios_base::openmode which) {
            off_type off = off_type(pos); // operation guaranteed by fpos.operations table 127
            if(pptr() != NULL && putend_ < pptr())
                putend_ = pptr();
            if(off != off_type(-1)) {
                if(which & ::std::ios_base::in && gptr() != NULL) {
                    // get area
                    if(0 <= off && off <= putend_ - eback()) {
                        streambuf_t::gbump(static_cast<int>(eback() - gptr() + off));
                        if(which & ::std::ios_base::out && pptr() != NULL) {
                            // update pptr to match gptr
                            streambuf_t::pbump(static_cast<int>(gptr()-pptr()));
                        }
                    }
                    else
                        off = off_type(-1);
                }
                else if(which & ::std::ios_base::out && pptr() != NULL) {
                    // put area
                    if(0 <= off && off <= putend_ - eback())
                        streambuf_t::pbump(static_cast<int>(eback() - pptr() + off));
                    else
                        off = off_type(-1);
                }
                else // neither in nor out
                    off = off_type(-1);
                return (pos_type(off));
            }
            else {
                BOOST_ASSERT(0); // fpos.operations allows undefined-behaviour here
                return pos_type(off_type(-1));
            }
        }
        // -end seekpos(..)


        template<class Ch, class Tr, class Alloc>
        typename basic_altstringbuf<Ch, Tr, Alloc>::int_type
        basic_altstringbuf<Ch, Tr, Alloc>:: 
        underflow () {
            if(gptr() == NULL) // no get area -> nothing to get.
                return (compat_traits_type::eof()); 
            else if(gptr() < egptr())  // ok, in buffer
                return (compat_traits_type::to_int_type(*gptr())); 
            else if(mode_ & ::std::ios_base::in && pptr() != NULL
                    && (gptr() < pptr() || gptr() < putend_) )
                {  // expand get area 
                    if(putend_ < pptr()) 
                        putend_ = pptr(); // remember pptr reached this far
                    streambuf_t::setg(eback(), gptr(), putend_);
                    return (compat_traits_type::to_int_type(*gptr()));
                }
            else // couldnt get anything. EOF.
                return (compat_traits_type::eof());
        }
        // -end underflow(..)


        template<class Ch, class Tr, class Alloc>
        typename basic_altstringbuf<Ch, Tr, Alloc>::int_type 
        basic_altstringbuf<Ch, Tr, Alloc>:: 
        pbackfail (int_type meta) {
            if(gptr() != NULL  &&  (eback() < gptr()) 
               && (mode_ & (::std::ios_base::out)
                   || compat_traits_type::eq_int_type(compat_traits_type::eof(), meta)
                   || compat_traits_type::eq(compat_traits_type::to_char_type(meta), gptr()[-1]) ) ) { 
                streambuf_t::gbump(-1); // back one character
                if(!compat_traits_type::eq_int_type(compat_traits_type::eof(), meta))
                    //  put-back meta into get area
                    *gptr() = compat_traits_type::to_char_type(meta);
                return (compat_traits_type::not_eof(meta));
            }
            else
                return (compat_traits_type::eof());  // failed putback
        }
        // -end pbackfail(..)


        template<class Ch, class Tr, class Alloc>
        typename basic_altstringbuf<Ch, Tr, Alloc>::int_type 
        basic_altstringbuf<Ch, Tr, Alloc>:: 
        overflow (int_type meta) {
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4996)
#endif
            if(compat_traits_type::eq_int_type(compat_traits_type::eof(), meta))
                return compat_traits_type::not_eof(meta); // nothing to do
            else if(pptr() != NULL && pptr() < epptr()) {
                streambuf_t::sputc(compat_traits_type::to_char_type(meta));
                return meta;
            }
            else if(! (mode_ & ::std::ios_base::out)) 
                // no write position, and cant make one
                return compat_traits_type::eof(); 
            else { // make a write position available
                std::size_t prev_size = pptr() == NULL ? 0 : epptr() - eback();
                std::size_t new_size = prev_size;
                // exponential growth : size *= 1.5
                std::size_t add_size = new_size / 2;
                if(add_size < alloc_min)
                    add_size = alloc_min;
                Ch * newptr = NULL,  *oldptr = eback();

                // make sure adding add_size wont overflow size_t
                while (0 < add_size && ((std::numeric_limits<std::size_t>::max)()
                                        - add_size < new_size) )
                    add_size /= 2;
                if(0 < add_size) {
                    new_size += add_size;
#ifdef _RWSTD_NO_CLASS_PARTIAL_SPEC
                    void *vdptr = boost::allocator_allocate(alloc_, new_size, is_allocated_? oldptr : 0);
                    newptr = static_cast<Ch *>(vdptr);
#else
                    newptr = boost::allocator_allocate(alloc_, new_size, is_allocated_? oldptr : 0);
#endif
                }

                if(0 < prev_size)
                    compat_traits_type::copy(newptr, oldptr, prev_size);
                if(is_allocated_)
                    alloc_.deallocate(oldptr, prev_size);
                is_allocated_=true;

                if(prev_size == 0) { // first allocation
                    putend_ = newptr;
                    streambuf_t::setp(newptr, newptr + new_size);
                    if(mode_ & ::std::ios_base::in)
                        streambuf_t::setg(newptr, newptr, newptr + 1);
                    else
                        streambuf_t::setg(newptr, 0, newptr);
                }
                else { // update pointers
                    putend_ = putend_ - oldptr + newptr;
                    int pptr_count = static_cast<int>(pptr()-pbase());
                    int gptr_count = static_cast<int>(gptr()-eback());
                    streambuf_t::setp(pbase() - oldptr + newptr, newptr + new_size);
                    streambuf_t::pbump(pptr_count);
                    if(mode_ & ::std::ios_base::in)
                        streambuf_t::setg(newptr, newptr + gptr_count, pptr() + 1);
                    else
                        streambuf_t::setg(newptr, 0, newptr);
                }
                streambuf_t::sputc(compat_traits_type::to_char_type(meta));
                return meta;
            }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
        }
        // -end overflow(..)

        template<class Ch, class Tr, class Alloc>
        void basic_altstringbuf<Ch, Tr, Alloc>:: dealloc() {
            if(is_allocated_)
                alloc_.deallocate(eback(), (pptr() != NULL ? epptr() : egptr()) - eback());
            is_allocated_ = false;
            streambuf_t::setg(0, 0, 0);
            streambuf_t::setp(0, 0);
            putend_ = NULL;
        }

    }// N.S. io
} // N.S. boost

#endif // include guard

