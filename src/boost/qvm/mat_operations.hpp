#ifndef BOOST_QVM_MAT_OPERATIONS_HPP_INCLUDED
#define BOOST_QVM_MAT_OPERATIONS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.
/// Copyright (c) 2019 agate-pris

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/detail/mat_assign.hpp>
#include <boost/qvm/mat_operations2.hpp>
#include <boost/qvm/mat_operations3.hpp>
#include <boost/qvm/mat_operations4.hpp>
#include <boost/qvm/math.hpp>
#include <boost/qvm/detail/determinant_impl.hpp>
#include <boost/qvm/detail/cofactor_impl.hpp>
#include <boost/qvm/detail/transp_impl.hpp>
#include <boost/qvm/scalar_traits.hpp>
#include <string>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    BOOST_QVM_INLINE_CRITICAL
    void const *
    get_valid_ptr_mat_operations()
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
    template <int R,int C>
    struct
    to_string_m_defined
        {
        static bool const value=false;
        };

    template <int I,int SizeMinusOne>
    struct
    to_string_matrix_elements
        {
        template <class A>
        static
        std::string
        f( A const & a )
            {
            using namespace qvm_to_string_detail;
            return
                ( (I%mat_traits<A>::cols)==0 ? '(' : ',' ) +
                to_string(mat_traits<A>::template read_element<I/mat_traits<A>::cols,I%mat_traits<A>::cols>(a)) +
                ( (I%mat_traits<A>::cols)==mat_traits<A>::cols-1 ? ")" : "" ) +
                to_string_matrix_elements<I+1,SizeMinusOne>::f(a);
            }
        };

    template <int SizeMinusOne>
    struct
    to_string_matrix_elements<SizeMinusOne,SizeMinusOne>
        {
        template <class A>
        static
        std::string
        f( A const & a )
            {
            using namespace qvm_to_string_detail;
            return
                ( (SizeMinusOne%mat_traits<A>::cols)==0 ? '(' : ',' ) +
                to_string(mat_traits<A>::template read_element<SizeMinusOne/mat_traits<A>::cols,SizeMinusOne%mat_traits<A>::cols>(a)) +
                ')';
            }
        };
    }

template <class A>
inline
typename enable_if_c<
    is_mat<A>::value  &&
    !qvm_detail::to_string_m_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    std::string>::type
to_string( A const & a )
    {
    return "("+qvm_detail::to_string_matrix_elements<0,mat_traits<A>::rows*mat_traits<A>::cols-1>::f(a)+')';
    }

////////////////////////////////////////////////

template <class A,class B,class Cmp>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value && is_mat<B>::value &&
    mat_traits<A>::rows==mat_traits<B>::rows &&
    mat_traits<A>::cols==mat_traits<B>::cols,
    bool>::type
cmp( A const & a, B const & b, Cmp f )
    {
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            if( !f(
                mat_traits<A>::read_element_idx(i, j, a),
                mat_traits<B>::read_element_idx(i, j, b)) )
                return false;
    return true;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    convert_to_m_defined
        {
        static bool const value=false;
        };
    }

template <class R,class A>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_mat<R>::value && is_mat<A>::value &&
    mat_traits<R>::rows==mat_traits<A>::rows &&
    mat_traits<R>::cols==mat_traits<A>::cols &&
    !qvm_detail::convert_to_m_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    R>::type
convert_to( A const & a )
    {
    R r; assign(r,a);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    determinant_defined
        {
        static bool const value=false;
        };
    }

template <class A>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    !qvm_detail::determinant_defined<mat_traits<A>::rows>::value,
    typename mat_traits<A>::scalar_type>::type
determinant( A const & a )
    {
    return qvm_detail::determinant_impl(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T,int Dim>
    class
    identity_mat_
        {
        identity_mat_( identity_mat_ const & );
        identity_mat_ & operator=( identity_mat_ const & );
        ~identity_mat_();

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

template <class T,int Dim>
struct
mat_traits< qvm_detail::identity_mat_<T,Dim> >
    {
    typedef qvm_detail::identity_mat_<T,Dim> this_matrix;
    typedef T scalar_type;
    static int const rows=Dim;
    static int const cols=Dim;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & /*x*/ )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<Dim);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<Dim);
        return scalar_traits<scalar_type>::value(Row==Col);
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & /*x*/ )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<Dim);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<Dim);
        return scalar_traits<scalar_type>::value(row==col);
        }
    };

template <class T,int Dim>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::identity_mat_<T,Dim> const &
identity_mat()
    {
    return *(qvm_detail::identity_mat_<T,Dim> const *)qvm_detail::get_valid_ptr_mat_operations();
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols,
    void>::type
set_identity( A & a )
    {
    assign(a,identity_mat<typename mat_traits<A>::scalar_type,mat_traits<A>::rows>());
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T>
    struct
    projection_
        {
        T const _00;
        T const _11;
        T const _22;
        T const _23;
        T const _32;

        BOOST_QVM_INLINE_TRIVIAL
        projection_( T _00, T _11, T _22, T _23, T _32 ):
            _00(_00),
            _11(_11),
            _22(_22),
            _23(_23),
            _32(_32)
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
        };

    template <int Row,int Col>
    struct
    projection_get
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( projection_<T> const & )
            {
            return scalar_traits<T>::value(0);
            }
        };

    template <> struct projection_get<0,0> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( projection_<T> const & m ) { return m._00; } };
    template <> struct projection_get<1,1> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( projection_<T> const & m ) { return m._11; } };
    template <> struct projection_get<2,2> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( projection_<T> const & m ) { return m._22; } };
    template <> struct projection_get<2,3> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( projection_<T> const & m ) { return m._23; } };
    template <> struct projection_get<3,2> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( projection_<T> const & m ) { return m._32; } };
    }

template <class T>
struct
mat_traits< qvm_detail::projection_<T> >
    {
    typedef qvm_detail::projection_<T> this_matrix;
    typedef T scalar_type;
    static int const rows=4;
    static int const cols=4;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<cols);
        return qvm_detail::projection_get<Row,Col>::get(x);
        }
    };

