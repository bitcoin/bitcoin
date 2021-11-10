#ifndef BOOST_QVM_QUAT_OPERATIONS
#define BOOST_QVM_QUAT_OPERATIONS

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/detail/quat_assign.hpp>
#include <boost/qvm/deduce_quat.hpp>
#include <boost/qvm/mat_traits.hpp>
#include <boost/qvm/scalar_traits.hpp>
#include <boost/qvm/math.hpp>
#include <boost/qvm/assert.hpp>
#include <boost/qvm/error.hpp>
#include <boost/qvm/throw_exception.hpp>
#include <string>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    BOOST_QVM_INLINE_CRITICAL
    void const *
    get_valid_ptr_quat_operations()
        {
        static int const obj=0;
        return &obj;
        }
    }

////////////////////////////////////////////////

namespace
msvc_parse_bug_workaround
    {
    template <class A,class B>
    struct
    quats
        {
        static bool const value=is_quat<A>::value && is_quat<B>::value;
        };
    }

namespace
qvm_to_string_detail
    {
    template <class T>
    std::string to_string( T const & x );
    }

template <class A>
inline
typename enable_if_c<
    is_quat<A>::value,
    std::string>::type
to_string( A const & a )
    {
    using namespace qvm_to_string_detail;
    return '('+
        to_string(quat_traits<A>::template read_element<0>(a))+','+
        to_string(quat_traits<A>::template read_element<1>(a))+','+
        to_string(quat_traits<A>::template read_element<2>(a))+','+
        to_string(quat_traits<A>::template read_element<3>(a))+')';
    }

////////////////////////////////////////////////

template <class A,class B,class Cmp>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value && is_quat<B>::value,
    bool>::type
cmp( A const & a, B const & b, Cmp f )
    {
    typedef typename quat_traits<A>::scalar_type T;
    typedef typename quat_traits<B>::scalar_type U;
    T q1[4] =
        {
        quat_traits<A>::template read_element<0>(a),
        quat_traits<A>::template read_element<1>(a),
        quat_traits<A>::template read_element<2>(a),
        quat_traits<A>::template read_element<3>(a)
        };
    U q2[4] =
        {
        quat_traits<B>::template read_element<0>(b),
        quat_traits<B>::template read_element<1>(b),
        quat_traits<B>::template read_element<2>(b),
        quat_traits<B>::template read_element<3>(b)
        };
    int i;
    for( i=0; i!=4; ++i )
        if( !f(q1[i],q2[i]) )
            break;
    if( i==4 )
        return true;
    for( i=0; i!=4; ++i )
        if( !f(q1[i],-q2[i]) )
            return false;
    return true;
    }

////////////////////////////////////////////////

template <class R,class A>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_quat<R>::value && is_quat<A>::value,
    R>::type
convert_to( A const & a )
    {
    R r;
    quat_traits<R>::template write_element<0>(r) = quat_traits<A>::template read_element<0>(a);
    quat_traits<R>::template write_element<1>(r) = quat_traits<A>::template read_element<1>(a);
    quat_traits<R>::template write_element<2>(r) = quat_traits<A>::template read_element<2>(a);
    quat_traits<R>::template write_element<3>(r) = quat_traits<A>::template read_element<3>(a);
    return r;
    }

template <class R,class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<R>::value && is_mat<A>::value &&
    mat_traits<A>::rows==3 && mat_traits<A>::cols==3,
    R>::type
