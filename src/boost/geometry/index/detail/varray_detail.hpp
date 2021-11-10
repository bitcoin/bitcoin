// Boost.Geometry
//
// varray details
//
// Copyright (c) 2011-2013 Andrew Hundt.
// Copyright (c) 2012-2020 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_DETAIL_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_DETAIL_HPP

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>

#include <boost/config.hpp>

#include <boost/core/no_exceptions_support.hpp>
#include <boost/move/move.hpp>
#include <boost/core/addressof.hpp>
#include <boost/iterator/iterator_traits.hpp>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <boost/move/detail/fwd_macros.hpp>
#endif

// TODO - move vectors iterators optimization to the other, optional file instead of checking defines?

#if defined(BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_ENABLE_VECTOR_OPTIMIZATION) && !defined(BOOST_NO_EXCEPTIONS)
#include <vector>
#include <boost/container/vector.hpp>
#endif // BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_ENABLE_VECTOR_OPTIMIZATION && !BOOST_NO_EXCEPTIONS

namespace boost { namespace geometry { namespace index { namespace detail { namespace varray_detail
{


template <typename I>
struct are_elements_contiguous : std::is_pointer<I>
{};
    
// EXPERIMENTAL - not finished
// Conditional setup - mark vector iterators defined in known implementations
// as iterators pointing to contiguous ranges

#if defined(BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_ENABLE_VECTOR_OPTIMIZATION) && !defined(BOOST_NO_EXCEPTIONS)
    
template <typename Pointer>
struct are_elements_contiguous<
    boost::container::container_detail::vector_const_iterator<Pointer>
> : std::true_type
{};

template <typename Pointer>
struct are_elements_contiguous<
    boost::container::container_detail::vector_iterator<Pointer>
> : std::true_type
{};

#if defined(BOOST_DINKUMWARE_STDLIB)
    
template <typename T>
struct are_elements_contiguous<
    std::_Vector_const_iterator<T>
> : std::true_type
{};

template <typename T>
struct are_elements_contiguous<
    std::_Vector_iterator<T>
> : std::true_type
{};

#elif defined(BOOST_GNU_STDLIB)

template <typename P, typename T, typename A>
struct are_elements_contiguous<
    __gnu_cxx::__normal_iterator<P, std::vector<T, A> >
> : std::true_type
{};

#elif defined(_LIBCPP_VERSION)

// TODO - test it first
//template <typename P>
//struct are_elements_contiguous<
//    __wrap_iter<P>
//> : std::true_type
//{};

#else // OTHER_STDLIB

// TODO - add other iterators implementations
    
#endif // STDLIB

#endif // BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_ENABLE_VECTOR_OPTIMIZATION && !BOOST_NO_EXCEPTIONS


template <typename I, typename O>
struct is_memop_safe_for_range
    : std::integral_constant
        <
            bool,
            std::is_same
                <
                    std::remove_const_t
                        <
                            typename ::boost::iterator_value<I>::type
                        >,
                    std::remove_const_t
                        <
                            typename ::boost::iterator_value<O>::type
                        >
                >::value
            &&
            are_elements_contiguous<I>::value
            &&
            are_elements_contiguous<O>::value
            &&
            std::is_trivially_copyable
                <
                    typename ::boost::iterator_value<O>::type
                >::value
        >
{};


template <typename I, typename V>
struct is_memop_safe_for_value
    : std::integral_constant
        <
            bool,
            std::is_same
                <
                    std::remove_const_t
                        <
                            typename ::boost::iterator_value<I>::type
                        >,
                    std::remove_const_t<V>
                >::value
            &&
            std::is_trivially_copyable
                <
                    V
                >::value
        >
{};

// destroy(I, I)

template <typename I>
void destroy_dispatch(I /*first*/, I /*last*/,
                      std::true_type /*has_trivial_destructor*/)
{}

template <typename I>
void destroy_dispatch(I first, I last,
                      std::false_type /*has_trivial_destructor*/)
{
    typedef typename boost::iterator_value<I>::type value_type;
    for ( ; first != last ; ++first )
        first->~value_type();
}

template <typename I>
void destroy(I first, I last)
{
    typedef typename boost::iterator_value<I>::type value_type;
    destroy_dispatch(first, last, std::is_trivially_destructible<value_type>());
}

// destroy(I)

template <typename I>
void destroy_dispatch(I /*pos*/,
                      std::true_type /*has_trivial_destructor*/)
{}

template <typename I>
void destroy_dispatch(I pos,
                      std::false_type /*has_trivial_destructor*/)
{
    typedef typename boost::iterator_value<I>::type value_type;
    pos->~value_type();
}

template <typename I>
void destroy(I pos)
{
    typedef typename boost::iterator_value<I>::type value_type;
    destroy_dispatch(pos, std::is_trivially_destructible<value_type>());
}

// copy(I, I, O)

template <typename I, typename O>
inline O copy_dispatch(I first, I last, O dst,
                       std::true_type /*use_memmove*/)
{
    typedef typename boost::iterator_value<I>::type value_type;
    typename boost::iterator_difference<I>::type d = std::distance(first, last);

    ::memmove(boost::addressof(*dst), boost::addressof(*first), sizeof(value_type) * d);
    return dst + d;
}

template <typename I, typename O>
inline O copy_dispatch(I first, I last, O dst,
                       std::false_type /*use_memmove*/)
{
    return std::copy(first, last, dst);                                         // may throw
}

template <typename I, typename O>
inline O copy(I first, I last, O dst)
{
    return copy_dispatch(first, last, dst, is_memop_safe_for_range<I, O>());    // may throw
}

// uninitialized_copy(I, I, O)

template <typename I, typename O>
inline
O uninitialized_copy_dispatch(I first, I last, O dst,
                              std::true_type /*use_memcpy*/)
{
    typedef typename boost::iterator_value<I>::type value_type;
    typename boost::iterator_difference<I>::type d = std::distance(first, last);

    ::memcpy(boost::addressof(*dst), boost::addressof(*first), sizeof(value_type) * d);
    return dst + d;
}

template <typename I, typename F>
inline
F uninitialized_copy_dispatch(I first, I last, F dst,
                              std::false_type /*use_memcpy*/)
{
    return std::uninitialized_copy(first, last, dst);                                       // may throw
}

template <typename I, typename F>
inline
F uninitialized_copy(I first, I last, F dst)
{
    return uninitialized_copy_dispatch(first, last, dst, is_memop_safe_for_range<I, F>());  // may throw
}

// uninitialized_move(I, I, O)

template <typename I, typename O>
inline
O uninitialized_move_dispatch(I first, I last, O dst,
                              std::true_type /*use_memcpy*/)
{
    typedef typename boost::iterator_value<I>::type value_type;
    typename boost::iterator_difference<I>::type d = std::distance(first, last);

    ::memcpy(boost::addressof(*dst), boost::addressof(*first), sizeof(value_type) * d);
    return dst + d;
}

template <typename I, typename O>
inline
O uninitialized_move_dispatch(I first, I last, O dst,
                              std::false_type /*use_memcpy*/)
{
    //return boost::uninitialized_move(first, last, dst);                         // may throw

    O o = dst;

    BOOST_TRY
    {
        typedef typename std::iterator_traits<O>::value_type value_type;
        for (; first != last; ++first, ++o )
            new (boost::addressof(*o)) value_type(boost::move(*first));
    }
    BOOST_CATCH(...)
    {
        varray_detail::destroy(dst, o);
        BOOST_RETHROW;
    }
    BOOST_CATCH_END

    return dst;
}

template <typename I, typename O>
inline
O uninitialized_move(I first, I last, O dst)
{
    return uninitialized_move_dispatch(first, last, dst, is_memop_safe_for_range<I, O>()); // may throw
}

// TODO - move uses memmove - implement 2nd version using memcpy?

// move(I, I, O)

template <typename I, typename O>
inline
O move_dispatch(I first, I last, O dst,
                std::true_type /*use_memmove*/)
{
    typedef typename boost::iterator_value<I>::type value_type;
    typename boost::iterator_difference<I>::type d = std::distance(first, last);

    ::memmove(boost::addressof(*dst), boost::addressof(*first), sizeof(value_type) * d);
    return dst + d;
}

template <typename I, typename O>
inline
O move_dispatch(I first, I last, O dst,
                std::false_type /*use_memmove*/)
{
    return boost::move(first, last, dst);                                         // may throw
}

template <typename I, typename O>
inline
O move(I first, I last, O dst)
{
    return move_dispatch(first, last, dst, is_memop_safe_for_range<I, O>());      // may throw
}

// move_backward(BDI, BDI, BDO)

template <typename BDI, typename BDO>
inline
BDO move_backward_dispatch(BDI first, BDI last, BDO dst,
                           std::true_type /*use_memmove*/)
{
    typedef typename boost::iterator_value<BDI>::type value_type;
    typename boost::iterator_difference<BDI>::type d = std::distance(first, last);

    BDO foo(dst - d);
    ::memmove(boost::addressof(*foo), boost::addressof(*first), sizeof(value_type) * d);
    return foo;
}

template <typename BDI, typename BDO>
inline
BDO move_backward_dispatch(BDI first, BDI last, BDO dst,
                           std::false_type /*use_memmove*/)
{
    return boost::move_backward(first, last, dst);                                // may throw
}

template <typename BDI, typename BDO>
inline
BDO move_backward(BDI first, BDI last, BDO dst)
{
    return move_backward_dispatch(first, last, dst, is_memop_safe_for_range<BDI, BDO>()); // may throw
}

template <typename T>
struct has_nothrow_move
    : std::integral_constant
        <
            bool,
            ::boost::has_nothrow_move<std::remove_const_t<T> >::value
            ||
            ::boost::has_nothrow_move<T>::value
        >
{};

// uninitialized_move_if_noexcept(I, I, O)

template <typename I, typename O>
inline
O uninitialized_move_if_noexcept_dispatch(I first, I last, O dst,
                                          std::true_type /*use_move*/)
{
    return varray_detail::uninitialized_move(first, last, dst);
}

template <typename I, typename O>
inline
O uninitialized_move_if_noexcept_dispatch(I first, I last, O dst,
                                          std::false_type const& /*use_move*/)
{
    return varray_detail::uninitialized_copy(first, last, dst);
}

template <typename I, typename O>
inline
O uninitialized_move_if_noexcept(I first, I last, O dst)
{
    typedef has_nothrow_move<
        typename ::boost::iterator_value<O>::type
    > use_move;

    return uninitialized_move_if_noexcept_dispatch(first, last, dst, use_move());         // may throw
}

// move_if_noexcept(I, I, O)

template <typename I, typename O>
inline
O move_if_noexcept_dispatch(I first, I last, O dst,
                            std::true_type /*use_move*/)
{
    return varray_detail::move(first, last, dst);
}

template <typename I, typename O>
inline
O move_if_noexcept_dispatch(I first, I last, O dst,
                            std::false_type /*use_move*/)
{
    return varray_detail::copy(first, last, dst);
}

template <typename I, typename O>
inline
O move_if_noexcept(I first, I last, O dst)
{
    typedef has_nothrow_move<
        typename ::boost::iterator_value<O>::type
    > use_move;

    return move_if_noexcept_dispatch(first, last, dst, use_move());         // may throw
}

// uninitialized_fill(I, I)

template <typename I>
inline
void uninitialized_fill_dispatch(I /*first*/, I /*last*/,
                                 std::true_type const& /*has_trivial_constructor*/,
                                 std::true_type const& /*disable_trivial_init*/)
{}

template <typename I>
inline
void uninitialized_fill_dispatch(I first, I last,
                                 std::true_type const& /*has_trivial_constructor*/,
                                 std::false_type const& /*disable_trivial_init*/)
{
    typedef typename boost::iterator_value<I>::type value_type;
    for ( ; first != last ; ++first )
        new (boost::addressof(*first)) value_type();
}

template <typename I, typename DisableTrivialInit>
inline
void uninitialized_fill_dispatch(I first, I last,
                                 std::false_type const& /*has_trivial_constructor*/,
                                 DisableTrivialInit const& /*not_used*/)
{
    typedef typename boost::iterator_value<I>::type value_type;
    I it = first;

    BOOST_TRY
    {
        for ( ; it != last ; ++it )
            new (boost::addressof(*it)) value_type();                           // may throw
    }
    BOOST_CATCH(...)
    {
        varray_detail::destroy(first, it);
        BOOST_RETHROW;
    }
    BOOST_CATCH_END
}

template <typename I, typename DisableTrivialInit>
inline
void uninitialized_fill(I first, I last, DisableTrivialInit const& disable_trivial_init)
{
    typedef typename boost::iterator_value<I>::type value_type;
    uninitialized_fill_dispatch(first, last, std::is_trivially_constructible<value_type>(), disable_trivial_init);     // may throw
}

// construct(I)

template <typename I>
inline
void construct_dispatch(std::true_type /*dont_init*/, I /*pos*/)
{}

template <typename I>
inline
void construct_dispatch(std::false_type /*dont_init*/, I pos)
{
    typedef typename ::boost::iterator_value<I>::type value_type;
    new (static_cast<void*>(::boost::addressof(*pos))) value_type();                      // may throw
}

template <typename DisableTrivialInit, typename I>
inline
void construct(DisableTrivialInit const&, I pos)
{
    typedef typename ::boost::iterator_value<I>::type value_type;
    typedef std::integral_constant
        <
            bool,
            std::is_trivially_constructible<value_type>::value
            &&
            DisableTrivialInit::value
        > dont_init;

    construct_dispatch(dont_init(), pos);                                                // may throw
}

// construct(I, V)

template <typename I, typename V>
inline
void construct_copy_dispatch(I pos, V const& v,
                             std::true_type /*use_memcpy*/)
{
    ::memcpy(boost::addressof(*pos), boost::addressof(v), sizeof(V));
}

template <typename I, typename P>
inline
void construct_copy_dispatch(I pos, P const& p,
                             std::false_type const& /*use_memcpy*/)
{
    typedef typename boost::iterator_value<I>::type V;
    new (static_cast<void*>(boost::addressof(*pos))) V(p);                      // may throw
}

template <typename DisableTrivialInit, typename I, typename P>
inline
void construct(DisableTrivialInit const&,
               I pos, P const& p)
{
    construct_copy_dispatch(pos, p, is_memop_safe_for_value<I, P>());           // may throw
}

// Needed by push_back(V &&)

template <typename I, typename V>
inline
void construct_move_dispatch(I pos, V const& v,
                             std::true_type const& /*use_memcpy*/)
{
    ::memcpy(boost::addressof(*pos), boost::addressof(v), sizeof(V));
}

template <typename I, typename P>
inline
void construct_move_dispatch(I pos, BOOST_RV_REF(P) p,
                             std::false_type const& /*use_memcpy*/)
{
    typedef typename boost::iterator_value<I>::type V;
    new (static_cast<void*>(boost::addressof(*pos))) V(::boost::move(p));       // may throw
}

template <typename DisableTrivialInit, typename I, typename P>
inline
void construct(DisableTrivialInit const&, I pos, BOOST_RV_REF(P) p)
{
    construct_move_dispatch(pos, ::boost::move(p), is_memop_safe_for_value<I, P>()); // may throw
}

// Needed by emplace_back() and emplace()

#if !defined(BOOST_CONTAINER_VARRAY_DISABLE_EMPLACE)
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

template <typename DisableTrivialInit, typename I, class ...Args>
inline
void construct(DisableTrivialInit const&,
               I pos,
               BOOST_FWD_REF(Args) ...args)
{
    typedef typename boost::iterator_value<I>::type V;
    new (static_cast<void*>(boost::addressof(*pos))) V(::boost::forward<Args>(args)...);    // may throw
}

#else // !BOOST_NO_CXX11_VARIADIC_TEMPLATES

// BOOST_NO_CXX11_RVALUE_REFERENCES -> P0 const& p0
// !BOOST_NO_CXX11_RVALUE_REFERENCES -> P0 && p0
// which means that version with one parameter may take V const& v

#define BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_DETAIL_CONSTRUCT(N)                                      \
template <typename DisableTrivialInit, typename I, typename P BOOST_MOVE_I##N BOOST_MOVE_CLASS##N > \
inline                                                                                              \
void construct(DisableTrivialInit const&,                                                           \
               I pos,                                                                               \
               BOOST_FWD_REF(P) p                                                                   \
               BOOST_MOVE_I##N BOOST_MOVE_UREF##N)                                                  \
{                                                                                                   \
    typedef typename boost::iterator_value<I>::type V;                                              \
    new                                                                                             \
    (static_cast<void*>(boost::addressof(*pos)))                                                    \
    V(boost::forward<P>(p) BOOST_MOVE_I##N BOOST_MOVE_FWD##N);                    /*may throw*/    \
}                                                                                                   \

BOOST_MOVE_ITERATE_1TO9(BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_DETAIL_CONSTRUCT)
#undef BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_DETAIL_CONSTRUCT

#endif // !BOOST_NO_CXX11_VARIADIC_TEMPLATES
#endif // !BOOST_CONTAINER_VARRAY_DISABLE_EMPLACE

// assign(I, V)

template <typename I, typename V>
inline
void assign_copy_dispatch(I pos, V const& v,
                          std::true_type /*use_memcpy*/)
{
// TODO - use memmove here?
    ::memcpy(boost::addressof(*pos), boost::addressof(v), sizeof(V));
}

template <typename I, typename V>
inline
void assign_copy_dispatch(I pos, V const& v,
                          std::false_type /*use_memcpy*/)
{
    *pos = v;                                                                   // may throw
}

template <typename I, typename V>
inline
void assign(I pos, V const& v)
{
    assign_copy_dispatch(pos, v, is_memop_safe_for_value<I, V>());              // may throw
}

template <typename I, typename V>
inline
void assign_move_dispatch(I pos, V const& v,
                          std::true_type /*use_memcpy*/)
{
// TODO - use memmove here?
    ::memcpy(boost::addressof(*pos), boost::addressof(v), sizeof(V));
}

template <typename I, typename V>
inline
void assign_move_dispatch(I pos, BOOST_RV_REF(V) v,
                          std::false_type /*use_memcpy*/)
{
    *pos = boost::move(v);                                                        // may throw
}

template <typename I, typename V>
inline
void assign(I pos, BOOST_RV_REF(V) v)
{
    assign_move_dispatch(pos, ::boost::move(v), is_memop_safe_for_value<I, V>());
}

// uninitialized_copy_s

template <typename I, typename F>
inline std::size_t uninitialized_copy_s(I first, I last, F dest, std::size_t max_count)
{
    std::size_t count = 0;
    F it = dest;

    BOOST_TRY
    {
        for ( ; first != last ; ++it, ++first, ++count )
        {
            if ( max_count <= count )
                return (std::numeric_limits<std::size_t>::max)();

            // dummy 0 as DisableTrivialInit
            construct(0, it, *first);                                              // may throw
        }
    }
    BOOST_CATCH(...)
    {
        varray_detail::destroy(dest, it);
        BOOST_RETHROW;
    }
    BOOST_CATCH_END

    return count;
}

// scoped_destructor

template<class T>
class scoped_destructor
{
public:
    scoped_destructor(T * ptr) : m_ptr(ptr) {}

    ~scoped_destructor()
    {
        if(m_ptr)
            varray_detail::destroy(m_ptr);
    }

    void release() { m_ptr = 0; }

private:
    T * m_ptr;
};

}}}}} // namespace boost::geometry::index::detail::varray_detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_DETAIL_HPP