template <class T>
qvm_detail::projection_<T>
BOOST_QVM_INLINE_OPERATIONS
perspective_lh( T fov_y, T aspect_ratio, T z_near, T z_far )
    {
    T const one = scalar_traits<T>::value(1);
    T const ys = one/tan<T>(fov_y/scalar_traits<T>::value(2));
    T const xs = ys/aspect_ratio;
    T const zd = z_far-z_near;
    T const z1 = z_far/zd;
    T const z2 = -z_near*z1;
    return qvm_detail::projection_<T>(xs,ys,z1,z2,one);
    }

template <class T>
qvm_detail::projection_<T>
BOOST_QVM_INLINE_OPERATIONS
perspective_rh( T fov_y, T aspect_ratio, T z_near, T z_far )
    {
    T const one = scalar_traits<T>::value(1);
    T const ys = one/tan<T>(fov_y/scalar_traits<T>::value(2));
    T const xs = ys/aspect_ratio;
    T const zd = z_near-z_far;
    T const z1 = z_far/zd;
    T const z2 = z_near*z1;
    return qvm_detail::projection_<T>(xs,ys,z1,z2,-one);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class OriginalType,class Scalar>
    class
    matrix_scalar_cast_
        {
        matrix_scalar_cast_( matrix_scalar_cast_ const & );
        matrix_scalar_cast_ & operator=( matrix_scalar_cast_ const & );
        ~matrix_scalar_cast_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        matrix_scalar_cast_ &
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

    template <bool> struct scalar_cast_matrix_filter { };
    template <> struct scalar_cast_matrix_filter<true> { typedef int type; };
    }

template <class OriginalType,class Scalar>
struct
mat_traits< qvm_detail::matrix_scalar_cast_<OriginalType,Scalar> >
    {
    typedef Scalar scalar_type;
    typedef qvm_detail::matrix_scalar_cast_<OriginalType,Scalar> this_matrix;
    static int const rows=mat_traits<OriginalType>::rows;
    static int const cols=mat_traits<OriginalType>::cols;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<cols);
        return scalar_type(mat_traits<OriginalType>::template read_element<Row,Col>(reinterpret_cast<OriginalType const &>(x)));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<cols);
        return scalar_type(mat_traits<OriginalType>::read_element_idx(col,row,reinterpret_cast<OriginalType const &>(x)));
        }
    };

template <class OriginalType,class Scalar,int R,int C>
struct
deduce_mat<qvm_detail::matrix_scalar_cast_<OriginalType,Scalar>,R,C>
    {
    typedef mat<Scalar,R,C> type;
    };

