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

#ifndef _BOOST_UBLAS_TRIANGULAR_
#define _BOOST_UBLAS_TRIANGULAR_

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/detail/temporary.hpp>
#include <boost/type_traits/remove_const.hpp>

// Iterators based on ideas of Jeremy Siek

namespace boost { namespace numeric { namespace ublas {

    namespace detail {
        using namespace boost::numeric::ublas;

        // Matrix resizing algorithm
        template <class L, class T, class M>
        BOOST_UBLAS_INLINE
        void matrix_resize_preserve (M& m, M& temporary) {
            typedef L layout_type;
            typedef T triangular_type;
            typedef typename M::size_type size_type;
            const size_type msize1 (m.size1 ());        // original size
            const size_type msize2 (m.size2 ());
            const size_type size1 (temporary.size1 ());    // new size is specified by temporary
            const size_type size2 (temporary.size2 ());
            // Common elements to preserve
            const size_type size1_min = (std::min) (size1, msize1);
            const size_type size2_min = (std::min) (size2, msize2);
            // Order for major and minor sizes
            const size_type major_size = layout_type::size_M (size1_min, size2_min);
            const size_type minor_size = layout_type::size_m (size1_min, size2_min);
            // Indexing copy over major
            for (size_type major = 0; major != major_size; ++major) {
                for (size_type minor = 0; minor != minor_size; ++minor) {
                        // find indexes - use invertability of element_ functions
                    const size_type i1 = layout_type::index_M(major, minor);
                    const size_type i2 = layout_type::index_m(major, minor);
                    if ( triangular_type::other(i1,i2) ) {
                        temporary.data () [triangular_type::element (layout_type (), i1, size1, i2, size2)] =
                            m.data() [triangular_type::element (layout_type (), i1, msize1, i2, msize2)];
                    }
                }
            }
            m.assign_temporary (temporary);
        }
    }

    /** \brief A triangular matrix of values of type \c T.
     *
     * For a \f$(n \times n )\f$-dimensional lower triangular matrix and if \f$0 \leq i < n\f$, \f$0 \leq j < n\f$ and \f$i>j\f$ holds, 
     * \f$m_{i,j}=0\f$. Furthermore if \f$m_{i,i}=1\f$, the matrix is called unit lower triangular.
     *
     * For a \f$(n \times n )\f$-dimensional upper triangular matrix and if \f$0 \leq i < n\f$, \f$0 \leq j < n\f$ and \f$i<j\f$ holds, 
     * \f$m_{i,j}=0\f$. Furthermore if \f$m_{i,i}=1\f$, the matrix is called unit upper triangular.
     *
     * The default storage for triangular matrices is packed. Orientation and storage can also be specified. 
     * Default is \c row_major and and unbounded_array. It is \b not required by the storage to initialize 
     * elements of the matrix.
     *
     * \tparam T the type of object stored in the matrix (like double, float, complex, etc...)
     * \tparam TRI the type of the triangular matrix. It can either be \c lower or \c upper. Default is \c lower
     * \tparam L the storage organization. It can be either \c row_major or \c column_major. Default is \c row_major
     * \tparam A the type of Storage array. Default is \c unbounded_array
     */
    template<class T, class TRI, class L, class A>
    class triangular_matrix:
        public matrix_container<triangular_matrix<T, TRI, L, A> > {

        typedef T *pointer;
        typedef TRI triangular_type;
        typedef L layout_type;
        typedef triangular_matrix<T, TRI, L, A> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_container<self_type>::operator ();
#endif
        typedef typename A::size_type size_type;
        typedef typename A::difference_type difference_type;
        typedef T value_type;
        typedef const T &const_reference;
        typedef T &reference;
        typedef A array_type;

        typedef const matrix_reference<const self_type> const_closure_type;
        typedef matrix_reference<self_type> closure_type;
        typedef vector<T, A> vector_temporary_type;
        typedef matrix<T, L, A> matrix_temporary_type;  // general sub-matrix
        typedef packed_tag storage_category;
        typedef typename L::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        triangular_matrix ():
            matrix_container<self_type> (),
            size1_ (0), size2_ (0), data_ (0) {}
        BOOST_UBLAS_INLINE
        triangular_matrix (size_type size1, size_type size2):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), data_ (triangular_type::packed_size (layout_type (), size1, size2)) {
        }
        BOOST_UBLAS_INLINE
        triangular_matrix (size_type size1, size_type size2, const array_type &data):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), data_ (data) {}
        BOOST_UBLAS_INLINE
        triangular_matrix (const triangular_matrix &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_), data_ (m.data_) {}
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_matrix (const matrix_expression<AE> &ae):
            matrix_container<self_type> (),
            size1_ (ae ().size1 ()), size2_ (ae ().size2 ()),
            data_ (triangular_type::packed_size (layout_type (), size1_, size2_)) {
            matrix_assign<scalar_assign> (*this, ae);
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size1 () const {
            return size1_;
        }
        BOOST_UBLAS_INLINE
        size_type size2 () const {
            return size2_;
        }

        // Storage accessors
        BOOST_UBLAS_INLINE
        const array_type &data () const {
            return data_;
        }
        BOOST_UBLAS_INLINE
        array_type &data () {
            return data_;
        }

        // Resizing
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool preserve = true) {
            if (preserve) {
                self_type temporary (size1, size2);
                detail::matrix_resize_preserve<layout_type, triangular_type> (*this, temporary);
            }
            else {
                data ().resize (triangular_type::packed_size (layout_type (), size1, size2));
                size1_ = size1;
                size2_ = size2;
            }
        }
        BOOST_UBLAS_INLINE
        void resize_packed_preserve (size_type size1, size_type size2) {
            size1_ = size1;
            size2_ = size2;
            data ().resize (triangular_type::packed_size (layout_type (), size1_, size2_), value_type ());
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            BOOST_UBLAS_CHECK (i < size1_, bad_index ());
            BOOST_UBLAS_CHECK (j < size2_, bad_index ());
            if (triangular_type::other (i, j))
                return data () [triangular_type::element (layout_type (), i, size1_, j, size2_)];
            else if (triangular_type::one (i, j))
                return one_;
            else
                return zero_;
        }
        BOOST_UBLAS_INLINE
        reference at_element (size_type i, size_type j) {
            BOOST_UBLAS_CHECK (i < size1_, bad_index ());
            BOOST_UBLAS_CHECK (j < size2_, bad_index ());
            return data () [triangular_type::element (layout_type (), i, size1_, j, size2_)];
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
            BOOST_UBLAS_CHECK (i < size1_, bad_index ());
            BOOST_UBLAS_CHECK (j < size2_, bad_index ());
            if (!triangular_type::other (i, j)) {
                bad_index ().raise ();
                // NEVER reached
            }
            return data () [triangular_type::element (layout_type (), i, size1_, j, size2_)];
        }
        
