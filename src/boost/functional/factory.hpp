/*
Copyright 2007 Tobias Schwinger

Copyright 2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_FUNCTIONAL_FACTORY_HPP
#define BOOST_FUNCTIONAL_FACTORY_HPP

#include <boost/config.hpp>
#include <boost/core/empty_value.hpp>
#include <boost/core/pointer_traits.hpp>
#include <boost/type_traits/remove_cv.hpp>
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
#include <memory>
#endif
#include <new>
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <utility>
#endif

namespace boost {

enum factory_alloc_propagation {
    factory_alloc_for_pointee_and_deleter,
    factory_passes_alloc_to_smart_pointer
};

namespace detail {

template<factory_alloc_propagation>
struct fc_tag { };

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A, class T>
struct fc_rebind {
    typedef typename std::allocator_traits<A>::template rebind_alloc<T> type;
};

template<class A>
struct fc_pointer {
    typedef typename std::allocator_traits<A>::pointer type;
};
#else
template<class A, class T>
struct fc_rebind {
    typedef typename A::template rebind<T>::other type;
};

template<class A>
struct fc_pointer {
    typedef typename A::pointer type;
};
#endif

#if !defined(BOOST_NO_CXX11_ALLOCATOR) && \
    !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
template<class A, class T>
inline void
fc_destroy(A& a, T* p)
{
    std::allocator_traits<A>::destroy(a, p);
}
#else
template<class A, class T>
inline void
fc_destroy(A&, T* p)
{
    p->~T();
}
#endif

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A, class T, class... Args>
inline void
fc_construct(A& a, T* p, Args&&... args)
{
    std::allocator_traits<A>::construct(a, p, std::forward<Args>(args)...);
}
#else
template<class A, class T, class... Args>
inline void
fc_construct(A&, T* p, Args&&... args)
{
    ::new((void*)p) T(std::forward<Args>(args)...);
}
#endif
#endif

template<class A>
class fc_delete
    : boost::empty_value<A> {
    typedef boost::empty_value<A> base;

public:
    explicit fc_delete(const A& a) BOOST_NOEXCEPT
        : base(boost::empty_init_t(), a) { }

    void operator()(typename fc_pointer<A>::type p) {
        boost::detail::fc_destroy(base::get(), boost::to_address(p));
        base::get().deallocate(p, 1);
    }
};

template<class R, class A>
class fc_allocate {
public:
    explicit fc_allocate(const A& a)
        : a_(a)
        , p_(a_.allocate(1)) { }

    ~fc_allocate() {
        if (p_) {
            a_.deallocate(p_, 1);
        }
    }

    A& state() BOOST_NOEXCEPT {
        return a_;
    }

    typename A::value_type* get() const BOOST_NOEXCEPT {
        return boost::to_address(p_);
    }

    R release(fc_tag<factory_alloc_for_pointee_and_deleter>) {
        return R(release(), fc_delete<A>(a_), a_);
    }

    R release(fc_tag<factory_passes_alloc_to_smart_pointer>) {
        return R(release(), fc_delete<A>(a_));
    }

private:
    typedef typename fc_pointer<A>::type pointer;

    pointer release() BOOST_NOEXCEPT {
        pointer p = p_;
        p_ = pointer();
        return p;
    }

    fc_allocate(const fc_allocate&);
    fc_allocate& operator=(const fc_allocate&);

    A a_;
    pointer p_;
};

} /* detail */

template<class Pointer, class Allocator = void,
    factory_alloc_propagation Policy = factory_alloc_for_pointee_and_deleter>
class factory;

template<class Pointer, factory_alloc_propagation Policy>
class factory<Pointer, void, Policy> {
public:
    typedef typename remove_cv<Pointer>::type result_type;

private:
    typedef typename pointer_traits<result_type>::element_type type;

public:
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template<class... Args>
    result_type operator()(Args&&... args) const {
        return result_type(new type(std::forward<Args>(args)...));
    }
#else
    result_type operator()() const {
        return result_type(new type());
    }

