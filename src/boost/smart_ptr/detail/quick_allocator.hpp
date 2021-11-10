#ifndef BOOST_SMART_PTR_DETAIL_QUICK_ALLOCATOR_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_QUICK_ALLOCATOR_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  detail/quick_allocator.hpp
//
//  Copyright (c) 2003 David Abrahams
//  Copyright (c) 2003 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/config.hpp>

#include <boost/smart_ptr/detail/lightweight_mutex.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
#include <boost/type_traits/alignment_of.hpp>

#include <new>              // ::operator new, ::operator delete
#include <cstddef>          // std::size_t

namespace boost
{

namespace detail
{

template<unsigned size, unsigned align_> union freeblock
{
    typedef typename boost::type_with_alignment<align_>::type aligner_type;
    aligner_type aligner;
    char bytes[size];
    freeblock * next;
};

template<unsigned size, unsigned align_> struct allocator_impl
{
    typedef freeblock<size, align_> block;

    // It may seem odd to use such small pages.
    //
    // However, on a typical Windows implementation that uses
    // the OS allocator, "normal size" pages interact with the
    // "ordinary" operator new, slowing it down dramatically.
    //
    // 512 byte pages are handled by the small object allocator,
    // and don't interfere with ::new.
    //
    // The other alternative is to use much bigger pages (1M.)
    //
    // It is surprisingly easy to hit pathological behavior by
    // varying the page size. g++ 2.96 on Red Hat Linux 7.2,
    // for example, passionately dislikes 496. 512 seems OK.

#if defined(BOOST_QA_PAGE_SIZE)

    enum { items_per_page = BOOST_QA_PAGE_SIZE / size };

#else

    enum { items_per_page = 512 / size }; // 1048560 / size

#endif

#ifdef BOOST_HAS_THREADS

    static lightweight_mutex & mutex()
    {
        static freeblock< sizeof( lightweight_mutex ), boost::alignment_of< lightweight_mutex >::value > fbm;
        static lightweight_mutex * pm = new( &fbm ) lightweight_mutex;
        return *pm;
    }

    static lightweight_mutex * mutex_init;

#endif

    static block * free;
    static block * page;
    static unsigned last;

    static inline void * alloc()
    {
#ifdef BOOST_HAS_THREADS
        lightweight_mutex::scoped_lock lock( mutex() );
#endif
        if(block * x = free)
        {
            free = x->next;
            return x;
        }
        else
        {
            if(last == items_per_page)
            {
                // "Listen to me carefully: there is no memory leak"
                // -- Scott Meyers, Eff C++ 2nd Ed Item 10
                page = ::new block[items_per_page];
                last = 0;
            }

            return &page[last++];
        }
    }

    static inline void * alloc(std::size_t n)
    {
        if(n != size) // class-specific new called for a derived object
        {
            return ::operator new(n);
        }
        else
        {
#ifdef BOOST_HAS_THREADS
            lightweight_mutex::scoped_lock lock( mutex() );
#endif
            if(block * x = free)
            {
                free = x->next;
                return x;
            }
            else
            {
                if(last == items_per_page)
                {
                    page = ::new block[items_per_page];
                    last = 0;
                }

                return &page[last++];
            }
        }
    }

    static inline void dealloc(void * pv)
    {
        if(pv != 0) // 18.4.1.1/13
        {
#ifdef BOOST_HAS_THREADS
            lightweight_mutex::scoped_lock lock( mutex() );
#endif
            block * pb = static_cast<block *>(pv);
            pb->next = free;
            free = pb;
        }
    }

    static inline void dealloc(void * pv, std::size_t n)
    {
        if(n != size) // class-specific delete called for a derived object
        {
            ::operator delete(pv);
        }
        else if(pv != 0) // 18.4.1.1/13
        {
#ifdef BOOST_HAS_THREADS
            lightweight_mutex::scoped_lock lock( mutex() );
#endif
            block * pb = static_cast<block *>(pv);
            pb->next = free;
            free = pb;
        }
    }
};

#ifdef BOOST_HAS_THREADS

template<unsigned size, unsigned align_>
  lightweight_mutex * allocator_impl<size, align_>::mutex_init = &allocator_impl<size, align_>::mutex();

#endif

template<unsigned size, unsigned align_>
  freeblock<size, align_> * allocator_impl<size, align_>::free = 0;

template<unsigned size, unsigned align_>
  freeblock<size, align_> * allocator_impl<size, align_>::page = 0;

template<unsigned size, unsigned align_>
  unsigned allocator_impl<size, align_>::last = allocator_impl<size, align_>::items_per_page;

template<class T>
struct quick_allocator: public allocator_impl< sizeof(T), boost::alignment_of<T>::value >
{
};

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_QUICK_ALLOCATOR_HPP_INCLUDED
