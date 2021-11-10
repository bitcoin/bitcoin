#ifndef BOOST_QVM_VEC_OPERATIONS_HPP_INCLUDED
#define BOOST_QVM_VEC_OPERATIONS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.
/// Copyright (c) 2019 agate-pris

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/detail/vec_assign.hpp>
#include <boost/qvm/vec_operations2.hpp>
#include <boost/qvm/vec_operations3.hpp>
#include <boost/qvm/vec_operations4.hpp>
#include <boost/qvm/assert.hpp>
#include <boost/qvm/scalar_traits.hpp>
#include <string>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    BOOST_QVM_INLINE_CRITICAL
    void const *
    get_valid_ptr_vec_operations()
        {
        static int const obj=0;
        return &obj;
        }
    }

////////////////////////////////////////////////

namespace
qvm_to_string_detail
    {
    template <class T>
    std::string to_string( T const & x );
    }

namespace
qvm_detail
    {
    template <int D>
    struct
    to_string_v_defined
        {
        static bool const value=false;
        };

    template <int I,int DimMinusOne>
    struct
    to_string_vector_elements
        {
        template <class A>
        static
        std::string
        f( A const & a )
            {
            using namespace qvm_to_string_detail;
            return to_string(vec_traits<A>::template read_element<I>(a))+','+to_string_vector_elements<I+1,DimMinusOne>::f(a);
            }
        };

    template <int DimMinusOne>
    struct
    to_string_vector_elements<DimMinusOne,DimMinusOne>
        {
        template <class A>
        static
        std::string
        f( A const & a )
            {
            using namespace qvm_to_string_detail;
            return to_string(vec_traits<A>::template read_element<DimMinusOne>(a));
            }
        };
    }

template <class A>
inline
typename enable_if_c<
    is_vec<A>::value  &&
    !qvm_detail::to_string_v_defined<vec_traits<A>::dim>::value,
    std::string>::type
to_string( A const & a )
    {
    return '('+qvm_detail::to_string_vector_elements<0,vec_traits<A>::dim-1>::f(a)+')';
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    convert_to_v_defined
        {
        static bool const value=false;
        };
    }

template <class R,class A>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_vec<R>::value && is_vec<A>::value &&
    vec_traits<R>::dim==vec_traits<A>::dim &&
    !qvm_detail::convert_to_v_defined<vec_traits<R>::dim>::value,
    R>::type
convert_to( A const & a )
    {
    R r; assign(r,a);
    return r;
    }

////////////////////////////////////////////////

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==3 && vec_traits<B>::dim==3,
    deduce_vec2<A,B,3> >::type
cross( A const & a, B const & b )
    {
    typedef typename deduce_vec2<A,B,3>::type R;
    R r;
    vec_traits<R>::template write_element<0>(r)=
        vec_traits<A>::template read_element<1>(a)*vec_traits<B>::template read_element<2>(b)-
        vec_traits<A>::template read_element<2>(a)*vec_traits<B>::template read_element<1>(b);
    vec_traits<R>::template write_element<1>(r)=
        vec_traits<A>::template read_element<2>(a)*vec_traits<B>::template read_element<0>(b)-
        vec_traits<A>::template read_element<0>(a)*vec_traits<B>::template read_element<2>(b);
    vec_traits<R>::template write_element<2>(r)=
        vec_traits<A>::template read_element<0>(a)*vec_traits<B>::template read_element<1>(b)-
        vec_traits<A>::template read_element<1>(a)*vec_traits<B>::template read_element<0>(b);
    return r;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==2 && vec_traits<B>::dim==2,
    deduce_scalar<typename vec_traits<A>::scalar_type,typename vec_traits<B>::scalar_type> >::type
cross( A const & a, B const & b )
    {
    typedef typename deduce_scalar<typename vec_traits<A>::scalar_type,typename vec_traits<B>::scalar_type>::type R;
    R const r =
        vec_traits<A>::template read_element<0>(a)*vec_traits<B>::template read_element<1>(b)-
        vec_traits<A>::template read_element<1>(a)*vec_traits<B>::template read_element<0>(b);
    return r;
    }

////////////////////////////////////////////////

template <class A,class B,class Cmp>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==vec_traits<B>::dim,
    bool>::type
cmp( A const & a, B const & b, Cmp f )
    {
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        if( !f(
            vec_traits<A>::read_element_idx(i,a),
            vec_traits<B>::read_element_idx(i,b)) )
            return false;
    return true;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T,int Dim>
    class
    zero_vec_
        {
        zero_vec_( zero_vec_ const & );
        zero_vec_ & operator=( zero_vec_ const & );
        ~zero_vec_();

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

template <class V>
struct vec_traits;

template <class T,int Dim>
struct
vec_traits< qvm_detail::zero_vec_<T,Dim> >
    {
    typedef qvm_detail::zero_vec_<T,Dim> this_vector;
    typedef T scalar_type;
    static int const dim=Dim;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<Dim);
        return scalar_traits<scalar_type>::value(0);
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_vector const & )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<Dim);
        return scalar_traits<scalar_type>::value(0);
        }
    };

