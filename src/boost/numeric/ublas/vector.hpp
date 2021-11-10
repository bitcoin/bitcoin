//
//  Copyright (c) 2000-2010
//  Joerg Walter, Mathias Koch, David Bellot
//  Copyright (c) 2014, Athanasios Iliopoulos
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//
//  And we acknowledge the support from all contributors.

/// \file vector.hpp Definition for the class vector and its derivative

#ifndef _BOOST_UBLAS_VECTOR_
#define _BOOST_UBLAS_VECTOR_

#include <boost/config.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>
#include <boost/numeric/ublas/detail/vector_assign.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/nvp.hpp>

#ifdef BOOST_UBLAS_CPP_GE_2011
#include <array>
#include <initializer_list>
#if defined(BOOST_MSVC) // For std::forward in fixed_vector
#include <utility>
#endif
#endif

// Iterators based on ideas of Jeremy Siek

namespace boost { namespace numeric { namespace ublas {

	 /** \brief A dense vector of values of type \c T.
	  *
	  * For a \f$n\f$-dimensional vector \f$v\f$ and \f$0\leq i < n\f$ every element \f$v_i\f$ is mapped 
	  * to the \f$i\f$-th element of the container. A storage type \c A can be specified which defaults to \c unbounded_array. 
	  * Elements are constructed by \c A, which need not initialise their value.
	  *
	  * \tparam T type of the objects stored in the vector (like int, double, complex,...)
	  * \tparam A The type of the storage array of the vector. Default is \c unbounded_array<T>. \c <bounded_array<T> and \c std::vector<T> can also be used
	  */
	 template<class T, class A>
	 class vector:
	     public vector_container<vector<T, A> > {

	     typedef vector<T, A> self_type;
	 public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
	     using vector_container<self_type>::operator ();
#endif

	typedef typename A::size_type size_type;
	    typedef typename A::difference_type difference_type;
	    typedef T value_type;
	    typedef typename type_traits<T>::const_reference const_reference;
	    typedef T &reference;
	    typedef T *pointer;
	    typedef const T *const_pointer;
	    typedef A array_type;
	    typedef const vector_reference<const self_type> const_closure_type;
	    typedef vector_reference<self_type> closure_type;
	    typedef self_type vector_temporary_type;
	    typedef dense_tag storage_category;

	    // Construction and destruction
	
	/// \brief Constructor of a vector
	/// By default it is empty, i.e. \c size()==0.
	    BOOST_UBLAS_INLINE
	    vector ():
	        vector_container<self_type> (),
	        data_ () {}

	/// \brief Constructor of a vector with a predefined size
	/// By default, its elements are initialized to 0.
	/// \param size initial size of the vector
	    explicit BOOST_UBLAS_INLINE
	    vector (size_type size):
	        vector_container<self_type> (),
	        data_ (size) {
	    }

	/// \brief Constructor of a vector by copying from another container
	/// This type has the generic name \c array_typ within the vector definition.
	/// \param size initial size of the vector \bug this value is not used
	/// \param data container of type \c A
	/// \todo remove this definition because \c size is not used
	    BOOST_UBLAS_INLINE
        vector (size_type /*size*/, const array_type &data):
	        vector_container<self_type> (),
	        data_ (data) {}

	/// \brief Constructor of a vector by copying from another container
	/// This type has the generic name \c array_typ within the vector definition.
	/// \param data container of type \c A
	     BOOST_UBLAS_INLINE
	     vector (const array_type &data):
	         vector_container<self_type> (),
	         data_ (data) {}

	/// \brief Constructor of a vector with a predefined size and a unique initial value
	/// \param size of the vector
	/// \param init value to assign to each element of the vector
	    BOOST_UBLAS_INLINE
	    vector (size_type size, const value_type &init):
	        vector_container<self_type> (),
	        data_ (size, init) {}

	/// \brief Copy-constructor of a vector
	/// \param v is the vector to be duplicated
	    BOOST_UBLAS_INLINE
	    vector (const vector &v):
	        vector_container<self_type> (),
	        data_ (v.data_) {}

	/// \brief Copy-constructor of a vector from a vector_expression
	/// Depending on the vector_expression, this constructor can have the cost of the computations 
	/// of the expression (trivial to say it, but it is to take into account in your complexity calculations).
	/// \param ae the vector_expression which values will be duplicated into the vector
	    template<class AE>
	    BOOST_UBLAS_INLINE
	    vector (const vector_expression<AE> &ae):
	        vector_container<self_type> (),
	        data_ (ae ().size ()) {
	        vector_assign<scalar_assign> (*this, ae);
	    }

	// -----------------------
	// Random Access Container
	// -----------------------
	
	/// \brief Return the maximum size of the data container.
	/// Return the upper bound (maximum size) on the data container. Depending on the container, it can be bigger than the current size of the vector.
	    BOOST_UBLAS_INLINE
	    size_type max_size () const {
	        return data_.max_size ();
	    }
	    
	/// \brief Return true if the vector is empty (\c size==0)
	/// \return \c true if empty, \c false otherwise
	    BOOST_UBLAS_INLINE
	    bool empty () const {
	        return data_.size () == 0;
	    }

	// ---------
	// Accessors
	// ---------
	
	/// \brief Return the size of the vector
	     BOOST_UBLAS_INLINE
	     size_type size () const {
	         return data_.size ();
	     }

	// -----------------
	// Storage accessors
	// -----------------
	
	/// \brief Return a \c const reference to the container. Useful to access data directly for specific type of container.
	     BOOST_UBLAS_INLINE
	     const array_type &data () const {
	         return data_;
	     }

	/// \brief Return a reference to the container. Useful to speed-up write operations to the data in very specific case.
	     BOOST_UBLAS_INLINE
	     array_type &data () {
	         return data_;
	     }

	// --------
	     // Resizing
	// --------
	
	/// \brief Resize the vector
	/// Resize the vector to a new size. If \c preserve is true, data are copied otherwise data are lost. If the new size is bigger, the remaining values are filled in with the initial value (0 by default) in the case of \c unbounded_array, which is the container by default. If the new size is smaller, last values are lost. This behaviour can be different if you explicitely specify another type of container.
	/// \param size new size of the vector
	/// \param preserve if true, keep values
	     BOOST_UBLAS_INLINE
	     void resize (size_type size, bool preserve = true) {
	         if (preserve)
	             data ().resize (size, typename A::value_type ());
	         else
	             data ().resize (size);
	     }

	// ---------------
	     // Element support
	// ---------------
	
	/// \brief Return a pointer to the element \f$i\f$
	/// \param i index of the element
	// XXX this semantic is not the one expected by the name of this method
	     BOOST_UBLAS_INLINE
	     pointer find_element (size_type i) {
	         return const_cast<pointer> (const_cast<const self_type&>(*this).find_element (i));
	     }

	/// \brief Return a const pointer to the element \f$i\f$
	/// \param i index of the element
	// XXX  this semantic is not the one expected by the name of this method
	     BOOST_UBLAS_INLINE
	     const_pointer find_element (size_type i) const {
	         return & (data () [i]);
	     }

	// --------------
	     // Element access
	// --------------

	/// \brief Return a const reference to the element \f$i\f$
	/// Return a const reference to the element \f$i\f$. With some compilers, this notation will be faster than \c[i]
	/// \param i index of the element
	     BOOST_UBLAS_INLINE
	     const_reference operator () (size_type i) const {
	         return data () [i];
	     }
	
	/// \brief Return a reference to the element \f$i\f$
	/// Return a reference to the element \f$i\f$. With some compilers, this notation will be faster than \c[i]
	/// \param i index of the element
	     BOOST_UBLAS_INLINE
	     reference operator () (size_type i) {
	         return data () [i];
	     }

	/// \brief Return a const reference to the element \f$i\f$
	/// \param i index of the element
	     BOOST_UBLAS_INLINE
	     const_reference operator [] (size_type i) const {
	         return (*this) (i);
	     }
	
	/// \brief Return a reference to the element \f$i\f$
	/// \param i index of the element
	     BOOST_UBLAS_INLINE
	     reference operator [] (size_type i) {
	         return (*this) (i);
	     }

	// ------------------
	     // Element assignment
	// ------------------
	
	/// \brief Set element \f$i\f$ to the value \c t
	/// \param i index of the element
	/// \param t reference to the value to be set
	// XXX semantic of this is to insert a new element and therefore size=size+1 ?
	     BOOST_UBLAS_INLINE
	     reference insert_element (size_type i, const_reference t) {
	         return (data () [i] = t);
	     }

	/// \brief Set element \f$i\f$ to the \e zero value
	/// \param i index of the element
	     BOOST_UBLAS_INLINE
	     void erase_element (size_type i) {
	         data () [i] = value_type/*zero*/();
	     }
	     
	// -------
	     // Zeroing
	// -------
	
	/// \brief Clear the vector, i.e. set all values to the \c zero value.
	     BOOST_UBLAS_INLINE
	     void clear () {
	         std::fill (data ().begin (), data ().end (), value_type/*zero*/());
	     }

	     // Assignment
#ifdef BOOST_UBLAS_MOVE_SEMANTICS

	/// \brief Assign a full vector (\e RHS-vector) to the current vector (\e LHS-vector)
	/// \param v is the source vector
	/// \return a reference to a vector (i.e. the destination vector)
	     /*! @note "pass by value" the key idea to enable move semantics */
	     BOOST_UBLAS_INLINE
	     vector &operator = (vector v) {
	         assign_temporary(v);
	         return *this;
	     }
#else
	/// \brief Assign a full vector (\e RHS-vector) to the current vector (\e LHS-vector)
	/// \param v is the source vector
	/// \return a reference to a vector (i.e. the destination vector)
	     BOOST_UBLAS_INLINE
	     vector &operator = (const vector &v) {
	         data () = v.data ();
	         return *this;
	     }
#endif

	/// \brief Assign a full vector (\e RHS-vector) to the current vector (\e LHS-vector)
	/// Assign a full vector (\e RHS-vector) to the current vector (\e LHS-vector). This method does not create any temporary.
	/// \param v is the source vector container
	/// \return a reference to a vector (i.e. the destination vector)
	     template<class C>          // Container assignment without temporary
	     BOOST_UBLAS_INLINE
	     vector &operator = (const vector_container<C> &v) {
	         resize (v ().size (), false);
	         assign (v);
	         return *this;
	     }

	/// \brief Assign a full vector (\e RHS-vector) to the current vector (\e LHS-vector)
	/// \param v is the source vector
	/// \return a reference to a vector (i.e. the destination vector)
	     BOOST_UBLAS_INLINE
	     vector &assign_temporary (vector &v) {
	         swap (v);
	         return *this;
	     }

	/// \brief Assign the result of a vector_expression to the vector
	/// Assign the result of a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// \tparam AE is the type of the vector_expression
	/// \param ae is a const reference to the vector_expression
	/// \return a reference to the resulting vector
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     vector &operator = (const vector_expression<AE> &ae) {
	         self_type temporary (ae);
	         return assign_temporary (temporary);
	     }

	/// \brief Assign the result of a vector_expression to the vector
	/// Assign the result of a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// \tparam AE is the type of the vector_expression
	/// \param ae is a const reference to the vector_expression
	/// \return a reference to the resulting vector
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     vector &assign (const vector_expression<AE> &ae) {
	         vector_assign<scalar_assign> (*this, ae);
	         return *this;
	     }

	// -------------------
	     // Computed assignment
	// -------------------
	
	/// \brief Assign the sum of the vector and a vector_expression to the vector
	/// Assign the sum of the vector and a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// A temporary is created for the computations.
	/// \tparam AE is the type of the vector_expression
	/// \param ae is a const reference to the vector_expression
	/// \return a reference to the resulting vector
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     vector &operator += (const vector_expression<AE> &ae) {
	         self_type temporary (*this + ae);
	         return assign_temporary (temporary);
	     }

	/// \brief Assign the sum of the vector and a vector_expression to the vector
	/// Assign the sum of the vector and a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// No temporary is created. Computations are done and stored directly into the resulting vector.
	/// \tparam AE is the type of the vector_expression
	/// \param ae is a const reference to the vector_expression
	/// \return a reference to the resulting vector
	     template<class C>          // Container assignment without temporary
	     BOOST_UBLAS_INLINE
	     vector &operator += (const vector_container<C> &v) {
	         plus_assign (v);
	         return *this;
	     }

	/// \brief Assign the sum of the vector and a vector_expression to the vector
	/// Assign the sum of the vector and a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// No temporary is created. Computations are done and stored directly into the resulting vector.
	/// \tparam AE is the type of the vector_expression
	/// \param ae is a const reference to the vector_expression
	/// \return a reference to the resulting vector
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     vector &plus_assign (const vector_expression<AE> &ae) {
	         vector_assign<scalar_plus_assign> (*this, ae);
	         return *this;
	     }
	
	/// \brief Assign the difference of the vector and a vector_expression to the vector
	/// Assign the difference of the vector and a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// A temporary is created for the computations.
	/// \tparam AE is the type of the vector_expression
	/// \param ae is a const reference to the vector_expression
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     vector &operator -= (const vector_expression<AE> &ae) {
	         self_type temporary (*this - ae);
	         return assign_temporary (temporary);
	     }

	/// \brief Assign the difference of the vector and a vector_expression to the vector
	/// Assign the difference of the vector and a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// No temporary is created. Computations are done and stored directly into the resulting vector.
	/// \tparam AE is the type of the vector_expression
	/// \param ae is a const reference to the vector_expression
	/// \return a reference to the resulting vector
	     template<class C>          // Container assignment without temporary
	     BOOST_UBLAS_INLINE
	     vector &operator -= (const vector_container<C> &v) {
	         minus_assign (v);
	         return *this;
	     }

	/// \brief Assign the difference of the vector and a vector_expression to the vector
	/// Assign the difference of the vector and a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// No temporary is created. Computations are done and stored directly into the resulting vector.
	/// \tparam AE is the type of the vector_expression
	/// \param ae is a const reference to the vector_expression
	/// \return a reference to the resulting vector
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     vector &minus_assign (const vector_expression<AE> &ae) {
	         vector_assign<scalar_minus_assign> (*this, ae);
	         return *this;
	     }

	/// \brief Assign the product of the vector and a scalar to the vector
	/// Assign the product of the vector and a scalar to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// No temporary is created. Computations are done and stored directly into the resulting vector.
	/// \tparam AE is the type of the vector_expression
	/// \param at is a const reference to the scalar
	/// \return a reference to the resulting vector
	     template<class AT>
	     BOOST_UBLAS_INLINE
	     vector &operator *= (const AT &at) {
	         vector_assign_scalar<scalar_multiplies_assign> (*this, at);
	         return *this;
	     }

	/// \brief Assign the division of the vector by a scalar to the vector
	/// Assign the division of the vector by a scalar to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
	/// No temporary is created. Computations are done and stored directly into the resulting vector.
	/// \tparam AE is the type of the vector_expression
	/// \param at is a const reference to the scalar
	/// \return a reference to the resulting vector
	    template<class AT>
	    BOOST_UBLAS_INLINE
	    vector &operator /= (const AT &at) {
	        vector_assign_scalar<scalar_divides_assign> (*this, at);
	        return *this;
	    }
	
	// --------
	    // Swapping
	// --------
	
	/// \brief Swap the content of the vector with another vector
	/// \param v is the vector to be swapped with
	    BOOST_UBLAS_INLINE
	    void swap (vector &v) {
	        if (this != &v) {
	            data ().swap (v.data ());
	        }
	    }

	/// \brief Swap the content of two vectors
	/// \param v1 is the first vector. It takes values from v2
	/// \param v2 is the second vector It takes values from v1
	     BOOST_UBLAS_INLINE
	     friend void swap (vector &v1, vector &v2) {
	         v1.swap (v2);
	     }

	     // Iterator types
	 private:
	     // Use the storage array iterator
	     typedef typename A::const_iterator const_subiterator_type;
	     typedef typename A::iterator subiterator_type;

	 public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
	     typedef indexed_iterator<self_type, dense_random_access_iterator_tag> iterator;
	     typedef indexed_const_iterator<self_type, dense_random_access_iterator_tag> const_iterator;
#else
	     class const_iterator;
	     class iterator;
#endif

	// --------------
	    // Element lookup
	// --------------
	
	/// \brief Return a const iterator to the element \e i
	/// \param i index of the element
	     BOOST_UBLAS_INLINE
	     const_iterator find (size_type i) const {
#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
	         return const_iterator (*this, data ().begin () + i);
#else
	         return const_iterator (*this, i);
#endif
	     }

	/// \brief Return an iterator to the element \e i
	/// \param i index of the element
	     BOOST_UBLAS_INLINE
	     iterator find (size_type i) {
#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
	         return iterator (*this, data ().begin () + i);
#else
	         return iterator (*this, i);
#endif
	     }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
	     class const_iterator:
	         public container_const_reference<vector>,
	         public random_access_iterator_base<dense_random_access_iterator_tag,
                   const_iterator, value_type, difference_type> {
	     public:
	         typedef typename vector::difference_type difference_type;
	         typedef typename vector::value_type value_type;
	         typedef typename vector::const_reference reference;
	         typedef const typename vector::pointer pointer;

	    // ----------------------------
	        // Construction and destruction
	    // ----------------------------
	
	
	        BOOST_UBLAS_INLINE
	        const_iterator ():
	            container_const_reference<self_type> (), it_ () {}
	        BOOST_UBLAS_INLINE
	        const_iterator (const self_type &v, const const_subiterator_type &it):
	            container_const_reference<self_type> (v), it_ (it) {}
	        BOOST_UBLAS_INLINE
	        const_iterator (const typename self_type::iterator &it):  // ISSUE vector:: stops VC8 using std::iterator here
	            container_const_reference<self_type> (it ()), it_ (it.it_) {}
	
	    // ----------
	        // Arithmetic
	    // ----------
	
	    /// \brief Increment by 1 the position of the iterator
	    /// \return a reference to the const iterator
	        BOOST_UBLAS_INLINE
	        const_iterator &operator ++ () {
	            ++ it_;
	            return *this;
	        }

	    /// \brief Decrement by 1 the position of the iterator
	    /// \return a reference to the const iterator
	        BOOST_UBLAS_INLINE
	        const_iterator &operator -- () {
	            -- it_;
	            return *this;
	        }
	  
	    /// \brief Increment by \e n the position of the iterator 
	    /// \return a reference to the const iterator
	        BOOST_UBLAS_INLINE
	        const_iterator &operator += (difference_type n) {
	            it_ += n;
	            return *this;
	        }
	
	    /// \brief Decrement by \e n the position of the iterator 
	    /// \return a reference to the const iterator
	        BOOST_UBLAS_INLINE
	        const_iterator &operator -= (difference_type n) {
	            it_ -= n;
	            return *this;
	        }
	
	    /// \brief Return the different in number of positions between 2 iterators
	        BOOST_UBLAS_INLINE
	        difference_type operator - (const const_iterator &it) const {
	            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	            return it_ - it.it_;
	        }
	
	        /// \brief Dereference an iterator
	        /// Dereference an iterator: a bounds' check is done before returning the value. A bad_index() expection is returned if out of bounds.
	    /// \return a const reference to the value pointed by the iterator
	        BOOST_UBLAS_INLINE
	        const_reference operator * () const {
	            BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_, bad_index ());
	            return *it_;
	        }
	
	    /// \brief Dereference an iterator at the n-th forward value
	    /// Dereference an iterator at the n-th forward value, that is the value pointed by iterator+n.
	        /// A bounds' check is done before returning the value. A bad_index() expection is returned if out of bounds.
	    /// \return a const reference
	        BOOST_UBLAS_INLINE
	        const_reference operator [] (difference_type n) const {
	            return *(it_ + n);
	        }
	
	        // Index
	    /// \brief return the index of the element referenced by the iterator
	         BOOST_UBLAS_INLINE
	         size_type index () const {
	             BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_, bad_index ());
	             return it_ - (*this) ().begin ().it_;
	         }

