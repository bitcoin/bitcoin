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

#ifndef _BOOST_UBLAS_VECTOR_PROXY_
#define _BOOST_UBLAS_VECTOR_PROXY_

#include <boost/numeric/ublas/vector_expression.hpp>
#include <boost/numeric/ublas/detail/vector_assign.hpp>
#include <boost/numeric/ublas/detail/temporary.hpp>

// Iterators based on ideas of Jeremy Siek

namespace boost { namespace numeric { namespace ublas {

    /** \brief A vector referencing a continuous subvector of elements of vector \c v containing all elements specified by \c range.
     *
     * A vector range can be used as a normal vector in any expression. 
     * If the specified range falls outside that of the index range of the vector, then
     * the \c vector_range is not a well formed \i Vector \i Expression and access to an 
     * element outside of index range of the vector is \b undefined.
     *
     * \tparam V the type of vector referenced (for example \c vector<double>)
     */
    template<class V>
    class vector_range:
        public vector_expression<vector_range<V> > {

        typedef vector_range<V> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using vector_expression<self_type>::operator ();
#endif
        typedef const V const_vector_type;
        typedef V vector_type;
        typedef typename V::size_type size_type;
        typedef typename V::difference_type difference_type;
        typedef typename V::value_type value_type;
        typedef typename V::const_reference const_reference;
        typedef typename boost::mpl::if_<boost::is_const<V>,
                                          typename V::const_reference,
                                          typename V::reference>::type reference;
        typedef typename boost::mpl::if_<boost::is_const<V>,
                                          typename V::const_closure_type,
                                          typename V::closure_type>::type vector_closure_type;
        typedef basic_range<size_type, difference_type> range_type;
        typedef const self_type const_closure_type;
        typedef self_type closure_type;
        typedef typename storage_restrict_traits<typename V::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        vector_range (vector_type &data, const range_type &r):
            data_ (data), r_ (r.preprocess (data.size ())) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (r_.start () <= data_.size () &&
            //                   r_.start () + r_.size () <= data_.size (), bad_index ());
        }
        BOOST_UBLAS_INLINE
        vector_range (const vector_closure_type &data, const range_type &r, bool):
            data_ (data), r_ (r.preprocess (data.size ())) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (r_.start () <= data_.size () &&
            //                    r_.start () + r_.size () <= data_.size (), bad_index ());
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type start () const {
            return r_.start ();
        }
        BOOST_UBLAS_INLINE
        size_type size () const {
            return r_.size ();
        }

        // Storage accessors
        BOOST_UBLAS_INLINE
        const vector_closure_type &data () const {
            return data_;
        }
        BOOST_UBLAS_INLINE
        vector_closure_type &data () {
            return data_;
        }

        // Element access
#ifndef BOOST_UBLAS_PROXY_CONST_MEMBER
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i) const {
            return data_ (r_ (i));
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) {
            return data_ (r_ (i));
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
            return data_ (r_ (i));
        }

        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) const {
            return (*this) (i);
        }
#endif

        // ISSUE can this be done in free project function?
        // Although a const function can create a non-const proxy to a non-const object
        // Critical is that vector_type and data_ (vector_closure_type) are const correct
        BOOST_UBLAS_INLINE
        vector_range<vector_type> project (const range_type &r) const {
            return vector_range<vector_type> (data_, r_.compose (r.preprocess (data_.size ())), false);
        }

