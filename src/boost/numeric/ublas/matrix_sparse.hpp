//
//  Copyright (c) 2000-2007
//  Joerg Walter, Mathias Koch, Gunter Winkler
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#ifndef _BOOST_UBLAS_MATRIX_SPARSE_
#define _BOOST_UBLAS_MATRIX_SPARSE_

#include <boost/numeric/ublas/vector_sparse.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/detail/matrix_assign.hpp>
#if BOOST_UBLAS_TYPE_CHECK
#include <boost/numeric/ublas/matrix.hpp>
#endif

// Iterators based on ideas of Jeremy Siek

namespace boost { namespace numeric { namespace ublas {

#ifdef BOOST_UBLAS_STRICT_MATRIX_SPARSE

    template<class M>
    class sparse_matrix_element:
       public container_reference<M> {
    public:
        typedef M matrix_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;
        typedef const value_type &const_reference;
        typedef value_type *pointer;
        typedef const value_type *const_pointer;

    private:
        // Proxied element operations
        void get_d () const {
            const_pointer p = (*this) ().find_element (i_, j_);
            if (p)
                d_ = *p;
            else
                d_ = value_type/*zero*/();
        }

        void set (const value_type &s) const {
            pointer p = (*this) ().find_element (i_, j_);
            if (!p)
                (*this) ().insert_element (i_, j_, s);
            else
                *p = s;
        }
        
    public:
        // Construction and destruction
        BOOST_UBLAS_INLINE
        sparse_matrix_element (matrix_type &m, size_type i, size_type j):
            container_reference<matrix_type> (m), i_ (i), j_ (j) {
        }
        BOOST_UBLAS_INLINE
        sparse_matrix_element (const sparse_matrix_element &p):
            container_reference<matrix_type> (p), i_ (p.i_), j_ (p.j_) {}
        BOOST_UBLAS_INLINE
        ~sparse_matrix_element () {
        }

        // Assignment
        BOOST_UBLAS_INLINE
        sparse_matrix_element &operator = (const sparse_matrix_element &p) {
            // Overide the implict copy assignment
            p.get_d ();
            set (p.d_);
            return *this;
        }
        template<class D>
        BOOST_UBLAS_INLINE
        sparse_matrix_element &operator = (const D &d) {
            set (d);
            return *this;
        }
        template<class D>
        BOOST_UBLAS_INLINE
        sparse_matrix_element &operator += (const D &d) {
            get_d ();
            d_ += d;
            set (d_);
            return *this;
        }
        template<class D>
        BOOST_UBLAS_INLINE
        sparse_matrix_element &operator -= (const D &d) {
            get_d ();
            d_ -= d;
            set (d_);
            return *this;
        }
        template<class D>
        BOOST_UBLAS_INLINE
        sparse_matrix_element &operator *= (const D &d) {
            get_d ();
            d_ *= d;
            set (d_);
            return *this;
        }
        template<class D>
        BOOST_UBLAS_INLINE
        sparse_matrix_element &operator /= (const D &d) {
            get_d ();
            d_ /= d;
            set (d_);
            return *this;
        }

        // Comparison
        template<class D>
        BOOST_UBLAS_INLINE
        bool operator == (const D &d) const {
            get_d ();
            return d_ == d;
        }
        template<class D>
        BOOST_UBLAS_INLINE
        bool operator != (const D &d) const {
            get_d ();
            return d_ != d;
        }

        // Conversion - weak link in proxy as d_ is not a perfect alias for the element
        BOOST_UBLAS_INLINE
        operator const_reference () const {
            get_d ();
            return d_;
        }

        // Conversion to reference - may be invalidated
        BOOST_UBLAS_INLINE
        value_type& ref () const {
            const pointer p = (*this) ().find_element (i_, j_);
            if (!p)
                return (*this) ().insert_element (i_, j_, value_type/*zero*/());
            else
                return *p;
        }

    private:
        size_type i_;
        size_type j_;
        mutable value_type d_;
    };

    /*
     * Generalise explicit reference access
     */
    namespace detail {
        template <class V>
        struct element_reference<sparse_matrix_element<V> > {
            typedef typename V::value_type& reference;
            static reference get_reference (const sparse_matrix_element<V>& sve)
            {
                return sve.ref ();
            }
        };
    }


    template<class M>
    struct type_traits<sparse_matrix_element<M> > {
        typedef typename M::value_type element_type;
        typedef type_traits<sparse_matrix_element<M> > self_type;
        typedef typename type_traits<element_type>::value_type value_type;
        typedef typename type_traits<element_type>::const_reference const_reference;
        typedef sparse_matrix_element<M> reference;
        typedef typename type_traits<element_type>::real_type real_type;
        typedef typename type_traits<element_type>::precision_type precision_type;

        static const unsigned plus_complexity = type_traits<element_type>::plus_complexity;
        static const unsigned multiplies_complexity = type_traits<element_type>::multiplies_complexity;

        static
        BOOST_UBLAS_INLINE
        real_type real (const_reference t) {
            return type_traits<element_type>::real (t);
        }
        static
        BOOST_UBLAS_INLINE
        real_type imag (const_reference t) {
            return type_traits<element_type>::imag (t);
        }
        static
        BOOST_UBLAS_INLINE
        value_type conj (const_reference t) {
            return type_traits<element_type>::conj (t);
        }

        static
        BOOST_UBLAS_INLINE
        real_type type_abs (const_reference t) {
            return type_traits<element_type>::type_abs (t);
        }
        static
        BOOST_UBLAS_INLINE
        value_type type_sqrt (const_reference t) {
            return type_traits<element_type>::type_sqrt (t);
        }

        static
        BOOST_UBLAS_INLINE
        real_type norm_1 (const_reference t) {
            return type_traits<element_type>::norm_1 (t);
        }
        static
        BOOST_UBLAS_INLINE
        real_type norm_2 (const_reference t) {
            return type_traits<element_type>::norm_2 (t);
        }
        static
        BOOST_UBLAS_INLINE
        real_type norm_inf (const_reference t) {
            return type_traits<element_type>::norm_inf (t);
        }

        static
        BOOST_UBLAS_INLINE
        bool equals (const_reference t1, const_reference t2) {
            return type_traits<element_type>::equals (t1, t2);
        }
    };

    template<class M1, class T2>
    struct promote_traits<sparse_matrix_element<M1>, T2> {
        typedef typename promote_traits<typename sparse_matrix_element<M1>::value_type, T2>::promote_type promote_type;
    };
    template<class T1, class M2>
    struct promote_traits<T1, sparse_matrix_element<M2> > {
        typedef typename promote_traits<T1, typename sparse_matrix_element<M2>::value_type>::promote_type promote_type;
    };
    template<class M1, class M2>
    struct promote_traits<sparse_matrix_element<M1>, sparse_matrix_element<M2> > {
        typedef typename promote_traits<typename sparse_matrix_element<M1>::value_type,
                                        typename sparse_matrix_element<M2>::value_type>::promote_type promote_type;
    };

#endif

