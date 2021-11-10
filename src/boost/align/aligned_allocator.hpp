/*
Copyright 2014-2015 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_ALIGNED_ALLOCATOR_HPP
#define BOOST_ALIGN_ALIGNED_ALLOCATOR_HPP

#include <boost/align/detail/add_reference.hpp>
#include <boost/align/detail/is_alignment_constant.hpp>
#include <boost/align/detail/max_objects.hpp>
#include <boost/align/detail/max_size.hpp>
#include <boost/align/detail/throw_exception.hpp>
#include <boost/align/aligned_alloc.hpp>
#include <boost/align/aligned_allocator_forward.hpp>
#include <boost/align/alignment_of.hpp>
#include <boost/static_assert.hpp>
#include <new>

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#include <utility>
#endif

namespace boost {
namespace alignment {

template<class T, std::size_t Alignment>
class aligned_allocator {
    BOOST_STATIC_ASSERT(detail::is_alignment_constant<Alignment>::value);

public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef void* void_pointer;
    typedef const void* const_void_pointer;
    typedef typename detail::add_lvalue_reference<T>::type reference;
    typedef typename detail::add_lvalue_reference<const
        T>::type const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef detail::true_type propagate_on_container_move_assignment;
    typedef detail::true_type is_always_equal;

    template<class U>
    struct rebind {
        typedef aligned_allocator<U, Alignment> other;
    };

#if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
    aligned_allocator() = default;
#else
    aligned_allocator() BOOST_NOEXCEPT { }
#endif

    template<class U>
    aligned_allocator(const aligned_allocator<U, Alignment>&)
        BOOST_NOEXCEPT { }

    pointer allocate(size_type size, const_void_pointer = 0) {
        enum {
            m = detail::max_size<Alignment,
                alignment_of<value_type>::value>::value
        };
        if (size == 0) {
            return 0;
        }
        void* p = boost::alignment::aligned_alloc(m, sizeof(T) * size);
        if (!p) {
            detail::throw_exception(std::bad_alloc());
        }
        return static_cast<T*>(p);
    }

    void deallocate(pointer ptr, size_type) {
        boost::alignment::aligned_free(ptr);
    }

    BOOST_CONSTEXPR size_type max_size() const BOOST_NOEXCEPT {
        return detail::max_objects<T>::value;
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template<class U, class... Args>
    void construct(U* ptr, Args&&... args) {
        ::new((void*)ptr) U(std::forward<Args>(args)...);
    }
#else
    template<class U, class V>
    void construct(U* ptr, V&& value) {
        ::new((void*)ptr) U(std::forward<V>(value));
    }
#endif
#else
    template<class U, class V>
    void construct(U* ptr, const V& value) {
        ::new((void*)ptr) U(value);
    }

    template<class U, class V>
    void construct(U* ptr, V& value) {
        ::new((void*)ptr) U(value);
    }
#endif

    template<class U>
    void construct(U* ptr) {
        ::new((void*)ptr) U();
    }

    template<class U>
    void destroy(U* ptr) {
        (void)ptr;
        ptr->~U();
    }
};

template<class T, class U, std::size_t Alignment>
inline bool
operator==(const aligned_allocator<T, Alignment>&,
    const aligned_allocator<U, Alignment>&) BOOST_NOEXCEPT
{
    return true;
}

template<class T, class U, std::size_t Alignment>
inline bool
operator!=(const aligned_allocator<T, Alignment>&,
    const aligned_allocator<U, Alignment>&) BOOST_NOEXCEPT
{
    return false;
}

} /* alignment */
} /* boost */

#endif
