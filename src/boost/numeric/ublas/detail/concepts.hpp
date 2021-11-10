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

#ifndef _BOOST_UBLAS_CONCEPTS_
#define _BOOST_UBLAS_CONCEPTS_

#include <boost/concept_check.hpp>

// Concept checks based on ideas of Jeremy Siek

namespace boost { namespace numeric { namespace ublas {


    template<class I>
    struct Indexed1DIteratorConcept {
        typedef I iterator_type;

        void constraints () {
            iterator_type it = iterator_type ();
            // Index
            it.index ();
        }
    };

    template<class I>
    struct IndexedBidirectional1DIteratorConcept {
        typedef I iterator_type;

        void constraints () {
            function_requires< BidirectionalIteratorConcept<iterator_type> >();
            function_requires< Indexed1DIteratorConcept<iterator_type> >();
        }
    };

    template<class I>
    struct Mutable_IndexedBidirectional1DIteratorConcept {
        typedef I iterator_type;

        void constraints () {
            function_requires< Mutable_BidirectionalIteratorConcept<iterator_type> >();
            function_requires< Indexed1DIteratorConcept<iterator_type> >();
        }
    };

    template<class I>
    struct IndexedRandomAccess1DIteratorConcept {
        typedef I iterator_type;

        void constraints () {
            function_requires< RandomAccessIteratorConcept<iterator_type> >();
            function_requires< Indexed1DIteratorConcept<iterator_type> >();
        }
    };

    template<class I>
    struct Mutable_IndexedRandomAccess1DIteratorConcept {
        typedef I iterator_type;

        void constraints () {
            function_requires< Mutable_RandomAccessIteratorConcept<iterator_type> >();
            function_requires< Indexed1DIteratorConcept<iterator_type> >();
        }
    };

    template<class I>
    struct Indexed2DIteratorConcept {
        typedef I iterator_type;
        typedef typename I::dual_iterator_type dual_iterator_type;
        typedef typename I::dual_reverse_iterator_type dual_reverse_iterator_type;

        void constraints () {
            iterator_type it = iterator_type ();
            // Indices
            it.index1 ();
            it.index2 ();
            // Iterator begin/end
            dual_iterator_type it_begin (it.begin ());
            dual_iterator_type it_end (it.end ());
            // Reverse iterator begin/end
            dual_reverse_iterator_type it_rbegin (it.rbegin ());
            dual_reverse_iterator_type it_rend (it.rend ());
            ignore_unused_variable_warning (it_begin);
            ignore_unused_variable_warning (it_end);
            ignore_unused_variable_warning (it_rbegin);
            ignore_unused_variable_warning (it_rend);
        }
    };

    template<class I1, class I2>
    struct IndexedBidirectional2DIteratorConcept {
        typedef I1 subiterator1_type;
        typedef I2 subiterator2_type;

        void constraints () {
            function_requires< BidirectionalIteratorConcept<subiterator1_type> >();
            function_requires< BidirectionalIteratorConcept<subiterator2_type> >();
            function_requires< Indexed2DIteratorConcept<subiterator1_type> >();
            function_requires< Indexed2DIteratorConcept<subiterator2_type> >();
        }
    };

    template<class I1, class I2>
    struct Mutable_IndexedBidirectional2DIteratorConcept {
        typedef I1 subiterator1_type;
        typedef I2 subiterator2_type;

        void constraints () {
            function_requires< Mutable_BidirectionalIteratorConcept<subiterator1_type> >();
            function_requires< Mutable_BidirectionalIteratorConcept<subiterator2_type> >();
            function_requires< Indexed2DIteratorConcept<subiterator1_type> >();
            function_requires< Indexed2DIteratorConcept<subiterator2_type> >();
        }
    };

    template<class I1, class I2>
    struct IndexedRandomAccess2DIteratorConcept {
        typedef I1 subiterator1_type;
        typedef I2 subiterator2_type;

        void constraints () {
            function_requires< RandomAccessIteratorConcept<subiterator1_type> >();
            function_requires< RandomAccessIteratorConcept<subiterator2_type> >();
            function_requires< Indexed2DIteratorConcept<subiterator1_type> >();
            function_requires< Indexed2DIteratorConcept<subiterator2_type> >();
        }
    };

    template<class I1, class I2>
    struct Mutable_IndexedRandomAccess2DIteratorConcept {
        typedef I1 subiterator1_type;
        typedef I2 subiterator2_type;

        void constraints () {
            function_requires< Mutable_RandomAccessIteratorConcept<subiterator1_type> >();
            function_requires< Mutable_RandomAccessIteratorConcept<subiterator2_type> >();
            function_requires< Indexed2DIteratorConcept<subiterator1_type> >();
            function_requires< Indexed2DIteratorConcept<subiterator2_type> >();
        }
    };

    template<class C>
    struct StorageArrayConcept {
        typedef C container_type;
        typedef typename C::size_type size_type;
        typedef typename C::value_type value_type;

        void constraints () {
            function_requires< RandomAccessContainerConcept<container_type> >();
            size_type n (0);
            // Sizing constructor
            container_type c = container_type (n);
            // Initialised sizing constructor
            container_type (n, value_type (5));
            ignore_unused_variable_warning (c);
        }
    };

    template<class C>
    struct Mutable_StorageArrayConcept {
        typedef C container_type;
        typedef typename C::size_type size_type;
        typedef typename C::value_type value_type;
        typedef typename C::iterator iterator_type;

