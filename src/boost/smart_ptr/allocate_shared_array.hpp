/*
Copyright 2012-2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_SMART_PTR_ALLOCATE_SHARED_ARRAY_HPP
#define BOOST_SMART_PTR_ALLOCATE_SHARED_ARRAY_HPP

#include <boost/core/allocator_access.hpp>
#include <boost/core/alloc_construct.hpp>
#include <boost/core/first_scalar.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/enable_if.hpp>
#include <boost/type_traits/extent.hpp>
#include <boost/type_traits/is_bounded_array.hpp>
#include <boost/type_traits/is_unbounded_array.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_extent.hpp>
#include <boost/type_traits/type_with_alignment.hpp>

namespace boost {
namespace detail {

template<class T>
struct sp_array_element {
    typedef typename boost::remove_cv<typename
        boost::remove_extent<T>::type>::type type;
};

template<class T>
struct sp_array_count {
    enum {
        value = 1
    };
};

template<class T, std::size_t N>
struct sp_array_count<T[N]> {
    enum {
        value = N * sp_array_count<T>::value
    };
};

template<std::size_t N, std::size_t M>
struct sp_max_size {
    enum {
        value = N < M ? M : N
    };
};

template<std::size_t N, std::size_t M>
struct sp_align_up {
    enum {
        value = (N + M - 1) & ~(M - 1)
    };
};

template<class T>
BOOST_CONSTEXPR inline std::size_t
sp_objects(std::size_t size) BOOST_SP_NOEXCEPT
{
    return (size + sizeof(T) - 1) / sizeof(T);
}

template<class A>
class sp_array_state {
public:
    typedef A type;

    template<class U>
    sp_array_state(const U& _allocator, std::size_t _size) BOOST_SP_NOEXCEPT
        : allocator_(_allocator),
          size_(_size) { }

    A& allocator() BOOST_SP_NOEXCEPT {
        return allocator_;
    }

    std::size_t size() const BOOST_SP_NOEXCEPT {
        return size_;
    }

private:
    A allocator_;
    std::size_t size_;
};

template<class A, std::size_t N>
class sp_size_array_state {
public:
    typedef A type;

    template<class U>
    sp_size_array_state(const U& _allocator, std::size_t) BOOST_SP_NOEXCEPT
        : allocator_(_allocator) { }

    A& allocator() BOOST_SP_NOEXCEPT {
        return allocator_;
    }

    BOOST_CONSTEXPR std::size_t size() const BOOST_SP_NOEXCEPT {
        return N;
    }

private:
    A allocator_;
};

template<class T, class U>
struct sp_array_alignment {
    enum {
        value = sp_max_size<boost::alignment_of<T>::value,
            boost::alignment_of<U>::value>::value
    };
};

template<class T, class U>
struct sp_array_offset {
    enum {
        value = sp_align_up<sizeof(T), sp_array_alignment<T, U>::value>::value
    };
};

template<class U, class T>
inline U*
sp_array_start(T* base) BOOST_SP_NOEXCEPT
{
    enum {
        size = sp_array_offset<T, U>::value
    };
    return reinterpret_cast<U*>(reinterpret_cast<char*>(base) + size);
}

template<class A, class T>
class sp_array_creator {
    typedef typename A::value_type element;

    enum {
        offset = sp_array_offset<T, element>::value
    };

    typedef typename boost::type_with_alignment<sp_array_alignment<T,
        element>::value>::type type;

public:
    template<class U>
    sp_array_creator(const U& other, std::size_t size) BOOST_SP_NOEXCEPT
        : other_(other),
          size_(sp_objects<type>(offset + sizeof(element) * size)) { }

    T* create() {
        return reinterpret_cast<T*>(other_.allocate(size_));
    }

    void destroy(T* base) {
        other_.deallocate(reinterpret_cast<type*>(base), size_);
    }

private:
    typename boost::allocator_rebind<A, type>::type other_;
    std::size_t size_;
};

template<class T>
class BOOST_SYMBOL_VISIBLE sp_array_base
    : public sp_counted_base {
    typedef typename T::type allocator;

public:
    typedef typename allocator::value_type type;

    template<class A>
    sp_array_base(const A& other, type* start, std::size_t size)
        : state_(other, size) {
        boost::alloc_construct_n(state_.allocator(),
            boost::first_scalar(start),
            state_.size() * sp_array_count<type>::value);
    }

    template<class A, class U>
    sp_array_base(const A& other, type* start, std::size_t size, const U& list)
        : state_(other, size) {
        enum {
            count = sp_array_count<type>::value
        };
        boost::alloc_construct_n(state_.allocator(),
            boost::first_scalar(start), state_.size() * count,
            boost::first_scalar(&list), count);
    }

    T& state() BOOST_SP_NOEXCEPT {
        return state_;
    }

    void dispose() BOOST_SP_NOEXCEPT BOOST_OVERRIDE {
        boost::alloc_destroy_n(state_.allocator(),
            boost::first_scalar(sp_array_start<type>(this)),
            state_.size() * sp_array_count<type>::value);
    }

    void destroy() BOOST_SP_NOEXCEPT BOOST_OVERRIDE {
        sp_array_creator<allocator, sp_array_base> other(state_.allocator(),
            state_.size());
        this->~sp_array_base();
        other.destroy(this);
    }

    void* get_deleter(const sp_typeinfo_&) BOOST_SP_NOEXCEPT BOOST_OVERRIDE {
        return 0;
    }

    void* get_local_deleter(const sp_typeinfo_&)
        BOOST_SP_NOEXCEPT BOOST_OVERRIDE {
        return 0;
    }

    void* get_untyped_deleter() BOOST_SP_NOEXCEPT BOOST_OVERRIDE {
        return 0;
    }

private:
    T state_;
};

template<class A, class T>
struct sp_array_result {
public:
    template<class U>
    sp_array_result(const U& other, std::size_t size)
        : creator_(other, size),
          result_(creator_.create()) { }

    ~sp_array_result() {
        if (result_) {
            creator_.destroy(result_);
        }
    }

    T* get() const BOOST_SP_NOEXCEPT {
        return result_;
    }

    void release() BOOST_SP_NOEXCEPT {
        result_ = 0;
    }

private:
    sp_array_result(const sp_array_result&);
    sp_array_result& operator=(const sp_array_result&);

    sp_array_creator<A, T> creator_;
    T* result_;
};

} /* detail */

