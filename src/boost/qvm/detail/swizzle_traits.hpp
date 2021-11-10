#ifndef BOOST_QVM_DETAIL_SWIZZLE_TRAITS_HPP_INCLUDED
#define BOOST_QVM_DETAIL_SWIZZLE_TRAITS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/deduce_vec.hpp>
#include <boost/qvm/enable_if.hpp>
#include <boost/qvm/assert.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    BOOST_QVM_INLINE_CRITICAL
    void const *
    get_null()
        {
        static int const obj=0;
        return &obj;
        }

    template <int A,class Next=void> struct swizzle_idx { static int const value=A; typedef Next next; };

    template <class V,int Idx>
    struct
    const_value
        {
        static
        BOOST_QVM_INLINE_TRIVIAL
        typename vec_traits<V>::scalar_type
        value()
            {
            BOOST_QVM_ASSERT(0);
            return typename vec_traits<V>::scalar_type();
            }
        };

    template <class V>
    struct
    const_value<V,-1>
        {
        static
        BOOST_QVM_INLINE_TRIVIAL
        typename vec_traits<V>::scalar_type
        value()
            {
            return scalar_traits<typename vec_traits<V>::scalar_type>::value(0);
            }
        };

    template <class V>
    struct
    const_value<V,-2>
        {
        static
        BOOST_QVM_INLINE_TRIVIAL
        typename vec_traits<V>::scalar_type
        value()
            {
            return scalar_traits<typename vec_traits<V>::scalar_type>::value(1);
            }
        };

    template <int Index,bool Neg=(Index<0)>
    struct neg_zero;

    template <int Index>
    struct
    neg_zero<Index,true>
        {
        static int const value=0;
        };

    template <int Index>
    struct
    neg_zero<Index,false>
        {
        static int const value=Index;
        };

    template <class SwizzleList,int Index,int C=0>
    struct
    swizzle
        {
        static int const value=swizzle<typename SwizzleList::next,Index,C+1>::value;
        };

    template <class SwizzleList,int Match>
    struct
    swizzle<SwizzleList,Match,Match>
        {
        static int const value=SwizzleList::value;
        };

    template <int Index,int C>
    struct swizzle<void,Index,C>;

    template <class SwizzleList,int C=0>
    struct
    swizzle_list_length
        {
        static int const value=swizzle_list_length<typename SwizzleList::next,C+1>::value;
        };

    template <int C>
    struct
    swizzle_list_length<void,C>
        {
        static int const value=C;
        };

    template <class SwizzleList,int D>
    struct
    validate_swizzle_list
        {
        static bool const value =
            ((SwizzleList::value)<D) && //don't touch extra (), MSVC parsing bug.
            validate_swizzle_list<typename SwizzleList::next,D>::value;
        };

    template <int D>
    struct
    validate_swizzle_list<void,D>
        {
        static bool const value=true;
        };

    template <class OriginalType,class SwizzleList>
    class
    sw_
        {
        sw_( sw_ const & );
        sw_ & operator=( sw_ const & );
        ~sw_();

        BOOST_QVM_STATIC_ASSERT((validate_swizzle_list<SwizzleList,vec_traits<OriginalType>::dim>::value));

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        sw_ &
        operator=( T const & x )
            {
            assign(*this,x);
            return *this;
            }

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        operator R() const
            {
            R r;
            assign(r,*this);
            return r;
            }
        };

    template <class SwizzleList>
    class
    sw01_
        {
        sw01_( sw01_ const & );
        sw01_ & operator=( sw01_ const & );
        ~sw01_();

        public:

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        operator R() const
            {
            R r;
            assign(r,*this);
            return r;
            }
        };

    template <class OriginalType,class SwizzleList>
    class
    sws_
        {
        sws_( sws_ const & );
        sws_ & operator=( sws_ const & );
        ~sws_();

        BOOST_QVM_STATIC_ASSERT((validate_swizzle_list<SwizzleList,1>::value));

        public:

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        operator R() const
            {
            R r;
            assign(r,*this);
            return r;
            }
        };
    }