        // Element assignment
        BOOST_UBLAS_INLINE
        reference insert_element (size_type i, size_type j, const_reference t) {
            return (operator () (i, j) = t);
        }
        BOOST_UBLAS_INLINE
        void erase_element (size_type i, size_type j) {
            operator () (i, j) = value_type/*zero*/();
        }
        
        // Zeroing
        BOOST_UBLAS_INLINE
        void clear () {
            // data ().clear ();
            std::fill (data ().begin (), data ().end (), value_type/*zero*/());
        }

        // Assignment
        BOOST_UBLAS_INLINE
        triangular_matrix &operator = (const triangular_matrix &m) {
            size1_ = m.size1_;
            size2_ = m.size2_;
            data () = m.data ();
            return *this;
        }
        BOOST_UBLAS_INLINE
        triangular_matrix &assign_temporary (triangular_matrix &m) {
            swap (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_matrix &operator = (const matrix_expression<AE> &ae) {
            self_type temporary (ae);
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_matrix &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_matrix& operator += (const matrix_expression<AE> &ae) {
            self_type temporary (*this + ae);
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_matrix &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_matrix& operator -= (const matrix_expression<AE> &ae) {
            self_type temporary (*this - ae);
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_matrix &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        triangular_matrix& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        triangular_matrix& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (triangular_matrix &m) {
            if (this != &m) {
                // BOOST_UBLAS_CHECK (size2_ == m.size2_, bad_size ());
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                data ().swap (m.data ());
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (triangular_matrix &m1, triangular_matrix &m2) {
            m1.swap (m2);
        }

        // Iterator types
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator1<self_type, packed_random_access_iterator_tag> iterator1;
        typedef indexed_iterator2<self_type, packed_random_access_iterator_tag> iterator2;
        typedef indexed_const_iterator1<self_type, packed_random_access_iterator_tag> const_iterator1;
        typedef indexed_const_iterator2<self_type, packed_random_access_iterator_tag> const_iterator2;
#else
        class const_iterator1;
        class iterator1;
        class const_iterator2;
        class iterator2;
#endif
        typedef reverse_iterator_base1<const_iterator1> const_reverse_iterator1;
        typedef reverse_iterator_base1<iterator1> reverse_iterator1;
        typedef reverse_iterator_base2<const_iterator2> const_reverse_iterator2;
        typedef reverse_iterator_base2<iterator2> reverse_iterator2;

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator1 find1 (int rank, size_type i, size_type j) const {
            if (rank == 1)
                i = triangular_type::restrict1 (i, j, size1_, size2_);
            if (rank == 0)
                i = triangular_type::global_restrict1 (i, size1_, j, size2_);
            return const_iterator1 (*this, i, j);
        }
        BOOST_UBLAS_INLINE
        iterator1 find1 (int rank, size_type i, size_type j) {
            if (rank == 1)
                i = triangular_type::mutable_restrict1 (i, j, size1_, size2_);
            if (rank == 0)
                i = triangular_type::global_mutable_restrict1 (i, size1_, j, size2_);
            return iterator1 (*this, i, j);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int rank, size_type i, size_type j) const {
            if (rank == 1)
                j = triangular_type::restrict2 (i, j, size1_, size2_);
            if (rank == 0)
                j = triangular_type::global_restrict2 (i, size1_, j, size2_);
            return const_iterator2 (*this, i, j);
        }
        BOOST_UBLAS_INLINE
        iterator2 find2 (int rank, size_type i, size_type j) {
            if (rank == 1)
                j = triangular_type::mutable_restrict2 (i, j, size1_, size2_);
            if (rank == 0)
                j = triangular_type::global_mutable_restrict2 (i, size1_, j, size2_);
            return iterator2 (*this, i, j);
        }

        // Iterators simply are indices.

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<triangular_matrix>,
            public random_access_iterator_base<packed_random_access_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename triangular_matrix::value_type value_type;
            typedef typename triangular_matrix::difference_type difference_type;
            typedef typename triangular_matrix::const_reference reference;
            typedef const typename triangular_matrix::pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, size_type it1, size_type it2):
                container_const_reference<self_type> (m), it1_ (it1), it2_ (it2) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), it1_ (it.it1_), it2_ (it.it2_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                ++ it1_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                -- it1_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator += (difference_type n) {
                it1_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -= (difference_type n) {
                it1_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ - it.it1_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                return (*this) () (it1_, it2_);
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 begin () const {
                return (*this) ().find2 (1, it1_, 0);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 cbegin () const {
                return begin ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 end () const {
                return (*this) ().find2 (1, it1_, (*this) ().size2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 cend () const {
                return end ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator2 rbegin () const {
                return const_reverse_iterator2 (end ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator2 crbegin () const {
                return rbegin ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator2 rend () const {
                return const_reverse_iterator2 (begin ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator2 crend () const {
                return rend ();
            }
#endif

            // Indices
            BOOST_UBLAS_INLINE
            size_type index1 () const {
                return it1_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_;
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator1 &operator = (const const_iterator1 &it) {
                container_const_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ == it.it1_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ < it.it1_;
            }

        private:
            size_type it1_;
            size_type it2_;
        };
#endif

        BOOST_UBLAS_INLINE
        const_iterator1 begin1 () const {
            return find1 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        const_iterator1 cbegin1 () const {
            return begin1 ();
        }
        BOOST_UBLAS_INLINE
        const_iterator1 end1 () const {
            return find1 (0, size1_, 0);
        }
        BOOST_UBLAS_INLINE
        const_iterator1 cend1 () const {
            return end1 ();
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class iterator1:
            public container_reference<triangular_matrix>,
            public random_access_iterator_base<packed_random_access_iterator_tag,
                                               iterator1, value_type> {
        public:
            typedef typename triangular_matrix::value_type value_type;
            typedef typename triangular_matrix::difference_type difference_type;
            typedef typename triangular_matrix::reference reference;
            typedef typename triangular_matrix::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, size_type it1, size_type it2):
                container_reference<self_type> (m), it1_ (it1), it2_ (it2) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                ++ it1_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                -- it1_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator += (difference_type n) {
                it1_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -= (difference_type n) {
                it1_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ - it.it1_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                return (*this) () (it1_, it2_);
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 begin () const {
                return (*this) ().find2 (1, it1_, 0);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 end () const {
                return (*this) ().find2 (1, it1_, (*this) ().size2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            reverse_iterator2 rbegin () const {
                return reverse_iterator2 (end ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            reverse_iterator2 rend () const {
                return reverse_iterator2 (begin ());
            }
#endif

            // Indices
            BOOST_UBLAS_INLINE
            size_type index1 () const {
                return it1_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_;
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator1 &operator = (const iterator1 &it) {
                container_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ == it.it1_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ < it.it1_;
            }

        private:
            size_type it1_;
            size_type it2_;

            friend class const_iterator1;
        };
#endif

        BOOST_UBLAS_INLINE
        iterator1 begin1 () {
            return find1 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator1 end1 () {
            return find1 (0, size1_, 0);
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator2:
            public container_const_reference<triangular_matrix>,
            public random_access_iterator_base<packed_random_access_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename triangular_matrix::value_type value_type;
            typedef typename triangular_matrix::difference_type difference_type;
            typedef typename triangular_matrix::const_reference reference;
            typedef const typename triangular_matrix::pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, size_type it1, size_type it2):
                container_const_reference<self_type> (m), it1_ (it1), it2_ (it2) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), it1_ (it.it1_), it2_ (it.it2_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator += (difference_type n) {
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -= (difference_type n) {
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ - it.it2_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                return (*this) () (it1_, it2_);
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 begin () const {
                return (*this) ().find1 (1, 0, it2_);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 cbegin () const {
                return begin ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 end () const {
                return (*this) ().find1 (1, (*this) ().size1 (), it2_);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 cend () const {
                return end ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator1 rbegin () const {
                return const_reverse_iterator1 (end ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator1 crbegin () const {
                return rbegin ();
            }

            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator1 rend () const {
                return const_reverse_iterator1 (begin ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator1 crend () const {
                return rend ();
            }
#endif

            // Indices
            BOOST_UBLAS_INLINE
            size_type index1 () const {
                return it1_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_;
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator2 &operator = (const const_iterator2 &it) {
                container_const_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ < it.it2_;
            }

        private:
            size_type it1_;
            size_type it2_;
        };
#endif

        BOOST_UBLAS_INLINE
        const_iterator2 begin2 () const {
            return find2 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 cbegin2 () const {
            return begin2 ();
        }
        BOOST_UBLAS_INLINE
        const_iterator2 end2 () const {
            return find2 (0, 0, size2_);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 cend2 () const {
            return end2 ();
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class iterator2:
            public container_reference<triangular_matrix>,
            public random_access_iterator_base<packed_random_access_iterator_tag,
                                               iterator2, value_type> {
        public:
            typedef typename triangular_matrix::value_type value_type;
            typedef typename triangular_matrix::difference_type difference_type;
            typedef typename triangular_matrix::reference reference;
            typedef typename triangular_matrix::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, size_type it1, size_type it2):
                container_reference<self_type> (m), it1_ (it1), it2_ (it2) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator += (difference_type n) {
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -= (difference_type n) {
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ - it.it2_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                return (*this) () (it1_, it2_);
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 begin () const {
                return (*this) ().find1 (1, 0, it2_);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 end () const {
                return (*this) ().find1 (1, (*this) ().size1 (), it2_);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            reverse_iterator1 rbegin () const {
                return reverse_iterator1 (end ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            reverse_iterator1 rend () const {
                return reverse_iterator1 (begin ());
            }
#endif

            // Indices
            BOOST_UBLAS_INLINE
            size_type index1 () const {
                return it1_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_;
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator2 &operator = (const iterator2 &it) {
                container_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ < it.it2_;
            }

        private:
            size_type it1_;
            size_type it2_;

            friend class const_iterator2;
        };
#endif

        BOOST_UBLAS_INLINE
        iterator2 begin2 () {
            return find2 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator2 end2 () {
            return find2 (0, 0, size2_);
        }

        // Reverse iterators

        BOOST_UBLAS_INLINE
        const_reverse_iterator1 rbegin1 () const {
            return const_reverse_iterator1 (end1 ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator1 crbegin1 () const {
            return rbegin1 ();
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator1 rend1 () const {
            return const_reverse_iterator1 (begin1 ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator1 crend1 () const {
            return rend1 ();
        }

        BOOST_UBLAS_INLINE
        reverse_iterator1 rbegin1 () {
            return reverse_iterator1 (end1 ());
        }
        BOOST_UBLAS_INLINE
        reverse_iterator1 rend1 () {
            return reverse_iterator1 (begin1 ());
        }

        BOOST_UBLAS_INLINE
        const_reverse_iterator2 rbegin2 () const {
            return const_reverse_iterator2 (end2 ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator2 crbegin2 () const {
            return rbegin2 ();
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator2 rend2 () const {
            return const_reverse_iterator2 (begin2 ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator2 crend2 () const {
            return rend2 ();
        }

        BOOST_UBLAS_INLINE
        reverse_iterator2 rbegin2 () {
            return reverse_iterator2 (end2 ());
        }
        BOOST_UBLAS_INLINE
        reverse_iterator2 rend2 () {
            return reverse_iterator2 (begin2 ());
        }

    private:
        size_type size1_;
        size_type size2_;
        array_type data_;
        static const value_type zero_;
        static const value_type one_;
    };

    template<class T, class TRI, class L, class A>
    const typename triangular_matrix<T, TRI, L, A>::value_type triangular_matrix<T, TRI, L, A>::zero_ = value_type/*zero*/();
    template<class T, class TRI, class L, class A>
    const typename triangular_matrix<T, TRI, L, A>::value_type triangular_matrix<T, TRI, L, A>::one_ (1);


    // Triangular matrix adaptor class
    template<class M, class TRI>
    class triangular_adaptor:
        public matrix_expression<triangular_adaptor<M, TRI> > {

        typedef triangular_adaptor<M, TRI> self_type;

    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_expression<self_type>::operator ();
#endif
        typedef const M const_matrix_type;
        typedef M matrix_type;
        typedef TRI triangular_type;
        typedef typename M::size_type size_type;
        typedef typename M::difference_type difference_type;
        typedef typename M::value_type value_type;
        typedef typename M::const_reference const_reference;
        typedef typename boost::mpl::if_<boost::is_const<M>,
                                          typename M::const_reference,
                                          typename M::reference>::type reference;
        typedef typename boost::mpl::if_<boost::is_const<M>,
                                          typename M::const_closure_type,
                                          typename M::closure_type>::type matrix_closure_type;
        typedef const self_type const_closure_type;
        typedef self_type closure_type;
        // Replaced by _temporary_traits to avoid type requirements on M
        //typedef typename M::vector_temporary_type vector_temporary_type;
        //typedef typename M::matrix_temporary_type matrix_temporary_type;
        typedef typename storage_restrict_traits<typename M::storage_category,
                                                 packed_proxy_tag>::storage_category storage_category;
        typedef typename M::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        triangular_adaptor (matrix_type &data):
            matrix_expression<self_type> (),
            data_ (data) {}
        BOOST_UBLAS_INLINE
        triangular_adaptor (const triangular_adaptor &m):
            matrix_expression<self_type> (),
            data_ (m.data_) {}

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size1 () const {
            return data_.size1 ();
        }
        BOOST_UBLAS_INLINE
        size_type size2 () const {
            return data_.size2 ();
        }

        // Storage accessors
        BOOST_UBLAS_INLINE
        const matrix_closure_type &data () const {
            return data_;
        }
        BOOST_UBLAS_INLINE
        matrix_closure_type &data () {
            return data_;
        }

        // Element access
#ifndef BOOST_UBLAS_PROXY_CONST_MEMBER
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            BOOST_UBLAS_CHECK (i < size1 (), bad_index ());
            BOOST_UBLAS_CHECK (j < size2 (), bad_index ());
            if (triangular_type::other (i, j))
                return data () (i, j);
            else if (triangular_type::one (i, j))
                return one_;
            else
                return zero_;
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
            BOOST_UBLAS_CHECK (i < size1 (), bad_index ());
            BOOST_UBLAS_CHECK (j < size2 (), bad_index ());
            if (!triangular_type::other (i, j)) {
                bad_index ().raise ();
                // NEVER reached
            }
            return data () (i, j);
        }
#else
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) const {
            BOOST_UBLAS_CHECK (i < size1 (), bad_index ());
            BOOST_UBLAS_CHECK (j < size2 (), bad_index ());
            if (!triangular_type::other (i, j)) {
                bad_index ().raise ();
                // NEVER reached
            }
            return data () (i, j);
        }
#endif

        // Assignment
        BOOST_UBLAS_INLINE
        triangular_adaptor &operator = (const triangular_adaptor &m) {
            matrix_assign<scalar_assign> (*this, m);
            return *this;
        }
        BOOST_UBLAS_INLINE
        triangular_adaptor &assign_temporary (triangular_adaptor &m) {
            *this = m;
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_adaptor &operator = (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, matrix<value_type> (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_adaptor &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_adaptor& operator += (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, matrix<value_type> (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_adaptor &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_adaptor& operator -= (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, matrix<value_type> (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        triangular_adaptor &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        triangular_adaptor& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        triangular_adaptor& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const triangular_adaptor &ta) const {
            return (*this).data ().same_closure (ta.data ());
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (triangular_adaptor &m) {
            if (this != &m)
                matrix_swap<scalar_swap> (*this, m);
        }
        BOOST_UBLAS_INLINE
        friend void swap (triangular_adaptor &m1, triangular_adaptor &m2) {
            m1.swap (m2);
        }

        // Iterator types
   private:
        typedef typename M::const_iterator1 const_subiterator1_type;
        typedef typename boost::mpl::if_<boost::is_const<M>,
                                          typename M::const_iterator1,
                                          typename M::iterator1>::type subiterator1_type;
        typedef typename M::const_iterator2 const_subiterator2_type;
        typedef typename boost::mpl::if_<boost::is_const<M>,
                                          typename M::const_iterator2,
                                          typename M::iterator2>::type subiterator2_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator1<self_type, packed_random_access_iterator_tag> iterator1;
        typedef indexed_iterator2<self_type, packed_random_access_iterator_tag> iterator2;
        typedef indexed_const_iterator1<self_type, packed_random_access_iterator_tag> const_iterator1;
        typedef indexed_const_iterator2<self_type, packed_random_access_iterator_tag> const_iterator2;
#else
        class const_iterator1;
        class iterator1;
        class const_iterator2;
        class iterator2;
#endif
        typedef reverse_iterator_base1<const_iterator1> const_reverse_iterator1;
        typedef reverse_iterator_base1<iterator1> reverse_iterator1;
        typedef reverse_iterator_base2<const_iterator2> const_reverse_iterator2;
        typedef reverse_iterator_base2<iterator2> reverse_iterator2;

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator1 find1 (int rank, size_type i, size_type j) const {
            if (rank == 1)
                i = triangular_type::restrict1 (i, j, size1(), size2());
            if (rank == 0)
                i = triangular_type::global_restrict1 (i, size1(), j, size2());
            return const_iterator1 (*this, data ().find1 (rank, i, j));
        }
        BOOST_UBLAS_INLINE
        iterator1 find1 (int rank, size_type i, size_type j) {
            if (rank == 1)
                i = triangular_type::mutable_restrict1 (i, j, size1(), size2());
            if (rank == 0)
                i = triangular_type::global_mutable_restrict1 (i, size1(), j, size2());
            return iterator1 (*this, data ().find1 (rank, i, j));
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int rank, size_type i, size_type j) const {
            if (rank == 1)
                j = triangular_type::restrict2 (i, j, size1(), size2());
            if (rank == 0)
                j = triangular_type::global_restrict2 (i, size1(), j, size2());
            return const_iterator2 (*this, data ().find2 (rank, i, j));
        }
        BOOST_UBLAS_INLINE
        iterator2 find2 (int rank, size_type i, size_type j) {
            if (rank == 1)
                j = triangular_type::mutable_restrict2 (i, j, size1(), size2());
            if (rank == 0)
                j = triangular_type::global_mutable_restrict2 (i, size1(), j, size2());
            return iterator2 (*this, data ().find2 (rank, i, j));
        }

        // Iterators simply are indices.

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<triangular_adaptor>,
            public random_access_iterator_base<typename iterator_restrict_traits<
                                                   typename const_subiterator1_type::iterator_category, packed_random_access_iterator_tag>::iterator_category,
                                               const_iterator1, value_type> {
        public:
            typedef typename const_subiterator1_type::value_type value_type;
            typedef typename const_subiterator1_type::difference_type difference_type;
            typedef typename const_subiterator1_type::reference reference;
            typedef typename const_subiterator1_type::pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), it1_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, const const_subiterator1_type &it1):
                container_const_reference<self_type> (m), it1_ (it1) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), it1_ (it.it1_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                ++ it1_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                -- it1_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator += (difference_type n) {
                it1_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -= (difference_type n) {
                it1_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it1_ - it.it1_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                size_type i = index1 ();
                size_type j = index2 ();
                BOOST_UBLAS_CHECK (i < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (j < (*this) ().size2 (), bad_index ());
                if (triangular_type::other (i, j))
                    return *it1_;
                else
                    return (*this) () (i, j);
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 begin () const {
                return (*this) ().find2 (1, index1 (), 0);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 cbegin () const {
                return begin ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 end () const {
                return (*this) ().find2 (1, index1 (), (*this) ().size2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 cend () const {
                return end ();
            }

            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator2 rbegin () const {
                return const_reverse_iterator2 (end ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator2 crbegin () const {
                return rbegin ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator2 rend () const {
                return const_reverse_iterator2 (begin ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator2 crend () const {
                return rend ();
            }
#endif

            // Indices
            BOOST_UBLAS_INLINE
            size_type index1 () const {
                return it1_.index1 ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it1_.index2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator1 &operator = (const const_iterator1 &it) {
                container_const_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it1_ == it.it1_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it1_ < it.it1_;
            }

        private:
            const_subiterator1_type it1_;
        };
#endif

        BOOST_UBLAS_INLINE
        const_iterator1 begin1 () const {
            return find1 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        const_iterator1 cbegin1 () const {
            return begin1 ();
        }
        BOOST_UBLAS_INLINE
        const_iterator1 end1 () const {
            return find1 (0, size1 (), 0);
        }
        BOOST_UBLAS_INLINE
        const_iterator1 cend1 () const {
            return end1 ();
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class iterator1:
            public container_reference<triangular_adaptor>,
            public random_access_iterator_base<typename iterator_restrict_traits<
                                                   typename subiterator1_type::iterator_category, packed_random_access_iterator_tag>::iterator_category,
                                               iterator1, value_type> {
        public:
            typedef typename subiterator1_type::value_type value_type;
            typedef typename subiterator1_type::difference_type difference_type;
            typedef typename subiterator1_type::reference reference;
            typedef typename subiterator1_type::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), it1_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, const subiterator1_type &it1):
                container_reference<self_type> (m), it1_ (it1) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                ++ it1_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                -- it1_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator += (difference_type n) {
                it1_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -= (difference_type n) {
                it1_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it1_ - it.it1_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                size_type i = index1 ();
                size_type j = index2 ();
                BOOST_UBLAS_CHECK (i < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (j < (*this) ().size2 (), bad_index ());
                if (triangular_type::other (i, j))
                    return *it1_;
                else
                    return (*this) () (i, j);
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 begin () const {
                return (*this) ().find2 (1, index1 (), 0);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 end () const {
                return (*this) ().find2 (1, index1 (), (*this) ().size2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            reverse_iterator2 rbegin () const {
                return reverse_iterator2 (end ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            reverse_iterator2 rend () const {
                return reverse_iterator2 (begin ());
            }
#endif

            // Indices
            BOOST_UBLAS_INLINE
            size_type index1 () const {
                return it1_.index1 ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it1_.index2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator1 &operator = (const iterator1 &it) {
                container_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it1_ == it.it1_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it1_ < it.it1_;
            }

        private:
            subiterator1_type it1_;

            friend class const_iterator1;
        };
#endif

        BOOST_UBLAS_INLINE
        iterator1 begin1 () {
            return find1 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator1 end1 () {
            return find1 (0, size1 (), 0);
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator2:
            public container_const_reference<triangular_adaptor>,
            public random_access_iterator_base<typename iterator_restrict_traits<
                                                   typename const_subiterator1_type::iterator_category, packed_random_access_iterator_tag>::iterator_category,
                                               const_iterator2, value_type> {
        public:
            typedef typename const_subiterator2_type::value_type value_type;
            typedef typename const_subiterator2_type::difference_type difference_type;
            typedef typename const_subiterator2_type::reference reference;
            typedef typename const_subiterator2_type::pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, const const_subiterator2_type &it2):
                container_const_reference<self_type> (m), it2_ (it2) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), it2_ (it.it2_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator += (difference_type n) {
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -= (difference_type n) {
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it2_ - it.it2_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                size_type i = index1 ();
                size_type j = index2 ();
                BOOST_UBLAS_CHECK (i < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (j < (*this) ().size2 (), bad_index ());
                if (triangular_type::other (i, j))
                    return *it2_;
                else
                    return (*this) () (i, j);
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 begin () const {
                return (*this) ().find1 (1, 0, index2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 cbegin () const {
                return begin ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 end () const {
                return (*this) ().find1 (1, (*this) ().size1 (), index2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 cend () const {
                return end ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator1 rbegin () const {
                return const_reverse_iterator1 (end ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator1 crbegin () const {
                return rbegin ();
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator1 rend () const {
                return const_reverse_iterator1 (begin ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_reverse_iterator1 crend () const {
                return rend ();
            }
#endif

            // Indices
            BOOST_UBLAS_INLINE
            size_type index1 () const {
                return it2_.index1 ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator2 &operator = (const const_iterator2 &it) {
                container_const_reference<self_type>::assign (&it ());
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it2_ < it.it2_;
            }

        private:
            const_subiterator2_type it2_;
        };
#endif

        BOOST_UBLAS_INLINE
        const_iterator2 begin2 () const {
            return find2 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 cbegin2 () const {
            return begin2 ();
        }
        BOOST_UBLAS_INLINE
        const_iterator2 end2 () const {
            return find2 (0, 0, size2 ());
        }
        BOOST_UBLAS_INLINE
        const_iterator2 cend2 () const {
            return end2 ();
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class iterator2:
            public container_reference<triangular_adaptor>,
            public random_access_iterator_base<typename iterator_restrict_traits<
                                                   typename subiterator1_type::iterator_category, packed_random_access_iterator_tag>::iterator_category,
                                               iterator2, value_type> {
        public:
            typedef typename subiterator2_type::value_type value_type;
            typedef typename subiterator2_type::difference_type difference_type;
            typedef typename subiterator2_type::reference reference;
            typedef typename subiterator2_type::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, const subiterator2_type &it2):
                container_reference<self_type> (m), it2_ (it2) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator += (difference_type n) {
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -= (difference_type n) {
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it2_ - it.it2_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                size_type i = index1 ();
                size_type j = index2 ();
                BOOST_UBLAS_CHECK (i < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (j < (*this) ().size2 (), bad_index ());
                if (triangular_type::other (i, j))
                    return *it2_;
                else
                    return (*this) () (i, j);
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 begin () const {
                return (*this) ().find1 (1, 0, index2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 end () const {
                return (*this) ().find1 (1, (*this) ().size1 (), index2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            reverse_iterator1 rbegin () const {
                return reverse_iterator1 (end ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            reverse_iterator1 rend () const {
                return reverse_iterator1 (begin ());
            }
#endif

            // Indices
            BOOST_UBLAS_INLINE
            size_type index1 () const {
                return it2_.index1 ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator2 &operator = (const iterator2 &it) {
                container_reference<self_type>::assign (&it ());
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it2_ < it.it2_;
            }

        private:
            subiterator2_type it2_;

            friend class const_iterator2;
        };
#endif

        BOOST_UBLAS_INLINE
        iterator2 begin2 () {
            return find2 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator2 end2 () {
            return find2 (0, 0, size2 ());
        }

        // Reverse iterators

        BOOST_UBLAS_INLINE
        const_reverse_iterator1 rbegin1 () const {
            return const_reverse_iterator1 (end1 ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator1 crbegin1 () const {
            return rbegin1 ();
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator1 rend1 () const {
            return const_reverse_iterator1 (begin1 ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator1 crend1 () const {
            return rend1 ();
        }

        BOOST_UBLAS_INLINE
        reverse_iterator1 rbegin1 () {
            return reverse_iterator1 (end1 ());
        }
        BOOST_UBLAS_INLINE
        reverse_iterator1 rend1 () {
            return reverse_iterator1 (begin1 ());
        }

        BOOST_UBLAS_INLINE
        const_reverse_iterator2 rbegin2 () const {
            return const_reverse_iterator2 (end2 ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator2 crbegin2 () const {
            return rbegin2 ();
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator2 rend2 () const {
            return const_reverse_iterator2 (begin2 ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator2 crend2 () const {
            return rend2 ();
        }

        BOOST_UBLAS_INLINE
        reverse_iterator2 rbegin2 () {
            return reverse_iterator2 (end2 ());
        }
        BOOST_UBLAS_INLINE
        reverse_iterator2 rend2 () {
            return reverse_iterator2 (begin2 ());
        }

    private:
        matrix_closure_type data_;
        static const value_type zero_;
        static const value_type one_;
    };

    template<class M, class TRI>
    const typename triangular_adaptor<M, TRI>::value_type triangular_adaptor<M, TRI>::zero_ = value_type/*zero*/();
    template<class M, class TRI>
    const typename triangular_adaptor<M, TRI>::value_type triangular_adaptor<M, TRI>::one_ (1);

    template <class M, class TRI>
    struct vector_temporary_traits< triangular_adaptor<M, TRI> >
    : vector_temporary_traits< typename boost::remove_const<M>::type > {} ;
    template <class M, class TRI>
    struct vector_temporary_traits< const triangular_adaptor<M, TRI> >
    : vector_temporary_traits< typename boost::remove_const<M>::type > {} ;

    template <class M, class TRI>
    struct matrix_temporary_traits< triangular_adaptor<M, TRI> >
    : matrix_temporary_traits< typename boost::remove_const<M>::type > {};
    template <class M, class TRI>
    struct matrix_temporary_traits< const triangular_adaptor<M, TRI> >
    : matrix_temporary_traits< typename boost::remove_const<M>::type > {};


    template<class E1, class E2>
    struct matrix_vector_solve_traits {
        typedef typename promote_traits<typename E1::value_type, typename E2::value_type>::promote_type promote_type;
        typedef vector<promote_type> result_type;
    };

    // Operations:
    //  n * (n - 1) / 2 + n = n * (n + 1) / 2 multiplications,
    //  n * (n - 1) / 2 additions

    // Dense (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        lower_tag, column_major_tag, dense_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e2 ().size ();
        for (size_type n = 0; n < size; ++ n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n) /= e1 () (n, n);
            if (t != value_type/*zero*/()) {
                for (size_type m = n + 1; m < size; ++ m)
                    e2 () (m) -= e1 () (m, n) * t;
            }
        }
    }
    // Packed (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        lower_tag, column_major_tag, packed_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e2 ().size ();
        for (size_type n = 0; n < size; ++ n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n) /= e1 () (n, n);
            if (t != value_type/*zero*/()) {
                typename E1::const_iterator1 it1e1 (e1 ().find1 (1, n + 1, n));
                typename E1::const_iterator1 it1e1_end (e1 ().find1 (1, e1 ().size1 (), n));
                difference_type m (it1e1_end - it1e1);
                while (-- m >= 0)
                    e2 () (it1e1.index1 ()) -= *it1e1 * t, ++ it1e1;
            }
        }
    }
    // Sparse (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        lower_tag, column_major_tag, unknown_storage_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e2 ().size ();
        for (size_type n = 0; n < size; ++ n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n) /= e1 () (n, n);
            if (t != value_type/*zero*/()) {
                typename E1::const_iterator1 it1e1 (e1 ().find1 (1, n + 1, n));
                typename E1::const_iterator1 it1e1_end (e1 ().find1 (1, e1 ().size1 (), n));
                while (it1e1 != it1e1_end)
                    e2 () (it1e1.index1 ()) -= *it1e1 * t, ++ it1e1;
            }
        }
    }

    // Dense (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        lower_tag, row_major_tag, dense_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e2 ().size ();
        for (size_type n = 0; n < size; ++ n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n) /= e1 () (n, n);
            if (t != value_type/*zero*/()) {
                for (size_type m = n + 1; m < size; ++ m)
                    e2 () (m) -= e1 () (m, n) * t;
            }
        }
    }
    // Packed (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        lower_tag, row_major_tag, packed_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e2 ().size ();
        for (size_type n = 0; n < size; ++ n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n);
            typename E1::const_iterator2 it2e1 (e1 ().find2 (1, n, 0));
            typename E1::const_iterator2 it2e1_end (e1 ().find2 (1, n, n));
            while (it2e1 != it2e1_end) {
              t -= *it2e1 * e2 () (it2e1.index2());
              ++ it2e1;
            }
            e2() (n) = t / e1 () (n, n);
        }
    }
    // Sparse (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        lower_tag, row_major_tag, unknown_storage_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e2 ().size ();
        for (size_type n = 0; n < size; ++ n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n);
            typename E1::const_iterator2 it2e1 (e1 ().find2 (1, n, 0));
            typename E1::const_iterator2 it2e1_end (e1 ().find2 (1, n, n));
            while (it2e1 != it2e1_end) {
              t -= *it2e1 * e2 () (it2e1.index2());
              ++ it2e1;
            }
            e2() (n) = t / e1 () (n, n);
        }
    }

    // Redirectors :-)
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        lower_tag, column_major_tag) {
        typedef typename E1::storage_category storage_category;
        inplace_solve (e1, e2,
                       lower_tag (), column_major_tag (), storage_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        lower_tag, row_major_tag) {
        typedef typename E1::storage_category storage_category;
        inplace_solve (e1, e2,
                       lower_tag (), row_major_tag (), storage_category ());
    }
    // Dispatcher
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        lower_tag) {
        typedef typename E1::orientation_category orientation_category;
        inplace_solve (e1, e2,
                       lower_tag (), orientation_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        unit_lower_tag) {
        typedef typename E1::orientation_category orientation_category;
        inplace_solve (triangular_adaptor<const E1, unit_lower> (e1 ()), e2,
                       unit_lower_tag (), orientation_category ());
    }

    // Dense (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        upper_tag, column_major_tag, dense_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e2 ().size ();
        for (difference_type n = size - 1; n >= 0; -- n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n) /= e1 () (n, n);
            if (t != value_type/*zero*/()) {
                for (difference_type m = n - 1; m >= 0; -- m)
                    e2 () (m) -= e1 () (m, n) * t;
            }
        }
    }
    // Packed (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        upper_tag, column_major_tag, packed_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e2 ().size ();
        for (difference_type n = size - 1; n >= 0; -- n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n) /= e1 () (n, n);
            if (t != value_type/*zero*/()) {
                typename E1::const_reverse_iterator1 it1e1 (e1 ().find1 (1, n, n));
                typename E1::const_reverse_iterator1 it1e1_rend (e1 ().find1 (1, 0, n));
                while (it1e1 != it1e1_rend) {
                  e2 () (it1e1.index1 ()) -= *it1e1 * t, ++ it1e1;
                }
            }
        }
    }
    // Sparse (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        upper_tag, column_major_tag, unknown_storage_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e2 ().size ();
        for (difference_type n = size - 1; n >= 0; -- n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n) /= e1 () (n, n);
            if (t != value_type/*zero*/()) {
                typename E1::const_reverse_iterator1 it1e1 (e1 ().find1 (1, n, n));
                typename E1::const_reverse_iterator1 it1e1_rend (e1 ().find1 (1, 0, n));
                while (it1e1 != it1e1_rend) {
                  e2 () (it1e1.index1 ()) -= *it1e1 * t, ++ it1e1;
                }
            }
        }
    }

    // Dense (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        upper_tag, row_major_tag, dense_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e1 ().size1 ();
        for (difference_type n = size-1; n >=0; -- n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n);
            for (difference_type m = n + 1; m < static_cast<difference_type>(e1 ().size2()); ++ m) {
              t -= e1 () (n, m)  * e2 () (m);
            }
            e2() (n) = t / e1 () (n, n);
        }
    }
    // Packed (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        upper_tag, row_major_tag, packed_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e1 ().size1 ();
        for (difference_type n = size-1; n >=0; -- n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n);
            typename E1::const_iterator2 it2e1 (e1 ().find2 (1, n, n+1));
            typename E1::const_iterator2 it2e1_end (e1 ().find2 (1, n, e1 ().size2 ()));
            while (it2e1 != it2e1_end) {
              t -= *it2e1 * e2 () (it2e1.index2());
              ++ it2e1;
            }
            e2() (n) = t / e1 () (n, n);

        }
    }
    // Sparse (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        upper_tag, row_major_tag, unknown_storage_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size (), bad_size ());
        size_type size = e1 ().size1 ();
        for (difference_type n = size-1; n >=0; -- n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            value_type t = e2 () (n);
            typename E1::const_iterator2 it2e1 (e1 ().find2 (1, n, n+1));
            typename E1::const_iterator2 it2e1_end (e1 ().find2 (1, n, e1 ().size2 ()));
            while (it2e1 != it2e1_end) {
              t -= *it2e1 * e2 () (it2e1.index2());
              ++ it2e1;
            }
            e2() (n) = t / e1 () (n, n);

        }
    }

    // Redirectors :-)
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        upper_tag, column_major_tag) {
        typedef typename E1::storage_category storage_category;
        inplace_solve (e1, e2,
                       upper_tag (), column_major_tag (), storage_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        upper_tag, row_major_tag) {
        typedef typename E1::storage_category storage_category;
        inplace_solve (e1, e2,
                       upper_tag (), row_major_tag (), storage_category ());
    }
    // Dispatcher
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        upper_tag) {
        typedef typename E1::orientation_category orientation_category;
        inplace_solve (e1, e2,
                       upper_tag (), orientation_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, vector_expression<E2> &e2,
                        unit_upper_tag) {
        typedef typename E1::orientation_category orientation_category;
        inplace_solve (triangular_adaptor<const E1, unit_upper> (e1 ()), e2,
                       unit_upper_tag (), orientation_category ());
    }

    template<class E1, class E2, class C>
    BOOST_UBLAS_INLINE
    typename matrix_vector_solve_traits<E1, E2>::result_type
    solve (const matrix_expression<E1> &e1,
           const vector_expression<E2> &e2,
           C) {
        typename matrix_vector_solve_traits<E1, E2>::result_type r (e2);
        inplace_solve (e1, r, C ());
        return r;
    }


    // Redirectors :-)
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (vector_expression<E1> &e1, const matrix_expression<E2> &e2,
                        lower_tag, row_major_tag) {
        typedef typename E2::storage_category storage_category;
        inplace_solve (trans(e2), e1,
                       upper_tag (), column_major_tag (), storage_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (vector_expression<E1> &e1, const matrix_expression<E2> &e2,
                        lower_tag, column_major_tag) {
        typedef typename E2::storage_category storage_category;
        inplace_solve (trans (e2), e1,
                       upper_tag (), row_major_tag (), storage_category ());
    }
    // Dispatcher
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (vector_expression<E1> &e1, const matrix_expression<E2> &e2,
                        lower_tag) {
        typedef typename E2::orientation_category orientation_category;
        inplace_solve (e1, e2,
                       lower_tag (), orientation_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (vector_expression<E1> &e1, const matrix_expression<E2> &e2,
                        unit_lower_tag) {
        typedef typename E2::orientation_category orientation_category;
        inplace_solve (e1, triangular_adaptor<const E2, unit_lower> (e2 ()),
                       unit_lower_tag (), orientation_category ());
    }


    // Redirectors :-)
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (vector_expression<E1> &e1, const matrix_expression<E2> &e2,
                        upper_tag, row_major_tag) {
        typedef typename E2::storage_category storage_category;
        inplace_solve (trans(e2), e1,
                       lower_tag (), column_major_tag (), storage_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (vector_expression<E1> &e1, const matrix_expression<E2> &e2,
                        upper_tag, column_major_tag) {
        typedef typename E2::storage_category storage_category;
        inplace_solve (trans (e2), e1,
                       lower_tag (), row_major_tag (), storage_category ());
    }
    // Dispatcher
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (vector_expression<E1> &e1, const matrix_expression<E2> &e2,
                        upper_tag) {
        typedef typename E2::orientation_category orientation_category;
        inplace_solve (e1, e2,
                       upper_tag (), orientation_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (vector_expression<E1> &e1, const matrix_expression<E2> &e2,
                        unit_upper_tag) {
        typedef typename E2::orientation_category orientation_category;
        inplace_solve (e1, triangular_adaptor<const E2, unit_upper> (e2 ()),
                       unit_upper_tag (), orientation_category ());
    }

    template<class E1, class E2, class C>
    BOOST_UBLAS_INLINE
    typename matrix_vector_solve_traits<E1, E2>::result_type
    solve (const vector_expression<E1> &e1,
           const matrix_expression<E2> &e2,
           C) {
        typename matrix_vector_solve_traits<E1, E2>::result_type r (e1);
        inplace_solve (r, e2, C ());
        return r;
    }

    template<class E1, class E2>
    struct matrix_matrix_solve_traits {
        typedef typename promote_traits<typename E1::value_type, typename E2::value_type>::promote_type promote_type;
        typedef matrix<promote_type> result_type;
    };

    // Operations:
    //  k * n * (n - 1) / 2 + k * n = k * n * (n + 1) / 2 multiplications,
    //  k * n * (n - 1) / 2 additions

    // Dense (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        lower_tag, dense_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size1 (), bad_size ());
        size_type size1 = e2 ().size1 ();
        size_type size2 = e2 ().size2 ();
        for (size_type n = 0; n < size1; ++ n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            for (size_type l = 0; l < size2; ++ l) {
                value_type t = e2 () (n, l) /= e1 () (n, n);
                if (t != value_type/*zero*/()) {
                    for (size_type m = n + 1; m < size1; ++ m)
                        e2 () (m, l) -= e1 () (m, n) * t;
                }
            }
        }
    }
    // Packed (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        lower_tag, packed_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size1 (), bad_size ());
        size_type size1 = e2 ().size1 ();
        size_type size2 = e2 ().size2 ();
        for (size_type n = 0; n < size1; ++ n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            for (size_type l = 0; l < size2; ++ l) {
                value_type t = e2 () (n, l) /= e1 () (n, n);
                if (t != value_type/*zero*/()) {
                    typename E1::const_iterator1 it1e1 (e1 ().find1 (1, n + 1, n));
                    typename E1::const_iterator1 it1e1_end (e1 ().find1 (1, e1 ().size1 (), n));
                    difference_type m (it1e1_end - it1e1);
                    while (-- m >= 0)
                        e2 () (it1e1.index1 (), l) -= *it1e1 * t, ++ it1e1;
                }
            }
        }
    }
    // Sparse (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        lower_tag, unknown_storage_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size1 (), bad_size ());
        size_type size1 = e2 ().size1 ();
        size_type size2 = e2 ().size2 ();
        for (size_type n = 0; n < size1; ++ n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            for (size_type l = 0; l < size2; ++ l) {
                value_type t = e2 () (n, l) /= e1 () (n, n);
                if (t != value_type/*zero*/()) {
                    typename E1::const_iterator1 it1e1 (e1 ().find1 (1, n + 1, n));
                    typename E1::const_iterator1 it1e1_end (e1 ().find1 (1, e1 ().size1 (), n));
                    while (it1e1 != it1e1_end)
                        e2 () (it1e1.index1 (), l) -= *it1e1 * t, ++ it1e1;
                }
            }
        }
    }
    // Dispatcher
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        lower_tag) {
        typedef typename E1::storage_category dispatch_category;
        inplace_solve (e1, e2,
                       lower_tag (), dispatch_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        unit_lower_tag) {
        typedef typename E1::storage_category dispatch_category;
        inplace_solve (triangular_adaptor<const E1, unit_lower> (e1 ()), e2,
                       unit_lower_tag (), dispatch_category ());
    }

    // Dense (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        upper_tag, dense_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size1 (), bad_size ());
        size_type size1 = e2 ().size1 ();
        size_type size2 = e2 ().size2 ();
        for (difference_type n = size1 - 1; n >= 0; -- n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            for (difference_type l = size2 - 1; l >= 0; -- l) {
                value_type t = e2 () (n, l) /= e1 () (n, n);
                if (t != value_type/*zero*/()) {
                    for (difference_type m = n - 1; m >= 0; -- m)
                        e2 () (m, l) -= e1 () (m, n) * t;
                }
            }
        }
    }
    // Packed (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        upper_tag, packed_proxy_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size1 (), bad_size ());
        size_type size1 = e2 ().size1 ();
        size_type size2 = e2 ().size2 ();
        for (difference_type n = size1 - 1; n >= 0; -- n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            for (difference_type l = size2 - 1; l >= 0; -- l) {
                value_type t = e2 () (n, l) /= e1 () (n, n);
                if (t != value_type/*zero*/()) {
                    typename E1::const_reverse_iterator1 it1e1 (e1 ().find1 (1, n, n));
                    typename E1::const_reverse_iterator1 it1e1_rend (e1 ().find1 (1, 0, n));
                    difference_type m (it1e1_rend - it1e1);
                    while (-- m >= 0)
                        e2 () (it1e1.index1 (), l) -= *it1e1 * t, ++ it1e1;
                }
            }
        }
    }
    // Sparse (proxy) case
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        upper_tag, unknown_storage_tag) {
        typedef typename E2::size_type size_type;
        typedef typename E2::difference_type difference_type;
        typedef typename E2::value_type value_type;

        BOOST_UBLAS_CHECK (e1 ().size1 () == e1 ().size2 (), bad_size ());
        BOOST_UBLAS_CHECK (e1 ().size2 () == e2 ().size1 (), bad_size ());
        size_type size1 = e2 ().size1 ();
        size_type size2 = e2 ().size2 ();
        for (difference_type n = size1 - 1; n >= 0; -- n) {
#ifndef BOOST_UBLAS_SINGULAR_CHECK
            BOOST_UBLAS_CHECK (e1 () (n, n) != value_type/*zero*/(), singular ());
#else
            if (e1 () (n, n) == value_type/*zero*/())
                singular ().raise ();
#endif
            for (difference_type l = size2 - 1; l >= 0; -- l) {
                value_type t = e2 () (n, l) /= e1 () (n, n);
                if (t != value_type/*zero*/()) {
                    typename E1::const_reverse_iterator1 it1e1 (e1 ().find1 (1, n, n));
                    typename E1::const_reverse_iterator1 it1e1_rend (e1 ().find1 (1, 0, n));
                    while (it1e1 != it1e1_rend)
                        e2 () (it1e1.index1 (), l) -= *it1e1 * t, ++ it1e1;
                }
            }
        }
    }
    // Dispatcher
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        upper_tag) {
        typedef typename E1::storage_category dispatch_category;
        inplace_solve (e1, e2,
                       upper_tag (), dispatch_category ());
    }
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    void inplace_solve (const matrix_expression<E1> &e1, matrix_expression<E2> &e2,
                        unit_upper_tag) {
        typedef typename E1::storage_category dispatch_category;
        inplace_solve (triangular_adaptor<const E1, unit_upper> (e1 ()), e2,
                       unit_upper_tag (), dispatch_category ());
    }

    template<class E1, class E2, class C>
    BOOST_UBLAS_INLINE
    typename matrix_matrix_solve_traits<E1, E2>::result_type
    solve (const matrix_expression<E1> &e1,
           const matrix_expression<E2> &e2,
           C) {
        typename matrix_matrix_solve_traits<E1, E2>::result_type r (e2);
        inplace_solve (e1, r, C ());
        return r;
    }

}}}

#endif
