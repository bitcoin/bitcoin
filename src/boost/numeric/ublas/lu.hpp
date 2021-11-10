//
//  Copyright (c) 2000-2002
//  Joerg Walter, Mathias Koch
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#ifndef _BOOST_UBLAS_LU_
#define _BOOST_UBLAS_LU_

#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/triangular.hpp>

// LU factorizations in the spirit of LAPACK and Golub & van Loan

namespace boost { namespace numeric { namespace ublas {

    /** \brief
     *
     * \tparam T
     * \tparam A
     */
    template<class T = std::size_t, class A = unbounded_array<T> >
    class permutation_matrix:
        public vector<T, A> {
    public:
        typedef vector<T, A> vector_type;
        typedef typename vector_type::size_type size_type;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        explicit
        permutation_matrix (size_type size):
            vector<T, A> (size) {
            for (size_type i = 0; i < size; ++ i)
                (*this) (i) = i;
        }
        BOOST_UBLAS_INLINE
        explicit
        permutation_matrix (const vector_type & init) 
            : vector_type(init)
        { }
        BOOST_UBLAS_INLINE
        ~permutation_matrix () {}

        // Assignment
        BOOST_UBLAS_INLINE
        permutation_matrix &operator = (const permutation_matrix &m) {
            vector_type::operator = (m);
            return *this;
        }
    };

    template<class PM, class MV>
    BOOST_UBLAS_INLINE
    void swap_rows (const PM &pm, MV &mv, vector_tag) {
        typedef typename PM::size_type size_type;

        size_type size = pm.size ();
        for (size_type i = 0; i < size; ++ i) {
            if (i != pm (i))
                std::swap (mv (i), mv (pm (i)));
        }
    }
    template<class PM, class MV>
    BOOST_UBLAS_INLINE
    void swap_rows (const PM &pm, MV &mv, matrix_tag) {
        typedef typename PM::size_type size_type;

        size_type size = pm.size ();
        for (size_type i = 0; i < size; ++ i) {
            if (i != pm (i))
                row (mv, i).swap (row (mv, pm (i)));
        }
    }
    // Dispatcher
    template<class PM, class MV>
    BOOST_UBLAS_INLINE
    void swap_rows (const PM &pm, MV &mv) {
        swap_rows (pm, mv, typename MV::type_category ());
    }

    // LU factorization without pivoting
    template<class M>
    typename M::size_type lu_factorize (M &m) {

        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

#if BOOST_UBLAS_TYPE_CHECK
        typedef M matrix_type;
        matrix_type cm (m);
#endif
        size_type singular = 0;
        size_type size1 = m.size1 ();
        size_type size2 = m.size2 ();
        size_type size = (std::min) (size1, size2);
        for (size_type i = 0; i < size; ++ i) {
            matrix_column<M> mci (column (m, i));
            matrix_row<M> mri (row (m, i));
            if (m (i, i) != value_type/*zero*/()) {
                value_type m_inv = value_type (1) / m (i, i);
                project (mci, range (i + 1, size1)) *= m_inv;
            } else if (singular == 0) {
                singular = i + 1;
            }
            project (m, range (i + 1, size1), range (i + 1, size2)).minus_assign (
                outer_prod (project (mci, range (i + 1, size1)),
                            project (mri, range (i + 1, size2))));
        }
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (singular != 0 ||
                           detail::expression_type_check (prod (triangular_adaptor<matrix_type, unit_lower> (m),
                                                                triangular_adaptor<matrix_type, upper> (m)), 
                                                          cm), internal_logic ());
#endif
        return singular;
    }