        void constraints () {
            function_requires< Mutable_RandomAccessContainerConcept<container_type> > ();
            size_type n (0);
            // Sizing constructor
            container_type c = container_type (n);
            // Initialised sizing constructor
            c = container_type (n, value_type (3));
            // Resize
            c.resize (n, value_type (5));
            // Resize - none preserving
            c.resize (n);
        }
    };

    template<class C>
    struct StorageSparseConcept {
        typedef C container_type;
        typedef typename C::size_type size_type;

        void constraints () {
            function_requires< ReversibleContainerConcept<container_type> > ();
        }
    };

    template<class C>
    struct Mutable_StorageSparseConcept {
        typedef C container_type;
        typedef typename C::size_type size_type;
        typedef typename C::value_type value_type;
        typedef typename C::iterator iterator_type;

        void constraints () {
            // NOTE - Not Mutable_ReversibleContainerConcept
            function_requires< ReversibleContainerConcept<container_type> >();
            container_type c = container_type ();
            value_type t = value_type ();
            iterator_type it = iterator_type (), it1 = iterator_type (), it2 = iterator_type ();
            // Insert
            c.insert (it, t);
            // Erase
            c.erase (it);
            // Range erase
            c.erase (it1, it2);
            // Clear
            c.clear ();
        }
    };

    template<class G>
    struct IndexSetConcept {
        typedef G generator_type;
        typedef typename G::size_type size_type;
        typedef typename G::value_type value_type;

        void constraints () {
            function_requires< AssignableConcept<generator_type> >();
            function_requires< ReversibleContainerConcept<generator_type> >();
            generator_type g = generator_type ();
            size_type n (0);
            value_type t;
            // Element access
            t = g (n);
            ignore_unused_variable_warning (t);
        }
    };

    /** \brief Scalar expression concept.
     *
     * requirements
     * \li \c SE::value_type is the type of the scalar expression
     * \li \c SE must be convertable to \c SE::value_type
     * \li the constant \c SE::complexity must exist
     *
     * \param SE the type of the scalar expression
     */
    template<class SE>
    struct ScalarExpressionConcept {
        typedef SE scalar_expression_type;
        typedef typename SE::value_type value_type;

        static const unsigned complexity = SE::complexity;

        void constraints () {
            scalar_expression_type *sp;
            scalar_expression_type s = *sp;
            value_type t;
            // Conversion
            t = s;
            ignore_unused_variable_warning (t);
        }
    };

    /** \brief Vector expression concept.
     *
     * requirements
     * \li \c VE::value_type is the type of the elements
     * \li \c VE::const_reference The return type when accessing an element of a constant vector 
     * expression. Must be convertable to a \c value_type.
     * \li \c VE::size_type is the (unsigned) type of the indices
     * \li \c VE::difference_type is the (signed) type of distances between indices
     * \li \c VE::category
     * 
     * \li the constant \c SE::complexity must exist
     *
     * \param SE the type of the scalar expression
     */
    template<class VE>
    struct VectorExpressionConcept {
        typedef VE vector_expression_type;
        typedef typename VE::type_category type_category;
        typedef typename VE::size_type size_type;
        typedef typename VE::difference_type difference_type;
        typedef typename VE::value_type value_type;
        typedef typename VE::const_reference const_reference;
        typedef typename VE::const_iterator const_iterator_type;
        typedef typename VE::const_reverse_iterator const_reverse_iterator_type;

        void constraints () {
            vector_expression_type *vp;
            const vector_expression_type *cvp;
            vector_expression_type v = *vp;
            const vector_expression_type cv = *cvp;
            size_type n (0), i (0);
            value_type t;
            // Find (internal?)
            const_iterator_type cit (v.find (i));
            // Beginning of range
            const_iterator_type cit_begin (v.begin ());
            // End of range
            const_iterator_type cit_end (v.end ());
            // Size
            n = v.size ();
            // Beginning of reverse range
            const_reverse_iterator_type crit_begin (cv.rbegin ());
            // End of reverse range
            const_reverse_iterator_type crit_end (cv.rend ());
            // Element access
            t = v (i);
            ignore_unused_variable_warning (n);
            ignore_unused_variable_warning (cit);
            ignore_unused_variable_warning (cit_begin);
            ignore_unused_variable_warning (cit_end);
            ignore_unused_variable_warning (crit_begin);
            ignore_unused_variable_warning (crit_end);
            ignore_unused_variable_warning (t);
        }
    };

    template<class VE>
    struct Mutable_VectorExpressionConcept {
        typedef VE vector_expression_type;
        typedef typename VE::size_type size_type;
        typedef typename VE::value_type value_type;
        typedef typename VE::iterator iterator_type;
        typedef typename VE::reverse_iterator reverse_iterator_type;