convert_to( A const & a )
    {
    typedef typename mat_traits<A>::scalar_type T;
    T const mat[3][3] =
        {
            { mat_traits<A>::template read_element<0,0>(a), mat_traits<A>::template read_element<0,1>(a), mat_traits<A>::template read_element<0,2>(a) },
            { mat_traits<A>::template read_element<1,0>(a), mat_traits<A>::template read_element<1,1>(a), mat_traits<A>::template read_element<1,2>(a) },
            { mat_traits<A>::template read_element<2,0>(a), mat_traits<A>::template read_element<2,1>(a), mat_traits<A>::template read_element<2,2>(a) }
        };
    R r;
    if( mat[0][0]+mat[1][1]+mat[2][2] > scalar_traits<T>::value(0) )
        {
        T t = mat[0][0] + mat[1][1] + mat[2][2] + scalar_traits<T>::value(1);
        T s = (scalar_traits<T>::value(1)/sqrt<T>(t))/2;
        quat_traits<R>::template write_element<0>(r)=s*t;
        quat_traits<R>::template write_element<1>(r)=(mat[2][1]-mat[1][2])*s;
        quat_traits<R>::template write_element<2>(r)=(mat[0][2]-mat[2][0])*s;
        quat_traits<R>::template write_element<3>(r)=(mat[1][0]-mat[0][1])*s;
        }
    else if( mat[0][0]>mat[1][1] && mat[0][0]>mat[2][2] )
        {
        T t = mat[0][0] - mat[1][1] - mat[2][2] + scalar_traits<T>::value(1);
        T s = (scalar_traits<T>::value(1)/sqrt<T>(t))/2;
        quat_traits<R>::template write_element<0>(r)=(mat[2][1]-mat[1][2])*s;
        quat_traits<R>::template write_element<1>(r)=s*t;
        quat_traits<R>::template write_element<2>(r)=(mat[1][0]+mat[0][1])*s;
        quat_traits<R>::template write_element<3>(r)=(mat[0][2]+mat[2][0])*s;
        }
    else if( mat[1][1]>mat[2][2] )
        {
        T t = - mat[0][0] + mat[1][1] - mat[2][2] + scalar_traits<T>::value(1);
        T s = (scalar_traits<T>::value(1)/sqrt<T>(t))/2;
        quat_traits<R>::template write_element<0>(r)=(mat[0][2]-mat[2][0])*s;
        quat_traits<R>::template write_element<1>(r)=(mat[1][0]+mat[0][1])*s;
        quat_traits<R>::template write_element<2>(r)=s*t;
        quat_traits<R>::template write_element<3>(r)=(mat[2][1]+mat[1][2])*s;
        }
    else
        {
        T t = - mat[0][0] - mat[1][1] + mat[2][2] + scalar_traits<T>::value(1);
        T s = (scalar_traits<T>::value(1)/sqrt<T>(t))/2;
        quat_traits<R>::template write_element<0>(r)=(mat[1][0]-mat[0][1])*s;
        quat_traits<R>::template write_element<1>(r)=(mat[0][2]+mat[2][0])*s;
        quat_traits<R>::template write_element<2>(r)=(mat[2][1]+mat[1][2])*s;
        quat_traits<R>::template write_element<3>(r)=s*t;
        }
    return r;
    }

////////////////////////////////////////////////

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value,
    deduce_quat<A> >::type
conjugate( A const & a )
    {
    typedef typename deduce_quat<A>::type R;
    R r;
    quat_traits<R>::template write_element<0>(r)=quat_traits<A>::template read_element<0>(a);
    quat_traits<R>::template write_element<1>(r)=-quat_traits<A>::template read_element<1>(a);
    quat_traits<R>::template write_element<2>(r)=-quat_traits<A>::template read_element<2>(a);
    quat_traits<R>::template write_element<3>(r)=-quat_traits<A>::template read_element<3>(a);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T>
    class
    identity_quat_
        {
        identity_quat_( identity_quat_ const & );
        identity_quat_ & operator=( identity_quat_ const & );
        ~identity_quat_();

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

template <class T>
struct
quat_traits< qvm_detail::identity_quat_<T> >
    {
    typedef qvm_detail::identity_quat_<T> this_quaternion;
    typedef T scalar_type;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_quaternion const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return scalar_traits<T>::value(I==0);
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_quaternion const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<4);
        return scalar_traits<T>::value(i==0);
        }
    };

template <class T>
struct
deduce_quat< qvm_detail::identity_quat_<T> >
    {
    typedef quat<T> type;
    };

template <class T>
struct
deduce_quat2< qvm_detail::identity_quat_<T>, qvm_detail::identity_quat_<T> >
    {
    typedef quat<T> type;
    };

template <class T>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::identity_quat_<T> const &
identity_quat()
    {
    return *(qvm_detail::identity_quat_<T> const *)qvm_detail::get_valid_ptr_quat_operations();
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    void>::type
set_identity( A & a )
    {
    typedef typename quat_traits<A>::scalar_type T;
    T const zero=scalar_traits<T>::value(0);
    T const one=scalar_traits<T>::value(1);
    quat_traits<A>::template write_element<0>(a) = one;
    quat_traits<A>::template write_element<1>(a) = zero;
    quat_traits<A>::template write_element<2>(a) = zero;
    quat_traits<A>::template write_element<3>(a) = zero;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class OriginalType,class Scalar>
    class
    quaternion_scalar_cast_
        {
        quaternion_scalar_cast_( quaternion_scalar_cast_ const & );
        quaternion_scalar_cast_ & operator=( quaternion_scalar_cast_ const & );
        ~quaternion_scalar_cast_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        quaternion_scalar_cast_ &
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

    template <bool> struct scalar_cast_quaternion_filter { };
    template <> struct scalar_cast_quaternion_filter<true> { typedef int type; };
    }

template <class OriginalType,class Scalar>
struct
quat_traits< qvm_detail::quaternion_scalar_cast_<OriginalType,Scalar> >
    {
    typedef Scalar scalar_type;
    typedef qvm_detail::quaternion_scalar_cast_<OriginalType,Scalar> this_quaternion;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_quaternion const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return scalar_type(quat_traits<OriginalType>::template read_element<I>(reinterpret_cast<OriginalType const &>(x)));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_quaternion const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<4);
        return scalar_type(quat_traits<OriginalType>::read_element_idx(i,reinterpret_cast<OriginalType const &>(x)));
        }
    };

