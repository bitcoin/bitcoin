#ifndef BOOST_QVM_MAP_VEC_MAT_HPP_INCLUDED
#define BOOST_QVM_MAP_VEC_MAT_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/deduce_mat.hpp>
#include <boost/qvm/vec_traits.hpp>
#include <boost/qvm/assert.hpp>
#include <boost/qvm/enable_if.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    template <class OriginalVector>
    class
    col_mat_
        {
        col_mat_( col_mat_ const & );
        col_mat_ & operator=( col_mat_ const & );
        ~col_mat_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        col_mat_ &
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

template <class OriginalVector>
struct
mat_traits< qvm_detail::col_mat_<OriginalVector> >
    {
    typedef qvm_detail::col_mat_<OriginalVector> this_matrix;
    typedef typename vec_traits<OriginalVector>::scalar_type scalar_type;
    static int const rows=vec_traits<OriginalVector>::dim;
    static int const cols=1;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Col==0);
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        return vec_traits<OriginalVector>::template read_element<Row>(reinterpret_cast<OriginalVector const &>(x));
        }

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_matrix & x )
        {
        BOOST_QVM_STATIC_ASSERT(Col==0);
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        return vec_traits<OriginalVector>::template write_element<Row>(reinterpret_cast<OriginalVector &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(col==0);
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        return vec_traits<OriginalVector>::read_element_idx(row,reinterpret_cast<OriginalVector const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int row, int col, this_matrix & x )
        {
        BOOST_QVM_ASSERT(col==0);
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        return vec_traits<OriginalVector>::write_element_idx(row,reinterpret_cast<OriginalVector &>(x));
        }
    };

template <class OriginalVector,int R,int C>
struct
deduce_mat<qvm_detail::col_mat_<OriginalVector>,R,C>
    {
    typedef mat<typename vec_traits<OriginalVector>::scalar_type,R,C> type;
    };

template <class OriginalVector,int R,int C>
struct
deduce_mat2<qvm_detail::col_mat_<OriginalVector>,qvm_detail::col_mat_<OriginalVector>,R,C>
    {
    typedef mat<typename vec_traits<OriginalVector>::scalar_type,R,C> type;
    };

template <class A>
typename enable_if_c<
    is_vec<A>::value,
    qvm_detail::col_mat_<A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
col_mat( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::col_mat_<A> const &>(a);
    }

template <class A>
typename enable_if_c<
    is_vec<A>::value,
    qvm_detail::col_mat_<A> &>::type
BOOST_QVM_INLINE_TRIVIAL
col_mat( A & a )
    {
    return reinterpret_cast<typename qvm_detail::col_mat_<A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class OriginalVector>
    class
    row_mat_
        {
        row_mat_( row_mat_ const & );
        row_mat_ & operator=( row_mat_ const & );
        ~row_mat_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        row_mat_ &
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

template <class OriginalVector>
struct
mat_traits< qvm_detail::row_mat_<OriginalVector> >
    {
    typedef qvm_detail::row_mat_<OriginalVector> this_matrix;
    typedef typename vec_traits<OriginalVector>::scalar_type scalar_type;
    static int const rows=1;
    static int const cols=vec_traits<OriginalVector>::dim;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row==0);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<cols);
        return vec_traits<OriginalVector>::template read_element<Col>(reinterpret_cast<OriginalVector const &>(x));
        }

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_matrix & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row==0);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<cols);
        return vec_traits<OriginalVector>::template write_element<Col>(reinterpret_cast<OriginalVector &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row==0);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<cols);
        return vec_traits<OriginalVector>::read_element_idx(col,reinterpret_cast<OriginalVector const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int row, int col, this_matrix & x )
        {
        BOOST_QVM_ASSERT(row==0);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<cols);
        return vec_traits<OriginalVector>::write_element_idx(col,reinterpret_cast<OriginalVector &>(x));
        }
    };

template <class OriginalVector,int R,int C>
struct
deduce_mat<qvm_detail::row_mat_<OriginalVector>,R,C>
    {
    typedef mat<typename vec_traits<OriginalVector>::scalar_type,R,C> type;
    };