template <class OriginalVector,class SwizzleList>
struct
vec_traits<qvm_detail::sw_<OriginalVector,SwizzleList> >
    {
    typedef qvm_detail::sw_<OriginalVector,SwizzleList> this_vector;
    typedef typename vec_traits<OriginalVector>::scalar_type scalar_type;
    static int const dim=qvm_detail::swizzle_list_length<SwizzleList>::value;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        int const idx=qvm_detail::swizzle<SwizzleList,I>::value;
        BOOST_QVM_STATIC_ASSERT(idx<vec_traits<OriginalVector>::dim);
        return idx>=0?
            vec_traits<OriginalVector>::template read_element<qvm_detail::neg_zero<idx>::value>(reinterpret_cast<OriginalVector const &>(x)) :
            qvm_detail::const_value<this_vector,idx>::value();
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_vector & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        int const idx=qvm_detail::swizzle<SwizzleList,I>::value;
        BOOST_QVM_STATIC_ASSERT(idx>=0);
        BOOST_QVM_STATIC_ASSERT(idx<vec_traits<OriginalVector>::dim);
        return vec_traits<OriginalVector>::template write_element<idx>(reinterpret_cast<OriginalVector &>(x));
        }
    };

template <class SwizzleList>
struct
vec_traits<qvm_detail::sw01_<SwizzleList> >
    {
    typedef qvm_detail::sw01_<SwizzleList> this_vector;
    typedef int scalar_type;
    static int const dim=qvm_detail::swizzle_list_length<SwizzleList>::value;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        int const idx=qvm_detail::swizzle<SwizzleList,I>::value;
        BOOST_QVM_STATIC_ASSERT(idx<0);
        return qvm_detail::const_value<this_vector,idx>::value();
        }
    };

template <class OriginalScalar,class SwizzleList>
struct
vec_traits<qvm_detail::sws_<OriginalScalar,SwizzleList> >
    {
    typedef qvm_detail::sws_<OriginalScalar,SwizzleList> this_vector;
    typedef OriginalScalar scalar_type;
    static int const dim=qvm_detail::swizzle_list_length<SwizzleList>::value;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        int const idx=qvm_detail::swizzle<SwizzleList,I>::value;
        BOOST_QVM_STATIC_ASSERT(idx<1);
        return idx==0?
            reinterpret_cast<OriginalScalar const &>(x) :
            qvm_detail::const_value<this_vector,idx>::value();
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_vector & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        int const idx=qvm_detail::swizzle<SwizzleList,I>::value;
        BOOST_QVM_STATIC_ASSERT(idx==0);
        return reinterpret_cast<OriginalScalar &>(x);
        }
    };

template <class OriginalVector,class SwizzleList,int D>
struct
deduce_vec<qvm_detail::sw_<OriginalVector,SwizzleList>,D>
    {
    typedef vec<typename vec_traits<OriginalVector>::scalar_type,D> type;
    };

template <class OriginalVector,class SwizzleList,int D>
struct
deduce_vec2<qvm_detail::sw_<OriginalVector,SwizzleList>,qvm_detail::sw_<OriginalVector,SwizzleList>,D>
    {
    typedef vec<typename vec_traits<OriginalVector>::scalar_type,D> type;
    };

template <class Scalar,class SwizzleList,int D>
struct
deduce_vec<qvm_detail::sws_<Scalar,SwizzleList>,D>
    {
    typedef vec<Scalar,D> type;
    };

template <class Scalar,class SwizzleList,int D>
struct
deduce_vec2<qvm_detail::sws_<Scalar,SwizzleList>,qvm_detail::sws_<Scalar,SwizzleList>,D>
    {
    typedef vec<Scalar,D> type;
    };

} }

#endif