        void constraints () {
            function_requires< AssignableConcept<vector_expression_type> >();
            function_requires< VectorExpressionConcept<vector_expression_type> >();
            vector_expression_type *vp;
            vector_expression_type v = *vp, v1 = *vp, v2 = *vp;
            size_type i (0);
            value_type t = value_type ();
            // Find (internal?)
            iterator_type it (v.find (i));
            // Beginning of range
            iterator_type it_begin (v.begin ());
            // End of range
            iterator_type it_end (v.end ());
            // Swap
            v1.swap (v2);
            // Beginning of reverse range
            reverse_iterator_type rit_begin (v.rbegin ());
            // End of reverse range
            reverse_iterator_type rit_end (v.rend ());
            // Assignments
            v2 = v1;
            v2.assign (v1);
            v2 += v1;
            v2.plus_assign (v1);
            v2 -= v1;
            v2.minus_assign (v1);
            v *= t;
            ignore_unused_variable_warning (it);
            ignore_unused_variable_warning (it_begin);
            ignore_unused_variable_warning (it_end);
            ignore_unused_variable_warning (rit_begin);
            ignore_unused_variable_warning (rit_end);
        }
    };

    template<class ME>
    struct MatrixExpressionConcept {
        typedef ME matrix_expression_type;
        typedef typename ME::type_category type_category;
        typedef typename ME::size_type size_type;
        typedef typename ME::value_type value_type;
        typedef typename ME::const_iterator1 const_subiterator1_type;
        typedef typename ME::const_iterator2 const_subiterator2_type;
        typedef typename ME::const_reverse_iterator1 const_reverse_subiterator1_type;
        typedef typename ME::const_reverse_iterator2 const_reverse_subiterator2_type;

        void constraints () {
            matrix_expression_type *mp;
            const matrix_expression_type *cmp;
            matrix_expression_type m = *mp;
            const matrix_expression_type cm = *cmp;
            size_type n (0), i (0), j (0);
            value_type t;
            // Find (internal?)
            const_subiterator1_type cit1 (m.find1 (0, i, j));
            const_subiterator2_type cit2 (m.find2 (0, i, j));
            // Beginning of range
            const_subiterator1_type cit1_begin (m.begin1 ());
            const_subiterator2_type cit2_begin (m.begin2 ());
            // End of range
            const_subiterator1_type cit1_end (m.end1 ());
            const_subiterator2_type cit2_end (m.end2 ());
            // Size
            n = m.size1 ();
            n = m.size2 ();
            // Beginning of reverse range
            const_reverse_subiterator1_type crit1_begin (cm.rbegin1 ());
            const_reverse_subiterator2_type crit2_begin (cm.rbegin2 ());
            // End of reverse range
            const_reverse_subiterator1_type crit1_end (cm.rend1 ());
            const_reverse_subiterator2_type crit2_end (cm.rend2 ());
            // Element access
            t = m (i, j);
            ignore_unused_variable_warning (n);
            ignore_unused_variable_warning (cit1);
            ignore_unused_variable_warning (cit2);
            ignore_unused_variable_warning (cit1_begin);
            ignore_unused_variable_warning (cit2_begin);
            ignore_unused_variable_warning (cit1_end);
            ignore_unused_variable_warning (cit2_end);
            ignore_unused_variable_warning (crit1_begin);
            ignore_unused_variable_warning (crit2_begin);
            ignore_unused_variable_warning (crit1_end);
            ignore_unused_variable_warning (crit2_end);
            ignore_unused_variable_warning (t);
        }
    };

    template<class ME>
    struct Mutable_MatrixExpressionConcept {
        typedef ME matrix_expression_type;
        typedef typename ME::size_type size_type;
        typedef typename ME::value_type value_type;
        typedef typename ME::iterator1 subiterator1_type;
        typedef typename ME::iterator2 subiterator2_type;
        typedef typename ME::reverse_iterator1 reverse_subiterator1_type;
        typedef typename ME::reverse_iterator2 reverse_subiterator2_type;

        void constraints () {
            function_requires< AssignableConcept<matrix_expression_type> >();
            function_requires< MatrixExpressionConcept<matrix_expression_type> >();
            matrix_expression_type *mp;
            matrix_expression_type m = *mp, m1 = *mp, m2 = *mp;
            size_type i (0), j (0);
            value_type t = value_type ();
            // Find (internal?)
            subiterator1_type it1 (m.find1 (0, i, j));
            subiterator2_type it2 (m.find2 (0, i, j));
            // Beginning of range
            subiterator1_type it1_begin (m.begin1 ());
            subiterator2_type it2_begin (m.begin2 ());
            // End of range
            subiterator1_type it1_end (m.end1 ());
            subiterator2_type it2_end (m.end2 ());
            // Swap
            m1.swap (m2);
            // Beginning of reverse range
            reverse_subiterator1_type rit1_begin (m.rbegin1 ());
            reverse_subiterator2_type rit2_begin (m.rbegin2 ());
            // End of reverse range
            reverse_subiterator1_type rit1_end (m.rend1 ());
            reverse_subiterator2_type rit2_end (m.rend2 ());
            // Assignments
            m2 = m1;
            m2.assign (m1);
            m2 += m1;
            m2.plus_assign (m1);
            m2 -= m1;
            m2.minus_assign (m1);
            m *= t;
            ignore_unused_variable_warning (it1);
            ignore_unused_variable_warning (it2);
            ignore_unused_variable_warning (it1_begin);
            ignore_unused_variable_warning (it2_begin);
            ignore_unused_variable_warning (it1_end);
            ignore_unused_variable_warning (it2_end);
            ignore_unused_variable_warning (rit1_begin);
            ignore_unused_variable_warning (rit2_begin);
            ignore_unused_variable_warning (rit1_end);
            ignore_unused_variable_warning (rit2_end);
        }
    };

    template<class V>
    struct VectorConcept {
        typedef V vector_type;
        typedef typename V::size_type size_type;
        typedef typename V::value_type value_type;
        typedef const value_type *const_pointer;

