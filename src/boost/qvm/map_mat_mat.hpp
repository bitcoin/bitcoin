#ifndef BOOST_QVM_MAP_MAT_MAT_HPP_INCLUDED
#define BOOST_QVM_MAP_MAT_MAT_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/deduce_mat.hpp>
#include <boost/qvm/assert.hpp>
#include <boost/qvm/enable_if.hpp>
#include <boost/qvm/detail/transp_impl.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    template <int Row,class OriginalMatrix>
    class
    del_row_
        {
        del_row_( del_row_ const & );
        del_row_ & operator=( del_row_ const & );
        ~del_row_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        del_row_ &
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

template <int I,class OriginalMatrix>
struct
mat_traits< qvm_detail::del_row_<I,OriginalMatrix> >
    {
    typedef qvm_detail::del_row_<I,OriginalMatrix> this_matrix;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const rows=mat_traits<OriginalMatrix>::rows-1;
    static int const cols=mat_traits<OriginalMatrix>::cols;

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
        return mat_traits<OriginalMatrix>::template read_element<Row+(Row>=I),Col>(reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::template write_element<Row+(Row>=I),Col>(reinterpret_cast<OriginalMatrix &>(x));
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
        return mat_traits<OriginalMatrix>::read_element_idx(row+(row>=I),col,reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::write_element_idx(row+(row>=I),col,reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <int J,class OriginalMatrix,int R,int C>
struct
deduce_mat<qvm_detail::del_row_<J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int J,class OriginalMatrix,int R,int C>
struct
deduce_mat2<qvm_detail::del_row_<J,OriginalMatrix>,qvm_detail::del_row_<J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int Row,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::del_row_<Row,A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
del_row( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::del_row_<Row,A> const &>(a);
    }

template <int Row,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::del_row_<Row,A> &>::type
BOOST_QVM_INLINE_TRIVIAL
del_row( A & a )
    {
    return reinterpret_cast<typename qvm_detail::del_row_<Row,A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Col,class OriginalMatrix>
    class
    del_col_
        {
        del_col_( del_col_ const & );
        del_col_ & operator=( del_col_ const & );
        ~del_col_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        del_col_ &
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

template <int J,class OriginalMatrix>
struct
mat_traits< qvm_detail::del_col_<J,OriginalMatrix> >
    {
    typedef qvm_detail::del_col_<J,OriginalMatrix> this_matrix;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const rows=mat_traits<OriginalMatrix>::rows;
    static int const cols=mat_traits<OriginalMatrix>::cols-1;

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
        return mat_traits<OriginalMatrix>::template read_element<Row,Col+(Col>=J)>(reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::template write_element<Row,Col+(Col>=J)>(reinterpret_cast<OriginalMatrix &>(x));
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
        return mat_traits<OriginalMatrix>::read_element_idx(row,col+(col>=J),reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::write_element_idx(row,col+(col>=J),reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <int J,class OriginalMatrix,int R,int C>
struct
deduce_mat<qvm_detail::del_col_<J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int J,class OriginalMatrix,int R,int C>
struct
deduce_mat2<qvm_detail::del_col_<J,OriginalMatrix>,qvm_detail::del_col_<J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int Col,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::del_col_<Col,A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
del_col( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::del_col_<Col,A> const &>(a);
    }

template <int Col,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::del_col_<Col,A> &>::type
BOOST_QVM_INLINE_TRIVIAL
del_col( A & a )
    {
    return reinterpret_cast<typename qvm_detail::del_col_<Col,A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Row,int Col,class OriginalMatrix>
    class
    del_row_col_
        {
        del_row_col_( del_row_col_ const & );
        ~del_row_col_();

        public:

        BOOST_QVM_INLINE_TRIVIAL
        del_row_col_ &
        operator=( del_row_col_ const & x )
            {
            assign(*this,x);
            return *this;
            }

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        del_row_col_ &
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

template <int I,int J,class OriginalMatrix>
struct
mat_traits< qvm_detail::del_row_col_<I,J,OriginalMatrix> >
    {
    typedef qvm_detail::del_row_col_<I,J,OriginalMatrix> this_matrix;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const rows=mat_traits<OriginalMatrix>::rows-1;
    static int const cols=mat_traits<OriginalMatrix>::cols-1;

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
        return mat_traits<OriginalMatrix>::template read_element<Row+(Row>=I),Col+(Col>=J)>(reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::template write_element<Row+(Row>=I),Col+(Col>=J)>(reinterpret_cast<OriginalMatrix &>(x));
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
        return mat_traits<OriginalMatrix>::read_element_idx(row+(row>=I),col+(col>=J),reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::write_element_idx(row+(row>=I),col+(col>=J),reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <int I,int J,class OriginalMatrix,int R,int C>
struct
deduce_mat<qvm_detail::del_row_col_<I,J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int I,int J,class OriginalMatrix,int R,int C>
struct
deduce_mat2<qvm_detail::del_row_col_<I,J,OriginalMatrix>,qvm_detail::del_row_col_<I,J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int Row,int Col,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::del_row_col_<Row,Col,A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
del_row_col( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::del_row_col_<Row,Col,A> const &>(a);
    }

template <int Row,int Col,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::del_row_col_<Row,Col,A> &>::type
BOOST_QVM_INLINE_TRIVIAL
del_row_col( A & a )
    {
    return reinterpret_cast<typename qvm_detail::del_row_col_<Row,Col,A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Row,class OriginalMatrix>
    class
    neg_row_
        {
        neg_row_( neg_row_ const & );
        neg_row_ & operator=( neg_row_ const & );
        ~neg_row_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        neg_row_ &
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

template <int I,class OriginalMatrix>
struct
mat_traits< qvm_detail::neg_row_<I,OriginalMatrix> >
    {
    typedef qvm_detail::neg_row_<I,OriginalMatrix> this_matrix;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const rows=mat_traits<OriginalMatrix>::rows;
    static int const cols=mat_traits<OriginalMatrix>::cols;

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
        return Row==I ?
            -mat_traits<OriginalMatrix>::template read_element<Row,Col>(reinterpret_cast<OriginalMatrix const &>(x)) :
            mat_traits<OriginalMatrix>::template read_element<Row,Col>(reinterpret_cast<OriginalMatrix const &>(x));
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
        return row==I?
            -mat_traits<OriginalMatrix>::read_element_idx(row,col,reinterpret_cast<OriginalMatrix const &>(x)) :
            mat_traits<OriginalMatrix>::read_element_idx(row,col,reinterpret_cast<OriginalMatrix const &>(x));
        }
    };

template <int J,class OriginalMatrix,int R,int C>
struct
deduce_mat<qvm_detail::neg_row_<J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int J,class OriginalMatrix,int R,int C>
struct
deduce_mat2<qvm_detail::neg_row_<J,OriginalMatrix>,qvm_detail::neg_row_<J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int Row,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::neg_row_<Row,A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
neg_row( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::neg_row_<Row,A> const &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Col,class OriginalMatrix>
    class
    neg_col_
        {
        neg_col_( neg_col_ const & );
        neg_col_ & operator=( neg_col_ const & );
        ~neg_col_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        neg_col_ &
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

template <int J,class OriginalMatrix>
struct
mat_traits< qvm_detail::neg_col_<J,OriginalMatrix> >
    {
    typedef qvm_detail::neg_col_<J,OriginalMatrix> this_matrix;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const rows=mat_traits<OriginalMatrix>::rows;
    static int const cols=mat_traits<OriginalMatrix>::cols;

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
        return Col==J?
            -mat_traits<OriginalMatrix>::template read_element<Row,Col>(reinterpret_cast<OriginalMatrix const &>(x)) :
            mat_traits<OriginalMatrix>::template read_element<Row,Col>(reinterpret_cast<OriginalMatrix const &>(x));
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
        return col==J?
            -mat_traits<OriginalMatrix>::read_element_idx(row,col,reinterpret_cast<OriginalMatrix const &>(x)) :
            mat_traits<OriginalMatrix>::read_element_idx(row,col,reinterpret_cast<OriginalMatrix const &>(x));
        }
    };

template <int J,class OriginalMatrix,int R,int C>
struct
deduce_mat<qvm_detail::neg_col_<J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int J,class OriginalMatrix,int R,int C>
struct
deduce_mat2<qvm_detail::neg_col_<J,OriginalMatrix>,qvm_detail::neg_col_<J,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int Col,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::neg_col_<Col,A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
neg_col( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::neg_col_<Col,A> const &>(a);
    }

////////////////////////////////////////////////

template <class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::transposed_<A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
transposed( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::transposed_<A> const &>(a);
    }

template <class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::transposed_<A> &>::type
BOOST_QVM_INLINE_TRIVIAL
transposed( A & a )
    {
    return reinterpret_cast<typename qvm_detail::transposed_<A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Row1,int Row2,class OriginalMatrix>
    class
    swap_rows_
        {
        swap_rows_( swap_rows_ const & );
        swap_rows_ & operator=( swap_rows_ const & );
        ~swap_rows_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        swap_rows_ &
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

template <int R1,int R2,class OriginalMatrix>
struct
mat_traits< qvm_detail::swap_rows_<R1,R2,OriginalMatrix> >
    {
    typedef qvm_detail::swap_rows_<R1,R2,OriginalMatrix> this_matrix;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const rows=mat_traits<OriginalMatrix>::rows;
    static int const cols=mat_traits<OriginalMatrix>::cols;

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
        return mat_traits<OriginalMatrix>::template read_element<(Row==R1 && R1!=R2)*R2+(Row==R2 && R1!=R2)*R1+((Row!=R1 && Row!=R2) || R1==R2)*Row,Col>(reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::template write_element<(Row==R1 && R1!=R2)*R2+(Row==R2 && R1!=R2)*R1+((Row!=R1 && Row!=R2) || R1==R2)*Row,Col>(reinterpret_cast<OriginalMatrix &>(x));
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
        return mat_traits<OriginalMatrix>::read_element_idx(row==R1?R2:row==R2?R1:row,col,reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::write_element_idx(row==R1?R2:row==R2?R1:row,col,reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <int R1,int R2,class OriginalMatrix,int R,int C>
struct
deduce_mat<qvm_detail::swap_rows_<R1,R2,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int R1,int R2,class OriginalMatrix,int R,int C>
struct
deduce_mat2<qvm_detail::swap_rows_<R1,R2,OriginalMatrix>,qvm_detail::swap_rows_<R1,R2,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int R1,int R2,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::swap_rows_<R1,R2,A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
swap_rows( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::swap_rows_<R1,R2,A> const &>(a);
    }

template <int R1,int R2,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::swap_rows_<R1,R2,A> &>::type
BOOST_QVM_INLINE_TRIVIAL
swap_rows( A & a )
    {
    return reinterpret_cast<typename qvm_detail::swap_rows_<R1,R2,A> &>(a);
    }

////////////////////////////////////////////////

namespace
qvm_detail
    {
    template <int Row1,int Row2,class OriginalMatrix>
    class
    swap_cols_
        {
        swap_cols_( swap_cols_ const & );
        swap_cols_ & operator=( swap_cols_ const & );
        ~swap_cols_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        swap_cols_ &
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

template <int C1,int C2,class OriginalMatrix>
struct
mat_traits< qvm_detail::swap_cols_<C1,C2,OriginalMatrix> >
    {
    typedef qvm_detail::swap_cols_<C1,C2,OriginalMatrix> this_matrix;
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    static int const rows=mat_traits<OriginalMatrix>::rows;
    static int const cols=mat_traits<OriginalMatrix>::cols;

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
        return mat_traits<OriginalMatrix>::template read_element<Row,(Col==C1 && C1!=C2)*C2+(Col==C2 && C1!=C2)*C1+((Col!=C1 && Col!=C2) || C1==C2)*Col>(reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::template write_element<Row,(Col==C1 && C1!=C2)*C2+(Col==C2 && C1!=C2)*C1+((Col!=C1 && Col!=C2) || C1==C2)*Col>(reinterpret_cast<OriginalMatrix &>(x));
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
        return mat_traits<OriginalMatrix>::read_element_idx(row,col==C1?C2:col==C2?C1:col,reinterpret_cast<OriginalMatrix const &>(x));
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
        return mat_traits<OriginalMatrix>::write_element_idx(row,col==C1?C2:col==C2?C1:col,reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <int C1,int C2,class OriginalMatrix,int R,int C>
struct
deduce_mat<qvm_detail::swap_cols_<C1,C2,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int C1,int C2,class OriginalMatrix,int R,int C>
struct
deduce_mat2<qvm_detail::swap_cols_<C1,C2,OriginalMatrix>,qvm_detail::swap_cols_<C1,C2,OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <int C1,int C2,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::swap_cols_<C1,C2,A> const &>::type
BOOST_QVM_INLINE_TRIVIAL
swap_cols( A const & a )
    {
    return reinterpret_cast<typename qvm_detail::swap_cols_<C1,C2,A> const &>(a);
    }

template <int C1,int C2,class A>
typename enable_if_c<
    is_mat<A>::value,
    qvm_detail::swap_cols_<C1,C2,A> &>::type
BOOST_QVM_INLINE_TRIVIAL
swap_cols( A & a )
    {
    return reinterpret_cast<typename qvm_detail::swap_cols_<C1,C2,A> &>(a);
    }

} }

#endif