	         // Assignment
	         BOOST_UBLAS_INLINE
	    /// \brief assign the value of an iterator to the iterator	   
	         const_iterator &operator = (const const_iterator &it) {
	             container_const_reference<self_type>::assign (&it ());
	             it_ = it.it_;
	             return *this;
	         }

	         // Comparison
	    /// \brief compare the value of two itetarors
	    /// \return true if they reference the same element
	        BOOST_UBLAS_INLINE
	        bool operator == (const const_iterator &it) const {
	            BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	            return it_ == it.it_;
	        }

	
	    /// \brief compare the value of two iterators
	    /// \return return true if the left-hand-side iterator refers to a value placed before the right-hand-side iterator
	         BOOST_UBLAS_INLINE
	         bool operator < (const const_iterator &it) const {
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ < it.it_;
	         }

	     private:
	         const_subiterator_type it_;

	         friend class iterator;
	     };
#endif

	/// \brief return an iterator on the first element of the vector
	    BOOST_UBLAS_INLINE
	    const_iterator begin () const {
	        return find (0);
	    }

    /// \brief return an iterator on the first element of the vector
        BOOST_UBLAS_INLINE
        const_iterator cbegin () const {
            return begin ();
        }

	/// \brief return an iterator after the last element of the vector
        BOOST_UBLAS_INLINE
        const_iterator end () const {
            return find (data_.size ());
        }