template <class Scalar,class T>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::quaternion_scalar_cast_<T,Scalar> const &
scalar_cast( T const & x, typename qvm_detail::scalar_cast_quaternion_filter<is_quat<T>::value>::type=0 )
    {
    return reinterpret_cast<qvm_detail::quaternion_scalar_cast_<T,Scalar> const &>(x);
    }

////////////////////////////////////////////////

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value && is_scalar<B>::value,
    A &>::type
operator/=( A & a, B b )
    {
    quat_traits<A>::template write_element<0>(a)/=b;
    quat_traits<A>::template write_element<1>(a)/=b;
    quat_traits<A>::template write_element<2>(a)/=b;
    quat_traits<A>::template write_element<3>(a)/=b;
    return a;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value && is_scalar<B>::value,
    deduce_quat2<A,B> >::type
operator/( A const & a, B b )
    {
    typedef typename deduce_quat2<A,B>::type R;
    R r;
    quat_traits<R>::template write_element<0>(r) = quat_traits<A>::template read_element<0>(a)/b;
    quat_traits<R>::template write_element<1>(r) = quat_traits<A>::template read_element<1>(a)/b;
    quat_traits<R>::template write_element<2>(r) = quat_traits<A>::template read_element<2>(a)/b;
    quat_traits<R>::template write_element<3>(r) = quat_traits<A>::template read_element<3>(a)/b;
    return r;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value && is_quat<B>::value,
    deduce_scalar<typename quat_traits<A>::scalar_type,typename quat_traits<B>::scalar_type> >::type
dot( A const & a, B const & b )
    {
    typedef typename quat_traits<A>::scalar_type Ta;
    typedef typename quat_traits<B>::scalar_type Tb;
    typedef typename deduce_scalar<Ta,Tb>::type Tr;
    Ta const a0=quat_traits<A>::template read_element<0>(a);
    Ta const a1=quat_traits<A>::template read_element<1>(a);
    Ta const a2=quat_traits<A>::template read_element<2>(a);
    Ta const a3=quat_traits<A>::template read_element<3>(a);
    Tb const b0=quat_traits<B>::template read_element<0>(b);
    Tb const b1=quat_traits<B>::template read_element<1>(b);
    Tb const b2=quat_traits<B>::template read_element<2>(b);
    Tb const b3=quat_traits<B>::template read_element<3>(b);
    Tr const dp=a0*b0+a1*b1+a2*b2+a3*b3;
    return dp;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value && is_quat<B>::value,
    bool>::type
operator==( A const & a, B const & b )
    {
    return
        quat_traits<A>::template read_element<0>(a)==quat_traits<B>::template read_element<0>(b) &&
        quat_traits<A>::template read_element<1>(a)==quat_traits<B>::template read_element<1>(b) &&
        quat_traits<A>::template read_element<2>(a)==quat_traits<B>::template read_element<2>(b) &&
        quat_traits<A>::template read_element<3>(a)==quat_traits<B>::template read_element<3>(b);
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value,
    deduce_quat<A> >::type
inverse( A const & a )
    {
    typedef typename deduce_quat<A>::type R;
    typedef typename quat_traits<A>::scalar_type TA;
    TA aa = quat_traits<A>::template read_element<0>(a);
    TA ab = quat_traits<A>::template read_element<1>(a);
    TA ac = quat_traits<A>::template read_element<2>(a);
    TA ad = quat_traits<A>::template read_element<3>(a);
    TA m2 = ab*ab + ac*ac + ad*ad + aa*aa;
    if( m2==scalar_traits<TA>::value(0) )
        BOOST_QVM_THROW_EXCEPTION(zero_magnitude_error());
    TA rm=scalar_traits<TA>::value(1)/m2;
    R r;
    quat_traits<R>::template write_element<0>(r) = aa*rm;
    quat_traits<R>::template write_element<1>(r) = -ab*rm;
    quat_traits<R>::template write_element<2>(r) = -ac*rm;
    quat_traits<R>::template write_element<3>(r) = -ad*rm;
    return r;
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    typename quat_traits<A>::scalar_type>::type
mag_sqr( A const & a )
    {
    typedef typename quat_traits<A>::scalar_type T;
    T x=quat_traits<A>::template read_element<0>(a);
    T y=quat_traits<A>::template read_element<1>(a);
    T z=quat_traits<A>::template read_element<2>(a);
    T w=quat_traits<A>::template read_element<3>(a);
    return x*x+y*y+z*z+w*w;
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    typename quat_traits<A>::scalar_type>::type
mag( A const & a )
    {
    typedef typename quat_traits<A>::scalar_type T;
    T x=quat_traits<A>::template read_element<0>(a);
    T y=quat_traits<A>::template read_element<1>(a);
    T z=quat_traits<A>::template read_element<2>(a);
    T w=quat_traits<A>::template read_element<3>(a);
    return sqrt<T>(x*x+y*y+z*z+w*w);
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if<
    msvc_parse_bug_workaround::quats<A,B>,
    A &>::type
operator-=( A & a, B const & b )
    {
    quat_traits<A>::template write_element<0>(a)-=quat_traits<B>::template read_element<0>(b);
    quat_traits<A>::template write_element<1>(a)-=quat_traits<B>::template read_element<1>(b);
    quat_traits<A>::template write_element<2>(a)-=quat_traits<B>::template read_element<2>(b);
    quat_traits<A>::template write_element<3>(a)-=quat_traits<B>::template read_element<3>(b);
    return a;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value && is_quat<B>::value,
    deduce_quat2<A,B> >::type
operator-( A const & a, B const & b )
    {
    typedef typename deduce_quat2<A,B>::type R;
    R r;
    quat_traits<R>::template write_element<0>(r)=quat_traits<A>::template read_element<0>(a)-quat_traits<B>::template read_element<0>(b);
    quat_traits<R>::template write_element<1>(r)=quat_traits<A>::template read_element<1>(a)-quat_traits<B>::template read_element<1>(b);
    quat_traits<R>::template write_element<2>(r)=quat_traits<A>::template read_element<2>(a)-quat_traits<B>::template read_element<2>(b);
    quat_traits<R>::template write_element<3>(r)=quat_traits<A>::template read_element<3>(a)-quat_traits<B>::template read_element<3>(b);
    return r;
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value,
    deduce_quat<A> >::type
operator-( A const & a )
    {
    typedef typename deduce_quat<A>::type R;
    R r;
    quat_traits<R>::template write_element<0>(r)=-quat_traits<A>::template read_element<0>(a);
    quat_traits<R>::template write_element<1>(r)=-quat_traits<A>::template read_element<1>(a);
    quat_traits<R>::template write_element<2>(r)=-quat_traits<A>::template read_element<2>(a);
    quat_traits<R>::template write_element<3>(r)=-quat_traits<A>::template read_element<3>(a);
    return r;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if<
    msvc_parse_bug_workaround::quats<A,B>,
    A &>::type
operator*=( A & a, B const & b )
    {
    typedef typename quat_traits<A>::scalar_type TA;
    typedef typename quat_traits<B>::scalar_type TB;
    TA const aa=quat_traits<A>::template read_element<0>(a);
    TA const ab=quat_traits<A>::template read_element<1>(a);
    TA const ac=quat_traits<A>::template read_element<2>(a);
    TA const ad=quat_traits<A>::template read_element<3>(a);
    TB const ba=quat_traits<B>::template read_element<0>(b);
    TB const bb=quat_traits<B>::template read_element<1>(b);
    TB const bc=quat_traits<B>::template read_element<2>(b);
    TB const bd=quat_traits<B>::template read_element<3>(b);
    quat_traits<A>::template write_element<0>(a) = aa*ba - ab*bb - ac*bc - ad*bd;
    quat_traits<A>::template write_element<1>(a) = aa*bb + ab*ba + ac*bd - ad*bc;
    quat_traits<A>::template write_element<2>(a) = aa*bc + ac*ba + ad*bb - ab*bd;
    quat_traits<A>::template write_element<3>(a) = aa*bd + ad*ba + ab*bc - ac*bb;
    return a;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value && is_scalar<B>::value,
    A &>::type
operator*=( A & a, B b )
    {
    quat_traits<A>::template write_element<0>(a)*=b;
    quat_traits<A>::template write_element<1>(a)*=b;
    quat_traits<A>::template write_element<2>(a)*=b;
    quat_traits<A>::template write_element<3>(a)*=b;
    return a;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value && is_quat<B>::value,
    deduce_quat2<A,B> >::type
operator*( A const & a, B const & b )
    {
    typedef typename deduce_quat2<A,B>::type R;
    typedef typename quat_traits<A>::scalar_type TA;
    typedef typename quat_traits<B>::scalar_type TB;
    TA const aa=quat_traits<A>::template read_element<0>(a);
    TA const ab=quat_traits<A>::template read_element<1>(a);
    TA const ac=quat_traits<A>::template read_element<2>(a);
    TA const ad=quat_traits<A>::template read_element<3>(a);
    TB const ba=quat_traits<B>::template read_element<0>(b);
    TB const bb=quat_traits<B>::template read_element<1>(b);
    TB const bc=quat_traits<B>::template read_element<2>(b);
    TB const bd=quat_traits<B>::template read_element<3>(b);
    R r;
    quat_traits<R>::template write_element<0>(r) = aa*ba - ab*bb - ac*bc - ad*bd;
    quat_traits<R>::template write_element<1>(r) = aa*bb + ab*ba + ac*bd - ad*bc;
    quat_traits<R>::template write_element<2>(r) = aa*bc + ac*ba + ad*bb - ab*bd;
    quat_traits<R>::template write_element<3>(r) = aa*bd + ad*ba + ab*bc - ac*bb;
    return r;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c2<
    is_quat<A>::value && is_scalar<B>::value,
    deduce_quat2<A,B> >::type
operator*( A const & a, B b )
    {
    typedef typename deduce_quat2<A,B>::type R;
    R r;
    quat_traits<R>::template write_element<0>(r)=quat_traits<A>::template read_element<0>(a)*b;
    quat_traits<R>::template write_element<1>(r)=quat_traits<A>::template read_element<1>(a)*b;
    quat_traits<R>::template write_element<2>(r)=quat_traits<A>::template read_element<2>(a)*b;
    quat_traits<R>::template write_element<3>(r)=quat_traits<A>::template read_element<3>(a)*b;
    return r;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value && is_quat<B>::value,
    bool>::type
operator!=( A const & a, B const & b )
    {
    return
        quat_traits<A>::template read_element<0>(a)!=quat_traits<B>::template read_element<0>(b) ||
        quat_traits<A>::template read_element<1>(a)!=quat_traits<B>::template read_element<1>(b) ||
        quat_traits<A>::template read_element<2>(a)!=quat_traits<B>::template read_element<2>(b) ||
        quat_traits<A>::template read_element<3>(a)!=quat_traits<B>::template read_element<3>(b);
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value,
    deduce_quat<A> >::type
normalized( A const & a )
    {
    typedef typename quat_traits<A>::scalar_type T;
    T const a0=quat_traits<A>::template read_element<0>(a);
    T const a1=quat_traits<A>::template read_element<1>(a);
    T const a2=quat_traits<A>::template read_element<2>(a);
    T const a3=quat_traits<A>::template read_element<3>(a);
    T const m2=a0*a0+a1*a1+a2*a2+a3*a3;
    if( m2==scalar_traits<typename quat_traits<A>::scalar_type>::value(0) )
        BOOST_QVM_THROW_EXCEPTION(zero_magnitude_error());
    T const rm=scalar_traits<T>::value(1)/sqrt<T>(m2);
    typedef typename deduce_quat<A>::type R;
    R r;
    quat_traits<R>::template write_element<0>(r)=a0*rm;
    quat_traits<R>::template write_element<1>(r)=a1*rm;
    quat_traits<R>::template write_element<2>(r)=a2*rm;
    quat_traits<R>::template write_element<3>(r)=a3*rm;
    return r;
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    void>::type
normalize( A & a )
    {
    typedef typename quat_traits<A>::scalar_type T;
    T const a0=quat_traits<A>::template read_element<0>(a);
    T const a1=quat_traits<A>::template read_element<1>(a);
    T const a2=quat_traits<A>::template read_element<2>(a);
    T const a3=quat_traits<A>::template read_element<3>(a);
    T const m2=a0*a0+a1*a1+a2*a2+a3*a3;
    if( m2==scalar_traits<typename quat_traits<A>::scalar_type>::value(0) )
        BOOST_QVM_THROW_EXCEPTION(zero_magnitude_error());
    T const rm=scalar_traits<T>::value(1)/sqrt<T>(m2);
    quat_traits<A>::template write_element<0>(a)*=rm;
    quat_traits<A>::template write_element<1>(a)*=rm;
    quat_traits<A>::template write_element<2>(a)*=rm;
    quat_traits<A>::template write_element<3>(a)*=rm;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if<
    msvc_parse_bug_workaround::quats<A,B>,
    A &>::type
operator+=( A & a, B const & b )
    {
    quat_traits<A>::template write_element<0>(a)+=quat_traits<B>::template read_element<0>(b);
    quat_traits<A>::template write_element<1>(a)+=quat_traits<B>::template read_element<1>(b);
    quat_traits<A>::template write_element<2>(a)+=quat_traits<B>::template read_element<2>(b);
    quat_traits<A>::template write_element<3>(a)+=quat_traits<B>::template read_element<3>(b);
    return a;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value && is_quat<B>::value,
    deduce_quat2<A,B> >::type
operator+( A const & a, B const & b )
    {
    typedef typename deduce_quat2<A,B>::type R;
    R r;
    quat_traits<R>::template write_element<0>(r)=quat_traits<A>::template read_element<0>(a)+quat_traits<B>::template read_element<0>(b);
    quat_traits<R>::template write_element<1>(r)=quat_traits<A>::template read_element<1>(a)+quat_traits<B>::template read_element<1>(b);
    quat_traits<R>::template write_element<2>(r)=quat_traits<A>::template read_element<2>(a)+quat_traits<B>::template read_element<2>(b);
    quat_traits<R>::template write_element<3>(r)=quat_traits<A>::template read_element<3>(a)+quat_traits<B>::template read_element<3>(b);
    return r;
    }

template <class A,class B,class C>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value && is_quat<B>::value && is_scalar<C>::value,
    deduce_quat2<A,B> >::type
slerp( A const & a, B const & b, C t )
    {
    typedef typename deduce_quat2<A,B>::type R;
    typedef typename quat_traits<R>::scalar_type TR;
    TR const one = scalar_traits<TR>::value(1);
    TR dp = dot(a,b);
    TR sc=one;
    if( dp < one )
        {
        TR const theta = acos<TR>(dp);
        TR const invsintheta = one/sin<TR>(theta);
        TR const scale = sin<TR>(theta*(one-t)) * invsintheta;
        TR const invscale = sin<TR>(theta*t) * invsintheta * sc;
        return a*scale + b*invscale;
        }
    else
        return normalized(a+(b-a)*t);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T>
    class
    qref_
        {
        qref_( qref_ const & );
        qref_ & operator=( qref_ const & );
        ~qref_();

        public:

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        qref_ &
        operator=( R const & x )
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
    }

template <class Q>
struct quat_traits;

template <class Q>
struct
quat_traits< qvm_detail::qref_<Q> >
    {
    typedef typename quat_traits<Q>::scalar_type scalar_type;
    typedef qvm_detail::qref_<Q> this_quaternion;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_quaternion const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return quat_traits<Q>::template read_element<I>(reinterpret_cast<Q const &>(x));
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_quaternion & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return quat_traits<Q>::template write_element<I>(reinterpret_cast<Q &>(x));
        }
    };

template <class Q>
struct
deduce_quat< qvm_detail::qref_<Q> >
    {
    typedef quat<typename quat_traits<Q>::scalar_type> type;
    };

template <class Q>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_quat<Q>::value,
    qvm_detail::qref_<Q> const &>::type
qref( Q const & a )
    {
    return reinterpret_cast<qvm_detail::qref_<Q> const &>(a);
    }

template <class Q>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_quat<Q>::value,
    qvm_detail::qref_<Q> &>::type
qref( Q & a )
    {
    return reinterpret_cast<qvm_detail::qref_<Q> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T>
    class
    zero_q_
        {
        zero_q_( zero_q_ const & );
        zero_q_ & operator=( zero_q_ const & );
        ~zero_q_();

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

template <class T>
struct
quat_traits< qvm_detail::zero_q_<T> >
    {
    typedef qvm_detail::zero_q_<T> this_quaternion;
    typedef T scalar_type;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_quaternion const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return scalar_traits<scalar_type>::value(0);
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_quaternion const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<4);
        return scalar_traits<scalar_type>::value(0);
        }
    };

template <class T>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::zero_q_<T> const &
zero_quat()
    {
    return *(qvm_detail::zero_q_<T> const *)qvm_detail::get_valid_ptr_quat_operations();
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    void>::type
set_zero( A & a )
    {
    typedef typename quat_traits<A>::scalar_type T;
    T const zero=scalar_traits<T>::value(0);
    quat_traits<A>::template write_element<0>(a) = zero;
    quat_traits<A>::template write_element<1>(a) = zero;
    quat_traits<A>::template write_element<2>(a) = zero;
    quat_traits<A>::template write_element<3>(a) = zero;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class V>
    struct
    rot_quat_
        {
        typedef typename vec_traits<V>::scalar_type scalar_type;
        scalar_type a[4];

        template <class Angle>
        BOOST_QVM_INLINE
        rot_quat_( V const & axis, Angle angle )
            {
            scalar_type const x=vec_traits<V>::template read_element<0>(axis);
            scalar_type const y=vec_traits<V>::template read_element<1>(axis);
            scalar_type const z=vec_traits<V>::template read_element<2>(axis);
            scalar_type const m2=x*x+y*y+z*z;
            if( m2==scalar_traits<scalar_type>::value(0) )
                BOOST_QVM_THROW_EXCEPTION(zero_magnitude_error());
            scalar_type const rm=scalar_traits<scalar_type>::value(1)/sqrt<scalar_type>(m2);
            angle/=2;
            scalar_type const s=sin<Angle>(angle);
            a[0] = cos<Angle>(angle);
            a[1] = rm*x*s;
            a[2] = rm*y*s;
            a[3] = rm*z*s;
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
    }

template <class V>
struct
quat_traits< qvm_detail::rot_quat_<V> >
    {
    typedef qvm_detail::rot_quat_<V> this_quaternion;
    typedef typename this_quaternion::scalar_type scalar_type;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_quaternion const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return x.a[I];
        }
    };

template <class V>
struct
deduce_quat< qvm_detail::rot_quat_<V> >
    {
    typedef quat<typename vec_traits<V>::scalar_type> type;
    };

template <class A,class Angle>
BOOST_QVM_INLINE
typename enable_if_c<
    is_vec<A>::value && vec_traits<A>::dim==3,
    qvm_detail::rot_quat_<A> >::type
rot_quat( A const & axis, Angle angle )
    {
    return qvm_detail::rot_quat_<A>(axis,angle);
    }

template <class A,class B,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value &&
    is_vec<B>::value && vec_traits<B>::dim==3,
    void>::type
set_rot( A & a, B const & axis, Angle angle )
    {
    assign(a,rot_quat(axis,angle));
    }

template <class A,class B,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value &&
    is_vec<B>::value && vec_traits<B>::dim==3,
    void>::type
rotate( A & a, B const & axis, Angle angle )
    {
    a *= rot_quat(axis,angle);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T>
    struct
    rotx_quat_
        {
        BOOST_QVM_INLINE_TRIVIAL
        rotx_quat_()
            {
            }

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        operator R() const
            {
            R r;
            assign(r,*this);
            return r;
            }

        private:

        rotx_quat_( rotx_quat_ const & );
        rotx_quat_ & operator=( rotx_quat_ const & );
        ~rotx_quat_();
        };

    template <int I>
    struct
    rotx_q_get
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & )
            {
            return scalar_traits<T>::value(0);
            }
        };

    template <>
    struct
    rotx_q_get<1>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return sin<T>(angle/2);
            }
        };

    template <>
    struct
    rotx_q_get<0>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return cos<T>(angle/2);
            }
        };
    }

template <class Angle>
struct
quat_traits< qvm_detail::rotx_quat_<Angle> >
    {
    typedef qvm_detail::rotx_quat_<Angle> this_quaternion;
    typedef Angle scalar_type;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_quaternion const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return qvm_detail::rotx_q_get<I>::get(reinterpret_cast<Angle const &>(x));
        }
    };

template <class Angle>
struct
deduce_quat< qvm_detail::rotx_quat_<Angle> >
    {
    typedef quat<Angle> type;
    };

template <class Angle>
struct
deduce_quat2< qvm_detail::rotx_quat_<Angle>, qvm_detail::rotx_quat_<Angle> >
    {
    typedef quat<Angle> type;
    };

template <class Angle>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::rotx_quat_<Angle> const &
rotx_quat( Angle const & angle )
    {
    return reinterpret_cast<qvm_detail::rotx_quat_<Angle> const &>(angle);
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    void>::type
set_rotx( A & a, Angle angle )
    {
    assign(a,rotx_quat(angle));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    void>::type
rotate_x( A & a, Angle angle )
    {
    a *= rotx_quat(angle);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T>
    struct
    roty_quat_
        {
        BOOST_QVM_INLINE_TRIVIAL
        roty_quat_()
            {
            }

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        operator R() const
            {
            R r;
            assign(r,*this);
            return r;
            }

        private:

        roty_quat_( roty_quat_ const & );
        roty_quat_ & operator=( roty_quat_ const & );
        ~roty_quat_();
        };

    template <int I>
    struct
    roty_q_get
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & )
            {
            return scalar_traits<T>::value(0);
            }
        };

    template <>
    struct
    roty_q_get<2>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return sin<T>(angle/2);
            }
        };

    template <>
    struct
    roty_q_get<0>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return cos<T>(angle/2);
            }
        };
    }

template <class Angle>
struct
quat_traits< qvm_detail::roty_quat_<Angle> >
    {
    typedef qvm_detail::roty_quat_<Angle> this_quaternion;
    typedef Angle scalar_type;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_quaternion const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return qvm_detail::roty_q_get<I>::get(reinterpret_cast<Angle const &>(x));
        }
    };

template <class Angle>
struct
deduce_quat< qvm_detail::roty_quat_<Angle> >
    {
    typedef quat<Angle> type;
    };

template <class Angle>
struct
deduce_quat2< qvm_detail::roty_quat_<Angle>, qvm_detail::roty_quat_<Angle> >
    {
    typedef quat<Angle> type;
    };

template <class Angle>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::roty_quat_<Angle> const &
roty_quat( Angle const & angle )
    {
    return reinterpret_cast<qvm_detail::roty_quat_<Angle> const &>(angle);
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    void>::type
set_roty( A & a, Angle angle )
    {
    assign(a,roty_quat(angle));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    void>::type
rotate_y( A & a, Angle angle )
    {
    a *= roty_quat(angle);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T>
    struct
    rotz_quat_
        {
        BOOST_QVM_INLINE_TRIVIAL
        rotz_quat_()
            {
            }

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        operator R() const
            {
            R r;
            assign(r,*this);
            return r;
            }

        private:

        rotz_quat_( rotz_quat_ const & );
        rotz_quat_ & operator=( rotz_quat_ const & );
        ~rotz_quat_();
        };

    template <int I>
    struct
    rotz_q_get
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & )
            {
            return scalar_traits<T>::value(0);
            }
        };

    template <>
    struct
    rotz_q_get<3>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return sin<T>(angle/2);
            }
        };

    template <>
    struct
    rotz_q_get<0>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return cos<T>(angle/2);
            }
        };
    }

template <class Angle>
struct
quat_traits< qvm_detail::rotz_quat_<Angle> >
    {
    typedef qvm_detail::rotz_quat_<Angle> this_quaternion;
    typedef Angle scalar_type;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_quaternion const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return qvm_detail::rotz_q_get<I>::get(reinterpret_cast<Angle const &>(x));
        }
    };

template <class Angle>
struct
deduce_quat< qvm_detail::rotz_quat_<Angle> >
    {
    typedef quat<Angle> type;
    };

template <class Angle>
struct
deduce_quat2< qvm_detail::rotz_quat_<Angle>, qvm_detail::rotz_quat_<Angle> >
    {
    typedef quat<Angle> type;
    };

template <class Angle>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::rotz_quat_<Angle> const &
rotz_quat( Angle const & angle )
    {
    return reinterpret_cast<qvm_detail::rotz_quat_<Angle> const &>(angle);
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    void>::type
set_rotz( A & a, Angle angle )
    {
    assign(a,rotz_quat(angle));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value,
    void>::type
rotate_z( A & a, Angle angle )
    {
    a *= rotz_quat(angle);
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value && is_vec<B>::value && vec_traits<B>::dim==3,
    typename quat_traits<A>::scalar_type>::type
axis_angle( A const & a, B & b )
    {
    typedef typename quat_traits<A>::scalar_type T;
    T a0=quat_traits<A>::template read_element<0>(a);
    T a1=quat_traits<A>::template read_element<1>(a);
    T a2=quat_traits<A>::template read_element<2>(a);
    T a3=quat_traits<A>::template read_element<3>(a);
    if( a0>1 )
        {
        T const m2=a0*a0+a1*a1+a2*a2+a3*a3;
        if( m2==scalar_traits<T>::value(0) )
            BOOST_QVM_THROW_EXCEPTION(zero_magnitude_error());
        T const s=sqrt<T>(m2);
        a0/=s;
        a1/=s;
        a2/=s;
        a3/=s;
        }
    if( T s=sqrt<T>(1-a0*a0) )
        {
        vec_traits<B>::template write_element<0>(b) = a1/s;
        vec_traits<B>::template write_element<1>(b) = a2/s;
        vec_traits<B>::template write_element<2>(b) = a3/s;
        }
    else
        {
        typedef typename vec_traits<B>::scalar_type U;
        vec_traits<B>::template write_element<0>(b) = scalar_traits<U>::value(1);
        vec_traits<B>::template write_element<1>(b) = vec_traits<B>::template write_element<2>(b) = scalar_traits<U>::value(0);
        }
    return scalar_traits<T>::value(2) * qvm::acos(a0);
    }

////////////////////////////////////////////////

namespace
sfinae
    {
    using ::boost::qvm::assign;
    using ::boost::qvm::cmp;
    using ::boost::qvm::convert_to;
    using ::boost::qvm::conjugate;
    using ::boost::qvm::set_identity;
    using ::boost::qvm::set_zero;
    using ::boost::qvm::scalar_cast;
    using ::boost::qvm::operator/=;
    using ::boost::qvm::operator/;
    using ::boost::qvm::dot;
    using ::boost::qvm::operator==;
    using ::boost::qvm::inverse;
    using ::boost::qvm::mag_sqr;
    using ::boost::qvm::mag;
    using ::boost::qvm::slerp;
    using ::boost::qvm::operator-=;
    using ::boost::qvm::operator-;
    using ::boost::qvm::operator*=;
    using ::boost::qvm::operator*;
    using ::boost::qvm::operator!=;
    using ::boost::qvm::normalized;
    using ::boost::qvm::normalize;
    using ::boost::qvm::operator+=;
    using ::boost::qvm::operator+;
    using ::boost::qvm::qref;
    using ::boost::qvm::rot_quat;
    using ::boost::qvm::set_rot;
    using ::boost::qvm::rotate;
    using ::boost::qvm::rotx_quat;
    using ::boost::qvm::set_rotx;
    using ::boost::qvm::rotate_x;
    using ::boost::qvm::roty_quat;
    using ::boost::qvm::set_roty;
    using ::boost::qvm::rotate_y;
    using ::boost::qvm::rotz_quat;
    using ::boost::qvm::set_rotz;
    using ::boost::qvm::rotate_z;
    }

} }

#endif
