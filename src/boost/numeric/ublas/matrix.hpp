//
//  Copyright (c) 2000-2010
//  Joerg Walter, Mathias Koch, Gunter Winkler, David Bellot
//  Copyright (c) 2014, Athanasios Iliopoulos
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#ifndef _BOOST_UBLAS_MATRIX_
#define _BOOST_UBLAS_MATRIX_

#include <boost/config.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/detail/matrix_assign.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/nvp.hpp>

// Iterators based on ideas of Jeremy Siek

namespace boost { namespace numeric { 
	
	/** \brief main namespace of uBLAS.
	 *
	 * Use this namespace for all operations with uBLAS. It can also be abbreviated with
	 * \code namespace ublas = boost::numeric::ublas; \endcode
	 *
	 * A common practice is to bring this namespace into the current scope with
	 * \code using namespace boost::numeric::ublas; \endcode.
	 *
	 * However, be warned that using the ublas namespace and the std::vector at the same time can lead to the compiler to confusion. 
	 * The solution is simply to prefix each ublas vector like \c boost::numeric::ublas::vector<T>. If you think it's too long to 
	 * write, you can define a new namespace like \c namespace ublas = boost::numeric::ublas and then just declare your vectors
	 * with \c ublas::vector<T>. STL vectors will be declared as vector<T>. No need to prefix with \c std::
	 */
	namespace ublas {

    namespace detail {
        using namespace boost::numeric::ublas;

        // Matrix resizing algorithm
        template <class L, class M>
        BOOST_UBLAS_INLINE
        void matrix_resize_preserve (M& m, M& temporary) {
            typedef L layout_type;
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
                    temporary.data () [layout_type::element (i1, size1, i2, size2)] =
                            m.data() [layout_type::element (i1, msize1, i2, msize2)];
                }
            }
            m.assign_temporary (temporary);
        }
    }

    /** \brief A dense matrix of values of type \c T.
     *
     * For a \f$(m \times n)\f$-dimensional matrix and \f$ 0 \leq i < m, 0 \leq j < n\f$, every element \f$ m_{i,j} \f$ is mapped to 
     * the \f$(i.n + j)\f$-th element of the container for row major orientation or the \f$ (i + j.m) \f$-th element of 
     * the container for column major orientation. In a dense matrix all elements are represented in memory in a 
     * contiguous chunk of memory by definition.
     * 
     * Orientation and storage can also be specified, otherwise a \c row_major and \c unbounded_array are used. It is \b not 
     * required by the storage to initialize elements of the matrix.
     *
     * \tparam T the type of object stored in the matrix (like double, float, complex, etc...)
     * \tparam L the storage organization. It can be either \c row_major or \c column_major. Default is \c row_major
     * \tparam A the type of Storage array. Default is \c unbounded_array
     */
    template<class T, class L, class A>
    class matrix:
        public matrix_container<matrix<T, L, A> > {

        typedef T *pointer;
        typedef L layout_type;
        typedef matrix<T, L, A> self_type;
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
        typedef self_type matrix_temporary_type;
        typedef dense_tag storage_category;
        // This could be better for performance,
        // typedef typename unknown_orientation_tag orientation_category;
        // but others depend on the orientation information...
        typedef typename L::orientation_category orientation_category;

        // Construction and destruction
	  
	  /// Default dense matrix constructor. Make a dense matrix of size (0,0)
	    BOOST_UBLAS_INLINE
	    matrix ():
	        matrix_container<self_type> (),
	        size1_ (0), size2_ (0), data_ () {}
	
	  /** Dense matrix constructor with defined size
	   * \param size1 number of rows
	   * \param size2 number of columns
	   */
	    BOOST_UBLAS_INLINE
	    matrix (size_type size1, size_type size2):
	        matrix_container<self_type> (),
	        size1_ (size1), size2_ (size2), data_ (layout_type::storage_size (size1, size2)) {
	    }
	
	  /** Dense matrix constructor with defined size a initial value for all the matrix elements
	   * \param size1 number of rows
	   * \param size2 number of columns
	   * \param init initial value assigned to all elements
	   */
	    matrix (size_type size1, size_type size2, const value_type &init):
	        matrix_container<self_type> (),
	        size1_ (size1), size2_ (size2), data_ (layout_type::storage_size (size1, size2), init) {
	    }

	  /** Dense matrix constructor with defined size and an initial data array
	   * \param size1 number of rows
	   * \param size2 number of columns
	   * \param data array to copy into the matrix. Must have the same dimension as the matrix
	   */
        BOOST_UBLAS_INLINE
        matrix (size_type size1, size_type size2, const array_type &data):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), data_ (data) {}

	  /** Copy-constructor of a dense matrix
	   * \param m is a dense matrix
	   */
        BOOST_UBLAS_INLINE
        matrix (const matrix &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_), data_ (m.data_) {}

	  /** Copy-constructor of a dense matrix from a matrix expression
	   * \param ae is a matrix expression
	   */
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix (const matrix_expression<AE> &ae):
            matrix_container<self_type> (),
            size1_ (ae ().size1 ()), size2_ (ae ().size2 ()), data_ (layout_type::storage_size (size1_, size2_)) {
            matrix_assign<scalar_assign> (*this, ae);
        }

        // Accessors
	  /** Return the number of rows of the matrix
	   * You can also use the free size<>() function in operation/size.hpp as size<1>(m) where m is a matrix
	   */
        BOOST_UBLAS_INLINE
        size_type size1 () const {
            return size1_;
        }

	  /** Return the number of colums of the matrix
	   * You can also use the free size<>() function in operation/size.hpp as size<2>(m) where m is a matrix
	   */
        BOOST_UBLAS_INLINE
        size_type size2 () const {
            return size2_;
        }

        // Storage accessors
	  /** Return a constant reference to the internal storage of a dense matrix, i.e. the raw data
	   * It's type depends on the type used by the matrix to store its data
	   */
        BOOST_UBLAS_INLINE
        const array_type &data () const {
            return data_;
        }
	  /** Return a reference to the internal storage of a dense matrix, i.e. the raw data
	   * It's type depends on the type used by the matrix to store its data
	   */
        BOOST_UBLAS_INLINE
        array_type &data () {
            return data_;
        }

        // Resizing
	  /** Resize a matrix to new dimensions
	   * If data are preserved, then if the size if bigger at least on one dimension, extra values are filled with zeros.
	   * If data are not preserved, then nothing has to be assumed regarding the content of the matrix after resizing.
	   * \param size1 the new number of rows
	   * \param size2 the new number of colums
	   * \param preserve a boolean to say if one wants the data to be preserved during the resizing. Default is true.
	   */
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool preserve = true) {
            if (preserve) {
                self_type temporary (size1, size2);
                detail::matrix_resize_preserve<layout_type> (*this, temporary);
            }
            else {
                data ().resize (layout_type::storage_size (size1, size2));
                size1_ = size1;
                size2_ = size2;
            }
        }

        // Element access
    
    /** Access a matrix element. Here we return a const reference
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     * \return a const reference to the element
     */
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            return data () [layout_type::element (i, size1_, j, size2_)];
        }

    /** Access a matrix element. Here we return a reference
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     * \return a reference to the element
     */
        BOOST_UBLAS_INLINE
        reference at_element (size_type i, size_type j) {
            return data () [layout_type::element (i, size1_, j, size2_)];
        }

    /** Access a matrix element. Here we return a reference
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     * \return a reference to the element
     */
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
            return at_element (i, j);
        }

	/** Access a matrix element. Here we return a reference
	 * \param i the first coordinate of the element. By default it's the row
	 * \param j the second coordinate of the element. By default it's the column
	 * \return a reference to the element
	 */
		BOOST_UBLAS_INLINE
		reference operator () (size_type i) {
			return data()[i];
		}

		BOOST_UBLAS_INLINE
		const_reference operator () (size_type i) const {
			return data()[i];
		}

//		/** Access a matrix element. Here we return a reference
//		 * \param i the first coordinate of the element. By default it's the row
//		 * \param j the second coordinate of the element. By default it's the column
//		 * \return a reference to the element
//		 */
//			BOOST_UBLAS_INLINE
//			const_reference operator () const (size_type i) {
//				return data()[i];
//			}
        // Element assignment

    /** Change the value of a matrix element. Return back a reference to it
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     * \param t the new value of the element
     * \return a reference to the newly changed element
     */
        BOOST_UBLAS_INLINE
        reference insert_element (size_type i, size_type j, const_reference t) {
            return (at_element (i, j) = t);
        }

    /** Erase the element 
     * For most types (int, double, etc...) it means setting 0 (zero) the element at zero in fact.
     * For user-defined types, it could be another value if you decided it. Your type in that case must
     * contain a default null value.
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     */
        void erase_element (size_type i, size_type j) {
            at_element (i, j) = value_type/*zero*/();
        }

        // Zeroing
    /** Erase all elements in the matrix
     * For most types (int, double, etc...) it means writing 0 (zero) everywhere.
     * For user-defined types, it could be another value if you decided it. Your type in that case must
     * contain a default null value.
     */
        BOOST_UBLAS_INLINE
        void clear () {
            std::fill (data ().begin (), data ().end (), value_type/*zero*/());
        }

        // Assignment
#ifdef BOOST_UBLAS_MOVE_SEMANTICS

        /*! @note "pass by value" the key idea to enable move semantics */
        BOOST_UBLAS_INLINE
        matrix &operator = (matrix m) {
            assign_temporary(m);
            return *this;
        }
#else
        BOOST_UBLAS_INLINE
        matrix &operator = (const matrix &m) {
            size1_ = m.size1_;
            size2_ = m.size2_;
            data () = m.data ();
            return *this;
        }