    /// \brief return an iterator after the last element of the vector
         BOOST_UBLAS_INLINE
         const_iterator cend () const {
             return end ();
         }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
	     class iterator:
	         public container_reference<vector>,
	         public random_access_iterator_base<dense_random_access_iterator_tag,
	                                            iterator, value_type, difference_type> {
	     public:
	         typedef typename vector::difference_type difference_type;
	         typedef typename vector::value_type value_type;
	         typedef typename vector::reference reference;
	         typedef typename vector::pointer pointer;


	         // Construction and destruction
	         BOOST_UBLAS_INLINE
	         iterator ():
	             container_reference<self_type> (), it_ () {}
	         BOOST_UBLAS_INLINE
	         iterator (self_type &v, const subiterator_type &it):
	             container_reference<self_type> (v), it_ (it) {}

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
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ - it.it_;
	         }

	         // Dereference
	         BOOST_UBLAS_INLINE
	         reference operator * () const {
	             BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_ , bad_index ());
	             return *it_;
	         }
	         BOOST_UBLAS_INLINE
	         reference operator [] (difference_type n) const {
	             return *(it_ + n);
	         }

	         // Index
	         BOOST_UBLAS_INLINE
	         size_type index () const {
	             BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_ , bad_index ());
	             return it_ - (*this) ().begin ().it_;
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
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ == it.it_;
	         }
	         BOOST_UBLAS_INLINE
	         bool operator < (const iterator &it) const {
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ < it.it_;
	         }

	     private:
	         subiterator_type it_;

	         friend class const_iterator;
	     };
#endif

	/// \brief Return an iterator on the first element of the vector
	    BOOST_UBLAS_INLINE
	    iterator begin () {
	        return find (0);
	    }
	
	/// \brief Return an iterator at the end of the vector
	    BOOST_UBLAS_INLINE
	    iterator end () {
	        return find (data_.size ());
	    }
	
	    // Reverse iterator
	    typedef reverse_iterator_base<const_iterator> const_reverse_iterator;
	    typedef reverse_iterator_base<iterator> reverse_iterator;
	
	/// \brief Return a const reverse iterator before the first element of the reversed vector (i.e. end() of normal vector)
	    BOOST_UBLAS_INLINE
	    const_reverse_iterator rbegin () const {
	        return const_reverse_iterator (end ());
	    }
	
    /// \brief Return a const reverse iterator before the first element of the reversed vector (i.e. end() of normal vector)
        BOOST_UBLAS_INLINE
        const_reverse_iterator crbegin () const {
            return rbegin ();
        }

	/// \brief Return a const reverse iterator on the end of the reverse vector (i.e. first element of the normal vector) 
	    BOOST_UBLAS_INLINE
	    const_reverse_iterator rend () const {
	        return const_reverse_iterator (begin ());
	    }
	
    /// \brief Return a const reverse iterator on the end of the reverse vector (i.e. first element of the normal vector)
        BOOST_UBLAS_INLINE
        const_reverse_iterator crend () const {
            return rend ();
        }

	/// \brief Return a const reverse iterator before the first element of the reversed vector (i.e. end() of normal vector)
	    BOOST_UBLAS_INLINE
	    reverse_iterator rbegin () {
	        return reverse_iterator (end ());
	    }
	
	/// \brief Return a const reverse iterator on the end of the reverse vector (i.e. first element of the normal vector) 
	    BOOST_UBLAS_INLINE
	    reverse_iterator rend () {
	        return reverse_iterator (begin ());
	    }
	
	// -------------
	    // Serialization
	// -------------
	
	/// Serialize a vector into and archive as defined in Boost
	/// \param ar Archive object. Can be a flat file, an XML file or any other stream
	/// \param file_version Optional file version (not yet used)
	     template<class Archive>
	     void serialize(Archive & ar, const unsigned int /* file_version */){
	         ar & serialization::make_nvp("data",data_);
	     }

	 private:
	     array_type data_;
	 };