template<class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value, shared_ptr<T> >::type
allocate_shared(const A& allocator, std::size_t count)
{
    typedef typename detail::sp_array_element<T>::type element;
    typedef typename allocator_rebind<A, element>::type other;
    typedef detail::sp_array_state<other> state;
    typedef detail::sp_array_base<state> base;
    detail::sp_array_result<other, base> result(allocator, count);
    base* node = result.get();
    element* start = detail::sp_array_start<element>(node);
    ::new(static_cast<void*>(node)) base(allocator, start, count);
    result.release();
    return shared_ptr<T>(detail::sp_internal_constructor_tag(), start,
        detail::shared_count(static_cast<detail::sp_counted_base*>(node)));
}

template<class T, class A>
inline typename enable_if_<is_bounded_array<T>::value, shared_ptr<T> >::type
allocate_shared(const A& allocator)
{
    enum {
        count = extent<T>::value
    };
    typedef typename detail::sp_array_element<T>::type element;
    typedef typename allocator_rebind<A, element>::type other;
    typedef detail::sp_size_array_state<other, extent<T>::value> state;
    typedef detail::sp_array_base<state> base;
    detail::sp_array_result<other, base> result(allocator, count);
    base* node = result.get();
    element* start = detail::sp_array_start<element>(node);
    ::new(static_cast<void*>(node)) base(allocator, start, count);
    result.release();
    return shared_ptr<T>(detail::sp_internal_constructor_tag(), start,
        detail::shared_count(static_cast<detail::sp_counted_base*>(node)));
}

template<class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value, shared_ptr<T> >::type
allocate_shared(const A& allocator, std::size_t count,
    const typename remove_extent<T>::type& value)
{
    typedef typename detail::sp_array_element<T>::type element;
    typedef typename allocator_rebind<A, element>::type other;
    typedef detail::sp_array_state<other> state;
    typedef detail::sp_array_base<state> base;
    detail::sp_array_result<other, base> result(allocator, count);
    base* node = result.get();
    element* start = detail::sp_array_start<element>(node);
    ::new(static_cast<void*>(node)) base(allocator, start, count, value);
    result.release();
    return shared_ptr<T>(detail::sp_internal_constructor_tag(), start,
        detail::shared_count(static_cast<detail::sp_counted_base*>(node)));
}

template<class T, class A>
inline typename enable_if_<is_bounded_array<T>::value, shared_ptr<T> >::type
allocate_shared(const A& allocator,
    const typename remove_extent<T>::type& value)
{
    enum {
        count = extent<T>::value
    };
    typedef typename detail::sp_array_element<T>::type element;
    typedef typename allocator_rebind<A, element>::type other;
    typedef detail::sp_size_array_state<other, extent<T>::value> state;
    typedef detail::sp_array_base<state> base;
    detail::sp_array_result<other, base> result(allocator, count);
    base* node = result.get();
    element* start = detail::sp_array_start<element>(node);
    ::new(static_cast<void*>(node)) base(allocator, start, count, value);
    result.release();
    return shared_ptr<T>(detail::sp_internal_constructor_tag(), start,
        detail::shared_count(static_cast<detail::sp_counted_base*>(node)));
}

template<class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value, shared_ptr<T> >::type
allocate_shared_noinit(const A& allocator, std::size_t count)
{
    return boost::allocate_shared<T>(boost::noinit_adapt(allocator), count);
}

template<class T, class A>
inline typename enable_if_<is_bounded_array<T>::value, shared_ptr<T> >::type
allocate_shared_noinit(const A& allocator)
{
    return boost::allocate_shared<T>(boost::noinit_adapt(allocator));
}

} /* boost */

#endif
