//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_CONTAINER_VALARRAY_HPP
#define BOOST_COMPUTE_CONTAINER_VALARRAY_HPP

#include <cstddef>
#include <valarray>

#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

#include <boost/compute/buffer.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/algorithm/fill.hpp>
#include <boost/compute/algorithm/max_element.hpp>
#include <boost/compute/algorithm/min_element.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/algorithm/accumulate.hpp>
#include <boost/compute/detail/buffer_value.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/functional/bind.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>
#include <boost/compute/type_traits.hpp>

namespace boost {
namespace compute {

template<class T>
class valarray
{
public:
    explicit valarray(const context &context = system::default_context())
        : m_buffer(context, 0)
    {
    }

    explicit valarray(size_t size,
                      const context &context = system::default_context())
        : m_buffer(context, size * sizeof(T))
    {
    }

    valarray(const T &value,
             size_t size,
             const context &context = system::default_context())
        : m_buffer(context, size * sizeof(T))
    {
        fill(begin(), end(), value);
    }

    valarray(const T *values,
             size_t size,
             const context &context = system::default_context())
        : m_buffer(context, size * sizeof(T))
    {
        copy(values, values + size, begin());
    }

    valarray(const valarray<T> &other)
        : m_buffer(other.m_buffer.get_context(), other.size() * sizeof(T))
    {
        copy(other.begin(), other.end(), begin());
    }

    valarray(const std::valarray<T> &valarray,
             const context &context = system::default_context())
        : m_buffer(context, valarray.size() * sizeof(T))
    {
        copy(&valarray[0], &valarray[valarray.size()], begin());
    }

    valarray<T>& operator=(const valarray<T> &other)
    {
        if(this != &other){
            // change to other's OpenCL context
            m_buffer = buffer(other.m_buffer.get_context(), other.size() * sizeof(T));
            copy(other.begin(), other.end(), begin());
        }

        return *this;
    }

    valarray<T>& operator=(const std::valarray<T> &valarray)
    {
        m_buffer = buffer(m_buffer.get_context(), valarray.size() * sizeof(T));
        copy(&valarray[0], &valarray[valarray.size()], begin());

        return *this;
    }

    valarray<T>& operator*=(const T&);

    valarray<T>& operator/=(const T&);

    valarray<T>& operator%=(const T& val);

    valarray<T> operator+() const
    {
        //  This operator can be used with any type.
        valarray<T> result(size());
        copy(begin(), end(), result.begin());
        return result;
    }

    valarray<T> operator-() const
    {
        BOOST_STATIC_ASSERT_MSG(
            is_fundamental<T>::value,
            "This operator can be used with all OpenCL built-in scalar"
            " and vector types"
        );
        valarray<T> result(size());
        BOOST_COMPUTE_FUNCTION(T, unary_minus, (T x),
        {
            return -x;
        });
        transform(begin(), end(), result.begin(), unary_minus);
        return result;
    }

    valarray<T> operator~() const
    {
        BOOST_STATIC_ASSERT_MSG(
            is_fundamental<T>::value &&
                !is_floating_point<typename scalar_type<T>::type>::value,
            "This operator can be used with all OpenCL built-in scalar"
            " and vector types except the built-in scalar and vector float types"
        );
        valarray<T> result(size());
        BOOST_COMPUTE_FUNCTION(T, bitwise_not, (T x),
        {
            return ~x;
        });
        transform(begin(), end(), result.begin(), bitwise_not);
        return result;
    }

    /// In OpenCL there cannot be memory buffer with bool type, for
    /// this reason return type is valarray<char> instead of valarray<bool>.
    /// 1 means true, 0 means false.
    valarray<char> operator!() const
    {
        BOOST_STATIC_ASSERT_MSG(
            is_fundamental<T>::value,
            "This operator can be used with all OpenCL built-in scalar"
            " and vector types"
        );
        valarray<char> result(size());
        BOOST_COMPUTE_FUNCTION(char, logical_not, (T x),
        {
            return !x;
        });
        transform(begin(), end(), &result[0], logical_not);
        return result;
    }

    valarray<T>& operator+=(const T&);

    valarray<T>& operator-=(const T&);

    valarray<T>& operator^=(const T&);

    valarray<T>& operator&=(const T&);

    valarray<T>& operator|=(const T&);

    valarray<T>& operator<<=(const T&);

    valarray<T>& operator>>=(const T&);

    valarray<T>& operator*=(const valarray<T>&);

    valarray<T>& operator/=(const valarray<T>&);

    valarray<T>& operator%=(const valarray<T>&);

    valarray<T>& operator+=(const valarray<T>&);

    valarray<T>& operator-=(const valarray<T>&);

    valarray<T>& operator^=(const valarray<T>&);

    valarray<T>& operator&=(const valarray<T>&);

    valarray<T>& operator|=(const valarray<T>&);

    valarray<T>& operator<<=(const valarray<T>&);

    valarray<T>& operator>>=(const valarray<T>&);

    ~valarray()
    {

    }

    size_t size() const
    {
        return m_buffer.size() / sizeof(T);
    }