#ifdef BOOST_UBLAS_CPP_GE_2011
     /** \brief A dense vector of values of type \c T.
      *
      * For a \f$n\f$-dimensional vector \f$v\f$ and \f$0\leq i < n\f$ every element \f$v_i\f$ is mapped
      * to the \f$i\f$-th element of the container. A storage type \c A can be specified which defaults to \c std::array.
      * Elements are constructed by \c A, which need not initialise their value.
      *
      * \tparam T type of the objects stored in the vector (like int, double, complex,...)
      * \tparam A The type of the storage array of the vector. Default is \c std::array<T>.
      */
     template<class T, std::size_t N, class A>
     class fixed_vector:
         public vector_container<fixed_vector<T, N, A> > {

         typedef fixed_vector<T, N, A> self_type;
     public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
         using vector_container<self_type>::operator ();
#endif

        typedef typename A::size_type       size_type;
        typedef typename A::difference_type difference_type;
        typedef T value_type;
        typedef typename type_traits<T>::const_reference const_reference;
        typedef T &reference;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef A array_type;
        typedef const vector_reference<const self_type> const_closure_type;
        typedef vector_reference<self_type> closure_type;
        typedef self_type vector_temporary_type;
        typedef dense_tag storage_category;

        // Construction and destruction

    /// \brief Constructor of a fixed_vector
        BOOST_UBLAS_INLINE
        fixed_vector ():
            vector_container<self_type> (),
            data_ () {}

    /// \brief Constructor of a fixed_vector by copying from another container
    /// This type uses the generic name \c array_type within the vector definition.
    /// \param data container of type \c A
         BOOST_UBLAS_INLINE
         fixed_vector (const array_type &data):
             vector_container<self_type> (),
             data_ (data) {}

    /// \brief Constructor of a fixed_vector with a unique initial value
    /// \param init value to assign to each element of the vector
         BOOST_UBLAS_INLINE
         fixed_vector (const value_type &init):
             vector_container<self_type> (),
             data_ () {
             data_.fill( init );
         }

    /// \brief Copy-constructor of a fixed_vector
    /// \param v is the fixed_vector to be duplicated
        BOOST_UBLAS_INLINE
        fixed_vector (const fixed_vector &v):
            vector_container<self_type> (),
            data_ (v.data_) {}

    /// \brief Copy-constructor of a vector from a vector_expression
    /// Depending on the vector_expression, this constructor can have the cost of the computations
    /// of the expression (trivial to say it, but take it must be taken into account in your complexity calculations).
    /// \param ae the vector_expression which values will be duplicated into the vector
        template<class AE>
        BOOST_UBLAS_INLINE
        fixed_vector (const vector_expression<AE> &ae):
            vector_container<self_type> (),
            data_ ( ) {
            vector_assign<scalar_assign> (*this, ae);
        }

    /// \brief Construct a fixed_vector from a list of values
    /// This constructor enables initialization by using any of:
    /// fixed_vector<double, 3> v = { 1, 2, 3 } or fixed_vector<double,3> v( {1, 2, 3} ) or fixed_vector<double,3> v( 1, 2, 3 )
        template <typename... Types>
        BOOST_UBLAS_INLINE
        fixed_vector(value_type v0, Types... vrest) :
            vector_container<self_type> (),
            data_( array_type{ v0, vrest... } ) {}

    // -----------------------
    // Random Access Container
    // -----------------------

    /// \brief Return the maximum size of the data container.
    /// Return the upper bound (maximum size) on the data container. Depending on the container, it can be bigger than the current size of the vector.
        BOOST_UBLAS_INLINE
        size_type max_size () const {
            return data_.max_size ();
        }

    /// \brief Return true if the vector is empty (\c size==0)
    /// \return \c true if empty, \c false otherwise
        BOOST_UBLAS_INLINE
        const bool &empty () const {
            return data_.empty();
        }

    // ---------
    // Accessors
    // ---------

    /// \brief Return the size of the vector
         BOOST_UBLAS_INLINE
         BOOST_CONSTEXPR size_type size () const{ // should have a const after C++14
             return data_.size ();
         }

    // -----------------
    // Storage accessors
    // -----------------

    /// \brief Return a \c const reference to the container. Useful to access data directly for specific type of container.
         BOOST_UBLAS_INLINE
         const array_type &data () const {
             return data_;
         }

    /// \brief Return a reference to the container. Useful to speed-up write operations to the data in very specific case.
         BOOST_UBLAS_INLINE
         array_type &data () {
             return data_;
         }

    // ---------------
         // Element support
    // ---------------

    /// \brief Return a pointer to the element \f$i\f$
    /// \param i index of the element
    // XXX this semantic is not the one expected by the name of this method
         BOOST_UBLAS_INLINE
         pointer find_element (size_type i) {
             return const_cast<pointer> (const_cast<const self_type&>(*this).find_element (i));
         }

    /// \brief Return a const pointer to the element \f$i\f$
    /// \param i index of the element
    // XXX  this semantic is not the one expected by the name of this method
         BOOST_UBLAS_INLINE
         const_pointer find_element (size_type i) const {
             BOOST_UBLAS_CHECK (i < data_.size(), bad_index() ); // Since std:array doesn't check for bounds
             return & (data () [i]);
         }

    // --------------
         // Element access
    // --------------

    /// \brief Return a const reference to the element \f$i\f$
    /// Return a const reference to the element \f$i\f$. With some compilers, this notation will be faster than \c[i]
    /// \param i index of the element
         BOOST_UBLAS_INLINE
         const_reference operator () (size_type i) const {
             BOOST_UBLAS_CHECK (i < data_.size(), bad_index() );
             return data () [i];
         }

    /// \brief Return a reference to the element \f$i\f$
    /// Return a reference to the element \f$i\f$. With some compilers, this notation will be faster than \c[i]
    /// \param i index of the element
         BOOST_UBLAS_INLINE
         reference operator () (size_type i) {
             BOOST_UBLAS_CHECK (i < data_.size(), bad_index() );
             return data () [i];
         }

    /// \brief Return a const reference to the element \f$i\f$
    /// \param i index of the element
         BOOST_UBLAS_INLINE
         const_reference operator [] (size_type i) const {
             BOOST_UBLAS_CHECK (i < data_.size(), bad_index() );
             return (*this) (i);
         }

    /// \brief Return a reference to the element \f$i\f$
    /// \param i index of the element
         BOOST_UBLAS_INLINE
         reference operator [] (size_type i) {
             BOOST_UBLAS_CHECK (i < data_.size(), bad_index() );
             return (*this) (i);
         }

    // ------------------
         // Element assignment
    // ------------------

    /// \brief Set element \f$i\f$ to the value \c t
    /// \param i index of the element
    /// \param t reference to the value to be set
    // XXX semantic of this is to insert a new element and therefore size=size+1 ?
         BOOST_UBLAS_INLINE
         reference insert_element (size_type i, const_reference t) {
             BOOST_UBLAS_CHECK (i < data_.size(), bad_index ());
             return (data () [i] = t);
         }

    /// \brief Set element \f$i\f$ to the \e zero value
    /// \param i index of the element
         BOOST_UBLAS_INLINE
         void erase_element (size_type i) {
             BOOST_UBLAS_CHECK (i < data_.size(), bad_index ());
             data () [i] = value_type/*zero*/();
         }

    // -------
         // Zeroing
    // -------

    /// \brief Clear the vector, i.e. set all values to the \c zero value.
         BOOST_UBLAS_INLINE
         void clear () {
             std::fill (data ().begin (), data ().end (), value_type/*zero*/());
         }

         // Assignment
#ifdef BOOST_UBLAS_MOVE_SEMANTICS

    /// \brief Assign a full fixed_vector (\e RHS-vector) to the current fixed_vector (\e LHS-vector)
    /// \param v is the source vector
    /// \return a reference to a fixed_vector (i.e. the destination vector)
         /*! @note "pass by value" the key idea to enable move semantics */
         BOOST_UBLAS_INLINE
         fixed_vector &operator = (fixed_vector v) {
             assign_temporary(v);
             return *this;
         }
#else
    /// \brief Assign a full fixed_vector (\e RHS-vector) to the current fixed_vector (\e LHS-vector)
    /// \param v is the source fixed_vector
    /// \return a reference to a fixed_vector (i.e. the destination vector)
         BOOST_UBLAS_INLINE
         fixed_vector &operator = (const fixed_vector &v) {
             data () = v.data ();
             return *this;
         }
#endif

    /// \brief Assign a full vector (\e RHS-vector) to the current fixed_vector (\e LHS-vector)
    /// Assign a full vector (\e RHS-vector) to the current fixed_vector (\e LHS-vector). This method does not create any temporary.
    /// \param v is the source vector container
    /// \return a reference to a vector (i.e. the destination vector)
         template<class C>          // Container assignment without temporary
         BOOST_UBLAS_INLINE
         fixed_vector &operator = (const vector_container<C> &v) {
             assign (v);
             return *this;
         }

    /// \brief Assign a full fixed_vector (\e RHS-vector) to the current fixed_vector (\e LHS-vector)
    /// \param v is the source fixed_vector
    /// \return a reference to a fixed_vector (i.e. the destination fixed_vector)
         BOOST_UBLAS_INLINE
         fixed_vector &assign_temporary (fixed_vector &v) {
             swap ( v );
             return *this;
         }

    /// \brief Assign the result of a vector_expression to the fixed_vector
    /// Assign the result of a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// \tparam AE is the type of the vector_expression
    /// \param ae is a const reference to the vector_expression
    /// \return a reference to the resulting fixed_vector
         template<class AE>
         BOOST_UBLAS_INLINE
         fixed_vector &operator = (const vector_expression<AE> &ae) {
             self_type temporary (ae);
             return assign_temporary (temporary);
         }

    /// \brief Assign the result of a vector_expression to the fixed_vector
    /// Assign the result of a vector_expression to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// \tparam AE is the type of the vector_expression
    /// \param ae is a const reference to the vector_expression
    /// \return a reference to the resulting fixed_vector
         template<class AE>
         BOOST_UBLAS_INLINE
         fixed_vector &assign (const vector_expression<AE> &ae) {
             vector_assign<scalar_assign> (*this, ae);
             return *this;
         }

    // -------------------
         // Computed assignment
    // -------------------

    /// \brief Assign the sum of the fixed_vector and a vector_expression to the fixed_vector
    /// Assign the sum of the fixed_vector and a vector_expression to the fixed_vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// A temporary is created for the computations.
    /// \tparam AE is the type of the vector_expression
    /// \param ae is a const reference to the vector_expression
    /// \return a reference to the resulting fixed_vector
         template<class AE>
         BOOST_UBLAS_INLINE
         fixed_vector &operator += (const vector_expression<AE> &ae) {
             self_type temporary (*this + ae);
             return assign_temporary (temporary);
         }

    /// \brief Assign the sum of the fixed_vector and a vector_expression to the fixed_vector
    /// Assign the sum of the fixed_vector and a vector_expression to the fixed_vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// No temporary is created. Computations are done and stored directly into the resulting vector.
    /// \tparam AE is the type of the vector_expression
    /// \param ae is a const reference to the vector_expression
    /// \return a reference to the resulting vector
         template<class C>          // Container assignment without temporary
         BOOST_UBLAS_INLINE
         fixed_vector &operator += (const vector_container<C> &v) {
             plus_assign (v);
             return *this;
         }

    /// \brief Assign the sum of the fixed_vector and a vector_expression to the fixed_vector
    /// Assign the sum of the fixed_vector and a vector_expression to the fixed_vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// No temporary is created. Computations are done and stored directly into the resulting fixed_vector.
    /// \tparam AE is the type of the vector_expression
    /// \param ae is a const reference to the vector_expression
    /// \return a reference to the resulting vector
         template<class AE>
         BOOST_UBLAS_INLINE
         fixed_vector &plus_assign (const vector_expression<AE> &ae) {
             vector_assign<scalar_plus_assign> (*this, ae);
             return *this;
         }

    /// \brief Assign the difference of the fixed_vector and a vector_expression to the fixed_vector
    /// Assign the difference of the fixed_vector and a vector_expression to the fixed_vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// A temporary is created for the computations.
    /// \tparam AE is the type of the vector_expression
    /// \param ae is a const reference to the vector_expression
         template<class AE>
         BOOST_UBLAS_INLINE
         fixed_vector &operator -= (const vector_expression<AE> &ae) {
             self_type temporary (*this - ae);
             return assign_temporary (temporary);
         }

    /// \brief Assign the difference of the fixed_vector and a vector_expression to the fixed_vector
    /// Assign the difference of the fixed_vector and a vector_expression to the fixed_vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// No temporary is created. Computations are done and stored directly into the resulting fixed_vector.
    /// \tparam AE is the type of the vector_expression
    /// \param ae is a const reference to the vector_expression
    /// \return a reference to the resulting vector
         template<class C>          // Container assignment without temporary
         BOOST_UBLAS_INLINE
         fixed_vector &operator -= (const vector_container<C> &v) {
             minus_assign (v);
             return *this;
         }

    /// \brief Assign the difference of the fixed_vector and a vector_expression to the fixed_vector
    /// Assign the difference of the fixed_vector and a vector_expression to the fixed_vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// No temporary is created. Computations are done and stored directly into the resulting fixed_vector.
    /// \tparam AE is the type of the vector_expression
    /// \param ae is a const reference to the vector_expression
    /// \return a reference to the resulting fixed_vector
         template<class AE>
         BOOST_UBLAS_INLINE
         fixed_vector &minus_assign (const vector_expression<AE> &ae) {
             vector_assign<scalar_minus_assign> (*this, ae);
             return *this;
         }

    /// \brief Assign the product of the fixed_vector and a scalar to the fixed_vector
    /// Assign the product of the fixed_vector and a scalar to the fixed_vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// No temporary is created. Computations are done and stored directly into the resulting fixed_vector.
    /// \tparam AE is the type of the vector_expression
    /// \param at is a const reference to the scalar
    /// \return a reference to the resulting fixed_vector
         template<class AT>
         BOOST_UBLAS_INLINE
         fixed_vector &operator *= (const AT &at) {
             vector_assign_scalar<scalar_multiplies_assign> (*this, at);
             return *this;
         }

    /// \brief Assign the division of the fixed_vector by a scalar to the fixed_vector
    /// Assign the division of the fixed_vector by a scalar to the vector. This is lazy-compiled and will be optimized out by the compiler on any type of expression.
    /// No temporary is created. Computations are done and stored directly into the resulting vector.
    /// \tparam AE is the type of the vector_expression
    /// \param at is a const reference to the scalar
    /// \return a reference to the resulting fixed_vector
        template<class AT>
        BOOST_UBLAS_INLINE
        fixed_vector &operator /= (const AT &at) {
            vector_assign_scalar<scalar_divides_assign> (*this, at);
            return *this;
        }

    // --------
        // Swapping
    // --------

    /// \brief Swap the content of the fixed_vector with another vector
    /// \param v is the fixed_vector to be swapped with
        BOOST_UBLAS_INLINE
        void swap (fixed_vector &v) {
            if (this != &v) {
                data ().swap (v.data ());
            }
        }

    /// \brief Swap the content of two fixed_vectors
    /// \param v1 is the first fixed_vector. It takes values from v2
    /// \param v2 is the second fixed_vector It takes values from v1
         BOOST_UBLAS_INLINE
         friend void swap (fixed_vector &v1, fixed_vector &v2) {
             v1.swap (v2);
         }

         // Iterator types
     private:
         // Use the storage array iterator
         typedef typename A::const_iterator const_subiterator_type;
         typedef typename A::iterator subiterator_type;

     public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
         typedef indexed_iterator<self_type, dense_random_access_iterator_tag> iterator;
         typedef indexed_const_iterator<self_type, dense_random_access_iterator_tag> const_iterator;
#else
         class const_iterator;
         class iterator;
#endif

    // --------------
        // Element lookup
    // --------------

    /// \brief Return a const iterator to the element \e i
    /// \param i index of the element
         BOOST_UBLAS_INLINE
         const_iterator find (size_type i) const {
#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
             return const_iterator (*this, data ().begin () + i);
#else
             return const_iterator (*this, i);
#endif
         }

    /// \brief Return an iterator to the element \e i
    /// \param i index of the element
         BOOST_UBLAS_INLINE
         iterator find (size_type i) {
#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
             return iterator (*this, data ().begin () + i);
#else
             return iterator (*this, i);
#endif
         }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
         class const_iterator:
             public container_const_reference<fixed_vector>,
             public random_access_iterator_base<dense_random_access_iterator_tag,
                   const_iterator, value_type, difference_type> {
         public:
             typedef typename fixed_vector::difference_type difference_type;
             typedef typename fixed_vector::value_type value_type;
             typedef typename fixed_vector::const_reference reference;
             typedef const typename fixed_vector::pointer pointer;

        // ----------------------------
            // Construction and destruction
        // ----------------------------


            BOOST_UBLAS_INLINE
            const_iterator ():
                container_const_reference<self_type> (), it_ () {}
            BOOST_UBLAS_INLINE
            const_iterator (const self_type &v, const const_subiterator_type &it):
                container_const_reference<self_type> (v), it_ (it) {}
            BOOST_UBLAS_INLINE
            const_iterator (const typename self_type::iterator &it):  // ISSUE vector:: stops VC8 using std::iterator here
                container_const_reference<self_type> (it ()), it_ (it.it_) {}

        // ----------
            // Arithmetic
        // ----------

        /// \brief Increment by 1 the position of the iterator
        /// \return a reference to the const iterator
            BOOST_UBLAS_INLINE
            const_iterator &operator ++ () {
                ++ it_;
                return *this;
            }

        /// \brief Decrement by 1 the position of the iterator
        /// \return a reference to the const iterator
            BOOST_UBLAS_INLINE
            const_iterator &operator -- () {
                -- it_;
                return *this;
            }

        /// \brief Increment by \e n the position of the iterator
        /// \return a reference to the const iterator
            BOOST_UBLAS_INLINE
            const_iterator &operator += (difference_type n) {
                it_ += n;
                return *this;
            }

        /// \brief Decrement by \e n the position of the iterator
        /// \return a reference to the const iterator
            BOOST_UBLAS_INLINE
            const_iterator &operator -= (difference_type n) {
                it_ -= n;
                return *this;
            }

        /// \brief Return the different in number of positions between 2 iterators
            BOOST_UBLAS_INLINE
            difference_type operator - (const const_iterator &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ - it.it_;
            }

            /// \brief Dereference an iterator
            /// Dereference an iterator: a bounds' check is done before returning the value. A bad_index() expection is returned if out of bounds.
        /// \return a const reference to the value pointed by the iterator
            BOOST_UBLAS_INLINE
            const_reference operator * () const {
                BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_, bad_index ());
                return *it_;
            }

        /// \brief Dereference an iterator at the n-th forward value
        /// Dereference an iterator at the n-th forward value, that is the value pointed by iterator+n.
            /// A bounds' check is done before returning the value. A bad_index() expection is returned if out of bounds.
        /// \return a const reference
            BOOST_UBLAS_INLINE
            const_reference operator [] (difference_type n) const {
                return *(it_ + n);
            }

            // Index
        /// \brief return the index of the element referenced by the iterator
             BOOST_UBLAS_INLINE
             size_type index () const {
                 BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_, bad_index ());
                 return it_ - (*this) ().begin ().it_;
             }

             // Assignment
             BOOST_UBLAS_INLINE
        /// \brief assign the value of an iterator to the iterator
             const_iterator &operator = (const const_iterator &it) {
                 container_const_reference<self_type>::assign (&it ());
                 it_ = it.it_;
                 return *this;
             }

             // Comparison
        /// \brief compare the value of two itetarors
        /// \return true if they reference the same element
            BOOST_UBLAS_INLINE
            bool operator == (const const_iterator &it) const {
                BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                return it_ == it.it_;
            }


        /// \brief compare the value of two iterators
        /// \return return true if the left-hand-side iterator refers to a value placed before the right-hand-side iterator
             BOOST_UBLAS_INLINE
             bool operator < (const const_iterator &it) const {
                 BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                 return it_ < it.it_;
             }

         private:
             const_subiterator_type it_;

             friend class iterator;
         };
