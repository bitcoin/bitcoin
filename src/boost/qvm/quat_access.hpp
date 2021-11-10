#ifndef BOOST_QVM_QUAT_ACCESS_HPP_INCLUDED
#define BOOST_QVM_QUAT_ACCESS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/quat_traits.hpp>
#include <boost/qvm/deduce_vec.hpp>
#include <boost/qvm/static_assert.hpp>
#include <boost/qvm/enable_if.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    template <class Q>
    struct
    quat_v_
        {
        template <class R>
        operator R() const
            {
            R r;
            assign(r,*this);
            return r;
            }

        private:

        quat_v_( quat_v_ const & );
        quat_v_ const & operator=( quat_v_ const & );
        ~quat_v_();
        };
    }

template <class V>
struct vec_traits;

template <class Q>
struct
vec_traits< qvm_detail::quat_v_<Q> >
    {
    typedef qvm_detail::quat_v_<Q> this_vector;
    typedef typename quat_traits<Q>::scalar_type scalar_type;
    static int const dim=3;

    template <int I>
    BOOST_QVM_INLINE_CRITICAL
    static
    scalar_type
    read_element( this_vector const & q )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return quat_traits<Q>::template read_element<I+1>( reinterpret_cast<Q const &>(q) );
        }

    template <int I>
    BOOST_QVM_INLINE_CRITICAL
    static
    scalar_type &
    write_element( this_vector & q )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return quat_traits<Q>::template write_element<I+1>( reinterpret_cast<Q &>(q) );
        }
    };

template <class Q,int D>
struct
deduce_vec<qvm_detail::quat_v_<Q>,D>
    {
    typedef vec<typename quat_traits<Q>::scalar_type,D> type;
    };

template <class Q,int D>
struct
deduce_vec2<qvm_detail::quat_v_<Q>,qvm_detail::quat_v_<Q>,D>
    {
    typedef vec<typename quat_traits<Q>::scalar_type,D> type;
    };

template <class Q>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_quat<Q>::value,
    qvm_detail::quat_v_<Q> const &>::type
V( Q const & a )
    {
    return reinterpret_cast<qvm_detail::quat_v_<Q> const &>(a);
    }

template <class Q>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_quat<Q>::value,
    qvm_detail::quat_v_<Q> &>::type
V( Q & a )
    {
    return reinterpret_cast<qvm_detail::quat_v_<Q> &>(a);
    }

template <class Q> BOOST_QVM_INLINE_TRIVIAL typename enable_if_c<is_quat<Q>::value,typename quat_traits<Q>::scalar_type>::type S( Q const & a ) { return quat_traits<Q>::template read_element<0>(a); }
template <class Q> BOOST_QVM_INLINE_TRIVIAL typename enable_if_c<is_quat<Q>::value,typename quat_traits<Q>::scalar_type>::type X( Q const & a ) { return quat_traits<Q>::template read_element<1>(a); }
template <class Q> BOOST_QVM_INLINE_TRIVIAL typename enable_if_c<is_quat<Q>::value,typename quat_traits<Q>::scalar_type>::type Y( Q const & a ) { return quat_traits<Q>::template read_element<2>(a); }
template <class Q> BOOST_QVM_INLINE_TRIVIAL typename enable_if_c<is_quat<Q>::value,typename quat_traits<Q>::scalar_type>::type Z( Q const & a ) { return quat_traits<Q>::template read_element<3>(a); }

template <class Q> BOOST_QVM_INLINE_TRIVIAL typename enable_if_c<is_quat<Q>::value,typename quat_traits<Q>::scalar_type &>::type S( Q & a ) { return quat_traits<Q>::template write_element<0>(a); }
template <class Q> BOOST_QVM_INLINE_TRIVIAL typename enable_if_c<is_quat<Q>::value,typename quat_traits<Q>::scalar_type &>::type X( Q & a ) { return quat_traits<Q>::template write_element<1>(a); }
template <class Q> BOOST_QVM_INLINE_TRIVIAL typename enable_if_c<is_quat<Q>::value,typename quat_traits<Q>::scalar_type &>::type Y( Q & a ) { return quat_traits<Q>::template write_element<2>(a); }
template <class Q> BOOST_QVM_INLINE_TRIVIAL typename enable_if_c<is_quat<Q>::value,typename quat_traits<Q>::scalar_type &>::type Z( Q & a ) { return quat_traits<Q>::template write_element<3>(a); }

} }

#endif