        void constraints () {
            function_requires< VectorExpressionConcept<vector_type> >();
            size_type n (0);
            size_type i (0);
            // Sizing constructor
            vector_type v (n);
            // Element support
            const_pointer p = v.find_element (i);

            ignore_unused_variable_warning (p);
        }
    };

    template<class V>
    struct Mutable_VectorConcept {
        typedef V vector_type;
        typedef typename V::size_type size_type;
        typedef typename V::value_type value_type;
        typedef value_type *pointer;

        void constraints () {
            function_requires< VectorConcept<vector_type> >();
            function_requires< DefaultConstructible<vector_type> >();
            function_requires< Mutable_VectorExpressionConcept<vector_type> >();
            size_type n (0);
            value_type t = value_type ();
            size_type i (0);
            vector_type v;
            // Element support
            pointer p = v.find_element (i);
            // Element assignment
            value_type r = v.insert_element (i, t);
            v.insert_element (i, t) = r;
            // Zeroing
            v.clear ();
            // Resize
            v.resize (n);

            ignore_unused_variable_warning (p);
            ignore_unused_variable_warning (r);
        }
    };

    template<class V>
    struct SparseVectorConcept {
        typedef V vector_type;
        typedef typename V::size_type size_type;

        void constraints () {
            function_requires< VectorConcept<vector_type> >();
        }
    };

    template<class V>
    struct Mutable_SparseVectorConcept {
        typedef V vector_type;
        typedef typename V::size_type size_type;
        typedef typename V::value_type value_type;

        void constraints () {
            function_requires< SparseVectorConcept<vector_type> >();
            function_requires< Mutable_VectorConcept<vector_type> >();
            size_type i (0);
            vector_type v;
            // Element erasure
            v.erase_element (i);
        }
    };

    template<class M>
    struct MatrixConcept {
        typedef M matrix_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;
        typedef const value_type *const_pointer;

        void constraints () {
            function_requires< MatrixExpressionConcept<matrix_type> >();
            size_type n (0);
            size_type i (0), j (0);
            // Sizing constructor
            matrix_type m (n, n);
            // Element support
#ifndef SKIP_BAD
            const_pointer p = m.find_element (i, j);
#else
            const_pointer p;
            ignore_unused_variable_warning (i);
            ignore_unused_variable_warning (j);
#endif
            ignore_unused_variable_warning (p);
        }
    };

    template<class M>
    struct Mutable_MatrixConcept {
        typedef M matrix_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;
        typedef value_type *pointer;

        void constraints () {
            function_requires< MatrixConcept<matrix_type> >();
            function_requires< DefaultConstructible<matrix_type> >();
            function_requires< Mutable_MatrixExpressionConcept<matrix_type> >();
            size_type n (0);
            value_type t = value_type ();
            size_type i (0), j (0);
            matrix_type m;
            // Element support
#ifndef SKIP_BAD
            pointer p = m.find_element (i, j);
            ignore_unused_variable_warning (i);
            ignore_unused_variable_warning (j);
#else
            pointer p;
#endif
            // Element assigment
            value_type r = m.insert_element (i, j, t);
            m.insert_element (i, j, t) = r;
            // Zeroing
            m.clear ();
            // Resize
            m.resize (n, n);
            m.resize (n, n, false);

            ignore_unused_variable_warning (p);
            ignore_unused_variable_warning (r);
        }
    };

    template<class M>
    struct SparseMatrixConcept {
        typedef M matrix_type;
        typedef typename M::size_type size_type;

        void constraints () {
            function_requires< MatrixConcept<matrix_type> >();
        }
    };

    template<class M>
    struct Mutable_SparseMatrixConcept {
        typedef M matrix_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

        void constraints () {
            function_requires< SparseMatrixConcept<matrix_type> >();
            function_requires< Mutable_MatrixConcept<matrix_type> >();
            size_type i (0), j (0);
            matrix_type m;
            // Elemnent erasure
            m.erase_element (i, j);
        }
    };

    /** introduce anonymous namespace to make following functions
     * local to the current compilation unit.
     */
    namespace {

    // Replaced the ZeroElement and OneElement functions with the templated versions
    // because the former where giving warnings with clang
    template<class T>
    T
    ZeroElement (T) {
        return static_cast<T> (0);
    }

    template<class T>
    vector<T>
    ZeroElement (vector<T>) {
        return zero_vector<T> ();
    }

    template<class T>
    matrix<T>
    ZeroElement (matrix<T>) {
        return zero_matrix<T> ();
    }

    template<class T>
    T
    OneElement (T) {
        return static_cast<T> (1);
    }

    template<class T>
    matrix<T>
    OneElement (matrix<T>) {
        return identity_matrix<T> ();
    }


    template<class E1, class E2>
    bool
    operator == (const vector_expression<E1> &e1, const vector_expression<E2> &e2) {
        typedef typename promote_traits<typename E1::value_type,
                                                    typename E2::value_type>::promote_type value_type;
        typedef typename type_traits<value_type>::real_type real_type;
        return norm_inf (e1 - e2) == real_type/*zero*/();
    }
    template<class E1, class E2>
    bool
    operator == (const matrix_expression<E1> &e1, const matrix_expression<E2> &e2) {
        typedef typename promote_traits<typename E1::value_type,
                                                    typename E2::value_type>::promote_type value_type;
        typedef typename type_traits<value_type>::real_type real_type;
        return norm_inf (e1 - e2) == real_type/*zero*/();
    }