#endif

    /// \brief return an iterator on the first element of the fixed_vector
        BOOST_UBLAS_INLINE
        const_iterator begin () const {
            return find (0);
        }

    /// \brief return an iterator on the first element of the fixed_vector
        BOOST_UBLAS_INLINE
        const_iterator cbegin () const {
            return begin ();
        }

    /// \brief return an iterator after the last element of the fixed_vector
         BOOST_UBLAS_INLINE
         const_iterator end () const {
             return find (data_.size ());
         }

    /// \brief return an iterator after the last element of the fixed_vector
         BOOST_UBLAS_INLINE
         const_iterator cend () const {
             return end ();
         }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
         class iterator:
             public container_reference<fixed_vector>,
             public random_access_iterator_base<dense_random_access_iterator_tag,
                                                iterator, value_type, difference_type> {
         public:
             typedef typename fixed_vector::difference_type difference_type;
             typedef typename fixed_vector::value_type value_type;
             typedef typename fixed_vector::reference reference;
             typedef typename fixed_vector::pointer pointer;


             // Construction and destruction
             BOOST_UBLAS_INLINE
             iterator ():
                 container_reference<self_type> (), it_ () {}
             BOOST_UBLAS_INLINE
             iterator (self_type &v, const subiterator_type &it):
                 container_reference<self_type> (v), it_ (it) {}

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
                 BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                 return it_ - it.it_;
             }

             // Dereference
             BOOST_UBLAS_INLINE
             reference operator * () const {
                 BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_ , bad_index ());
                 return *it_;
             }
             BOOST_UBLAS_INLINE
             reference operator [] (difference_type n) const {
                 return *(it_ + n);
             }

             // Index
             BOOST_UBLAS_INLINE
             size_type index () const {
                 BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_ , bad_index ());
                 return it_ - (*this) ().begin ().it_;
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
                 BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                 return it_ == it.it_;
             }
             BOOST_UBLAS_INLINE
             bool operator < (const iterator &it) const {
                 BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
                 return it_ < it.it_;
             }

         private:
             subiterator_type it_;

             friend class const_iterator;
         };
#endif

    /// \brief Return an iterator on the first element of the fixed_vector
        BOOST_UBLAS_INLINE
        iterator begin () {
            return find (0);
        }

    /// \brief Return an iterator at the end of the fixed_vector
        BOOST_UBLAS_INLINE
        iterator end () {
            return find (data_.size ());
        }

        // Reverse iterator
        typedef reverse_iterator_base<const_iterator> const_reverse_iterator;
        typedef reverse_iterator_base<iterator> reverse_iterator;

    /// \brief Return a const reverse iterator before the first element of the reversed fixed_vector (i.e. end() of normal fixed_vector)
        BOOST_UBLAS_INLINE
        const_reverse_iterator rbegin () const {
            return const_reverse_iterator (end ());
        }

    /// \brief Return a const reverse iterator before the first element of the reversed fixed_vector (i.e. end() of normal fixed_vector)
        BOOST_UBLAS_INLINE
        const_reverse_iterator crbegin () const {
            return rbegin ();
        }

    /// \brief Return a const reverse iterator on the end of the reverse fixed_vector (i.e. first element of the normal fixed_vector)
        BOOST_UBLAS_INLINE
        const_reverse_iterator rend () const {
            return const_reverse_iterator (begin ());
        }

    /// \brief Return a const reverse iterator on the end of the reverse fixed_vector (i.e. first element of the normal fixed_vector)
        BOOST_UBLAS_INLINE
        const_reverse_iterator crend () const {
            return rend ();
        }

    /// \brief Return a const reverse iterator before the first element of the reversed fixed_vector (i.e. end() of normal fixed_vector)
        BOOST_UBLAS_INLINE
        reverse_iterator rbegin () {
            return reverse_iterator (end ());
        }

    /// \brief Return a const reverse iterator on the end of the reverse fixed_vector (i.e. first element of the normal fixed_vector)
        BOOST_UBLAS_INLINE
        reverse_iterator rend () {
            return reverse_iterator (begin ());
        }

    // -------------
        // Serialization
    // -------------

    /// Serialize a fixed_vector into and archive as defined in Boost
    /// \param ar Archive object. Can be a flat file, an XML file or any other stream
    /// \param file_version Optional file version (not yet used)
         template<class Archive>
         void serialize(Archive & ar, const unsigned int /* file_version */){
             ar & serialization::make_nvp("data",data_);
         }

     private:
         array_type data_;
     };

