/*
Copyright 2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_SMART_PTR_ALLOCATE_UNIQUE_HPP
#define BOOST_SMART_PTR_ALLOCATE_UNIQUE_HPP

#include <boost/smart_ptr/detail/sp_noexcept.hpp>
#include <boost/smart_ptr/detail/sp_nullptr_t.hpp>
#include <boost/core/allocator_access.hpp>
#include <boost/core/alloc_construct.hpp>
#include <boost/core/empty_value.hpp>
#include <boost/core/first_scalar.hpp>
#include <boost/core/noinit_adaptor.hpp>
#include <boost/core/pointer_traits.hpp>
#include <boost/type_traits/enable_if.hpp>
#include <boost/type_traits/extent.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_bounded_array.hpp>
#include <boost/type_traits/is_unbounded_array.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_extent.hpp>
#include <boost/type_traits/type_identity.hpp>
#include <boost/config.hpp>
#include <memory>
#include <utility>

namespace boost {
namespace detail {

template<class T>
struct sp_alloc_size {
    BOOST_STATIC_CONSTEXPR std::size_t value = 1;
};

template<class T>
struct sp_alloc_size<T[]> {
    BOOST_STATIC_CONSTEXPR std::size_t value = sp_alloc_size<T>::value;
};

template<class T, std::size_t N>
struct sp_alloc_size<T[N]> {
    BOOST_STATIC_CONSTEXPR std::size_t value = N * sp_alloc_size<T>::value;
};

template<class T>
struct sp_alloc_result {
    typedef T type;
};

template<class T, std::size_t N>
struct sp_alloc_result<T[N]> {
    typedef T type[];
};

template<class T>
struct sp_alloc_value {
    typedef typename boost::remove_cv<typename
        boost::remove_extent<T>::type>::type type;
};

template<class T, class P>
class sp_alloc_ptr {
public:
    typedef T element_type;

    sp_alloc_ptr() BOOST_SP_NOEXCEPT
        : p_() { }

#if defined(BOOST_MSVC) && BOOST_MSVC == 1600
    sp_alloc_ptr(T* p) BOOST_SP_NOEXCEPT
        : p_(const_cast<typename boost::remove_cv<T>::type*>(p)) { }
#endif

    sp_alloc_ptr(std::size_t, P p) BOOST_SP_NOEXCEPT
        : p_(p) { }

#if !defined(BOOST_NO_CXX11_NULLPTR)
    sp_alloc_ptr(detail::sp_nullptr_t) BOOST_SP_NOEXCEPT
        : p_() { }
#endif

    T& operator*() const {
        return *p_;
    }

    T* operator->() const BOOST_SP_NOEXCEPT {
        return boost::to_address(p_);
    }

#if !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
    explicit operator bool() const BOOST_SP_NOEXCEPT {
        return !!p_;
    }
#endif

    bool operator!() const BOOST_SP_NOEXCEPT {
        return !p_;
    }

    P ptr() const BOOST_SP_NOEXCEPT {
        return p_;
    }

    BOOST_STATIC_CONSTEXPR std::size_t size() BOOST_SP_NOEXCEPT {
        return 1;
    }

#if defined(BOOST_MSVC) && BOOST_MSVC < 1910
    static sp_alloc_ptr pointer_to(T& v) {
        return sp_alloc_ptr(1,
            std::pointer_traits<P>::pointer_to(const_cast<typename
                boost::remove_cv<T>::type&>(v)));
    }
#endif

private:
    P p_;
};

template<class T, class P>
class sp_alloc_ptr<T[], P> {
public:
    typedef T element_type;

    sp_alloc_ptr() BOOST_SP_NOEXCEPT
        : p_() { }

    sp_alloc_ptr(std::size_t n, P p) BOOST_SP_NOEXCEPT
        : p_(p)
        , n_(n) { }

#if !defined(BOOST_NO_CXX11_NULLPTR)
    sp_alloc_ptr(detail::sp_nullptr_t) BOOST_SP_NOEXCEPT
        : p_() { }
#endif

    T& operator[](std::size_t i) const {
        return p_[i];
    }

#if !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
    explicit operator bool() const BOOST_SP_NOEXCEPT {
        return !!p_;
    }
#endif

    bool operator!() const BOOST_SP_NOEXCEPT {
        return !p_;
    }

    P ptr() const BOOST_SP_NOEXCEPT {
        return p_;
    }

    std::size_t size() const BOOST_SP_NOEXCEPT {
        return n_;
    }

#if defined(BOOST_MSVC) && BOOST_MSVC < 1910
    static sp_alloc_ptr pointer_to(T& v) {
        return sp_alloc_ptr(n_,
            std::pointer_traits<P>::pointer_to(const_cast<typename
                boost::remove_cv<T>::type&>(v)));
    }
#endif

private:
    P p_;
    std::size_t n_;
};

template<class T, std::size_t N, class P>
class sp_alloc_ptr<T[N], P> {
public:
    typedef T element_type;

    sp_alloc_ptr() BOOST_SP_NOEXCEPT
        : p_() { }

    sp_alloc_ptr(std::size_t, P p) BOOST_SP_NOEXCEPT
        : p_(p) { }

#if !defined(BOOST_NO_CXX11_NULLPTR)
    sp_alloc_ptr(detail::sp_nullptr_t) BOOST_SP_NOEXCEPT
        : p_() { }
#endif

    T& operator[](std::size_t i) const {
        return p_[i];
    }

#if !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
    explicit operator bool() const BOOST_SP_NOEXCEPT {
        return !!p_;
    }
#endif

    bool operator!() const BOOST_SP_NOEXCEPT {
        return !p_;
    }

    P ptr() const BOOST_SP_NOEXCEPT {
        return p_;
    }

    BOOST_STATIC_CONSTEXPR std::size_t size() BOOST_SP_NOEXCEPT {
        return N;
    }

#if defined(BOOST_MSVC) && BOOST_MSVC < 1910
    static sp_alloc_ptr pointer_to(T& v) {
        return sp_alloc_ptr(N,
            std::pointer_traits<P>::pointer_to(const_cast<typename
                boost::remove_cv<T>::type&>(v)));
    }
#endif

private:
    P p_;
};

template<class T, class P>
inline bool
operator==(const sp_alloc_ptr<T, P>& lhs, const sp_alloc_ptr<T, P>& rhs)
{
    return lhs.ptr() == rhs.ptr();
}

template<class T, class P>
inline bool
operator!=(const sp_alloc_ptr<T, P>& lhs, const sp_alloc_ptr<T, P>& rhs)
{
    return !(lhs == rhs);
}

#if !defined(BOOST_NO_CXX11_NULLPTR)
template<class T, class P>
inline bool
operator==(const sp_alloc_ptr<T, P>& lhs,
    detail::sp_nullptr_t) BOOST_SP_NOEXCEPT
{
    return !lhs.ptr();
}

template<class T, class P>
inline bool
operator==(detail::sp_nullptr_t,
    const sp_alloc_ptr<T, P>& rhs) BOOST_SP_NOEXCEPT
{
    return !rhs.ptr();
}

template<class T, class P>
inline bool
operator!=(const sp_alloc_ptr<T, P>& lhs,
    detail::sp_nullptr_t) BOOST_SP_NOEXCEPT
{
    return !!lhs.ptr();
}

template<class T, class P>
inline bool
operator!=(detail::sp_nullptr_t,
    const sp_alloc_ptr<T, P>& rhs) BOOST_SP_NOEXCEPT
{
    return !!rhs.ptr();
}
#endif

template<class A>
inline void
sp_alloc_clear(A& a, typename boost::allocator_pointer<A>::type p, std::size_t,
    boost::false_type)
{
    boost::alloc_destroy(a, boost::to_address(p));
}

template<class A>
inline void
sp_alloc_clear(A& a, typename boost::allocator_pointer<A>::type p,
    std::size_t n, boost::true_type)
{
#if defined(BOOST_MSVC) && BOOST_MSVC < 1800
    if (!p) {
        return;
    }
#endif
    boost::alloc_destroy_n(a, boost::first_scalar(boost::to_address(p)),
        n * sp_alloc_size<typename A::value_type>::value);
}

} /* detail */

