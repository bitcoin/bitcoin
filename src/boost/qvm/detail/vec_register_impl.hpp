#ifndef BOOST_QVM_DETAIL_VEC_REGISTER_IMPL_HPP
#define BOOST_QVM_DETAIL_VEC_REGISTER_IMPL_HPP

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.
/// Copyright (c) 2018 agate-pris

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/assert.hpp>
#include <boost/qvm/inline.hpp>
#include <boost/qvm/static_assert.hpp>
#include <boost/qvm/vec_traits.hpp>

namespace boost { namespace qvm { namespace qvm_detail {

template<class VecType, class ScalarType, int Dim>
struct vec_register_common
{
    typedef VecType vec_type;
    typedef ScalarType scalar_type;
    static int const dim = Dim;
};

template<class VecType, class ScalarType, int Dim>
struct vec_register_read
{
    template<int I> static ScalarType read_element(VecType const& v);

    template<int I, int N> struct read_element_idx_detail
    {
        static BOOST_QVM_INLINE_CRITICAL ScalarType impl(int const i, VecType const& v)
        {
            return I == i
                ? read_element<I>(v)
                : read_element_idx_detail<I + 1, N>::impl(i, v);
        }
    };

    template<int N> struct read_element_idx_detail<N, N>
    {
        static BOOST_QVM_INLINE_TRIVIAL ScalarType impl(int, VecType const& v)
        {
            BOOST_QVM_ASSERT(0);
            return read_element<0>(v);
        }
    };

    static BOOST_QVM_INLINE_CRITICAL ScalarType read_element_idx(int const i, VecType const& v)
    {
        return read_element_idx_detail<0, Dim>::impl(i, v);
    }
};

template<class VecType, class ScalarType, int Dim>
struct vec_register_write
{
    template<int I> static ScalarType& write_element(VecType& v);

    template<int I, int N> struct write_element_idx_detail
    {
        static BOOST_QVM_INLINE_CRITICAL ScalarType& impl(int const i, VecType& v)
        {
            return I == i
                ? write_element<I>(v)
                : write_element_idx_detail<I + 1, N>::impl(i, v);
        }
    };

    template<int N> struct write_element_idx_detail<N, N>
    {
        static BOOST_QVM_INLINE_TRIVIAL ScalarType& impl(int, VecType& v)
        {
            BOOST_QVM_ASSERT(0);
            return write_element<0>(v);
        }
    };

    static BOOST_QVM_INLINE_CRITICAL ScalarType& write_element_idx(int const i, VecType& v)
    {
        return write_element_idx_detail<0, Dim>::impl(i, v);
    }
};

}}}

#define BOOST_QVM_DETAIL_SPECIALIZE_QVM_DETAIL_VEC_REGISTER_READ(VecType, ScalarType, Dim, I, Read) \
namespace boost { namespace qvm {namespace qvm_detail{ \
template<> \
template<> \
BOOST_QVM_INLINE_CRITICAL \
ScalarType vec_register_read<VecType, ScalarType, Dim>::read_element<I>(VecType const& v) \
{ \
    BOOST_QVM_STATIC_ASSERT(I>=0); \
    BOOST_QVM_STATIC_ASSERT(I<Dim); \
    return v. Read; \
} \
}}}

#define BOOST_QVM_DETAIL_SPECIALIZE_QVM_DETAIL_VEC_REGISTER_WRITE(VecType, ScalarType, Dim, I, Write) \
namespace boost { namespace qvm {namespace qvm_detail{ \
template<> \
template<> \
BOOST_QVM_INLINE_CRITICAL \
ScalarType& vec_register_write<VecType, ScalarType, Dim>::write_element<I>(VecType& v) \
{ \
    BOOST_QVM_STATIC_ASSERT(I>=0); \
    BOOST_QVM_STATIC_ASSERT(I<Dim); \
    return v. Write; \
}; \
}}}

#define BOOST_QVM_DETAIL_SPECIALIZE_QVM_DETAIL_VEC_REGISTER_READ_WRITE(VecType, ScalarType, Dim, I, Read, Write)\
BOOST_QVM_DETAIL_SPECIALIZE_QVM_DETAIL_VEC_REGISTER_READ(VecType, ScalarType, Dim, I, Read) \
BOOST_QVM_DETAIL_SPECIALIZE_QVM_DETAIL_VEC_REGISTER_WRITE(VecType, ScalarType, Dim, I, Write)

#define BOOST_QVM_DETAIL_REGISTER_VEC_SPECIALIZE_VEC_TRAITS_READ(VecType, ScalarType, Dim) \
namespace boost { namespace qvm { \
template<> \
struct vec_traits<VecType> \
: qvm_detail::vec_register_common<VecType, ScalarType, Dim> \
, qvm_detail::vec_register_read<VecType, ScalarType, Dim> \
{ \
}; \
}}

#define BOOST_QVM_DETAIL_REGISTER_VEC_SPECIALIZE_VEC_TRAITS_READ_WRITE(VecType, ScalarType, Dim)\
namespace boost { namespace qvm { \
template<> \
struct vec_traits<VecType> \
: qvm_detail::vec_register_common<VecType, ScalarType, Dim> \
, qvm_detail::vec_register_read<VecType, ScalarType, Dim> \
, qvm_detail::vec_register_write<VecType, ScalarType, Dim> \
{ \
}; \
}}

#endif