#endif // BOOST_UBLAS_CPP_GE_2011

	 // --------------------
	 // Bounded vector class
	 // --------------------

	 /// \brief a dense vector of values of type \c T, of variable size but with maximum \f$N\f$.
	 /// A dense vector of values of type \c T, of variable size but with maximum \f$N\f$.  The default constructor 
	 /// creates the vector with size \f$N\f$. Elements are constructed by the storage type \c bounded_array, which \b need \b not \b initialise their value.
	 template<class T, std::size_t N>
	 class bounded_vector:
	     public vector<T, bounded_array<T, N> > {

	     typedef vector<T, bounded_array<T, N> > vector_type;
	 public:
	     typedef typename vector_type::size_type size_type;
	     static const size_type max_size = N;

	     // Construction and destruction
	     BOOST_UBLAS_INLINE
	     bounded_vector ():
	         vector_type (N) {}
	     BOOST_UBLAS_INLINE
	     bounded_vector (size_type size):
	         vector_type (size) {}
	     BOOST_UBLAS_INLINE
	     bounded_vector (const bounded_vector &v):
	         vector_type (v) {}
	     template<class A2>              // Allow vector<T,bounded_array<N> construction
	     BOOST_UBLAS_INLINE
	     bounded_vector (const vector<T, A2> &v):
	         vector_type (v) {}
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     bounded_vector (const vector_expression<AE> &ae):
	         vector_type (ae) {}
	     BOOST_UBLAS_INLINE
	     ~bounded_vector () {}

	     // Assignment
#ifdef BOOST_UBLAS_MOVE_SEMANTICS

	     /*! @note "pass by value" the key idea to enable move semantics */
	     BOOST_UBLAS_INLINE
	     bounded_vector &operator = (bounded_vector v) {
	         vector_type::operator = (v);
	         return *this;
	     }
#else
	     BOOST_UBLAS_INLINE
	     bounded_vector &operator = (const bounded_vector &v) {
	         vector_type::operator = (v);
	         return *this;
	     }
#endif
	     template<class A2>         // Generic vector assignment
	     BOOST_UBLAS_INLINE
	     bounded_vector &operator = (const vector<T, A2> &v) {
	         vector_type::operator = (v);
	         return *this;
	     }
	     template<class C>          // Container assignment without temporary
	     BOOST_UBLAS_INLINE
	     bounded_vector &operator = (const vector_container<C> &v) {
	         vector_type::operator = (v);
	         return *this;
	     }
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     bounded_vector &operator = (const vector_expression<AE> &ae) {
	         vector_type::operator = (ae);
	         return *this;
	     }
	 };



	 // -----------------
	 // Zero vector class
	 // -----------------
	 
	 /// \brief A zero vector of type \c T and a given \c size
	 /// A zero vector of type \c T and a given \c size. This is a virtual vector in the sense that no memory is allocated 
	 /// for storing the zero values: it still acts like any other vector. However assigning values to it will not change the zero
	 /// vector into a normal vector. It must first be assigned to another normal vector by any suitable means. Its memory footprint is constant.
	 template<class T, class ALLOC>
	 class zero_vector:
	     public vector_container<zero_vector<T, ALLOC> > {

	     typedef const T *const_pointer;
	     typedef zero_vector<T, ALLOC> self_type;
	 public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
	     using vector_container<self_type>::operator ();
#endif
         typedef typename boost::allocator_size_type<ALLOC>::type size_type;
         typedef typename boost::allocator_difference_type<ALLOC>::type difference_type;
	     typedef T value_type;
	     typedef const T &const_reference;
	     typedef T &reference;
	     typedef const vector_reference<const self_type> const_closure_type;
	     typedef vector_reference<self_type> closure_type;
	     typedef sparse_tag storage_category;

	     // Construction and destruction
	     BOOST_UBLAS_INLINE
	     zero_vector ():
	         vector_container<self_type> (),
	         size_ (0) {}
	     explicit BOOST_UBLAS_INLINE
	     zero_vector (size_type size):
	         vector_container<self_type> (),
	         size_ (size) {}
	     BOOST_UBLAS_INLINE
	     zero_vector (const zero_vector &v):
	         vector_container<self_type> (),
	         size_ (v.size_) {}

	     // Accessors
	     BOOST_UBLAS_INLINE
	     size_type size () const {
	         return size_;
	     }

	     // Resizing
	     BOOST_UBLAS_INLINE
	     void resize (size_type size, bool /*preserve*/ = true) {
	         size_ = size;
	     }

	     // Element support
	     BOOST_UBLAS_INLINE
         const_pointer find_element (size_type /*i*/) const {
	         return & zero_;
	     }

	     // Element access
	     BOOST_UBLAS_INLINE
	     const_reference operator () (size_type /* i */) const {
	         return zero_;
	     }

	     BOOST_UBLAS_INLINE
	     const_reference operator [] (size_type i) const {
	         return (*this) (i);
	     }

	     // Assignment
	     BOOST_UBLAS_INLINE
	     zero_vector &operator = (const zero_vector &v) {
	         size_ = v.size_;
	         return *this;
	     }
	     BOOST_UBLAS_INLINE
	     zero_vector &assign_temporary (zero_vector &v) {
	         swap (v);
	         return *this;
	     }

	     // Swapping
	     BOOST_UBLAS_INLINE
	     void swap (zero_vector &v) {
	         if (this != &v) {
	             std::swap (size_, v.size_);
	         }
	     }
	     BOOST_UBLAS_INLINE
	     friend void swap (zero_vector &v1, zero_vector &v2) {
	         v1.swap (v2);
	     }

	     // Iterator types
	 public:
	     class const_iterator;

	     // Element lookup
	     BOOST_UBLAS_INLINE
	     const_iterator find (size_type /*i*/) const {
	         return const_iterator (*this);
	     }

	     class const_iterator:
	         public container_const_reference<zero_vector>,
	         public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
	                                            const_iterator, value_type> {
	     public:
	         typedef typename zero_vector::difference_type difference_type;
	         typedef typename zero_vector::value_type value_type;
	         typedef typename zero_vector::const_reference reference;
	         typedef typename zero_vector::const_pointer pointer;

	         // Construction and destruction
	         BOOST_UBLAS_INLINE
	         const_iterator ():
	             container_const_reference<self_type> () {}
	         BOOST_UBLAS_INLINE
	         const_iterator (const self_type &v):
	             container_const_reference<self_type> (v) {}

	         // Arithmetic
	         BOOST_UBLAS_INLINE
	         const_iterator &operator ++ () {
	             BOOST_UBLAS_CHECK_FALSE (bad_index ());
	             return *this;
	         }
	         BOOST_UBLAS_INLINE
	         const_iterator &operator -- () {
	             BOOST_UBLAS_CHECK_FALSE (bad_index ());
	             return *this;
	         }

	         // Dereference
	         BOOST_UBLAS_INLINE
	         const_reference operator * () const {
	             BOOST_UBLAS_CHECK_FALSE (bad_index ());
	             return zero_;   // arbitary return value
	         }

	         // Index
	         BOOST_UBLAS_INLINE
	         size_type index () const {
	             BOOST_UBLAS_CHECK_FALSE (bad_index ());
	             return 0;   // arbitary return value
	         }

	         // Assignment
	         BOOST_UBLAS_INLINE
	         const_iterator &operator = (const const_iterator &it) {
	             container_const_reference<self_type>::assign (&it ());
	             return *this;
	         }

	         // Comparison
	         BOOST_UBLAS_INLINE
	         bool operator == (const const_iterator &it) const {
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             detail::ignore_unused_variable_warning(it);
	             return true;
	         }
	     };

	     typedef const_iterator iterator;

	     BOOST_UBLAS_INLINE
	     const_iterator begin () const {
	         return const_iterator (*this);
	     }
         BOOST_UBLAS_INLINE
         const_iterator cbegin () const {
             return begin ();
         }
	     BOOST_UBLAS_INLINE
	     const_iterator end () const {
	         return const_iterator (*this);
	     }
         BOOST_UBLAS_INLINE
         const_iterator cend () const {
             return end ();
         }

	     // Reverse iterator
	     typedef reverse_iterator_base<const_iterator> const_reverse_iterator;

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

	      // Serialization
	     template<class Archive>
	     void serialize(Archive & ar, const unsigned int /* file_version */){
	         serialization::collection_size_type s (size_);
	         ar & serialization::make_nvp("size",s);
	         if (Archive::is_loading::value) {
	             size_ = s;
	         }
	     }

	 private:
	     size_type size_;
	     typedef const value_type const_value_type;
	     static const_value_type zero_;
	 };

	 template<class T, class ALLOC>
	 typename zero_vector<T, ALLOC>::const_value_type zero_vector<T, ALLOC>::zero_ = T(/*zero*/);


	 // Unit vector class
	 /// \brief unit_vector represents a canonical unit vector
	 /// unit_vector represents a canonical unit vector. The \e k-th unit vector of dimension \f$n\f$ holds 0 for every value \f$u_i\f$ s.t. \f$i \neq k\f$ and 1 when \f$i=k\f$.
	 /// At construction, the value \e k is given after the dimension of the vector.
	 /// \tparam T is the type of elements in the vector. They must be 0 and 1 assignable in order for the vector to have its unit-vector semantic.
	 /// \tparam ALLOC a specific allocator can be specified if needed. Most of the time this parameter is omited.
	 template<class T, class ALLOC>
	 class unit_vector:
	     public vector_container<unit_vector<T, ALLOC> > {

	     typedef const T *const_pointer;
	     typedef unit_vector<T, ALLOC> self_type;
	 public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
	     using vector_container<self_type>::operator ();
#endif
         typedef typename boost::allocator_size_type<ALLOC>::type size_type;
         typedef typename boost::allocator_difference_type<ALLOC>::type difference_type;
	     typedef T value_type;
	     typedef const T &const_reference;
	     typedef T &reference;
	     typedef const vector_reference<const self_type> const_closure_type;
	     typedef vector_reference<self_type> closure_type;
	     typedef sparse_tag storage_category;

	     // Construction and destruction
	/// \brief Simple constructor with dimension and index 0
	    BOOST_UBLAS_INLINE
	    unit_vector ():
	        vector_container<self_type> (),
	        size_ (0), index_ (0) {}
	
	/// \brief Constructor of unit_vector
	/// \param size is the dimension of the vector
	/// \param index is the order of the vector
	    BOOST_UBLAS_INLINE
	    explicit unit_vector (size_type size, size_type index = 0):
	        vector_container<self_type> (),
	        size_ (size), index_ (index) {}
	
	/// \brief Copy-constructor
	    BOOST_UBLAS_INLINE
	    unit_vector (const unit_vector &v):
	        vector_container<self_type> (),
	        size_ (v.size_), index_ (v.index_) {}
	
	    // Accessors
	//----------
	
	/// \brief Return the size (dimension) of the vector
	    BOOST_UBLAS_INLINE
	    size_type size () const {
	        return size_;
	    }
	
	/// \brief Return the order of the unit vector
	    BOOST_UBLAS_INLINE
	    size_type index () const {
	        return index_;
	    }
	
	    // Resizing
	// --------
	
	/// \brief Resize the vector. The values are preserved by default (i.e. the index does not change)
	/// \param size is the new size of the vector
	    BOOST_UBLAS_INLINE
	    void resize (size_type size, bool /*preserve*/ = true) {
	        size_ = size;
	    }
	
	    // Element support
	// ---------------
	
	/// \brief Return a const pointer to the element of index i
	     BOOST_UBLAS_INLINE
	     const_pointer find_element (size_type i) const {
	         if (i == index_)
	             return & one_;
	         else
	             return & zero_;
	     }

	     // Element access
	     BOOST_UBLAS_INLINE
	     const_reference operator () (size_type i) const {
	         if (i == index_)
	             return one_;
	         else
	             return zero_;
	     }

	     BOOST_UBLAS_INLINE
	     const_reference operator [] (size_type i) const {
	         return (*this) (i);
	     }

	     // Assignment
	     BOOST_UBLAS_INLINE
	     unit_vector &operator = (const unit_vector &v) {
	         size_ = v.size_;
	         index_ = v.index_;
	         return *this;
	     }
	     BOOST_UBLAS_INLINE
	     unit_vector &assign_temporary (unit_vector &v) {
	         swap (v);
	         return *this;
	     }

	     // Swapping
	     BOOST_UBLAS_INLINE
	     void swap (unit_vector &v) {
	         if (this != &v) {
	             std::swap (size_, v.size_);
	             std::swap (index_, v.index_);
	         }
	     }
	     BOOST_UBLAS_INLINE
	     friend void swap (unit_vector &v1, unit_vector &v2) {
	         v1.swap (v2);
	     }

	     // Iterator types
	 private:
	     // Use bool to indicate begin (one_ as value)
	     typedef bool const_subiterator_type;
	 public:
	     class const_iterator;

	     // Element lookup
	     BOOST_UBLAS_INLINE
	     const_iterator find (size_type i) const {
	         return const_iterator (*this, i <= index_);
	     }

	     class const_iterator:
	         public container_const_reference<unit_vector>,
	         public bidirectional_iterator_base<sparse_bidirectional_iterator_tag,
	                                            const_iterator, value_type> {
	     public:
	         typedef typename unit_vector::difference_type difference_type;
	         typedef typename unit_vector::value_type value_type;
	         typedef typename unit_vector::const_reference reference;
	         typedef typename unit_vector::const_pointer pointer;

	         // Construction and destruction
	         BOOST_UBLAS_INLINE
	         const_iterator ():
	             container_const_reference<unit_vector> (), it_ () {}
	         BOOST_UBLAS_INLINE
	         const_iterator (const unit_vector &v, const const_subiterator_type &it):
	             container_const_reference<unit_vector> (v), it_ (it) {}

	         // Arithmetic
	         BOOST_UBLAS_INLINE
	         const_iterator &operator ++ () {
	             BOOST_UBLAS_CHECK (it_, bad_index ());
	             it_ = !it_;
	             return *this;
	         }
	         BOOST_UBLAS_INLINE
	         const_iterator &operator -- () {
	             BOOST_UBLAS_CHECK (!it_, bad_index ());
	             it_ = !it_;
	             return *this;
	         }

	         // Dereference
	         BOOST_UBLAS_INLINE
	         const_reference operator * () const {
	             BOOST_UBLAS_CHECK (it_, bad_index ());
	             return one_;
	         }

	         // Index
	         BOOST_UBLAS_INLINE
	         size_type index () const {
	             BOOST_UBLAS_CHECK (it_, bad_index ());
	             return (*this) ().index_;
	         }

	         // Assignment
	         BOOST_UBLAS_INLINE
	         const_iterator &operator = (const const_iterator &it) {
	             container_const_reference<unit_vector>::assign (&it ());
	             it_ = it.it_;
	             return *this;
	         }

	         // Comparison
	         BOOST_UBLAS_INLINE
	         bool operator == (const const_iterator &it) const {
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ == it.it_;
	         }

	     private:
	         const_subiterator_type it_;
	     };

	     typedef const_iterator iterator;

	     BOOST_UBLAS_INLINE
	     const_iterator begin () const {
	         return const_iterator (*this, true);
	     }
         BOOST_UBLAS_INLINE
         const_iterator cbegin () const {
             return begin ();
         }
	     BOOST_UBLAS_INLINE
	     const_iterator end () const {
	         return const_iterator (*this, false);
	     }
         BOOST_UBLAS_INLINE
         const_iterator cend () const {
             return end ();
         }

	     // Reverse iterator
	     typedef reverse_iterator_base<const_iterator> const_reverse_iterator;

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

	      // Serialization
	     template<class Archive>
	     void serialize(Archive & ar, const unsigned int /* file_version */){
	         serialization::collection_size_type s (size_);
	         ar & serialization::make_nvp("size",s);
	         if (Archive::is_loading::value) {
	             size_ = s;
	         }
	         ar & serialization::make_nvp("index", index_);
	     }

	 private:
	     size_type size_;
	     size_type index_;
	     typedef const value_type const_value_type;
	     static const_value_type zero_;
	     static const_value_type one_;
	 };

	 template<class T, class ALLOC>
	 typename unit_vector<T, ALLOC>::const_value_type unit_vector<T, ALLOC>::zero_ = T(/*zero*/);
	 template<class T, class ALLOC>
	 typename unit_vector<T, ALLOC>::const_value_type unit_vector<T, ALLOC>::one_ (1);  // ISSUE: need 'one'-traits here

	 /// \brief A scalar (i.e. unique value) vector of type \c T and a given \c size
	 /// A scalar (i.e. unique value) vector of type \c T and a given \c size. This is a virtual vector in the sense that no memory is allocated 
	 /// for storing the unique value more than once: it still acts like any other vector. However assigning a new value will change all the value at once.
	 /// vector into a normal vector. It must first be assigned to another normal vector by any suitable means. Its memory footprint is constant.
	 /// \tparam T type of the objects stored in the vector: it can be anything even if most of the time, scalar types will be used like \c double or \c int. Complex types can be used, or even classes like boost::interval.
	 template<class T, class ALLOC>
	 class scalar_vector:
	     public vector_container<scalar_vector<T, ALLOC> > {

	     typedef const T *const_pointer;
	     typedef scalar_vector<T, ALLOC> self_type;
	 public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
	     using vector_container<self_type>::operator ();
#endif
         typedef typename boost::allocator_size_type<ALLOC>::type size_type;
         typedef typename boost::allocator_difference_type<ALLOC>::type difference_type;
	     typedef T value_type;
	     typedef const T &const_reference;
	     typedef T &reference;
	     typedef const vector_reference<const self_type> const_closure_type;
	     typedef vector_reference<self_type> closure_type;
	     typedef dense_tag storage_category;

	     // Construction and destruction
	     BOOST_UBLAS_INLINE
	     scalar_vector ():
	         vector_container<self_type> (),
	         size_ (0), value_ () {}
	     BOOST_UBLAS_INLINE
	     explicit scalar_vector (size_type size, const value_type &value = value_type(1)):
	         vector_container<self_type> (),
	         size_ (size), value_ (value) {}
	     BOOST_UBLAS_INLINE
	     scalar_vector (const scalar_vector &v):
	         vector_container<self_type> (),
	         size_ (v.size_), value_ (v.value_) {}

	     // Accessors
	     BOOST_UBLAS_INLINE
	     size_type size () const {
	         return size_;
	     }

	     // Resizing
	     BOOST_UBLAS_INLINE
	     void resize (size_type size, bool /*preserve*/ = true) {
	         size_ = size;
	     }

	     // Element support
	     BOOST_UBLAS_INLINE
	     const_pointer find_element (size_type /*i*/) const {
	         return & value_;
	     }

	     // Element access
	     BOOST_UBLAS_INLINE
	     const_reference operator () (size_type /*i*/) const {
	         return value_;
	     }

	     BOOST_UBLAS_INLINE
	     const_reference operator [] (size_type /*i*/) const {
	         return value_;
	     }

	     // Assignment
	     BOOST_UBLAS_INLINE
	     scalar_vector &operator = (const scalar_vector &v) {
	         size_ = v.size_;
	         value_ = v.value_;
	         return *this;
	     }
	     BOOST_UBLAS_INLINE
	     scalar_vector &assign_temporary (scalar_vector &v) {
	         swap (v);
	         return *this;
	     }

	     // Swapping
	     BOOST_UBLAS_INLINE
	     void swap (scalar_vector &v) {
	         if (this != &v) {
	             std::swap (size_, v.size_);
	             std::swap (value_, v.value_);
	         }
	     }
	     BOOST_UBLAS_INLINE
	     friend void swap (scalar_vector &v1, scalar_vector &v2) {
	         v1.swap (v2);
	     }

	     // Iterator types
	 private:
	     // Use an index
	     typedef size_type const_subiterator_type;

	 public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
	     typedef indexed_const_iterator<self_type, dense_random_access_iterator_tag> iterator;
	     typedef indexed_const_iterator<self_type, dense_random_access_iterator_tag> const_iterator;
#else
	     class const_iterator;
#endif

	     // Element lookup
	     BOOST_UBLAS_INLINE
	     const_iterator find (size_type i) const {
	         return const_iterator (*this, i);
	     }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
	     class const_iterator:
	         public container_const_reference<scalar_vector>,
	         public random_access_iterator_base<dense_random_access_iterator_tag,
	                                            const_iterator, value_type> {
	     public:
	         typedef typename scalar_vector::difference_type difference_type;
	         typedef typename scalar_vector::value_type value_type;
	         typedef typename scalar_vector::const_reference reference;
	         typedef typename scalar_vector::const_pointer pointer;

	         // Construction and destruction
	         BOOST_UBLAS_INLINE
	         const_iterator ():
	             container_const_reference<scalar_vector> (), it_ () {}
	         BOOST_UBLAS_INLINE
	         const_iterator (const scalar_vector &v, const const_subiterator_type &it):
	             container_const_reference<scalar_vector> (v), it_ (it) {}

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
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ - it.it_;
	         }

	         // Dereference
	         BOOST_UBLAS_INLINE
	         const_reference operator * () const {
	             BOOST_UBLAS_CHECK (it_ < (*this) ().size (), bad_index ());
	             return (*this) () (index ());
	         }
	         BOOST_UBLAS_INLINE
	         const_reference operator [] (difference_type n) const {
	             return *(*this + n);
	         }

	         // Index
	         BOOST_UBLAS_INLINE
	         size_type index () const {
	             BOOST_UBLAS_CHECK (it_ < (*this) ().size (), bad_index ());
	             return it_;
	         }

	         // Assignment
	         BOOST_UBLAS_INLINE
	         const_iterator &operator = (const const_iterator &it) {
	             container_const_reference<scalar_vector>::assign (&it ());
	             it_ = it.it_;
	             return *this;
	         }

	         // Comparison
	         BOOST_UBLAS_INLINE
	         bool operator == (const const_iterator &it) const {
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ == it.it_;
	         }
	         BOOST_UBLAS_INLINE
	         bool operator < (const const_iterator &it) const {
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ < it.it_;
	         }

	     private:
	         const_subiterator_type it_;
	     };

	     typedef const_iterator iterator;
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
	         return find (size_);
	     }
         BOOST_UBLAS_INLINE
         const_iterator cend () const {
             return end ();
         }

	     // Reverse iterator
	     typedef reverse_iterator_base<const_iterator> const_reverse_iterator;

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

	      // Serialization
	     template<class Archive>
	     void serialize(Archive & ar, const unsigned int /* file_version */){
	         serialization::collection_size_type s (size_);
	         ar & serialization::make_nvp("size",s);
	         if (Archive::is_loading::value) {
	             size_ = s;
	         }
	         ar & serialization::make_nvp("value", value_);
	     }

	 private:
	     size_type size_;
	     value_type value_;
	 };

	 // ------------------------
	 // Array based vector class
	 // ------------------------

	 /// \brief A dense vector of values of type \c T with the given \c size. The data is stored as an ordinary C++ array \c T \c data_[M]
	 template<class T, std::size_t N>
	 class c_vector:
	     public vector_container<c_vector<T, N> > {

	     typedef c_vector<T, N> self_type;
	 public:
#ifdef BOOST_UBLAS_ENABLE_PROXY_SHORTCUTS
	     using vector_container<self_type>::operator ();
#endif
	     typedef std::size_t size_type;
	     typedef std::ptrdiff_t difference_type;
	     typedef T value_type;
	     typedef const T &const_reference;
	     typedef T &reference;
	     typedef value_type array_type[N];
	     typedef T *pointer;
	     typedef const T *const_pointer;
	     typedef const vector_reference<const self_type> const_closure_type;
	     typedef vector_reference<self_type> closure_type;
	     typedef self_type vector_temporary_type;
	     typedef dense_tag storage_category;

	     // Construction and destruction
	     BOOST_UBLAS_INLINE
	     c_vector ():
	         size_ (N) /* , data_ () */ {}
	     explicit BOOST_UBLAS_INLINE
	     c_vector (size_type size):
	         size_ (size) /* , data_ () */ {
	         if (size_ > N)
                 bad_size ().raise ();
	     }
	     BOOST_UBLAS_INLINE
	     c_vector (const c_vector &v):
	         size_ (v.size_) /* , data_ () */ {
	         if (size_ > N)
                 bad_size ().raise ();
	         assign(v);
	     }
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     c_vector (const vector_expression<AE> &ae):
	         size_ (ae ().size ()) /* , data_ () */ {
	         if (size_ > N)
                 bad_size ().raise ();
	         vector_assign<scalar_assign> (*this, ae);
	     }

	     // Accessors
	     BOOST_UBLAS_INLINE
	     size_type size () const {
	         return size_;
	     }
	     BOOST_UBLAS_INLINE
	     const_pointer data () const {
	         return data_;
	     }
	     BOOST_UBLAS_INLINE
	     pointer data () {
	         return data_;
	     }

	     // Resizing
	     BOOST_UBLAS_INLINE
         void resize (size_type size, bool /*preserve*/ = true) {
	         if (size > N)
                 bad_size ().raise ();
	         size_ = size;
	     }

	     // Element support
	     BOOST_UBLAS_INLINE
	     pointer find_element (size_type i) {
	         return const_cast<pointer> (const_cast<const self_type&>(*this).find_element (i));
	     }
	     BOOST_UBLAS_INLINE
	     const_pointer find_element (size_type i) const {
	         return & data_ [i];
	     }

	     // Element access
	     BOOST_UBLAS_INLINE
	     const_reference operator () (size_type i) const {
	         BOOST_UBLAS_CHECK (i < size_,  bad_index ());
	         return data_ [i];
	     }
	     BOOST_UBLAS_INLINE
	     reference operator () (size_type i) {
	         BOOST_UBLAS_CHECK (i < size_, bad_index ());
	         return data_ [i];
	     }

	     BOOST_UBLAS_INLINE
	     const_reference operator [] (size_type i) const {
	         return (*this) (i);
	     }
	     BOOST_UBLAS_INLINE
	     reference operator [] (size_type i) {
	         return (*this) (i);
	     }

	     // Element assignment
	     BOOST_UBLAS_INLINE
	     reference insert_element (size_type i, const_reference t) {
	         BOOST_UBLAS_CHECK (i < size_, bad_index ());
	         return (data_ [i] = t);
	     }
	     BOOST_UBLAS_INLINE
	     void erase_element (size_type i) {
	         BOOST_UBLAS_CHECK (i < size_, bad_index ());
	         data_ [i] = value_type/*zero*/();
	     }
	     
	     // Zeroing
	     BOOST_UBLAS_INLINE
	     void clear () {
	         std::fill (data_, data_ + size_, value_type/*zero*/());
	     }

	     // Assignment
#ifdef BOOST_UBLAS_MOVE_SEMANTICS

	     /*! @note "pass by value" the key idea to enable move semantics */
	     BOOST_UBLAS_INLINE
	     c_vector &operator = (c_vector v) {
	         assign_temporary(v);
	         return *this;
	     }
#else
	     BOOST_UBLAS_INLINE
	     c_vector &operator = (const c_vector &v) {
	         size_ = v.size_;
	         std::copy (v.data_, v.data_ + v.size_, data_);
	         return *this;
	     }
#endif
	     template<class C>          // Container assignment without temporary
	     BOOST_UBLAS_INLINE
	     c_vector &operator = (const vector_container<C> &v) {
	         resize (v ().size (), false);
	         assign (v);
	         return *this;
	     }
	     BOOST_UBLAS_INLINE
	     c_vector &assign_temporary (c_vector &v) {
	         swap (v);
	         return *this;
	     }
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     c_vector &operator = (const vector_expression<AE> &ae) {
	         self_type temporary (ae);
	         return assign_temporary (temporary);
	     }
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     c_vector &assign (const vector_expression<AE> &ae) {
	         vector_assign<scalar_assign> (*this, ae);
	         return *this;
	     }

	     // Computed assignment
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     c_vector &operator += (const vector_expression<AE> &ae) {
	         self_type temporary (*this + ae);
	         return assign_temporary (temporary);
	     }
	     template<class C>          // Container assignment without temporary
	     BOOST_UBLAS_INLINE
	     c_vector &operator += (const vector_container<C> &v) {
	         plus_assign (v);
	         return *this;
	     }
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     c_vector &plus_assign (const vector_expression<AE> &ae) {
	         vector_assign<scalar_plus_assign> ( *this, ae);
	         return *this;
	     }
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     c_vector &operator -= (const vector_expression<AE> &ae) {
	         self_type temporary (*this - ae);
	         return assign_temporary (temporary);
	     }
	     template<class C>          // Container assignment without temporary
	     BOOST_UBLAS_INLINE
	     c_vector &operator -= (const vector_container<C> &v) {
	         minus_assign (v);
	         return *this;
	     }
	     template<class AE>
	     BOOST_UBLAS_INLINE
	     c_vector &minus_assign (const vector_expression<AE> &ae) {
	         vector_assign<scalar_minus_assign> (*this, ae);
	         return *this;
	     }
	     template<class AT>
	     BOOST_UBLAS_INLINE
	     c_vector &operator *= (const AT &at) {
	         vector_assign_scalar<scalar_multiplies_assign> (*this, at);
	         return *this;
	     }
	     template<class AT>
	     BOOST_UBLAS_INLINE
	     c_vector &operator /= (const AT &at) {
	         vector_assign_scalar<scalar_divides_assign> (*this, at);
	         return *this;
	     }

	     // Swapping
	     BOOST_UBLAS_INLINE
	     void swap (c_vector &v) {
	         if (this != &v) {
                 BOOST_UBLAS_CHECK (size_ == v.size_, bad_size ());
	             std::swap (size_, v.size_);
	             std::swap_ranges (data_, data_ + size_, v.data_);
	         }
	     }
	     BOOST_UBLAS_INLINE
	     friend void swap (c_vector &v1, c_vector &v2) {
	         v1.swap (v2);
	     }

	     // Iterator types
	 private:
	     // Use pointers for iterator
	     typedef const_pointer const_subiterator_type;
	     typedef pointer subiterator_type;

	 public:
#ifdef BOOST_UBLAS_USE_INDEXED_ITERATOR
	     typedef indexed_iterator<self_type, dense_random_access_iterator_tag> iterator;
	     typedef indexed_const_iterator<self_type, dense_random_access_iterator_tag> const_iterator;
#else
	     class const_iterator;
	     class iterator;
#endif

	     // Element lookup
	     BOOST_UBLAS_INLINE
	     const_iterator find (size_type i) const {
#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
	         return const_iterator (*this, &data_ [i]);
#else
	         return const_iterator (*this, i);
#endif
	     }
	     BOOST_UBLAS_INLINE
	     iterator find (size_type i) {
#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
	         return iterator (*this, &data_ [i]);
#else
	         return iterator (*this, i);
#endif
	     }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
	     class const_iterator:
	         public container_const_reference<c_vector>,
	         public random_access_iterator_base<dense_random_access_iterator_tag,
	                                            const_iterator, value_type> {
	     public:
	         typedef typename c_vector::difference_type difference_type;
	         typedef typename c_vector::value_type value_type;
	         typedef typename c_vector::const_reference reference;
	         typedef typename c_vector::const_pointer pointer;

	         // Construction and destruction
	         BOOST_UBLAS_INLINE
	         const_iterator ():
	             container_const_reference<self_type> (), it_ () {}
	         BOOST_UBLAS_INLINE
	         const_iterator (const self_type &v, const const_subiterator_type &it):
	             container_const_reference<self_type> (v), it_ (it) {}
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
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ - it.it_;
	         }

	         // Dereference
	         BOOST_UBLAS_INLINE
	         const_reference operator * () const {
	             BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_, bad_index ());
	             return *it_;
	         }
	         BOOST_UBLAS_INLINE
	         const_reference operator [] (difference_type n) const {
	             return *(it_ + n);
	         }

	         // Index
	         BOOST_UBLAS_INLINE
	         size_type index () const {
	             BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_, bad_index ());
	             const self_type &v = (*this) ();
	             return it_ - v.begin ().it_;
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
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ == it.it_;
	         }
	         BOOST_UBLAS_INLINE
	         bool operator < (const const_iterator &it) const {
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ < it.it_;
	         }

	     private:
	         const_subiterator_type it_;

	         friend class iterator;
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
	         return find (size_);
	     }
         BOOST_UBLAS_INLINE
         const_iterator cend () const {
             return end ();
         }