   /** \brief Index map based sparse matrix of values of type \c T
    *
    * This class represents a matrix by using a \c key to value mapping. The default type is
    * \code template<class T, class L = row_major, class A =  map_std<std::size_t, T> > class mapped_matrix; \endcode
    * So, by default a STL map container is used to associate keys and values. The key is computed depending on 
    * the layout type \c L as \code key = layout_type::element(i, size1_, j, size2_); \endcode
    * which means \code key = (i*size2+j) \endcode for a row major matrix.
    * Limitations: The matrix size must not exceed \f$(size1*size2) < \f$ \code std::limits<std::size_t> \endcode. 
    * The \ref find1() and \ref find2() operations have a complexity of at least \f$\mathcal{O}(log(nnz))\f$, depending
    * on the efficiency of \c std::lower_bound on the key set of the map.
    * Orientation and storage can also be specified, otherwise a row major orientation is used. 
    * It is \b not required by the storage to initialize elements of the matrix. By default, the orientation is \c row_major. 
    *
    * \sa fwd.hpp, storage_sparse.hpp
    *
    * \tparam T the type of object stored in the matrix (like double, float, complex, etc...)
    * \tparam L the storage organization. It can be either \c row_major or \c column_major. By default it is \c row_major
    */
    template<class T, class L, class A>
    class mapped_matrix:
        public matrix_container<mapped_matrix<T, L, A> > {

        typedef T &true_reference;
        typedef T *pointer;
        typedef const T * const_pointer;
        typedef L layout_type;
        typedef mapped_matrix<T, L, A> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_container<self_type>::operator ();
#endif
        typedef typename A::size_type size_type;
        typedef typename A::difference_type difference_type;
        typedef T value_type;
        typedef A array_type;
        typedef const T &const_reference;
#ifndef BOOST_UBLAS_STRICT_MATRIX_SPARSE
        typedef typename detail::map_traits<A, T>::reference reference;
#else
        typedef sparse_matrix_element<self_type> reference;
#endif
        typedef const matrix_reference<const self_type> const_closure_type;
        typedef matrix_reference<self_type> closure_type;
        typedef mapped_vector<T, A> vector_temporary_type;
        typedef self_type matrix_temporary_type;
        typedef sparse_tag storage_category;
        typedef typename L::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        mapped_matrix ():
            matrix_container<self_type> (),
            size1_ (0), size2_ (0), data_ () {}
        BOOST_UBLAS_INLINE
        mapped_matrix (size_type size1, size_type size2, size_type non_zeros = 0):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), data_ () {
            detail::map_reserve (data (), restrict_capacity (non_zeros));
        }
        BOOST_UBLAS_INLINE
        mapped_matrix (const mapped_matrix &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_), data_ (m.data_) {}
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_matrix (const matrix_expression<AE> &ae, size_type non_zeros = 0):
            matrix_container<self_type> (),
            size1_ (ae ().size1 ()), size2_ (ae ().size2 ()), data_ () {
            detail::map_reserve (data (), restrict_capacity (non_zeros));
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
        BOOST_UBLAS_INLINE
        size_type nnz_capacity () const {
            return detail::map_capacity (data ());
        }
        BOOST_UBLAS_INLINE
        size_type nnz () const {
            return data (). size ();
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
    private:
        BOOST_UBLAS_INLINE
        size_type restrict_capacity (size_type non_zeros) const {
            // Guarding against overflow - thanks to Alexei Novakov for the hint.
            // non_zeros = (std::min) (non_zeros, size1_ * size2_);
            if (size1_ > 0 && non_zeros / size1_ >= size2_)
                non_zeros = size1_ * size2_;
            return non_zeros;
        }
    public:
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool preserve = true) {
            // FIXME preserve unimplemented
            BOOST_UBLAS_CHECK (!preserve, internal_logic ());
            size1_ = size1;
            size2_ = size2;
            data ().clear ();
        }

        // Reserving
        BOOST_UBLAS_INLINE
	void reserve (size_type non_zeros, bool /*preserve*/ = true) {
            detail::map_reserve (data (), restrict_capacity (non_zeros));
        }

        // Element support
        BOOST_UBLAS_INLINE
        pointer find_element (size_type i, size_type j) {
            return const_cast<pointer> (const_cast<const self_type&>(*this).find_element (i, j));
        }
        BOOST_UBLAS_INLINE
        const_pointer find_element (size_type i, size_type j) const {
            const size_type element = layout_type::element (i, size1_, j, size2_);
            const_subiterator_type it (data ().find (element));
            if (it == data ().end ())
                return 0;
            BOOST_UBLAS_CHECK ((*it).first == element, internal_logic ());   // broken map
            return &(*it).second;
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            const size_type element = layout_type::element (i, size1_, j, size2_);
            const_subiterator_type it (data ().find (element));
            if (it == data ().end ())
                return zero_;
            BOOST_UBLAS_CHECK ((*it).first == element, internal_logic ());   // broken map
            return (*it).second;
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
#ifndef BOOST_UBLAS_STRICT_MATRIX_SPARSE
            const size_type element = layout_type::element (i, size1_, j, size2_);
            std::pair<subiterator_type, bool> ii (data ().insert (typename array_type::value_type (element, value_type/*zero*/())));
            BOOST_UBLAS_CHECK ((ii.first)->first == element, internal_logic ());   // broken map
            return (ii.first)->second;
#else
            return reference (*this, i, j);
#endif
        }

        // Element assingment
        BOOST_UBLAS_INLINE
        true_reference insert_element (size_type i, size_type j, const_reference t) {
            BOOST_UBLAS_CHECK (!find_element (i, j), bad_index ());        // duplicate element
            const size_type element = layout_type::element (i, size1_, j, size2_);
            std::pair<subiterator_type, bool> ii (data ().insert (typename array_type::value_type (element, t)));
            BOOST_UBLAS_CHECK ((ii.first)->first == element, internal_logic ());   // broken map
            if (!ii.second)     // existing element
                (ii.first)->second = t;
            return (ii.first)->second;
        }
        BOOST_UBLAS_INLINE
        void erase_element (size_type i, size_type j) {
            subiterator_type it = data ().find (layout_type::element (i, size1_, j, size2_));
            if (it == data ().end ())
                return;
            data ().erase (it);
        }
        
        // Zeroing
        BOOST_UBLAS_INLINE
        void clear () {
            data ().clear ();
        }

        // Assignment
        BOOST_UBLAS_INLINE
        mapped_matrix &operator = (const mapped_matrix &m) {
            if (this != &m) {
                size1_ = m.size1_;
                size2_ = m.size2_;
                data () = m.data ();
            }
            return *this;
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        mapped_matrix &operator = (const matrix_container<C> &m) {
            resize (m ().size1 (), m ().size2 (), false);
            assign (m);
            return *this;
        }
        BOOST_UBLAS_INLINE
        mapped_matrix &assign_temporary (mapped_matrix &m) {
            swap (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_matrix &operator = (const matrix_expression<AE> &ae) {
            self_type temporary (ae, detail::map_capacity (data ()));
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_matrix &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_matrix& operator += (const matrix_expression<AE> &ae) {
            self_type temporary (*this + ae, detail::map_capacity (data ()));
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        mapped_matrix &operator += (const matrix_container<C> &m) {
            plus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_matrix &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_matrix& operator -= (const matrix_expression<AE> &ae) {
            self_type temporary (*this - ae, detail::map_capacity (data ()));
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        mapped_matrix &operator -= (const matrix_container<C> &m) {
            minus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_matrix &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        mapped_matrix& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        mapped_matrix& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (mapped_matrix &m) {
            if (this != &m) {
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                data ().swap (m.data ());
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (mapped_matrix &m1, mapped_matrix &m2) {
            m1.swap (m2);
        }

        // Iterator types
    private:
        // Use storage iterator
        typedef typename A::const_iterator const_subiterator_type;
        typedef typename A::iterator subiterator_type;

        BOOST_UBLAS_INLINE
        true_reference at_element (size_type i, size_type j) {
            const size_type element = layout_type::element (i, size1_, j, size2_);
            subiterator_type it (data ().find (element));
            BOOST_UBLAS_CHECK (it != data ().end(), bad_index ());
            BOOST_UBLAS_CHECK ((*it).first == element, internal_logic ());   // broken map
            return it->second;
        }

    public:
        class const_iterator1;
        class iterator1;
        class const_iterator2;
        class iterator2;
        typedef reverse_iterator_base1<const_iterator1> const_reverse_iterator1;
        typedef reverse_iterator_base1<iterator1> reverse_iterator1;
        typedef reverse_iterator_base2<const_iterator2> const_reverse_iterator2;
        typedef reverse_iterator_base2<iterator2> reverse_iterator2;

        // Element lookup
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        const_iterator1 find1 (int rank, size_type i, size_type j, int direction = 1) const {
            const_subiterator_type it (data ().lower_bound (layout_type::address (i, size1_, j, size2_)));
            const_subiterator_type it_end (data ().end ());
            size_type index1 = size_type (-1);
            size_type index2 = size_type (-1);
            while (rank == 1 && it != it_end) {
                index1 = layout_type::index_i ((*it).first, size1_, size2_);
                index2 = layout_type::index_j ((*it).first, size1_, size2_);
                if (direction > 0) {
                    if ((index1 >= i && index2 == j) || (i >= size1_))
                        break;
                    ++ i;
                } else /* if (direction < 0) */ {
                    if ((index1 <= i && index2 == j) || (i == 0))
                        break;
                    -- i;
                }
                it = data ().lower_bound (layout_type::address (i, size1_, j, size2_));
            }
            if (rank == 1 && index2 != j) {
                if (direction > 0)
                    i = size1_;
                else /* if (direction < 0) */
                    i = 0;
                rank = 0;
            }
            return const_iterator1 (*this, rank, i, j, it);
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        iterator1 find1 (int rank, size_type i, size_type j, int direction = 1) {
            subiterator_type it (data ().lower_bound (layout_type::address (i, size1_, j, size2_)));
            subiterator_type it_end (data ().end ());
            size_type index1 = size_type (-1);
            size_type index2 = size_type (-1);
            while (rank == 1 && it != it_end) {
                index1 = layout_type::index_i ((*it).first, size1_, size2_);
                index2 = layout_type::index_j ((*it).first, size1_, size2_);
                if (direction > 0) {
                    if ((index1 >= i && index2 == j) || (i >= size1_))
                        break;
                    ++ i;
                } else /* if (direction < 0) */ {
                    if ((index1 <= i && index2 == j) || (i == 0))
                        break;
                    -- i;
                }
                it = data ().lower_bound (layout_type::address (i, size1_, j, size2_));
            }
            if (rank == 1 && index2 != j) {
                if (direction > 0)
                    i = size1_;
                else /* if (direction < 0) */
                    i = 0;
                rank = 0;
            }
            return iterator1 (*this, rank, i, j, it);
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        const_iterator2 find2 (int rank, size_type i, size_type j, int direction = 1) const {
            const_subiterator_type it (data ().lower_bound (layout_type::address (i, size1_, j, size2_)));
            const_subiterator_type it_end (data ().end ());
            size_type index1 = size_type (-1);
            size_type index2 = size_type (-1);
            while (rank == 1 && it != it_end) {
                index1 = layout_type::index_i ((*it).first, size1_, size2_);
                index2 = layout_type::index_j ((*it).first, size1_, size2_);
                if (direction > 0) {
                    if ((index2 >= j && index1 == i) || (j >= size2_))
                        break;
                    ++ j;
                } else /* if (direction < 0) */ {
                    if ((index2 <= j && index1 == i) || (j == 0))
                        break;
                    -- j;
                }
                it = data ().lower_bound (layout_type::address (i, size1_, j, size2_));
            }
            if (rank == 1 && index1 != i) {
                if (direction > 0)
                    j = size2_;
                else /* if (direction < 0) */
                    j = 0;
                rank = 0;
            }
            return const_iterator2 (*this, rank, i, j, it);
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        iterator2 find2 (int rank, size_type i, size_type j, int direction = 1) {
            subiterator_type it (data ().lower_bound (layout_type::address (i, size1_, j, size2_)));
            subiterator_type it_end (data ().end ());
            size_type index1 = size_type (-1);
            size_type index2 = size_type (-1);
            while (rank == 1 && it != it_end) {
                index1 = layout_type::index_i ((*it).first, size1_, size2_);
                index2 = layout_type::index_j ((*it).first, size1_, size2_);
                if (direction > 0) {
                    if ((index2 >= j && index1 == i) || (j >= size2_))
                        break;
                    ++ j;
                } else /* if (direction < 0) */ {
                    if ((index2 <= j && index1 == i) || (j == 0))
                        break;
                    -- j;
                }
                it = data ().lower_bound (layout_type::address (i, size1_, j, size2_));
            }
            if (rank == 1 && index1 != i) {
                if (direction > 0)
                    j = size2_;
                else /* if (direction < 0) */
                    j = 0;
                rank = 0;
            }
            return iterator2 (*this, rank, i, j, it);
        }


        class const_iterator1:
            public container_const_reference<mapped_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename mapped_matrix::value_type value_type;
            typedef typename mapped_matrix::difference_type difference_type;
            typedef typename mapped_matrix::const_reference reference;
            typedef const typename mapped_matrix::pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), rank_ (), i_ (), j_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, int rank, size_type i, size_type j, const const_subiterator_type &it):
                container_const_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), rank_ (it.rank_), i_ (it.i_), j_ (it.j_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_i ())
                    ++ it_;
                else
                    *this = (*this) ().find1 (rank_, index1 () + 1, j_, 1);
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                if (rank_ == 1 && layout_type::fast_i ())
                    -- it_;
                else
                    *this = (*this) ().find1 (rank_, index1 () - 1, j_, -1);
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*it_).second;
                } else {
                    return (*this) () (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 begin () const {
                const self_type &m = (*this) ();
                return m.find2 (1, index1 (), 0);
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
                const self_type &m = (*this) ();
                return m.find2 (1, index1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (*this != (*this) ().find1 (0, (*this) ().size1 (), j_), bad_index ());
                if (rank_ == 1) {
                    const self_type &m = (*this) ();
                    BOOST_UBLAS_CHECK (layout_type::index_i ((*it_).first, m.size1 (), m.size2 ()) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_i ((*it_).first, m.size1 (), m.size2 ());
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                if (rank_ == 1) {
                    const self_type &m = (*this) ();
                    BOOST_UBLAS_CHECK (layout_type::index_j ((*it_).first, m.size1 (), m.size2 ()) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_j ((*it_).first, m.size1 (), m.size2 ());
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator1 &operator = (const const_iterator1 &it) {
                container_const_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            const_subiterator_type it_;
        };

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

        class iterator1:
            public container_reference<mapped_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               iterator1, value_type> {
        public:
            typedef typename mapped_matrix::value_type value_type;
            typedef typename mapped_matrix::difference_type difference_type;
            typedef typename mapped_matrix::true_reference reference;
            typedef typename mapped_matrix::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), rank_ (), i_ (), j_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, int rank, size_type i, size_type j, const subiterator_type &it):
                container_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_i ())
                    ++ it_;
                else
                    *this = (*this) ().find1 (rank_, index1 () + 1, j_, 1);
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                if (rank_ == 1 && layout_type::fast_i ())
                    -- it_;
                else
                    *this = (*this) ().find1 (rank_, index1 () - 1, j_, -1);
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*it_).second;
                } else {
                    return (*this) ().at_element (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 begin () const {
                self_type &m = (*this) ();
                return m.find2 (1, index1 (), 0);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 end () const {
                self_type &m = (*this) ();
                return m.find2 (1, index1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (*this != (*this) ().find1 (0, (*this) ().size1 (), j_), bad_index ());
                if (rank_ == 1) {
                    const self_type &m = (*this) ();
                    BOOST_UBLAS_CHECK (layout_type::index_i ((*it_).first, m.size1 (), m.size2 ()) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_i ((*it_).first, m.size1 (), m.size2 ());
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                if (rank_ == 1) {
                    const self_type &m = (*this) ();
                    BOOST_UBLAS_CHECK (layout_type::index_j ((*it_).first, m.size1 (), m.size2 ()) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_j ((*it_).first, m.size1 (), m.size2 ());
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator1 &operator = (const iterator1 &it) {
                container_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            subiterator_type it_;

            friend class const_iterator1;
        };

        BOOST_UBLAS_INLINE
        iterator1 begin1 () {
            return find1 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator1 end1 () {
            return find1 (0, size1_, 0);
        }

        class const_iterator2:
            public container_const_reference<mapped_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename mapped_matrix::value_type value_type;
            typedef typename mapped_matrix::difference_type difference_type;
            typedef typename mapped_matrix::const_reference reference;
            typedef const typename mapped_matrix::pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), rank_ (), i_ (), j_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, int rank, size_type i, size_type j, const const_subiterator_type &it):
                container_const_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), rank_ (it.rank_), i_ (it.i_), j_ (it.j_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_j ())
                    ++ it_;
                else
                    *this = (*this) ().find2 (rank_, i_, index2 () + 1, 1);
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                if (rank_ == 1 && layout_type::fast_j ())
                    -- it_;
                else
                    *this = (*this) ().find2 (rank_, i_, index2 () - 1, -1);
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*it_).second;
                } else {
                    return (*this) () (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 begin () const {
                const self_type &m = (*this) ();
                return m.find1 (1, 0, index2 ());
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
                const self_type &m = (*this) ();
                return m.find1 (1, m.size1 (), index2 ());
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
                if (rank_ == 1) {
                    const self_type &m = (*this) ();
                    BOOST_UBLAS_CHECK (layout_type::index_i ((*it_).first, m.size1 (), m.size2 ()) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_i ((*it_).first, m.size1 (), m.size2 ());
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK (*this != (*this) ().find2 (0, i_, (*this) ().size2 ()), bad_index ());
                if (rank_ == 1) {
                    const self_type &m = (*this) ();
                    BOOST_UBLAS_CHECK (layout_type::index_j ((*it_).first, m.size1 (), m.size2 ()) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_j ((*it_).first, m.size1 (), m.size2 ());
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator2 &operator = (const const_iterator2 &it) {
                container_const_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            const_subiterator_type it_;
        };

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

        class iterator2:
            public container_reference<mapped_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               iterator2, value_type> {
        public:
            typedef typename mapped_matrix::value_type value_type;
            typedef typename mapped_matrix::difference_type difference_type;
            typedef typename mapped_matrix::true_reference reference;
            typedef typename mapped_matrix::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), rank_ (), i_ (), j_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, int rank, size_type i, size_type j, const subiterator_type &it):
                container_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_j ())
                    ++ it_;
                else
                    *this = (*this) ().find2 (rank_, i_, index2 () + 1, 1);
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                if (rank_ == 1 && layout_type::fast_j ())
                    -- it_;
                else
                    *this = (*this) ().find2 (rank_, i_, index2 () - 1, -1);
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*it_).second;
                } else {
                    return (*this) ().at_element (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 begin () const {
                self_type &m = (*this) ();
                return m.find1 (1, 0, index2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 end () const {
                self_type &m = (*this) ();
                return m.find1 (1, m.size1 (), index2 ());
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
                if (rank_ == 1) {
                    const self_type &m = (*this) ();
                    BOOST_UBLAS_CHECK (layout_type::index_i ((*it_).first, m.size1 (), m.size2 ()) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_i ((*it_).first, m.size1 (), m.size2 ());
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK (*this != (*this) ().find2 (0, i_, (*this) ().size2 ()), bad_index ());
                if (rank_ == 1) {
                    const self_type &m = (*this) ();
                    BOOST_UBLAS_CHECK (layout_type::index_j ((*it_).first, m.size1 (), m.size2 ()) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_j ((*it_).first, m.size1 (), m.size2 ());
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator2 &operator = (const iterator2 &it) {
                container_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            subiterator_type it_;

            friend class const_iterator2;
        };

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

         // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
            ar & serialization::make_nvp("size1",s1);
            ar & serialization::make_nvp("size2",s2);
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
            }
            ar & serialization::make_nvp("data", data_);
        }

    private:
        size_type size1_;
        size_type size2_;
        array_type data_;
        static const value_type zero_;
    };

    template<class T, class L, class A>
    const typename mapped_matrix<T, L, A>::value_type mapped_matrix<T, L, A>::zero_ = value_type/*zero*/();


    // Vector index map based sparse matrix class
    template<class T, class L, class A>
    class mapped_vector_of_mapped_vector:
        public matrix_container<mapped_vector_of_mapped_vector<T, L, A> > {

        typedef T &true_reference;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef A array_type;
        typedef const A const_array_type;
        typedef L layout_type;
        typedef mapped_vector_of_mapped_vector<T, L, A> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_container<self_type>::operator ();
#endif
        typedef typename A::size_type size_type;
        typedef typename A::difference_type difference_type;
        typedef T value_type;
        typedef const T &const_reference;
#ifndef BOOST_UBLAS_STRICT_MATRIX_SPARSE
        typedef typename detail::map_traits<typename A::data_value_type, T>::reference reference;
#else
        typedef sparse_matrix_element<self_type> reference;
#endif
        typedef const matrix_reference<const self_type> const_closure_type;
        typedef matrix_reference<self_type> closure_type;
        typedef mapped_vector<T> vector_temporary_type;
        typedef self_type matrix_temporary_type;
        typedef typename A::value_type::second_type vector_data_value_type;
        typedef sparse_tag storage_category;
        typedef typename L::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector ():
            matrix_container<self_type> (),
            size1_ (0), size2_ (0), data_ () {
            data_ [layout_type::size_M (size1_, size2_)] = vector_data_value_type ();
        }
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector (size_type size1, size_type size2, size_type /*non_zeros*/ = 0):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), data_ () {
            data_ [layout_type::size_M (size1_, size2_)] = vector_data_value_type ();
        }
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector (const mapped_vector_of_mapped_vector &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_), data_ (m.data_) {}
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector (const matrix_expression<AE> &ae, size_type /*non_zeros*/ = 0):
            matrix_container<self_type> (),
            size1_ (ae ().size1 ()), size2_ (ae ().size2 ()), data_ () {
            data_ [layout_type::size_M (size1_, size2_)] = vector_data_value_type ();
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
        BOOST_UBLAS_INLINE
        size_type nnz_capacity () const {
            size_type non_zeros = 0;
            for (vector_const_subiterator_type itv = data_ ().begin (); itv != data_ ().end (); ++ itv)
                non_zeros += detail::map_capacity (*itv);
            return non_zeros;
        }
        BOOST_UBLAS_INLINE
        size_type nnz () const {
            size_type filled = 0;
            for (vector_const_subiterator_type itv = data_ ().begin (); itv != data_ ().end (); ++ itv)
                filled += (*itv).size ();
            return filled;
        }

        // Storage accessors
        BOOST_UBLAS_INLINE
        const_array_type &data () const {
            return data_;
        }
        BOOST_UBLAS_INLINE
        array_type &data () {
            return data_;
        }

        // Resizing
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool preserve = true) {
            // FIXME preserve unimplemented
            BOOST_UBLAS_CHECK (!preserve, internal_logic ());
            size1_ = size1;
            size2_ = size2;
            data ().clear ();
            data () [layout_type::size_M (size1_, size2_)] = vector_data_value_type ();
        }

        // Element support
        BOOST_UBLAS_INLINE
        pointer find_element (size_type i, size_type j) {
            return const_cast<pointer> (const_cast<const self_type&>(*this).find_element (i, j));
        }
        BOOST_UBLAS_INLINE
        const_pointer find_element (size_type i, size_type j) const {
            const size_type element1 = layout_type::index_M (i, j);
            const size_type element2 = layout_type::index_m (i, j);
            vector_const_subiterator_type itv (data ().find (element1));
            if (itv == data ().end ())
                return 0;
            BOOST_UBLAS_CHECK ((*itv).first == element1, internal_logic ());   // broken map
            const_subiterator_type it ((*itv).second.find (element2));
            if (it == (*itv).second.end ())
                return 0;
            BOOST_UBLAS_CHECK ((*it).first == element2, internal_logic ());   // broken map
            return &(*it).second;
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            const size_type element1 = layout_type::index_M (i, j);
            const size_type element2 = layout_type::index_m (i, j);
            vector_const_subiterator_type itv (data ().find (element1));
            if (itv == data ().end ())
                return zero_;
            BOOST_UBLAS_CHECK ((*itv).first == element1, internal_logic ());   // broken map
            const_subiterator_type it ((*itv).second.find (element2));
            if (it == (*itv).second.end ())
                return zero_;
            BOOST_UBLAS_CHECK ((*itv).first == element1, internal_logic ());   // broken map
            return (*it).second;
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
#ifndef BOOST_UBLAS_STRICT_MATRIX_SPARSE
            const size_type element1 = layout_type::index_M (i, j);
            const size_type element2 = layout_type::index_m (i, j);
            vector_data_value_type& vd (data () [element1]);
            std::pair<subiterator_type, bool> ii (vd.insert (typename array_type::value_type::second_type::value_type (element2, value_type/*zero*/())));
            BOOST_UBLAS_CHECK ((ii.first)->first == element2, internal_logic ());   // broken map
            return (ii.first)->second;
#else
            return reference (*this, i, j);
#endif
        }

        // Element assignment
        BOOST_UBLAS_INLINE
        true_reference insert_element (size_type i, size_type j, const_reference t) {
            BOOST_UBLAS_CHECK (!find_element (i, j), bad_index ());        // duplicate element
            const size_type element1 = layout_type::index_M (i, j);
            const size_type element2 = layout_type::index_m (i, j);

            vector_data_value_type& vd (data () [element1]);
            std::pair<subiterator_type, bool> ii (vd.insert (typename vector_data_value_type::value_type (element2, t)));
            BOOST_UBLAS_CHECK ((ii.first)->first == element2, internal_logic ());   // broken map
            if (!ii.second)     // existing element
                (ii.first)->second = t;
            return (ii.first)->second;
        }
        BOOST_UBLAS_INLINE
        void erase_element (size_type i, size_type j) {
            vector_subiterator_type itv (data ().find (layout_type::index_M (i, j)));
            if (itv == data ().end ())
                return;
            subiterator_type it ((*itv).second.find (layout_type::index_m (i, j)));
            if (it == (*itv).second.end ())
                return;
            (*itv).second.erase (it);
        }
        
        // Zeroing
        BOOST_UBLAS_INLINE
        void clear () {
            data ().clear ();
            data_ [layout_type::size_M (size1_, size2_)] = vector_data_value_type ();
        }

        // Assignment
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector &operator = (const mapped_vector_of_mapped_vector &m) {
            if (this != &m) {
                size1_ = m.size1_;
                size2_ = m.size2_;
                data () = m.data ();
            }
            return *this;
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector &operator = (const matrix_container<C> &m) {
            resize (m ().size1 (), m ().size2 (), false);
            assign (m);
            return *this;
        }
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector &assign_temporary (mapped_vector_of_mapped_vector &m) {
            swap (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector &operator = (const matrix_expression<AE> &ae) {
            self_type temporary (ae);
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector& operator += (const matrix_expression<AE> &ae) {
            self_type temporary (*this + ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector &operator += (const matrix_container<C> &m) {
            plus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector& operator -= (const matrix_expression<AE> &ae) {
            self_type temporary (*this - ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector &operator -= (const matrix_container<C> &m) {
            minus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        mapped_vector_of_mapped_vector& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (mapped_vector_of_mapped_vector &m) {
            if (this != &m) {
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                data ().swap (m.data ());
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (mapped_vector_of_mapped_vector &m1, mapped_vector_of_mapped_vector &m2) {
            m1.swap (m2);
        }

        // Iterator types
    private:
        // Use storage iterators
        typedef typename A::const_iterator vector_const_subiterator_type;
        typedef typename A::iterator vector_subiterator_type;
        typedef typename A::value_type::second_type::const_iterator const_subiterator_type;
        typedef typename A::value_type::second_type::iterator subiterator_type;

        BOOST_UBLAS_INLINE
        true_reference at_element (size_type i, size_type j) {
            const size_type element1 = layout_type::index_M (i, j);
            const size_type element2 = layout_type::index_m (i, j);
            vector_subiterator_type itv (data ().find (element1));
            BOOST_UBLAS_CHECK (itv != data ().end(), bad_index ());
            BOOST_UBLAS_CHECK ((*itv).first == element1, internal_logic ());   // broken map
            subiterator_type it ((*itv).second.find (element2));
            BOOST_UBLAS_CHECK (it != (*itv).second.end (), bad_index ());
            BOOST_UBLAS_CHECK ((*it).first == element2, internal_logic ());    // broken map
            
            return it->second;
        }

    public:
        class const_iterator1;
        class iterator1;
        class const_iterator2;
        class iterator2;
        typedef reverse_iterator_base1<const_iterator1> const_reverse_iterator1;
        typedef reverse_iterator_base1<iterator1> reverse_iterator1;
        typedef reverse_iterator_base2<const_iterator2> const_reverse_iterator2;
        typedef reverse_iterator_base2<iterator2> reverse_iterator2;

        // Element lookup
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        const_iterator1 find1 (int rank, size_type i, size_type j, int direction = 1) const {
            BOOST_UBLAS_CHECK (data ().begin () != data ().end (), internal_logic ());
            for (;;) {
                vector_const_subiterator_type itv (data ().lower_bound (layout_type::index_M (i, j)));
                vector_const_subiterator_type itv_end (data ().end ());
                if (itv == itv_end)
                    return const_iterator1 (*this, rank, i, j, itv_end, (*(-- itv)).second.end ());

                const_subiterator_type it ((*itv).second.lower_bound (layout_type::index_m (i, j)));
                const_subiterator_type it_end ((*itv).second.end ());
                if (rank == 0) {
                    // advance to the first available major index
                    size_type M = itv->first;
                    size_type m;
                    if (it != it_end) { 
                        m = it->first; 
                    } else {
                        m = layout_type::size_m(size1_, size2_);
                    }
                    size_type first_i = layout_type::index_M(M,m);
                    return const_iterator1 (*this, rank, first_i, j, itv, it);
                }
                if (it != it_end && (*it).first == layout_type::index_m (i, j))
                    return const_iterator1 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_i ()) {
                        if (it == it_end)
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        i = (*it).first;
                    } else {
                        if (i >= size1_)
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        ++ i;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_i ()) {
                        if (it == (*itv).second.begin ())
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        -- it;
                        i = (*it).first;
                    } else {
                        if (i == 0)
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        -- i;
                    }
                }
            }
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        iterator1 find1 (int rank, size_type i, size_type j, int direction = 1) {
            BOOST_UBLAS_CHECK (data ().begin () != data ().end (), internal_logic ());
            for (;;) {
                vector_subiterator_type itv (data ().lower_bound (layout_type::index_M (i, j)));
                vector_subiterator_type itv_end (data ().end ());
                if (itv == itv_end)
                    return iterator1 (*this, rank, i, j, itv_end, (*(-- itv)).second.end ());

                subiterator_type it ((*itv).second.lower_bound (layout_type::index_m (i, j)));
                subiterator_type it_end ((*itv).second.end ());
                if (rank == 0) {
                    // advance to the first available major index
                    size_type M = itv->first;
                    size_type m;
                    if (it != it_end) { 
                        m = it->first; 
                    } else {
                        m = layout_type::size_m(size1_, size2_);
                    }
                    size_type first_i = layout_type::index_M(M,m);
                    return iterator1 (*this, rank, first_i, j, itv, it);
                }
                if (it != it_end && (*it).first == layout_type::index_m (i, j))
                    return iterator1 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_i ()) {
                        if (it == it_end)
                            return iterator1 (*this, rank, i, j, itv, it);
                        i = (*it).first;
                    } else {
                        if (i >= size1_)
                            return iterator1 (*this, rank, i, j, itv, it);
                        ++ i;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_i ()) {
                        if (it == (*itv).second.begin ())
                            return iterator1 (*this, rank, i, j, itv, it);
                        -- it;
                        i = (*it).first;
                    } else {
                        if (i == 0)
                            return iterator1 (*this, rank, i, j, itv, it);
                        -- i;
                    }
                }
            }
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        const_iterator2 find2 (int rank, size_type i, size_type j, int direction = 1) const {
            BOOST_UBLAS_CHECK (data ().begin () != data ().end (), internal_logic ());
            for (;;) {
                vector_const_subiterator_type itv (data ().lower_bound (layout_type::index_M (i, j)));
                vector_const_subiterator_type itv_end (data ().end ());
                if (itv == itv_end)
                    return const_iterator2 (*this, rank, i, j, itv_end, (*(-- itv)).second.end ());

                const_subiterator_type it ((*itv).second.lower_bound (layout_type::index_m (i, j)));
                const_subiterator_type it_end ((*itv).second.end ());
                if (rank == 0) {
                    // advance to the first available major index
                    size_type M = itv->first;
                    size_type m;
                    if (it != it_end) { 
                        m = it->first; 
                    } else {
                        m = layout_type::size_m(size1_, size2_);
                    }
                    size_type first_j = layout_type::index_m(M,m);
                    return const_iterator2 (*this, rank, i, first_j, itv, it);
                }
                if (it != it_end && (*it).first == layout_type::index_m (i, j))
                    return const_iterator2 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_j ()) {
                        if (it == it_end)
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        j = (*it).first;
                    } else {
                        if (j >= size2_)
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        ++ j;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_j ()) {
                        if (it == (*itv).second.begin ())
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        -- it;
                        j = (*it).first;
                    } else {
                        if (j == 0)
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        -- j;
                    }
                }
            }
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        iterator2 find2 (int rank, size_type i, size_type j, int direction = 1) {
            BOOST_UBLAS_CHECK (data ().begin () != data ().end (), internal_logic ());
            for (;;) {
                vector_subiterator_type itv (data ().lower_bound (layout_type::index_M (i, j)));
                vector_subiterator_type itv_end (data ().end ());
                if (itv == itv_end)
                    return iterator2 (*this, rank, i, j, itv_end, (*(-- itv)).second.end ());

                subiterator_type it ((*itv).second.lower_bound (layout_type::index_m (i, j)));
                subiterator_type it_end ((*itv).second.end ());
                if (rank == 0) {
                    // advance to the first available major index
                    size_type M = itv->first;
                    size_type m;
                    if (it != it_end) { 
                        m = it->first; 
                    } else {
                        m = layout_type::size_m(size1_, size2_);
                    }
                    size_type first_j = layout_type::index_m(M,m);
                    return iterator2 (*this, rank, i, first_j, itv, it);
                }
                if (it != it_end && (*it).first == layout_type::index_m (i, j))
                    return iterator2 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_j ()) {
                        if (it == it_end)
                            return iterator2 (*this, rank, i, j, itv, it);
                        j = (*it).first;
                    } else {
                        if (j >= size2_)
                            return iterator2 (*this, rank, i, j, itv, it);
                        ++ j;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_j ()) {
                        if (it == (*itv).second.begin ())
                            return iterator2 (*this, rank, i, j, itv, it);
                        -- it;
                        j = (*it).first;
                    } else {
                        if (j == 0)
                            return iterator2 (*this, rank, i, j, itv, it);
                        -- j;
                    }
                }
            }
        }

        class const_iterator1:
            public container_const_reference<mapped_vector_of_mapped_vector>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename mapped_vector_of_mapped_vector::value_type value_type;
            typedef typename mapped_vector_of_mapped_vector::difference_type difference_type;
            typedef typename mapped_vector_of_mapped_vector::const_reference reference;
            typedef const typename mapped_vector_of_mapped_vector::pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, int rank, size_type i, size_type j, const vector_const_subiterator_type &itv, const const_subiterator_type &it):
                container_const_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), rank_ (it.rank_), i_ (it.i_), j_ (it.j_), itv_ (it.itv_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_i ())
                    ++ it_;
                else {
                    const self_type &m = (*this) ();
                    if (rank_ == 0) {
                        ++ itv_;
                        i_ = itv_->first;
                    } else {
                        i_ = index1 () + 1;
                    }
                    if (rank_ == 1 && ++ itv_ == m.end1 ().itv_)
                        *this = m.find1 (rank_, i_, j_, 1);
                    else if (rank_ == 1) {
                        it_ = (*itv_).second.begin ();
                        if (it_ == (*itv_).second.end () || index2 () != j_)
                            *this = m.find1 (rank_, i_, j_, 1);
                    }
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                if (rank_ == 1 && layout_type::fast_i ())
                    -- it_;
                else {
                    const self_type &m = (*this) ();
                    if (rank_ == 0) {
                        -- itv_;
                        i_ = itv_->first;
                    } else {
                        i_ = index1 () - 1;
                    }
                    // FIXME: this expression should never become true!
                    if (rank_ == 1 && -- itv_ == m.end1 ().itv_)
                        *this = m.find1 (rank_, i_, j_, -1);
                    else if (rank_ == 1) {
                        it_ = (*itv_).second.begin ();
                        if (it_ == (*itv_).second.end () || index2 () != j_)
                            *this = m.find1 (rank_, i_, j_, -1);
                    }
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*it_).second;
                } else {
                    return (*this) () (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 begin () const {
                const self_type &m = (*this) ();
                return m.find2 (1, index1 (), 0);
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
                const self_type &m = (*this) ();
                return m.find2 (1, index1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (*this != (*this) ().find1 (0, (*this) ().size1 (), j_), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M ((*itv_).first, (*it_).first) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M ((*itv_).first, (*it_).first);
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m ((*itv_).first, (*it_).first) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m ((*itv_).first, (*it_).first);
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator1 &operator = (const const_iterator1 &it) {
                container_const_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_const_subiterator_type itv_;
            const_subiterator_type it_;
        };

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

        class iterator1:
            public container_reference<mapped_vector_of_mapped_vector>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               iterator1, value_type> {
        public:
            typedef typename mapped_vector_of_mapped_vector::value_type value_type;
            typedef typename mapped_vector_of_mapped_vector::difference_type difference_type;
            typedef typename mapped_vector_of_mapped_vector::true_reference reference;
            typedef typename mapped_vector_of_mapped_vector::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, int rank, size_type i, size_type j, const vector_subiterator_type &itv, const subiterator_type &it):
                container_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_i ())
                    ++ it_;
                else {
                    self_type &m = (*this) ();
                    if (rank_ == 0) {
                        ++ itv_;
                        i_ = itv_->first;
                    } else {
                        i_ = index1 () + 1;
                    }
                    if (rank_ == 1 && ++ itv_ == m.end1 ().itv_)
                        *this = m.find1 (rank_, i_, j_, 1);
                    else if (rank_ == 1) {
                        it_ = (*itv_).second.begin ();
                        if (it_ == (*itv_).second.end () || index2 () != j_)
                            *this = m.find1 (rank_, i_, j_, 1);
                    }
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                if (rank_ == 1 && layout_type::fast_i ())
                    -- it_;
                else {
                    self_type &m = (*this) ();
                    if (rank_ == 0) {
                        -- itv_;
                        i_ = itv_->first;
                    } else {
                        i_ = index1 () - 1;
                    }
                    // FIXME: this expression should never become true!
                    if (rank_ == 1 && -- itv_ == m.end1 ().itv_)
                        *this = m.find1 (rank_, i_, j_, -1);
                    else if (rank_ == 1) {
                        it_ = (*itv_).second.begin ();
                        if (it_ == (*itv_).second.end () || index2 () != j_)
                            *this = m.find1 (rank_, i_, j_, -1);
                    }
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*it_).second;
                } else {
                    return (*this) ().at_element (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 begin () const {
                self_type &m = (*this) ();
                return m.find2 (1, index1 (), 0);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 end () const {
                self_type &m = (*this) ();
                return m.find2 (1, index1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (*this != (*this) ().find1 (0, (*this) ().size1 (), j_), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M ((*itv_).first, (*it_).first) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M ((*itv_).first, (*it_).first);
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m ((*itv_).first, (*it_).first) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m ((*itv_).first, (*it_).first);
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator1 &operator = (const iterator1 &it) {
                container_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_subiterator_type itv_;
            subiterator_type it_;

            friend class const_iterator1;
        };

        BOOST_UBLAS_INLINE
        iterator1 begin1 () {
            return find1 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator1 end1 () {
            return find1 (0, size1_, 0);
        }

        class const_iterator2:
            public container_const_reference<mapped_vector_of_mapped_vector>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename mapped_vector_of_mapped_vector::value_type value_type;
            typedef typename mapped_vector_of_mapped_vector::difference_type difference_type;
            typedef typename mapped_vector_of_mapped_vector::const_reference reference;
            typedef const typename mapped_vector_of_mapped_vector::pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, int rank, size_type i, size_type j, const vector_const_subiterator_type &itv, const const_subiterator_type &it):
                container_const_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), rank_ (it.rank_), i_ (it.i_), j_ (it.j_), itv_ (it.itv_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_j ())
                    ++ it_;
                else {
                    const self_type &m = (*this) ();
                    if (rank_ == 0) {
                        ++ itv_;
                        j_ = itv_->first;
                    } else {
                        j_ = index2 () + 1;
                    }
                    if (rank_ == 1 && ++ itv_ == m.end2 ().itv_)
                        *this = m.find2 (rank_, i_, j_, 1);
                    else if (rank_ == 1) {
                        it_ = (*itv_).second.begin ();
                        if (it_ == (*itv_).second.end () || index1 () != i_)
                            *this = m.find2 (rank_, i_, j_, 1);
                    }
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                if (rank_ == 1 && layout_type::fast_j ())
                    -- it_;
                else {
                    const self_type &m = (*this) ();
                    if (rank_ == 0) {
                        -- itv_;
                        j_ = itv_->first;
                    } else {
                        j_ = index2 () - 1;
                    }
                    // FIXME: this expression should never become true!
                    if (rank_ == 1 && -- itv_ == m.end2 ().itv_)
                        *this = m.find2 (rank_, i_, j_, -1);
                    else if (rank_ == 1) {
                        it_ = (*itv_).second.begin ();
                        if (it_ == (*itv_).second.end () || index1 () != i_)
                            *this = m.find2 (rank_, i_, j_, -1);
                    }
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*it_).second;
                } else {
                    return (*this) () (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 begin () const {
                const self_type &m = (*this) ();
                return m.find1 (1, 0, index2 ());
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
                const self_type &m = (*this) ();
                return m.find1 (1, m.size1 (), index2 ());
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
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M ((*itv_).first, (*it_).first) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M ((*itv_).first, (*it_).first);
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK (*this != (*this) ().find2 (0, i_, (*this) ().size2 ()), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m ((*itv_).first, (*it_).first) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m ((*itv_).first, (*it_).first);
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator2 &operator = (const const_iterator2 &it) {
                container_const_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_const_subiterator_type itv_;
            const_subiterator_type it_;
        };

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

        class iterator2:
            public container_reference<mapped_vector_of_mapped_vector>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               iterator2, value_type> {
        public:
            typedef typename mapped_vector_of_mapped_vector::value_type value_type;
            typedef typename mapped_vector_of_mapped_vector::difference_type difference_type;
            typedef typename mapped_vector_of_mapped_vector::true_reference reference;
            typedef typename mapped_vector_of_mapped_vector::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, int rank, size_type i, size_type j, const vector_subiterator_type &itv, const subiterator_type &it):
                container_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_j ())
                    ++ it_;
                else {
                    self_type &m = (*this) ();
                    if (rank_ == 0) {
                        ++ itv_;
                        j_ = itv_->first;
                    } else {
                        j_ = index2 () + 1;
                    }
                    if (rank_ == 1 && ++ itv_ == m.end2 ().itv_)
                        *this = m.find2 (rank_, i_, j_, 1);
                    else if (rank_ == 1) {
                        it_ = (*itv_).second.begin ();
                        if (it_ == (*itv_).second.end () || index1 () != i_)
                            *this = m.find2 (rank_, i_, j_, 1);
                    }
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                if (rank_ == 1 && layout_type::fast_j ())
                    -- it_;
                else {
                    self_type &m = (*this) ();
                    if (rank_ == 0) {
                        -- itv_;
                        j_ = itv_->first;
                    } else {
                        j_ = index2 () - 1;
                    }
                    // FIXME: this expression should never become true!
                    if (rank_ == 1 && -- itv_ == m.end2 ().itv_)
                        *this = m.find2 (rank_, i_, j_, -1);
                    else if (rank_ == 1) {
                        it_ = (*itv_).second.begin ();
                        if (it_ == (*itv_).second.end () || index1 () != i_)
                            *this = m.find2 (rank_, i_, j_, -1);
                    }
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*it_).second;
                } else {
                    return (*this) ().at_element (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 begin () const {
                self_type &m = (*this) ();
                return m.find1 (1, 0, index2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 end () const {
                self_type &m = (*this) ();
                return m.find1 (1, m.size1 (), index2 ());
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
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M ((*itv_).first, (*it_).first) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M ((*itv_).first, (*it_).first);
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK (*this != (*this) ().find2 (0, i_, (*this) ().size2 ()), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m ((*itv_).first, (*it_).first) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m ((*itv_).first, (*it_).first);
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator2 &operator = (const iterator2 &it) {
                container_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_subiterator_type itv_;
            subiterator_type it_;

            friend class const_iterator2;
        };

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

         // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
            ar & serialization::make_nvp("size1",s1);
            ar & serialization::make_nvp("size2",s2);
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
            }
            ar & serialization::make_nvp("data", data_);
        }

    private:
        size_type size1_;
        size_type size2_;
        array_type data_;
        static const value_type zero_;
    };

    template<class T, class L, class A>
    const typename mapped_vector_of_mapped_vector<T, L, A>::value_type mapped_vector_of_mapped_vector<T, L, A>::zero_ = value_type/*zero*/();


    // Comperssed array based sparse matrix class
    // Thanks to Kresimir Fresl for extending this to cover different index bases.
    template<class T, class L, std::size_t IB, class IA, class TA>
    class compressed_matrix:
        public matrix_container<compressed_matrix<T, L, IB, IA, TA> > {

        typedef T &true_reference;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef L layout_type;
        typedef compressed_matrix<T, L, IB, IA, TA> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_container<self_type>::operator ();
#endif
        // ISSUE require type consistency check
        // is_convertable (IA::size_type, TA::size_type)
        typedef typename IA::value_type size_type;
        // size_type for the data arrays.
        typedef typename IA::size_type array_size_type;
        // FIXME difference type for sparse storage iterators should it be in the container?
        typedef typename IA::difference_type difference_type;
        typedef T value_type;
        typedef const T &const_reference;
#ifndef BOOST_UBLAS_STRICT_MATRIX_SPARSE
        typedef T &reference;
#else
        typedef sparse_matrix_element<self_type> reference;
#endif
        typedef IA index_array_type;
        typedef TA value_array_type;
        typedef const matrix_reference<const self_type> const_closure_type;
        typedef matrix_reference<self_type> closure_type;
        typedef compressed_vector<T, IB, IA, TA> vector_temporary_type;
        typedef self_type matrix_temporary_type;
        typedef sparse_tag storage_category;
        typedef typename L::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        compressed_matrix ():
            matrix_container<self_type> (),
            size1_ (0), size2_ (0), capacity_ (restrict_capacity (0)),
            filled1_ (1), filled2_ (0),
            index1_data_ (layout_type::size_M (size1_, size2_) + 1), index2_data_ (capacity_), value_data_ (capacity_) {
            index1_data_ [filled1_ - 1] = k_based (filled2_);
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        compressed_matrix (size_type size1, size_type size2, size_type non_zeros = 0):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), capacity_ (restrict_capacity (non_zeros)),
            filled1_ (1), filled2_ (0),
            index1_data_ (layout_type::size_M (size1_, size2_) + 1), index2_data_ (capacity_), value_data_ (capacity_) {
            index1_data_ [filled1_ - 1] = k_based (filled2_);
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        compressed_matrix (const compressed_matrix &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_), capacity_ (m.capacity_),
            filled1_ (m.filled1_), filled2_ (m.filled2_),
            index1_data_ (m.index1_data_), index2_data_ (m.index2_data_), value_data_ (m.value_data_) {
            storage_invariants ();
        }
         
        BOOST_UBLAS_INLINE
        compressed_matrix (const coordinate_matrix<T, L, IB, IA, TA> &m):
            matrix_container<self_type> (),
            size1_ (m.size1()), size2_ (m.size2()),
            index1_data_ (layout_type::size_M (size1_, size2_) + 1)
        {
            m.sort();
            reserve(m.nnz(), false);
            filled2_ = m.nnz();
            const_subiterator_type  i_start = m.index1_data().begin();
            const_subiterator_type  i_end   = (i_start + filled2_);
            const_subiterator_type  i = i_start;
            size_type r = 1;
            for (; (r < layout_type::size_M (size1_, size2_)) && (i != i_end); ++r) {
                i = std::lower_bound(i, i_end, r);
                index1_data_[r] = k_based( i - i_start );
            }
            filled1_ = r + 1;
            std::copy( m.index2_data().begin(), m.index2_data().begin() + filled2_, index2_data_.begin());
            std::copy( m.value_data().begin(), m.value_data().begin() + filled2_, value_data_.begin());
            index1_data_ [filled1_ - 1] = k_based(filled2_);
            storage_invariants ();
        }

       template<class AE>
       BOOST_UBLAS_INLINE
       compressed_matrix (const matrix_expression<AE> &ae, size_type non_zeros = 0):
            matrix_container<self_type> (),
            size1_ (ae ().size1 ()), size2_ (ae ().size2 ()), capacity_ (restrict_capacity (non_zeros)),
            filled1_ (1), filled2_ (0),
            index1_data_ (layout_type::size_M (ae ().size1 (), ae ().size2 ()) + 1),
            index2_data_ (capacity_), value_data_ (capacity_) {
            index1_data_ [filled1_ - 1] = k_based (filled2_);
            storage_invariants ();
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
        BOOST_UBLAS_INLINE
        size_type nnz_capacity () const {
            return capacity_;
        }
        BOOST_UBLAS_INLINE
        size_type nnz () const {
            return filled2_;
        }
        
        // Storage accessors
        BOOST_UBLAS_INLINE
        static size_type index_base () {
            return IB;
        }
        BOOST_UBLAS_INLINE
        array_size_type filled1 () const {
            return filled1_;
        }
        BOOST_UBLAS_INLINE
        array_size_type filled2 () const {
            return filled2_;
        }
        BOOST_UBLAS_INLINE
        const index_array_type &index1_data () const {
            return index1_data_;
        }
        BOOST_UBLAS_INLINE
        const index_array_type &index2_data () const {
            return index2_data_;
        }
        BOOST_UBLAS_INLINE
        const value_array_type &value_data () const {
            return value_data_;
        }
        BOOST_UBLAS_INLINE
        void set_filled (const array_size_type& filled1, const array_size_type& filled2) {
            filled1_ = filled1;
            filled2_ = filled2;
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        index_array_type &index1_data () {
            return index1_data_;
        }
        BOOST_UBLAS_INLINE
        index_array_type &index2_data () {
            return index2_data_;
        }
        BOOST_UBLAS_INLINE
        value_array_type &value_data () {
            return value_data_;
        }
        BOOST_UBLAS_INLINE
        void complete_index1_data () {
            while (filled1_ <= layout_type::size_M (size1_, size2_)) {
                this->index1_data_ [filled1_] = k_based (filled2_);
                ++ this->filled1_;
            }
        }

        // Resizing
    private:
        BOOST_UBLAS_INLINE
        size_type restrict_capacity (size_type non_zeros) const {
            non_zeros = (std::max) (non_zeros, (std::min) (size1_, size2_));
            // Guarding against overflow - Thanks to Alexei Novakov for the hint.
            // non_zeros = (std::min) (non_zeros, size1_ * size2_);
            if (size1_ > 0 && non_zeros / size1_ >= size2_)
                non_zeros = size1_ * size2_;
            return non_zeros;
        }
    public:
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool preserve = true) {
            // FIXME preserve unimplemented
            BOOST_UBLAS_CHECK (!preserve, internal_logic ());
            size1_ = size1;
            size2_ = size2;
            capacity_ = restrict_capacity (capacity_);
            filled1_ = 1;
            filled2_ = 0;
            index1_data_.resize (layout_type::size_M (size1_, size2_) + 1);
            index2_data_.resize (capacity_);
            value_data_.resize (capacity_);
            index1_data_ [filled1_ - 1] = k_based (filled2_);
            storage_invariants ();
        }

        // Reserving
        BOOST_UBLAS_INLINE
        void reserve (size_type non_zeros, bool preserve = true) {
            capacity_ = restrict_capacity (non_zeros);
            if (preserve) {
                index2_data_.resize (capacity_, size_type ());
                value_data_.resize (capacity_, value_type ());
                filled2_ = (std::min) (capacity_, filled2_);
            }
            else {
                index2_data_.resize (capacity_);
                value_data_.resize (capacity_);
                filled1_ = 1;
                filled2_ = 0;
                index1_data_ [filled1_ - 1] = k_based (filled2_);
            }
            storage_invariants ();
       }

        // Element support
        BOOST_UBLAS_INLINE
        pointer find_element (size_type i, size_type j) {
            return const_cast<pointer> (const_cast<const self_type&>(*this).find_element (i, j));
        }
        BOOST_UBLAS_INLINE
        const_pointer find_element (size_type i, size_type j) const {
            size_type element1 (layout_type::index_M (i, j));
            size_type element2 (layout_type::index_m (i, j));
            if (filled1_ <= element1 + 1)
                return 0;
            vector_const_subiterator_type itv (index1_data_.begin () + element1);
            const_subiterator_type it_begin (index2_data_.begin () + zero_based (*itv));
            const_subiterator_type it_end (index2_data_.begin () + zero_based (*(itv + 1)));
            const_subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (element2), std::less<size_type> ()));
            if (it == it_end || *it != k_based (element2))
                return 0;
            return &value_data_ [it - index2_data_.begin ()];
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            const_pointer p = find_element (i, j);
            if (p)
                return *p;
            else
                return zero_;
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
#ifndef BOOST_UBLAS_STRICT_MATRIX_SPARSE
            size_type element1 (layout_type::index_M (i, j));
            size_type element2 (layout_type::index_m (i, j));
            if (filled1_ <= element1 + 1)
                return insert_element (i, j, value_type/*zero*/());
            pointer p = find_element (i, j);
            if (p)
                return *p;
            else
                return insert_element (i, j, value_type/*zero*/());
#else
            return reference (*this, i, j);
#endif
        }

        // Element assignment
        BOOST_UBLAS_INLINE
        true_reference insert_element (size_type i, size_type j, const_reference t) {
            BOOST_UBLAS_CHECK (!find_element (i, j), bad_index ());        // duplicate element
            if (filled2_ >= capacity_)
                reserve (2 * filled2_, true);
            BOOST_UBLAS_CHECK (filled2_ < capacity_, internal_logic ());
            size_type element1 = layout_type::index_M (i, j);
            size_type element2 = layout_type::index_m (i, j);
            while (filled1_ <= element1 + 1) {
                index1_data_ [filled1_] = k_based (filled2_);
                ++ filled1_;
            }
            vector_subiterator_type itv (index1_data_.begin () + element1);
            subiterator_type it_begin (index2_data_.begin () + zero_based (*itv));
            subiterator_type it_end (index2_data_.begin () + zero_based (*(itv + 1)));
            subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (element2), std::less<size_type> ()));
            typename std::iterator_traits<subiterator_type>::difference_type n = it - index2_data_.begin ();
            BOOST_UBLAS_CHECK (it == it_end || *it != k_based (element2), internal_logic ());   // duplicate bound by lower_bound
            ++ filled2_;
            it = index2_data_.begin () + n;
            std::copy_backward (it, index2_data_.begin () + filled2_ - 1, index2_data_.begin () + filled2_);
            *it = k_based (element2);
            typename value_array_type::iterator itt (value_data_.begin () + n);
            std::copy_backward (itt, value_data_.begin () + filled2_ - 1, value_data_.begin () + filled2_);
            *itt = t;
            while (element1 + 1 < filled1_) {
                ++ index1_data_ [element1 + 1];
                ++ element1;
            }
            storage_invariants ();
            return *itt;
        }
        BOOST_UBLAS_INLINE
        void erase_element (size_type i, size_type j) {
            size_type element1 = layout_type::index_M (i, j);
            size_type element2 = layout_type::index_m (i, j);
            if (element1 + 1 >= filled1_)
                return;
            vector_subiterator_type itv (index1_data_.begin () + element1);
            subiterator_type it_begin (index2_data_.begin () + zero_based (*itv));
            subiterator_type it_end (index2_data_.begin () + zero_based (*(itv + 1)));
            subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (element2), std::less<size_type> ()));
            if (it != it_end && *it == k_based (element2)) {
                typename std::iterator_traits<subiterator_type>::difference_type n = it - index2_data_.begin ();
                std::copy (it + 1, index2_data_.begin () + filled2_, it);
                typename value_array_type::iterator itt (value_data_.begin () + n);
                std::copy (itt + 1, value_data_.begin () + filled2_, itt);
                -- filled2_;
                while (index1_data_ [filled1_ - 2] > k_based (filled2_)) {
                    index1_data_ [filled1_ - 1] = 0;
                    -- filled1_;
                }
                while (element1 + 1 < filled1_) {
                    -- index1_data_ [element1 + 1];
                    ++ element1;
                }
            }
            storage_invariants ();
        }
        
        // Zeroing
        BOOST_UBLAS_INLINE
        void clear () {
            filled1_ = 1;
            filled2_ = 0;
            index1_data_ [filled1_ - 1] = k_based (filled2_);
            storage_invariants ();
        }

        // Assignment
        BOOST_UBLAS_INLINE
        compressed_matrix &operator = (const compressed_matrix &m) {
            if (this != &m) {
                size1_ = m.size1_;
                size2_ = m.size2_;
                capacity_ = m.capacity_;
                filled1_ = m.filled1_;
                filled2_ = m.filled2_;
                index1_data_ = m.index1_data_;
                index2_data_ = m.index2_data_;
                value_data_ = m.value_data_;
            }
            storage_invariants ();
            return *this;
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        compressed_matrix &operator = (const matrix_container<C> &m) {
            resize (m ().size1 (), m ().size2 (), false);
            assign (m);
            return *this;
        }
        BOOST_UBLAS_INLINE
        compressed_matrix &assign_temporary (compressed_matrix &m) {
            swap (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        compressed_matrix &operator = (const matrix_expression<AE> &ae) {
            self_type temporary (ae, capacity_);
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        compressed_matrix &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        compressed_matrix& operator += (const matrix_expression<AE> &ae) {
            self_type temporary (*this + ae, capacity_);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        compressed_matrix &operator += (const matrix_container<C> &m) {
            plus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        compressed_matrix &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        compressed_matrix& operator -= (const matrix_expression<AE> &ae) {
            self_type temporary (*this - ae, capacity_);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        compressed_matrix &operator -= (const matrix_container<C> &m) {
            minus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        compressed_matrix &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        compressed_matrix& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        compressed_matrix& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (compressed_matrix &m) {
            if (this != &m) {
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                std::swap (capacity_, m.capacity_);
                std::swap (filled1_, m.filled1_);
                std::swap (filled2_, m.filled2_);
                index1_data_.swap (m.index1_data_);
                index2_data_.swap (m.index2_data_);
                value_data_.swap (m.value_data_);
            }
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        friend void swap (compressed_matrix &m1, compressed_matrix &m2) {
            m1.swap (m2);
        }

        // Back element insertion and erasure
        BOOST_UBLAS_INLINE
        void push_back (size_type i, size_type j, const_reference t) {
            if (filled2_ >= capacity_)
                reserve (2 * filled2_, true);
            BOOST_UBLAS_CHECK (filled2_ < capacity_, internal_logic ());
            size_type element1 = layout_type::index_M (i, j);
            size_type element2 = layout_type::index_m (i, j);
            while (filled1_ < element1 + 2) {
                index1_data_ [filled1_] = k_based (filled2_);
                ++ filled1_;
            }
            // must maintain sort order
            BOOST_UBLAS_CHECK ((filled1_ == element1 + 2 &&
                                (filled2_ == zero_based (index1_data_ [filled1_ - 2]) ||
                                index2_data_ [filled2_ - 1] < k_based (element2))), external_logic ());
            ++ filled2_;
            index1_data_ [filled1_ - 1] = k_based (filled2_);
            index2_data_ [filled2_ - 1] = k_based (element2);
            value_data_ [filled2_ - 1] = t;
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        void pop_back () {
            BOOST_UBLAS_CHECK (filled1_ > 0 && filled2_ > 0, external_logic ());
            -- filled2_;
            while (index1_data_ [filled1_ - 2] > k_based (filled2_)) {
                index1_data_ [filled1_ - 1] = 0;
                -- filled1_;
            }
            -- index1_data_ [filled1_ - 1];
            storage_invariants ();
        }

        // Iterator types
    private:
        // Use index array iterator
        typedef typename IA::const_iterator vector_const_subiterator_type;
        typedef typename IA::iterator vector_subiterator_type;
        typedef typename IA::const_iterator const_subiterator_type;
        typedef typename IA::iterator subiterator_type;

        BOOST_UBLAS_INLINE
        true_reference at_element (size_type i, size_type j) {
            pointer p = find_element (i, j);
            BOOST_UBLAS_CHECK (p, bad_index ());
            return *p;
        }

    public:
        class const_iterator1;
        class iterator1;
        class const_iterator2;
        class iterator2;
        typedef reverse_iterator_base1<const_iterator1> const_reverse_iterator1;
        typedef reverse_iterator_base1<iterator1> reverse_iterator1;
        typedef reverse_iterator_base2<const_iterator2> const_reverse_iterator2;
        typedef reverse_iterator_base2<iterator2> reverse_iterator2;

        // Element lookup
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        const_iterator1 find1 (int rank, size_type i, size_type j, int direction = 1) const {
            for (;;) {
                array_size_type address1 (layout_type::index_M (i, j));
                array_size_type address2 (layout_type::index_m (i, j));
                vector_const_subiterator_type itv (index1_data_.begin () + (std::min) (filled1_ - 1, address1));
                if (filled1_ <= address1 + 1)
                    return const_iterator1 (*this, rank, i, j, itv, index2_data_.begin () + filled2_);

                const_subiterator_type it_begin (index2_data_.begin () + zero_based (*itv));
                const_subiterator_type it_end (index2_data_.begin () + zero_based (*(itv + 1)));

                const_subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (address2), std::less<size_type> ()));
                if (rank == 0)
                    return const_iterator1 (*this, rank, i, j, itv, it);
                if (it != it_end && zero_based (*it) == address2)
                    return const_iterator1 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_i ()) {
                        if (it == it_end)
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        i = zero_based (*it);
                    } else {
                        if (i >= size1_)
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        ++ i;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_i ()) {
                        if (it == index2_data_.begin () + zero_based (*itv))
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        i = zero_based (*(it - 1));
                    } else {
                        if (i == 0)
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        -- i;
                    }
                }
            }
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        iterator1 find1 (int rank, size_type i, size_type j, int direction = 1) {
            for (;;) {
                array_size_type address1 (layout_type::index_M (i, j));
                array_size_type address2 (layout_type::index_m (i, j));
                vector_subiterator_type itv (index1_data_.begin () + (std::min) (filled1_ - 1, address1));
                if (filled1_ <= address1 + 1)
                    return iterator1 (*this, rank, i, j, itv, index2_data_.begin () + filled2_);

                subiterator_type it_begin (index2_data_.begin () + zero_based (*itv));
                subiterator_type it_end (index2_data_.begin () + zero_based (*(itv + 1)));

                subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (address2), std::less<size_type> ()));
                if (rank == 0)
                    return iterator1 (*this, rank, i, j, itv, it);
                if (it != it_end && zero_based (*it) == address2)
                    return iterator1 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_i ()) {
                        if (it == it_end)
                            return iterator1 (*this, rank, i, j, itv, it);
                        i = zero_based (*it);
                    } else {
                        if (i >= size1_)
                            return iterator1 (*this, rank, i, j, itv, it);
                        ++ i;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_i ()) {
                        if (it == index2_data_.begin () + zero_based (*itv))
                            return iterator1 (*this, rank, i, j, itv, it);
                        i = zero_based (*(it - 1));
                    } else {
                        if (i == 0)
                            return iterator1 (*this, rank, i, j, itv, it);
                        -- i;
                    }
                }
            }
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        const_iterator2 find2 (int rank, size_type i, size_type j, int direction = 1) const {
            for (;;) {
                array_size_type address1 (layout_type::index_M (i, j));
                array_size_type address2 (layout_type::index_m (i, j));
                vector_const_subiterator_type itv (index1_data_.begin () + (std::min) (filled1_ - 1, address1));
                if (filled1_ <= address1 + 1)
                    return const_iterator2 (*this, rank, i, j, itv, index2_data_.begin () + filled2_);

                const_subiterator_type it_begin (index2_data_.begin () + zero_based (*itv));
                const_subiterator_type it_end (index2_data_.begin () + zero_based (*(itv + 1)));

                const_subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (address2), std::less<size_type> ()));
                if (rank == 0)
                    return const_iterator2 (*this, rank, i, j, itv, it);
                if (it != it_end && zero_based (*it) == address2)
                    return const_iterator2 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_j ()) {
                        if (it == it_end)
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        j = zero_based (*it);
                    } else {
                        if (j >= size2_)
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        ++ j;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_j ()) {
                        if (it == index2_data_.begin () + zero_based (*itv))
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        j = zero_based (*(it - 1));
                    } else {
                        if (j == 0)
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        -- j;
                    }
                }
            }
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        iterator2 find2 (int rank, size_type i, size_type j, int direction = 1) {
            for (;;) {
                array_size_type address1 (layout_type::index_M (i, j));
                array_size_type address2 (layout_type::index_m (i, j));
                vector_subiterator_type itv (index1_data_.begin () + (std::min) (filled1_ - 1, address1));
                if (filled1_ <= address1 + 1)
                    return iterator2 (*this, rank, i, j, itv, index2_data_.begin () + filled2_);

                subiterator_type it_begin (index2_data_.begin () + zero_based (*itv));
                subiterator_type it_end (index2_data_.begin () + zero_based (*(itv + 1)));

                subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (address2), std::less<size_type> ()));
                if (rank == 0)
                    return iterator2 (*this, rank, i, j, itv, it);
                if (it != it_end && zero_based (*it) == address2)
                    return iterator2 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_j ()) {
                        if (it == it_end)
                            return iterator2 (*this, rank, i, j, itv, it);
                        j = zero_based (*it);
                    } else {
                        if (j >= size2_)
                            return iterator2 (*this, rank, i, j, itv, it);
                        ++ j;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_j ()) {
                        if (it == index2_data_.begin () + zero_based (*itv))
                            return iterator2 (*this, rank, i, j, itv, it);
                        j = zero_based (*(it - 1));
                    } else {
                        if (j == 0)
                            return iterator2 (*this, rank, i, j, itv, it);
                        -- j;
                    }
                }
            }
        }


        class const_iterator1:
            public container_const_reference<compressed_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename compressed_matrix::value_type value_type;
            typedef typename compressed_matrix::difference_type difference_type;
            typedef typename compressed_matrix::const_reference reference;
            typedef const typename compressed_matrix::pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, int rank, size_type i, size_type j, const vector_const_subiterator_type &itv, const const_subiterator_type &it):
                container_const_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), rank_ (it.rank_), i_ (it.i_), j_ (it.j_), itv_ (it.itv_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_i ())
                    ++ it_;
                else {
                    i_ = index1 () + 1;
                    if (rank_ == 1)
                        *this = (*this) ().find1 (rank_, i_, j_, 1);
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                if (rank_ == 1 && layout_type::fast_i ())
                    -- it_;
                else {
                    --i_;
                    if (rank_ == 1)
                        *this = (*this) ().find1 (rank_, i_, j_, -1);
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*this) ().value_data_ [it_ - (*this) ().index2_data_.begin ()];
                } else {
                    return (*this) () (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 begin () const {
                const self_type &m = (*this) ();
                return m.find2 (1, index1 (), 0);
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
                const self_type &m = (*this) ();
                return m.find2 (1, index1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (*this != (*this) ().find1 (0, (*this) ().size1 (), j_), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_)) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_));
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_)) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_));
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator1 &operator = (const const_iterator1 &it) {
                container_const_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_const_subiterator_type itv_;
            const_subiterator_type it_;
        };

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

        class iterator1:
            public container_reference<compressed_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               iterator1, value_type> {
        public:
            typedef typename compressed_matrix::value_type value_type;
            typedef typename compressed_matrix::difference_type difference_type;
            typedef typename compressed_matrix::true_reference reference;
            typedef typename compressed_matrix::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, int rank, size_type i, size_type j, const vector_subiterator_type &itv, const subiterator_type &it):
                container_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_i ())
                    ++ it_;
                else {
                    i_ = index1 () + 1;
                    if (rank_ == 1)
                        *this = (*this) ().find1 (rank_, i_, j_, 1);
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                if (rank_ == 1 && layout_type::fast_i ())
                    -- it_;
                else {
                    --i_;
                    if (rank_ == 1)
                        *this = (*this) ().find1 (rank_, i_, j_, -1);
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*this) ().value_data_ [it_ - (*this) ().index2_data_.begin ()];
                } else {
                    return (*this) ().at_element (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 begin () const {
                self_type &m = (*this) ();
                return m.find2 (1, index1 (), 0);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 end () const {
                self_type &m = (*this) ();
                return m.find2 (1, index1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (*this != (*this) ().find1 (0, (*this) ().size1 (), j_), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_)) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_));
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_)) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_));
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator1 &operator = (const iterator1 &it) {
                container_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_subiterator_type itv_;
            subiterator_type it_;

            friend class const_iterator1;
        };

        BOOST_UBLAS_INLINE
        iterator1 begin1 () {
            return find1 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator1 end1 () {
            return find1 (0, size1_, 0);
        }

        class const_iterator2:
            public container_const_reference<compressed_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename compressed_matrix::value_type value_type;
            typedef typename compressed_matrix::difference_type difference_type;
            typedef typename compressed_matrix::const_reference reference;
            typedef const typename compressed_matrix::pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, int rank, size_type i, size_type j, const vector_const_subiterator_type itv, const const_subiterator_type &it):
                container_const_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), rank_ (it.rank_), i_ (it.i_), j_ (it.j_), itv_ (it.itv_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_j ())
                    ++ it_;
                else {
                    j_ = index2 () + 1;
                    if (rank_ == 1)
                        *this = (*this) ().find2 (rank_, i_, j_, 1);
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                if (rank_ == 1 && layout_type::fast_j ())
                    -- it_;
                else {
                    --j_;
                    if (rank_ == 1)
                        *this = (*this) ().find2 (rank_, i_, j_, -1);
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*this) ().value_data_ [it_ - (*this) ().index2_data_.begin ()];
                } else {
                    return (*this) () (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 begin () const {
                const self_type &m = (*this) ();
                return m.find1 (1, 0, index2 ());
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
                const self_type &m = (*this) ();
                return m.find1 (1, m.size1 (), index2 ());
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
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_)) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_));
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK (*this != (*this) ().find2 (0, i_, (*this) ().size2 ()), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_)) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_));
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator2 &operator = (const const_iterator2 &it) {
                container_const_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_const_subiterator_type itv_;
            const_subiterator_type it_;
        };

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

        class iterator2:
            public container_reference<compressed_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               iterator2, value_type> {
        public:
            typedef typename compressed_matrix::value_type value_type;
            typedef typename compressed_matrix::difference_type difference_type;
            typedef typename compressed_matrix::true_reference reference;
            typedef typename compressed_matrix::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, int rank, size_type i, size_type j, const vector_subiterator_type &itv, const subiterator_type &it):
                container_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_j ())
                    ++ it_;
                else {
                    j_ = index2 () + 1;
                    if (rank_ == 1)
                        *this = (*this) ().find2 (rank_, i_, j_, 1);
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                if (rank_ == 1 && layout_type::fast_j ())
                    -- it_;
                else {
                    --j_;
                    if (rank_ == 1)
                        *this = (*this) ().find2 (rank_, i_, j_, -1);
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*this) ().value_data_ [it_ - (*this) ().index2_data_.begin ()];
                } else {
                    return (*this) ().at_element (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 begin () const {
                self_type &m = (*this) ();
                return m.find1 (1, 0, index2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 end () const {
                self_type &m = (*this) ();
                return m.find1 (1, m.size1 (), index2 ());
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
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_)) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_));
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK (*this != (*this) ().find2 (0, i_, (*this) ().size2 ()), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_)) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m (itv_ - (*this) ().index1_data_.begin (), (*this) ().zero_based (*it_));
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator2 &operator = (const iterator2 &it) {
                container_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_subiterator_type itv_;
            subiterator_type it_;

            friend class const_iterator2;
        };

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

         // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
            ar & serialization::make_nvp("size1",s1);
            ar & serialization::make_nvp("size2",s2);
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
            }
            ar & serialization::make_nvp("capacity", capacity_);
            ar & serialization::make_nvp("filled1", filled1_);
            ar & serialization::make_nvp("filled2", filled2_);
            ar & serialization::make_nvp("index1_data", index1_data_);
            ar & serialization::make_nvp("index2_data", index2_data_);
            ar & serialization::make_nvp("value_data", value_data_);
            storage_invariants();
        }

    private:
        void storage_invariants () const {
            BOOST_UBLAS_CHECK (layout_type::size_M (size1_, size2_) + 1 == index1_data_.size (), internal_logic ());
            BOOST_UBLAS_CHECK (capacity_ == index2_data_.size (), internal_logic ());
            BOOST_UBLAS_CHECK (capacity_ == value_data_.size (), internal_logic ());
            BOOST_UBLAS_CHECK (filled1_ > 0 && filled1_ <= layout_type::size_M (size1_, size2_) + 1, internal_logic ());
            BOOST_UBLAS_CHECK (filled2_ <= capacity_, internal_logic ());
            BOOST_UBLAS_CHECK (index1_data_ [filled1_ - 1] == k_based (filled2_), internal_logic ());
        }
        
        size_type size1_;
        size_type size2_;
        array_size_type capacity_;
        array_size_type filled1_;
        array_size_type filled2_;
        index_array_type index1_data_;
        index_array_type index2_data_;
        value_array_type value_data_;
        static const value_type zero_;

        BOOST_UBLAS_INLINE
        static size_type zero_based (size_type k_based_index) {
            return k_based_index - IB;
        }
        BOOST_UBLAS_INLINE
        static size_type k_based (size_type zero_based_index) {
            return zero_based_index + IB;
        }

        friend class iterator1;
        friend class iterator2;
        friend class const_iterator1;
        friend class const_iterator2;
    };

    template<class T, class L, std::size_t IB, class IA, class TA>
    const typename compressed_matrix<T, L, IB, IA, TA>::value_type compressed_matrix<T, L, IB, IA, TA>::zero_ = value_type/*zero*/();


    // Coordinate array based sparse matrix class
    // Thanks to Kresimir Fresl for extending this to cover different index bases.
    template<class T, class L, std::size_t IB, class IA, class TA>
    class coordinate_matrix:
        public matrix_container<coordinate_matrix<T, L, IB, IA, TA> > {

        typedef T &true_reference;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef L layout_type;
        typedef coordinate_matrix<T, L, IB, IA, TA> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_container<self_type>::operator ();
#endif
        // ISSUE require type consistency check, is_convertable (IA::size_type, TA::size_type)
        typedef typename IA::value_type size_type;
        // ISSUE difference_type cannot be deduced for sparse indices, we only know the value_type
        typedef std::ptrdiff_t difference_type;
        // size_type for the data arrays.
        typedef typename IA::size_type array_size_type;
        typedef T value_type;
        typedef const T &const_reference;
#ifndef BOOST_UBLAS_STRICT_MATRIX_SPARSE
        typedef T &reference;
#else
        typedef sparse_matrix_element<self_type> reference;
#endif
        typedef IA index_array_type;
        typedef TA value_array_type;
        typedef const matrix_reference<const self_type> const_closure_type;
        typedef matrix_reference<self_type> closure_type;
        typedef coordinate_vector<T, IB, IA, TA> vector_temporary_type;
        typedef self_type matrix_temporary_type;
        typedef sparse_tag storage_category;
        typedef typename L::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        coordinate_matrix ():
            matrix_container<self_type> (),
            size1_ (0), size2_ (0), capacity_ (restrict_capacity (0)),
            filled_ (0), sorted_filled_ (filled_), sorted_ (true),
            index1_data_ (capacity_), index2_data_ (capacity_), value_data_ (capacity_) {
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        coordinate_matrix (size_type size1, size_type size2, array_size_type non_zeros = 0):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), capacity_ (restrict_capacity (non_zeros)),
            filled_ (0), sorted_filled_ (filled_), sorted_ (true),
            index1_data_ (capacity_), index2_data_ (capacity_), value_data_ (capacity_) {
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        coordinate_matrix (const coordinate_matrix &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_), capacity_ (m.capacity_),
            filled_ (m.filled_), sorted_filled_ (m.sorted_filled_), sorted_ (m.sorted_),
            index1_data_ (m.index1_data_), index2_data_ (m.index2_data_), value_data_ (m.value_data_) {
            storage_invariants ();
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        coordinate_matrix (const matrix_expression<AE> &ae, array_size_type non_zeros = 0):
            matrix_container<self_type> (),
            size1_ (ae ().size1 ()), size2_ (ae ().size2 ()), capacity_ (restrict_capacity (non_zeros)),
            filled_ (0), sorted_filled_ (filled_), sorted_ (true),
            index1_data_ (capacity_), index2_data_ (capacity_), value_data_ (capacity_) {
            storage_invariants ();
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
        BOOST_UBLAS_INLINE
        size_type nnz_capacity () const {
            return capacity_;
        }
        BOOST_UBLAS_INLINE
        size_type nnz () const {
            return filled_;
        }

        // Storage accessors
        BOOST_UBLAS_INLINE
        static size_type index_base () {
            return IB;
        }
        BOOST_UBLAS_INLINE
        array_size_type filled () const {
            return filled_;
        }
        BOOST_UBLAS_INLINE
        const index_array_type &index1_data () const {
            return index1_data_;
        }
        BOOST_UBLAS_INLINE
        const index_array_type &index2_data () const {
            return index2_data_;
        }
        BOOST_UBLAS_INLINE
        const value_array_type &value_data () const {
            return value_data_;
        }
        BOOST_UBLAS_INLINE
        void set_filled (const array_size_type &filled) {
            // Make sure that storage_invariants() succeeds
            if (sorted_ && filled < filled_)
                sorted_filled_ = filled;
            else
                sorted_ = (sorted_filled_ == filled);
            filled_ = filled;
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        index_array_type &index1_data () {
            return index1_data_;
        }
        BOOST_UBLAS_INLINE
        index_array_type &index2_data () {
            return index2_data_;
        }
        BOOST_UBLAS_INLINE
        value_array_type &value_data () {
            return value_data_;
        }

        // Resizing
    private:
        BOOST_UBLAS_INLINE
        array_size_type restrict_capacity (array_size_type non_zeros) const {
            // minimum non_zeros
            non_zeros = (std::max) (non_zeros, array_size_type((std::min) (size1_, size2_)));
            // ISSUE no maximum as coordinate may contain inserted duplicates
            return non_zeros;
        }
    public:
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool preserve = true) {
            // FIXME preserve unimplemented
            BOOST_UBLAS_CHECK (!preserve, internal_logic ());
            size1_ = size1;
            size2_ = size2;
            capacity_ = restrict_capacity (capacity_);
            index1_data_.resize (capacity_);
            index2_data_.resize (capacity_);
            value_data_.resize (capacity_);
            filled_ = 0;
            sorted_filled_ = filled_;
            sorted_ = true;
            storage_invariants ();
        }

        // Reserving
        BOOST_UBLAS_INLINE
        void reserve (array_size_type non_zeros, bool preserve = true) {
            sort ();    // remove duplicate elements
            capacity_ = restrict_capacity (non_zeros);
            if (preserve) {
                index1_data_.resize (capacity_, size_type ());
                index2_data_.resize (capacity_, size_type ());
                value_data_.resize (capacity_, value_type ());
                filled_ = (std::min) (capacity_, filled_);
            }
            else {
                index1_data_.resize (capacity_);
                index2_data_.resize (capacity_);
                value_data_.resize (capacity_);
                filled_ = 0;
            }
            sorted_filled_ = filled_;
            storage_invariants ();
        }

        // Element support
        BOOST_UBLAS_INLINE
        pointer find_element (size_type i, size_type j) {
            return const_cast<pointer> (const_cast<const self_type&>(*this).find_element (i, j));
        }
        BOOST_UBLAS_INLINE
        const_pointer find_element (size_type i, size_type j) const {
            sort ();
            size_type element1 (layout_type::index_M (i, j));
            size_type element2 (layout_type::index_m (i, j));
            vector_const_subiterator_type itv_begin (detail::lower_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (element1), std::less<size_type> ()));
            vector_const_subiterator_type itv_end (detail::upper_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (element1), std::less<size_type> ()));
            if (itv_begin == itv_end)
                return 0;
            const_subiterator_type it_begin (index2_data_.begin () + (itv_begin - index1_data_.begin ()));
            const_subiterator_type it_end (index2_data_.begin () + (itv_end - index1_data_.begin ()));
            const_subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (element2), std::less<size_type> ()));
            if (it == it_end || *it != k_based (element2))
                return 0;
            return &value_data_ [it - index2_data_.begin ()];
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            const_pointer p = find_element (i, j);
            if (p)
                return *p;
            else 
                return zero_;
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
#ifndef BOOST_UBLAS_STRICT_MATRIX_SPARSE
            pointer p = find_element (i, j);
            if (p)
                return *p;
            else
                return insert_element (i, j, value_type/*zero*/());
#else
            return reference (*this, i, j);
#endif
        }

        // Element assignment
        BOOST_UBLAS_INLINE
        void append_element (size_type i, size_type j, const_reference t) {
            if (filled_ >= capacity_)
                reserve (2 * filled_, true);
            BOOST_UBLAS_CHECK (filled_ < capacity_, internal_logic ());
            size_type element1 = layout_type::index_M (i, j);
            size_type element2 = layout_type::index_m (i, j);
            index1_data_ [filled_] = k_based (element1);
            index2_data_ [filled_] = k_based (element2);
            value_data_ [filled_] = t;
            ++ filled_;
            sorted_ = false;
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        true_reference insert_element (size_type i, size_type j, const_reference t) {
            BOOST_UBLAS_CHECK (!find_element (i, j), bad_index ());        // duplicate element
            append_element (i, j, t);
            return value_data_ [filled_ - 1];
        }
        BOOST_UBLAS_INLINE
        void erase_element (size_type i, size_type j) {
            size_type element1 = layout_type::index_M (i, j);
            size_type element2 = layout_type::index_m (i, j);
            sort ();
            vector_subiterator_type itv_begin (detail::lower_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (element1), std::less<size_type> ()));
            vector_subiterator_type itv_end (detail::upper_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (element1), std::less<size_type> ()));
            subiterator_type it_begin (index2_data_.begin () + (itv_begin - index1_data_.begin ()));
            subiterator_type it_end (index2_data_.begin () + (itv_end - index1_data_.begin ()));
            subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (element2), std::less<size_type> ()));
            if (it != it_end && *it == k_based (element2)) {
                typename std::iterator_traits<subiterator_type>::difference_type n = it - index2_data_.begin ();
                vector_subiterator_type itv (index1_data_.begin () + n);
                std::copy (itv + 1, index1_data_.begin () + filled_, itv);
                std::copy (it + 1, index2_data_.begin () + filled_, it);
                typename value_array_type::iterator itt (value_data_.begin () + n);
                std::copy (itt + 1, value_data_.begin () + filled_, itt);
                -- filled_;
                sorted_filled_ = filled_;
            }
            storage_invariants ();
        }
        
        // Zeroing
        BOOST_UBLAS_INLINE
        void clear () {
            filled_ = 0;
            sorted_filled_ = filled_;
            sorted_ = true;
            storage_invariants ();
        }

        // Assignment
        BOOST_UBLAS_INLINE
        coordinate_matrix &operator = (const coordinate_matrix &m) {
            if (this != &m) {
                size1_ = m.size1_;
                size2_ = m.size2_;
                capacity_ = m.capacity_;
                filled_ = m.filled_;
                sorted_filled_ = m.sorted_filled_;
                sorted_ = m.sorted_;
                index1_data_ = m.index1_data_;
                index2_data_ = m.index2_data_;
                value_data_ = m.value_data_;
                BOOST_UBLAS_CHECK (capacity_ == index1_data_.size (), internal_logic ());
                BOOST_UBLAS_CHECK (capacity_ == index2_data_.size (), internal_logic ());
                BOOST_UBLAS_CHECK (capacity_ == value_data_.size (), internal_logic ());
            }
            storage_invariants ();
            return *this;
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        coordinate_matrix &operator = (const matrix_container<C> &m) {
            resize (m ().size1 (), m ().size2 (), false);
            assign (m);
            return *this;
        }
        BOOST_UBLAS_INLINE
        coordinate_matrix &assign_temporary (coordinate_matrix &m) {
            swap (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        coordinate_matrix &operator = (const matrix_expression<AE> &ae) {
            self_type temporary (ae, capacity_);
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        coordinate_matrix &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        coordinate_matrix& operator += (const matrix_expression<AE> &ae) {
            self_type temporary (*this + ae, capacity_);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        coordinate_matrix &operator += (const matrix_container<C> &m) {
            plus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        coordinate_matrix &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        coordinate_matrix& operator -= (const matrix_expression<AE> &ae) {
            self_type temporary (*this - ae, capacity_);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        coordinate_matrix &operator -= (const matrix_container<C> &m) {
            minus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        coordinate_matrix &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        coordinate_matrix& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        coordinate_matrix& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (coordinate_matrix &m) {
            if (this != &m) {
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                std::swap (capacity_, m.capacity_);
                std::swap (filled_, m.filled_);
                std::swap (sorted_filled_, m.sorted_filled_);
                std::swap (sorted_, m.sorted_);
                index1_data_.swap (m.index1_data_);
                index2_data_.swap (m.index2_data_);
                value_data_.swap (m.value_data_);
            }
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        friend void swap (coordinate_matrix &m1, coordinate_matrix &m2) {
            m1.swap (m2);
        }

        // replacement if STL lower bound algorithm for use of inplace_merge
        array_size_type lower_bound (array_size_type beg, array_size_type end, array_size_type target) const {
            while (end > beg) {
                array_size_type mid = (beg + end) / 2;
                if (((index1_data_[mid] < index1_data_[target]) ||
                     ((index1_data_[mid] == index1_data_[target]) &&
                      (index2_data_[mid] < index2_data_[target])))) {
                    beg = mid + 1;
                } else {
                    end = mid;
                }
            }
            return beg;
        }

        // specialized replacement of STL inplace_merge to avoid compilation
        // problems with respect to the array_triple iterator
        void inplace_merge (array_size_type beg, array_size_type mid, array_size_type end) const {
            array_size_type len_lef = mid - beg;
            array_size_type len_rig = end - mid;

            if (len_lef == 1 && len_rig == 1) {
                if ((index1_data_[mid] < index1_data_[beg]) ||
                    ((index1_data_[mid] == index1_data_[beg]) && (index2_data_[mid] < index2_data_[beg])))
                    {
                        std::swap(index1_data_[beg], index1_data_[mid]);
                        std::swap(index2_data_[beg], index2_data_[mid]);
                        std::swap(value_data_[beg], value_data_[mid]);
                    }
            } else if (len_lef > 0 && len_rig > 0) {
                array_size_type lef_mid, rig_mid;
                if (len_lef >= len_rig) {
                    lef_mid = (beg + mid) / 2;
                    rig_mid = lower_bound(mid, end, lef_mid);
                } else {
                    rig_mid = (mid + end) / 2;
                    lef_mid = lower_bound(beg, mid, rig_mid);
                }
                std::rotate(&index1_data_[0] + lef_mid, &index1_data_[0] + mid, &index1_data_[0] + rig_mid);
                std::rotate(&index2_data_[0] + lef_mid, &index2_data_[0] + mid, &index2_data_[0] + rig_mid);
                std::rotate(&value_data_[0] + lef_mid, &value_data_[0] + mid, &value_data_[0] + rig_mid);

                array_size_type new_mid = lef_mid + rig_mid - mid;
                inplace_merge(beg, lef_mid, new_mid);
                inplace_merge(new_mid, rig_mid, end);
            }
        }

        // Sorting and summation of duplicates
        BOOST_UBLAS_INLINE
        void sort () const {
            if (! sorted_ && filled_ > 0) {
                typedef index_triple_array<index_array_type, index_array_type, value_array_type> array_triple;
                array_triple ita (filled_, index1_data_, index2_data_, value_data_);
#ifndef BOOST_UBLAS_COO_ALWAYS_DO_FULL_SORT
                const typename array_triple::iterator iunsorted = ita.begin () + sorted_filled_;
                // sort new elements and merge
                std::sort (iunsorted, ita.end ());
                inplace_merge(0, sorted_filled_, filled_);
#else
                const typename array_triple::iterator iunsorted = ita.begin ();
                std::sort (iunsorted, ita.end ());
#endif                
                // sum duplicates with += and remove
                array_size_type filled = 0;
                for (array_size_type i = 1; i < filled_; ++ i) {
                    if (index1_data_ [filled] != index1_data_ [i] ||
                        index2_data_ [filled] != index2_data_ [i]) {
                        ++ filled;
                        if (filled != i) {
                            index1_data_ [filled] = index1_data_ [i];
                            index2_data_ [filled] = index2_data_ [i];
                            value_data_ [filled] = value_data_ [i];
                        }
                    } else {
                        value_data_ [filled] += value_data_ [i];
                    }
                }
                filled_ = filled + 1;
                sorted_filled_ = filled_;
                sorted_ = true;
                storage_invariants ();
            }
        }

        // Back element insertion and erasure
        BOOST_UBLAS_INLINE
        void push_back (size_type i, size_type j, const_reference t) {
            size_type element1 = layout_type::index_M (i, j);
            size_type element2 = layout_type::index_m (i, j);
            // must maintain sort order
            BOOST_UBLAS_CHECK (sorted_ && 
                    (filled_ == 0 ||
                    index1_data_ [filled_ - 1] < k_based (element1) ||
                    (index1_data_ [filled_ - 1] == k_based (element1) && index2_data_ [filled_ - 1] < k_based (element2)))
                    , external_logic ());
            if (filled_ >= capacity_)
                reserve (2 * filled_, true);
            BOOST_UBLAS_CHECK (filled_ < capacity_, internal_logic ());
            index1_data_ [filled_] = k_based (element1);
            index2_data_ [filled_] = k_based (element2);
            value_data_ [filled_] = t;
            ++ filled_;
            sorted_filled_ = filled_;
            storage_invariants ();
        }
        BOOST_UBLAS_INLINE
        void pop_back () {
            // ISSUE invariants could be simpilfied if sorted required as precondition
            BOOST_UBLAS_CHECK (filled_ > 0, external_logic ());
            -- filled_;
            sorted_filled_ = (std::min) (sorted_filled_, filled_);
            sorted_ = sorted_filled_ = filled_;
            storage_invariants ();
        }

        // Iterator types
    private:
        // Use index array iterator
        typedef typename IA::const_iterator vector_const_subiterator_type;
        typedef typename IA::iterator vector_subiterator_type;
        typedef typename IA::const_iterator const_subiterator_type;
        typedef typename IA::iterator subiterator_type;

        BOOST_UBLAS_INLINE
        true_reference at_element (size_type i, size_type j) {
            pointer p = find_element (i, j);
            BOOST_UBLAS_CHECK (p, bad_index ());
            return *p;
        }

    public:
        class const_iterator1;
        class iterator1;
        class const_iterator2;
        class iterator2;
        typedef reverse_iterator_base1<const_iterator1> const_reverse_iterator1;
        typedef reverse_iterator_base1<iterator1> reverse_iterator1;
        typedef reverse_iterator_base2<const_iterator2> const_reverse_iterator2;
        typedef reverse_iterator_base2<iterator2> reverse_iterator2;

        // Element lookup
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        const_iterator1 find1 (int rank, size_type i, size_type j, int direction = 1) const {
            sort ();
            for (;;) {
                size_type address1 (layout_type::index_M (i, j));
                size_type address2 (layout_type::index_m (i, j));
                vector_const_subiterator_type itv_begin (detail::lower_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (address1), std::less<size_type> ()));
                vector_const_subiterator_type itv_end (detail::upper_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (address1), std::less<size_type> ()));

                const_subiterator_type it_begin (index2_data_.begin () + (itv_begin - index1_data_.begin ()));
                const_subiterator_type it_end (index2_data_.begin () + (itv_end - index1_data_.begin ()));

                const_subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (address2), std::less<size_type> ()));
                vector_const_subiterator_type itv (index1_data_.begin () + (it - index2_data_.begin ()));
                if (rank == 0)
                    return const_iterator1 (*this, rank, i, j, itv, it);
                if (it != it_end && zero_based (*it) == address2)
                    return const_iterator1 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_i ()) {
                        if (it == it_end)
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        i = zero_based (*it);
                    } else {
                        if (i >= size1_)
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        ++ i;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_i ()) {
                        if (it == index2_data_.begin () + array_size_type (zero_based (*itv)))
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        i = zero_based (*(it - 1));
                    } else {
                        if (i == 0)
                            return const_iterator1 (*this, rank, i, j, itv, it);
                        -- i;
                    }
                }
            }
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        iterator1 find1 (int rank, size_type i, size_type j, int direction = 1) {
            sort ();
            for (;;) {
                size_type address1 (layout_type::index_M (i, j));
                size_type address2 (layout_type::index_m (i, j));
                vector_subiterator_type itv_begin (detail::lower_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (address1), std::less<size_type> ()));
                vector_subiterator_type itv_end (detail::upper_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (address1), std::less<size_type> ()));

                subiterator_type it_begin (index2_data_.begin () + (itv_begin - index1_data_.begin ()));
                subiterator_type it_end (index2_data_.begin () + (itv_end - index1_data_.begin ()));

                subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (address2), std::less<size_type> ()));
                vector_subiterator_type itv (index1_data_.begin () + (it - index2_data_.begin ()));
                if (rank == 0)
                    return iterator1 (*this, rank, i, j, itv, it);
                if (it != it_end && zero_based (*it) == address2)
                    return iterator1 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_i ()) {
                        if (it == it_end)
                            return iterator1 (*this, rank, i, j, itv, it);
                        i = zero_based (*it);
                    } else {
                        if (i >= size1_)
                            return iterator1 (*this, rank, i, j, itv, it);
                        ++ i;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_i ()) {
                        if (it == index2_data_.begin () + array_size_type (zero_based (*itv)))
                            return iterator1 (*this, rank, i, j, itv, it);
                        i = zero_based (*(it - 1));
                    } else {
                        if (i == 0)
                            return iterator1 (*this, rank, i, j, itv, it);
                        -- i;
                    }
                }
            }
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        const_iterator2 find2 (int rank, size_type i, size_type j, int direction = 1) const {
            sort ();
            for (;;) {
                size_type address1 (layout_type::index_M (i, j));
                size_type address2 (layout_type::index_m (i, j));
                vector_const_subiterator_type itv_begin (detail::lower_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (address1), std::less<size_type> ()));
                vector_const_subiterator_type itv_end (detail::upper_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (address1), std::less<size_type> ()));

                const_subiterator_type it_begin (index2_data_.begin () + (itv_begin - index1_data_.begin ()));
                const_subiterator_type it_end (index2_data_.begin () + (itv_end - index1_data_.begin ()));

                const_subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (address2), std::less<size_type> ()));
                vector_const_subiterator_type itv (index1_data_.begin () + (it - index2_data_.begin ()));
                if (rank == 0)
                    return const_iterator2 (*this, rank, i, j, itv, it);
                if (it != it_end && zero_based (*it) == address2)
                    return const_iterator2 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_j ()) {
                        if (it == it_end)
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        j = zero_based (*it);
                    } else {
                        if (j >= size2_)
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        ++ j;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_j ()) {
                        if (it == index2_data_.begin () + array_size_type (zero_based (*itv)))
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        j = zero_based (*(it - 1));
                    } else {
                        if (j == 0)
                            return const_iterator2 (*this, rank, i, j, itv, it);
                        -- j;
                    }
                }
            }
        }
        // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.    
        iterator2 find2 (int rank, size_type i, size_type j, int direction = 1) {
            sort ();
            for (;;) {
                size_type address1 (layout_type::index_M (i, j));
                size_type address2 (layout_type::index_m (i, j));
                vector_subiterator_type itv_begin (detail::lower_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (address1), std::less<size_type> ()));
                vector_subiterator_type itv_end (detail::upper_bound (index1_data_.begin (), index1_data_.begin () + filled_, k_based (address1), std::less<size_type> ()));

                subiterator_type it_begin (index2_data_.begin () + (itv_begin - index1_data_.begin ()));
                subiterator_type it_end (index2_data_.begin () + (itv_end - index1_data_.begin ()));

                subiterator_type it (detail::lower_bound (it_begin, it_end, k_based (address2), std::less<size_type> ()));
                vector_subiterator_type itv (index1_data_.begin () + (it - index2_data_.begin ()));
                if (rank == 0)
                    return iterator2 (*this, rank, i, j, itv, it);
                if (it != it_end && zero_based (*it) == address2)
                    return iterator2 (*this, rank, i, j, itv, it);
                if (direction > 0) {
                    if (layout_type::fast_j ()) {
                        if (it == it_end)
                            return iterator2 (*this, rank, i, j, itv, it);
                        j = zero_based (*it);
                    } else {
                        if (j >= size2_)
                            return iterator2 (*this, rank, i, j, itv, it);
                        ++ j;
                    }
                } else /* if (direction < 0)  */ {
                    if (layout_type::fast_j ()) {
                        if (it == index2_data_.begin () + array_size_type (zero_based (*itv)))
                            return iterator2 (*this, rank, i, j, itv, it);
                        j = zero_based (*(it - 1));
                    } else {
                        if (j == 0)
                            return iterator2 (*this, rank, i, j, itv, it);
                        -- j;
                    }
                }
            }
        }


        class const_iterator1:
            public container_const_reference<coordinate_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename coordinate_matrix::value_type value_type;
            typedef typename coordinate_matrix::difference_type difference_type;
            typedef typename coordinate_matrix::const_reference reference;
            typedef const typename coordinate_matrix::pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, int rank, size_type i, size_type j, const vector_const_subiterator_type &itv, const const_subiterator_type &it):
                container_const_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), rank_ (it.rank_), i_ (it.i_), j_ (it.j_), itv_ (it.itv_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_i ())
                    ++ it_;
                else {
                    i_ = index1 () + 1;
                    if (rank_ == 1)
                        *this = (*this) ().find1 (rank_, i_, j_, 1);
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                if (rank_ == 1 && layout_type::fast_i ())
                    -- it_;
                else {
                    i_ = index1 () - 1;
                    if (rank_ == 1)
                        *this = (*this) ().find1 (rank_, i_, j_, -1);
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*this) ().value_data_ [it_ - (*this) ().index2_data_.begin ()];
                } else {
                    return (*this) () (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 begin () const {
                const self_type &m = (*this) ();
                return m.find2 (1, index1 (), 0);
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
                const self_type &m = (*this) ();
                return m.find2 (1, index1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (*this != (*this) ().find1 (0, (*this) ().size1 (), j_), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_)) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_));
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_)) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_));
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator1 &operator = (const const_iterator1 &it) {
                container_const_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_const_subiterator_type itv_;
            const_subiterator_type it_;
        };

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

        class iterator1:
            public container_reference<coordinate_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               iterator1, value_type> {
        public:
            typedef typename coordinate_matrix::value_type value_type;
            typedef typename coordinate_matrix::difference_type difference_type;
            typedef typename coordinate_matrix::true_reference reference;
            typedef typename coordinate_matrix::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, int rank, size_type i, size_type j, const vector_subiterator_type &itv, const subiterator_type &it):
                container_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_i ())
                    ++ it_;
                else {
                    i_ = index1 () + 1;
                    if (rank_ == 1)
                        *this = (*this) ().find1 (rank_, i_, j_, 1);
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                if (rank_ == 1 && layout_type::fast_i ())
                    -- it_;
                else {
                    i_ = index1 () - 1;
                    if (rank_ == 1)
                        *this = (*this) ().find1 (rank_, i_, j_, -1);
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*this) ().value_data_ [it_ - (*this) ().index2_data_.begin ()];
                } else {
                    return (*this) ().at_element (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 begin () const {
                self_type &m = (*this) ();
                return m.find2 (1, index1 (), 0);
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator2 end () const {
                self_type &m = (*this) ();
                return m.find2 (1, index1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (*this != (*this) ().find1 (0, (*this) ().size1 (), j_), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_)) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_));
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_)) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_));
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator1 &operator = (const iterator1 &it) {
                container_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_subiterator_type itv_;
            subiterator_type it_;

            friend class const_iterator1;
        };

        BOOST_UBLAS_INLINE
        iterator1 begin1 () {
            return find1 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator1 end1 () {
            return find1 (0, size1_, 0);
        }

        class const_iterator2:
            public container_const_reference<coordinate_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename coordinate_matrix::value_type value_type;
            typedef typename coordinate_matrix::difference_type difference_type;
            typedef typename coordinate_matrix::const_reference reference;
            typedef const typename coordinate_matrix::pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, int rank, size_type i, size_type j, const vector_const_subiterator_type itv, const const_subiterator_type &it):
                container_const_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), rank_ (it.rank_), i_ (it.i_), j_ (it.j_), itv_ (it.itv_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_j ())
                    ++ it_;
                else {
                    j_ = index2 () + 1;
                    if (rank_ == 1)
                        *this = (*this) ().find2 (rank_, i_, j_, 1);
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                if (rank_ == 1 && layout_type::fast_j ())
                    -- it_;
                else {
                    j_ = index2 () - 1;
                    if (rank_ == 1)
                        *this = (*this) ().find2 (rank_, i_, j_, -1);
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*this) ().value_data_ [it_ - (*this) ().index2_data_.begin ()];
                } else {
                    return (*this) () (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 begin () const {
                const self_type &m = (*this) ();
                return m.find1 (1, 0, index2 ());
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
                const self_type &m = (*this) ();
                return m.find1 (1, m.size1 (), index2 ());
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
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_)) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_));
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK (*this != (*this) ().find2 (0, i_, (*this) ().size2 ()), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_)) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_));
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator2 &operator = (const const_iterator2 &it) {
                container_const_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_const_subiterator_type itv_;
            const_subiterator_type it_;
        };

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