template<class T, class A>
class alloc_deleter
    : empty_value<typename allocator_rebind<A,
        typename detail::sp_alloc_value<T>::type>::type> {
    typedef typename allocator_rebind<A,
        typename detail::sp_alloc_value<T>::type>::type allocator;
    typedef empty_value<allocator> base;

public:
    typedef detail::sp_alloc_ptr<T,
        typename allocator_pointer<allocator>::type> pointer;

    explicit alloc_deleter(const allocator& a) BOOST_SP_NOEXCEPT
        : base(empty_init_t(), a) { }

    void operator()(pointer p) {
        detail::sp_alloc_clear(base::get(), p.ptr(), p.size(), is_array<T>());
        base::get().deallocate(p.ptr(), p.size());
    }
};

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
template<class T, class A>
using alloc_noinit_deleter = alloc_deleter<T, noinit_adaptor<A> >;
#endif

namespace detail {

template<class T, class A>
class sp_alloc_make {
public:
    typedef typename boost::allocator_rebind<A,
        typename sp_alloc_value<T>::type>::type allocator;

private:
    typedef boost::alloc_deleter<T, A> deleter;

public:
    typedef std::unique_ptr<typename sp_alloc_result<T>::type, deleter> type;

    sp_alloc_make(const A& a, std::size_t n)
        : a_(a)
        , n_(n)
        , p_(a_.allocate(n)) { }