        // Assignment
        BOOST_UBLAS_INLINE
        vector_range &operator = (const vector_range &vr) {
            // ISSUE need a temporary, proxy can be overlaping alias
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (vr));
            return *this;
        }
        BOOST_UBLAS_INLINE
        vector_range &assign_temporary (vector_range &vr) {
            // assign elements, proxied container remains the same
            vector_assign<scalar_assign> (*this, vr);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_range &operator = (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_range &assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_range &operator += (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_range &plus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_range &operator -= (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_range &minus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        vector_range &operator *= (const AT &at) {
            vector_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        vector_range &operator /= (const AT &at) {
            vector_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const vector_range &vr) const {
            return (*this).data_.same_closure (vr.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const vector_range &vr) const {
            return (*this).data_ == vr.data_ && r_ == vr.r_;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (vector_range vr) {
            if (this != &vr) {
                BOOST_UBLAS_CHECK (size () == vr.size (), bad_size ());
                // Sparse ranges may be nonconformant now.
                // std::swap_ranges (begin (), end (), vr.begin ());
                vector_swap<scalar_swap> (*this, vr);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (vector_range vr1, vector_range vr2) {
            vr1.swap (vr2);
        }

        // Iterator types
    private:
        typedef typename V::const_iterator const_subiterator_type;
        typedef typename boost::mpl::if_<boost::is_const<V>,
                                          typename V::const_iterator,
                                          typename V::iterator>::type subiterator_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator<vector_range<vector_type>,
                                 typename subiterator_type::iterator_category> iterator;
        typedef indexed_const_iterator<vector_range<vector_type>,
                                       typename const_subiterator_type::iterator_category> const_iterator;
#else
        class const_iterator;
        class iterator;
#endif

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator find (size_type i) const {
            const_subiterator_type it (data_.find (start () + i));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator (*this, it.index ());
#else
            return const_iterator (*this, it);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator find (size_type i) {
            subiterator_type it (data_.find (start () + i));
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator (*this, it.index ());
#else
            return iterator (*this, it);
#endif
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator:
            public container_const_reference<vector_range>,
            public iterator_base_traits<typename const_subiterator_type::iterator_category>::template
                        iterator_base<const_iterator, value_type>::type {
        public:
            typedef typename const_subiterator_type::difference_type difference_type;
            typedef typename const_subiterator_type::value_type value_type;
            typedef typename const_subiterator_type::reference reference;
            typedef typename const_subiterator_type::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator (const self_type &vr, const const_subiterator_type &it):
                container_const_reference<self_type> (vr), it_ (it) {}
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
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
                return it_.index () - (*this) ().start ();
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
            public container_reference<vector_range>,
            public iterator_base_traits<typename subiterator_type::iterator_category>::template
                        iterator_base<iterator, value_type>::type {
        public:
            typedef typename subiterator_type::difference_type difference_type;
            typedef typename subiterator_type::value_type value_type;
            typedef typename subiterator_type::reference reference;
            typedef typename subiterator_type::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator (self_type &vr, const subiterator_type &it):
                container_reference<self_type> (vr), it_ (it) {}

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
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
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
                return it_.index () - (*this) ().start ();
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
        vector_closure_type data_;
        range_type r_;
    };

    // ------------------
    // Simple Projections
    // ------------------

    /** \brief Return a \c vector_range on a specified vector, a start and stop index.
     * Return a \c vector_range on a specified vector, a start and stop index. The resulting \c vector_range can be manipulated like a normal vector.
     * If the specified range falls outside that of of the index range of the vector, then the resulting \c vector_range is not a well formed
     * Vector Expression and access to an element outside of index range of the vector is \b undefined.
     */
    template<class V>
    BOOST_UBLAS_INLINE
    vector_range<V> subrange (V &data, typename V::size_type start, typename V::size_type stop) {
        typedef basic_range<typename V::size_type, typename V::difference_type> range_type;
        return vector_range<V> (data, range_type (start, stop));
    }

    /** \brief Return a \c const \c vector_range on a specified vector, a start and stop index.
     * Return a \c const \c vector_range on a specified vector, a start and stop index. The resulting \c const \c vector_range can be manipulated like a normal vector.
     *If the specified range falls outside that of of the index range of the vector, then the resulting \c vector_range is not a well formed
     * Vector Expression and access to an element outside of index range of the vector is \b undefined.
     */
    template<class V>
    BOOST_UBLAS_INLINE
    vector_range<const V> subrange (const V &data, typename V::size_type start, typename V::size_type stop) {
        typedef basic_range<typename V::size_type, typename V::difference_type> range_type;
        return vector_range<const V> (data, range_type (start, stop));
    }

    // -------------------
    // Generic Projections
    // -------------------
    
    /** \brief Return a \c const \c vector_range on a specified vector and \c range
     * Return a \c const \c vector_range on a specified vector and \c range. The resulting \c vector_range can be manipulated like a normal vector.
     * If the specified range falls outside that of of the index range of the vector, then the resulting \c vector_range is not a well formed
     * Vector Expression and access to an element outside of index range of the vector is \b undefined.
     */
    template<class V>
    BOOST_UBLAS_INLINE
    vector_range<V> project (V &data, typename vector_range<V>::range_type const &r) {
        return vector_range<V> (data, r);
    }

    /** \brief Return a \c vector_range on a specified vector and \c range
     * Return a \c vector_range on a specified vector and \c range. The resulting \c vector_range can be manipulated like a normal vector.
     * If the specified range falls outside that of of the index range of the vector, then the resulting \c vector_range is not a well formed
     * Vector Expression and access to an element outside of index range of the vector is \b undefined.
     */
    template<class V>
    BOOST_UBLAS_INLINE
    const vector_range<const V> project (const V &data, typename vector_range<V>::range_type const &r) {
        // ISSUE was: return vector_range<V> (const_cast<V &> (data), r);
        return vector_range<const V> (data, r);
   }

    /** \brief Return a \c const \c vector_range on a specified vector and const \c range
     * Return a \c const \c vector_range on a specified vector and const \c range. The resulting \c vector_range can be manipulated like a normal vector.
     * If the specified range falls outside that of of the index range of the vector, then the resulting \c vector_range is not a well formed
     * Vector Expression and access to an element outside of index range of the vector is \b undefined.
     */
    template<class V>
    BOOST_UBLAS_INLINE
    vector_range<V> project (vector_range<V> &data, const typename vector_range<V>::range_type &r) {
        return data.project (r);
    }

    /** \brief Return a \c vector_range on a specified vector and const \c range
     * Return a \c vector_range on a specified vector and const \c range. The resulting \c vector_range can be manipulated like a normal vector.
     * If the specified range falls outside that of of the index range of the vector, then the resulting \c vector_range is not a well formed
     * Vector Expression and access to an element outside of index range of the vector is \b undefined.
     */
    template<class V>
    BOOST_UBLAS_INLINE
    const vector_range<V> project (const vector_range<V> &data, const typename vector_range<V>::range_type &r) {
        return data.project (r);
    }

    // Specialization of temporary_traits
    template <class V>
    struct vector_temporary_traits< vector_range<V> >
    : vector_temporary_traits< V > {} ;
    template <class V>
    struct vector_temporary_traits< const vector_range<V> >
    : vector_temporary_traits< V > {} ;


    /** \brief A vector referencing a non continuous subvector of elements of vector v containing all elements specified by \c slice.
     *
     * A vector slice can be used as a normal vector in any expression.
     * If the specified slice falls outside that of the index slice of the vector, then
     * the \c vector_slice is not a well formed \i Vector \i Expression and access to an 
     * element outside of index slice of the vector is \b undefined.
     *
     * A slice is a generalization of a range. In a range going from \f$a\f$ to \f$b\f$, 
     * all elements belong to the range. In a slice, a \i \f$step\f$ can be specified meaning to
     * take one element over \f$step\f$ in the range specified from \f$a\f$ to \f$b\f$.
     * Obviously, a slice with a \f$step\f$ of 1 is equivalent to a range.
     *
     * \tparam V the type of vector referenced (for example \c vector<double>)
     */
    template<class V>
    class vector_slice:
        public vector_expression<vector_slice<V> > {

        typedef vector_slice<V> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using vector_expression<self_type>::operator ();
#endif
        typedef const V const_vector_type;
        typedef V vector_type;
        typedef typename V::size_type size_type;
        typedef typename V::difference_type difference_type;
        typedef typename V::value_type value_type;
        typedef typename V::const_reference const_reference;
        typedef typename boost::mpl::if_<boost::is_const<V>,
                                          typename V::const_reference,
                                          typename V::reference>::type reference;
        typedef typename boost::mpl::if_<boost::is_const<V>,
                                          typename V::const_closure_type,
                                          typename V::closure_type>::type vector_closure_type;
        typedef basic_range<size_type, difference_type> range_type;
        typedef basic_slice<size_type, difference_type> slice_type;
        typedef const self_type const_closure_type;
        typedef self_type closure_type;
        typedef typename storage_restrict_traits<typename V::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        vector_slice (vector_type &data, const slice_type &s):
            data_ (data), s_ (s.preprocess (data.size ())) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (s_.start () <= data_.size () &&
            //                    s_.start () + s_.stride () * (s_.size () - (s_.size () > 0)) <= data_.size (), bad_index ());
        }
        BOOST_UBLAS_INLINE
        vector_slice (const vector_closure_type &data, const slice_type &s, int):
            data_ (data), s_ (s.preprocess (data.size ())) {
            // Early checking of preconditions here.
            // BOOST_UBLAS_CHECK (s_.start () <= data_.size () &&
            //                    s_.start () + s_.stride () * (s_.size () - (s_.size () > 0)) <= data_.size (), bad_index ());
        }

        // Accessors
        BOOST_UBLAS_INLINE
        size_type start () const {
            return s_.start ();
        }
        BOOST_UBLAS_INLINE
        difference_type stride () const {
            return s_.stride ();
        }
        BOOST_UBLAS_INLINE
        size_type size () const {
            return s_.size ();
        }

        // Storage accessors
        BOOST_UBLAS_INLINE
        const vector_closure_type &data () const {
            return data_;
        }
        BOOST_UBLAS_INLINE
        vector_closure_type &data () {
            return data_;
        }

        // Element access
#ifndef BOOST_UBLAS_PROXY_CONST_MEMBER
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i) const {
            return data_ (s_ (i));
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) {
            return data_ (s_ (i));
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
            return data_ (s_ (i));
        }

        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) const {
            return (*this) (i);
        }
#endif

        // ISSUE can this be done in free project function?
        // Although a const function can create a non-const proxy to a non-const object
        // Critical is that vector_type and data_ (vector_closure_type) are const correct
        BOOST_UBLAS_INLINE
        vector_slice<vector_type> project (const range_type &r) const {
            return vector_slice<vector_type>  (data_, s_.compose (r.preprocess (data_.size ())), false);
        }
        BOOST_UBLAS_INLINE
        vector_slice<vector_type> project (const slice_type &s) const {
            return vector_slice<vector_type>  (data_, s_.compose (s.preprocess (data_.size ())), false);
        }

        // Assignment
        BOOST_UBLAS_INLINE
        vector_slice &operator = (const vector_slice &vs) {
            // ISSUE need a temporary, proxy can be overlaping alias
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (vs));
            return *this;
        }
        BOOST_UBLAS_INLINE
        vector_slice &assign_temporary (vector_slice &vs) {
            // assign elements, proxied container remains the same
            vector_assign<scalar_assign> (*this, vs);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_slice &operator = (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_slice &assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_slice &operator += (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_slice &plus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_slice &operator -= (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_slice &minus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        vector_slice &operator *= (const AT &at) {
            vector_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        vector_slice &operator /= (const AT &at) {
            vector_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const vector_slice &vr) const {
            return (*this).data_.same_closure (vr.data_);
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const vector_slice &vs) const {
            return (*this).data_ == vs.data_ && s_ == vs.s_;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (vector_slice vs) {
            if (this != &vs) {
                BOOST_UBLAS_CHECK (size () == vs.size (), bad_size ());
                // Sparse ranges may be nonconformant now.
                // std::swap_ranges (begin (), end (), vs.begin ());
                vector_swap<scalar_swap> (*this, vs);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (vector_slice vs1, vector_slice vs2) {
            vs1.swap (vs2);
        }

        // Iterator types
    private:
        // Use slice as an index - FIXME this fails for packed assignment
        typedef typename slice_type::const_iterator const_subiterator_type;
        typedef typename slice_type::const_iterator subiterator_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator<vector_slice<vector_type>,
                                 typename vector_type::iterator::iterator_category> iterator;
        typedef indexed_const_iterator<vector_slice<vector_type>,
                                       typename vector_type::const_iterator::iterator_category> const_iterator;
#else
        class const_iterator;
        class iterator;
#endif

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator find (size_type i) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator (*this, i);
#else
            return const_iterator (*this, s_.begin () + i);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator find (size_type i) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator (*this, i);
#else
            return iterator (*this, s_.begin () + i);
#endif
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator:
            public container_const_reference<vector_slice>,
            public iterator_base_traits<typename V::const_iterator::iterator_category>::template
                        iterator_base<const_iterator, value_type>::type {
        public:
            typedef typename V::const_iterator::difference_type difference_type;
            typedef typename V::const_iterator::value_type value_type;
            typedef typename V::const_reference reference;    //FIXME due to indexing access
            typedef typename V::const_iterator::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator (const self_type &vs, const const_subiterator_type &it):
                container_const_reference<self_type> (vs), it_ (it) {}
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                // FIXME replace find with at_element
                BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
                return (*this) ().data_ (*it_);
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return it_.index ();
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
            public container_reference<vector_slice>,
            public iterator_base_traits<typename V::iterator::iterator_category>::template
                        iterator_base<iterator, value_type>::type {
        public:
            typedef typename V::iterator::difference_type difference_type;
            typedef typename V::iterator::value_type value_type;
            typedef typename V::reference reference;    //FIXME due to indexing access
            typedef typename V::iterator::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator (self_type &vs, const subiterator_type &it):
                container_reference<self_type> (vs), it_ (it) {}

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
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                // FIXME replace find with at_element
                BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
                return (*this) ().data_ (*it_);
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }


            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return it_.index ();
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
        vector_closure_type data_;
        slice_type s_;
    };

    // Simple Projections
    template<class V>
    BOOST_UBLAS_INLINE
    vector_slice<V> subslice (V &data, typename V::size_type start, typename V::difference_type stride, typename V::size_type size) {
        typedef basic_slice<typename V::size_type, typename V::difference_type> slice_type;
        return vector_slice<V> (data, slice_type (start, stride, size));
    }
    template<class V>
    BOOST_UBLAS_INLINE
    vector_slice<const V> subslice (const V &data, typename V::size_type start, typename V::difference_type stride, typename V::size_type size)  {
        typedef basic_slice<typename V::size_type, typename V::difference_type> slice_type;
        return vector_slice<const V> (data, slice_type (start, stride, size));
    }

    // Generic Projections
    template<class V>
    BOOST_UBLAS_INLINE
    vector_slice<V> project (V &data, const typename vector_slice<V>::slice_type &s) {
        return vector_slice<V> (data, s);
    }
    template<class V>
    BOOST_UBLAS_INLINE
    const vector_slice<const V> project (const V &data, const typename vector_slice<V>::slice_type &s) {
        // ISSUE was: return vector_slice<V> (const_cast<V &> (data), s);
        return vector_slice<const V> (data, s);
    }
    template<class V>
    BOOST_UBLAS_INLINE
    vector_slice<V> project (vector_slice<V> &data, const typename vector_slice<V>::slice_type &s) {
        return data.project (s);
    }
    template<class V>
    BOOST_UBLAS_INLINE
    const vector_slice<V> project (const vector_slice<V> &data, const typename vector_slice<V>::slice_type &s) {
        return data.project (s);
    }
    // ISSUE in the following two functions it would be logical to use vector_slice<V>::range_type but this confuses VC7.1 and 8.0
    template<class V>
    BOOST_UBLAS_INLINE
    vector_slice<V> project (vector_slice<V> &data, const typename vector_range<V>::range_type &r) {
        return data.project (r);
    }
    template<class V>
    BOOST_UBLAS_INLINE
    const vector_slice<V> project (const vector_slice<V> &data, const typename vector_range<V>::range_type &r) {
        return data.project (r);
    }

    // Specialization of temporary_traits
    template <class V>
    struct vector_temporary_traits< vector_slice<V> >
    : vector_temporary_traits< V > {} ;
    template <class V>
    struct vector_temporary_traits< const vector_slice<V> >
    : vector_temporary_traits< V > {} ;


    // Vector based indirection class
    // Contributed by Toon Knapen.
    // Extended and optimized by Kresimir Fresl.

    /** \brief A vector referencing a non continuous subvector of elements given another vector of indices.
     *
     * It is the most general version of any subvectors because it uses another vector of indices to reference
     * the subvector. 
     *
     * The vector of indices can be of any type with the restriction that its elements must be
     * type-compatible with the size_type \c of the container. In practice, the following are good candidates:
     * - \c boost::numeric::ublas::indirect_array<A> where \c A can be \c int, \c size_t, \c long, etc...
     * - \c std::vector<A> where \c A can \c int, \c size_t, \c long, etc...
     * - \c boost::numeric::ublas::vector<int> can work too (\c int can be replaced by another integer type)
     * - etc...
     *
     * An indirect vector can be used as a normal vector in any expression. If the specified indirect vector 
     * falls outside that of the indices of the vector, then the \c vector_indirect is not a well formed 
     * \i Vector \i Expression and access to an element outside of indices of the vector is \b undefined.
     *
     * \tparam V the type of vector referenced (for example \c vector<double>)
     * \tparam IA the type of index vector. Default is \c ublas::indirect_array<>
     */
    template<class V, class IA>
    class vector_indirect:
        public vector_expression<vector_indirect<V, IA> > {

        typedef vector_indirect<V, IA> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using vector_expression<self_type>::operator ();
#endif
        typedef const V const_vector_type;
        typedef V vector_type;
        typedef const IA const_indirect_array_type;
        typedef IA indirect_array_type;
        typedef typename V::size_type size_type;
        typedef typename V::difference_type difference_type;
        typedef typename V::value_type value_type;
        typedef typename V::const_reference const_reference;
        typedef typename boost::mpl::if_<boost::is_const<V>,
                                          typename V::const_reference,
                                          typename V::reference>::type reference;
        typedef typename boost::mpl::if_<boost::is_const<V>,
                                          typename V::const_closure_type,
                                          typename V::closure_type>::type vector_closure_type;
        typedef basic_range<size_type, difference_type> range_type;
        typedef basic_slice<size_type, difference_type> slice_type;
        typedef const self_type const_closure_type;
        typedef self_type closure_type;
        typedef typename storage_restrict_traits<typename V::storage_category,
                                                 dense_proxy_tag>::storage_category storage_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        vector_indirect (vector_type &data, size_type size):
            data_ (data), ia_ (size) {}
        BOOST_UBLAS_INLINE
        vector_indirect (vector_type &data, const indirect_array_type &ia):
            data_ (data), ia_ (ia.preprocess (data.size ())) {}
        BOOST_UBLAS_INLINE
        vector_indirect (const vector_closure_type &data, const indirect_array_type &ia, int):
            data_ (data), ia_ (ia.preprocess (data.size ())) {}

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size () const {
            return ia_.size ();
        }
        BOOST_UBLAS_INLINE
        const_indirect_array_type &indirect () const {
            return ia_;
        }
        BOOST_UBLAS_INLINE
        indirect_array_type &indirect () {
            return ia_;
        }

        // Storage accessors
        BOOST_UBLAS_INLINE
        const vector_closure_type &data () const {
            return data_;
        }
        BOOST_UBLAS_INLINE
        vector_closure_type &data () {
            return data_;
        }

        // Element access
#ifndef BOOST_UBLAS_PROXY_CONST_MEMBER
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i) const {
            return data_ (ia_ (i));
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i) {
            return data_ (ia_ (i));
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
            return data_ (ia_ (i));
        }

        BOOST_UBLAS_INLINE
        reference operator [] (size_type i) const {
            return (*this) (i);
        }
#endif

        // ISSUE can this be done in free project function?
        // Although a const function can create a non-const proxy to a non-const object
        // Critical is that vector_type and data_ (vector_closure_type) are const correct
        BOOST_UBLAS_INLINE
        vector_indirect<vector_type, indirect_array_type> project (const range_type &r) const {
            return vector_indirect<vector_type, indirect_array_type> (data_, ia_.compose (r.preprocess (data_.size ())), 0);
        }
        BOOST_UBLAS_INLINE
        vector_indirect<vector_type, indirect_array_type> project (const slice_type &s) const {
            return vector_indirect<vector_type, indirect_array_type> (data_, ia_.compose (s.preprocess (data_.size ())), 0);
        }
        BOOST_UBLAS_INLINE
        vector_indirect<vector_type, indirect_array_type> project (const indirect_array_type &ia) const {
            return vector_indirect<vector_type, indirect_array_type> (data_, ia_.compose (ia.preprocess (data_.size ())), 0);
        }

        // Assignment
        BOOST_UBLAS_INLINE
        vector_indirect &operator = (const vector_indirect &vi) {
            // ISSUE need a temporary, proxy can be overlaping alias
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (vi));
            return *this;
        }
        BOOST_UBLAS_INLINE
        vector_indirect &assign_temporary (vector_indirect &vi) {
            // assign elements, proxied container remains the same
            vector_assign<scalar_assign> (*this, vi);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_indirect &operator = (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_indirect &assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_indirect &operator += (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (*this + ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_indirect &plus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_indirect &operator -= (const vector_expression<AE> &ae) {
            vector_assign<scalar_assign> (*this, typename vector_temporary_traits<V>::type (*this - ae));
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_indirect &minus_assign (const vector_expression<AE> &ae) {
            vector_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        vector_indirect &operator *= (const AT &at) {
            vector_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        vector_indirect &operator /= (const AT &at) {
            vector_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Closure comparison
        BOOST_UBLAS_INLINE
        bool same_closure (const vector_indirect &/*vr*/) const {
            return true;
        }

        // Comparison
        BOOST_UBLAS_INLINE
        bool operator == (const vector_indirect &vi) const {
            return (*this).data_ == vi.data_ && ia_ == vi.ia_;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (vector_indirect vi) {
            if (this != &vi) {
                BOOST_UBLAS_CHECK (size () == vi.size (), bad_size ());
                // Sparse ranges may be nonconformant now.
                // std::swap_ranges (begin (), end (), vi.begin ());
                vector_swap<scalar_swap> (*this, vi);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (vector_indirect vi1, vector_indirect vi2) {
            vi1.swap (vi2);
        }

        // Iterator types
    private:
        // Use indirect array as an index - FIXME this fails for packed assignment
        typedef typename IA::const_iterator const_subiterator_type;
        typedef typename IA::const_iterator subiterator_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator<vector_indirect<vector_type, indirect_array_type>,
                                 typename vector_type::iterator::iterator_category> iterator;
        typedef indexed_const_iterator<vector_indirect<vector_type, indirect_array_type>,
                                       typename vector_type::const_iterator::iterator_category> const_iterator;
#else
        class const_iterator;
        class iterator;
#endif
        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator find (size_type i) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator (*this, i);
#else
            return const_iterator (*this, ia_.begin () + i);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator find (size_type i) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator (*this, i);
#else
            return iterator (*this, ia_.begin () + i);
#endif
        }

        // Iterators simply are indices.

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator:
            public container_const_reference<vector_indirect>,
            public iterator_base_traits<typename V::const_iterator::iterator_category>::template
                        iterator_base<const_iterator, value_type>::type {
        public:
            typedef typename V::const_iterator::difference_type difference_type;
            typedef typename V::const_iterator::value_type value_type;
            typedef typename V::const_reference reference;    //FIXME due to indexing access
            typedef typename V::const_iterator::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator (const self_type &vi, const const_subiterator_type &it):
                container_const_reference<self_type> (vi), it_ (it) {}
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
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                // FIXME replace find with at_element
                BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
                return (*this) ().data_ (*it_);
            }
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return it_.index ();
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
            public container_reference<vector_indirect>,
            public iterator_base_traits<typename V::iterator::iterator_category>::template
                        iterator_base<iterator, value_type>::type {
        public:
            typedef typename V::iterator::difference_type difference_type;
            typedef typename V::iterator::value_type value_type;
            typedef typename V::reference reference;    //FIXME due to indexing access
            typedef typename V::iterator::pointer pointer;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator (self_type &vi, const subiterator_type &it):
                container_reference<self_type> (vi), it_ (it) {}

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
                BOOST_UBLAS_CHECK ((*this) ().same_closure (it ()), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                // FIXME replace find with at_element
                BOOST_UBLAS_CHECK (index () < (*this) ().size (), bad_index ());
                return (*this) ().data_ (*it_);
            }
            BOOST_UBLAS_INLINE
            reference operator [] (difference_type n) const {
                return *(*this + n);
            }

            // Index
            BOOST_UBLAS_INLINE
            size_type index () const {
                return it_.index ();
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
        vector_closure_type data_;
        indirect_array_type ia_;
    };

    // Projections
    template<class V, class A>
    BOOST_UBLAS_INLINE
    vector_indirect<V, indirect_array<A> > project (V &data, const indirect_array<A> &ia) {
        return vector_indirect<V, indirect_array<A> > (data, ia);
    }
    template<class V, class A>
    BOOST_UBLAS_INLINE
    const vector_indirect<const V, indirect_array<A> > project (const V &data, const indirect_array<A> &ia) {
        // ISSUE was: return vector_indirect<V, indirect_array<A> > (const_cast<V &> (data), ia)
        return vector_indirect<const V, indirect_array<A> > (data, ia);
    }
    template<class V, class IA>
    BOOST_UBLAS_INLINE
    vector_indirect<V, IA> project (vector_indirect<V, IA> &data, const typename vector_indirect<V, IA>::range_type &r) {
        return data.project (r);
    }
    template<class V, class IA>
    BOOST_UBLAS_INLINE
    const vector_indirect<V, IA> project (const vector_indirect<V, IA> &data, const typename vector_indirect<V, IA>::range_type &r) {
        return data.project (r);
    }
    template<class V, class IA>
    BOOST_UBLAS_INLINE
    vector_indirect<V, IA> project (vector_indirect<V, IA> &data, const typename vector_indirect<V, IA>::slice_type &s) {
        return data.project (s);
    }
    template<class V, class IA>
    BOOST_UBLAS_INLINE
    const vector_indirect<V, IA> project (const vector_indirect<V, IA> &data, const typename vector_indirect<V, IA>::slice_type &s) {
        return data.project (s);
    }
    template<class V, class A>
    BOOST_UBLAS_INLINE
    vector_indirect<V, indirect_array<A> > project (vector_indirect<V, indirect_array<A> > &data, const indirect_array<A> &ia) {
        return data.project (ia);
    }
    template<class V, class A>
    BOOST_UBLAS_INLINE
    const vector_indirect<V, indirect_array<A> > project (const vector_indirect<V, indirect_array<A> > &data, const indirect_array<A> &ia) {
        return data.project (ia);
    }

    // Specialization of temporary_traits
    template <class V>
    struct vector_temporary_traits< vector_indirect<V> >
    : vector_temporary_traits< V > {} ;
    template <class V>
    struct vector_temporary_traits< const vector_indirect<V> >
    : vector_temporary_traits< V > {} ;

}}}

#endif
