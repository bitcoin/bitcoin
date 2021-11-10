/*
Copyright 2014-2016 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_ALIGNED_ALLOCATOR_ADAPTOR_HPP
#define BOOST_ALIGN_ALIGNED_ALLOCATOR_ADAPTOR_HPP

#include <boost/align/detail/is_alignment_constant.hpp>
#include <boost/align/detail/max_align.hpp>
#include <boost/align/detail/max_size.hpp>
#include <boost/align/align.hpp>
#include <boost/align/aligned_allocator_adaptor_forward.hpp>
#include <boost/align/alignment_of.hpp>
#include <boost/core/pointer_traits.hpp>
#include <boost/static_assert.hpp>
#include <new>

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
#include <memory>
#endif

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#include <utility>
#endif

namespace boost {
namespace alignment {

template<class Allocator, std::size_t Alignment>
class aligned_allocator_adaptor
    : public Allocator {
    BOOST_STATIC_ASSERT(detail::is_alignment_constant<Alignment>::value);

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
    typedef std::allocator_traits<Allocator> traits;
    typedef typename traits::template rebind_alloc<char> char_alloc;
    typedef typename traits::template rebind_traits<char> char_traits;
    typedef typename char_traits::pointer char_ptr;
#else
    typedef typename Allocator::template rebind<char>::other char_alloc;
    typedef typename char_alloc::pointer char_ptr;
#endif

public:
    typedef typename Allocator::value_type value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef void* void_pointer;
    typedef const void* const_void_pointer;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

private:
    template<class U>
    struct min_align {
        enum {
            value = detail::max_size<Alignment,
                detail::max_align<U, char_ptr>::value>::value
        };
    };

public:
    template<class U>
    struct rebind {
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
        typedef aligned_allocator_adaptor<typename traits::template
            rebind_alloc<U>, Alignment> other;
#else
        typedef aligned_allocator_adaptor<typename Allocator::template
            rebind<U>::other, Alignment> other;
#endif
    };

    aligned_allocator_adaptor()
        : Allocator() { }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template<class A>
    explicit aligned_allocator_adaptor(A&& alloc) BOOST_NOEXCEPT
        : Allocator(std::forward<A>(alloc)) { }
#else
    template<class A>
    explicit aligned_allocator_adaptor(const A& alloc) BOOST_NOEXCEPT
        : Allocator(alloc) { }
#endif

    template<class U>
    aligned_allocator_adaptor(const aligned_allocator_adaptor<U,
        Alignment>& other) BOOST_NOEXCEPT
        : Allocator(other.base()) { }

    Allocator& base() BOOST_NOEXCEPT {
        return static_cast<Allocator&>(*this);
    }

    const Allocator& base() const BOOST_NOEXCEPT {
        return static_cast<const Allocator&>(*this);
    }

    pointer allocate(size_type size) {
        enum {
            m = min_align<value_type>::value
        };
        std::size_t s = size * sizeof(value_type);
        std::size_t n = s + m - 1;
        char_alloc a(base());
        char_ptr p = a.allocate(sizeof p + n);
        void* r = boost::to_address(p) + sizeof p;
        (void)boost::alignment::align(m, s, r, n);
        ::new(static_cast<void*>(static_cast<char_ptr*>(r) - 1)) char_ptr(p);
        return static_cast<pointer>(r);
    }

    pointer allocate(size_type size, const_void_pointer hint) {
        enum {
            m = min_align<value_type>::value
        };
        std::size_t s = size * sizeof(value_type);
        std::size_t n = s + m - 1;
        char_ptr h = char_ptr();
        if (hint) {
            h = *(static_cast<const char_ptr*>(hint) - 1);
        }
        char_alloc a(base());
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
        char_ptr p = char_traits::allocate(a, sizeof p + n, h);
#else
        char_ptr p = a.allocate(sizeof p + n, h);
#endif
        void* r = boost::to_address(p) + sizeof p;
        (void)boost::alignment::align(m, s, r, n);
        ::new(static_cast<void*>(static_cast<char_ptr*>(r) - 1)) char_ptr(p);
        return static_cast<pointer>(r);
    }

    void deallocate(pointer ptr, size_type size) {
        enum {
            m = min_align<value_type>::value
        };
        char_ptr* p = reinterpret_cast<char_ptr*>(ptr) - 1;
        char_ptr r = *p;
        p->~char_ptr();
        char_alloc a(base());
        a.deallocate(r, sizeof r + size * sizeof(value_type) + m - 1);
    }
};

template<class A, class B, std::size_t Alignment>
inline bool
operator==(const aligned_allocator_adaptor<A, Alignment>& a,
    const aligned_allocator_adaptor<B, Alignment>& b) BOOST_NOEXCEPT
{
    return a.base() == b.base();
}

template<class A, class B, std::size_t Alignment>
inline bool
operator!=(const aligned_allocator_adaptor<A, Alignment>& a,
    const aligned_allocator_adaptor<B, Alignment>& b) BOOST_NOEXCEPT
{
    return !(a == b);
}

} /* alignment */
} /* boost */

#endif