        class iterator2:
            public container_reference<coordinate_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               iterator2, value_type> {
        public:
            typedef typename coordinate_matrix::value_type value_type;
            typedef typename coordinate_matrix::difference_type difference_type;
            typedef typename coordinate_matrix::true_reference reference;
            typedef typename coordinate_matrix::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), rank_ (), i_ (), j_ (), itv_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, int rank, size_type i, size_type j, const vector_subiterator_type &itv, const subiterator_type &it):
                container_reference<self_type> (m), rank_ (rank), i_ (i), j_ (j), itv_ (itv), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                if (rank_ == 1 && layout_type::fast_j ())
                    ++ it_;
                else {
                    j_ = index2 () + 1;
                    if (rank_ == 1)
                        *this = (*this) ().find2 (rank_, i_, j_, 1);
                }
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                if (rank_ == 1 && layout_type::fast_j ())
                    -- it_;
                else {
                    j_ = index2 ();
                    if (rank_ == 1)
                        *this = (*this) ().find2 (rank_, i_, j_, -1);
                }
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                if (rank_ == 1) {
                    return (*this) ().value_data_ [it_ - (*this) ().index2_data_.begin ()];
                } else {
                    return (*this) ().at_element (i_, j_);
                }
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 begin () const {
                self_type &m = (*this) ();
                return m.find1 (1, 0, index2 ());
            }
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            iterator1 end () const {
                self_type &m = (*this) ();
                return m.find1 (1, m.size1 (), index2 ());
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
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_M ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_)) < (*this) ().size1 (), bad_index ());
                    return layout_type::index_M ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_));
                } else {
                    return i_;
                }
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK (*this != (*this) ().find2 (0, i_, (*this) ().size2 ()), bad_index ());
                if (rank_ == 1) {
                    BOOST_UBLAS_CHECK (layout_type::index_m ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_)) < (*this) ().size2 (), bad_index ());
                    return layout_type::index_m ((*this) ().zero_based (*itv_), (*this) ().zero_based (*it_));
                } else {
                    return j_;
                }
            }

            // Assignment
            BOOST_UBLAS_INLINE
            iterator2 &operator = (const iterator2 &it) {
                container_reference<self_type>::assign (&it ());
                rank_ = it.rank_;
                i_ = it.i_;
                j_ = it.j_;
                itv_ = it.itv_;
                it_ = it.it_;
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                // BOOST_UBLAS_CHECK (rank_ == it.rank_, internal_logic ());
                if (rank_ == 1 || it.rank_ == 1) {
                    return it_ == it.it_;
                } else {
                    return i_ == it.i_ && j_ == it.j_;
                }
            }

        private:
            int rank_;
            size_type i_;
            size_type j_;
            vector_subiterator_type itv_;
            subiterator_type it_;

            friend class const_iterator2;
        };

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

         // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
            ar & serialization::make_nvp("size1",s1);
            ar & serialization::make_nvp("size2",s2);
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
            }
            ar & serialization::make_nvp("capacity", capacity_);
            ar & serialization::make_nvp("filled", filled_);
            ar & serialization::make_nvp("sorted_filled", sorted_filled_);
            ar & serialization::make_nvp("sorted", sorted_);
            ar & serialization::make_nvp("index1_data", index1_data_);
            ar & serialization::make_nvp("index2_data", index2_data_);
            ar & serialization::make_nvp("value_data", value_data_);
            storage_invariants();
        }

    private:
        void storage_invariants () const
        {
            BOOST_UBLAS_CHECK (capacity_ == index1_data_.size (), internal_logic ());
            BOOST_UBLAS_CHECK (capacity_ == index2_data_.size (), internal_logic ());
            BOOST_UBLAS_CHECK (capacity_ == value_data_.size (), internal_logic ());
            BOOST_UBLAS_CHECK (filled_ <= capacity_, internal_logic ());
            BOOST_UBLAS_CHECK (sorted_filled_ <= filled_, internal_logic ());
            BOOST_UBLAS_CHECK (sorted_ == (sorted_filled_ == filled_), internal_logic ());
        }

        size_type size1_;
        size_type size2_;
        array_size_type capacity_;
        mutable array_size_type filled_;
        mutable array_size_type sorted_filled_;
        mutable bool sorted_;
        mutable index_array_type index1_data_;
        mutable index_array_type index2_data_;
        mutable value_array_type value_data_;
        static const value_type zero_;

        BOOST_UBLAS_INLINE
        static size_type zero_based (size_type k_based_index) {
            return k_based_index - IB;
        }
        BOOST_UBLAS_INLINE
        static size_type k_based (size_type zero_based_index) {
            return zero_based_index + IB;
        }

        friend class iterator1;
        friend class iterator2;
        friend class const_iterator1;
        friend class const_iterator2;
    };

    template<class T, class L, std::size_t IB, class IA, class TA>
    const typename coordinate_matrix<T, L, IB, IA, TA>::value_type coordinate_matrix<T, L, IB, IA, TA>::zero_ = value_type/*zero*/();

}}}

#endif