template <class Scalar,class T>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::matrix_scalar_cast_<T,Scalar> const &
scalar_cast( T const & x, typename qvm_detail::scalar_cast_matrix_filter<is_mat<T>::value>::type=0 )
    {
    return reinterpret_cast<qvm_detail::matrix_scalar_cast_<T,Scalar> const &>(x);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    div_eq_ms_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value && is_scalar<B>::value &&
    !qvm_detail::div_eq_ms_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    A &>::type
operator/=( A & a, B b )
    {
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            mat_traits<A>::write_element_idx(i,j,a)/=b;
    return a;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    div_ms_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_mat<A>::value && is_scalar<B>::value &&
    !qvm_detail::div_ms_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols> >::type
operator/( A const & a, B b )
    {
    typedef typename deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols>::type R;
    R r;
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            mat_traits<R>::write_element_idx(i,j,r)=mat_traits<A>::read_element_idx(i,j,a)/b;
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    eq_mm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value && is_mat<B>::value &&
    mat_traits<A>::rows==mat_traits<B>::rows &&
    mat_traits<A>::cols==mat_traits<B>::cols &&
    !qvm_detail::eq_mm_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    bool>::type
operator==( A const & a, B const & b )
    {
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            if( mat_traits<A>::read_element_idx(i,j,a)!=mat_traits<B>::read_element_idx(i,j,b) )
                return false;
    return true;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    minus_eq_mm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value && is_mat<B>::value &&
    mat_traits<A>::rows==mat_traits<B>::rows &&
    mat_traits<A>::cols==mat_traits<B>::cols &&
    !qvm_detail::minus_eq_mm_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    A &>::type
operator-=( A & a, B const & b )
    {
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            mat_traits<A>::write_element_idx(i,j,a)-=mat_traits<B>::read_element_idx(i,j,b);
    return a;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    minus_m_defined
        {
        static bool const value=false;
        };
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_mat<A>::value &&
    !qvm_detail::minus_m_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    deduce_mat<A> >::type
operator-( A const & a )
    {
    typedef typename deduce_mat<A>::type R;
    R r;
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            mat_traits<R>::write_element_idx(i,j,r)=-mat_traits<A>::read_element_idx(i,j,a);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    minus_mm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_mat<A>::value && is_mat<B>::value &&
    mat_traits<A>::rows==mat_traits<B>::rows &&
    mat_traits<A>::cols==mat_traits<B>::cols &&
    !qvm_detail::minus_mm_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols> >::type
operator-( A const & a, B const & b )
    {
    typedef typename deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols>::type R;
    R r;
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            mat_traits<R>::write_element_idx(i,j,r)=mat_traits<A>::read_element_idx(i,j,a)-mat_traits<B>::read_element_idx(i,j,b);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    mul_eq_mm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    is_mat<B>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows==mat_traits<B>::rows &&
    mat_traits<A>::cols==mat_traits<B>::cols &&
    !qvm_detail::mul_eq_mm_defined<mat_traits<A>::rows>::value,
    A &>::type
operator*=( A & r, B const & b )
    {
    typedef typename mat_traits<A>::scalar_type Ta;
    Ta a[mat_traits<A>::rows][mat_traits<A>::cols];
    for( int i=0; i<mat_traits<A>::rows; ++i )
        for( int j=0; j<mat_traits<B>::cols; ++j )
            a[i][j]=mat_traits<A>::read_element_idx(i,j,r);
    for( int i=0; i<mat_traits<A>::rows; ++i )
        for( int j=0; j<mat_traits<B>::cols; ++j )
            {
            Ta x(scalar_traits<Ta>::value(0));
            for( int k=0; k<mat_traits<A>::cols; ++k )
                x += a[i][k]*mat_traits<B>::read_element_idx(k,j,b);
            mat_traits<A>::write_element_idx(i,j,r) = x;
            }
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    mul_eq_ms_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value && is_scalar<B>::value &&
    !qvm_detail::mul_eq_ms_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    A &>::type
operator*=( A & a, B b )
    {
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            mat_traits<A>::write_element_idx(i,j,a)*=b;
    return a;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int R,int /*CR*/,int C>
    struct
    mul_mm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_mat<A>::value && is_mat<B>::value &&
    mat_traits<A>::cols==mat_traits<B>::rows &&
    !qvm_detail::mul_mm_defined<mat_traits<A>::rows,mat_traits<A>::cols,mat_traits<B>::cols>::value,
    deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<B>::cols> >::type
operator*( A const & a, B const & b )
    {
    typedef typename deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<B>::cols>::type R;
    R r;
    for( int i=0; i<mat_traits<A>::rows; ++i )
        for( int j=0; j<mat_traits<B>::cols; ++j )
            {
            typedef typename mat_traits<A>::scalar_type Ta;
            Ta x(scalar_traits<Ta>::value(0));
            for( int k=0; k<mat_traits<A>::cols; ++k )
                x += mat_traits<A>::read_element_idx(i,k,a)*mat_traits<B>::read_element_idx(k,j,b);
            mat_traits<R>::write_element_idx(i,j,r) = x;
            }
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    mul_ms_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_mat<A>::value && is_scalar<B>::value &&
    !qvm_detail::mul_ms_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols> >::type
operator*( A const & a, B b )
    {
    typedef typename deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols>::type R;
    R r;
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            mat_traits<R>::write_element_idx(i,j,r)=mat_traits<A>::read_element_idx(i,j,a)*b;
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    mul_sm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_scalar<A>::value && is_mat<B>::value &&
    !qvm_detail::mul_sm_defined<mat_traits<B>::rows,mat_traits<B>::cols>::value,
    deduce_mat2<A,B,mat_traits<B>::rows,mat_traits<B>::cols> >::type
operator*( A a, B const & b )
    {
    typedef typename deduce_mat2<A,B,mat_traits<B>::rows,mat_traits<B>::cols>::type R;
    R r;
    for( int i=0; i!=mat_traits<B>::rows; ++i )
        for( int j=0; j!=mat_traits<B>::cols; ++j )
            mat_traits<R>::write_element_idx(i,j,r)=a*mat_traits<B>::read_element_idx(i,j,b);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    neq_mm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value && is_mat<B>::value &&
    mat_traits<A>::rows==mat_traits<B>::rows &&
    mat_traits<A>::cols==mat_traits<B>::cols &&
    !qvm_detail::neq_mm_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    bool>::type
operator!=( A const & a, B const & b )
    {
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            if( mat_traits<A>::read_element_idx(i,j,a)!=mat_traits<B>::read_element_idx(i,j,b) )
                return true;
    return false;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    plus_eq_mm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value && is_mat<B>::value &&
    mat_traits<A>::rows==mat_traits<B>::rows &&
    mat_traits<A>::cols==mat_traits<B>::cols &&
    !qvm_detail::plus_eq_mm_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    A &>::type
operator+=( A & a, B const & b )
    {
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            mat_traits<A>::write_element_idx(i,j,a)+=mat_traits<B>::read_element_idx(i,j,b);
    return a;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    plus_mm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_mat<A>::value && is_mat<B>::value &&
    mat_traits<A>::rows==mat_traits<B>::rows &&
    mat_traits<A>::cols==mat_traits<B>::cols &&
    !qvm_detail::plus_mm_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols> >::type
operator+( A const & a, B const & b )
    {
    typedef typename deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols>::type R;
    R r;
    for( int i=0; i!=mat_traits<A>::rows; ++i )
        for( int j=0; j!=mat_traits<A>::cols; ++j )
            mat_traits<R>::write_element_idx(i,j,r)=mat_traits<A>::read_element_idx(i,j,a)+mat_traits<B>::read_element_idx(i,j,b);
    return r;
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T>
    class
    mref_
        {
        mref_( mref_ const & );
        mref_ & operator=( mref_ const & );
        ~mref_();

        public:

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        mref_ &
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

template <class M>
struct
mat_traits< qvm_detail::mref_<M> >
    {
    typedef typename mat_traits<M>::scalar_type scalar_type;
    typedef qvm_detail::mref_<M> this_matrix;
    static int const rows=mat_traits<M>::rows;
    static int const cols=mat_traits<M>::cols;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<cols);
        return mat_traits<M>::template read_element<Row,Col>(reinterpret_cast<M const &>(x));
        }

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_matrix & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<cols);
        return mat_traits<M>::template write_element<Row,Col>(reinterpret_cast<M &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<cols);
        return mat_traits<M>::read_element_idx(row,col,reinterpret_cast<M const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int row, int col, this_matrix & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<cols);
        return mat_traits<M>::write_element_idx(row,col,reinterpret_cast<M &>(x));
        }
    };

template <class M,int R,int C>
struct
deduce_mat<qvm_detail::mref_<M>,R,C>
    {
    typedef mat<typename mat_traits<M>::scalar_type,R,C> type;
    };

template <class M>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_mat<M>::value,
    qvm_detail::mref_<M> const &>::type
mref( M const & a )
    {
    return reinterpret_cast<qvm_detail::mref_<M> const &>(a);
    }

template <class M>
BOOST_QVM_INLINE_TRIVIAL
typename enable_if_c<
    is_mat<M>::value,
    qvm_detail::mref_<M> &>::type
mref( M & a )
    {
    return reinterpret_cast<qvm_detail::mref_<M> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class T,int Rows,int Cols>
    class
    zero_mat_
        {
        zero_mat_( zero_mat_ const & );
        zero_mat_ & operator=( zero_mat_ const & );
        ~zero_mat_();

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

template <class T,int Rows,int Cols>
struct
mat_traits< qvm_detail::zero_mat_<T,Rows,Cols> >
    {
    typedef qvm_detail::zero_mat_<T,Rows,Cols> this_matrix;
    typedef T scalar_type;
    static int const rows=Rows;
    static int const cols=Cols;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<Rows);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<Cols);
        return scalar_traits<scalar_type>::value(0);
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<cols);
        return scalar_traits<scalar_type>::value(0);
        }
    };

template <class T,int Rows,int Cols,int R,int C>
struct
deduce_mat<qvm_detail::zero_mat_<T,Rows,Cols>,R,C>
    {
    typedef mat<T,R,C> type;
    };

template <class T,int Rows,int Cols>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::zero_mat_<T,Rows,Cols> const &
zero_mat()
    {
    return *(qvm_detail::zero_mat_<T,Rows,Cols> const *)qvm_detail::get_valid_ptr_mat_operations();
    }

template <class T,int Dim>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::zero_mat_<T,Dim,Dim> const &
zero_mat()
    {
    return *(qvm_detail::zero_mat_<T,Dim,Dim> const *)qvm_detail::get_valid_ptr_mat_operations();
    }

template <class A>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value,
    void>::type
set_zero( A & a )
    {
    assign(a,zero_mat<typename mat_traits<A>::scalar_type,mat_traits<A>::rows,mat_traits<A>::cols>());
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D,class S>
    struct
    rot_mat_
        {
        typedef S scalar_type;
        scalar_type a[3][3];

        BOOST_QVM_INLINE
        rot_mat_(
                scalar_type a00, scalar_type a01, scalar_type a02,
                scalar_type a10, scalar_type a11, scalar_type a12,
                scalar_type a20, scalar_type a21, scalar_type a22 )
            {
            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
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

    template <int Row,int Col>
    struct
    rot_m_get
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const (&)[3][3] )
            {
            return scalar_traits<T>::value(Row==Col);
            }
        };

    template <> struct rot_m_get<0,0> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( T const (&a)[3][3] ) { return a[0][0]; } };
    template <> struct rot_m_get<0,1> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( T const (&a)[3][3] ) { return a[0][1]; } };
    template <> struct rot_m_get<0,2> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( T const (&a)[3][3] ) { return a[0][2]; } };
    template <> struct rot_m_get<1,0> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( T const (&a)[3][3] ) { return a[1][0]; } };
    template <> struct rot_m_get<1,1> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( T const (&a)[3][3] ) { return a[1][1]; } };
    template <> struct rot_m_get<1,2> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( T const (&a)[3][3] ) { return a[1][2]; } };
    template <> struct rot_m_get<2,0> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( T const (&a)[3][3] ) { return a[2][0]; } };
    template <> struct rot_m_get<2,1> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( T const (&a)[3][3] ) { return a[2][1]; } };
    template <> struct rot_m_get<2,2> { template <class T> static BOOST_QVM_INLINE_CRITICAL T get( T const (&a)[3][3] ) { return a[2][2]; } };
    }

template <class M>
struct mat_traits;

template <int D,class S>
struct
mat_traits< qvm_detail::rot_mat_<D,S> >
    {
    typedef qvm_detail::rot_mat_<D,S> this_matrix;
    typedef typename this_matrix::scalar_type scalar_type;
    static int const rows=D;
    static int const cols=D;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<D);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<D);
        return qvm_detail::rot_m_get<Row,Col>::get(x.a);
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<D);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<D);
        return row<3 && col<3?
            x.a[row][col] :
            scalar_traits<scalar_type>::value(row==col);
        }
    };