template <class T,int Dim,int D>
struct
deduce_vec<qvm_detail::zero_vec_<T,Dim>,D>
    {
    typedef vec<T,D> type;
    };

template <class T,int Dim>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::zero_vec_<T,Dim> const &
zero_vec()
    {
    return *(qvm_detail::zero_vec_<T,Dim> const *)qvm_detail::get_valid_ptr_vec_operations();
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value,
    void>::type
set_zero( A & a )
    {
    assign(a,zero_vec<typename vec_traits<A>::scalar_type,vec_traits<A>::dim>());
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class OriginalType,class Scalar>
    class
    vector_scalar_cast_
        {
        vector_scalar_cast_( vector_scalar_cast_ const & );
        vector_scalar_cast_ & operator=( vector_scalar_cast_ const & );
        ~vector_scalar_cast_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        vector_scalar_cast_ &
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

    template <bool> struct scalar_cast_vector_filter { };
    template <> struct scalar_cast_vector_filter<true> { typedef int type; };
    }

template <class OriginalType,class Scalar>
struct
vec_traits< qvm_detail::vector_scalar_cast_<OriginalType,Scalar> >
    {
    typedef Scalar scalar_type;
    typedef qvm_detail::vector_scalar_cast_<OriginalType,Scalar> this_vector;
    static int const dim=vec_traits<OriginalType>::dim;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return scalar_type(vec_traits<OriginalType>::template read_element<I>(reinterpret_cast<OriginalType const &>(x)));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_vector const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return scalar_type(vec_traits<OriginalType>::read_element_idx(i,reinterpret_cast<OriginalType const &>(x)));
        }
    };

template <class OriginalType,class Scalar,int D>
struct
deduce_vec<qvm_detail::vector_scalar_cast_<OriginalType,Scalar>,D>
    {
    typedef vec<Scalar,D> type;
    };

template <class Scalar,class T>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::vector_scalar_cast_<T,Scalar> const &
scalar_cast( T const & x, typename qvm_detail::scalar_cast_vector_filter<is_vec<T>::value>::type=0 )
    {
    return reinterpret_cast<qvm_detail::vector_scalar_cast_<T,Scalar> const &>(x);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    div_eq_vs_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value && is_scalar<B>::value &&
    !qvm_detail::div_eq_vs_defined<vec_traits<A>::dim>::value,
    A &>::type
operator/=( A & a, B b )
    {
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<A>::write_element_idx(i,a)/=b;
    return a;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    div_vs_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value && is_scalar<B>::value &&
    !qvm_detail::div_vs_defined<vec_traits<A>::dim>::value,
    deduce_vec2<A,B,vec_traits<A>::dim> >::type
operator/( A const & a, B b )
    {
    typedef typename deduce_vec2<A,B,vec_traits<A>::dim>::type R;
    R r;
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<R>::write_element_idx(i,r)=vec_traits<A>::read_element_idx(i,a)/b;
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    dot_vv_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==vec_traits<B>::dim &&
    !qvm_detail::dot_vv_defined<vec_traits<A>::dim>::value,
    deduce_scalar<typename vec_traits<A>::scalar_type,typename vec_traits<B>::scalar_type> >::type
dot( A const & a, B const & b )
    {
    typedef typename deduce_scalar<typename vec_traits<A>::scalar_type,typename vec_traits<B>::scalar_type>::type T;
    T m(scalar_traits<T>::value(0));
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        m+=vec_traits<A>::read_element_idx(i,a)*vec_traits<B>::read_element_idx(i,b);
    return m;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    eq_vv_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==vec_traits<B>::dim &&
    !qvm_detail::eq_vv_defined<vec_traits<A>::dim>::value,
    bool>::type
operator==( A const & a, B const & b )
    {
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        if( vec_traits<A>::read_element_idx(i,a)!=vec_traits<B>::read_element_idx(i,b) )
            return false;
    return true;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    mag_sqr_v_defined
        {
        static bool const value=false;
        };
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value &&
    !qvm_detail::mag_sqr_v_defined<vec_traits<A>::dim>::value,
    typename vec_traits<A>::scalar_type>::type
mag_sqr( A const & a )
    {
    typedef typename vec_traits<A>::scalar_type T;
    T m(scalar_traits<T>::value(0));
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        {
        T x=vec_traits<A>::read_element_idx(i,a);
        m+=x*x;
        }
    return m;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    mag_v_defined
        {
        static bool const value=false;
        };
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value &&
    !qvm_detail::mag_v_defined<vec_traits<A>::dim>::value,
    typename vec_traits<A>::scalar_type>::type
mag( A const & a )
    {
    typedef typename vec_traits<A>::scalar_type T;
    T m(scalar_traits<T>::value(0));
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        {
        T x=vec_traits<A>::read_element_idx(i,a);
        m+=x*x;
        }
    return sqrt<T>(m);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    minus_eq_vv_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==vec_traits<B>::dim &&
    !qvm_detail::minus_eq_vv_defined<vec_traits<A>::dim>::value,
    A &>::type
operator-=( A & a, B const & b )
    {
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<A>::write_element_idx(i,a)-=vec_traits<B>::read_element_idx(i,b);
    return a;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    minus_v_defined
        {
        static bool const value=false;
        };
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value &&
    !qvm_detail::minus_v_defined<vec_traits<A>::dim>::value,
    deduce_vec<A> >::type
operator-( A const & a )
    {
    typedef typename deduce_vec<A>::type R;
    R r;
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<R>::write_element_idx(i,r)=-vec_traits<A>::read_element_idx(i,a);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    minus_vv_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==vec_traits<B>::dim &&
    !qvm_detail::minus_vv_defined<vec_traits<A>::dim>::value,
    deduce_vec2<A,B,vec_traits<A>::dim> >::type
operator-( A const & a, B const & b )
    {
    typedef typename deduce_vec2<A,B,vec_traits<A>::dim>::type R;
    R r;
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<R>::write_element_idx(i,r)=vec_traits<A>::read_element_idx(i,a)-vec_traits<B>::read_element_idx(i,b);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    mul_eq_vs_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value && is_scalar<B>::value &&
    !qvm_detail::mul_eq_vs_defined<vec_traits<A>::dim>::value,
    A &>::type
operator*=( A & a, B b )
    {
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<A>::write_element_idx(i,a)*=b;
    return a;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    mul_vs_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value && is_scalar<B>::value &&
    !qvm_detail::mul_vs_defined<vec_traits<A>::dim>::value,
    deduce_vec2<A,B,vec_traits<A>::dim> >::type
operator*( A const & a, B b )
    {
    typedef typename deduce_vec2<A,B,vec_traits<A>::dim>::type R;
    R r;
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<R>::write_element_idx(i,r)=vec_traits<A>::read_element_idx(i,a)*b;
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    mul_sv_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_scalar<A>::value && is_vec<B>::value &&
    !qvm_detail::mul_sv_defined<vec_traits<B>::dim>::value,
    deduce_vec2<A,B,vec_traits<B>::dim> >::type
operator*( A a, B const & b )
    {
    typedef typename deduce_vec2<A,B,vec_traits<B>::dim>::type R;
    R r;
    for( int i=0; i!=vec_traits<B>::dim; ++i )
        vec_traits<R>::write_element_idx(i,r)=a*vec_traits<B>::read_element_idx(i,b);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    neq_vv_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==vec_traits<B>::dim &&
    !qvm_detail::neq_vv_defined<vec_traits<A>::dim>::value,
    bool>::type
operator!=( A const & a, B const & b )
    {
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        if( vec_traits<A>::read_element_idx(i,a)!=vec_traits<B>::read_element_idx(i,b) )
            return true;
    return false;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    normalize_v_defined
        {
        static bool const value=false;
        };
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value &&
    !qvm_detail::normalize_v_defined<vec_traits<A>::dim>::value,
    deduce_vec<A> >::type
normalized( A const & a )
    {
    typedef typename vec_traits<A>::scalar_type T;
    T m(scalar_traits<T>::value(0));
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        {
        T x=vec_traits<A>::read_element_idx(i,a);
        m+=x*x;
        }
    if( m==scalar_traits<T>::value(0) )
        BOOST_QVM_THROW_EXCEPTION(zero_magnitude_error());
    T rm=scalar_traits<T>::value(1)/sqrt<T>(m);
    typedef typename deduce_vec<A>::type R;
    R r;
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<R>::write_element_idx(i,r)=vec_traits<A>::read_element_idx(i,a)*rm;
    return r;
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value &&
    !qvm_detail::normalize_v_defined<vec_traits<A>::dim>::value,
    void>::type
normalize( A & a )
    {
    typedef typename vec_traits<A>::scalar_type T;
    T m(scalar_traits<T>::value(0));
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        {
        T x=vec_traits<A>::read_element_idx(i,a);
        m+=x*x;
        }
    if( m==scalar_traits<T>::value(0) )
        BOOST_QVM_THROW_EXCEPTION(zero_magnitude_error());
    T rm=scalar_traits<T>::value(1)/sqrt<T>(m);
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<A>::write_element_idx(i,a)*=rm;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    plus_eq_vv_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==vec_traits<B>::dim &&
    !qvm_detail::plus_eq_vv_defined<vec_traits<A>::dim>::value,
    A &>::type
operator+=( A & a, B const & b )
    {
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<A>::write_element_idx(i,a)+=vec_traits<B>::read_element_idx(i,b);
    return a;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    plus_vv_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==vec_traits<B>::dim &&
    !qvm_detail::plus_vv_defined<vec_traits<A>::dim>::value,
    deduce_vec2<A,B,vec_traits<A>::dim> >::type
operator+( A const & a, B const & b )
    {
    typedef typename deduce_vec2<A,B,vec_traits<A>::dim>::type R;
    R r;
    for( int i=0; i!=vec_traits<A>::dim; ++i )
        vec_traits<R>::write_element_idx(i,r)=vec_traits<A>::read_element_idx(i,a)+vec_traits<B>::read_element_idx(i,b);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T>
    class
    vref_
        {
        vref_( vref_ const & );
        vref_ & operator=( vref_ const & );
        ~vref_();

        public:

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        vref_ &
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

template <class V>
struct
vec_traits< qvm_detail::vref_<V> >
    {
    typedef typename vec_traits<V>::scalar_type scalar_type;
    typedef qvm_detail::vref_<V> this_vector;
    static int const dim=vec_traits<V>::dim;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return vec_traits<V>::template read_element<I>(reinterpret_cast<V const &>(x));
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_vector & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return vec_traits<V>::template write_element<I>(reinterpret_cast<V &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_vector const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return vec_traits<V>::read_element_idx(i,reinterpret_cast<V const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int i, this_vector & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return vec_traits<V>::write_element_idx(i,reinterpret_cast<V &>(x));
        }
    };

template <class V,int D>
struct
deduce_vec<qvm_detail::vref_<V>,D>
    {
    typedef vec<typename vec_traits<V>::scalar_type,D> type;
    };

template <class V>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_vec<V>::value,
    qvm_detail::vref_<V> const &>::type
vref( V const & a )
    {
    return reinterpret_cast<qvm_detail::vref_<V> const &>(a);
    }

template <class V>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_vec<V>::value,
    qvm_detail::vref_<V> &>::type
vref( V & a )
    {
    return reinterpret_cast<qvm_detail::vref_<V> &>(a);
    }

////////////////////////////////////////////////

namespace
sfinae
    {
    using ::boost::qvm::to_string;
    using ::boost::qvm::assign;
    using ::boost::qvm::convert_to;
    using ::boost::qvm::cross;
    using ::boost::qvm::cmp;
    using ::boost::qvm::set_zero;
    using ::boost::qvm::scalar_cast;
    using ::boost::qvm::operator/=;
    using ::boost::qvm::operator/;
    using ::boost::qvm::dot;
    using ::boost::qvm::operator==;
    using ::boost::qvm::mag_sqr;
    using ::boost::qvm::mag;
    using ::boost::qvm::operator-=;
    using ::boost::qvm::operator-;
    using ::boost::qvm::operator*=;
    using ::boost::qvm::operator*;
    using ::boost::qvm::operator!=;
    using ::boost::qvm::normalized;
    using ::boost::qvm::normalize;
    using ::boost::qvm::operator+=;
    using ::boost::qvm::operator+;
    using ::boost::qvm::vref;
    }

} }

#endif