#endif
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        matrix &operator = (const matrix_container<C> &m) {
            resize (m ().size1 (), m ().size2 (), false);
            assign (m);
            return *this;
        }
        BOOST_UBLAS_INLINE
        matrix &assign_temporary (matrix &m) {
            swap (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix &operator = (const matrix_expression<AE> &ae) {
            self_type temporary (ae);
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix& operator += (const matrix_expression<AE> &ae) {
            self_type temporary (*this + ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        matrix &operator += (const matrix_container<C> &m) {
            plus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix& operator -= (const matrix_expression<AE> &ae) {
            self_type temporary (*this - ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        matrix &operator -= (const matrix_container<C> &m) {
            minus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        matrix &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        matrix& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (matrix &m) {
            if (this != &m) {
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                data ().swap (m.data ());
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (matrix &m1, matrix &m2) {
            m1.swap (m2);
        }

        // Iterator types
    private:
        // Use the storage array iterator
        typedef typename A::const_iterator const_subiterator_type;
        typedef typename A::iterator subiterator_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator1<self_type, dense_random_access_iterator_tag> iterator1;
        typedef indexed_iterator2<self_type, dense_random_access_iterator_tag> iterator2;
        typedef indexed_const_iterator1<self_type, dense_random_access_iterator_tag> const_iterator1;
        typedef indexed_const_iterator2<self_type, dense_random_access_iterator_tag> const_iterator2;
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
            return const_iterator1 (*this, data ().begin () + layout_type::address (i, size1_, j, size2_));
#endif
        }
        BOOST_UBLAS_INLINE
        iterator1 find1 (int /* rank */, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator1 (*this, i, j);
#else
            return iterator1 (*this, data ().begin () + layout_type::address (i, size1_, j, size2_));
#endif
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int /* rank */, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator2 (*this, i, j);
#else
            return const_iterator2 (*this, data ().begin () + layout_type::address (i, size1_, j, size2_));
#endif
        }
        BOOST_UBLAS_INLINE
        iterator2 find2 (int /* rank */, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator2 (*this, i, j);
#else
            return iterator2 (*this, data ().begin () + layout_type::address (i, size1_, j, size2_));
#endif
        }


#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename matrix::value_type value_type;
            typedef typename matrix::difference_type difference_type;
            typedef typename matrix::const_reference reference;
            typedef const typename matrix::pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, const const_subiterator_type &it):
                container_const_reference<self_type> (m), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                layout_type::increment_i (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                layout_type::decrement_i (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator += (difference_type n) {
                layout_type::increment_i (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -= (difference_type n) {
                layout_type::decrement_i (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return layout_type::distance_i (it_ - it.it_, (*this) ().size1 (), (*this) ().size2 ());
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                const self_type &m = (*this) ();
                return layout_type::index_i (it_ - m.begin1 ().it_, m.size1 (), m.size2 ());
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                const self_type &m = (*this) ();
                return layout_type::index_j (it_ - m.begin1 ().it_, m.size1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator_type it_;

            friend class iterator1;
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
            public container_reference<matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               iterator1, value_type> {
        public:
            typedef typename matrix::value_type value_type;
            typedef typename matrix::difference_type difference_type;
            typedef typename matrix::reference reference;
            typedef typename matrix::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, const subiterator_type &it):
                container_reference<self_type> (m), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                layout_type::increment_i (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                layout_type::decrement_i (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator += (difference_type n) {
                layout_type::increment_i (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -= (difference_type n) {
                layout_type::decrement_i (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return layout_type::distance_i (it_ - it.it_, (*this) ().size1 (), (*this) ().size2 ());
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                self_type &m = (*this) ();
                return layout_type::index_i (it_ - m.begin1 ().it_, m.size1 (), m.size2 ());
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                self_type &m = (*this) ();
                return layout_type::index_j (it_ - m.begin1 ().it_, m.size1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator_type it_;

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
            public container_const_reference<matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename matrix::value_type value_type;
            typedef typename matrix::difference_type difference_type;
            typedef typename matrix::const_reference reference;
            typedef const typename matrix::pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, const const_subiterator_type &it):
                container_const_reference<self_type> (m), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                layout_type::increment_j (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                layout_type::decrement_j (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator += (difference_type n) {
                layout_type::increment_j (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -= (difference_type n) {
                layout_type::decrement_j (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return layout_type::distance_j (it_ - it.it_, (*this) ().size1 (), (*this) ().size2 ());
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                const self_type &m = (*this) ();
                return layout_type::index_i (it_ - m.begin2 ().it_, m.size1 (), m.size2 ());
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                const self_type &m = (*this) ();
                return layout_type::index_j (it_ - m.begin2 ().it_, m.size1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator_type it_;

            friend class iterator2;
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
            public container_reference<matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               iterator2, value_type> {
        public:
            typedef typename matrix::value_type value_type;
            typedef typename matrix::difference_type difference_type;
            typedef typename matrix::reference reference;
            typedef typename matrix::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, const subiterator_type &it):
                container_reference<self_type> (m), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                layout_type::increment_j (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                layout_type::decrement_j (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator += (difference_type n) {
                layout_type::increment_j (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -= (difference_type n) {
                layout_type::decrement_j (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return layout_type::distance_j (it_ - it.it_, (*this) ().size1 (), (*this) ().size2 ());
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                self_type &m = (*this) ();
                return layout_type::index_i (it_ - m.begin2 ().it_, m.size1 (), m.size2 ());
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                self_type &m = (*this) ();
                return layout_type::index_j (it_ - m.begin2 ().it_, m.size1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator_type it_;

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

        // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
        
            // we need to copy to a collection_size_type to get a portable
            // and efficient serialization
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
          
            // serialize the sizes
            ar & serialization::make_nvp("size1",s1)
               & serialization::make_nvp("size2",s2);

            // copy the values back if loading
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
            }
            ar & serialization::make_nvp("data",data_);
        }

    private:
        size_type size1_;
        size_type size2_;
        array_type data_;
    };


#ifdef BOOST_UBLAS_CPP_GE_2011
    /** \brief A fixed size dense matrix of values of type \c T. Equivalent to a c-style 2 dimensional array.
     *
     * For a \f$(m \times n)\f$-dimensional fixed_matrix and \f$ 0 \leq i < m, 0 \leq j < n\f$, every element \f$ m_{i,j} \f$ is mapped to
     * the \f$(i.n + j)\f$-th element of the container for row major orientation or the \f$ (i + j.m) \f$-th element of
     * the container for column major orientation. In a dense matrix all elements are represented in memory in a
     * contiguous chunk of memory by definition.
     *
     * Orientation and storage can also be specified, otherwise \c row_major and \c std::array are used. It is \b not
     * required by the storage container to initialize elements of the matrix.
     *
     * \tparam T the type of object stored in the matrix (like double, float, std::complex<double>, etc...)
     * \tparam L the storage organization. It can be either \c row_major or \c column_major. Default is \c row_major
     * \tparam A the type of Storage array. Default is \c std::array<T, M*N>
     */
    template<class T, std::size_t M, std::size_t N, class L, class A>
    class fixed_matrix:
        public matrix_container<fixed_matrix<T, M, N, L, A> > {

        typedef T *pointer;
        typedef L layout_type;
        typedef fixed_matrix<T, M, N, L, A> self_type;
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
        typedef self_type matrix_temporary_type;
        typedef dense_tag storage_category;
        // This could be better for performance,
        // typedef typename unknown_orientation_tag orientation_category;
        // but others depend on the orientation information...
        typedef typename L::orientation_category orientation_category;

        // Construction and destruction

      /// Default dense fixed_matrix constructor. Make a dense fixed_matrix of size M x N
        BOOST_UBLAS_INLINE
        fixed_matrix ():
            matrix_container<self_type> (),
            data_ () {}

      /** Dense fixed_matrix constructor with defined initial value for all the matrix elements
       * \param init initial value assigned to all elements
       */
        fixed_matrix (const value_type &init):
            matrix_container<self_type> (),
            data_ ( ) {
            data_.fill(init);
        }

      /** Dense matrix constructor with defined initial data array
       * \param data array to copy into the matrix. Must have the same dimension as the matrix
       */
        BOOST_UBLAS_INLINE
        fixed_matrix (const array_type &data):
            matrix_container<self_type> (),
            data_ (data) {}

      /** Copy-constructor of a dense fixed_matrix
       * \param m is a dense fixed_matrix
       */
        BOOST_UBLAS_INLINE
        fixed_matrix (const fixed_matrix &m):
            matrix_container<self_type> (),
            data_ (m.data_) {}

      /** Copy-constructor of a dense matrix from a matrix expression
       * \param ae is a matrix expression
       */
        template<class AE>
        BOOST_UBLAS_INLINE
        fixed_matrix (const matrix_expression<AE> &ae):
            matrix_container<self_type> (),
            data_ () {
            matrix_assign<scalar_assign> (*this, ae);
        }

        /// \brief Construct a fixed_matrix from a list of values
        /// The list may be included in curly braces. Typical syntax is choices are :
        /// fixed_matrix<double, 2,2> v = { 1, 2, 3, 4 } or fixed_matrix<double,4> v( {1, 2, 3, 4} ) or fixed_matrix<double,2,2> v( 1, 2, 3, 4 )
        template <typename... Types>
        BOOST_UBLAS_INLINE
        fixed_matrix(value_type v0, Types... vrest) :
            matrix_container<self_type> (),
            data_( array_type{ v0, vrest... } ) {}

        // Accessors
      /** Return the number of rows of the fixed_matrix
       * You can also use the free size<>() function in operation/size.hpp as size<1>(m) where m is a fixed_matrix
       */
        BOOST_UBLAS_INLINE
        BOOST_CONSTEXPR size_type size1 () const {
            return M;
        }

      /** Return the number of colums of the fixed_matrix
       * You can also use the free size<>() function in operation/size.hpp as size<2>(m) where m is a fixed_matrix
       */
        BOOST_UBLAS_INLINE
        BOOST_CONSTEXPR size_type size2 () const {
            return N;
        }

        // Storage accessors
      /** Return a constant reference to the internal storage of a dense matrix, i.e. the raw data
       * It's type depends on the type used by the matrix to store its data
       */
        BOOST_UBLAS_INLINE
        const array_type &data () const {
            return data_;
        }
      /** Return a reference to the internal storage of a dense fixed_matrix, i.e. the raw data
       * It's type depends on the type used by the fixed_matrix to store its data
       */
        BOOST_UBLAS_INLINE
        array_type &data () {
            return data_;
        }


        // Element access

    /** Access a fixed_matrix element. Here we return a const reference
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     * \return a const reference to the element
     */
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            return data () [layout_type::element (i, M, j, N)]; // Fixme: add static lookup for element(...) i.e.: element<M, N>(i,j)
        }

    /** Access a fixed_matrix element. Here we return a reference
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     * \return a reference to the element
     */
        BOOST_UBLAS_INLINE
        reference at_element (size_type i, size_type j) {
            return data () [layout_type::element (i, M, j, N)];
        }

    /** Access a fixed_matrix element. Here we return a reference
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     * \return a reference to the element
     */
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
            return at_element (i, j);
        }

        // Element assignment

    /** Change the value of a fixed_matrix element. Return back a reference to it
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     * \param t the new value of the element
     * \return a reference to the newly changed element
     */
        BOOST_UBLAS_INLINE
        reference insert_element (size_type i, size_type j, const_reference t) {
            return (at_element (i, j) = t);
        }

    /** Erase the element
     * For most types (int, double, etc...) it means setting 0 (zero) the element at zero in fact.
     * For user-defined types, it could be another value if you decided it. Your type in that case must
     * contain a default null value.
     * \param i the first coordinate of the element. By default it's the row
     * \param j the second coordinate of the element. By default it's the column
     */
        void erase_element (size_type i, size_type j) {
            at_element (i, j) = value_type/*zero*/();
        }

        // Zeroing
    /** Erase all elements in the fixed_matrix
     * For most types (int, double, etc...) it means writing 0 (zero) everywhere.
     * For user-defined types, it could be another value if you decided it. Your type in that case must
     * contain a default null value.
     */
        BOOST_UBLAS_INLINE
        void clear () {
            std::fill (data ().begin (), data ().end (), value_type/*zero*/());
        }

        // Assignment
#ifdef BOOST_UBLAS_MOVE_SEMANTICS

        /*! @note "pass by value" the key idea to enable move semantics */
        BOOST_UBLAS_INLINE
        fixed_matrix &operator = (fixed_matrix m) {
            assign_temporary(m);
            return *this;
        }
#else
        BOOST_UBLAS_INLINE
        fixed_matrix &operator = (const fixed_matrix &m) {
            data () = m.data ();
            return *this;
        }
#endif
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        fixed_matrix &operator = (const matrix_container<C> &m) {
            resize (m ().size1 (), m ().size2 (), false);
            assign (m);
            return *this;
        }
        BOOST_UBLAS_INLINE
        fixed_matrix &assign_temporary (fixed_matrix &m) {
            swap (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        fixed_matrix &operator = (const matrix_expression<AE> &ae) {
            self_type temporary (ae);
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        fixed_matrix &assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        fixed_matrix& operator += (const matrix_expression<AE> &ae) {
            self_type temporary (*this + ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        fixed_matrix &operator += (const matrix_container<C> &m) {
            plus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        fixed_matrix &plus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_plus_assign> (*this, ae);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        fixed_matrix& operator -= (const matrix_expression<AE> &ae) {
            self_type temporary (*this - ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        fixed_matrix &operator -= (const matrix_container<C> &m) {
            minus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        fixed_matrix &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        fixed_matrix& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        fixed_matrix& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (fixed_matrix &m) {
            if (this != &m) {
                data ().swap (m.data ());
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (fixed_matrix &m1, fixed_matrix &m2) {
            m1.swap (m2);
        }

        // Iterator types
    private:
        // Use the storage array iterator
        typedef typename A::const_iterator const_subiterator_type;
        typedef typename A::iterator subiterator_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator1<self_type, dense_random_access_iterator_tag> iterator1;
        typedef indexed_iterator2<self_type, dense_random_access_iterator_tag> iterator2;
        typedef indexed_const_iterator1<self_type, dense_random_access_iterator_tag> const_iterator1;
        typedef indexed_const_iterator2<self_type, dense_random_access_iterator_tag> const_iterator2;
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
            return const_iterator1 (*this, data ().begin () + layout_type::address (i, M, j, N));
#endif
        }
        BOOST_UBLAS_INLINE
        iterator1 find1 (int /* rank */, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator1 (*this, i, j);
#else
            return iterator1 (*this, data ().begin () + layout_type::address (i, M, j, N));
#endif
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int /* rank */, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator2 (*this, i, j);
#else
            return const_iterator2 (*this, data ().begin () + layout_type::address (i, M, j, N));
#endif
        }
        BOOST_UBLAS_INLINE
        iterator2 find2 (int /* rank */, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator2 (*this, i, j);
#else
            return iterator2 (*this, data ().begin () + layout_type::address (i, M, j, N));
#endif
        }


#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<fixed_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename fixed_matrix::value_type value_type;
            typedef typename fixed_matrix::difference_type difference_type;
            typedef typename fixed_matrix::const_reference reference;
            typedef const typename fixed_matrix::pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, const const_subiterator_type &it):
                container_const_reference<self_type> (m), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                layout_type::increment_i (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                layout_type::decrement_i (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator += (difference_type n) {
                layout_type::increment_i (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -= (difference_type n) {
                layout_type::decrement_i (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return layout_type::distance_i (it_ - it.it_, (*this) ().size1 (), (*this) ().size2 ());
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                const self_type &m = (*this) ();
                return layout_type::index_i (it_ - m.begin1 ().it_, m.size1 (), m.size2 ());
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                const self_type &m = (*this) ();
                return layout_type::index_j (it_ - m.begin1 ().it_, m.size1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator_type it_;

            friend class iterator1;
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
            return find1 (0, M, 0);
        }
        BOOST_UBLAS_INLINE
        const_iterator1 cend1 () const {
            return end1 ();
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class iterator1:
            public container_reference<fixed_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               iterator1, value_type> {
        public:
            typedef typename fixed_matrix::value_type value_type;
            typedef typename fixed_matrix::difference_type difference_type;
            typedef typename fixed_matrix::reference reference;
            typedef typename fixed_matrix::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, const subiterator_type &it):
                container_reference<self_type> (m), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                layout_type::increment_i (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                layout_type::decrement_i (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator += (difference_type n) {
                layout_type::increment_i (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -= (difference_type n) {
                layout_type::decrement_i (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return layout_type::distance_i (it_ - it.it_, (*this) ().size1 (), (*this) ().size2 ());
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                self_type &m = (*this) ();
                return layout_type::index_i (it_ - m.begin1 ().it_, m.size1 (), m.size2 ());
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                self_type &m = (*this) ();
                return layout_type::index_j (it_ - m.begin1 ().it_, m.size1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator_type it_;

            friend class const_iterator1;
        };
#endif

        BOOST_UBLAS_INLINE
        iterator1 begin1 () {
            return find1 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator1 end1 () {
            return find1 (0, M, 0);
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator2:
            public container_const_reference<fixed_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename fixed_matrix::value_type value_type;
            typedef typename fixed_matrix::difference_type difference_type;
            typedef typename fixed_matrix::const_reference reference;
            typedef const typename fixed_matrix::pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, const const_subiterator_type &it):
                container_const_reference<self_type> (m), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                layout_type::increment_j (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                layout_type::decrement_j (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator += (difference_type n) {
                layout_type::increment_j (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -= (difference_type n) {
                layout_type::decrement_j (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return layout_type::distance_j (it_ - it.it_, (*this) ().size1 (), (*this) ().size2 ());
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                const self_type &m = (*this) ();
                return layout_type::index_i (it_ - m.begin2 ().it_, m.size1 (), m.size2 ());
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                const self_type &m = (*this) ();
                return layout_type::index_j (it_ - m.begin2 ().it_, m.size1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator_type it_;

            friend class iterator2;
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
            return find2 (0, 0, N);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 cend2 () const {
            return end2 ();
        }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class iterator2:
            public container_reference<fixed_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               iterator2, value_type> {
        public:
            typedef typename fixed_matrix::value_type value_type;
            typedef typename fixed_matrix::difference_type difference_type;
            typedef typename fixed_matrix::reference reference;
            typedef typename fixed_matrix::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, const subiterator_type &it):
                container_reference<self_type> (m), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                layout_type::increment_j (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                layout_type::decrement_j (it_, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator += (difference_type n) {
                layout_type::increment_j (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -= (difference_type n) {
                layout_type::decrement_j (it_, n, (*this) ().size1 (), (*this) ().size2 ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return layout_type::distance_j (it_ - it.it_, (*this) ().size1 (), (*this) ().size2 ());
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                self_type &m = (*this) ();
                return layout_type::index_i (it_ - m.begin2 ().it_, m.size1 (), m.size2 ());
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                self_type &m = (*this) ();
                return layout_type::index_j (it_ - m.begin2 ().it_, m.size1 (), m.size2 ());
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator_type it_;

            friend class const_iterator2;
        };
#endif

        BOOST_UBLAS_INLINE
        iterator2 begin2 () {
            return find2 (0, 0, 0);
        }
        BOOST_UBLAS_INLINE
        iterator2 end2 () {
            return find2 (0, 0, N);
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
            ar & serialization::make_nvp("data",data_);
        }

    private:
        array_type data_;
    };

#endif // BOOST_UBLAS_CPP_GE_2011

    /** \brief A dense matrix of values of type \c T with a variable size bounded to a maximum of \f$M\f$ by \f$N\f$. 
     *
     * For a \f$(m \times n)\f$-dimensional matrix and \f$ 0 \leq i < m, 0 \leq j < n\f$, every element \f$m_{i,j}\f$ is mapped
     * to the \f$(i.n + j)\f$-th element of the container for row major orientation or the \f$(i + j.m)\f$-th element
     * of the container for column major orientation. Finally in a dense matrix all elements are represented in memory 
     * in a contiguous chunk of memory.
     *
     * Orientation can be specified. Default is \c row_major
     * The default constructor creates the matrix with size \f$M\f$ by \f$N\f$. Elements are constructed by the storage 
     * type \c bounded_array, which need not initialise their value.
     *
     * \tparam T the type of object stored in the matrix (like double, float, complex, etc...)
     * \tparam M maximum and default number of rows (if not specified at construction)
     * \tparam N maximum and default number of columns (if not specified at construction)
     * \tparam L the storage organization. It can be either \c row_major or \c column_major. Default is \c row_major
     */
    template<class T, std::size_t M, std::size_t N, class L>
    class bounded_matrix:
        public matrix<T, L, bounded_array<T, M * N> > {

        typedef matrix<T, L, bounded_array<T, M * N> > matrix_type;
    public:
        typedef typename matrix_type::size_type size_type;
        static const size_type max_size1 = M;
        static const size_type max_size2 = N;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        bounded_matrix ():
            matrix_type (M, N) {}
        BOOST_UBLAS_INLINE
        bounded_matrix (size_type size1, size_type size2):
            matrix_type (size1, size2) {}
        BOOST_UBLAS_INLINE
        bounded_matrix (const bounded_matrix &m):
            matrix_type (m) {}
        template<class A2>              // Allow matrix<T, L, bounded_array<M,N> > construction
        BOOST_UBLAS_INLINE
        bounded_matrix (const matrix<T, L, A2> &m):
            matrix_type (m) {}
        template<class AE>
        BOOST_UBLAS_INLINE
        bounded_matrix (const matrix_expression<AE> &ae):
            matrix_type (ae) {}
        BOOST_UBLAS_INLINE
        ~bounded_matrix () {}

        // Assignment
#ifdef BOOST_UBLAS_MOVE_SEMANTICS

        /*! @note "pass by value" the key idea to enable move semantics */
        BOOST_UBLAS_INLINE
        bounded_matrix &operator = (bounded_matrix m) {
            matrix_type::operator = (m);
            return *this;
        }
#else
        BOOST_UBLAS_INLINE
        bounded_matrix &operator = (const bounded_matrix &m) {
            matrix_type::operator = (m);
            return *this;
        }
#endif
        template<class L2, class A2>        // Generic matrix assignment
        BOOST_UBLAS_INLINE
        bounded_matrix &operator = (const matrix<T, L2, A2> &m) {
            matrix_type::operator = (m);
            return *this;
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        bounded_matrix &operator = (const matrix_container<C> &m) {
            matrix_type::operator = (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        bounded_matrix &operator = (const matrix_expression<AE> &ae) {
            matrix_type::operator = (ae);
            return *this;
        }
    };


    /** \brief A dense matrix of values of type \c T stored as a vector of vectors.
    *
    * Rows or columns are not stored into contiguous chunks of memory but data inside rows (or columns) are. 
    * Orientation and storage can also be specified, otherwise a row major and unbounded arrays are used.
    * The data is stored as a vector of vectors, meaning that rows or columns might not be stored into contiguous chunks
    * of memory. Orientation and storage can also be specified, otherwise a row major and unbounded arrays are used. 
    * The storage type defaults to \c unbounded_array<unbounded_array<T>> and orientation is \c row_major. It is \b not 
    * required by the storage to initialize elements of the matrix. For a \f$(m \times n)\f$-dimensional matrix and 
    * \f$ 0 \leq i < m, 0 \leq j < n\f$, every element \f$m_{i,j}\f$ is mapped to the \f$(i.n + j)\f$-th element of the 
    * container for row major orientation or the \f$(i + j.m)\f$-th element of the container for column major orientation.
    *
    * \tparam T the type of object stored in the matrix (like double, float, complex, etc...)
    * \tparam L the storage organization. It can be either \c row_major or \c column_major. By default it is \c row_major
    * \tparam A the type of Storage array. By default, it is an \unbounded_array<unbounder_array<T>>
    */
    template<class T, class L, class A>
    class vector_of_vector:
        public matrix_container<vector_of_vector<T, L, A> > {

        typedef T *pointer;
        typedef L layout_type;
        typedef vector_of_vector<T, L, A> self_type;
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
        typedef vector<T, typename A::value_type> vector_temporary_type;
        typedef self_type matrix_temporary_type;
        typedef dense_tag storage_category;
        // This could be better for performance,
        // typedef typename unknown_orientation_tag orientation_category;
        // but others depend on the orientation information...
        typedef typename L::orientation_category orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        vector_of_vector ():
            matrix_container<self_type> (),
            size1_ (0), size2_ (0), data_ (1) {}
        BOOST_UBLAS_INLINE
        vector_of_vector (size_type size1, size_type size2):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), data_ (1) {
            resize (size1, size2, true);
        }
        BOOST_UBLAS_INLINE
        vector_of_vector (const vector_of_vector &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_), data_ (m.data_) {}
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_of_vector (const matrix_expression<AE> &ae):
            matrix_container<self_type> (),
            size1_ (ae ().size1 ()), size2_ (ae ().size2 ()), data_ (layout_type::size_M (size1_, size2_) + 1) {
            for (size_type k = 0; k < layout_type::size_M (size1_, size2_); ++ k)
                data ()[k].resize (layout_type::size_m (size1_, size2_));
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
            size1_ = size1;
            size2_ = size2;
            if (preserve)
                data ().resize (layout_type::size_M (size1, size2) + 1, typename array_type::value_type ());
            else
                data ().resize (layout_type::size_M (size1, size2) + 1);
            for (size_type k = 0; k < layout_type::size_M (size1, size2); ++ k) {
                if (preserve)
                    data () [k].resize (layout_type::size_m (size1, size2), value_type ());
                else
                    data () [k].resize (layout_type::size_m (size1, size2));
            }
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            return data () [layout_type::index_M (i, j)] [layout_type::index_m (i, j)];
        }
        BOOST_UBLAS_INLINE
        reference at_element (size_type i, size_type j) {
            return data () [layout_type::index_M (i, j)] [layout_type::index_m (i, j)];
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
            return at_element (i, j); 
        }

        // Element assignment
        BOOST_UBLAS_INLINE
        reference insert_element (size_type i, size_type j, const_reference t) {
            return (at_element (i, j) = t); 
        }
        BOOST_UBLAS_INLINE
        void erase_element (size_type i, size_type j) {
            at_element (i, j) = value_type/*zero*/(); 
        }
        
        // Zeroing
        BOOST_UBLAS_INLINE
        void clear () {
            for (size_type k = 0; k < layout_type::size_M (size1_, size2_); ++ k)
                std::fill (data () [k].begin (), data () [k].end (), value_type/*zero*/());
        }

        // Assignment
        BOOST_UBLAS_INLINE
        vector_of_vector &operator = (const vector_of_vector &m) {
            size1_ = m.size1_;
            size2_ = m.size2_;
            data () = m.data ();
            return *this;
        }
        BOOST_UBLAS_INLINE
        vector_of_vector &assign_temporary (vector_of_vector &m) { 
            swap (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_of_vector &operator = (const matrix_expression<AE> &ae) { 
            self_type temporary (ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        vector_of_vector &operator = (const matrix_container<C> &m) {
            resize (m ().size1 (), m ().size2 (), false);
            assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_of_vector &assign (const matrix_expression<AE> &ae) { 
            matrix_assign<scalar_assign> (*this, ae); 
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_of_vector& operator += (const matrix_expression<AE> &ae) {
            self_type temporary (*this + ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        vector_of_vector &operator += (const matrix_container<C> &m) {
            plus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_of_vector &plus_assign (const matrix_expression<AE> &ae) { 
            matrix_assign<scalar_plus_assign> (*this, ae); 
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_of_vector& operator -= (const matrix_expression<AE> &ae) {
            self_type temporary (*this - ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        vector_of_vector &operator -= (const matrix_container<C> &m) {
            minus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        vector_of_vector &minus_assign (const matrix_expression<AE> &ae) {
            matrix_assign<scalar_minus_assign> (*this, ae); 
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        vector_of_vector& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        vector_of_vector& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (vector_of_vector &m) {
            if (this != &m) {
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                data ().swap (m.data ());
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (vector_of_vector &m1, vector_of_vector &m2) {
            m1.swap (m2);
        }

        // Iterator types
    private:
        // Use the vector iterator
        typedef typename A::value_type::const_iterator const_subiterator_type;
        typedef typename A::value_type::iterator subiterator_type;
    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator1<self_type, dense_random_access_iterator_tag> iterator1;
        typedef indexed_iterator2<self_type, dense_random_access_iterator_tag> iterator2;
        typedef indexed_const_iterator1<self_type, dense_random_access_iterator_tag> const_iterator1;
        typedef indexed_const_iterator2<self_type, dense_random_access_iterator_tag> const_iterator2;
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
        const_iterator1 find1 (int /*rank*/, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator1 (*this, i, j);
#else
            return const_iterator1 (*this, i, j, data () [layout_type::index_M (i, j)].begin ()  + layout_type::index_m (i, j));
#endif
        }
        BOOST_UBLAS_INLINE
        iterator1 find1 (int /*rank*/, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator1 (*this, i, j);
#else
            return iterator1 (*this, i, j, data () [layout_type::index_M (i, j)].begin ()  + layout_type::index_m (i, j));
#endif
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int /*rank*/, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator2 (*this, i, j);
#else
            return const_iterator2 (*this, i, j, data () [layout_type::index_M (i, j)].begin ()  + layout_type::index_m (i, j));
#endif
        }
        BOOST_UBLAS_INLINE
        iterator2 find2 (int /*rank*/, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator2 (*this, i, j);
#else
            return iterator2 (*this, i, j, data () [layout_type::index_M (i, j)].begin () + layout_type::index_m (i, j));
#endif
        }


#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<vector_of_vector>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename vector_of_vector::value_type value_type;
            typedef typename vector_of_vector::difference_type difference_type;
            typedef typename vector_of_vector::const_reference reference;
            typedef const typename vector_of_vector::pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), i_ (), j_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, size_type i, size_type j, const const_subiterator_type &it):
                container_const_reference<self_type> (m), i_ (i), j_ (j), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), i_ (it.i_), j_ (it.j_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                ++ i_;
                const self_type &m = (*this) ();
                if (layout_type::fast_i ())
                    ++ it_;
                else 
                    it_ = m.find1 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                -- i_;
                const self_type &m = (*this) ();
                if (layout_type::fast_i ())
                    -- it_;
                else
                    it_ = m.find1 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator += (difference_type n) {
                i_ += n;
                const self_type &m = (*this) ();
                it_ = m.find1 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -= (difference_type n) {
                i_ -= n;
                const self_type &m = (*this) ();
                it_ = m.find1 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index2 () == it.index2 (), bad_index ());
                return index1 () - it.index1 ();
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                return i_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return j_;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index2 () == it.index2 (), bad_index ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index2 () == it.index2 (), bad_index ());
                return it_ < it.it_;
            }

        private:
            size_type i_;
            size_type j_;
            const_subiterator_type it_;

            friend class iterator1;
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
            public container_reference<vector_of_vector>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               iterator1, value_type> {
        public:
            typedef typename vector_of_vector::value_type value_type;
            typedef typename vector_of_vector::difference_type difference_type;
            typedef typename vector_of_vector::reference reference;
            typedef typename vector_of_vector::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), i_ (), j_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, size_type i, size_type j, const subiterator_type &it):
                container_reference<self_type> (m), i_ (i), j_ (j), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                ++ i_;
                self_type &m = (*this) ();
                if (layout_type::fast_i ())
                    ++ it_;
                else
                    it_ = m.find1 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                -- i_;
                self_type &m = (*this) ();
                if (layout_type::fast_i ())
                    -- it_;
                else
                    it_ = m.find1 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator += (difference_type n) {
                i_ += n;
                self_type &m = (*this) ();
                it_ = m.find1 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -= (difference_type n) {
                i_ -= n;
                self_type &m = (*this) ();
                it_ = m.find1 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index2 () == it.index2 (), bad_index ());
                return index1 () - it.index1 ();
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                return i_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return j_;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index2 () == it.index2 (), bad_index ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index2 () == it.index2 (), bad_index ());
                return it_ < it.it_;
            }

        private:
            size_type i_;
            size_type j_;
            subiterator_type it_;

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
            public container_const_reference<vector_of_vector>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename vector_of_vector::value_type value_type;
            typedef typename vector_of_vector::difference_type difference_type;
            typedef typename vector_of_vector::const_reference reference;
            typedef const typename vector_of_vector::pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), i_ (), j_ (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, size_type i, size_type j, const const_subiterator_type &it):
                container_const_reference<self_type> (m), i_ (i), j_ (j), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const iterator2 &it):
                container_const_reference<self_type> (it ()), i_ (it.i_), j_ (it.j_), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                ++ j_;
                const self_type &m = (*this) ();
                if (layout_type::fast_j ())
                    ++ it_;
                else
                    it_ = m.find2 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                -- j_;
                const self_type &m = (*this) ();
                if (layout_type::fast_j ())
                    -- it_;
                else
                    it_ = m.find2 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator += (difference_type n) {
                j_ += n;
                const self_type &m = (*this) ();
                it_ = m.find2 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -= (difference_type n) {
                j_ -= n;
                const self_type &m = (*this) ();
                it_ = m.find2 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index1 () == it.index1 (), bad_index ());
                return index2 () - it.index2 ();
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
            const_reverse_iterator2 crend () const {
                return rend ();
            }
#endif

            // Indices
            BOOST_UBLAS_INLINE
            size_type index1 () const {
                return i_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return j_;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index1 () == it.index1 (), bad_index ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index1 () == it.index1 (), bad_index ());
                return it_ < it.it_;
            }

        private:
            size_type i_;
            size_type j_;
            const_subiterator_type it_;

            friend class iterator2;
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
            public container_reference<vector_of_vector>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               iterator2, value_type> {
        public:
            typedef typename vector_of_vector::value_type value_type;
            typedef typename vector_of_vector::difference_type difference_type;
            typedef typename vector_of_vector::reference reference;
            typedef typename vector_of_vector::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), i_ (), j_ (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, size_type i, size_type j, const subiterator_type &it):
                container_reference<self_type> (m), i_ (i), j_ (j), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator2 &operator ++ () {
                ++ j_;
                self_type &m = (*this) ();
                if (layout_type::fast_j ())
                    ++ it_;
                else
                    it_ = m.find2 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -- () {
                -- j_;
                self_type &m = (*this) ();
                if (layout_type::fast_j ())
                    -- it_;
                else
                    it_ = m.find2 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator += (difference_type n) {
                j_ += n;
                self_type &m = (*this) ();
                it_ = m.find2 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator2 &operator -= (difference_type n) {
                j_ -= n;
                self_type &m = (*this) ();
                it_ = m.find2 (1, i_, j_).it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index1 () == it.index1 (), bad_index ());
                return index2 () - it.index2 ();
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                return i_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return j_;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index1 () == it.index1 (), bad_index ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                BOOST_UBLAS_CHECK (index1 () == it.index1 (), bad_index ());
                return it_ < it.it_;
            }

        private:
            size_type i_;
            size_type j_;
            subiterator_type it_;

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

        // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
        
            // we need to copy to a collection_size_type to get a portable
            // and efficient serialization
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
          
            // serialize the sizes
            ar & serialization::make_nvp("size1",s1)
               & serialization::make_nvp("size2",s2);

            // copy the values back if loading
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
            }
            ar & serialization::make_nvp("data",data_);
        }

    private:
        size_type size1_;
        size_type size2_;
        array_type data_;
    };


    /** \brief A matrix with all values of type \c T equal to zero
     *
     * Changing values does not affect the matrix, however assigning it to a normal matrix will put zero 
     * everywhere in the target matrix. All accesses are constant time, due to the trivial value.
     *
     * \tparam T the type of object stored in the matrix (like double, float, complex, etc...)
     * \tparam ALLOC an allocator for storing the zero element. By default, a standar allocator is used.
     */
    template<class T, class ALLOC>
    class zero_matrix:
        public matrix_container<zero_matrix<T, ALLOC> > {

        typedef const T *const_pointer;
        typedef zero_matrix<T, ALLOC> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_container<self_type>::operator ();
#endif
        typedef typename boost::allocator_size_type<ALLOC>::type size_type;
        typedef typename boost::allocator_difference_type<ALLOC>::type difference_type;
        typedef T value_type;
        typedef const T &const_reference;
        typedef T &reference;
        typedef const matrix_reference<const self_type> const_closure_type;
        typedef matrix_reference<self_type> closure_type;
        typedef sparse_tag storage_category;
        typedef unknown_orientation_tag orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        zero_matrix ():
            matrix_container<self_type> (),
            size1_ (0), size2_ (0) {}
        BOOST_UBLAS_INLINE
        zero_matrix (size_type size):
            matrix_container<self_type> (),
            size1_ (size), size2_ (size) {}
        BOOST_UBLAS_INLINE
        zero_matrix (size_type size1, size_type size2):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2) {}
        BOOST_UBLAS_INLINE
        zero_matrix (const zero_matrix &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_) {}

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size1 () const {
            return size1_;
        }
        BOOST_UBLAS_INLINE
        size_type size2 () const {
            return size2_;
        }

        // Resizing
        BOOST_UBLAS_INLINE
        void resize (size_type size, bool /*preserve*/ = true) {
            size1_ = size;
            size2_ = size;
        }
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool /*preserve*/ = true) {
            size1_ = size1;
            size2_ = size2;
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type /* i */, size_type /* j */) const {
            return zero_;
        }

        // Assignment
        BOOST_UBLAS_INLINE
        zero_matrix &operator = (const zero_matrix &m) {
            size1_ = m.size1_;
            size2_ = m.size2_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        zero_matrix &assign_temporary (zero_matrix &m) {
            swap (m);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (zero_matrix &m) {
            if (this != &m) {
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (zero_matrix &m1, zero_matrix &m2) {
            m1.swap (m2);
        }

        // Iterator types
    public:
        class const_iterator1;
        class const_iterator2;
        typedef reverse_iterator_base1<const_iterator1> const_reverse_iterator1;
        typedef reverse_iterator_base2<const_iterator2> const_reverse_iterator2;

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator1 find1 (int /*rank*/, size_type /*i*/, size_type /*j*/) const {
            return const_iterator1 (*this);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int /*rank*/, size_type /*i*/, size_type /*j*/) const {
            return const_iterator2 (*this);
        }

        class const_iterator1:
            public container_const_reference<zero_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename zero_matrix::value_type value_type;
            typedef typename zero_matrix::difference_type difference_type;
            typedef typename zero_matrix::const_reference reference;
            typedef typename zero_matrix::const_pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m):
                container_const_reference<self_type> (m) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return zero_;   // arbitary return value
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 begin () const {
                return const_iterator2 ((*this) ());
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
                return const_iterator2 ((*this) ());
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
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return 0;   // arbitary return value
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return 0;   // arbitary return value
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator1 &operator = (const const_iterator1 &it) {
                container_const_reference<self_type>::assign (&it ());
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                detail::ignore_unused_variable_warning(it);
                return true;
            }
        };

        typedef const_iterator1 iterator1;

        BOOST_UBLAS_INLINE
        const_iterator1 begin1 () const {
            return const_iterator1 (*this);
        }
        BOOST_UBLAS_INLINE
        const_iterator1 cbegin1 () const {
            return begin1 ();
        }
        BOOST_UBLAS_INLINE
        const_iterator1 end1 () const {
            return const_iterator1 (*this);
        }
        BOOST_UBLAS_INLINE
        const_iterator1 cend1 () const {
            return end1 ();
        }

        class const_iterator2:
            public container_const_reference<zero_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename zero_matrix::value_type value_type;
            typedef typename zero_matrix::difference_type difference_type;
            typedef typename zero_matrix::const_reference reference;
            typedef typename zero_matrix::const_pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m):
                container_const_reference<self_type> (m) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return zero_;   // arbitary return value
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 begin () const {
                return const_iterator1 ((*this) ());
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
                return const_iterator1 ((*this) ());
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
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return 0;   // arbitary return value
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                BOOST_UBLAS_CHECK_FALSE (bad_index ());
                return 0;   // arbitary return value
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator2 &operator = (const const_iterator2 &it) {
                container_const_reference<self_type>::assign (&it ());
                return *this;
            }

            // Comparison
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                detail::ignore_unused_variable_warning(it);
                return true;
            }
        };

        typedef const_iterator2 iterator2;

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

         // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
        
            // we need to copy to a collection_size_type to get a portable
            // and efficient serialization
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
          
            // serialize the sizes
            ar & serialization::make_nvp("size1",s1)
               & serialization::make_nvp("size2",s2);

            // copy the values back if loading
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
            }
        }

    private:
        size_type size1_;
        size_type size2_;
        static const value_type zero_;
    };

    template<class T, class ALLOC>
    const typename zero_matrix<T, ALLOC>::value_type zero_matrix<T, ALLOC>::zero_ = T(/*zero*/);

    /** \brief An identity matrix with values of type \c T
     *
     * Elements or cordinates \f$(i,i)\f$ are equal to 1 (one) and all others to 0 (zero). 
     * Changing values does not affect the matrix, however assigning it to a normal matrix will
     * make the matrix equal to an identity matrix. All accesses are constant du to the trivial values.
     *
     * \tparam T the type of object stored in the matrix (like double, float, complex, etc...)
     * \tparam ALLOC an allocator for storing the zeros and one elements. By default, a standar allocator is used.
     */ 
    template<class T, class ALLOC>
    class identity_matrix:
        public matrix_container<identity_matrix<T, ALLOC> > {

        typedef const T *const_pointer;
        typedef identity_matrix<T, ALLOC> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_container<self_type>::operator ();
#endif
        typedef typename boost::allocator_size_type<ALLOC>::type size_type;
        typedef typename boost::allocator_difference_type<ALLOC>::type difference_type;
        typedef T value_type;
        typedef const T &const_reference;
        typedef T &reference;
        typedef const matrix_reference<const self_type> const_closure_type;
        typedef matrix_reference<self_type> closure_type;
        typedef sparse_tag storage_category;
        typedef unknown_orientation_tag orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        identity_matrix ():
            matrix_container<self_type> (),
            size1_ (0), size2_ (0), size_common_ (0) {}
        BOOST_UBLAS_INLINE
        identity_matrix (size_type size):
            matrix_container<self_type> (),
            size1_ (size), size2_ (size), size_common_ ((std::min) (size1_, size2_)) {}
        BOOST_UBLAS_INLINE
        identity_matrix (size_type size1, size_type size2):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), size_common_ ((std::min) (size1_, size2_)) {}
        BOOST_UBLAS_INLINE
        identity_matrix (const identity_matrix &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_), size_common_ ((std::min) (size1_, size2_)) {}

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size1 () const {
            return size1_;
        }
        BOOST_UBLAS_INLINE
        size_type size2 () const {
            return size2_;
        }

        // Resizing
        BOOST_UBLAS_INLINE
        void resize (size_type size, bool /*preserve*/ = true) {
            size1_ = size;
            size2_ = size;
            size_common_ = ((std::min)(size1_, size2_));
        }
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool /*preserve*/ = true) {
            size1_ = size1;
            size2_ = size2;
            size_common_ = ((std::min)(size1_, size2_));
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            if (i == j)
                return one_;
            else
                return zero_;
        }

        // Assignment
        BOOST_UBLAS_INLINE
        identity_matrix &operator = (const identity_matrix &m) {
            size1_ = m.size1_;
            size2_ = m.size2_;
            size_common_ = m.size_common_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        identity_matrix &assign_temporary (identity_matrix &m) { 
            swap (m);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (identity_matrix &m) {
            if (this != &m) {
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                std::swap (size_common_, m.size_common_);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (identity_matrix &m1, identity_matrix &m2) {
            m1.swap (m2);
        }

        // Iterator types
    private:
        // Use an index
        typedef size_type const_subiterator_type;

    public:
        class const_iterator1;
        class const_iterator2;
        typedef reverse_iterator_base1<const_iterator1> const_reverse_iterator1;
        typedef reverse_iterator_base2<const_iterator2> const_reverse_iterator2;

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator1 find1 (int rank, size_type i, size_type j) const {
            if (rank == 1) {
                i = (std::max) (i, j);
                i = (std::min) (i, j + 1);
            }
            return const_iterator1 (*this, i);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int rank, size_type i, size_type j) const {
            if (rank == 1) {
                j = (std::max) (j, i);
                j = (std::min) (j, i + 1);
            }
            return const_iterator2 (*this, j);
        }


        class const_iterator1:
            public container_const_reference<identity_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename identity_matrix::value_type value_type;
            typedef typename identity_matrix::difference_type difference_type;
            typedef typename identity_matrix::const_reference reference;
            typedef typename identity_matrix::const_pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, const const_subiterator_type &it):
                container_const_reference<self_type> (m), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                BOOST_UBLAS_CHECK (it_ < (*this) ().size1 (), bad_index ());
                ++it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                BOOST_UBLAS_CHECK (it_ > 0, bad_index ());
                --it_;
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                return one_;
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator2 begin () const {
                return const_iterator2 ((*this) (), it_); 
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
                return const_iterator2 ((*this) (), it_ + 1); 
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
                return it_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it_;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }

        private:
            const_subiterator_type it_;
        };

        typedef const_iterator1 iterator1;

        BOOST_UBLAS_INLINE
        const_iterator1 begin1 () const {
            return const_iterator1 (*this, 0);
        }
        BOOST_UBLAS_INLINE
        const_iterator1 cbegin1 () const {
            return begin1 ();
        }
        BOOST_UBLAS_INLINE
        const_iterator1 end1 () const {
            return const_iterator1 (*this, size_common_);
        }
        BOOST_UBLAS_INLINE
        const_iterator1 cend1 () const {
            return end1 ();
        }

        class const_iterator2:
            public container_const_reference<identity_matrix>,
            public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename identity_matrix::value_type value_type;
            typedef typename identity_matrix::difference_type difference_type;
            typedef typename identity_matrix::const_reference reference;
            typedef typename identity_matrix::const_pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, const const_subiterator_type &it):
                container_const_reference<self_type> (m), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator2 &operator ++ () {
                BOOST_UBLAS_CHECK (it_ < (*this) ().size_common_, bad_index ());
                ++it_;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator2 &operator -- () {
                BOOST_UBLAS_CHECK (it_ > 0, bad_index ());
                --it_;
                return *this;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                return one_;
            }

#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
            BOOST_UBLAS_INLINE
#ifdef BOOST_UBLAS_MSVC_NESTED_CLASS_RELATION
            typename self_type::
#endif
            const_iterator1 begin () const {
                return const_iterator1 ((*this) (), it_); 
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
                return const_iterator1 ((*this) (), it_ + 1); 
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
                return it_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it_;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }

        private:
            const_subiterator_type it_;
        };

        typedef const_iterator2 iterator2;

        BOOST_UBLAS_INLINE
        const_iterator2 begin2 () const {
            return const_iterator2 (*this, 0);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 cbegin2 () const {
            return begin2 ();
        }
        BOOST_UBLAS_INLINE
        const_iterator2 end2 () const {
            return const_iterator2 (*this, size_common_);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 cend2 () const {
            return end2 ();
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

         // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
        
            // we need to copy to a collection_size_type to get a portable
            // and efficient serialization
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
          
            // serialize the sizes
            ar & serialization::make_nvp("size1",s1)
               & serialization::make_nvp("size2",s2);

            // copy the values back if loading
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
                size_common_ = ((std::min)(size1_, size2_));
            }
        }

    private:
        size_type size1_;
        size_type size2_;
        size_type size_common_;
        static const value_type zero_;
        static const value_type one_;
    };

    template<class T, class ALLOC>
    const typename identity_matrix<T, ALLOC>::value_type identity_matrix<T, ALLOC>::zero_ = T(/*zero*/);
    template<class T, class ALLOC>
    const typename identity_matrix<T, ALLOC>::value_type identity_matrix<T, ALLOC>::one_ (1); // ISSUE: need 'one'-traits here


    /** \brief A matrix with all values of type \c T equal to the same value
     *
     * Changing one value has the effect of changing all the values. Assigning it to a normal matrix will copy
     * the same value everywhere in this matrix. All accesses are constant time, due to the trivial value.
     *
     * \tparam T the type of object stored in the matrix (like double, float, complex, etc...)
     * \tparam ALLOC an allocator for storing the unique value. By default, a standar allocator is used.
     */
    template<class T, class ALLOC>
    class scalar_matrix:
        public matrix_container<scalar_matrix<T, ALLOC> > {

        typedef const T *const_pointer;
        typedef scalar_matrix<T, ALLOC> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_container<self_type>::operator ();
#endif
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T value_type;
        typedef const T &const_reference;
        typedef T &reference;
        typedef const matrix_reference<const self_type> const_closure_type;
        typedef matrix_reference<self_type> closure_type;
        typedef dense_tag storage_category;
        typedef unknown_orientation_tag orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        scalar_matrix ():
            matrix_container<self_type> (),
            size1_ (0), size2_ (0), value_ () {}
        BOOST_UBLAS_INLINE
        scalar_matrix (size_type size1, size_type size2, const value_type &value = value_type(1)):
            matrix_container<self_type> (),
            size1_ (size1), size2_ (size2), value_ (value) {}
        BOOST_UBLAS_INLINE
        scalar_matrix (const scalar_matrix &m):
            matrix_container<self_type> (),
            size1_ (m.size1_), size2_ (m.size2_), value_ (m.value_) {}

        // Accessors
        BOOST_UBLAS_INLINE
        size_type size1 () const {
            return size1_;
        }
        BOOST_UBLAS_INLINE
        size_type size2 () const {
            return size2_;
        }

        // Resizing
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool /*preserve*/ = true) {
            size1_ = size1;
            size2_ = size2;
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type /*i*/, size_type /*j*/) const {
            return value_; 
        }

        // Assignment
        BOOST_UBLAS_INLINE
        scalar_matrix &operator = (const scalar_matrix &m) {
            size1_ = m.size1_;
            size2_ = m.size2_;
            value_ = m.value_;
            return *this;
        }
        BOOST_UBLAS_INLINE
        scalar_matrix &assign_temporary (scalar_matrix &m) { 
            swap (m);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (scalar_matrix &m) {
            if (this != &m) {
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                std::swap (value_, m.value_);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (scalar_matrix &m1, scalar_matrix &m2) {
            m1.swap (m2);
        }

        // Iterator types
    private:
        // Use an index
        typedef size_type const_subiterator_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_const_iterator1<self_type, dense_random_access_iterator_tag> iterator1;
        typedef indexed_const_iterator2<self_type, dense_random_access_iterator_tag> iterator2;
        typedef indexed_const_iterator1<self_type, dense_random_access_iterator_tag> const_iterator1;
        typedef indexed_const_iterator2<self_type, dense_random_access_iterator_tag> const_iterator2;
#else
        class const_iterator1;
        class const_iterator2;
#endif
        typedef reverse_iterator_base1<const_iterator1> const_reverse_iterator1;
        typedef reverse_iterator_base2<const_iterator2> const_reverse_iterator2;

        // Element lookup
        BOOST_UBLAS_INLINE
        const_iterator1 find1 (int /*rank*/, size_type i, size_type j) const {
            return const_iterator1 (*this, i, j);
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int /*rank*/, size_type i, size_type j) const {
            return const_iterator2 (*this, i, j);
        }   


#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<scalar_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename scalar_matrix::value_type value_type;
            typedef typename scalar_matrix::difference_type difference_type;
            typedef typename scalar_matrix::const_reference reference;
            typedef typename scalar_matrix::const_pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<scalar_matrix> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const scalar_matrix &m, const const_subiterator_type &it1, const const_subiterator_type &it2):
                container_const_reference<scalar_matrix> (m), it1_ (it1), it2_ (it2) {}

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
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                return (*this) () (index1 (), index2 ());
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
                const scalar_matrix &m = (*this) ();
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
                const scalar_matrix &m = (*this) ();
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
                return it1_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_;
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator1 &operator = (const const_iterator1 &it) {
                container_const_reference<scalar_matrix>::assign (&it ());
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
            const_subiterator_type it1_;
            const_subiterator_type it2_;
        };

        typedef const_iterator1 iterator1;
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
        class const_iterator2:
            public container_const_reference<scalar_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename scalar_matrix::value_type value_type;
            typedef typename scalar_matrix::difference_type difference_type;
            typedef typename scalar_matrix::const_reference reference;
            typedef typename scalar_matrix::const_pointer pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<scalar_matrix> (), it1_ (), it2_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const scalar_matrix &m, const const_subiterator_type &it1, const const_subiterator_type &it2):
                container_const_reference<scalar_matrix> (m), it1_ (it1), it2_ (it2) {}

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
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
                return (*this) () (index1 (), index2 ());
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
                const scalar_matrix &m = (*this) ();
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
                const scalar_matrix &m = (*this) ();
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
                return it1_;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                return it2_;
            }

            // Assignment
            BOOST_UBLAS_INLINE
            const_iterator2 &operator = (const const_iterator2 &it) {
                container_const_reference<scalar_matrix>::assign (&it ());
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
            const_subiterator_type it1_;
            const_subiterator_type it2_;
        };

        typedef const_iterator2 iterator2;
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

         // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
        
            // we need to copy to a collection_size_type to get a portable
            // and efficient serialization
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
          
            // serialize the sizes
            ar & serialization::make_nvp("size1",s1)
               & serialization::make_nvp("size2",s2);

            // copy the values back if loading
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
            }

            ar & serialization::make_nvp("value", value_);
        }

    private:
        size_type size1_;
        size_type size2_;
        value_type value_;
    };


    /** \brief An array based matrix class which size is defined at type specification or object instanciation
     *
     * This matrix is directly based on a predefined C-style arry of data, thus providing the fastest
     * implementation possible. The constraint is that dimensions of the matrix must be specified at 
     * the instanciation or the type specification. 
     *
     * For instance, \code typedef c_matrix<double,4,4> my_4by4_matrix \endcode 
     * defines a 4 by 4 double-precision matrix.  You can also instantiate it directly with 
     * \code c_matrix<int,8,5> my_fast_matrix \endcode. This will make a 8 by 5 integer matrix. The 
     * price to pay for this speed is that you cannot resize it to a size larger than the one defined 
     * in the template parameters. In the previous example, a size of 4 by 5 or 3 by 2 is acceptable, 
     * but a new size of 9 by 5 or even 10 by 10 will raise a bad_size() exception.
     *
     * \tparam T the type of object stored in the matrix (like double, float, complex, etc...)
     * \tparam N the default maximum number of rows
     * \tparam M the default maximum number of columns
     */
    template<class T, std::size_t N, std::size_t M>
    class c_matrix:
        public matrix_container<c_matrix<T, N, M> > {

        typedef c_matrix<T, N, M> self_type;
    public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
        using matrix_container<self_type>::operator ();
#endif
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T value_type;
        typedef const T &const_reference;
        typedef T &reference;
        typedef const T *const_pointer;
        typedef T *pointer;
        typedef const matrix_reference<const self_type> const_closure_type;
        typedef matrix_reference<self_type> closure_type;
        typedef c_vector<T, N * M> vector_temporary_type;     // vector able to store all elements of c_matrix
        typedef self_type matrix_temporary_type;
        typedef dense_tag storage_category;
        // This could be better for performance,
        // typedef typename unknown_orientation_tag orientation_category;
        // but others depend on the orientation information...
        typedef row_major_tag orientation_category;

        // Construction and destruction
        BOOST_UBLAS_INLINE
        c_matrix ():
            size1_ (N), size2_ (M) /* , data_ () */ {
        }
        BOOST_UBLAS_INLINE
        c_matrix (size_type size1, size_type size2):
            size1_ (size1), size2_ (size2) /* , data_ () */ {
            if (size1_ > N || size2_ > M)
                bad_size ().raise ();
        }
        BOOST_UBLAS_INLINE
        c_matrix (const c_matrix &m):
            size1_ (m.size1_), size2_ (m.size2_) /* , data_ () */ {
            if (size1_ > N || size2_ > M)
                bad_size ().raise ();
            assign(m);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        c_matrix (const matrix_expression<AE> &ae):
            size1_ (ae ().size1 ()), size2_ (ae ().size2 ()) /* , data_ () */ {
            if (size1_ > N || size2_ > M)
                bad_size ().raise ();
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
        const_pointer data () const {
            return reinterpret_cast<const_pointer> (data_);
        }
        BOOST_UBLAS_INLINE
        pointer data () {
            return reinterpret_cast<pointer> (data_);
        }

        // Resizing
        BOOST_UBLAS_INLINE
        void resize (size_type size1, size_type size2, bool preserve = true) {
            if (size1 > N || size2 > M)
                bad_size ().raise ();
            if (preserve) {
                self_type temporary (size1, size2);
                // Common elements to preserve
                const size_type size1_min = (std::min) (size1, size1_);
                const size_type size2_min = (std::min) (size2, size2_);
                for (size_type i = 0; i != size1_min; ++i) {    // indexing copy over major
                    for (size_type j = 0; j != size2_min; ++j) {
                        temporary.data_[i][j] = data_[i][j];
                    }
                }
                assign_temporary (temporary);
            }
            else {
                size1_ = size1;
                size2_ = size2;
            }
        }

        // Element access
        BOOST_UBLAS_INLINE
        const_reference operator () (size_type i, size_type j) const {
            BOOST_UBLAS_CHECK (i < size1_, bad_index ());
            BOOST_UBLAS_CHECK (j < size2_, bad_index ());
            return data_ [i] [j];
        }
        BOOST_UBLAS_INLINE
        reference at_element (size_type i, size_type j) {
            BOOST_UBLAS_CHECK (i < size1_, bad_index ());
            BOOST_UBLAS_CHECK (j < size2_, bad_index ());
            return data_ [i] [j];
        }
        BOOST_UBLAS_INLINE
        reference operator () (size_type i, size_type j) {
            return at_element (i, j);
        }

        // Element assignment
        BOOST_UBLAS_INLINE
        reference insert_element (size_type i, size_type j, const_reference t) {
            return (at_element (i, j) = t);
        }
        
        // Zeroing
        BOOST_UBLAS_INLINE
        void clear () {
            for (size_type i = 0; i < size1_; ++ i)
                std::fill (data_ [i], data_ [i] + size2_, value_type/*zero*/());
        }

        // Assignment
#ifdef BOOST_UBLAS_MOVE_SEMANTICS

        /*! @note "pass by value" the key idea to enable move semantics */
        BOOST_UBLAS_INLINE
        c_matrix &operator = (c_matrix m) {
            assign_temporary(m);
            return *this;
        }
#else
        BOOST_UBLAS_INLINE
        c_matrix &operator = (const c_matrix &m) {
            size1_ = m.size1_;
            size2_ = m.size2_;
            for (size_type i = 0; i < m.size1_; ++ i)
                std::copy (m.data_ [i], m.data_ [i] + m.size2_, data_ [i]);
            return *this;
        }
#endif
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        c_matrix &operator = (const matrix_container<C> &m) {
            resize (m ().size1 (), m ().size2 (), false);
            assign (m);
            return *this;
        }
        BOOST_UBLAS_INLINE
        c_matrix &assign_temporary (c_matrix &m) {
            swap (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        c_matrix &operator = (const matrix_expression<AE> &ae) { 
            self_type temporary (ae);
            return assign_temporary (temporary);
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        c_matrix &assign (const matrix_expression<AE> &ae) { 
            matrix_assign<scalar_assign> (*this, ae); 
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        c_matrix& operator += (const matrix_expression<AE> &ae) {
            self_type temporary (*this + ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        c_matrix &operator += (const matrix_container<C> &m) {
            plus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        c_matrix &plus_assign (const matrix_expression<AE> &ae) { 
            matrix_assign<scalar_plus_assign> (*this, ae); 
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        c_matrix& operator -= (const matrix_expression<AE> &ae) {
            self_type temporary (*this - ae);
            return assign_temporary (temporary);
        }
        template<class C>          // Container assignment without temporary
        BOOST_UBLAS_INLINE
        c_matrix &operator -= (const matrix_container<C> &m) {
            minus_assign (m);
            return *this;
        }
        template<class AE>
        BOOST_UBLAS_INLINE
        c_matrix &minus_assign (const matrix_expression<AE> &ae) { 
            matrix_assign<scalar_minus_assign> (*this, ae); 
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        c_matrix& operator *= (const AT &at) {
            matrix_assign_scalar<scalar_multiplies_assign> (*this, at);
            return *this;
        }
        template<class AT>
        BOOST_UBLAS_INLINE
        c_matrix& operator /= (const AT &at) {
            matrix_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

        // Swapping
        BOOST_UBLAS_INLINE
        void swap (c_matrix &m) {
            if (this != &m) {
                BOOST_UBLAS_CHECK (size1_ == m.size1_, bad_size ());
                BOOST_UBLAS_CHECK (size2_ == m.size2_, bad_size ());
                std::swap (size1_, m.size1_);
                std::swap (size2_, m.size2_);
                for (size_type i = 0; i < size1_; ++ i)
                    std::swap_ranges (data_ [i], data_ [i] + size2_, m.data_ [i]);
            }
        }
        BOOST_UBLAS_INLINE
        friend void swap (c_matrix &m1, c_matrix &m2) {
            m1.swap (m2);
        }

        // Iterator types
    private:
        // Use pointers for iterator
        typedef const_pointer const_subiterator_type;
        typedef pointer subiterator_type;

    public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
        typedef indexed_iterator1<self_type, dense_random_access_iterator_tag> iterator1;
        typedef indexed_iterator2<self_type, dense_random_access_iterator_tag> iterator2;
        typedef indexed_const_iterator1<self_type, dense_random_access_iterator_tag> const_iterator1;
        typedef indexed_const_iterator2<self_type, dense_random_access_iterator_tag> const_iterator2;
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
        const_iterator1 find1 (int /*rank*/, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator1 (*this, i, j);
#else
            return const_iterator1 (*this, &data_ [i] [j]);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator1 find1 (int /*rank*/, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator1 (*this, i, j);
#else
            return iterator1 (*this, &data_ [i] [j]);
#endif
        }
        BOOST_UBLAS_INLINE
        const_iterator2 find2 (int /*rank*/, size_type i, size_type j) const {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return const_iterator2 (*this, i, j);
#else
            return const_iterator2 (*this, &data_ [i] [j]);
#endif
        }
        BOOST_UBLAS_INLINE
        iterator2 find2 (int /*rank*/, size_type i, size_type j) {
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
            return iterator2 (*this, i, j);
#else
            return iterator2 (*this, &data_ [i] [j]);
#endif
        }


#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
        class const_iterator1:
            public container_const_reference<c_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator1, value_type> {
        public:
            typedef typename c_matrix::difference_type difference_type;
            typedef typename c_matrix::value_type value_type;
            typedef typename c_matrix::const_reference reference;
            typedef typename c_matrix::const_pointer pointer;

            typedef const_iterator2 dual_iterator_type;
            typedef const_reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator1 ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const self_type &m, const const_subiterator_type &it):
                container_const_reference<self_type> (m), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator1 (const iterator1 &it):
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            const_iterator1 &operator ++ () {
                it_ += M;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -- () {
                it_ -= M;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator += (difference_type n) {
                it_ += n * M;
                return *this;
            }
            BOOST_UBLAS_INLINE
            const_iterator1 &operator -= (difference_type n) {
                it_ -= n * M;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return (it_ - it.it_) / M;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                const self_type &m = (*this) ();
                return (it_ - m.begin1 ().it_) / M;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                const self_type &m = (*this) ();
                return (it_ - m.begin1 ().it_) % M;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator_type it_;

            friend class iterator1;
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
            public container_reference<c_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               iterator1, value_type> {
        public:

            typedef typename c_matrix::difference_type difference_type;
            typedef typename c_matrix::value_type value_type;
            typedef typename c_matrix::reference reference;
            typedef typename c_matrix::pointer pointer;

            typedef iterator2 dual_iterator_type;
            typedef reverse_iterator2 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator1 ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator1 (self_type &m, const subiterator_type &it):
                container_reference<self_type> (m), it_ (it) {}

            // Arithmetic
            BOOST_UBLAS_INLINE
            iterator1 &operator ++ () {
                it_ += M;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -- () {
                it_ -= M;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator += (difference_type n) {
                it_ += n * M;
                return *this;
            }
            BOOST_UBLAS_INLINE
            iterator1 &operator -= (difference_type n) {
                it_ -= n * M;
                return *this;
            }
            BOOST_UBLAS_INLINE
            difference_type operator - (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return (it_ - it.it_) / M;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                const self_type &m = (*this) ();
                return (it_ - m.begin1 ().it_) / M;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                const self_type &m = (*this) ();
                return (it_ - m.begin1 ().it_) % M;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator1 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator_type it_;

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
            public container_const_reference<c_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               const_iterator2, value_type> {
        public:
            typedef typename c_matrix::difference_type difference_type;
            typedef typename c_matrix::value_type value_type;
            typedef typename c_matrix::const_reference reference;
            typedef typename c_matrix::const_reference pointer;

            typedef const_iterator1 dual_iterator_type;
            typedef const_reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            const_iterator2 ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator2 (const self_type &m, const const_subiterator_type &it):
                container_const_reference<self_type> (m), it_ (it) {}
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                const self_type &m = (*this) ();
                return (it_ - m.begin2 ().it_) / M;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                const self_type &m = (*this) ();
                return (it_ - m.begin2 ().it_) % M;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const const_iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            const_subiterator_type it_;

            friend class iterator2;
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
            public container_reference<c_matrix>,
            public random_access_iterator_base<dense_random_access_iterator_tag,
                                               iterator2, value_type> {
        public:
            typedef typename c_matrix::difference_type difference_type;
            typedef typename c_matrix::value_type value_type;
            typedef typename c_matrix::reference reference;
            typedef typename c_matrix::pointer pointer;

            typedef iterator1 dual_iterator_type;
            typedef reverse_iterator1 dual_reverse_iterator_type;

            // Construction and destruction
            BOOST_UBLAS_INLINE
            iterator2 ():
                container_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            iterator2 (self_type &m, const subiterator_type &it):
                container_reference<self_type> (m), it_ (it) {}

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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ - it.it_;
            }

            // Dereference
            BOOST_UBLAS_INLINE
            reference operator * () const {
                BOOST_UBLAS_CHECK (index1 () < (*this) ().size1 (), bad_index ());
                BOOST_UBLAS_CHECK (index2 () < (*this) ().size2 (), bad_index ());
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
                const self_type &m = (*this) ();
                return (it_ - m.begin2 ().it_) / M;
            }
            BOOST_UBLAS_INLINE
            size_type index2 () const {
                const self_type &m = (*this) ();
                return (it_ - m.begin2 ().it_) % M;
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
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }
            BOOST_UBLAS_INLINE
            bool operator < (const iterator2 &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ < it.it_;
            }

        private:
            subiterator_type it_;

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

         // Serialization
        template<class Archive>
        void serialize(Archive & ar, const unsigned int /* file_version */){
        
            // we need to copy to a collection_size_type to get a portable
            // and efficient serialization
            serialization::collection_size_type s1 (size1_);
            serialization::collection_size_type s2 (size2_);
          
            // serialize the sizes
            ar & serialization::make_nvp("size1",s1)
               & serialization::make_nvp("size2",s2);

            // copy the values back if loading
            if (Archive::is_loading::value) {
                size1_ = s1;
                size2_ = s2;
            }
            // could probably use make_array( &(data[0][0]), N*M ) 
            ar & serialization::make_array(data_, N);
        }

    private:
        size_type size1_;
        size_type size2_;
        value_type data_ [N] [M];
    };

}}}

#endif
