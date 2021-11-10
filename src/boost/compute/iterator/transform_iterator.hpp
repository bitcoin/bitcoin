//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ITERATOR_TRANSFORM_ITERATOR_HPP
#define BOOST_COMPUTE_ITERATOR_TRANSFORM_ITERATOR_HPP

#include <cstddef>
#include <iterator>

#include <boost/config.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

#include <boost/compute/functional.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/is_buffer_iterator.hpp>
#include <boost/compute/detail/read_write_single_value.hpp>
#include <boost/compute/iterator/detail/get_base_iterator_buffer.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>
#include <boost/compute/type_traits/result_of.hpp>

namespace boost {
namespace compute {

// forward declaration for transform_iterator
template<class InputIterator, class UnaryFunction>
class transform_iterator;

namespace detail {

// meta-function returning the value_type for a transform_iterator
template<class InputIterator, class UnaryFunction>
struct make_transform_iterator_value_type
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    typedef typename boost::compute::result_of<UnaryFunction(value_type)>::type type;
};

// helper class which defines the iterator_adaptor super-class
// type for transform_iterator
template<class InputIterator, class UnaryFunction>
class transform_iterator_base
{
public:
    typedef ::boost::iterator_adaptor<
        ::boost::compute::transform_iterator<InputIterator, UnaryFunction>,
        InputIterator,
        typename make_transform_iterator_value_type<InputIterator, UnaryFunction>::type,
        typename std::iterator_traits<InputIterator>::iterator_category,
        typename make_transform_iterator_value_type<InputIterator, UnaryFunction>::type
    > type;
};

template<class InputIterator, class UnaryFunction, class IndexExpr>
struct transform_iterator_index_expr
{
    typedef typename
        make_transform_iterator_value_type<
            InputIterator,
            UnaryFunction
        >::type result_type;

    transform_iterator_index_expr(const InputIterator &input_iter,
                                  const UnaryFunction &transform_expr,
                                  const IndexExpr &index_expr)
        : m_input_iter(input_iter),
          m_transform_expr(transform_expr),
          m_index_expr(index_expr)
    {
    }

    const InputIterator m_input_iter;
    const UnaryFunction m_transform_expr;
    const IndexExpr m_index_expr;
};

template<class InputIterator, class UnaryFunction, class IndexExpr>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const transform_iterator_index_expr<InputIterator,
                                                                   UnaryFunction,
                                                                   IndexExpr> &expr)
{
    return kernel << expr.m_transform_expr(expr.m_input_iter[expr.m_index_expr]);
}

} // end detail namespace

/// \class transform_iterator
/// \brief A transform iterator adaptor.
///
/// The transform_iterator adaptor applies a unary function to each element
/// produced from the underlying iterator when dereferenced.
///
/// For example, to copy from an input range to an output range while taking
/// the absolute value of each element:
///
/// \snippet test/test_transform_iterator.cpp copy_abs
///
/// \see buffer_iterator, make_transform_iterator()
template<class InputIterator, class UnaryFunction>
class transform_iterator :
    public detail::transform_iterator_base<InputIterator, UnaryFunction>::type
{
public:
    typedef typename
        detail::transform_iterator_base<InputIterator,
                                        UnaryFunction>::type super_type;
    typedef typename super_type::value_type value_type;
    typedef typename super_type::reference reference;
    typedef typename super_type::base_type base_type;
    typedef typename super_type::difference_type difference_type;
    typedef UnaryFunction unary_function;

    transform_iterator(InputIterator iterator, UnaryFunction transform)
        : super_type(iterator),
          m_transform(transform)
    {
    }

    transform_iterator(const transform_iterator<InputIterator,
                                                UnaryFunction> &other)
        : super_type(other.base()),
          m_transform(other.m_transform)
    {
    }

    transform_iterator<InputIterator, UnaryFunction>&
    operator=(const transform_iterator<InputIterator,
                                       UnaryFunction> &other)
    {
        if(this != &other){
            super_type::operator=(other);

            m_transform = other.m_transform;
        }

        return *this;
    }

    ~transform_iterator()
    {
    }

    size_t get_index() const
    {
        return super_type::base().get_index();
    }

    const buffer& get_buffer() const
    {
        return detail::get_base_iterator_buffer(*this);
    }

    template<class IndexExpression>
    detail::transform_iterator_index_expr<InputIterator, UnaryFunction, IndexExpression>
    operator[](const IndexExpression &expr) const
    {
        return detail::transform_iterator_index_expr<InputIterator,
                                                     UnaryFunction,
                                                     IndexExpression>(super_type::base(),
                                                                      m_transform,
                                                                      expr);
    }

private:
    friend class ::boost::iterator_core_access;

    reference dereference() const
    {
        const context &context = super_type::base().get_buffer().get_context();
        command_queue queue(context, context.get_device());

        detail::meta_kernel k("read");
        size_t output_arg = k.add_arg<value_type *>(memory_object::global_memory, "output");
        k << "*output = " << m_transform(super_type::base()[k.lit(0)]) << ";";

        kernel kernel = k.compile(context);

        buffer output_buffer(context, sizeof(value_type));

        kernel.set_arg(output_arg, output_buffer);

        queue.enqueue_task(kernel);

        return detail::read_single_value<value_type>(output_buffer, queue);
    }

private:
    UnaryFunction m_transform;
};

/// Returns a transform_iterator for \p iterator with \p transform.
///
/// \param iterator the underlying iterator
/// \param transform the unary transform function
///
/// \return a \c transform_iterator for \p iterator with \p transform
///
/// For example, to create an iterator which returns the square-root of each
/// value in a \c vector<int>:
/// \code
/// auto sqrt_iterator = make_transform_iterator(vec.begin(), sqrt<int>());
/// \endcode
template<class InputIterator, class UnaryFunction>
inline transform_iterator<InputIterator, UnaryFunction>
make_transform_iterator(InputIterator iterator, UnaryFunction transform)
{
    return transform_iterator<InputIterator,
                              UnaryFunction>(iterator, transform);
}

/// \internal_ (is_device_iterator specialization for transform_iterator)
template<class InputIterator, class UnaryFunction>
struct is_device_iterator<
    transform_iterator<InputIterator, UnaryFunction> > : boost::true_type {};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ITERATOR_TRANSFORM_ITERATOR_HPP
