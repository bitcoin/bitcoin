//  Copyright (c) 2012 Oswin Krause
//  Copyright (c) 2013 Joaquim Duran
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_UBLAS_MATRIX_VECTOR_HPP
#define BOOST_UBLAS_MATRIX_VECTOR_HPP

#include <boost/numeric/ublas/matrix_proxy.hpp> //for matrix_row, matrix_column and matrix_expression
#include <boost/numeric/ublas/vector.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace numeric { namespace ublas {

namespace detail{

/** \brief Iterator used in the represention of a matrix as a vector of rows or columns
 *
 * Iterator used in the represention of a matrix as a vector of rows/columns. It refers
 * to the i-th element of the matrix, a column or a row depending of Reference type.
 *
 * The type of Reference should provide a constructor Reference(matrix, i)
 *
 * This iterator is invalidated when the underlying matrix is resized.
 *
 * \tparameter Matrix type of matrix that is represented as a vector of row/column
 * \tparameter Reference Matrix row or matrix column type.
 */
template<class Matrix, class Reference>
class matrix_vector_iterator: public boost::iterator_facade<
    matrix_vector_iterator<Matrix,Reference>,
    typename vector_temporary_traits<Reference>::type,
    boost::random_access_traversal_tag,
    Reference
>{
public:
    matrix_vector_iterator(){}

    ///\brief constructs a matrix_vector_iterator as pointing to the i-th proxy
    BOOST_UBLAS_INLINE
    matrix_vector_iterator(Matrix& matrix, std::size_t position)
    : matrix_(&matrix),position_(position) {}

    template<class M, class R>
    BOOST_UBLAS_INLINE
    matrix_vector_iterator(matrix_vector_iterator<M,R> const& other)
    : matrix_(other.matrix_),position_(other.position_) {}

private:
    friend class boost::iterator_core_access;
    template <class M,class R> friend class matrix_vector_iterator;

    BOOST_UBLAS_INLINE
    void increment() {
        ++position_;
    }

    BOOST_UBLAS_INLINE
    void decrement() {
        --position_;
    }

    BOOST_UBLAS_INLINE
    void advance(std::ptrdiff_t n){
        position_ += n;
    }

    template<class M,class R>
    BOOST_UBLAS_INLINE
    std::ptrdiff_t distance_to(matrix_vector_iterator<M,R> const& other) const{
        BOOST_UBLAS_CHECK (matrix_ == other.matrix_, external_logic ());
        return (std::ptrdiff_t)other.position_ - (std::ptrdiff_t)position_;
    }

    template<class M,class R>
    BOOST_UBLAS_INLINE
    bool equal(matrix_vector_iterator<M,R> const& other) const{
        BOOST_UBLAS_CHECK (matrix_ == other.matrix_, external_logic ());
        return (position_ == other.position_);
    }

    BOOST_UBLAS_INLINE
    Reference dereference() const {
        return Reference(*matrix_,position_);
    }

    Matrix* matrix_;//no matrix_closure here to ensure easy usage
    std::size_t position_;
};

}

/** \brief Represents a \c Matrix as a vector of rows.
 *
 * Implements an interface to Matrix that the underlaying matrix is represented as a
 * vector of rows.
 *
 * The vector could be resized which causes the resize of the number of rows of
 * the underlaying matrix.
 */
template<class Matrix>
class matrix_row_vector {
public:
    typedef ublas::matrix_row<Matrix> value_type;
    typedef ublas::matrix_row<Matrix> reference;
    typedef ublas::matrix_row<Matrix const> const_reference;

    typedef ublas::detail::matrix_vector_iterator<Matrix, ublas::matrix_row<Matrix> > iterator;
    typedef ublas::detail::matrix_vector_iterator<Matrix const, ublas::matrix_row<Matrix const> const> const_iterator;
    typedef boost::reverse_iterator<iterator> reverse_iterator;
    typedef boost::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef typename boost::iterator_difference<iterator>::type difference_type;
    typedef typename Matrix::size_type size_type;

    BOOST_UBLAS_INLINE
    explicit matrix_row_vector(Matrix& matrix) :
        matrix_(&matrix) {
    }

    BOOST_UBLAS_INLINE
    iterator begin(){
        return iterator(*matrix_, 0);
    }

    BOOST_UBLAS_INLINE
    const_iterator begin() const {
        return const_iterator(*matrix_, 0);
    }

    BOOST_UBLAS_INLINE
    const_iterator cbegin() const {
        return begin();
    }

    BOOST_UBLAS_INLINE
    iterator end() {
        return iterator(*matrix_, matrix_->size1());
    }

    BOOST_UBLAS_INLINE
    const_iterator end() const {
        return const_iterator(*matrix_, matrix_->size1());
    }

    BOOST_UBLAS_INLINE
    const_iterator cend() const {
        return end();
    }

    BOOST_UBLAS_INLINE
    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    BOOST_UBLAS_INLINE
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    BOOST_UBLAS_INLINE
    const_reverse_iterator crbegin() const {
        return rbegin();
    }  

    BOOST_UBLAS_INLINE
    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    BOOST_UBLAS_INLINE
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    BOOST_UBLAS_INLINE
    const_reverse_iterator crend() const {
        return end();
    }

    BOOST_UBLAS_INLINE
    value_type operator()(size_type index) {
        return value_type(*matrix_, index);
    }

    BOOST_UBLAS_INLINE
    value_type operator()(size_type index) const {
        return value_type(*matrix_, index);
    }

    BOOST_UBLAS_INLINE
    reference operator[](size_type index){
        return (*this) (index);
    }

    BOOST_UBLAS_INLINE
    const_reference operator[](size_type index) const {
        return (*this) (index);
    }