    template<class A0>
    result_type operator()(A0& a0) const {
        return result_type(new type(a0));
    }

    template<class A0, class A1>
    result_type operator()(A0& a0, A1& a1) const {
        return result_type(new type(a0, a1));
    }

    template<class A0, class A1, class A2>
    result_type operator()(A0& a0, A1& a1, A2& a2) const {
        return result_type(new type(a0, a1, a2));
    }

    template<class A0, class A1, class A2, class A3>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3) const {
        return result_type(new type(a0, a1, a2, a3));
    }

    template<class A0, class A1, class A2, class A3, class A4>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4) const {
        return result_type(new type(a0, a1, a2, a3, a4));
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4,
        A5& a5) const {
        return result_type(new type(a0, a1, a2, a3, a4, a5));
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5,
        class A6>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5,
        A6& a6) const {
        return result_type(new type(a0, a1, a2, a3, a4, a5, a6));
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5,
        class A6, class A7>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5,
        A6& a6, A7& a7) const {
        return result_type(new type(a0, a1, a2, a3, a4, a5, a6, a7));
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5,
        class A6, class A7, class A8>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5,
        A6& a6, A7& a7, A8& a8) const {
        return result_type(new type(a0, a1, a2, a3, a4, a5, a6, a7, a8));
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5,
        class A6, class A7, class A8, class A9>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5,
        A6& a6, A7& a7, A8& a8, A9& a9) const {
        return result_type(new type(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9));
    }
#endif
};

template<class Pointer, class Allocator, factory_alloc_propagation Policy>
class factory
    : empty_value<typename detail::fc_rebind<Allocator,
        typename pointer_traits<typename
            remove_cv<Pointer>::type>::element_type>::type> {
public:
    typedef typename remove_cv<Pointer>::type result_type;

private:
    typedef typename pointer_traits<result_type>::element_type type;
    typedef typename detail::fc_rebind<Allocator, type>::type allocator;
    typedef empty_value<allocator> base;

public:
    factory() BOOST_NOEXCEPT
        : base(empty_init_t()) { }

    explicit factory(const Allocator& a) BOOST_NOEXCEPT
        : base(empty_init_t(), a) { }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template<class... Args>
    result_type operator()(Args&&... args) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        detail::fc_construct(s.state(), s.get(), std::forward<Args>(args)...);
        return s.release(detail::fc_tag<Policy>());
    }
#else
    result_type operator()() const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type();
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0>
    result_type operator()(A0& a0) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0);
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0, class A1>
    result_type operator()(A0& a0, A1& a1) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0, a1);
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0, class A1, class A2>
    result_type operator()(A0& a0, A1& a1, A2& a2) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0, a1, a2);
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0, class A1, class A2, class A3>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0, a1, a2, a3);
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0, class A1, class A2, class A3, class A4>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0, a1, a2, a3, a4);
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4,
        A5& a5) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0, a1, a2, a3, a4, a5);
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5,
        class A6>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5,
        A6& a6) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0, a1, a2, a3, a4, a5, a6);
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5,
        class A6, class A7>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5,
        A6& a6, A7& a7) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0, a1, a2, a3, a4, a5, a6, a7);
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5,
        class A6, class A7, class A8>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5,
        A6& a6, A7& a7, A8& a8) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0, a1, a2, a3, a4, a5, a6, a7, a8);
        return s.release(detail::fc_tag<Policy>());
    }

    template<class A0, class A1, class A2, class A3, class A4, class A5,
        class A6, class A7, class A8, class A9>
    result_type operator()(A0& a0, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5,
        A6& a6, A7& a7, A8& a8, A9& a9) const {
        detail::fc_allocate<result_type, allocator> s(base::get());
        ::new((void*)s.get()) type(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
        return s.release(detail::fc_tag<Policy>());
    }
#endif
};

template<class Pointer, class Allocator, factory_alloc_propagation Policy>
class factory<Pointer&, Allocator, Policy> { };

} /* boost */

#endif