template <int Dim,class V,class Angle>
BOOST_QVM_INLINE
typename enable_if_c<
    is_vec<V>::value && vec_traits<V>::dim==3,
    qvm_detail::rot_mat_<Dim,Angle> >::type
rot_mat( V const & axis, Angle angle )
    {
    typedef Angle scalar_type;
    scalar_type const x=vec_traits<V>::template read_element<0>(axis);
    scalar_type const y=vec_traits<V>::template read_element<1>(axis);
    scalar_type const z=vec_traits<V>::template read_element<2>(axis);
    scalar_type const m2=x*x+y*y+z*z;
    if( m2==scalar_traits<scalar_type>::value(0) )
        BOOST_QVM_THROW_EXCEPTION(zero_magnitude_error());
    scalar_type const s = sin<scalar_type>(angle);
    scalar_type const c = cos<scalar_type>(angle);
    scalar_type const x2 = x*x;
    scalar_type const y2 = y*y;
    scalar_type const z2 = z*z;
    scalar_type const xy = x*y;
    scalar_type const xz = x*z;
    scalar_type const yz = y*z;
    scalar_type const xs = x*s;
    scalar_type const ys = y*s;
    scalar_type const zs = z*s;
    scalar_type const one = scalar_traits<scalar_type>::value(1);
    scalar_type const c1 = one-c;
    return qvm_detail::rot_mat_<Dim,Angle>(
        x2+(one-x2)*c, xy*c1-zs, xz*(one-c)+ys,
        xy*c1+zs, y2+(one-y2)*c, yz*c1-xs,
        xz*c1-ys, yz*c1+xs, z2+(one-z2)*c );
    }

template <class A,class B,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3 &&
    is_vec<B>::value && vec_traits<B>::dim==3,
    void>::type
set_rot( A & a, B const & axis, Angle angle )
    {
    assign(a,rot_mat<mat_traits<A>::rows>(axis,angle));
    }

template <class A,class B,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3 &&
    is_vec<B>::value && vec_traits<B>::dim==3,
    void>::type
