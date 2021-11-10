//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ITERATOR_FUNCTION_INPUT_ITERATOR_HPP
#define BOOST_COMPUTE_ITERATOR_FUNCTION_INPUT_ITERATOR_HPP

#include <cstddef>
#include <iterator>

#include <boost/config.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>
#include <boost/compute/type_traits/result_of.hpp>

namespace boost {
namespace compute {

// forward declaration for function_input_iterator<Function>
template<class Function> class function_input_iterator;

namespace detail {

// helper class which defines the iterator_facade super-class
// type for function_input_iterator<Function>
template<class Function>
class function_input_iterator_base
{
public:
    typedef ::boost::iterator_facade<
        ::boost::compute::function_input_iterator<Function>,
        typename ::boost::compute::result_of<Function()>::type,
        ::std::random_access_iterator_tag,
        typename ::boost::compute::result_of<Function()>::type
    > type;
};

template<class Function>
struct function_input_iterator_expr
{
    typedef typename ::boost::compute::result_of<Function()>::type result_type;

    function_input_iterator_expr(const Function &function)
        : m_function(function)
    {
    }

    const Function m_function;
};

template<class Function>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const function_input_iterator_expr<Function> &expr)
{
    return kernel << expr.m_function();
}

} // end detail namespace

/// \class function_input_iterator
/// \brief Iterator which returns the result of a function when dereferenced
///
/// For example:
///
/// \snippet test/test_function_input_iterator.cpp generate_42
///
/// \see make_function_input_iterator()
template<class Function>
class function_input_iterator :
    public detail::function_input_iterator_base<Function>::type
{
public:
    typedef typename detail::function_input_iterator_base<Function>::type super_type;
    typedef typename super_type::reference reference;
    typedef typename super_type::difference_type difference_type;
    typedef Function function;

    function_input_iterator(const Function &function, size_t index = 0)
        : m_function(function),
          m_index(index)
    {
    }

    function_input_iterator(const function_input_iterator<Function> &other)
        : m_function(other.m_function),
          m_index(other.m_index)
    {
    }

    function_input_iterator<Function>&
    operator=(const function_input_iterator<Function> &other)
    {
        if(this != &other){
            m_function = other.m_function;
            m_index = other.m_index;
        }

        return *this;
    }

    ~function_input_iterator()
    {
    }

    size_t get_index() const
    {
        return m_index;
    }

    template<class Expr>
    detail::function_input_iterator_expr<Function>
    operator[](const Expr &expr) const
    {
        (void) expr;

        return detail::function_input_iterator_expr<Function>(m_function);
    }

private:
    friend class ::boost::iterator_core_access;

    reference dereference() const
    {
        return reference();
    }

    bool equal(const function_input_iterator<Function> &other) const
    {
        return m_function == other.m_function && m_index == other.m_index;
    }

    void increment()
    {
        m_index++;
    }

    void decrement()
    {
        m_index--;
    }

    void advance(difference_type n)
    {
        m_index = static_cast<size_t>(static_cast<difference_type>(m_index) + n);
    }

    difference_type
    distance_to(const function_input_iterator<Function> &other) const
    {
        return static_cast<difference_type>(other.m_index - m_index);
    }

private:
    Function m_function;
    size_t m_index;
};

/// Returns a function_input_iterator with \p function.
///
/// \param function function to execute when dereferenced
/// \param index index of the iterator
///
/// \return a \c function_input_iterator with \p function
template<class Function>
inline function_input_iterator<Function>
make_function_input_iterator(const Function &function, size_t index = 0)
{
    return function_input_iterator<Function>(function, index);
}

/// \internal_ (is_device_iterator specialization for function_input_iterator)
template<class Function>
struct is_device_iterator<function_input_iterator<Function> > : boost::true_type {};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ITERATOR_FUNCTION_INPUT_ITERATOR_HPP