    template<class T>
    struct AdditiveAbelianGroupConcept {
        typedef T value_type;

        void constraints () {
            bool r;
            value_type a = value_type (), b = value_type (), c = value_type ();
            r = (a + b) + c == a + (b + c);
            r = ZeroElement (value_type ()) + a == a;
            r = a + ZeroElement (value_type ()) == a;
            r = a + (- a) == ZeroElement (value_type ());
            r = (- a) + a == ZeroElement (value_type ());
            r = a + b == b + a;
            ignore_unused_variable_warning (r);
        }
    };

    template<class T>
    struct MultiplicativeAbelianGroupConcept {
        typedef T value_type;

        void constraints () {
            bool r;
            value_type a = value_type (), b = value_type (), c = value_type ();
            r = (a * b) * c == a * (b * c);
            r = OneElement (value_type ()) * a == a;
            r = a * OneElement (value_type ()) == a;
            r = a * (OneElement (value_type ()) / a) == a;
            r = (OneElement (value_type ()) / a) * a == a;
            r = a * b == b * a;
            ignore_unused_variable_warning (r);
        }
    };

    template<class T>
    struct RingWithIdentityConcept {
        typedef T value_type;

        void constraints () {
            function_requires< AdditiveAbelianGroupConcept<value_type> >();
            bool r;
            value_type a = value_type (), b = value_type (), c = value_type ();
            r = (a * b) * c == a * (b * c);
            r = (a + b) * c == a * c + b * c;
            r = OneElement (value_type ()) * a == a;
            r = a * OneElement (value_type ()) == a;
            ignore_unused_variable_warning (r);
        }
    };

    template<class T>
    struct Prod_RingWithIdentityConcept {
        typedef T value_type;

        void constraints () {
            function_requires< AdditiveAbelianGroupConcept<value_type> >();
            bool r;
            value_type a = value_type (), b = value_type (), c = value_type ();
            r = prod (T (prod (a, b)), c) == prod (a, T (prod (b, c)));
            r = prod (a + b, c) == prod (a, c) + prod (b, c);
            r = prod (OneElement (value_type ()), a) == a;
            r = prod (a, OneElement (value_type ())) == a;
            ignore_unused_variable_warning (r);
        }
    };

    template<class T>
    struct CommutativeRingWithIdentityConcept {
        typedef T value_type;

        void constraints () {
            function_requires< RingWithIdentityConcept<value_type> >();
            bool r;
            value_type a = value_type (), b = value_type ();
            r = a * b == b * a;
            ignore_unused_variable_warning (r);
        }
    };

    template<class T>
    struct FieldConcept {
        typedef T value_type;

        void constraints () {
            function_requires< CommutativeRingWithIdentityConcept<value_type> >();
            bool r;
            value_type a = value_type ();
            r = a == ZeroElement (value_type ()) || a * (OneElement (value_type ()) / a) == a;
            r = a == ZeroElement (value_type ()) || (OneElement (value_type ()) / a) * a == a;
            ignore_unused_variable_warning (r);
        }
    };

    template<class T, class V>
    struct VectorSpaceConcept {
        typedef T value_type;
        typedef V vector_type;

        void constraints () {
            function_requires< FieldConcept<value_type> >();
            function_requires< AdditiveAbelianGroupConcept<vector_type> >();
            bool r;
            value_type alpha = value_type (), beta = value_type ();
            vector_type a = vector_type (), b = vector_type ();
            r = alpha * (a + b) == alpha * a + alpha * b;
            r = (alpha + beta) * a == alpha * a + beta * a;
            r = (alpha * beta) * a == alpha * (beta * a);
            r = OneElement (value_type ()) * a == a;
            ignore_unused_variable_warning (r);
        }
    };

    template<class T, class V, class M>
    struct LinearOperatorConcept {
        typedef T value_type;
        typedef V vector_type;
        typedef M matrix_type;

