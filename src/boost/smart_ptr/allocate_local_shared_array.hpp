/*
Copyright 2017-2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_SMART_PTR_ALLOCATE_LOCAL_SHARED_ARRAY_HPP
#define BOOST_SMART_PTR_ALLOCATE_LOCAL_SHARED_ARRAY_HPP

#include <boost/smart_ptr/allocate_shared_array.hpp>
#include <boost/smart_ptr/local_shared_ptr.hpp>

namespace boost {
namespace detail {

class BOOST_SYMBOL_VISIBLE lsp_array_base
    : public local_counted_base {
public:
    void set(sp_counted_base* base) BOOST_SP_NOEXCEPT {
        count_ = shared_count(base);
    }

    void local_cb_destroy() BOOST_SP_NOEXCEPT BOOST_OVERRIDE {
        shared_count().swap(count_);
    }

    shared_count local_cb_get_shared_count() const
        BOOST_SP_NOEXCEPT BOOST_OVERRIDE {
        return count_;
    }

private:
    shared_count count_;
};

template<class A>
class lsp_array_state
    : public sp_array_state<A> {
public:
    template<class U>
    lsp_array_state(const U& other, std::size_t size) BOOST_SP_NOEXCEPT
        : sp_array_state<A>(other, size) { }

    lsp_array_base& base() BOOST_SP_NOEXCEPT {
        return base_;
    }

private:
    lsp_array_base base_;
};

template<class A, std::size_t N>
class lsp_size_array_state
    : public sp_size_array_state<A, N> {
public:
    template<class U>
    lsp_size_array_state(const U& other, std::size_t size) BOOST_SP_NOEXCEPT
        : sp_size_array_state<A, N>(other, size) { }

    lsp_array_base& base() BOOST_SP_NOEXCEPT {
        return base_;
    }

private:
    lsp_array_base base_;
};

} /* detail */

template<class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value,
    local_shared_ptr<T> >::type
allocate_local_shared(const A& allocator, std::size_t count)
{
    typedef typename detail::sp_array_element<T>::type element;
    typedef typename allocator_rebind<A, element>::type other;
    typedef detail::lsp_array_state<other> state;
    typedef detail::sp_array_base<state> base;
    detail::sp_array_result<other, base> result(allocator, count);
    base* node = result.get();
    element* start = detail::sp_array_start<element>(node);
    ::new(static_cast<void*>(node)) base(allocator, start, count);
    detail::lsp_array_base& local = node->state().base();
    local.set(node);
    result.release();
    return local_shared_ptr<T>(detail::lsp_internal_constructor_tag(), start,
        &local);
}

template<class T, class A>
inline typename enable_if_<is_bounded_array<T>::value,
    local_shared_ptr<T> >::type
allocate_local_shared(const A& allocator)
{
    enum {
        count = extent<T>::value
    };
    typedef typename detail::sp_array_element<T>::type element;
    typedef typename allocator_rebind<A, element>::type other;
    typedef detail::lsp_size_array_state<other, count> state;
    typedef detail::sp_array_base<state> base;
    detail::sp_array_result<other, base> result(allocator, count);
    base* node = result.get();
    element* start = detail::sp_array_start<element>(node);
    ::new(static_cast<void*>(node)) base(allocator, start, count);
    detail::lsp_array_base& local = node->state().base();
    local.set(node);
    result.release();
    return local_shared_ptr<T>(detail::lsp_internal_constructor_tag(), start,
        &local);
}

template<class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value,
    local_shared_ptr<T> >::type
allocate_local_shared(const A& allocator, std::size_t count,
    const typename remove_extent<T>::type& value)
{
    typedef typename detail::sp_array_element<T>::type element;
    typedef typename allocator_rebind<A, element>::type other;
    typedef detail::lsp_array_state<other> state;
    typedef detail::sp_array_base<state> base;
    detail::sp_array_result<other, base> result(allocator, count);
    base* node = result.get();
    element* start = detail::sp_array_start<element>(node);
    ::new(static_cast<void*>(node)) base(allocator, start, count, value);
    detail::lsp_array_base& local = node->state().base();
    local.set(node);
    result.release();
    return local_shared_ptr<T>(detail::lsp_internal_constructor_tag(), start,
        &local);
}

template<class T, class A>
inline typename enable_if_<is_bounded_array<T>::value,
    local_shared_ptr<T> >::type
allocate_local_shared(const A& allocator,
    const typename remove_extent<T>::type& value)
{
    enum {
        count = extent<T>::value
    };
    typedef typename detail::sp_array_element<T>::type element;
    typedef typename allocator_rebind<A, element>::type other;
    typedef detail::lsp_size_array_state<other, count> state;
    typedef detail::sp_array_base<state> base;
    detail::sp_array_result<other, base> result(allocator, count);
    base* node = result.get();
    element* start = detail::sp_array_start<element>(node);
    ::new(static_cast<void*>(node)) base(allocator, start, count, value);
    detail::lsp_array_base& local = node->state().base();
    local.set(node);
    result.release();
    return local_shared_ptr<T>(detail::lsp_internal_constructor_tag(), start,
        &local);
}

template<class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value,
    local_shared_ptr<T> >::type
allocate_local_shared_noinit(const A& allocator, std::size_t count)
{
    return boost::allocate_local_shared<T>(boost::noinit_adapt(allocator),
        count);
}

template<class T, class A>
inline typename enable_if_<is_bounded_array<T>::value,
    local_shared_ptr<T> >::type
allocate_local_shared_noinit(const A& allocator)
{
    return boost::allocate_local_shared<T>(boost::noinit_adapt(allocator));
}

} /* boost */

#endif