template <class OriginalVector,int R,int C>
struct
deduce_mat2<qvm_detail::row_mat_<OriginalVector>,qvm_detail::row_mat_<OriginalVector>,R,C>
    {
    typedef mat<typename vec_traits<OriginalVector>::scalar_type,R,C> type;
    };

template <class A>
typename enable_if_c<
    is_vec<A>::value,
    qvm_detail::row_mat_<A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
row_mat( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::row_mat_<A> const &>(a);
    }

template <class A>
typename enable_if_c<
    is_vec<A>::value,
    qvm_detail::row_mat_<A> &>::type
BOOST_QVM_INLINE_TRIVIAL
row_mat( A & a )
    {
    return reinterpret_cast<typename qvm_detail::row_mat_<A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class OriginalVector>
    class
    translation_mat_
        {
        translation_mat_( translation_mat_ const & );
        translation_mat_ & operator=( translation_mat_ const & );
        ~translation_mat_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        translation_mat_ &
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

    template <class M,int Row,int Col,bool TransCol=(Col==mat_traits<M>::cols-1)>
    struct read_translation_matat;

    template <class OriginalVector,int Row,int Col,bool TransCol>
    struct
    read_translation_matat<translation_mat_<OriginalVector>,Row,Col,TransCol>
        {
        static
        BOOST_QVM_INLINE_CRITICAL
        typename mat_traits< translation_mat_<OriginalVector> >::scalar_type
        f( translation_mat_<OriginalVector> const & )
            {
            return scalar_traits<typename mat_traits< translation_mat_<OriginalVector> >::scalar_type>::value(0);
            }
        };

    template <class OriginalVector,int D>
    struct
    read_translation_matat<translation_mat_<OriginalVector>,D,D,false>
        {
        static
        BOOST_QVM_INLINE_CRITICAL
        typename mat_traits< translation_mat_<OriginalVector> >::scalar_type
        f( translation_mat_<OriginalVector> const & )
            {
            return scalar_traits<typename mat_traits< translation_mat_<OriginalVector> >::scalar_type>::value(1);
            }
        };

    template <class OriginalVector,int D>
    struct
    read_translation_matat<translation_mat_<OriginalVector>,D,D,true>
        {
        static
        BOOST_QVM_INLINE_CRITICAL
        typename mat_traits< translation_mat_<OriginalVector> >::scalar_type
        f( translation_mat_<OriginalVector> const & )
            {
            return scalar_traits<typename mat_traits< translation_mat_<OriginalVector> >::scalar_type>::value(1);
            }
        };

    template <class OriginalVector,int Row,int Col>
    struct
    read_translation_matat<translation_mat_<OriginalVector>,Row,Col,true>
        {
        static
        BOOST_QVM_INLINE_CRITICAL
        typename mat_traits< translation_mat_<OriginalVector> >::scalar_type
        f( translation_mat_<OriginalVector> const & x )
            {
            return vec_traits<OriginalVector>::template read_element<Row>(reinterpret_cast<OriginalVector const &>(x));
            }
        };
    }

template <class OriginalVector>
struct
mat_traits< qvm_detail::translation_mat_<OriginalVector> >
    {
    typedef qvm_detail::translation_mat_<OriginalVector> this_matrix;
    typedef typename vec_traits<OriginalVector>::scalar_type scalar_type;
    static int const rows=vec_traits<OriginalVector>::dim+1;
    static int const cols=vec_traits<OriginalVector>::dim+1;

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
        return qvm_detail::read_translation_matat<qvm_detail::translation_mat_<OriginalVector>,Row,Col>::f(x);
        }

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_matrix & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        BOOST_QVM_STATIC_ASSERT(Col==cols-1);
        BOOST_QVM_STATIC_ASSERT(Col!=Row);
        return vec_traits<OriginalVector>::template write_element<Row>(reinterpret_cast<OriginalVector &>(x));
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
        return
            row==col?
                scalar_traits<scalar_type>::value(1):
                (col==cols-1?
                    vec_traits<OriginalVector>::read_element_idx(row,reinterpret_cast<OriginalVector const &>(x)):
                    scalar_traits<scalar_type>::value(0));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        BOOST_QVM_ASSERT(col==cols-1);
        BOOST_QVM_ASSERT(col!=row);
        return vec_traits<OriginalVector>::write_element_idx(row,reinterpret_cast<OriginalVector &>(x));
        }
    };