    ~sp_alloc_make() {
        if (p_) {
            a_.deallocate(p_, n_);
        }
    }

    typename allocator::value_type* get() const BOOST_SP_NOEXCEPT {
        return boost::to_address(p_);
    }

    allocator& state() BOOST_SP_NOEXCEPT {
        return a_;
    }

    type release() BOOST_SP_NOEXCEPT {
        pointer p = p_;
        p_ = pointer();
        return type(typename deleter::pointer(n_, p), deleter(a_));
    }

private:
    typedef typename boost::allocator_pointer<allocator>::type pointer;

    allocator a_;
    std::size_t n_;
    pointer p_;
};

} /* detail */

template<class T, class A>
inline typename enable_if_<!is_array<T>::value,
    std::unique_ptr<T, alloc_deleter<T, A> > >::type
allocate_unique(const A& alloc)
{
    detail::sp_alloc_make<T, A> c(alloc, 1);
    boost::alloc_construct(c.state(), c.get());
    return c.release();
}

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
template<class T, class A, class... Args>
inline typename enable_if_<!is_array<T>::value,
    std::unique_ptr<T, alloc_deleter<T, A> > >::type
allocate_unique(const A& alloc, Args&&... args)
{
    detail::sp_alloc_make<T, A> c(alloc, 1);
    boost::alloc_construct(c.state(), c.get(), std::forward<Args>(args)...);
    return c.release();
}
#endif

template<class T, class A>
inline typename enable_if_<!is_array<T>::value,
    std::unique_ptr<T, alloc_deleter<T, A> > >::type
allocate_unique(const A& alloc, typename type_identity<T>::type&& value)
{
    detail::sp_alloc_make<T, A> c(alloc, 1);
    boost::alloc_construct(c.state(), c.get(), std::move(value));
    return c.release();
}

template<class T, class A>
inline typename enable_if_<!is_array<T>::value,
    std::unique_ptr<T, alloc_deleter<T, noinit_adaptor<A> > > >::type
allocate_unique_noinit(const A& alloc)
{
    return boost::allocate_unique<T, noinit_adaptor<A> >(alloc);
}

template<class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value,
    std::unique_ptr<T, alloc_deleter<T, A> > >::type
allocate_unique(const A& alloc, std::size_t size)
{
    detail::sp_alloc_make<T, A> c(alloc, size);
    boost::alloc_construct_n(c.state(), boost::first_scalar(c.get()),
        size * detail::sp_alloc_size<T>::value);
    return c.release();
}

template<class T, class A>
inline typename enable_if_<is_bounded_array<T>::value,
    std::unique_ptr<typename detail::sp_alloc_result<T>::type,
        alloc_deleter<T, A> > >::type
allocate_unique(const A& alloc)
{
    detail::sp_alloc_make<T, A> c(alloc, extent<T>::value);
    boost::alloc_construct_n(c.state(), boost::first_scalar(c.get()),
        detail::sp_alloc_size<T>::value);
    return c.release();
}

template<class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value,
    std::unique_ptr<T, alloc_deleter<T, noinit_adaptor<A> > > >::type
allocate_unique_noinit(const A& alloc, std::size_t size)
{
    return boost::allocate_unique<T, noinit_adaptor<A> >(alloc, size);
}

template<class T, class A>
inline typename enable_if_<is_bounded_array<T>::value,
    std::unique_ptr<typename detail::sp_alloc_result<T>::type,
        alloc_deleter<T, noinit_adaptor<A> > > >::type
allocate_unique_noinit(const A& alloc)
{
    return boost::allocate_unique<T, noinit_adaptor<A> >(alloc);
}

template<class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value,
    std::unique_ptr<T, alloc_deleter<T, A> > >::type
allocate_unique(const A& alloc, std::size_t size,
    const typename remove_extent<T>::type& value)
{
    detail::sp_alloc_make<T, A> c(alloc, size);
    boost::alloc_construct_n(c.state(), boost::first_scalar(c.get()),
        size * detail::sp_alloc_size<T>::value, boost::first_scalar(&value),
        detail::sp_alloc_size<typename remove_extent<T>::type>::value);
    return c.release();
}

template<class T, class A>
inline typename enable_if_<is_bounded_array<T>::value,
    std::unique_ptr<typename detail::sp_alloc_result<T>::type,
        alloc_deleter<T, A> > >::type
allocate_unique(const A& alloc,
    const typename remove_extent<T>::type& value)
{
    detail::sp_alloc_make<T, A> c(alloc, extent<T>::value);
    boost::alloc_construct_n(c.state(), boost::first_scalar(c.get()),
        detail::sp_alloc_size<T>::value, boost::first_scalar(&value),
        detail::sp_alloc_size<typename remove_extent<T>::type>::value);
    return c.release();
}

} /* boost */

#endif