#ifndef BOOST_UBLAS_USE_INDEXED_ITERATOR
	     class iterator:
	         public container_reference<c_vector>,
	         public random_access_iterator_base<dense_random_access_iterator_tag,
	                                            iterator, value_type> {
	     public:
	         typedef typename c_vector::difference_type difference_type;
	         typedef typename c_vector::value_type value_type;
	         typedef typename c_vector::reference reference;
	         typedef typename c_vector::pointer pointer;

	         // Construction and destruction
	         BOOST_UBLAS_INLINE
	         iterator ():
	             container_reference<self_type> (), it_ () {}
	         BOOST_UBLAS_INLINE
	         iterator (self_type &v, const subiterator_type &it):
	             container_reference<self_type> (v), it_ (it) {}

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
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ - it.it_;
	         }

	         // Dereference
	         BOOST_UBLAS_INLINE
	         reference operator * () const {
	             BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_, bad_index ());
	             return *it_;
	         }
	         BOOST_UBLAS_INLINE
	         reference operator [] (difference_type n) const {
	             return *(it_ + n);
	         }

	         // Index
	         BOOST_UBLAS_INLINE
	         size_type index () const {
	             BOOST_UBLAS_CHECK (it_ >= (*this) ().begin ().it_ && it_ < (*this) ().end ().it_, bad_index ());
	             // EDG won't allow const self_type it doesn't allow friend access to it_
	             self_type &v = (*this) ();
	             return it_ - v.begin ().it_;
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
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
	             return it_ == it.it_;
	         }
	         BOOST_UBLAS_INLINE
	         bool operator < (const iterator &it) const {
	             BOOST_UBLAS_CHECK (&(*this) () == &it (), external_logic ());
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
	         return find (size_);
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

	     // Serialization
	     template<class Archive>
	     void serialize(Archive & ar, const unsigned int /* file_version */){
	         serialization::collection_size_type s (size_);
	         ar & serialization::make_nvp("size",s);
	         
	         // copy the value back if loading
	         if (Archive::is_loading::value) {
	           if (s > N) bad_size("too large size in bounded_vector::load()\n").raise();
	           size_ = s;
	         }
	         // ISSUE: this writes the full array
	         ar & serialization::make_nvp("data",data_);
	     }

	 private:
	     size_type size_;
	     array_type data_;
	 };

}}}

#endif