    BOOST_UBLAS_INLINE
    size_type size() const {
        return matrix_->size1();
    }

    BOOST_UBLAS_INLINE
    void resize(size_type size, bool preserve = true) {
        matrix_->resize(size, matrix_->size2(), preserve);
    }

private:
    Matrix* matrix_;
};


/** \brief Convenience function to create \c matrix_row_vector.
 *
 * Function to create \c matrix_row_vector objects.
 * \param matrix the \c matrix_expression that generates the matrix that \c matrix_row_vector is referring.
 * \return Created \c matrix_row_vector object.
 *
 * \tparam Matrix the type of matrix that \c matrix_row_vector is referring.
 */
template<class Matrix>
BOOST_UBLAS_INLINE
matrix_row_vector<Matrix> make_row_vector(matrix_expression<Matrix>& matrix){
    return matrix_row_vector<Matrix>(matrix());
}


/** \brief Convenience function to create \c matrix_row_vector.
 *
 * Function to create \c matrix_row_vector objects.
 * \param matrix the \c matrix_expression that generates the matrix that \c matrix_row_vector is referring.
 * \return Created \c matrix_row_vector object.
 *
 * \tparam Matrix the type of matrix that \c matrix_row_vector is referring.
 */
template<class Matrix>
BOOST_UBLAS_INLINE
matrix_row_vector<Matrix const> make_row_vector(matrix_expression<Matrix> const& matrix){
    return matrix_row_vector<Matrix const>(matrix());
}


/** \brief Represents a \c Matrix as a vector of columns.
 *
 * Implements an interface to Matrix that the underlaying matrix is represented as a
 * vector of columns.
 *
 * The vector could be resized which causes the resize of the number of columns of
 * the underlaying matrix.
 */
template<class Matrix>
class matrix_column_vector {
public:
    typedef ublas::matrix_column<Matrix> value_type;
    typedef ublas::matrix_column<Matrix> reference;
    typedef const ublas::matrix_column<Matrix const> const_reference;

    typedef ublas::detail::matrix_vector_iterator<Matrix, ublas::matrix_column<Matrix> > iterator;
    typedef ublas::detail::matrix_vector_iterator<Matrix const, ublas::matrix_column<Matrix const> const > const_iterator;
    typedef boost::reverse_iterator<iterator> reverse_iterator;
    typedef boost::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef typename boost::iterator_difference<iterator>::type difference_type;
    typedef typename Matrix::size_type size_type;

    BOOST_UBLAS_INLINE
    explicit matrix_column_vector(Matrix& matrix) :
        matrix_(&matrix){
    }

    BOOST_UBLAS_INLINE
    iterator begin() {
        return iterator(*matrix_, 0);
    }

    BOOST_UBLAS_INLINE
    const_iterator begin() const {
        return const_iterator(*matrix_, 0);
    }

    BOOST_UBLAS_INLINE
    const_iterator cbegin() const {
        return begin();
    }

    BOOST_UBLAS_INLINE
    iterator end() {
        return iterator(*matrix_, matrix_->size2());
    }

    BOOST_UBLAS_INLINE
    const_iterator end() const {
        return const_iterator(*matrix_, matrix_->size2());
    }

    BOOST_UBLAS_INLINE
    const_iterator cend() const {
        return end();
    }

    BOOST_UBLAS_INLINE
    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    BOOST_UBLAS_INLINE
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    BOOST_UBLAS_INLINE
    const_reverse_iterator crbegin() const {
        return rbegin();
    }

    BOOST_UBLAS_INLINE
    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    BOOST_UBLAS_INLINE
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    BOOST_UBLAS_INLINE
    const_reverse_iterator crend() const {
        return rend();
    }

    BOOST_UBLAS_INLINE
    value_type operator()(size_type index) {
        return value_type(*matrix_, index);
    }

    BOOST_UBLAS_INLINE
    value_type operator()(size_type index) const {
        return value_type(*matrix_, index);
    }

    BOOST_UBLAS_INLINE
    reference operator[](size_type index) {
        return (*this) (index);
    }

    BOOST_UBLAS_INLINE
    const_reference operator[](size_type index) const {
        return (*this) (index);
    }

    BOOST_UBLAS_INLINE
    size_type size() const {
        return matrix_->size2();
    }

    BOOST_UBLAS_INLINE
    void resize(size_type size, bool preserve = true) {
        matrix_->resize(matrix_->size1(), size, preserve);
    }

private:
    Matrix* matrix_;
};


/** \brief Convenience function to create \c matrix_column_vector.
 *
 * Function to create \c matrix_column_vector objects.
 * \param matrix the \c matrix_expression that generates the matrix that \c matrix_column_vector is referring.
 * \return Created \c matrix_column_vector object.
 *
 * \tparam Matrix the type of matrix that \c matrix_column_vector is referring.
 */
template<class Matrix>
BOOST_UBLAS_INLINE
matrix_column_vector<Matrix> make_column_vector(matrix_expression<Matrix>& matrix){
    return matrix_column_vector<Matrix>(matrix());
}


/** \brief Convenience function to create \c matrix_column_vector.
 *
 * Function to create \c matrix_column_vector objects.
 * \param matrix the \c matrix_expression that generates the matrix that \c matrix_column_vector is referring.
 * \return Created \c matrix_column_vector object.
 *
 * \tparam Matrix the type of matrix that \c matrix_column_vector is referring.
 */
template<class Matrix>
BOOST_UBLAS_INLINE
matrix_column_vector<Matrix const> make_column_vector(matrix_expression<Matrix> const& matrix){
    return matrix_column_vector<Matrix const>(matrix());
}

}}}

#endif