        void constraints () {
            function_requires< VectorSpaceConcept<value_type, vector_type> >();
            bool r;
            value_type alpha = value_type (), beta = value_type ();
            vector_type a = vector_type (), b = vector_type ();
            matrix_type A = matrix_type ();
            r = prod (A, alpha * a + beta * b) == alpha * prod (A, a) + beta * prod (A, b);
            ignore_unused_variable_warning (r);
        }
    };

inline void concept_checks () {

        // Allow tests to be group to keep down compiler storage requirement
#ifdef INTERAL
#define INTERNAL_STORAGE
#define INTERNAL_VECTOR
#define INTERNAL_MATRIX
#define INTERNAL_SPECIAL
#define INTERNAL_SPARSE
#define INTERNAL_EXPRESSION
#endif

        // TODO enable this for development
        // #define VIEW_CONCEPTS

        // Element value type for tests
        typedef float T;

        // Storage Array
#if defined (INTERNAL_STORAGE) || defined (INTERNAL_STORAGE_DENSE)
        {
            typedef std::vector<T> container_model;
            function_requires< Mutable_StorageArrayConcept<container_model> >();
            function_requires< RandomAccessIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_RandomAccessIteratorConcept<container_model::iterator> >();
        }

        {
            typedef bounded_array<T, 1> container_model;
            function_requires< Mutable_StorageArrayConcept<container_model> >();
            function_requires< RandomAccessIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_RandomAccessIteratorConcept<container_model::iterator> >();
        }

        {
            typedef unbounded_array<T> container_model;
            function_requires< Mutable_StorageArrayConcept<container_model> >();
            function_requires< RandomAccessIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_RandomAccessIteratorConcept<container_model::iterator> >();
        }

/* FIXME array_adaptors are in progress
        {
            typedef array_adaptor<T> container_model;
            function_requires< Mutable_StorageArrayConcept<container_model> >();
            function_requires< RandomAccessIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_RandomAccessIteratorConcept<container_model::iterator> >();
        }
*/

        {
            typedef range container_model;
            function_requires< IndexSetConcept<range> >();
            function_requires< RandomAccessIteratorConcept<range::const_iterator> >();
        }

        {
            typedef slice container_model;
            function_requires< IndexSetConcept<range> >();
            function_requires< RandomAccessIteratorConcept<range::const_iterator> >();
        }

        {
            typedef indirect_array<> container_model;
            function_requires< IndexSetConcept<range> >();
            function_requires< RandomAccessIteratorConcept<range::const_iterator> >();
        }
#endif

        // Storage Sparse
#if defined (INTERNAL_STORAGE) || defined (INTERNAL_STORAGE_SPARSE)
        {
           typedef map_array<std::size_t, T> container_model;
           function_requires< Mutable_StorageSparseConcept<container_model> >();
           function_requires< RandomAccessIteratorConcept<container_model::const_iterator> >();
           function_requires< RandomAccessIteratorConcept<container_model::iterator> >();
        }

        {
           typedef std::map<std::size_t, T> container_model;
           function_requires< Mutable_StorageSparseConcept<container_model > >();
           function_requires< BidirectionalIteratorConcept<container_model::const_iterator> >();
           function_requires< BidirectionalIteratorConcept<container_model::iterator> >();
        }
#endif

#ifdef VIEW_CONCEPTS
        // read only vectors
        {
           typedef vector_view<T> container_model;
           function_requires< RandomAccessContainerConcept<container_model> >();
           function_requires< VectorConcept<container_model> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
        }
#endif

        // Vector
#if defined (INTERNAL_VECTOR) || defined (INTERNAL_VECTOR_DENSE)
        {
           typedef vector<T> container_model;
           function_requires< RandomAccessContainerConcept<container_model> >();
           function_requires< Mutable_VectorConcept<container_model> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
           typedef zero_vector<T> container_model;
           function_requires< VectorConcept<container_model> >();
           function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_iterator> >();
           function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_reverse_iterator> >();
        }

        {
           typedef unit_vector<T> container_model;
           function_requires< VectorConcept<container_model> >();
           function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_iterator> >();
           function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_reverse_iterator> >();
        }

        {
           typedef scalar_vector<T> container_model;
           function_requires< VectorConcept<container_model> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
        }

        {
           typedef c_vector<T, 1> container_model;
           function_requires< Mutable_VectorConcept<container_model> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }
#endif

        // Vector Proxies
#if defined (INTERNAL_VECTOR) || defined (INTERNAL_VECTOR_PROXY)
        {
           typedef vector_range<vector<T> > container_model;
           function_requires< Mutable_VectorExpressionConcept<container_model> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
           typedef vector_slice<vector<T> > container_model;
           function_requires< Mutable_VectorExpressionConcept<container_model> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
           typedef vector_indirect<vector<T> > container_model;
           function_requires< Mutable_VectorExpressionConcept<container_model> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
           function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
           function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }
#endif

        // Sparse Vector
#if defined (INTERNAL_SPARSE) || defined (INTERNAL_VECTOR_SPARSE)
        {
            typedef mapped_vector<T> container_model;
            function_requires< Mutable_SparseVectorConcept<container_model> >();
            function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_IndexedBidirectional1DIteratorConcept<container_model::iterator> >();
            function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_reverse_iterator> >();
            function_requires< Mutable_IndexedBidirectional1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
            typedef compressed_vector<T> container_model;
            function_requires< Mutable_SparseVectorConcept<container_model> >();
            function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_IndexedBidirectional1DIteratorConcept<container_model::iterator> >();
            function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_reverse_iterator> >();
            function_requires< Mutable_IndexedBidirectional1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
            typedef coordinate_vector<T> container_model;
            function_requires< Mutable_SparseVectorConcept<container_model> >();
            function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_IndexedBidirectional1DIteratorConcept<container_model::iterator> >();
            function_requires< IndexedBidirectional1DIteratorConcept<container_model::const_reverse_iterator> >();
            function_requires< Mutable_IndexedBidirectional1DIteratorConcept<container_model::reverse_iterator> >();
        }
#endif

        // Matrix
#if defined (INTERNAL_MATRIX) || defined (INTERNAL_MATRIX_DENSE)
        {
            typedef matrix<T> container_model;
            function_requires< Mutable_MatrixConcept<matrix<T> > >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }

        {
            typedef vector_of_vector<T> container_model;
            function_requires< Mutable_MatrixConcept<matrix<T> > >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }

        {
            typedef zero_matrix<T> container_model;
            function_requires< Mutable_MatrixConcept<matrix<T> > >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
        }

        {
            typedef identity_matrix<T> container_model;
            function_requires< Mutable_MatrixConcept<matrix<T> > >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
        }

        {
            typedef scalar_matrix<T> container_model;
            function_requires< Mutable_MatrixConcept<matrix<T> > >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
        }

        {
            typedef c_matrix<T, 1, 1> container_model;
            function_requires< Mutable_MatrixConcept<matrix<T> > >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
#endif

        // Matrix Proxies
#if defined (INTERNAL_MATRIX) || defined (INTERNAL_MATRIX_PROXY)
        {
            typedef matrix_row<matrix<T> > container_model;
            function_requires< Mutable_VectorExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
            typedef matrix_column<matrix<T> > container_model;
            function_requires< Mutable_VectorExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
            typedef matrix_vector_range<matrix<T> > container_model;
            function_requires< Mutable_VectorExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
            typedef matrix_vector_slice<matrix<T> > container_model;
            function_requires< Mutable_VectorExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
            typedef matrix_vector_indirect<matrix<T> > container_model;
            function_requires< Mutable_VectorExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<container_model::const_reverse_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<container_model::reverse_iterator> >();
        }

        {
            typedef matrix_range<matrix<T> > container_model;
            function_requires< Mutable_MatrixExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }

        {
            typedef matrix_slice<matrix<T> > container_model;
            function_requires< Mutable_MatrixExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }

        {
            typedef matrix_indirect<matrix<T> > container_model;
            function_requires< Mutable_MatrixExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
#endif

        // Banded Matrix
#if defined (INTERNAL_SPECIAL) || defined (INTERNAL_BANDED)
        {
            typedef banded_matrix<T> container_model;
            function_requires< Mutable_MatrixConcept<container_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }

        {
            typedef banded_adaptor<matrix<T> > container_model;
            function_requires< Mutable_MatrixExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
#endif

        // Triangular Matrix
#if defined (INTERNAL_SPECIAL) || defined (INTERNAL_TRIANGULAR)
        {
            typedef triangular_matrix<T> container_model;
            function_requires< Mutable_MatrixConcept<container_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }

        {
            typedef triangular_adaptor<matrix<T> > container_model;
            function_requires< Mutable_MatrixExpressionConcept<container_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
#endif

        // Symmetric Matrix
#if defined (INTERNA_SPECIAL) || defined (INTERNAL_SYMMETRIC)
        {
            typedef symmetric_matrix<T> container_model;
            function_requires< Mutable_MatrixConcept<container_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }

        {
            typedef banded_adaptor<matrix<T> > container_model;
#ifndef SKIP_BAD
           // const_iterator (iterator) constructor is bad
            function_requires< Mutable_MatrixExpressionConcept<container_model> >();
#endif
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
#endif

        // Hermitian Matrix
#if defined (INTERNAL_SPECIAL) || defined (INTERNAL_HERMITIAN)
        {
            typedef hermitian_matrix<T> container_model;
            function_requires< Mutable_MatrixConcept<container_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
        
        {
            typedef hermitian_adaptor<matrix<T> > container_model;
#ifndef SKIP_BAD
           // const_iterator (iterator) constructor is bad
            function_requires< Mutable_MatrixExpressionConcept<container_model> >();
#endif
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
#endif

        // Sparse Matrix
#if defined (INTERNAL_SPARSE) || defined (INTERNAL_MATRIX_SPARSE)
        {
            typedef mapped_matrix<T> container_model;
            function_requires< Mutable_SparseMatrixConcept<container_model> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
        {
            typedef mapped_vector_of_mapped_vector<T> container_model;
            function_requires< Mutable_SparseMatrixConcept<container_model> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
        {
            typedef compressed_matrix<T> container_model;
            function_requires< Mutable_SparseMatrixConcept<container_model> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
        {
            typedef coordinate_matrix<T> container_model;
            function_requires< Mutable_SparseMatrixConcept<container_model> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }
        {
            typedef generalized_vector_of_vector<T, row_major, vector< coordinate_vector<T> > > container_model;
            function_requires< Mutable_SparseMatrixConcept<container_model> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_iterator1, container_model::const_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::iterator1, container_model::iterator2> >();
            function_requires< IndexedBidirectional2DIteratorConcept<container_model::const_reverse_iterator1, container_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedBidirectional2DIteratorConcept<container_model::reverse_iterator1, container_model::reverse_iterator2> >();
        }

#endif

        // Scalar Expressions
#if defined (INTERNAL_EXPRESSION) || defined (INTERNAL_VECTOR_EXPRESSION)
        function_requires< ScalarExpressionConcept<scalar_value<T> > >();
        function_requires< ScalarExpressionConcept<scalar_reference<T> > >();

        // Vector Expressions
        {
            typedef vector_reference<vector<T> > expression_model;
            function_requires< VectorExpressionConcept<expression_model> >();
            function_requires< Mutable_VectorExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<expression_model::iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_reverse_iterator> >();
            function_requires< Mutable_IndexedRandomAccess1DIteratorConcept<expression_model::reverse_iterator> >();
        }

        {
            typedef vector_unary<vector<T>, scalar_identity<T> > expression_model;
            function_requires< VectorExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_reverse_iterator> >();
        }

        {
            typedef vector_binary<vector<T>, vector<T>, scalar_plus<T, T> > expression_model;
            function_requires< VectorExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_reverse_iterator> >();
        }

        {
            typedef vector_binary_scalar1<T, vector<T>, scalar_multiplies<T, T> > expression_model;
            function_requires< VectorExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_reverse_iterator> >();
        }

        {
            typedef vector_binary_scalar2<vector<T>, scalar_value<T>, scalar_multiplies<T, T> > expression_model;
            function_requires< VectorExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_reverse_iterator> >();
        }

        {
            typedef vector_binary_scalar1<scalar_value<T>, vector<T>, scalar_multiplies<T, T> > expression_model;
            function_requires< VectorExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_reverse_iterator> >();
        }

        {
            typedef vector_binary_scalar2<vector<T>, scalar_value<T>, scalar_multiplies<T, T> > expression_model;
            function_requires< VectorExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_reverse_iterator> >();
        }

        function_requires< ScalarExpressionConcept<vector_scalar_unary<vector<T>, vector_sum<vector<T> > > > >();
        function_requires< ScalarExpressionConcept<vector_scalar_unary<vector<T>, vector_norm_1<vector<T> > > > >();
        function_requires< ScalarExpressionConcept<vector_scalar_unary<vector<T>, vector_norm_2<vector<T> > > > >();
        function_requires< ScalarExpressionConcept<vector_scalar_unary<vector<T>, vector_norm_inf<vector<T> > > > >();

        function_requires< ScalarExpressionConcept<vector_scalar_binary<vector<T>, vector<T>, vector_inner_prod<vector<T>, vector<T>, T> > > >();
#endif

        // Matrix Expressions
#if defined (INTERNAL_EXPRESSION) || defined (INTERNAL_MATRIX_EXPRESSION)
        {
            typedef matrix_reference<matrix<T> > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< Mutable_MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<expression_model::iterator1, expression_model::iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
            function_requires< Mutable_IndexedRandomAccess2DIteratorConcept<expression_model::reverse_iterator1, expression_model::reverse_iterator2> >();
        }

        {
            typedef vector_matrix_binary<vector<T>, vector<T>, scalar_multiplies<T, T> > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
        }

        {
            typedef matrix_unary1<matrix<T>, scalar_identity<T> > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
        }

        {
            typedef matrix_unary2<matrix<T>, scalar_identity<T> > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
        }

        {
            typedef matrix_binary<matrix<T>, matrix<T>, scalar_plus<T, T> > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
        }

        {
            typedef matrix_binary_scalar1<T, matrix<T>, scalar_multiplies<T, T> > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
        }

        {
            typedef matrix_binary_scalar2<matrix<T>, T, scalar_multiplies<T, T> > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
        }

        {
            typedef matrix_binary_scalar1<scalar_value<T>, matrix<T>, scalar_multiplies<T, T> > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
        }

        {
            typedef matrix_binary_scalar2<matrix<T>, scalar_value<T>, scalar_multiplies<T, T> > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
        }

        {
            typedef matrix_vector_binary1<matrix<T>, vector<T>, matrix_vector_prod1<matrix<T>, vector<T>, T> > expression_model;
            function_requires< VectorExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_reverse_iterator> >();
        }

        {
            typedef matrix_vector_binary2<vector<T>, matrix<T>, matrix_vector_prod2<matrix<T>, vector<T>, T > > expression_model;
            function_requires< VectorExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_iterator> >();
            function_requires< IndexedRandomAccess1DIteratorConcept<expression_model::const_reverse_iterator> >();
        }

        {
            typedef matrix_matrix_binary<matrix<T>, matrix<T>, matrix_matrix_prod<matrix<T>, matrix<T>, T > > expression_model;
            function_requires< MatrixExpressionConcept<expression_model> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_iterator1, expression_model::const_iterator2> >();
            function_requires< IndexedRandomAccess2DIteratorConcept<expression_model::const_reverse_iterator1, expression_model::const_reverse_iterator2> >();
        }

        function_requires< ScalarExpressionConcept<matrix_scalar_unary<matrix<T>, matrix_norm_1<vector<T> > > > >();
        function_requires< ScalarExpressionConcept<matrix_scalar_unary<matrix<T>, matrix_norm_frobenius<vector<T> > > > >();
        function_requires< ScalarExpressionConcept<matrix_scalar_unary<matrix<T>, matrix_norm_inf<vector<T> > > > >();
#endif

#ifdef EXTERNAL
        function_requires< AdditiveAbelianGroupConcept<T> >();
        function_requires< CommutativeRingWithIdentityConcept<T> >();
        function_requires< FieldConcept<T> >();
        function_requires< VectorSpaceConcept<T, vector<T> > >();
        function_requires< Prod_RingWithIdentityConcept<matrix<T> > >();
        function_requires< VectorSpaceConcept<T, matrix<T> > >();
        function_requires< LinearOperatorConcept<T, vector<T>, matrix<T> > >();

        function_requires< AdditiveAbelianGroupConcept<std::complex<T> > >();
        function_requires< CommutativeRingWithIdentityConcept<std::complex<T> > >();
        function_requires< FieldConcept<std::complex<T> > >();
        function_requires< VectorSpaceConcept<std::complex<T>, vector<std::complex<T> > > >();
        function_requires< Prod_RingWithIdentityConcept<matrix<std::complex<T> > > >();
        function_requires< VectorSpaceConcept<std::complex<T>, matrix<std::complex<T> > > >();
        function_requires< LinearOperatorConcept<std::complex<T>, vector<std::complex<T> >, matrix<std::complex<T> > > >();
#endif
    }

    } // end of anonymous namespace

}}}

#endif
