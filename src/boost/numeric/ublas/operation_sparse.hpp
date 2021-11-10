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

#ifndef _BOOST_UBLAS_OPERATION_SPARSE_
#define _BOOST_UBLAS_OPERATION_SPARSE_

#include <boost/numeric/ublas/traits.hpp>

// These scaled additions were borrowed from MTL unashamedly.
// But Alexei Novakov had a lot of ideas to improve these. Thanks.

namespace boost { namespace numeric { namespace ublas {

    template<class M, class E1, class E2, class TRI>
    BOOST_UBLAS_INLINE
    M &
    sparse_prod (const matrix_expression<E1> &e1,
                 const matrix_expression<E2> &e2,
                 M &m, TRI,
                 row_major_tag) {
        typedef M matrix_type;
        typedef TRI triangular_restriction;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

        // ISSUE why is there a dense vector here?
        vector<value_type> temporary (e2 ().size2 ());
        temporary.clear ();
        typename expression1_type::const_iterator1 it1 (e1 ().begin1 ());
        typename expression1_type::const_iterator1 it1_end (e1 ().end1 ());
        while (it1 != it1_end) {
            size_type jb (temporary.size ());
            size_type je (0);
#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            typename expression1_type::const_iterator2 it2 (it1.begin ());
            typename expression1_type::const_iterator2 it2_end (it1.end ());
#else
            typename expression1_type::const_iterator2 it2 (boost::numeric::ublas::begin (it1, iterator1_tag ()));
            typename expression1_type::const_iterator2 it2_end (boost::numeric::ublas::end (it1, iterator1_tag ()));
#endif
            while (it2 != it2_end) {
                // temporary.plus_assign (*it2 * row (e2 (), it2.index2 ()));
                matrix_row<expression2_type> mr (e2 (), it2.index2 ());
                typename matrix_row<expression2_type>::const_iterator itr (mr.begin ());
                typename matrix_row<expression2_type>::const_iterator itr_end (mr.end ());
                while (itr != itr_end) {
                    size_type j (itr.index ());
                    temporary (j) += *it2 * *itr;
                    jb = (std::min) (jb, j);
                    je = (std::max) (je, j);
                    ++ itr;
                }
                ++ it2;
            }
            for (size_type j = jb; j < je + 1; ++ j) {
                if (temporary (j) != value_type/*zero*/()) {
                    // FIXME we'll need to extend the container interface!
                    // m.push_back (it1.index1 (), j, temporary (j));
                    // FIXME What to do with adaptors?
                    // m.insert (it1.index1 (), j, temporary (j));
                    if (triangular_restriction::other (it1.index1 (), j))
                        m (it1.index1 (), j) = temporary (j);
                    temporary (j) = value_type/*zero*/();
                }
            }
            ++ it1;
        }
        return m;
    }

    template<class M, class E1, class E2, class TRI>
    BOOST_UBLAS_INLINE
    M &
    sparse_prod (const matrix_expression<E1> &e1,
                 const matrix_expression<E2> &e2,
                 M &m, TRI,
                 column_major_tag) {
        typedef M matrix_type;
        typedef TRI triangular_restriction;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

        // ISSUE why is there a dense vector here?
        vector<value_type> temporary (e1 ().size1 ());
        temporary.clear ();
        typename expression2_type::const_iterator2 it2 (e2 ().begin2 ());
        typename expression2_type::const_iterator2 it2_end (e2 ().end2 ());
        while (it2 != it2_end) {
            size_type ib (temporary.size ());
            size_type ie (0);
#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            typename expression2_type::const_iterator1 it1 (it2.begin ());
            typename expression2_type::const_iterator1 it1_end (it2.end ());
#else
            typename expression2_type::const_iterator1 it1 (boost::numeric::ublas::begin (it2, iterator2_tag ()));
            typename expression2_type::const_iterator1 it1_end (boost::numeric::ublas::end (it2, iterator2_tag ()));
#endif
            while (it1 != it1_end) {
                // column (m, it2.index2 ()).plus_assign (*it1 * column (e1 (), it1.index1 ()));
                matrix_column<expression1_type> mc (e1 (), it1.index1 ());
                typename matrix_column<expression1_type>::const_iterator itc (mc.begin ());
                typename matrix_column<expression1_type>::const_iterator itc_end (mc.end ());
                while (itc != itc_end) {
                    size_type i (itc.index ());
                    temporary (i) += *it1 * *itc;
                    ib = (std::min) (ib, i);
                    ie = (std::max) (ie, i);
                    ++ itc;
                }
                ++ it1;
            }
            for (size_type i = ib; i < ie + 1; ++ i) {
                if (temporary (i) != value_type/*zero*/()) {
                    // FIXME we'll need to extend the container interface!
                    // m.push_back (i, it2.index2 (), temporary (i));
                    // FIXME What to do with adaptors?
                    // m.insert (i, it2.index2 (), temporary (i));
                    if (triangular_restriction::other (i, it2.index2 ()))
                        m (i, it2.index2 ()) = temporary (i);
                    temporary (i) = value_type/*zero*/();
                }
            }
            ++ it2;
        }
        return m;
    }

    // Dispatcher
    template<class M, class E1, class E2, class TRI>
    BOOST_UBLAS_INLINE
    M &
    sparse_prod (const matrix_expression<E1> &e1,
                 const matrix_expression<E2> &e2,
                 M &m, TRI, bool init = true) {
        typedef typename M::value_type value_type;
        typedef TRI triangular_restriction;
        typedef typename M::orientation_category orientation_category;

        if (init)
            m.assign (zero_matrix<value_type> (e1 ().size1 (), e2 ().size2 ()));
        return sparse_prod (e1, e2, m, triangular_restriction (), orientation_category ());
    }
    template<class M, class E1, class E2, class TRI>
    BOOST_UBLAS_INLINE
    M
    sparse_prod (const matrix_expression<E1> &e1,
                 const matrix_expression<E2> &e2,
                 TRI) {
        typedef M matrix_type;
        typedef TRI triangular_restriction;

        matrix_type m (e1 ().size1 (), e2 ().size2 ());
        // FIXME needed for c_matrix?!
        // return sparse_prod (e1, e2, m, triangular_restriction (), false);
        return sparse_prod (e1, e2, m, triangular_restriction (), true);
    }
    template<class M, class E1, class E2>
    BOOST_UBLAS_INLINE
    M &
    sparse_prod (const matrix_expression<E1> &e1,
                 const matrix_expression<E2> &e2,
                 M &m, bool init = true) {
        typedef typename M::value_type value_type;
        typedef typename M::orientation_category orientation_category;

        if (init)
            m.assign (zero_matrix<value_type> (e1 ().size1 (), e2 ().size2 ()));
        return sparse_prod (e1, e2, m, full (), orientation_category ());
    }
    template<class M, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    sparse_prod (const matrix_expression<E1> &e1,
                 const matrix_expression<E2> &e2) {
        typedef M matrix_type;

        matrix_type m (e1 ().size1 (), e2 ().size2 ());
        // FIXME needed for c_matrix?!
        // return sparse_prod (e1, e2, m, full (), false);
        return sparse_prod (e1, e2, m, full (), true);
    }

}}}

#endif