template <class OriginalVector,int R,int C>
struct
deduce_mat<qvm_detail::translation_mat_<OriginalVector>,R,C>
    {
    typedef mat<typename vec_traits<OriginalVector>::scalar_type,R,C> type;
    };

template <class OriginalVector,int R,int C>
struct
deduce_mat2<qvm_detail::translation_mat_<OriginalVector>,qvm_detail::translation_mat_<OriginalVector>,R,C>
    {
    typedef mat<typename vec_traits<OriginalVector>::scalar_type,R,C> type;
    };

template <class A>
typename enable_if_c<
    is_vec<A>::value,
    qvm_detail::translation_mat_<A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
translation_mat( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::translation_mat_<A> const &>(a);
    }

template <class A>
typename enable_if_c<
    is_vec<A>::value,
    qvm_detail::translation_mat_<A> &>::type
BOOST_QVM_INLINE_TRIVIAL
translation_mat( A & a )
    {
    return reinterpret_cast<typename qvm_detail::translation_mat_<A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <class OriginalVector>
    class
    diag_mat_
        {
        diag_mat_( diag_mat_ const & );
        diag_mat_ & operator=( diag_mat_ const & );
        ~diag_mat_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        diag_mat_ &
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

template <class OriginalVector>
struct
mat_traits< qvm_detail::diag_mat_<OriginalVector> >
    {
    typedef qvm_detail::diag_mat_<OriginalVector> this_matrix;
    typedef typename vec_traits<OriginalVector>::scalar_type scalar_type;
    static int const rows=vec_traits<OriginalVector>::dim;
    static int const cols=vec_traits<OriginalVector>::dim;

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
        return Row==Col?vec_traits<OriginalVector>::template read_element<Row>(reinterpret_cast<OriginalVector const &>(x)):scalar_traits<scalar_type>::value(0);
        }

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_matrix & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        BOOST_QVM_STATIC_ASSERT(Row==Col);
        return vec_traits<OriginalVector>::template write_element<Row>(reinterpret_cast<OriginalVector &>(x));
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
        return row==col?vec_traits<OriginalVector>::read_element_idx(row,reinterpret_cast<OriginalVector const &>(x)):scalar_traits<scalar_type>::value(0);
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int row, int col, this_matrix & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        BOOST_QVM_ASSERT(row==col);
        return vec_traits<OriginalVector>::write_element_idx(row,reinterpret_cast<OriginalVector &>(x));
        }
    };

template <class OriginalVector,int R,int C>
struct
deduce_mat<qvm_detail::diag_mat_<OriginalVector>,R,C>
    {
    typedef mat<typename vec_traits<OriginalVector>::scalar_type,R,C> type;
    };

template <class OriginalVector,int R,int C>
struct
deduce_mat2<qvm_detail::diag_mat_<OriginalVector>,qvm_detail::diag_mat_<OriginalVector>,R,C>
    {
    typedef mat<typename vec_traits<OriginalVector>::scalar_type,R,C> type;
    };

template <class A>
typename enable_if_c<
    is_vec<A>::value,
    qvm_detail::diag_mat_<A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
diag_mat( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::diag_mat_<A> const &>(a);
    }

template <class A>
typename enable_if_c<
    is_vec<A>::value,
    qvm_detail::diag_mat_<A> &>::type
BOOST_QVM_INLINE_TRIVIAL
diag_mat( A & a )
    {
    return reinterpret_cast<typename qvm_detail::diag_mat_<A> &>(a);
    }

} }

#endif