    void resize(size_t size, T value = T())
    {
        m_buffer = buffer(m_buffer.get_context(), size * sizeof(T));
        fill(begin(), end(), value);
    }

    detail::buffer_value<T> operator[](size_t index)
    {
        return *(begin() + static_cast<ptrdiff_t>(index));
    }

    const detail::buffer_value<T> operator[](size_t index) const
    {
        return *(begin() + static_cast<ptrdiff_t>(index));
    }

    T (min)() const
    {
        return *(boost::compute::min_element(begin(), end()));
    }

    T (max)() const
    {
        return *(boost::compute::max_element(begin(), end()));
    }

    T sum() const
    {
        return boost::compute::accumulate(begin(), end(), T(0));
    }

    template<class UnaryFunction>
    valarray<T> apply(UnaryFunction function) const
    {
        valarray<T> result(size());
        transform(begin(), end(), result.begin(), function);
        return result;
    }

    const buffer& get_buffer() const
    {
        return m_buffer;
    }


private:
    buffer_iterator<T> begin() const
    {
        return buffer_iterator<T>(m_buffer, 0);
    }

    buffer_iterator<T> end() const
    {
        return buffer_iterator<T>(m_buffer, size());
    }

private:
    buffer m_buffer;
};

/// \internal_
#define BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT(op, op_name, assert) \
    template<class T> \
    inline valarray<T>& \
    valarray<T>::operator op##=(const T& val) \
    { \
        assert \
        transform(begin(), end(), begin(), \
            ::boost::compute::bind(op_name<T>(), placeholders::_1, val)); \
        return *this; \
    } \
    \
    template<class T> \
    inline valarray<T>& \
    valarray<T>::operator op##=(const valarray<T> &rhs) \
    { \
        assert \
        transform(begin(), end(), rhs.begin(), begin(), op_name<T>()); \
        return *this; \
    }

/// \internal_
#define BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_ANY(op, op_name) \
    BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT(op, op_name, \
        BOOST_STATIC_ASSERT_MSG( \
            is_fundamental<T>::value, \
            "This operator can be used with all OpenCL built-in scalar" \
            " and vector types" \
        ); \
    )

/// \internal_
/// For some operators class T can't be floating point type.
/// See OpenCL specification, operators chapter.
#define BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_NO_FP(op, op_name) \
    BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT(op, op_name, \
        BOOST_STATIC_ASSERT_MSG( \
            is_fundamental<T>::value && \
                !is_floating_point<typename scalar_type<T>::type>::value, \
            "This operator can be used with all OpenCL built-in scalar" \
            " and vector types except the built-in scalar and vector float types" \
        ); \
    )

// defining operators
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_ANY(+, plus)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_ANY(-, minus)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_ANY(*, multiplies)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_ANY(/, divides)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_NO_FP(^, bit_xor)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_NO_FP(&, bit_and)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_NO_FP(|, bit_or)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_NO_FP(<<, shift_left)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_NO_FP(>>, shift_right)

// The remainder (%) operates on
// integer scalar and integer vector data types only.
// See OpenCL specification.
BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT(%, modulus,
    BOOST_STATIC_ASSERT_MSG(
        is_integral<typename scalar_type<T>::type>::value,
        "This operator can be used only with OpenCL built-in integer types"
    );
)

#undef BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_ANY
#undef BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT_NO_FP

#undef BOOST_COMPUTE_DEFINE_VALARRAY_COMPOUND_ASSIGNMENT

/// \internal_
/// Macro for defining binary operators for valarray
#define BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR(op, op_name, assert) \
    template<class T> \
    valarray<T> operator op (const valarray<T>& lhs, const valarray<T>& rhs) \
    { \
        assert \
        valarray<T> result(lhs.size()); \
        transform(buffer_iterator<T>(lhs.get_buffer(), 0), \
                  buffer_iterator<T>(lhs.get_buffer(), lhs.size()), \
                  buffer_iterator<T>(rhs.get_buffer(), 0), \
                  buffer_iterator<T>(result.get_buffer(), 0), \
                  op_name<T>()); \
        return result; \
    } \
    \
    template<class T> \
    valarray<T> operator op (const T& val, const valarray<T>& rhs) \
    { \
        assert \
        valarray<T> result(rhs.size()); \
        transform(buffer_iterator<T>(rhs.get_buffer(), 0), \
                  buffer_iterator<T>(rhs.get_buffer(), rhs.size()), \
                  buffer_iterator<T>(result.get_buffer(), 0), \
                  ::boost::compute::bind(op_name<T>(), val, placeholders::_1)); \
        return result; \
    } \
    \
    template<class T> \
    valarray<T> operator op (const valarray<T>& lhs, const T& val) \
    { \
        assert \
        valarray<T> result(lhs.size()); \
        transform(buffer_iterator<T>(lhs.get_buffer(), 0), \
                  buffer_iterator<T>(lhs.get_buffer(), lhs.size()), \
                  buffer_iterator<T>(result.get_buffer(), 0), \
                  ::boost::compute::bind(op_name<T>(), placeholders::_1, val)); \
        return result; \
    }

/// \internal_
#define BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_ANY(op, op_name) \
    BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR(op, op_name, \
        BOOST_STATIC_ASSERT_MSG( \
            is_fundamental<T>::value, \
            "This operator can be used with all OpenCL built-in scalar" \
            " and vector types" \
        ); \
    )

/// \internal_
/// For some operators class T can't be floating point type.
/// See OpenCL specification, operators chapter.
#define BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_NO_FP(op, op_name) \
    BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR(op, op_name, \
        BOOST_STATIC_ASSERT_MSG( \
            is_fundamental<T>::value && \
                !is_floating_point<typename scalar_type<T>::type>::value, \
            "This operator can be used with all OpenCL built-in scalar" \
            " and vector types except the built-in scalar and vector float types" \
        ); \
    )

// defining binary operators for valarray
BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_ANY(+, plus)
BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_ANY(-, minus)
BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_ANY(*, multiplies)
BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_ANY(/, divides)
BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_NO_FP(^, bit_xor)
BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_NO_FP(&, bit_and)
BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_NO_FP(|, bit_or)
BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_NO_FP(<<, shift_left)
BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_NO_FP(>>, shift_right)

#undef BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_ANY
#undef BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR_NO_FP

#undef BOOST_COMPUTE_DEFINE_VALARRAY_BINARY_OPERATOR

/// \internal_
/// Macro for defining valarray comparison operators.
/// For return type valarray<char> is used instead of valarray<bool> because
/// in OpenCL there cannot be memory buffer with bool type.
///
/// Note it's also used for defining binary logical operators (==, &&)
#define BOOST_COMPUTE_DEFINE_VALARRAY_COMPARISON_OPERATOR(op, op_name) \
    template<class T> \
    valarray<char> operator op (const valarray<T>& lhs, const valarray<T>& rhs) \
    { \
        BOOST_STATIC_ASSERT_MSG( \
            is_fundamental<T>::value, \
            "This operator can be used with all OpenCL built-in scalar" \
            " and vector types" \
        ); \
        valarray<char> result(lhs.size()); \
        transform(buffer_iterator<T>(lhs.get_buffer(), 0), \
                  buffer_iterator<T>(lhs.get_buffer(), lhs.size()), \
                  buffer_iterator<T>(rhs.get_buffer(), 0), \
                  buffer_iterator<char>(result.get_buffer(), 0), \
                  op_name<T>()); \
        return result; \
    } \
    \
    template<class T> \
    valarray<char> operator op (const T& val, const valarray<T>& rhs) \
    { \
        BOOST_STATIC_ASSERT_MSG( \
            is_fundamental<T>::value, \
            "This operator can be used with all OpenCL built-in scalar" \
            " and vector types" \
        ); \
        valarray<char> result(rhs.size()); \
        transform(buffer_iterator<T>(rhs.get_buffer(), 0), \
                  buffer_iterator<T>(rhs.get_buffer(), rhs.size()), \
                  buffer_iterator<char>(result.get_buffer(), 0), \
                  ::boost::compute::bind(op_name<T>(), val, placeholders::_1)); \
        return result; \
    } \
    \
    template<class T> \
    valarray<char> operator op (const valarray<T>& lhs, const T& val) \
    { \
        BOOST_STATIC_ASSERT_MSG( \
            is_fundamental<T>::value, \
            "This operator can be used with all OpenCL built-in scalar" \
            " and vector types" \
        ); \
        valarray<char> result(lhs.size()); \
        transform(buffer_iterator<T>(lhs.get_buffer(), 0), \
                  buffer_iterator<T>(lhs.get_buffer(), lhs.size()), \
                  buffer_iterator<char>(result.get_buffer(), 0), \
                  ::boost::compute::bind(op_name<T>(), placeholders::_1, val)); \
        return result; \
    }

BOOST_COMPUTE_DEFINE_VALARRAY_COMPARISON_OPERATOR(==, equal_to)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPARISON_OPERATOR(!=, not_equal_to)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPARISON_OPERATOR(>, greater)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPARISON_OPERATOR(<, less)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPARISON_OPERATOR(>=, greater_equal)
BOOST_COMPUTE_DEFINE_VALARRAY_COMPARISON_OPERATOR(<=, less_equal)

/// \internal_
/// Macro for defining binary logical operators for valarray.
///
/// For return type valarray<char> is used instead of valarray<bool> because
/// in OpenCL there cannot be memory buffer with bool type.
/// 1 means true, 0 means false.
#define BOOST_COMPUTE_DEFINE_VALARRAY_LOGICAL_OPERATOR(op, op_name) \
    BOOST_COMPUTE_DEFINE_VALARRAY_COMPARISON_OPERATOR(op, op_name)

BOOST_COMPUTE_DEFINE_VALARRAY_LOGICAL_OPERATOR(&&, logical_and)
BOOST_COMPUTE_DEFINE_VALARRAY_LOGICAL_OPERATOR(||, logical_or)

#undef BOOST_COMPUTE_DEFINE_VALARRAY_LOGICAL_OPERATOR

#undef BOOST_COMPUTE_DEFINE_VALARRAY_COMPARISON_OPERATOR

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_CONTAINER_VALARRAY_HPP
