#ifndef BOOST_QVM_MAP_MAT_VEC_HPP_INCLUDED
#define BOOST_QVM_MAP_MAT_VEC_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/mat_traits.hpp>
#include <boost/qvm/deduce_vec.hpp>
#include <boost/qvm/assert.hpp>
#include <boost/qvm/enable_if.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    template <int Col,class OriginalMatrix>
    class
    col_
        {
        col_( col_ const & );
        col_ & operator=( col_ const & );
        ~col_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        col_ &
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
    }

template <int Col,class OriginalMatrix>
struct
vec_traits< qvm_detail::col_<Col,OriginalMatrix> >
    {
    typedef qvm_detail::col_<Col,OriginalMatrix> this_vector;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const dim=mat_traits<OriginalMatrix>::rows;
    BOOST_QVM_STATIC_ASSERT(Col>=0);
    BOOST_QVM_STATIC_ASSERT(Col<mat_traits<OriginalMatrix>::cols);

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return mat_traits<OriginalMatrix>::template read_element<I,Col>(reinterpret_cast<OriginalMatrix const &>(x));
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_vector & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return mat_traits<OriginalMatrix>::template write_element<I,Col>(reinterpret_cast<OriginalMatrix &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_vector const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return mat_traits<OriginalMatrix>::read_element_idx(i,Col,reinterpret_cast<OriginalMatrix const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int i, this_vector & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return mat_traits<OriginalMatrix>::write_element_idx(i,Col,reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <int Col,class OriginalMatrix,int D>
struct
deduce_vec<qvm_detail::col_<Col,OriginalMatrix>,D>
    {
    typedef vec<typename mat_traits<OriginalMatrix>::scalar_type,D> type;
    };

template <int Col,class OriginalMatrix,int D>
struct
deduce_vec2<qvm_detail::col_<Col,OriginalMatrix>,qvm_detail::col_<Col,OriginalMatrix>,D>
    {
    typedef vec<typename mat_traits<OriginalMatrix>::scalar_type,D> type;
    };

template <int Col,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::col_<Col,A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
col( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::col_<Col,A> const &>(a);
    }

template <int Col,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::col_<Col,A> &>::type
BOOST_QVM_INLINE_TRIVIAL
col( A & a )
    {
    return reinterpret_cast<typename qvm_detail::col_<Col,A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Row,class OriginalMatrix>
    class
    row_
        {
        row_( row_ const & );
        row_ & operator=( row_ const & );
        ~row_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        row_ &
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
    }

template <int Row,class OriginalMatrix>
struct
vec_traits< qvm_detail::row_<Row,OriginalMatrix> >
    {
    typedef qvm_detail::row_<Row,OriginalMatrix> this_vector;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const dim=mat_traits<OriginalMatrix>::cols;
    BOOST_QVM_STATIC_ASSERT(Row>=0);
    BOOST_QVM_STATIC_ASSERT(Row<mat_traits<OriginalMatrix>::rows);

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return mat_traits<OriginalMatrix>::template read_element<Row,I>(reinterpret_cast<OriginalMatrix const &>(x));
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_vector & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return mat_traits<OriginalMatrix>::template write_element<Row,I>(reinterpret_cast<OriginalMatrix &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_vector const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return mat_traits<OriginalMatrix>::read_element_idx(Row,i,reinterpret_cast<OriginalMatrix const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int i, this_vector & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return mat_traits<OriginalMatrix>::write_element_idx(Row,i,reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <int Row,class OriginalMatrix,int D>
struct
deduce_vec<qvm_detail::row_<Row,OriginalMatrix>,D>
    {
    typedef vec<typename mat_traits<OriginalMatrix>::scalar_type,D> type;
    };

template <int Row,class OriginalMatrix,int D>
struct
deduce_vec2<qvm_detail::row_<Row,OriginalMatrix>,qvm_detail::row_<Row,OriginalMatrix>,D>
    {
    typedef vec<typename mat_traits<OriginalMatrix>::scalar_type,D> type;
    };

template <int Row,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::row_<Row,A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
row( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::row_<Row,A> const &>(a);
    }

template <int Row,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::row_<Row,A> &>::type
BOOST_QVM_INLINE_TRIVIAL
row( A & a )
    {
    return reinterpret_cast<typename qvm_detail::row_<Row,A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class OriginalMatrix>
    class
    diag_
        {
        diag_( diag_ const & );
        diag_ & operator=( diag_ const & );
        ~diag_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        diag_ &
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

    template <int X,int Y,bool Which>
    struct diag_bool_dispatch;

    template <int X,int Y>
    struct
    diag_bool_dispatch<X,Y,true>
        {
        static int const value=X;
        };

    template <int X,int Y>
    struct
    diag_bool_dispatch<X,Y,false>
        {
        static int const value=Y;
        };
    }

template <class OriginalMatrix>
struct
vec_traits< qvm_detail::diag_<OriginalMatrix> >
    {
    typedef qvm_detail::diag_<OriginalMatrix> this_vector;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const dim=qvm_detail::diag_bool_dispatch<
            mat_traits<OriginalMatrix>::rows,
            mat_traits<OriginalMatrix>::cols,
            mat_traits<OriginalMatrix>::rows<=mat_traits<OriginalMatrix>::cols>::value;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return mat_traits<OriginalMatrix>::template read_element<I,I>(reinterpret_cast<OriginalMatrix const &>(x));
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_vector & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return mat_traits<OriginalMatrix>::template write_element<I,I>(reinterpret_cast<OriginalMatrix &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_vector const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return mat_traits<OriginalMatrix>::read_element_idx(i,i,reinterpret_cast<OriginalMatrix const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int i, this_vector & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return mat_traits<OriginalMatrix>::write_element_idx(i,i,reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <class OriginalMatrix,int D>
struct
deduce_vec<qvm_detail::diag_<OriginalMatrix>,D>
    {
    typedef vec<typename mat_traits<OriginalMatrix>::scalar_type,D> type;
    };

template <class OriginalMatrix,int D>
struct
deduce_vec2<qvm_detail::diag_<OriginalMatrix>,qvm_detail::diag_<OriginalMatrix>,D>
    {
    typedef vec<typename mat_traits<OriginalMatrix>::scalar_type,D> type;
    };

template <class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::diag_<A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
diag( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::diag_<A> const &>(a);
    }

template <class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::diag_<A> &>::type
BOOST_QVM_INLINE_TRIVIAL
diag( A & a )
    {
    return reinterpret_cast<typename qvm_detail::diag_<A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class OriginalMatrix>
    class
    translation_
        {
        translation_( translation_ const & );
        ~translation_();

        public:

        translation_ &
        operator=( translation_ const & x )
            {
            assign(*this,x);
            return *this;
            }

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        translation_ &
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
    }

template <class OriginalMatrix>
struct
vec_traits< qvm_detail::translation_<OriginalMatrix> >
    {
    typedef qvm_detail::translation_<OriginalMatrix> this_vector;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const dim=mat_traits<OriginalMatrix>::rows-1;
    BOOST_QVM_STATIC_ASSERT(mat_traits<OriginalMatrix>::rows==mat_traits<OriginalMatrix>::cols);
    BOOST_QVM_STATIC_ASSERT(mat_traits<OriginalMatrix>::rows>=3);

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return mat_traits<OriginalMatrix>::template read_element<I,dim>(reinterpret_cast<OriginalMatrix const &>(x));
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_vector & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<dim);
        return mat_traits<OriginalMatrix>::template write_element<I,dim>(reinterpret_cast<OriginalMatrix &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_vector const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return mat_traits<OriginalMatrix>::read_element_idx(i,dim,reinterpret_cast<OriginalMatrix const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int i, this_vector & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<dim);
        return mat_traits<OriginalMatrix>::write_element_idx(i,dim,reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <class OriginalMatrix,int D>
struct
deduce_vec<qvm_detail::translation_<OriginalMatrix>,D>
    {
    typedef vec<typename mat_traits<OriginalMatrix>::scalar_type,D> type;
    };

template <class OriginalMatrix,int D>
struct
deduce_vec2<qvm_detail::translation_<OriginalMatrix>,qvm_detail::translation_<OriginalMatrix>,D>
    {
    typedef vec<typename mat_traits<OriginalMatrix>::scalar_type,D> type;
    };

template <class A>
typename enable_if_c<
    is_mat<A>::value && mat_traits<A>::rows==mat_traits<A>::cols && mat_traits<A>::rows>=3,
    qvm_detail::translation_<A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
translation( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::translation_<A> const &>(a);
    }

template <class A>
typename enable_if_c<
    is_mat<A>::value && mat_traits<A>::rows==mat_traits<A>::cols && mat_traits<A>::rows>=3,
    qvm_detail::translation_<A> &>::type
BOOST_QVM_INLINE_TRIVIAL
translation( A & a )
    {
    return reinterpret_cast<typename qvm_detail::translation_<A> &>(a);
    }

} }

#endif
