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

#ifndef _BOOST_UBLAS_OPERATION_BLOCKED_
#define _BOOST_UBLAS_OPERATION_BLOCKED_

#include <boost/numeric/ublas/traits.hpp>
#include <boost/numeric/ublas/detail/vector_assign.hpp> // indexing_vector_assign
#include <boost/numeric/ublas/detail/matrix_assign.hpp> // indexing_matrix_assign


namespace boost { namespace numeric { namespace ublas {

    template<class V, typename V::size_type BS, class E1, class E2>
    BOOST_UBLAS_INLINE
    V
    block_prod (const matrix_expression<E1> &e1,
                const vector_expression<E2> &e2) {
        typedef V vector_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename V::size_type size_type;
        typedef typename V::value_type value_type;
        const size_type block_size = BS;

        V v (e1 ().size1 ());
#if BOOST_UBLAS_TYPE_CHECK
        vector<value_type> cv (v.size ());
        typedef typename type_traits<value_type>::real_type real_type;
        real_type verrorbound (norm_1 (v) + norm_1 (e1) * norm_1 (e2));
        indexing_vector_assign<scalar_assign> (cv, prod (e1, e2));
#endif
        size_type i_size = e1 ().size1 ();
        size_type j_size = BOOST_UBLAS_SAME (e1 ().size2 (), e2 ().size ());
        for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
            size_type i_end = i_begin + (std::min) (i_size - i_begin, block_size);
            // FIX: never ignore Martin Weiser's advice ;-(
#ifdef BOOST_UBLAS_NO_CACHE
            vector_range<vector_type> v_range (v, range (i_begin, i_end));
#else
            // vector<value_type, bounded_array<value_type, block_size> > v_range (i_end - i_begin);
            vector<value_type> v_range (i_end - i_begin);
#endif
            v_range.assign (zero_vector<value_type> (i_end - i_begin));
            for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
                size_type j_end = j_begin + (std::min) (j_size - j_begin, block_size);
#ifdef BOOST_UBLAS_NO_CACHE
                const matrix_range<expression1_type> e1_range (e1 (), range (i_begin, i_end), range (j_begin, j_end));
                const vector_range<expression2_type> e2_range (e2 (), range (j_begin, j_end));
                v_range.plus_assign (prod (e1_range, e2_range));
#else
                // const matrix<value_type, row_major, bounded_array<value_type, block_size * block_size> > e1_range (project (e1 (), range (i_begin, i_end), range (j_begin, j_end)));
                // const vector<value_type, bounded_array<value_type, block_size> > e2_range (project (e2 (), range (j_begin, j_end)));
                const matrix<value_type, row_major> e1_range (project (e1 (), range (i_begin, i_end), range (j_begin, j_end)));
                const vector<value_type> e2_range (project (e2 (), range (j_begin, j_end)));
                v_range.plus_assign (prod (e1_range, e2_range));
#endif
            }
#ifndef BOOST_UBLAS_NO_CACHE
            project (v, range (i_begin, i_end)).assign (v_range);
#endif
        }
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (norm_1 (v - cv) <= 2 * std::numeric_limits<real_type>::epsilon () * verrorbound, internal_logic ());
#endif
        return v;
    }