    // LU factorization with partial pivoting
    template<class M, class PM>
    typename M::size_type lu_factorize (M &m, PM &pm) {
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

#if BOOST_UBLAS_TYPE_CHECK
        typedef M matrix_type;
        matrix_type cm (m);
#endif
        size_type singular = 0;
        size_type size1 = m.size1 ();
        size_type size2 = m.size2 ();
        size_type size = (std::min) (size1, size2);
        for (size_type i = 0; i < size; ++ i) {
            matrix_column<M> mci (column (m, i));
            matrix_row<M> mri (row (m, i));
            size_type i_norm_inf = i + index_norm_inf (project (mci, range (i, size1)));
            BOOST_UBLAS_CHECK (i_norm_inf < size1, external_logic ());
            if (m (i_norm_inf, i) != value_type/*zero*/()) {
                if (i_norm_inf != i) {
                    pm (i) = i_norm_inf;
                    row (m, i_norm_inf).swap (mri);
                } else {
                    BOOST_UBLAS_CHECK (pm (i) == i_norm_inf, external_logic ());
                }
                value_type m_inv = value_type (1) / m (i, i);
                project (mci, range (i + 1, size1)) *= m_inv;
            } else if (singular == 0) {
                singular = i + 1;
            }
            project (m, range (i + 1, size1), range (i + 1, size2)).minus_assign (
                outer_prod (project (mci, range (i + 1, size1)),
                            project (mri, range (i + 1, size2))));
        }
#if BOOST_UBLAS_TYPE_CHECK
        swap_rows (pm, cm);
        BOOST_UBLAS_CHECK (singular != 0 ||
                           detail::expression_type_check (prod (triangular_adaptor<matrix_type, unit_lower> (m),
                                                                triangular_adaptor<matrix_type, upper> (m)), cm), internal_logic ());
#endif
        return singular;
    }

    template<class M, class PM>
    typename M::size_type axpy_lu_factorize (M &m, PM &pm) {
        typedef M matrix_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;
        typedef vector<value_type> vector_type;

#if BOOST_UBLAS_TYPE_CHECK
        matrix_type cm (m);
#endif
        size_type singular = 0;
        size_type size1 = m.size1 ();
        size_type size2 = m.size2 ();
        size_type size = (std::min) (size1, size2);
#ifndef BOOST_UBLAS_LU_WITH_INPLACE_SOLVE
        matrix_type mr (m);
        mr.assign (zero_matrix<value_type> (size1, size2));
        vector_type v (size1);
        for (size_type i = 0; i < size; ++ i) {
            matrix_range<matrix_type> lrr (project (mr, range (0, i), range (0, i)));
            vector_range<matrix_column<matrix_type> > urr (project (column (mr, i), range (0, i)));
            urr.assign (solve (lrr, project (column (m, i), range (0, i)), unit_lower_tag ()));
            project (v, range (i, size1)).assign (
                project (column (m, i), range (i, size1)) -
                axpy_prod<vector_type> (project (mr, range (i, size1), range (0, i)), urr));
            size_type i_norm_inf = i + index_norm_inf (project (v, range (i, size1)));
            BOOST_UBLAS_CHECK (i_norm_inf < size1, external_logic ());
            if (v (i_norm_inf) != value_type/*zero*/()) {
                if (i_norm_inf != i) {
                    pm (i) = i_norm_inf;
                    std::swap (v (i_norm_inf), v (i));
                    project (row (m, i_norm_inf), range (i + 1, size2)).swap (project (row (m, i), range (i + 1, size2)));
                } else {
                    BOOST_UBLAS_CHECK (pm (i) == i_norm_inf, external_logic ());
                }
                project (column (mr, i), range (i + 1, size1)).assign (
                    project (v, range (i + 1, size1)) / v (i));
                if (i_norm_inf != i) {
                    project (row (mr, i_norm_inf), range (0, i)).swap (project (row (mr, i), range (0, i)));
                }
            } else if (singular == 0) {
                singular = i + 1;
            }
            mr (i, i) = v (i);
        }
        m.assign (mr);
#else
        matrix_type lr (m);
        matrix_type ur (m);
        lr.assign (identity_matrix<value_type> (size1, size2));
        ur.assign (zero_matrix<value_type> (size1, size2));
        vector_type v (size1);
        for (size_type i = 0; i < size; ++ i) {
            matrix_range<matrix_type> lrr (project (lr, range (0, i), range (0, i)));
            vector_range<matrix_column<matrix_type> > urr (project (column (ur, i), range (0, i)));
            urr.assign (project (column (m, i), range (0, i)));
            inplace_solve (lrr, urr, unit_lower_tag ());
            project (v, range (i, size1)).assign (
                project (column (m, i), range (i, size1)) -
                axpy_prod<vector_type> (project (lr, range (i, size1), range (0, i)), urr));
            size_type i_norm_inf = i + index_norm_inf (project (v, range (i, size1)));
            BOOST_UBLAS_CHECK (i_norm_inf < size1, external_logic ());
            if (v (i_norm_inf) != value_type/*zero*/()) {
                if (i_norm_inf != i) {
                    pm (i) = i_norm_inf;
                    std::swap (v (i_norm_inf), v (i));
                    project (row (m, i_norm_inf), range (i + 1, size2)).swap (project (row (m, i), range (i + 1, size2)));
                } else {
                    BOOST_UBLAS_CHECK (pm (i) == i_norm_inf, external_logic ());
                }
                project (column (lr, i), range (i + 1, size1)).assign (
                    project (v, range (i + 1, size1)) / v (i));
                if (i_norm_inf != i) {
                    project (row (lr, i_norm_inf), range (0, i)).swap (project (row (lr, i), range (0, i)));
                }
            } else if (singular == 0) {
                singular = i + 1;
            }
            ur (i, i) = v (i);
        }
        m.assign (triangular_adaptor<matrix_type, strict_lower> (lr) +
                  triangular_adaptor<matrix_type, upper> (ur));
#endif
#if BOOST_UBLAS_TYPE_CHECK
        swap_rows (pm, cm);
        BOOST_UBLAS_CHECK (singular != 0 ||
                           detail::expression_type_check (prod (triangular_adaptor<matrix_type, unit_lower> (m),
                                                                triangular_adaptor<matrix_type, upper> (m)), cm), internal_logic ());
#endif
        return singular;
    }

