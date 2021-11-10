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

#ifndef _BOOST_UBLAS_MATRIX_PROXY_
#define _BOOST_UBLAS_MATRIX_PROXY_

#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/detail/vector_assign.hpp>
#include <boost/numeric/ublas/detail/matrix_assign.hpp>
#include <boost/numeric/ublas/detail/temporary.hpp>

// Iterators based on ideas of Jeremy Siek

namespace boost { namespace numeric { namespace ublas {

    /** \brief 
     */
    template<class M>
    class matrix_row:
        public vector_expression<matrix_row<M> > {

        typedef matrix_row<M> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using vector_expression<self_type>::operator ();
#endif
        typedef M matrix_type;
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
        typedef typename storage_restrict_traits<typename M::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        matrix_row (matrix_type &data, size_type i):
            data_ (data), i_ (i) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (i_ < data_.size1 (), bad_index ());
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size () const {
            return data_.size2 ();
        }
        BOOST_UBLAS_INLINE
        size_type index () const {
            return i_;
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
        const_reference operator () (size_type j) const {
            return data_ (i_, j);
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type j) {
            return data_ (i_, j);
        }

        BOOST_UBLAS_INLINE
        const_reference operator [] (size_type j) const {
            return (*this) (j);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (size_type j) {
            return (*this) (j);
        }
#else
        BOOST_UBLAS_INLINE
        reference operator () (size_type j) const {
            return data_ (i_, j);
        }

        BOOST_UBLAS_INLINE
        reference operator [] (size_type j) const {
            return (*this) (j);
        }
#endif

        // Assignment
        BOOST_UBLAS_INLINE
        matrix_row &operator = (const matrix_row &mr) {
            // ISSUE need a temporary, proxy can be overlaping alias
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (mr));
            return *this;
        }
        BOOST_UBLAS_INLINE
        matrix_row &assign_temporary (matrix_row &mr) {
            // assign elements, proxied container remains the same
            vector_assign<scalar_assign> (*this, mr);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_row &operator = (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_row &assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_row &operator += (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_row &plus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_row &operator -= (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_row &minus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_row &operator *= (const AT &at) {
            vector_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_row &operator /= (const AT &at) {
            vector_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const matrix_row &mr) const {
            return (*this).data_.same_closure (mr.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const matrix_row &mr) const {
            return (*this).data_ == mr.data_ && index () == mr.index ();
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (matrix_row mr) {
            if (this != &mr) {
                BOOST_UBLAS_CHECK (size () == mr.size (), bad_size ());
                // Sparse ranges may be nonconformant now.
                // std::swap_ranges (begin (), end (), mr.begin ());
                vector_swap<scalar_swap> (*this, mr);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (matrix_row mr1, matrix_row mr2) {
            mr1.swap (mr2);
        }

        // Iterator types
    private:
        typedef typename M::const_iterator2 const_subiterator_type;
        typedef typename boost::mpl::if_<boost::is_const<M>,
                                          typename M::const_iterator2,
                                          typename M::iterator2>::type subiterator_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator<matrix_row<matrix_type>,
                                 typename subiterator_type::iterator_category> iterator;
        typedef indexed_const_iterator<matrix_row<matrix_type>,
                                       typename const_subiterator_type::iterator_category> const_iterator;
#else
        class const_iterator;
        class iterator;
#endif

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator find (size_type j) const {
            const_subiterator_type it2 (data_.find2 (1, i_, j));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator (*this, it2.index2 ());
#else
            return const_iterator (*this, it2);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator find (size_type j) {
            subiterator_type it2 (data_.find2 (1, i_, j));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator (*this, it2.index2 ());
#else
            return iterator (*this, it2);
#endif
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator:
            public container_const_reference<matrix_row>,
            public iterator_base_traits<typename const_subiterator_type::iterator_category>::template
                        iterator_base<const_iterator, value_type>::type {
        public:
            typedef typename const_subiterator_type::value_type value_type;
            typedef typename const_subiterator_type::difference_type difference_type;
            typedef typename const_subiterator_type::reference reference;
            typedef typename const_subiterator_type::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator (const self_type &mr, const const_subiterator_type &it):
                container_const_reference<self_type> (mr), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator (const typename self_type::iterator &it):  // ISSUE self_type:: stops VC8 using std::iterator here
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator &operator ++ () {
                ++ it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -- () {
                -- it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator += (difference_type n) {
                it_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -= (difference_type n) {
                it_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
                return *it_;
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return it_.index2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator &operator = (const const_iterator &it) {
                container_const_reference<self_type>::assign (&it ());
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator_type it_;
        };
#endif

        BOOST_UBLAS_INLINE
        const_iterator begin () const {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        const_iterator cbegin () const {
            return begin ();
        }
        BOOST_UBLAS_INLINE
        const_iterator end () const {
            return find (size ());
        }
        BOOST_UBLAS_INLINE
        const_iterator cend () const {
            return end ();
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class iterator:
            public container_reference<matrix_row>,
            public iterator_base_traits<typename subiterator_type::iterator_category>::template
                        iterator_base<iterator, value_type>::type {
        public:
            typedef typename subiterator_type::value_type value_type;
            typedef typename subiterator_type::difference_type difference_type;
            typedef typename subiterator_type::reference reference;
            typedef typename subiterator_type::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator (self_type &mr, const subiterator_type &it):
                container_reference<self_type> (mr), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator &operator ++ () {
                ++ it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -- () {
                -- it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator += (difference_type n) {
                it_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -= (difference_type n) {
                it_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
                return *it_;
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return it_.index2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator &operator = (const iterator &it) {
                container_reference<self_type>::assign (&it ());
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator_type it_;

            friend class const_iterator;
        };
#endif

        BOOST_UBLAS_INLINE
        iterator begin () {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        iterator end () {
            return find (size ());
        }

        // Reverse iterator
        typedef reverse_iterator_base<const_iterator> const_reverse_iterator;
        typedef reverse_iterator_base<iterator> reverse_iterator;

        BOOST_UBLAS_INLINE
        const_reverse_iterator rbegin () const {
            return const_reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crbegin () const {
            return rbegin ();
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator rend () const {
            return const_reverse_iterator (begin ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crend () const {
            return rend ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator rbegin () {
            return reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        reverse_iterator rend () {
            return reverse_iterator (begin ());
        }

    private:
        matrix_closure_type data_;
        size_type i_;
    };

    // Projections
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_row<M> row (M &data, typename M::size_type i) {
        return matrix_row<M> (data, i);
    }
    template<class M>
    BOOST_UBLAS_INLINE
    const matrix_row<const M> row (const M &data, typename M::size_type i) {
        return matrix_row<const M> (data, i);
    }

    // Specialize temporary
    template <class M>
    struct vector_temporary_traits< matrix_row<M> >
    : vector_temporary_traits< M > {} ;
    template <class M>
    struct vector_temporary_traits< const matrix_row<M> >
    : vector_temporary_traits< M > {} ;

    // Matrix based column vector class
    template<class M>
    class matrix_column:
        public vector_expression<matrix_column<M> > {

        typedef matrix_column<M> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using vector_expression<self_type>::operator ();
#endif
        typedef M matrix_type;
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
        typedef typename storage_restrict_traits<typename M::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        matrix_column (matrix_type &data, size_type j):
            data_ (data), j_ (j) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (j_ < data_.size2 (), bad_index ());
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size () const {
            return data_.size1 ();
        }
        BOOST_UBLAS_INLINE
        size_type index () const {
            return j_;
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
        const_reference operator () (size_type i) const {
            return data_ (i, j_);
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) {
            return data_ (i, j_);
        }

        BOOST_UBLAS_INLINE
        const_reference operator [] (size_type i) const {
            return (*this) (i);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) {
            return (*this) (i);
        }
#else
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) const {
            return data_ (i, j_);
        }

        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) const {
            return (*this) (i);
        }
#endif

        // Assignment
        BOOST_UBLAS_INLINE
        matrix_column &operator = (const matrix_column &mc) {
            // ISSUE need a temporary, proxy can be overlaping alias
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (mc));
            return *this;
        }
        BOOST_UBLAS_INLINE
        matrix_column &assign_temporary (matrix_column &mc) {
            // assign elements, proxied container remains the same
            vector_assign<scalar_assign> (*this, mc);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_column &operator = (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_column &assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_column &operator += (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_column &plus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_column &operator -= (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_column &minus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_column &operator *= (const AT &at) {
            vector_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_column &operator /= (const AT &at) {
            vector_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const matrix_column &mc) const {
            return (*this).data_.same_closure (mc.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const matrix_column &mc) const {
            return (*this).data_ == mc.data_ && index () == mc.index ();
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (matrix_column mc) {
            if (this != &mc) {
                BOOST_UBLAS_CHECK (size () == mc.size (), bad_size ());
                // Sparse ranges may be nonconformant now.
                // std::swap_ranges (begin (), end (), mc.begin ());
                vector_swap<scalar_swap> (*this, mc);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (matrix_column mc1, matrix_column mc2) {
            mc1.swap (mc2);
        }

        // Iterator types
    private:
        typedef typename M::const_iterator1 const_subiterator_type;
        typedef typename boost::mpl::if_<boost::is_const<M>,
                                          typename M::const_iterator1,
                                          typename M::iterator1>::type subiterator_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator<matrix_column<matrix_type>,
                                 typename subiterator_type::iterator_category> iterator;
        typedef indexed_const_iterator<matrix_column<matrix_type>,
                                       typename const_subiterator_type::iterator_category> const_iterator;
#else
        class const_iterator;
        class iterator;
#endif

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator find (size_type i) const {
            const_subiterator_type it1 (data_.find1 (1, i, j_));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator (*this, it1.index1 ());
#else
            return const_iterator (*this, it1);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator find (size_type i) {
            subiterator_type it1 (data_.find1 (1, i, j_));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator (*this, it1.index1 ());
#else
            return iterator (*this, it1);
#endif
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator:
            public container_const_reference<matrix_column>,
            public iterator_base_traits<typename const_subiterator_type::iterator_category>::template
                        iterator_base<const_iterator, value_type>::type {
        public:
            typedef typename const_subiterator_type::value_type value_type;
            typedef typename const_subiterator_type::difference_type difference_type;
            typedef typename const_subiterator_type::reference reference;
            typedef typename const_subiterator_type::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator (const self_type &mc, const const_subiterator_type &it):
                container_const_reference<self_type> (mc), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator (const typename self_type::iterator &it):  // ISSUE self_type:: stops VC8 using std::iterator here
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator &operator ++ () {
                ++ it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -- () {
                -- it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator += (difference_type n) {
                it_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -= (difference_type n) {
                it_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
                return *it_;
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return it_.index1 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator &operator = (const const_iterator &it) {
                container_const_reference<self_type>::assign (&it ());
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator_type it_;
        };
#endif

        BOOST_UBLAS_INLINE
        const_iterator begin () const {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        const_iterator cbegin () const {
            return begin ();
        }
        BOOST_UBLAS_INLINE
        const_iterator end () const {
            return find (size ());
        }
        BOOST_UBLAS_INLINE
        const_iterator cend () const {
            return end ();
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class iterator:
            public container_reference<matrix_column>,
            public iterator_base_traits<typename subiterator_type::iterator_category>::template
                        iterator_base<iterator, value_type>::type {
        public:
            typedef typename subiterator_type::value_type value_type;
            typedef typename subiterator_type::difference_type difference_type;
            typedef typename subiterator_type::reference reference;
            typedef typename subiterator_type::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator (self_type &mc, const subiterator_type &it):
                container_reference<self_type> (mc), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator &operator ++ () {
                ++ it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -- () {
                -- it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator += (difference_type n) {
                it_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -= (difference_type n) {
                it_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
                return *it_;
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return it_.index1 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator &operator = (const iterator &it) {
                container_reference<self_type>::assign (&it ());
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator_type it_;

            friend class const_iterator;
        };
#endif

        BOOST_UBLAS_INLINE
        iterator begin () {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        iterator end () {
            return find (size ());
        }

        // Reverse iterator
        typedef reverse_iterator_base<const_iterator> const_reverse_iterator;
        typedef reverse_iterator_base<iterator> reverse_iterator;

        BOOST_UBLAS_INLINE
        const_reverse_iterator rbegin () const {
            return const_reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crbegin () const {
            return rbegin ();
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator rend () const {
            return const_reverse_iterator (begin ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crend () const {
            return rend ();
        }
        reverse_iterator rbegin () {
            return reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        reverse_iterator rend () {
            return reverse_iterator (begin ());
        }

    private:
        matrix_closure_type data_;
        size_type j_;
    };

    // Projections
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_column<M> column (M &data, typename M::size_type j) {
        return matrix_column<M> (data, j);
    }
    template<class M>
    BOOST_UBLAS_INLINE
    const matrix_column<const M> column (const M &data, typename M::size_type j) {
        return matrix_column<const M> (data, j);
    }

    // Specialize temporary
    template <class M>
    struct vector_temporary_traits< matrix_column<M> >
    : vector_temporary_traits< M > {} ;
    template <class M>
    struct vector_temporary_traits< const matrix_column<M> >
    : vector_temporary_traits< M > {} ;

    // Matrix based vector range class
    template<class M>
    class matrix_vector_range:
        public vector_expression<matrix_vector_range<M> > {

        typedef matrix_vector_range<M> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using vector_expression<self_type>::operator ();
#endif
        typedef M matrix_type;
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
        typedef basic_range<size_type, difference_type> range_type;
        typedef const self_type const_closure_type;
        typedef self_type closure_type;
        typedef typename storage_restrict_traits<typename M::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        matrix_vector_range (matrix_type &data, const range_type &r1, const range_type &r2):
            data_ (data), r1_ (r1.preprocess (data.size1 ())), r2_ (r2.preprocess (data.size2 ())) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (r1_.start () <= data_.size1 () &&
            //                     r1_.start () + r1_.size () <= data_.size1 (), bad_index ());
            // BOOST_UBLAS_CHECK (r2_.start () <= data_.size2 () &&
            //                   r2_.start () + r2_.size () <= data_.size2 (), bad_index ());
            // BOOST_UBLAS_CHECK (r1_.size () == r2_.size (), bad_size ());
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type start1 () const {
            return r1_.start ();
        }
        BOOST_UBLAS_INLINE
        size_type start2 () const {
            return r2_.start ();
        }
        BOOST_UBLAS_INLINE
        size_type size () const {
            return BOOST_UBLAS_SAME (r1_.size (), r2_.size ());
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
        const_reference operator () (size_type i) const {
            return data_ (r1_ (i), r2_ (i));
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) {
            return data_ (r1_ (i), r2_ (i));
        }

        BOOST_UBLAS_INLINE
        const_reference operator [] (size_type i) const {
            return (*this) (i);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) {
            return (*this) (i);
        }
#else
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) const {
            return data_ (r1_ (i), r2_ (i));
        }

        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) const {
            return (*this) (i);
        }
#endif

        // Assignment
        BOOST_UBLAS_INLINE
        matrix_vector_range &operator = (const matrix_vector_range &mvr) {
            // ISSUE need a temporary, proxy can be overlaping alias
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (mvr));
            return *this;
        }
        BOOST_UBLAS_INLINE
        matrix_vector_range &assign_temporary (matrix_vector_range &mvr) {
            // assign elements, proxied container remains the same
            vector_assign<scalar_assign> (*this, mvr);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_range &operator = (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_range &assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_range &operator += (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_range &plus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_range &operator -= (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_range &minus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_vector_range &operator *= (const AT &at) {
            vector_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_vector_range &operator /= (const AT &at) {
            vector_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const matrix_vector_range &mvr) const {
            return (*this).data_.same_closure (mvr.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const matrix_vector_range &mvr) const {
            return (*this).data_ == mvr.data_ && r1_ == mvr.r1_ && r2_ == mvr.r2_;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (matrix_vector_range mvr) {
            if (this != &mvr) {
                BOOST_UBLAS_CHECK (size () == mvr.size (), bad_size ());
                // Sparse ranges may be nonconformant now.
                // std::swap_ranges (begin (), end (), mvr.begin ());
                vector_swap<scalar_swap> (*this, mvr);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (matrix_vector_range mvr1, matrix_vector_range mvr2) {
            mvr1.swap (mvr2);
        }

        // Iterator types
    private:
        // Use range as an index - FIXME this fails for packed assignment
        typedef typename range_type::const_iterator const_subiterator1_type;
        typedef typename range_type::const_iterator subiterator1_type;
        typedef typename range_type::const_iterator const_subiterator2_type;
        typedef typename range_type::const_iterator subiterator2_type;

    public:
        class const_iterator;
        class iterator;

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator find (size_type i) const {
            return const_iterator (*this, r1_.begin () + i, r2_.begin () + i);
        }
        BOOST_UBLAS_INLINE
        iterator find (size_type i) {
            return iterator (*this, r1_.begin () + i, r2_.begin () + i);
        }

        class const_iterator:
            public container_const_reference<matrix_vector_range>,
            public iterator_base_traits<typename M::const_iterator1::iterator_category>::template
                        iterator_base<const_iterator, value_type>::type {
        public:
            // FIXME Iterator can never be different code was:
            // typename iterator_restrict_traits<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::iterator_category>
            BOOST_STATIC_ASSERT ((boost::is_same<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::value ));

            typedef typename matrix_vector_range::value_type value_type;
            typedef typename matrix_vector_range::difference_type difference_type;
            typedef typename matrix_vector_range::const_reference reference;
            typedef const typename matrix_vector_range::value_type *pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator ():
                container_const_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator (const self_type &mvr, const const_subiterator1_type &it1, const const_subiterator2_type &it2):
                container_const_reference<self_type> (mvr), it1_ (it1), it2_ (it2) {}
            BOOST_UBLAS_INLINE
            const_iterator (const typename self_type::iterator &it):  // ISSUE self_type:: stops VC8 using std::iterator here
                container_const_reference<self_type> (it ()), it1_ (it.it1_), it2_ (it.it2_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator &operator ++ () {
                ++ it1_;
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -- () {
                -- it1_;
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator += (difference_type n) {
                it1_ += n;
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -= (difference_type n) {
                it1_ -= n;
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return BOOST_UBLAS_SAME (it1_ - it.it1_, it2_ - it.it2_);
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type  index () const {
                return BOOST_UBLAS_SAME (it1_.index (), it2_.index ());
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator &operator = (const const_iterator &it) {
                container_const_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ == it.it1_ && it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ < it.it1_ && it2_ < it.it2_;
            }

        private:
            const_subiterator1_type it1_;
            const_subiterator2_type it2_;
        };

        BOOST_UBLAS_INLINE
        const_iterator begin () const {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        const_iterator cbegin () const {
            return begin ();
        }
        BOOST_UBLAS_INLINE
        const_iterator end () const {
            return find (size ());
        }
        BOOST_UBLAS_INLINE
        const_iterator cend () const {
            return end ();
        }

        class iterator:
            public container_reference<matrix_vector_range>,
            public iterator_base_traits<typename M::iterator1::iterator_category>::template
                        iterator_base<iterator, value_type>::type {
        public:
            // FIXME Iterator can never be different code was:
            // typename iterator_restrict_traits<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::iterator_category>
            BOOST_STATIC_ASSERT ((boost::is_same<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::value ));

            typedef typename matrix_vector_range::value_type value_type;
            typedef typename matrix_vector_range::difference_type difference_type;
            typedef typename matrix_vector_range::reference reference;
            typedef typename matrix_vector_range::value_type *pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator ():
                container_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator (self_type &mvr, const subiterator1_type &it1, const subiterator2_type &it2):
                container_reference<self_type> (mvr), it1_ (it1), it2_ (it2) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator &operator ++ () {
                ++ it1_;
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -- () {
                -- it1_;
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator += (difference_type n) {
                it1_ += n;
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -= (difference_type n) {
                it1_ -= n;
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return BOOST_UBLAS_SAME (it1_ - it.it1_, it2_ - it.it2_);
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return BOOST_UBLAS_SAME (it1_.index (), it2_.index ());
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator &operator = (const iterator &it) {
                container_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ == it.it1_ && it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ < it.it1_ && it2_ < it.it2_;
            }

        private:
            subiterator1_type it1_;
            subiterator2_type it2_;

            friend class const_iterator;
        };

        BOOST_UBLAS_INLINE
        iterator begin () {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        iterator end () {
            return find (size ());
        }

        // Reverse iterator
        typedef reverse_iterator_base<const_iterator> const_reverse_iterator;
        typedef reverse_iterator_base<iterator> reverse_iterator;

        BOOST_UBLAS_INLINE
        const_reverse_iterator rbegin () const {
            return const_reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crbegin () const {
            return rbegin ();
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator rend () const {
            return const_reverse_iterator (begin ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crend () const {
            return rend ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator rbegin () {
            return reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        reverse_iterator rend () {
            return reverse_iterator (begin ());
        }

    private:
        matrix_closure_type data_;
        range_type r1_;
        range_type r2_;
    };

    // Specialize temporary
    template <class M>
    struct vector_temporary_traits< matrix_vector_range<M> >
    : vector_temporary_traits< M > {} ;
    template <class M>
    struct vector_temporary_traits< const matrix_vector_range<M> >
    : vector_temporary_traits< M > {} ;

    // Matrix based vector slice class
    template<class M>
    class matrix_vector_slice:
        public vector_expression<matrix_vector_slice<M> > {

        typedef matrix_vector_slice<M> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using vector_expression<self_type>::operator ();
#endif
        typedef M matrix_type;
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
        typedef basic_range<size_type, difference_type> range_type;
        typedef basic_slice<size_type, difference_type> slice_type;
        typedef const self_type const_closure_type;
        typedef self_type closure_type;
        typedef typename storage_restrict_traits<typename M::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        matrix_vector_slice (matrix_type &data, const slice_type &s1, const slice_type &s2):
            data_ (data), s1_ (s1.preprocess (data.size1 ())), s2_ (s2.preprocess (data.size2 ())) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (s1_.start () <= data_.size1 () &&
            //                    s1_.start () + s1_.stride () * (s1_.size () - (s1_.size () > 0)) <= data_.size1 (), bad_index ());
            // BOOST_UBLAS_CHECK (s2_.start () <= data_.size2 () &&
            //                    s2_.start () + s2_.stride () * (s2_.size () - (s2_.size () > 0)) <= data_.size2 (), bad_index ());
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type start1 () const {
            return s1_.start ();
        }
        BOOST_UBLAS_INLINE
        size_type start2 () const {
            return s2_.start ();
        }
        BOOST_UBLAS_INLINE
        difference_type stride1 () const {
            return s1_.stride ();
        }
        BOOST_UBLAS_INLINE
        difference_type stride2 () const {
            return s2_.stride ();
        }
        BOOST_UBLAS_INLINE
        size_type size () const {
            return BOOST_UBLAS_SAME (s1_.size (), s2_.size ());
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
        const_reference operator () (size_type i) const {
            return data_ (s1_ (i), s2_ (i));
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) {
            return data_ (s1_ (i), s2_ (i));
        }

        BOOST_UBLAS_INLINE
        const_reference operator [] (size_type i) const {
            return (*this) (i);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) {
            return (*this) (i);
        }
#else
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) const {
            return data_ (s1_ (i), s2_ (i));
        }

        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) const {
            return (*this) (i);
        }
#endif

        // Assignment
        BOOST_UBLAS_INLINE
        matrix_vector_slice &operator = (const matrix_vector_slice &mvs) {
            // ISSUE need a temporary, proxy can be overlaping alias
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (mvs));
            return *this;
        }
        BOOST_UBLAS_INLINE
        matrix_vector_slice &assign_temporary (matrix_vector_slice &mvs) {
            // assign elements, proxied container remains the same
            vector_assign<scalar_assign> (*this, mvs);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_slice &operator = (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_slice &assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_slice &operator += (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_slice &plus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_slice &operator -= (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_slice &minus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_vector_slice &operator *= (const AT &at) {
            vector_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_vector_slice &operator /= (const AT &at) {
            vector_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const matrix_vector_slice &mvs) const {
            return (*this).data_.same_closure (mvs.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const matrix_vector_slice &mvs) const {
            return (*this).data_ == mvs.data_ && s1_ == mvs.s1_ && s2_ == mvs.s2_;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (matrix_vector_slice mvs) {
            if (this != &mvs) {
                BOOST_UBLAS_CHECK (size () == mvs.size (), bad_size ());
                // Sparse ranges may be nonconformant now.
                // std::swap_ranges (begin (), end (), mvs.begin ());
                vector_swap<scalar_swap> (*this, mvs);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (matrix_vector_slice mvs1, matrix_vector_slice mvs2) {
            mvs1.swap (mvs2);
        }

        // Iterator types
    private:
        // Use slice as an index - FIXME this fails for packed assignment
        typedef typename slice_type::const_iterator const_subiterator1_type;
        typedef typename slice_type::const_iterator subiterator1_type;
        typedef typename slice_type::const_iterator const_subiterator2_type;
        typedef typename slice_type::const_iterator subiterator2_type;

    public:
        class const_iterator;
        class iterator;

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator find (size_type i) const {
            return const_iterator (*this, s1_.begin () + i, s2_.begin () + i);
        }
        BOOST_UBLAS_INLINE
        iterator find (size_type i) {
            return iterator (*this, s1_.begin () + i, s2_.begin () + i);
        }

        // Iterators simply are indices.

        class const_iterator:
            public container_const_reference<matrix_vector_slice>,
            public iterator_base_traits<typename M::const_iterator1::iterator_category>::template
                        iterator_base<const_iterator, value_type>::type {
        public:
            // FIXME Iterator can never be different code was:
            // typename iterator_restrict_traits<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::iterator_category>
            BOOST_STATIC_ASSERT ((boost::is_same<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::value ));

            typedef typename matrix_vector_slice::value_type value_type;
            typedef typename matrix_vector_slice::difference_type difference_type;
            typedef typename matrix_vector_slice::const_reference reference;
            typedef const typename matrix_vector_slice::value_type *pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator ():
                container_const_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator (const self_type &mvs, const const_subiterator1_type &it1, const const_subiterator2_type &it2):
                container_const_reference<self_type> (mvs), it1_ (it1), it2_ (it2) {}
            BOOST_UBLAS_INLINE
            const_iterator (const typename self_type::iterator &it):  // ISSUE vector:: stops VC8 using std::iterator here
                container_const_reference<self_type> (it ()), it1_ (it.it1_), it2_ (it.it2_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator &operator ++ () {
                ++ it1_;
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -- () {
                -- it1_;
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator += (difference_type n) {
                it1_ += n;
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -= (difference_type n) {
                it1_ -= n;
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return BOOST_UBLAS_SAME (it1_ - it.it1_, it2_ - it.it2_);
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type  index () const {
                return BOOST_UBLAS_SAME (it1_.index (), it2_.index ());
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator &operator = (const const_iterator &it) {
                container_const_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ == it.it1_ && it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ < it.it1_ && it2_ < it.it2_;
            }

        private:
            const_subiterator1_type it1_;
            const_subiterator2_type it2_;
        };

        BOOST_UBLAS_INLINE
        const_iterator begin () const {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        const_iterator cbegin () const {
            return begin ();
        }
        BOOST_UBLAS_INLINE
        const_iterator end () const {
            return find (size ());
        }
        BOOST_UBLAS_INLINE
        const_iterator cend () const {
            return end ();
        }

        class iterator:
            public container_reference<matrix_vector_slice>,
            public iterator_base_traits<typename M::iterator1::iterator_category>::template
                        iterator_base<iterator, value_type>::type {
        public:
            // FIXME Iterator can never be different code was:
            // typename iterator_restrict_traits<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::iterator_category>
            BOOST_STATIC_ASSERT ((boost::is_same<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::value ));

            typedef typename matrix_vector_slice::value_type value_type;
            typedef typename matrix_vector_slice::difference_type difference_type;
            typedef typename matrix_vector_slice::reference reference;
            typedef typename matrix_vector_slice::value_type *pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator ():
                container_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator (self_type &mvs, const subiterator1_type &it1, const subiterator2_type &it2):
                container_reference<self_type> (mvs), it1_ (it1), it2_ (it2) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator &operator ++ () {
                ++ it1_;
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -- () {
                -- it1_;
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator += (difference_type n) {
                it1_ += n;
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -= (difference_type n) {
                it1_ -= n;
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return BOOST_UBLAS_SAME (it1_ - it.it1_, it2_ - it.it2_);
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return BOOST_UBLAS_SAME (it1_.index (), it2_.index ());
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator &operator = (const iterator &it) {
                container_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ == it.it1_ && it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ < it.it1_ && it2_ < it.it2_;
            }

        private:
            subiterator1_type it1_;
            subiterator2_type it2_;

            friend class const_iterator;
        };

        BOOST_UBLAS_INLINE
        iterator begin () {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        iterator end () {
            return find (size ());
        }

        // Reverse iterator
        typedef reverse_iterator_base<const_iterator> const_reverse_iterator;
        typedef reverse_iterator_base<iterator> reverse_iterator;

        BOOST_UBLAS_INLINE
        const_reverse_iterator rbegin () const {
            return const_reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crbegin () const {
            return rbegin ();
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator rend () const {
            return const_reverse_iterator (begin ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crend () const {
            return rend ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator rbegin () {
            return reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        reverse_iterator rend () {
            return reverse_iterator (begin ());
        }

    private:
        matrix_closure_type data_;
        slice_type s1_;
        slice_type s2_;
    };

    // Specialize temporary
    template <class M>
    struct vector_temporary_traits< matrix_vector_slice<M> >
    : vector_temporary_traits< M > {} ;
    template <class M>
    struct vector_temporary_traits< const matrix_vector_slice<M> >
    : vector_temporary_traits< M > {} ;

    // Matrix based vector indirection class

    template<class M, class IA>
    class matrix_vector_indirect:
        public vector_expression<matrix_vector_indirect<M, IA> > {

        typedef matrix_vector_indirect<M, IA> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using vector_expression<self_type>::operator ();
#endif
        typedef M matrix_type;
        typedef IA indirect_array_type;
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
        typedef typename storage_restrict_traits<typename M::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        matrix_vector_indirect (matrix_type &data, size_type size):
            data_ (data), ia1_ (size), ia2_ (size) {}
        BOOST_UBLAS_INLINE
        matrix_vector_indirect (matrix_type &data, const indirect_array_type &ia1, const indirect_array_type &ia2):
            data_ (data), ia1_ (ia1), ia2_ (ia2) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (ia1_.size () == ia2_.size (), bad_size ());
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size () const {
            return BOOST_UBLAS_SAME (ia1_.size (), ia2_.size ());
        }
        BOOST_UBLAS_INLINE
        const indirect_array_type &indirect1 () const {
            return ia1_;
        }
        BOOST_UBLAS_INLINE
        indirect_array_type &indirect1 () {
            return ia1_;
        }
        BOOST_UBLAS_INLINE
        const indirect_array_type &indirect2 () const {
            return ia2_;
        }
        BOOST_UBLAS_INLINE
        indirect_array_type &indirect2 () {
            return ia2_;
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
        const_reference operator () (size_type i) const {
            return data_ (ia1_ (i), ia2_ (i));
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) {
            return data_ (ia1_ (i), ia2_ (i));
        }

        BOOST_UBLAS_INLINE
        const_reference operator [] (size_type i) const {
            return (*this) (i);
        }
        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) {
            return (*this) (i);
        }
#else
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) const {
            return data_ (ia1_ (i), ia2_ (i));
        }

        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) const {
            return (*this) (i);
        }
#endif

        // Assignment
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &operator = (const matrix_vector_indirect &mvi) {
            // ISSUE need a temporary, proxy can be overlaping alias
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (mvi));
            return *this;
        }
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &assign_temporary (matrix_vector_indirect &mvi) {
            // assign elements, proxied container remains the same
            vector_assign<scalar_assign> (*this, mvi);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &operator = (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &operator += (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &plus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &operator -= (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<M>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &minus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &operator *= (const AT &at) {
            vector_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_vector_indirect &operator /= (const AT &at) {
            vector_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const matrix_vector_indirect &mvi) const {
            return (*this).data_.same_closure (mvi.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const matrix_vector_indirect &mvi) const {
            return (*this).data_ == mvi.data_ && ia1_ == mvi.ia1_ && ia2_ == mvi.ia2_;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (matrix_vector_indirect mvi) {
            if (this != &mvi) {
                BOOST_UBLAS_CHECK (size () == mvi.size (), bad_size ());
                // Sparse ranges may be nonconformant now.
                // std::swap_ranges (begin (), end (), mvi.begin ());
                vector_swap<scalar_swap> (*this, mvi);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (matrix_vector_indirect mvi1, matrix_vector_indirect mvi2) {
            mvi1.swap (mvi2);
        }

        // Iterator types
    private:
        // Use indirect array as an index - FIXME this fails for packed assignment
        typedef typename IA::const_iterator const_subiterator1_type;
        typedef typename IA::const_iterator subiterator1_type;
        typedef typename IA::const_iterator const_subiterator2_type;
        typedef typename IA::const_iterator subiterator2_type;

    public:
        class const_iterator;
        class iterator;

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator find (size_type i) const {
            return const_iterator (*this, ia1_.begin () + i, ia2_.begin () + i);
        }
        BOOST_UBLAS_INLINE
        iterator find (size_type i) {
            return iterator (*this, ia1_.begin () + i, ia2_.begin () + i);
        }

        // Iterators simply are indices.

        class const_iterator:
            public container_const_reference<matrix_vector_indirect>,
            public iterator_base_traits<typename M::const_iterator1::iterator_category>::template
                        iterator_base<const_iterator, value_type>::type {
        public:
            // FIXME Iterator can never be different code was:
            // typename iterator_restrict_traits<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::iterator_category>
            BOOST_STATIC_ASSERT ((boost::is_same<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::value ));

            typedef typename matrix_vector_indirect::value_type value_type;
            typedef typename matrix_vector_indirect::difference_type difference_type;
            typedef typename matrix_vector_indirect::const_reference reference;
            typedef const typename matrix_vector_indirect::value_type *pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator ():
                container_const_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator (const self_type &mvi, const const_subiterator1_type &it1, const const_subiterator2_type &it2):
                container_const_reference<self_type> (mvi), it1_ (it1), it2_ (it2) {}
            BOOST_UBLAS_INLINE
            const_iterator (const typename self_type::iterator &it):  // ISSUE self_type:: stops VC8 using std::iterator here
                container_const_reference<self_type> (it ()), it1_ (it.it1_), it2_ (it.it2_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator &operator ++ () {
                ++ it1_;
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -- () {
                -- it1_;
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator += (difference_type n) {
                it1_ += n;
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator &operator -= (difference_type n) {
                it1_ -= n;
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return BOOST_UBLAS_SAME (it1_ - it.it1_, it2_ - it.it2_);
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type  index () const {
                return BOOST_UBLAS_SAME (it1_.index (), it2_.index ());
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator &operator = (const const_iterator &it) {
                container_const_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ == it.it1_ && it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ < it.it1_ && it2_ < it.it2_;
            }

        private:
            const_subiterator1_type it1_;
            const_subiterator2_type it2_;
        };

        BOOST_UBLAS_INLINE
        const_iterator begin () const {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        const_iterator cbegin () const {
            return begin ();
        }
        BOOST_UBLAS_INLINE
        const_iterator end () const {
            return find (size ());
        }
        BOOST_UBLAS_INLINE
        const_iterator cend () const {
            return end ();
        }

        class iterator:
            public container_reference<matrix_vector_indirect>,
            public iterator_base_traits<typename M::iterator1::iterator_category>::template
                        iterator_base<iterator, value_type>::type {
        public:
            // FIXME Iterator can never be different code was:
            // typename iterator_restrict_traits<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::iterator_category>
            BOOST_STATIC_ASSERT ((boost::is_same<typename M::const_iterator1::iterator_category, typename M::const_iterator2::iterator_category>::value ));

            typedef typename matrix_vector_indirect::value_type value_type;
            typedef typename matrix_vector_indirect::difference_type difference_type;
            typedef typename matrix_vector_indirect::reference reference;
            typedef typename matrix_vector_indirect::value_type *pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator ():
                container_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator (self_type &mvi, const subiterator1_type &it1, const subiterator2_type &it2):
                container_reference<self_type> (mvi), it1_ (it1), it2_ (it2) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator &operator ++ () {
                ++ it1_;
                ++ it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -- () {
                -- it1_;
                -- it2_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator += (difference_type n) {
                it1_ += n;
                it2_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator &operator -= (difference_type n) {
                it1_ -= n;
                it2_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return BOOST_UBLAS_SAME (it1_ - it.it1_, it2_ - it.it2_);
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return BOOST_UBLAS_SAME (it1_.index (), it2_.index ());
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator &operator = (const iterator &it) {
                container_reference<self_type>::assign (&it ());
                it1_ = it.it1_;
                it2_ = it.it2_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ == it.it1_ && it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it1_ < it.it1_ && it2_ < it.it2_;
            }

        private:
            subiterator1_type it1_;
            subiterator2_type it2_;

            friend class const_iterator;
        };

        BOOST_UBLAS_INLINE
        iterator begin () {
            return find (0);
        }
        BOOST_UBLAS_INLINE
        iterator end () {
            return find (size ());
        }

        // Reverse iterator
        typedef reverse_iterator_base<const_iterator> const_reverse_iterator;
        typedef reverse_iterator_base<iterator> reverse_iterator;

        BOOST_UBLAS_INLINE
        const_reverse_iterator rbegin () const {
            return const_reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crbegin () const {
            return rbegin ();
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator rend () const {
            return const_reverse_iterator (begin ());
        }
        BOOST_UBLAS_INLINE
        const_reverse_iterator crend () const {
            return rend ();
        }
        BOOST_UBLAS_INLINE
        reverse_iterator rbegin () {
            return reverse_iterator (end ());
        }
        BOOST_UBLAS_INLINE
        reverse_iterator rend () {
            return reverse_iterator (begin ());
        }

    private:
        matrix_closure_type data_;
        indirect_array_type ia1_;
        indirect_array_type ia2_;
    };

    // Specialize temporary
    template <class M, class IA>
    struct vector_temporary_traits< matrix_vector_indirect<M,IA> >
    : vector_temporary_traits< M > {} ;
    template <class M, class IA>
    struct vector_temporary_traits< const matrix_vector_indirect<M,IA> >
    : vector_temporary_traits< M > {} ;

    // Matrix based range class
    template<class M>
    class matrix_range:
        public matrix_expression<matrix_range<M> > {

        typedef matrix_range<M> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_expression<self_type>::operator ();
#endif
        typedef M matrix_type;
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
        typedef basic_range<size_type, difference_type> range_type;
        typedef const self_type const_closure_type;
        typedef self_type closure_type;
        typedef typename storage_restrict_traits<typename M::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;
        typedef typename M::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        matrix_range (matrix_type &data, const range_type &r1, const range_type &r2):
            data_ (data), r1_ (r1.preprocess (data.size1 ())), r2_ (r2.preprocess (data.size2 ())) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (r1_.start () <= data_.size1 () &&
            //                    r1_.start () + r1_.size () <= data_.size1 (), bad_index ());
            // BOOST_UBLAS_CHECK (r2_.start () <= data_.size2 () &&
            //                    r2_.start () + r2_.size () <= data_.size2 (), bad_index ());
        }
        BOOST_UBLAS_INLINE
        matrix_range (const matrix_closure_type &data, const range_type &r1, const range_type &r2, int):
            data_ (data), r1_ (r1.preprocess (data.size1 ())), r2_ (r2.preprocess (data.size2 ())) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (r1_.start () <= data_.size1 () &&
            //                    r1_.start () + r1_.size () <= data_.size1 (), bad_index ());
            // BOOST_UBLAS_CHECK (r2_.start () <= data_.size2 () &&
            //                    r2_.start () + r2_.size () <= data_.size2 (), bad_index ());
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type start1 () const {
            return r1_.start ();
        }
        BOOST_UBLAS_INLINE
        size_type size1 () const {
            return r1_.size ();
        }
        BOOST_UBLAS_INLINE
        size_type start2() const {
            return r2_.start ();
        }
        BOOST_UBLAS_INLINE
        size_type size2 () const {
            return r2_.size ();
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
            return data_ (r1_ (i), r2_ (j));
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
            return data_ (r1_ (i), r2_ (j));
        }
#else
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) const {
            return data_ (r1_ (i), r2_ (j));
        }
#endif

        // ISSUE can this be done in free project function?
        // Although a const function can create a non-const proxy to a non-const object
        // Critical is that matrix_type and data_ (vector_closure_type) are const correct
        BOOST_UBLAS_INLINE
        matrix_range<matrix_type> project (const range_type &r1, const range_type &r2) const {
            return matrix_range<matrix_type>  (data_, r1_.compose (r1.preprocess (data_.size1 ())), r2_.compose (r2.preprocess (data_.size2 ())), 0);
        }

        // Assignment
        BOOST_UBLAS_INLINE
        matrix_range &operator = (const matrix_range &mr) {
            matrix_assign<scalar_assign> (*this, mr);
            return *this;
        }
        BOOST_UBLAS_INLINE
        matrix_range &assign_temporary (matrix_range &mr) {
            return *this = mr;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_range &operator = (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, typename matrix_temporary_traits<M>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_range &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_range& operator += (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, typename matrix_temporary_traits<M>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_range &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_range& operator -= (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, typename matrix_temporary_traits<M>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_range &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_range& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_range& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const matrix_range &mr) const {
            return (*this).data_.same_closure (mr.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const matrix_range &mr) const {
            return (*this).data_ == (mr.data_) && r1_ == mr.r1_ && r2_ == mr.r2_;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (matrix_range mr) {
            if (this != &mr) {
                BOOST_UBLAS_CHECK (size1 () == mr.size1 (), bad_size ());
                BOOST_UBLAS_CHECK (size2 () == mr.size2 (), bad_size ());
                matrix_swap<scalar_swap> (*this, mr);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (matrix_range mr1, matrix_range mr2) {
            mr1.swap (mr2);
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
        typedef indexed_iterator1<matrix_range<matrix_type>,
                                  typename subiterator1_type::iterator_category> iterator1;
        typedef indexed_iterator2<matrix_range<matrix_type>,
                                  typename subiterator2_type::iterator_category> iterator2;
        typedef indexed_const_iterator1<matrix_range<matrix_type>,
                                        typename const_subiterator1_type::iterator_category> const_iterator1;
        typedef indexed_const_iterator2<matrix_range<matrix_type>,
                                        typename const_subiterator2_type::iterator_category> const_iterator2;
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
            const_subiterator1_type it1 (data_.find1 (rank, start1 () + i, start2 () + j));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator1 (*this, it1.index1 (), it1.index2 ());
#else
            return const_iterator1 (*this, it1);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator1 find1 (int rank, size_type i, size_type j) {
            subiterator1_type it1 (data_.find1 (rank, start1 () + i, start2 () + j));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator1 (*this, it1.index1 (), it1.index2 ());
#else
            return iterator1 (*this, it1);
#endif
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int rank, size_type i, size_type j) const {
            const_subiterator2_type it2 (data_.find2 (rank, start1 () + i, start2 () + j));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator2 (*this, it2.index1 (), it2.index2 ());
#else
            return const_iterator2 (*this, it2);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator2 find2 (int rank, size_type i, size_type j) {
            subiterator2_type it2 (data_.find2 (rank, start1 () + i, start2 () + j));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator2 (*this, it2.index1 (), it2.index2 ());
#else
            return iterator2 (*this, it2);
#endif
        }


#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<matrix_range>,
            public iterator_base_traits<typename const_subiterator1_type::iterator_category>::template
                        iterator_base<const_iterator1, value_type>::type {
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
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &mr, const const_subiterator1_type &it):
                container_const_reference<self_type> (mr), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                ++ it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                -- it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator += (difference_type n) {
                it_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -= (difference_type n) {
                it_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                return *it_;
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
                const self_type &mr = (*this) ();
                return mr.find2 (1, index1 (), 0);
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
                const self_type &mr = (*this) ();
                return mr.find2 (1, index1 (), mr.size2 ());
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
                return it_.index1 () - (*this) ().start1 ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it_.index2 () - (*this) ().start2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator1 &operator = (const const_iterator1 &it) {
                container_const_reference<self_type>::assign (&it ());
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator1_type it_;
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
            public container_reference<matrix_range>,
            public iterator_base_traits<typename subiterator1_type::iterator_category>::template
                        iterator_base<iterator1, value_type>::type {
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
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &mr, const subiterator1_type &it):
                container_reference<self_type> (mr), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                ++ it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                -- it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator += (difference_type n) {
                it_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -= (difference_type n) {
                it_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                return *it_;
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
                self_type &mr = (*this) ();
                return mr.find2 (1, index1 (), 0);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 end () const {
                self_type &mr = (*this) ();
                return mr.find2 (1, index1 (), mr.size2 ());
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
                return it_.index1 () - (*this) ().start1 ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it_.index2 () - (*this) ().start2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator1 &operator = (const iterator1 &it) {
                container_reference<self_type>::assign (&it ());
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator1_type it_;

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
            public container_const_reference<matrix_range>,
            public iterator_base_traits<typename const_subiterator2_type::iterator_category>::template
                        iterator_base<const_iterator2, value_type>::type {
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
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &mr, const const_subiterator2_type &it):
                container_const_reference<self_type> (mr), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                ++ it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                -- it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator += (difference_type n) {
                it_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -= (difference_type n) {
                it_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                return *it_;
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
                const self_type &mr = (*this) ();
                return mr.find1 (1, 0, index2 ());
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
                const self_type &mr = (*this) ();
                return mr.find1 (1, mr.size1 (), index2 ());
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
                return it_.index1 () - (*this) ().start1 ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it_.index2 () - (*this) ().start2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator2 &operator = (const const_iterator2 &it) {
                container_const_reference<self_type>::assign (&it ());
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator2_type it_;
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
            public container_reference<matrix_range>,
            public iterator_base_traits<typename subiterator2_type::iterator_category>::template
                        iterator_base<iterator2, value_type>::type {
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
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &mr, const subiterator2_type &it):
                container_reference<self_type> (mr), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                ++ it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                -- it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator += (difference_type n) {
                it_ += n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -= (difference_type n) {
                it_ -= n;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator2 &it) const {
               BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                return *it_;
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
                self_type &mr = (*this) ();
                return mr.find1 (1, 0, index2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 end () const {
                self_type &mr = (*this) ();
                return mr.find1 (1, mr.size1 (), index2 ());
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
                return it_.index1 () - (*this) ().start1 ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it_.index2 () - (*this) ().start2 ();
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator2 &operator = (const iterator2 &it) {
                container_reference<self_type>::assign (&it ());
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator2 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator2 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator2_type it_;

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
        range_type r1_;
        range_type r2_;
    };

    // Simple Projections
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_range<M> subrange (M &data, typename M::size_type start1, typename M::size_type stop1, typename M::size_type start2, typename M::size_type stop2) {
        typedef basic_range<typename M::size_type, typename M::difference_type> range_type;
        return matrix_range<M> (data, range_type (start1, stop1), range_type (start2, stop2));
    }
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_range<const M> subrange (const M &data, typename M::size_type start1, typename M::size_type stop1, typename M::size_type start2, typename M::size_type stop2) {
        typedef basic_range<typename M::size_type, typename M::difference_type> range_type;
        return matrix_range<const M> (data, range_type (start1, stop1), range_type (start2, stop2));
    }

    // Generic Projections
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_range<M> project (M &data, const typename matrix_range<M>::range_type &r1, const typename matrix_range<M>::range_type &r2) {
        return matrix_range<M> (data, r1, r2);
    }
    template<class M>
    BOOST_UBLAS_INLINE
    const matrix_range<const M> project (const M &data, const typename matrix_range<M>::range_type &r1, const typename matrix_range<M>::range_type &r2) {
        // ISSUE was: return matrix_range<M> (const_cast<M &> (data), r1, r2);
        return matrix_range<const M> (data, r1, r2);
    }
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_range<M> project (matrix_range<M> &data, const typename matrix_range<M>::range_type &r1, const typename matrix_range<M>::range_type &r2) {
        return data.project (r1, r2);
    }
    template<class M>
    BOOST_UBLAS_INLINE
    const matrix_range<M> project (const matrix_range<M> &data, const typename matrix_range<M>::range_type &r1, const typename matrix_range<M>::range_type &r2) {
        return data.project (r1, r2);
    }

    // Specialization of temporary_traits
    template <class M>
    struct matrix_temporary_traits< matrix_range<M> >
    : matrix_temporary_traits< M > {} ;
    template <class M>
    struct matrix_temporary_traits< const matrix_range<M> >
    : matrix_temporary_traits< M > {} ;

    template <class M>
    struct vector_temporary_traits< matrix_range<M> >
    : vector_temporary_traits< M > {} ;
    template <class M>
    struct vector_temporary_traits< const matrix_range<M> >
    : vector_temporary_traits< M > {} ;

    // Matrix based slice class
    template<class M>
    class matrix_slice:
        public matrix_expression<matrix_slice<M> > {

        typedef matrix_slice<M> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_expression<self_type>::operator ();
#endif
        typedef M matrix_type;
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
        typedef basic_range<size_type, difference_type> range_type;
        typedef basic_slice<size_type, difference_type> slice_type;
        typedef const self_type const_closure_type;
        typedef self_type closure_type;
        typedef typename storage_restrict_traits<typename M::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;
        typedef typename M::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        matrix_slice (matrix_type &data, const slice_type &s1, const slice_type &s2):
            data_ (data), s1_ (s1.preprocess (data.size1 ())), s2_ (s2.preprocess (data.size2 ())) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (s1_.start () <= data_.size1 () &&
            //                    s1_.start () + s1_.stride () * (s1_.size () - (s1_.size () > 0)) <= data_.size1 (), bad_index ());
            // BOOST_UBLAS_CHECK (s2_.start () <= data_.size2 () &&
            //                    s2_.start () + s2_.stride () * (s2_.size () - (s2_.size () > 0)) <= data_.size2 (), bad_index ());
        }
        BOOST_UBLAS_INLINE
        matrix_slice (const matrix_closure_type &data, const slice_type &s1, const slice_type &s2, int):
            data_ (data), s1_ (s1.preprocess (data.size1 ())), s2_ (s2.preprocess (data.size2 ())) {
            // Early checking of preconditions.
            // BOOST_UBLAS_CHECK (s1_.start () <= data_.size1 () &&
            //                    s1_.start () + s1_.stride () * (s1_.size () - (s1_.size () > 0)) <= data_.size1 (), bad_index ());
            // BOOST_UBLAS_CHECK (s2_.start () <= data_.size2 () &&
            //                    s2_.start () + s2_.stride () * (s2_.size () - (s2_.size () > 0)) <= data_.size2 (), bad_index ());
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type start1 () const {
            return s1_.start ();
        }
        BOOST_UBLAS_INLINE
        size_type start2 () const {
            return s2_.start ();
        }
        BOOST_UBLAS_INLINE
        difference_type stride1 () const {
            return s1_.stride ();
        }
        BOOST_UBLAS_INLINE
        difference_type stride2 () const {
            return s2_.stride ();
        }
        BOOST_UBLAS_INLINE
        size_type size1 () const {
            return s1_.size ();
        }
        BOOST_UBLAS_INLINE
        size_type size2 () const {
            return s2_.size ();
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
            return data_ (s1_ (i), s2_ (j));
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
            return data_ (s1_ (i), s2_ (j));
        }
#else
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) const {
            return data_ (s1_ (i), s2_ (j));
        }
#endif

        // ISSUE can this be done in free project function?
        // Although a const function can create a non-const proxy to a non-const object
        // Critical is that matrix_type and data_ (vector_closure_type) are const correct
        BOOST_UBLAS_INLINE
        matrix_slice<matrix_type> project (const range_type &r1, const range_type &r2) const {
            return matrix_slice<matrix_type>  (data_, s1_.compose (r1.preprocess (data_.size1 ())), s2_.compose (r2.preprocess (data_.size2 ())), 0);
        }
        BOOST_UBLAS_INLINE
        matrix_slice<matrix_type> project (const slice_type &s1, const slice_type &s2) const {
            return matrix_slice<matrix_type>  (data_, s1_.compose (s1.preprocess (data_.size1 ())), s2_.compose (s2.preprocess (data_.size2 ())), 0);
        }

        // Assignment
        BOOST_UBLAS_INLINE
        matrix_slice &operator = (const matrix_slice &ms) {
            matrix_assign<scalar_assign> (*this, ms);
            return *this;
        }
        BOOST_UBLAS_INLINE
        matrix_slice &assign_temporary (matrix_slice &ms) {
            return *this = ms;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_slice &operator = (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, typename matrix_temporary_traits<M>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_slice &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_slice& operator += (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, typename matrix_temporary_traits<M>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_slice &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_slice& operator -= (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, typename matrix_temporary_traits<M>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_slice &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_slice& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_slice& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const matrix_slice &ms) const {
            return (*this).data_.same_closure (ms.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const matrix_slice &ms) const {
            return (*this).data_ == ms.data_ && s1_ == ms.s1_ && s2_ == ms.s2_;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (matrix_slice ms) {
            if (this != &ms) {
                BOOST_UBLAS_CHECK (size1 () == ms.size1 (), bad_size ());
                BOOST_UBLAS_CHECK (size2 () == ms.size2 (), bad_size ());
                matrix_swap<scalar_swap> (*this, ms);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (matrix_slice ms1, matrix_slice ms2) {
            ms1.swap (ms2);
        }

        // Iterator types
    private:
        // Use slice as an index - FIXME this fails for packed assignment
        typedef typename slice_type::const_iterator const_subiterator1_type;
        typedef typename slice_type::const_iterator subiterator1_type;
        typedef typename slice_type::const_iterator const_subiterator2_type;
        typedef typename slice_type::const_iterator subiterator2_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator1<matrix_slice<matrix_type>,
                                  typename matrix_type::iterator1::iterator_category> iterator1;
        typedef indexed_iterator2<matrix_slice<matrix_type>,
                                  typename matrix_type::iterator2::iterator_category> iterator2;
        typedef indexed_const_iterator1<matrix_slice<matrix_type>,
                                        typename matrix_type::const_iterator1::iterator_category> const_iterator1;
        typedef indexed_const_iterator2<matrix_slice<matrix_type>,
                                        typename matrix_type::const_iterator2::iterator_category> const_iterator2;
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
        const_iterator1 find1 (int /* rank */, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator1 (*this, i, j);
#else
            return const_iterator1 (*this, s1_.begin () + i, s2_.begin () + j);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator1 find1 (int /* rank */, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator1 (*this, i, j);
#else
            return iterator1 (*this, s1_.begin () + i, s2_.begin () + j);
#endif
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int /* rank */, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator2 (*this, i, j);
#else
            return const_iterator2 (*this, s1_.begin () + i, s2_.begin () + j);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator2 find2 (int /* rank */, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator2 (*this, i, j);
#else
            return iterator2 (*this, s1_.begin () + i, s2_.begin () + j);
#endif
        }

        // Iterators simply are indices.

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<matrix_slice>,
            public iterator_base_traits<typename M::const_iterator1::iterator_category>::template
                        iterator_base<const_iterator1, value_type>::type {
        public:
            typedef typename M::const_iterator1::value_type value_type;
            typedef typename M::const_iterator1::difference_type difference_type;
            typedef typename M::const_reference reference;    //FIXME due to indexing access
            typedef typename M::const_iterator1::pointer pointer;
            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &ms, const const_subiterator1_type &it1, const const_subiterator2_type &it2):
                container_const_reference<self_type> (ms), it1_ (it1), it2_ (it2) {}
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ - it.it1_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
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
                return const_iterator2 ((*this) (), it1_, it2_ ().begin ());
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
                return const_iterator2 ((*this) (), it1_, it2_ ().end ());
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
                return it1_.index ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index ();
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ == it.it1_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ < it.it1_;
            }

        private:
            const_subiterator1_type it1_;
            const_subiterator2_type it2_;
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
            public container_reference<matrix_slice>,
            public iterator_base_traits<typename M::iterator1::iterator_category>::template
                        iterator_base<iterator1, value_type>::type {
        public:
            typedef typename M::iterator1::value_type value_type;
            typedef typename M::iterator1::difference_type difference_type;
            typedef typename M::reference reference;    //FIXME due to indexing access
            typedef typename M::iterator1::pointer pointer;
            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &ms, const subiterator1_type &it1, const subiterator2_type &it2):
                container_reference<self_type> (ms), it1_ (it1), it2_ (it2) {}

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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ - it.it1_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
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
                return iterator2 ((*this) (), it1_, it2_ ().begin ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 end () const {
                return iterator2 ((*this) (), it1_, it2_ ().end ());
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
                return it1_.index ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index ();
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ == it.it1_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ < it.it1_;
            }

        private:
            subiterator1_type it1_;
            subiterator2_type it2_;

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
            public container_const_reference<matrix_slice>,
            public iterator_base_traits<typename M::const_iterator2::iterator_category>::template
                        iterator_base<const_iterator2, value_type>::type {
        public:
            typedef typename M::const_iterator2::value_type value_type;
            typedef typename M::const_iterator2::difference_type difference_type;
            typedef typename M::const_reference reference;    //FIXME due to indexing access
            typedef typename M::const_iterator2::pointer pointer;
            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &ms, const const_subiterator1_type &it1, const const_subiterator2_type &it2):
                container_const_reference<self_type> (ms), it1_ (it1), it2_ (it2) {}
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ - it.it2_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
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
                return const_iterator1 ((*this) (), it1_ ().begin (), it2_);
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
                return const_iterator1 ((*this) (), it1_ ().end (), it2_);
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
                return it1_.index ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index ();
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ < it.it2_;
            }

        private:
            const_subiterator1_type it1_;
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
            public container_reference<matrix_slice>,
            public iterator_base_traits<typename M::iterator2::iterator_category>::template
                        iterator_base<iterator2, value_type>::type {
        public:
            typedef typename M::iterator2::value_type value_type;
            typedef typename M::iterator2::difference_type difference_type;
            typedef typename M::reference reference;    //FIXME due to indexing access
            typedef typename M::iterator2::pointer pointer;
            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &ms, const subiterator1_type &it1, const subiterator2_type &it2):
                container_reference<self_type> (ms), it1_ (it1), it2_ (it2) {}

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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ - it.it2_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
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
                return iterator1 ((*this) (), it1_ ().begin (), it2_);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 end () const {
                return iterator1 ((*this) (), it1_ ().end (), it2_);
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
                return it1_.index ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index ();
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator2 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ < it.it2_;
            }

        private:
            subiterator1_type it1_;
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
        slice_type s1_;
        slice_type s2_;
    };

    // Simple Projections
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_slice<M> subslice (M &data, typename M::size_type start1, typename M::difference_type stride1, typename M::size_type size1, typename M::size_type start2, typename M::difference_type stride2, typename M::size_type size2) {
        typedef basic_slice<typename M::size_type, typename M::difference_type> slice_type;
        return matrix_slice<M> (data, slice_type (start1, stride1, size1), slice_type (start2, stride2, size2));
    }
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_slice<const M> subslice (const M &data, typename M::size_type start1, typename M::difference_type stride1, typename M::size_type size1, typename M::size_type start2, typename M::difference_type stride2, typename M::size_type size2) {
        typedef basic_slice<typename M::size_type, typename M::difference_type> slice_type;
        return matrix_slice<const M> (data, slice_type (start1, stride1, size1), slice_type (start2, stride2, size2));
    }

    // Generic Projections
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_slice<M> project (M &data, const typename matrix_slice<M>::slice_type &s1, const typename matrix_slice<M>::slice_type &s2) {
        return matrix_slice<M> (data, s1, s2);
    }
    template<class M>
    BOOST_UBLAS_INLINE
    const matrix_slice<const M> project (const M &data, const typename matrix_slice<M>::slice_type &s1, const typename matrix_slice<M>::slice_type &s2) {
        // ISSUE was: return matrix_slice<M> (const_cast<M &> (data), s1, s2);
        return matrix_slice<const M> (data, s1, s2);
    }
    // ISSUE in the following two functions it would be logical to use matrix_slice<V>::range_type but this confuses VC7.1 and 8.0
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_slice<M> project (matrix_slice<M> &data, const typename matrix_range<M>::range_type &r1, const typename matrix_range<M>::range_type &r2) {
        return data.project (r1, r2);
    }
    template<class M>
    BOOST_UBLAS_INLINE
    const matrix_slice<M> project (const matrix_slice<M> &data, const typename matrix_range<M>::range_type &r1, const typename matrix_range<M>::range_type &r2) {
        return data.project (r1, r2);
    }
    template<class M>
    BOOST_UBLAS_INLINE
    matrix_slice<M> project (matrix_slice<M> &data, const typename matrix_slice<M>::slice_type &s1, const typename matrix_slice<M>::slice_type &s2) {
        return data.project (s1, s2);
    }
    template<class M>
    BOOST_UBLAS_INLINE
    const matrix_slice<M> project (const matrix_slice<M> &data, const typename matrix_slice<M>::slice_type &s1, const typename matrix_slice<M>::slice_type &s2) {
        return data.project (s1, s2);
    }

    // Specialization of temporary_traits
    template <class M>
    struct matrix_temporary_traits< matrix_slice<M> >
    : matrix_temporary_traits< M > {};
    template <class M>
    struct matrix_temporary_traits< const matrix_slice<M> >
    : matrix_temporary_traits< M > {};

    template <class M>
    struct vector_temporary_traits< matrix_slice<M> >
    : vector_temporary_traits< M > {};
    template <class M>
    struct vector_temporary_traits< const matrix_slice<M> >
    : vector_temporary_traits< M > {};

    // Matrix based indirection class
    // Contributed by Toon Knapen.
    // Extended and optimized by Kresimir Fresl.
    /** \brief A matrix referencing a non continuous submatrix of elements given another matrix of indices.
     *
     * It is the most general version of any submatrices because it uses another matrix of indices to reference
     * the submatrix. 
     *
     * The matrix of indices can be of any type with the restriction that its elements must be
     * type-compatible with the size_type \c of the container. In practice, the following are good candidates:
     * - \c boost::numeric::ublas::indirect_array<A> where \c A can be \c int, \c size_t, \c long, etc...
     * - \c boost::numeric::ublas::matrix<int> can work too (\c int can be replaced by another integer type)
     * - etc...
     *
     * An indirect matrix can be used as a normal matrix in any expression. If the specified indirect matrix 
     * falls outside that of the indices of the matrix, then the \c matrix_indirect is not a well formed 
     * \i Matrix \i Expression and access to an element outside of indices of the matrix is \b undefined.
     *
     * \tparam V the type of the referenced matrix, for example \c matrix<double>)
     * \tparam IA the type of index matrix. Default is \c ublas::indirect_array<>
     */
    template<class M, class IA>
    class matrix_indirect:
        public matrix_expression<matrix_indirect<M, IA> > {

        typedef matrix_indirect<M, IA> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_expression<self_type>::operator ();
#endif
        typedef M matrix_type;
        typedef IA indirect_array_type;
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
        typedef basic_range<size_type, difference_type> range_type;
        typedef basic_slice<size_type, difference_type> slice_type;
        typedef const self_type const_closure_type;
        typedef self_type closure_type;
        typedef typename storage_restrict_traits<typename M::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;
        typedef typename M::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        matrix_indirect (matrix_type &data, size_type size1, size_type size2):
            data_ (data), ia1_ (size1), ia2_ (size2) {}
        BOOST_UBLAS_INLINE
        matrix_indirect (matrix_type &data, const indirect_array_type &ia1, const indirect_array_type &ia2):
            data_ (data), ia1_ (ia1.preprocess (data.size1 ())), ia2_ (ia2.preprocess (data.size2 ())) {}
        BOOST_UBLAS_INLINE
        matrix_indirect (const matrix_closure_type &data, const indirect_array_type &ia1, const indirect_array_type &ia2, int):
            data_ (data), ia1_ (ia1.preprocess (data.size1 ())), ia2_ (ia2.preprocess (data.size2 ())) {}

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size1 () const {
            return ia1_.size ();
        }
        BOOST_UBLAS_INLINE
        size_type size2 () const {
            return ia2_.size ();
        }
        BOOST_UBLAS_INLINE
        const indirect_array_type &indirect1 () const {
            return ia1_;
        }
        BOOST_UBLAS_INLINE
        indirect_array_type &indirect1 () {
            return ia1_;
        }
        BOOST_UBLAS_INLINE
        const indirect_array_type &indirect2 () const {
            return ia2_;
        }
        BOOST_UBLAS_INLINE
        indirect_array_type &indirect2 () {
            return ia2_;
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
            return data_ (ia1_ (i), ia2_ (j));
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
            return data_ (ia1_ (i), ia2_ (j));
        }
#else
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) const {
            return data_ (ia1_ (i), ia2_ (j));
        }
#endif

        // ISSUE can this be done in free project function?
        // Although a const function can create a non-const proxy to a non-const object
        // Critical is that matrix_type and data_ (vector_closure_type) are const correct
        BOOST_UBLAS_INLINE
        matrix_indirect<matrix_type, indirect_array_type> project (const range_type &r1, const range_type &r2) const {
            return matrix_indirect<matrix_type, indirect_array_type> (data_, ia1_.compose (r1.preprocess (data_.size1 ())), ia2_.compose (r2.preprocess (data_.size2 ())), 0);
        }
        BOOST_UBLAS_INLINE
        matrix_indirect<matrix_type, indirect_array_type> project (const slice_type &s1, const slice_type &s2) const {
            return matrix_indirect<matrix_type, indirect_array_type> (data_, ia1_.compose (s1.preprocess (data_.size1 ())), ia2_.compose (s2.preprocess (data_.size2 ())), 0);
        }
        BOOST_UBLAS_INLINE
        matrix_indirect<matrix_type, indirect_array_type> project (const indirect_array_type &ia1, const indirect_array_type &ia2) const {
            return matrix_indirect<matrix_type, indirect_array_type> (data_, ia1_.compose (ia1.preprocess (data_.size1 ())), ia2_.compose (ia2.preprocess (data_.size2 ())), 0);
        }

        // Assignment
        BOOST_UBLAS_INLINE
        matrix_indirect &operator = (const matrix_indirect &mi) {
            matrix_assign<scalar_assign> (*this, mi);
            return *this;
        }
        BOOST_UBLAS_INLINE
        matrix_indirect &assign_temporary (matrix_indirect &mi) {
            return *this = mi;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_indirect &operator = (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, typename matrix_temporary_traits<M>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_indirect &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_indirect& operator += (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, typename matrix_temporary_traits<M>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_indirect &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_indirect& operator -= (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, typename matrix_temporary_traits<M>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix_indirect &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_indirect& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix_indirect& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const matrix_indirect &mi) const {
            return (*this).data_.same_closure (mi.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const matrix_indirect &mi) const {
            return (*this).data_ == mi.data_ && ia1_ == mi.ia1_ && ia2_ == mi.ia2_;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (matrix_indirect mi) {
            if (this != &mi) {
                BOOST_UBLAS_CHECK (size1 () == mi.size1 (), bad_size ());
                BOOST_UBLAS_CHECK (size2 () == mi.size2 (), bad_size ());
                matrix_swap<scalar_swap> (*this, mi);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (matrix_indirect mi1, matrix_indirect mi2) {
            mi1.swap (mi2);
        }

        // Iterator types
    private:
        typedef typename IA::const_iterator const_subiterator1_type;
        typedef typename IA::const_iterator subiterator1_type;
        typedef typename IA::const_iterator const_subiterator2_type;
        typedef typename IA::const_iterator subiterator2_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator1<matrix_indirect<matrix_type, indirect_array_type>,
                                  typename matrix_type::iterator1::iterator_category> iterator1;
        typedef indexed_iterator2<matrix_indirect<matrix_type, indirect_array_type>,
                                  typename matrix_type::iterator2::iterator_category> iterator2;
        typedef indexed_const_iterator1<matrix_indirect<matrix_type, indirect_array_type>,
                                        typename matrix_type::const_iterator1::iterator_category> const_iterator1;
        typedef indexed_const_iterator2<matrix_indirect<matrix_type, indirect_array_type>,
                                        typename matrix_type::const_iterator2::iterator_category> const_iterator2;
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
        const_iterator1 find1 (int /* rank */, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator1 (*this, i, j);
#else
            return const_iterator1 (*this, ia1_.begin () + i, ia2_.begin () + j);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator1 find1 (int /* rank */, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator1 (*this, i, j);
#else
            return iterator1 (*this, ia1_.begin () + i, ia2_.begin () + j);
#endif
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int /* rank */, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator2 (*this, i, j);
#else
            return const_iterator2 (*this, ia1_.begin () + i, ia2_.begin () + j);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator2 find2 (int /* rank */, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator2 (*this, i, j);
#else
            return iterator2 (*this, ia1_.begin () + i, ia2_.begin () + j);
#endif
        }

        // Iterators simply are indices.

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<matrix_indirect>,
            public iterator_base_traits<typename M::const_iterator1::iterator_category>::template
                        iterator_base<const_iterator1, value_type>::type {
        public:
            typedef typename M::const_iterator1::value_type value_type;
            typedef typename M::const_iterator1::difference_type difference_type;
            typedef typename M::const_reference reference;    //FIXME due to indexing access
            typedef typename M::const_iterator1::pointer pointer;
            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &mi, const const_subiterator1_type &it1, const const_subiterator2_type &it2):
                container_const_reference<self_type> (mi), it1_ (it1), it2_ (it2) {}
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ - it.it1_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
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
                return const_iterator2 ((*this) (), it1_, it2_ ().begin ());
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
                return const_iterator2 ((*this) (), it1_, it2_ ().end ());
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
                return it1_.index ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index ();
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ == it.it1_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ < it.it1_;
            }

        private:
            const_subiterator1_type it1_;
            const_subiterator2_type it2_;
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
            public container_reference<matrix_indirect>,
            public iterator_base_traits<typename M::iterator1::iterator_category>::template
                        iterator_base<iterator1, value_type>::type {
        public:
            typedef typename M::iterator1::value_type value_type;
            typedef typename M::iterator1::difference_type difference_type;
            typedef typename M::reference reference;    //FIXME due to indexing access
            typedef typename M::iterator1::pointer pointer;
            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &mi, const subiterator1_type &it1, const subiterator2_type &it2):
                container_reference<self_type> (mi), it1_ (it1), it2_ (it2) {}

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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ - it.it1_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
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
                return iterator2 ((*this) (), it1_, it2_ ().begin ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 end () const {
                return iterator2 ((*this) (), it1_, it2_ ().end ());
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
                return it1_.index ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index ();
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ == it.it1_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator1 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it2_ == it.it2_, external_logic ());
                return it1_ < it.it1_;
            }

        private:
            subiterator1_type it1_;
            subiterator2_type it2_;

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
            public container_const_reference<matrix_indirect>,
            public iterator_base_traits<typename M::const_iterator2::iterator_category>::template
                        iterator_base<const_iterator2, value_type>::type {
        public:
            typedef typename M::const_iterator2::value_type value_type;
            typedef typename M::const_iterator2::difference_type difference_type;
            typedef typename M::const_reference reference;    //FIXME due to indexing access
            typedef typename M::const_iterator2::pointer pointer;
            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &mi, const const_subiterator1_type &it1, const const_subiterator2_type &it2):
                container_const_reference<self_type> (mi), it1_ (it1), it2_ (it2) {}
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ - it.it2_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
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
                return const_iterator1 ((*this) (), it1_ ().begin (), it2_);
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
                return const_iterator1 ((*this) (), it1_ ().end (), it2_);
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
                return it1_.index ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index ();
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ < it.it2_;
            }

        private:
            const_subiterator1_type it1_;
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
            public container_reference<matrix_indirect>,
            public iterator_base_traits<typename M::iterator2::iterator_category>::template
                        iterator_base<iterator2, value_type>::type {
        public:
            typedef typename M::iterator2::value_type value_type;
            typedef typename M::iterator2::difference_type difference_type;
            typedef typename M::reference reference;    //FIXME due to indexing access
            typedef typename M::iterator2::pointer pointer;
            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &mi, const subiterator1_type &it1, const subiterator2_type &it2):
                container_reference<self_type> (mi), it1_ (it1), it2_ (it2) {}

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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ - it.it2_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                // FIXME replace find with at_element
                return (*this) ().data_ (*it1_, *it2_);
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
                return iterator1 ((*this) (), it1_ ().begin (), it2_);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 end () const {
                return iterator1 ((*this) (), it1_ ().end (), it2_);
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
                return it1_.index ();
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_.index ();
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ == it.it2_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator2 &it) const {
                BOOST_UBLAS_CHECK ((*this) ().same_closure  (it ()), external_logic ());
                BOOST_UBLAS_CHECK (it1_ == it.it1_, external_logic ());
                return it2_ < it.it2_;
            }

        private:
            subiterator1_type it1_;
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
        indirect_array_type ia1_;
        indirect_array_type ia2_;
    };

    // Projections
    template<class M, class A>
    BOOST_UBLAS_INLINE
    matrix_indirect<M, indirect_array<A> > project (M &data, const indirect_array<A> &ia1, const indirect_array<A> &ia2) {
        return matrix_indirect<M, indirect_array<A> > (data, ia1, ia2);
    }
    template<class M, class A>
    BOOST_UBLAS_INLINE
    const matrix_indirect<const M, indirect_array<A> > project (const M &data, const indirect_array<A> &ia1, const indirect_array<A> &ia2) {
        // ISSUE was: return matrix_indirect<M, indirect_array<A> > (const_cast<M &> (data), ia1, ia2);
        return matrix_indirect<const M, indirect_array<A> > (data, ia1, ia2);
    }
    template<class M, class IA>
    BOOST_UBLAS_INLINE
    matrix_indirect<M, IA> project (matrix_indirect<M, IA> &data, const typename matrix_indirect<M, IA>::range_type &r1, const typename matrix_indirect<M, IA>::range_type &r2) {
        return data.project (r1, r2);
    }
    template<class M, class IA>
    BOOST_UBLAS_INLINE
    const matrix_indirect<M, IA> project (const matrix_indirect<M, IA> &data, const typename matrix_indirect<M, IA>::range_type &r1, const typename matrix_indirect<M, IA>::range_type &r2) {
        return data.project (r1, r2);
    }
    template<class M, class IA>
    BOOST_UBLAS_INLINE
    matrix_indirect<M, IA> project (matrix_indirect<M, IA> &data, const typename matrix_indirect<M, IA>::slice_type &s1, const typename matrix_indirect<M, IA>::slice_type &s2) {
        return data.project (s1, s2);
    }
    template<class M, class IA>
    BOOST_UBLAS_INLINE
    const matrix_indirect<M, IA> project (const matrix_indirect<M, IA> &data, const typename matrix_indirect<M, IA>::slice_type &s1, const typename matrix_indirect<M, IA>::slice_type &s2) {
        return data.project (s1, s2);
    }
    template<class M, class A>
    BOOST_UBLAS_INLINE
    matrix_indirect<M, indirect_array<A> > project (matrix_indirect<M, indirect_array<A> > &data, const indirect_array<A> &ia1, const indirect_array<A> &ia2) {
        return data.project (ia1, ia2);
    }
    template<class M, class A>
    BOOST_UBLAS_INLINE
    const matrix_indirect<M, indirect_array<A> > project (const matrix_indirect<M, indirect_array<A> > &data, const indirect_array<A> &ia1, const indirect_array<A> &ia2) {
        return data.project (ia1, ia2);
    }

    /// Specialization of temporary_traits
    template <class M>
    struct matrix_temporary_traits< matrix_indirect<M> >
    : matrix_temporary_traits< M > {};
    template <class M>
    struct matrix_temporary_traits< const matrix_indirect<M> >
    : matrix_temporary_traits< M > {};

    template <class M>
    struct vector_temporary_traits< matrix_indirect<M> >
    : vector_temporary_traits< M > {};
    template <class M>
    struct vector_temporary_traits< const matrix_indirect<M> >
    : vector_temporary_traits< M > {};

}}}

#endif