    template<class V, typename V::size_type BS, class E1, class E2>
    BOOST_UBLAS_INLINE
    V
    block_prod (const vector_expression<E1> &e1,
                const matrix_expression<E2> &e2) {
        typedef V vector_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename V::size_type size_type;
        typedef typename V::value_type value_type;
        const size_type block_size = BS;

        V v (e2 ().size2 ());
#if BOOST_UBLAS_TYPE_CHECK
        vector<value_type> cv (v.size ());
        typedef typename type_traits<value_type>::real_type real_type;
        real_type verrorbound (norm_1 (v) + norm_1 (e1) * norm_1 (e2));
        indexing_vector_assign<scalar_assign> (cv, prod (e1, e2));
#endif
        size_type i_size = BOOST_UBLAS_SAME (e1 ().size (), e2 ().size1 ());
        size_type j_size = e2 ().size2 ();
        for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
            size_type j_end = j_begin + (std::min) (j_size - j_begin, block_size);
            // FIX: never ignore Martin Weiser's advice ;-(
#ifdef BOOST_UBLAS_NO_CACHE
            vector_range<vector_type> v_range (v, range (j_begin, j_end));
#else
            // vector<value_type, bounded_array<value_type, block_size> > v_range (j_end - j_begin);
            vector<value_type> v_range (j_end - j_begin);
#endif
            v_range.assign (zero_vector<value_type> (j_end - j_begin));
            for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
                size_type i_end = i_begin + (std::min) (i_size - i_begin, block_size);
#ifdef BOOST_UBLAS_NO_CACHE
                const vector_range<expression1_type> e1_range (e1 (), range (i_begin, i_end));
                const matrix_range<expression2_type> e2_range (e2 (), range (i_begin, i_end), range (j_begin, j_end));
#else
                // const vector<value_type, bounded_array<value_type, block_size> > e1_range (project (e1 (), range (i_begin, i_end)));
                // const matrix<value_type, column_major, bounded_array<value_type, block_size * block_size> > e2_range (project (e2 (), range (i_begin, i_end), range (j_begin, j_end)));
                const vector<value_type> e1_range (project (e1 (), range (i_begin, i_end)));
                const matrix<value_type, column_major> e2_range (project (e2 (), range (i_begin, i_end), range (j_begin, j_end)));
#endif
                v_range.plus_assign (prod (e1_range, e2_range));
            }
#ifndef BOOST_UBLAS_NO_CACHE
            project (v, range (j_begin, j_end)).assign (v_range);
#endif
        }
#if BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (norm_1 (v - cv) <= 2 * std::numeric_limits<real_type>::epsilon () * verrorbound, internal_logic ());
#endif
        return v;
    }

    template<class M, typename M::size_type BS, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    block_prod (const matrix_expression<E1> &e1,
                const matrix_expression<E2> &e2,
                row_major_tag) {
        typedef M matrix_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;
        const size_type block_size = BS;

        M m (e1 ().size1 (), e2 ().size2 ());
#if BOOST_UBLAS_TYPE_CHECK
        matrix<value_type, row_major> cm (m.size1 (), m.size2 ());
        typedef typename type_traits<value_type>::real_type real_type;
        real_type merrorbound (norm_1 (m) + norm_1 (e1) * norm_1 (e2));
        indexing_matrix_assign<scalar_assign> (cm, prod (e1, e2), row_major_tag ());
        disable_type_check<bool>::value = true;
#endif
        size_type i_size = e1 ().size1 ();
        size_type j_size = e2 ().size2 ();
        size_type k_size = BOOST_UBLAS_SAME (e1 ().size2 (), e2 ().size1 ());
        for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
            size_type i_end = i_begin + (std::min) (i_size - i_begin, block_size);
            for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
                size_type j_end = j_begin + (std::min) (j_size - j_begin, block_size);
                // FIX: never ignore Martin Weiser's advice ;-(
#ifdef BOOST_UBLAS_NO_CACHE
                matrix_range<matrix_type> m_range (m, range (i_begin, i_end), range (j_begin, j_end));
#else
                // matrix<value_type, row_major, bounded_array<value_type, block_size * block_size> > m_range (i_end - i_begin, j_end - j_begin);
                matrix<value_type, row_major> m_range (i_end - i_begin, j_end - j_begin);
#endif
                m_range.assign (zero_matrix<value_type> (i_end - i_begin, j_end - j_begin));
                for (size_type k_begin = 0; k_begin < k_size; k_begin += block_size) {
                    size_type k_end = k_begin + (std::min) (k_size - k_begin, block_size);
#ifdef BOOST_UBLAS_NO_CACHE
                    const matrix_range<expression1_type> e1_range (e1 (), range (i_begin, i_end), range (k_begin, k_end));
                    const matrix_range<expression2_type> e2_range (e2 (), range (k_begin, k_end), range (j_begin, j_end));
#else
                    // const matrix<value_type, row_major, bounded_array<value_type, block_size * block_size> > e1_range (project (e1 (), range (i_begin, i_end), range (k_begin, k_end)));
                    // const matrix<value_type, column_major, bounded_array<value_type, block_size * block_size> > e2_range (project (e2 (), range (k_begin, k_end), range (j_begin, j_end)));
                    const matrix<value_type, row_major> e1_range (project (e1 (), range (i_begin, i_end), range (k_begin, k_end)));
                    const matrix<value_type, column_major> e2_range (project (e2 (), range (k_begin, k_end), range (j_begin, j_end)));
#endif
                    m_range.plus_assign (prod (e1_range, e2_range));
                }
#ifndef BOOST_UBLAS_NO_CACHE
                project (m, range (i_begin, i_end), range (j_begin, j_end)).assign (m_range);
#endif
            }
        }