    // LU substitution
    template<class M, class E>
    void lu_substitute (const M &m, vector_expression<E> &e) {
#if BOOST_UBLAS_TYPE_CHECK
        typedef const M const_matrix_type;
        typedef vector<typename E::value_type> vector_type;

        vector_type cv1 (e);
#endif
        inplace_solve (m, e, unit_lower_tag ());
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (detail::expression_type_check (prod (triangular_adaptor<const_matrix_type, unit_lower> (m), e), cv1), internal_logic ());
        vector_type cv2 (e);
#endif
        inplace_solve (m, e, upper_tag ());
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (detail::expression_type_check (prod (triangular_adaptor<const_matrix_type, upper> (m), e), cv2), internal_logic ());
#endif
    }
    template<class M, class E>
    void lu_substitute (const M &m, matrix_expression<E> &e) {
#if BOOST_UBLAS_TYPE_CHECK
        typedef const M const_matrix_type;
        typedef matrix<typename E::value_type> matrix_type;

        matrix_type cm1 (e);
#endif
        inplace_solve (m, e, unit_lower_tag ());
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (detail::expression_type_check (prod (triangular_adaptor<const_matrix_type, unit_lower> (m), e), cm1), internal_logic ());
        matrix_type cm2 (e);
#endif
        inplace_solve (m, e, upper_tag ());
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (detail::expression_type_check (prod (triangular_adaptor<const_matrix_type, upper> (m), e), cm2), internal_logic ());
#endif
    }
    template<class M, class PMT, class PMA, class MV>
    void lu_substitute (const M &m, const permutation_matrix<PMT, PMA> &pm, MV &mv) {
        swap_rows (pm, mv);
        lu_substitute (m, mv);
    }
    template<class E, class M>
    void lu_substitute (vector_expression<E> &e, const M &m) {
#if BOOST_UBLAS_TYPE_CHECK
        typedef const M const_matrix_type;
        typedef vector<typename E::value_type> vector_type;

        vector_type cv1 (e);
#endif
        inplace_solve (e, m, upper_tag ());
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (detail::expression_type_check (prod (e, triangular_adaptor<const_matrix_type, upper> (m)), cv1), internal_logic ());
        vector_type cv2 (e);
#endif
        inplace_solve (e, m, unit_lower_tag ());
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (detail::expression_type_check (prod (e, triangular_adaptor<const_matrix_type, unit_lower> (m)), cv2), internal_logic ());
#endif
    }
    template<class E, class M>
    void lu_substitute (matrix_expression<E> &e, const M &m) {
#if BOOST_UBLAS_TYPE_CHECK
        typedef const M const_matrix_type;
        typedef matrix<typename E::value_type> matrix_type;

        matrix_type cm1 (e);
#endif
        inplace_solve (e, m, upper_tag ());
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (detail::expression_type_check (prod (e, triangular_adaptor<const_matrix_type, upper> (m)), cm1), internal_logic ());
        matrix_type cm2 (e);
#endif
        inplace_solve (e, m, unit_lower_tag ());
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (detail::expression_type_check (prod (e, triangular_adaptor<const_matrix_type, unit_lower> (m)), cm2), internal_logic ());
#endif
    }
    template<class MV, class M, class PMT, class PMA>
    void lu_substitute (MV &mv, const M &m, const permutation_matrix<PMT, PMA> &pm) {
        swap_rows (pm, mv);
        lu_substitute (mv, m);
    }

}}}

#endif