rotate( A & a, B const & axis, Angle angle )
    {
    a *= rot_mat<mat_traits<A>::rows>(axis,angle);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_xzy( Angle x1, Angle z2, Angle y3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(x1);
    scalar_type const s1 = sin<scalar_type>(x1);
    scalar_type const c2 = cos<scalar_type>(z2);
    scalar_type const s2 = sin<scalar_type>(z2);
    scalar_type const c3 = cos<scalar_type>(y3);
    scalar_type const s3 = sin<scalar_type>(y3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c2*c3, -s2, c2*s3,
        s1*s3 + c1*c3*s2, c1*c2, c1*s2*s3 - c3*s1,
        c3*s1*s2 - c1*s3, c2*s1, c1*c3 + s1*s2*s3 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_xzy( A & a, Angle x1, Angle z2, Angle y3 )
    {
    assign(a,rot_mat_xzy<mat_traits<A>::rows>(x1,z2,y3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_xzy( A & a, Angle x1, Angle z2, Angle y3 )
    {
    a *= rot_mat_xzy<mat_traits<A>::rows>(x1,z2,y3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_xyz( Angle x1, Angle y2, Angle z3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(x1);
    scalar_type const s1 = sin<scalar_type>(x1);
    scalar_type const c2 = cos<scalar_type>(y2);
    scalar_type const s2 = sin<scalar_type>(y2);
    scalar_type const c3 = cos<scalar_type>(z3);
    scalar_type const s3 = sin<scalar_type>(z3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c2*c3, -c2*s3, s2,
        c1*s3 + c3*s1*s2, c1*c3 - s1*s2*s3, -c2*s1,
        s1*s3 - c1*c3*s2, c3*s1 + c1*s2*s3, c1*c2 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_xyz( A & a, Angle x1, Angle y2, Angle z3 )
    {
    assign(a,rot_mat_xyz<mat_traits<A>::rows>(x1,y2,z3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_xyz( A & a, Angle x1, Angle y2, Angle z3 )
    {
    a *= rot_mat_xyz<mat_traits<A>::rows>(x1,y2,z3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_yxz( Angle y1, Angle x2, Angle z3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(y1);
    scalar_type const s1 = sin<scalar_type>(y1);
    scalar_type const c2 = cos<scalar_type>(x2);
    scalar_type const s2 = sin<scalar_type>(x2);
    scalar_type const c3 = cos<scalar_type>(z3);
    scalar_type const s3 = sin<scalar_type>(z3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c1*c3 + s1*s2*s3, c3*s1*s2 - c1*s3, c2*s1,
        c2*s3, c2*c3, -s2,
        c1*s2*s3 - c3*s1, c1*c3*s2 + s1*s3, c1*c2 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_yxz( A & a, Angle y1, Angle x2, Angle z3 )
    {
    assign(a,rot_mat_yxz<mat_traits<A>::rows>(y1,x2,z3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_yxz( A & a, Angle y1, Angle x2, Angle z3 )
    {
    a *= rot_mat_yxz<mat_traits<A>::rows>(y1,x2,z3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_yzx( Angle y1, Angle z2, Angle x3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(y1);
    scalar_type const s1 = sin<scalar_type>(y1);
    scalar_type const c2 = cos<scalar_type>(z2);
    scalar_type const s2 = sin<scalar_type>(z2);
    scalar_type const c3 = cos<scalar_type>(x3);
    scalar_type const s3 = sin<scalar_type>(x3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c1*c2, s1*s3 - c1*c3*s2, c3*s1 + c1*s2*s3,
        s2, c2*c3, -c2*s3,
        -c2*s1, c1*s3 + c3*s1*s2, c1*c3 - s1*s2*s3 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_yzx( A & a, Angle y1, Angle z2, Angle x3 )
    {
    assign(a,rot_mat_yzx<mat_traits<A>::rows>(y1,z2,x3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_yzx( A & a, Angle y1, Angle z2, Angle x3 )
    {
    a *= rot_mat_yzx<mat_traits<A>::rows>(y1,z2,x3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_zyx( Angle z1, Angle y2, Angle x3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(z1);
    scalar_type const s1 = sin<scalar_type>(z1);
    scalar_type const c2 = cos<scalar_type>(y2);
    scalar_type const s2 = sin<scalar_type>(y2);
    scalar_type const c3 = cos<scalar_type>(x3);
    scalar_type const s3 = sin<scalar_type>(x3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c1*c2, c1*s2*s3 - c3*s1, s1*s3 + c1*c3*s2,
        c2*s1, c1*c3 + s1*s2*s3, c3*s1*s2 - c1*s3,
        -s2, c2*s3, c2*c3 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_zyx( A & a, Angle z1, Angle y2, Angle x3 )
    {
    assign(a,rot_mat_zyx<mat_traits<A>::rows>(z1,y2,x3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_zyx( A & a, Angle z1, Angle y2, Angle x3 )
    {
    a *= rot_mat_zyx<mat_traits<A>::rows>(z1,y2,x3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_zxy( Angle z1, Angle x2, Angle y3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(z1);
    scalar_type const s1 = sin<scalar_type>(z1);
    scalar_type const c2 = cos<scalar_type>(x2);
    scalar_type const s2 = sin<scalar_type>(x2);
    scalar_type const c3 = cos<scalar_type>(y3);
    scalar_type const s3 = sin<scalar_type>(y3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c1*c3 - s1*s2*s3, -c2*s1, c1*s3 + c3*s1*s2,
        c3*s1 + c1*s2*s3, c1*c2, s1*s3 - c1*c3*s2,
        -c2*s3, s2, c2*c3 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_zxy( A & a, Angle z1, Angle x2, Angle y3 )
    {
    assign(a,rot_mat_zxy<mat_traits<A>::rows>(z1,x2,y3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_zxy( A & a, Angle z1, Angle x2, Angle y3 )
    {
    a *= rot_mat_zxy<mat_traits<A>::rows>(z1,x2,y3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_xzx( Angle x1, Angle z2, Angle x3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(x1);
    scalar_type const s1 = sin<scalar_type>(x1);
    scalar_type const c2 = cos<scalar_type>(z2);
    scalar_type const s2 = sin<scalar_type>(z2);
    scalar_type const c3 = cos<scalar_type>(x3);
    scalar_type const s3 = sin<scalar_type>(x3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c2, -c3*s2, s2*s3,
        c1*s2, c1*c2*c3 - s1*s3, -c3*s1 - c1*c2*s3,
        s1*s2, c1*s3 + c2*c3*s1, c1*c3 - c2*s1*s3 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_xzx( A & a, Angle x1, Angle z2, Angle x3 )
    {
    assign(a,rot_mat_xzx<mat_traits<A>::rows>(x1,z2,x3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_xzx( A & a, Angle x1, Angle z2, Angle x3 )
    {
    a *= rot_mat_xzx<mat_traits<A>::rows>(x1,z2,x3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_xyx( Angle x1, Angle y2, Angle x3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(x1);
    scalar_type const s1 = sin<scalar_type>(x1);
    scalar_type const c2 = cos<scalar_type>(y2);
    scalar_type const s2 = sin<scalar_type>(y2);
    scalar_type const c3 = cos<scalar_type>(x3);
    scalar_type const s3 = sin<scalar_type>(x3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c2, s2*s3, c3*s2,
        s1*s2, c1*c3 - c2*s1*s3, -c1*s3 - c2*c3*s1,
        -c1*s2, c3*s1 + c1*c2*s3, c1*c2*c3 - s1*s3 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_xyx( A & a, Angle x1, Angle y2, Angle x3 )
    {
    assign(a,rot_mat_xyx<mat_traits<A>::rows>(x1,y2,x3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_xyx( A & a, Angle x1, Angle y2, Angle x3 )
    {
    a *= rot_mat_xyx<mat_traits<A>::rows>(x1,y2,x3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_yxy( Angle y1, Angle x2, Angle y3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(y1);
    scalar_type const s1 = sin<scalar_type>(y1);
    scalar_type const c2 = cos<scalar_type>(x2);
    scalar_type const s2 = sin<scalar_type>(x2);
    scalar_type const c3 = cos<scalar_type>(y3);
    scalar_type const s3 = sin<scalar_type>(y3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c1*c3 - c2*s1*s3, s1*s2, c1*s3 + c2*c3*s1,
        s2*s3, c2, -c3*s2,
        -c3*s1 - c1*c2*s3, c1*s2, c1*c2*c3 - s1*s3 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_yxy( A & a, Angle y1, Angle x2, Angle y3 )
    {
    assign(a,rot_mat_yxy<mat_traits<A>::rows>(y1,x2,y3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_yxy( A & a, Angle y1, Angle x2, Angle y3 )
    {
    a *= rot_mat_yxy<mat_traits<A>::rows>(y1,x2,y3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_yzy( Angle y1, Angle z2, Angle y3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(y1);
    scalar_type const s1 = sin<scalar_type>(y1);
    scalar_type const c2 = cos<scalar_type>(z2);
    scalar_type const s2 = sin<scalar_type>(z2);
    scalar_type const c3 = cos<scalar_type>(y3);
    scalar_type const s3 = sin<scalar_type>(y3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c1*c2*c3 - s1*s3, -c1*s2, c3*s1 + c1*c2*s3,
        c3*s2, c2, s2*s3,
        -c1*s3 - c2*c3*s1, s1*s2, c1*c3 - c2*s1*s3 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_yzy( A & a, Angle y1, Angle z2, Angle y3 )
    {
    assign(a,rot_mat_yzy<mat_traits<A>::rows>(y1,z2,y3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_yzy( A & a, Angle y1, Angle z2, Angle y3 )
    {
    a *= rot_mat_yzy<mat_traits<A>::rows>(y1,z2,y3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_zyz( Angle z1, Angle y2, Angle z3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(z1);
    scalar_type const s1 = sin<scalar_type>(z1);
    scalar_type const c2 = cos<scalar_type>(y2);
    scalar_type const s2 = sin<scalar_type>(y2);
    scalar_type const c3 = cos<scalar_type>(z3);
    scalar_type const s3 = sin<scalar_type>(z3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c1*c2*c3 - s1*s3, -c3*s1 - c1*c2*s3, c1*s2,
        c1*s3 + c2*c3*s1, c1*c3 - c2*s1*s3, s1*s2,
        -c3*s2, s2*s3, c2 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_zyz( A & a, Angle z1, Angle y2, Angle z3 )
    {
    assign(a,rot_mat_zyz<mat_traits<A>::rows>(z1,y2,z3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_zyz( A & a, Angle z1, Angle y2, Angle z3 )
    {
    a *= rot_mat_zyz<mat_traits<A>::rows>(z1,y2,z3);
    }

////////////////////////////////////////////////

template <int Dim,class Angle>
BOOST_QVM_INLINE
qvm_detail::rot_mat_<Dim,Angle>
rot_mat_zxz( Angle z1, Angle x2, Angle z3 )
    {
    typedef Angle scalar_type;
    scalar_type const c1 = cos<scalar_type>(z1);
    scalar_type const s1 = sin<scalar_type>(z1);
    scalar_type const c2 = cos<scalar_type>(x2);
    scalar_type const s2 = sin<scalar_type>(x2);
    scalar_type const c3 = cos<scalar_type>(z3);
    scalar_type const s3 = sin<scalar_type>(z3);
    return qvm_detail::rot_mat_<Dim,Angle>(
        c1*c3 - c2*s1*s3, -c1*s3 - c2*c3*s1, s1*s2,
        c3*s1 + c1*c2*s3, c1*c2*c3 - s1*s3, -c1*s2,
        s2*s3, c3*s2, c2 );
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
set_rot_zxz( A & a, Angle z1, Angle x2, Angle z3 )
    {
    assign(a,rot_mat_zxz<mat_traits<A>::rows>(z1,x2,z3));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    mat_traits<A>::rows>=3,
    void>::type
rotate_zxz( A & a, Angle z1, Angle x2, Angle z3 )
    {
    a *= rot_mat_zxz<mat_traits<A>::rows>(z1,x2,z3);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Dim,class Angle>
    struct
    rotx_mat_
        {
        BOOST_QVM_INLINE_TRIVIAL
        rotx_mat_()
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

        rotx_mat_( rotx_mat_ const & );
        rotx_mat_ & operator=( rotx_mat_ const & );
        ~rotx_mat_();
        };

    template <int Row,int Col>
    struct
    rotx_m_get
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & )
            {
            return scalar_traits<T>::value(Row==Col);
            }
        };

    template <>
    struct
    rotx_m_get<1,1>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return cos<T>(angle);
            }
        };

    template <>
    struct
    rotx_m_get<1,2>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return -sin<T>(angle);
            }
        };

    template <>
    struct
    rotx_m_get<2,1>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return sin<T>(angle);
            }
        };

    template <>
    struct
    rotx_m_get<2,2>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return cos<T>(angle);
            }
        };
    }

template <int Dim,class Angle>
struct
mat_traits< qvm_detail::rotx_mat_<Dim,Angle> >
    {
    typedef qvm_detail::rotx_mat_<Dim,Angle> this_matrix;
    typedef Angle scalar_type;
    static int const rows=Dim;
    static int const cols=Dim;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Row<Dim);
        BOOST_QVM_STATIC_ASSERT(Col<Dim);
        return qvm_detail::rotx_m_get<Row,Col>::get(reinterpret_cast<Angle const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(row<Dim);
        BOOST_QVM_ASSERT(col<Dim);
        Angle const & a=reinterpret_cast<Angle const &>(x);
        if( row==1 )
            {
            if( col==1 )
                return cos<scalar_type>(a);
            if( col==2 )
                return -sin<scalar_type>(a);
            }
        if( row==2 )
            {
            if( col==1 )
                return sin<scalar_type>(a);
            if( col==2 )
                return cos<scalar_type>(a);
            }
        return scalar_traits<scalar_type>::value(row==col);
        }
    };

template <int Dim,class Angle>
struct
deduce_mat<qvm_detail::rotx_mat_<Dim,Angle>,Dim,Dim>
    {
    typedef mat<Angle,Dim,Dim> type;
    };

template <int Dim,class Angle>
struct
deduce_mat2<qvm_detail::rotx_mat_<Dim,Angle>,qvm_detail::rotx_mat_<Dim,Angle>,Dim,Dim>
    {
    typedef mat<Angle,Dim,Dim> type;
    };

template <int Dim,class Angle>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::rotx_mat_<Dim,Angle> const &
rotx_mat( Angle const & angle )
    {
    BOOST_QVM_STATIC_ASSERT(Dim>=3);
    return reinterpret_cast<qvm_detail::rotx_mat_<Dim,Angle> const &>(angle);
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows>=3 &&
    mat_traits<A>::rows==mat_traits<A>::cols,
    void>::type
set_rotx( A & a, Angle angle )
    {
    assign(a,rotx_mat<mat_traits<A>::rows>(angle));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows>=3 &&
    mat_traits<A>::rows==mat_traits<A>::cols,
    void>::type
rotate_x( A & a, Angle angle )
    {
    a *= rotx_mat<mat_traits<A>::rows>(angle);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Dim,class Angle>
    struct
    roty_mat_
        {
        BOOST_QVM_INLINE_TRIVIAL
        roty_mat_()
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

        roty_mat_( roty_mat_ const & );
        roty_mat_ & operator=( roty_mat_ const & );
        ~roty_mat_();
        };

    template <int Row,int Col>
    struct
    roty_m_get
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & )
            {
            return scalar_traits<T>::value(Row==Col);
            }
        };

    template <>
    struct
    roty_m_get<0,0>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return cos<T>(angle);
            }
        };

    template <>
    struct
    roty_m_get<0,2>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return sin<T>(angle);
            }
        };

    template <>
    struct
    roty_m_get<2,0>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return -sin<T>(angle);
            }
        };

    template <>
    struct
    roty_m_get<2,2>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return cos<T>(angle);
            }
        };
    }

template <int Dim,class Angle>
struct
mat_traits< qvm_detail::roty_mat_<Dim,Angle> >
    {
    typedef qvm_detail::roty_mat_<Dim,Angle> this_matrix;
    typedef Angle scalar_type;
    static int const rows=Dim;
    static int const cols=Dim;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Row<Dim);
        BOOST_QVM_STATIC_ASSERT(Col<Dim);
        return qvm_detail::roty_m_get<Row,Col>::get(reinterpret_cast<Angle const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(row<Dim);
        BOOST_QVM_ASSERT(col<Dim);
        Angle const & a=reinterpret_cast<Angle const &>(x);
        if( row==0 )
            {
            if( col==0 )
                return cos<scalar_type>(a);
            if( col==2 )
                return sin<scalar_type>(a);
            }
        if( row==2 )
            {
            if( col==0 )
                return -sin<scalar_type>(a);
            if( col==2 )
                return cos<scalar_type>(a);
            }
        return scalar_traits<scalar_type>::value(row==col);
        }
    };

template <int Dim,class Angle>
struct
deduce_mat<qvm_detail::roty_mat_<Dim,Angle>,Dim,Dim>
    {
    typedef mat<Angle,Dim,Dim> type;
    };

template <int Dim,class Angle>
struct
deduce_mat2<qvm_detail::roty_mat_<Dim,Angle>,qvm_detail::roty_mat_<Dim,Angle>,Dim,Dim>
    {
    typedef mat<Angle,Dim,Dim> type;
    };

template <int Dim,class Angle>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::roty_mat_<Dim,Angle> const &
roty_mat( Angle const & angle )
    {
    BOOST_QVM_STATIC_ASSERT(Dim>=3);
    return reinterpret_cast<qvm_detail::roty_mat_<Dim,Angle> const &>(angle);
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows>=2 &&
    mat_traits<A>::rows==mat_traits<A>::cols,
    void>::type
set_roty( A & a, Angle angle )
    {
    assign(a,roty_mat<mat_traits<A>::rows>(angle));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows>=3 &&
    mat_traits<A>::rows==mat_traits<A>::cols,
    void>::type
rotate_y( A & a, Angle angle )
    {
    a *= roty_mat<mat_traits<A>::rows>(angle);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Dim,class Angle>
    struct
    rotz_mat_
        {
        BOOST_QVM_INLINE_TRIVIAL
        rotz_mat_()
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

        rotz_mat_( rotz_mat_ const & );
        rotz_mat_ & operator=( rotz_mat_ const & );
        ~rotz_mat_();
        };

    template <int Row,int Col>
    struct
    rotz_m_get
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & )
            {
            return scalar_traits<T>::value(Row==Col);
            }
        };

    template <>
    struct
    rotz_m_get<0,0>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return cos<T>(angle);
            }
        };

    template <>
    struct
    rotz_m_get<0,1>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return -sin<T>(angle);
            }
        };

    template <>
    struct
    rotz_m_get<1,0>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return sin<T>(angle);
            }
        };

    template <>
    struct
    rotz_m_get<1,1>
        {
        template <class T>
        static
        BOOST_QVM_INLINE_CRITICAL
        T
        get( T const & angle )
            {
            return cos<T>(angle);
            }
        };
    }

template <int Dim,class Angle>
struct
mat_traits< qvm_detail::rotz_mat_<Dim,Angle> >
    {
    typedef qvm_detail::rotz_mat_<Dim,Angle> this_matrix;
    typedef Angle scalar_type;
    static int const rows=Dim;
    static int const cols=Dim;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Row<Dim);
        BOOST_QVM_STATIC_ASSERT(Col<Dim);
        return qvm_detail::rotz_m_get<Row,Col>::get(reinterpret_cast<Angle const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(row<Dim);
        BOOST_QVM_ASSERT(col<Dim);
        Angle const & a=reinterpret_cast<Angle const &>(x);
        if( row==0 )
            {
            if( col==0 )
                return cos<scalar_type>(a);
            if( col==1 )
                return -sin<scalar_type>(a);
            }
        if( row==1 )
            {
            if( col==0 )
                return sin<scalar_type>(a);
            if( col==1 )
                return cos<scalar_type>(a);
            }
        return scalar_traits<scalar_type>::value(row==col);
        }
    };

template <int Dim,class Angle>
struct
deduce_mat<qvm_detail::rotz_mat_<Dim,Angle>,Dim,Dim>
    {
    typedef mat<Angle,Dim,Dim> type;
    };

template <int Dim,class Angle,int R,int C>
struct
deduce_mat2<qvm_detail::rotz_mat_<Dim,Angle>,qvm_detail::rotz_mat_<Dim,Angle>,R,C>
    {
    typedef mat<Angle,R,C> type;
    };

template <int Dim,class Angle>
BOOST_QVM_INLINE_TRIVIAL
qvm_detail::rotz_mat_<Dim,Angle> const &
rotz_mat( Angle const & angle )
    {
    BOOST_QVM_STATIC_ASSERT(Dim>=2);
    return reinterpret_cast<qvm_detail::rotz_mat_<Dim,Angle> const &>(angle);
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows>=2 &&
    mat_traits<A>::rows==mat_traits<A>::cols,
    void>::type
set_rotz( A & a, Angle angle )
    {
    assign(a,rotz_mat<mat_traits<A>::rows>(angle));
    }

template <class A,class Angle>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows>=2 &&
    mat_traits<A>::rows==mat_traits<A>::cols,
    void>::type
rotate_z( A & a, Angle angle )
    {
    a *= rotz_mat<mat_traits<A>::rows>(angle);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int D>
    struct
    inverse_m_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_TRIVIAL
typename lazy_enable_if_c<
    is_mat<A>::value && is_scalar<B>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    !qvm_detail::inverse_m_defined<mat_traits<A>::rows>::value,
    deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols> >::type
inverse( A const & a, B det )
    {
    typedef typename mat_traits<A>::scalar_type T;
    BOOST_QVM_ASSERT(det!=scalar_traits<T>::value(0));
    T f=scalar_traits<T>::value(1)/det;
    typedef typename deduce_mat2<A,B,mat_traits<A>::rows,mat_traits<A>::cols>::type cofactor_return_type;
    cofactor_return_type c=qvm_detail::cofactor_impl(a);
    return reinterpret_cast<qvm_detail::transposed_<cofactor_return_type> const &>(c) * f;
    }

template <class A>
BOOST_QVM_INLINE_TRIVIAL
typename lazy_enable_if_c<
    is_mat<A>::value &&
    mat_traits<A>::rows==mat_traits<A>::cols &&
    !qvm_detail::inverse_m_defined<mat_traits<A>::rows>::value,
    deduce_mat<A> >::type
inverse( A const & a )
    {
    typedef typename mat_traits<A>::scalar_type T;
    T det=determinant(a);
    if( det==scalar_traits<T>::value(0) )
        BOOST_QVM_THROW_EXCEPTION(zero_determinant_error());
    return inverse(a,det);
    }

////////////////////////////////////////////////

namespace
sfinae
    {
    using ::boost::qvm::to_string;
    using ::boost::qvm::assign;
    using ::boost::qvm::determinant;
    using ::boost::qvm::cmp;
    using ::boost::qvm::convert_to;
    using ::boost::qvm::set_identity;
    using ::boost::qvm::set_zero;
    using ::boost::qvm::scalar_cast;
    using ::boost::qvm::operator/=;
    using ::boost::qvm::operator/;
    using ::boost::qvm::operator==;
    using ::boost::qvm::operator-=;
    using ::boost::qvm::operator-;
    using ::boost::qvm::operator*=;
    using ::boost::qvm::operator*;
    using ::boost::qvm::operator!=;
    using ::boost::qvm::operator+=;
    using ::boost::qvm::operator+;
    using ::boost::qvm::mref;
    using ::boost::qvm::rot_mat;
    using ::boost::qvm::set_rot;
    using ::boost::qvm::rotate;
    using ::boost::qvm::set_rotx;
    using ::boost::qvm::rotate_x;
    using ::boost::qvm::set_roty;
    using ::boost::qvm::rotate_y;
    using ::boost::qvm::set_rotz;
    using ::boost::qvm::rotate_z;
    using ::boost::qvm::inverse;
    }

} }

#endif