#if BOOST_UBLAS_TYPE_CHECK
        disable_type_check<bool>::value = false;
        BOOST_UBLAS_CHECK (norm_1 (m - cm) <= 2 * std::numeric_limits<real_type>::epsilon () * merrorbound, internal_logic ());
#endif
        return m;
    }

    template<class M, typename M::size_type BS, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    block_prod (const matrix_expression<E1> &e1,
                const matrix_expression<E2> &e2,
                column_major_tag) {
        typedef M matrix_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;
        const size_type block_size = BS;

        M m (e1 ().size1 (), e2 ().size2 ());
#if BOOST_UBLAS_TYPE_CHECK
        matrix<value_type, column_major> cm (m.size1 (), m.size2 ());
        typedef typename type_traits<value_type>::real_type real_type;
        real_type merrorbound (norm_1 (m) + norm_1 (e1) * norm_1 (e2));
        indexing_matrix_assign<scalar_assign> (cm, prod (e1, e2), column_major_tag ());
        disable_type_check<bool>::value = true;
#endif
        size_type i_size = e1 ().size1 ();
        size_type j_size = e2 ().size2 ();
        size_type k_size = BOOST_UBLAS_SAME (e1 ().size2 (), e2 ().size1 ());
        for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
            size_type j_end = j_begin + (std::min) (j_size - j_begin, block_size);
            for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
                size_type i_end = i_begin + (std::min) (i_size - i_begin, block_size);
                // FIX: never ignore Martin Weiser's advice ;-(
#ifdef BOOST_UBLAS_NO_CACHE
                matrix_range<matrix_type> m_range (m, range (i_begin, i_end), range (j_begin, j_end));
#else
                // matrix<value_type, column_major, bounded_array<value_type, block_size * block_size> > m_range (i_end - i_begin, j_end - j_begin);
                matrix<value_type, column_major> m_range (i_end - i_begin, j_end - j_begin);
#endif
                m_range.assign (zero_matrix<value_type> (i_end - i_begin, j_end - j_begin));
                for (size_type k_begin = 0; k_begin < k_size; k_begin += block_size) {
                    size_type k_end = k_begin + (std::min) (k_size - k_begin, block_size);
#ifdef BOOST_UBLAS_NO_CACHE
                    const matrix_range<expression1_type> e1_range (e1 (), range (i_begin, i_end), range (k_begin, k_end));
                    const matrix_range<expression2_type> e2_range (e2 (), range (k_begin, k_end), range (j_begin, j_end));
#else
                    // const matrix<value_type, row_major, bounded_array<value_type, block_size * block_size> > e1_range (project (e1 (), range (i_begin, i_end), range (k_begin, k_end)));
                    // const matrix<value_type, column_major, bounded_array<value_type, block_size * block_size> > e2_range (project (e2 (), range (k_begin, k_end), range (j_begin, j_end)));
                    const matrix<value_type, row_major> e1_range (project (e1 (), range (i_begin, i_end), range (k_begin, k_end)));
                    const matrix<value_type, column_major> e2_range (project (e2 (), range (k_begin, k_end), range (j_begin, j_end)));
#endif
                    m_range.plus_assign (prod (e1_range, e2_range));
                }
#ifndef BOOST_UBLAS_NO_CACHE
                project (m, range (i_begin, i_end), range (j_begin, j_end)).assign (m_range);
#endif
            }
        }
#if BOOST_UBLAS_TYPE_CHECK
        disable_type_check<bool>::value = false;
        BOOST_UBLAS_CHECK (norm_1 (m - cm) <= 2 * std::numeric_limits<real_type>::epsilon () * merrorbound, internal_logic ());
#endif
        return m;
    }

    // Dispatcher
    template<class M, typename M::size_type BS, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    block_prod (const matrix_expression<E1> &e1,
                const matrix_expression<E2> &e2) {
        typedef typename M::orientation_category orientation_category;
        return block_prod<M, BS> (e1, e2, orientation_category ());
    }

}}}

#endif
