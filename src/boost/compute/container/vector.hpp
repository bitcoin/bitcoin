//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_CONTAINER_VECTOR_HPP
#define BOOST_COMPUTE_CONTAINER_VECTOR_HPP

#include <vector>
#include <cstddef>
#include <iterator>
#include <exception>

#include <boost/throw_exception.hpp>

#include <boost/compute/config.hpp>

#ifndef BOOST_COMPUTE_NO_HDR_INITIALIZER_LIST
#include <initializer_list>
#endif

#include <boost/compute/buffer.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/algorithm/copy_n.hpp>
#include <boost/compute/algorithm/fill_n.hpp>
#include <boost/compute/allocator/buffer_allocator.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>
#include <boost/compute/type_traits/detail/capture_traits.hpp>
#include <boost/compute/detail/buffer_value.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>

namespace boost {
namespace compute {

/// \class vector
/// \brief A resizable array of values.
///
/// The vector<T> class stores a dynamic array of values. Internally, the data
/// is stored in an OpenCL buffer object.
///
/// The vector class is the prefered container for storing and accessing data
/// on a compute device. In most cases it should be used instead of directly
/// dealing with buffer objects. If the undelying buffer is needed, it can be
/// accessed with the get_buffer() method.
///
/// The internal storage is allocated in a specific OpenCL context which is
/// passed as an argument to the constructor when the vector is created.
///
/// For example, to create a vector on the device containing space for ten
/// \c int values:
/// \code
/// boost::compute::vector<int> vec(10, context);
/// \endcode
///
/// Allocation and data transfer can also be performed in a single step:
/// \code
/// // values on the host
/// int data[] = { 1, 2, 3, 4 };
///
/// // create a vector of size four and copy the values from data
/// boost::compute::vector<int> vec(data, data + 4, queue);
/// \endcode
///
/// The Boost.Compute \c vector class provides a STL-like API and is modeled
/// after the \c std::vector class from the C++ standard library. It can be
/// used with any of the STL-like algorithms provided by Boost.Compute
/// including \c copy(), \c transform(), and \c sort() (among many others).
///
/// For example:
/// \code
/// // a vector on a compute device
/// boost::compute::vector<float> vec = ...
///
/// // copy data to the vector from a host std:vector
/// boost::compute::copy(host_vec.begin(), host_vec.end(), vec.begin(), queue);
///
/// // copy data from the vector to a host std::vector
/// boost::compute::copy(vec.begin(), vec.end(), host_vec.begin(), queue);
///
/// // sort the values in the vector
/// boost::compute::sort(vec.begin(), vec.end(), queue);
///
/// // calculate the sum of the values in the vector (also see reduce())
/// float sum = boost::compute::accumulate(vec.begin(), vec.end(), 0, queue);
///
/// // reverse the values in the vector
/// boost::compute::reverse(vec.begin(), vec.end(), queue);
///
/// // fill the vector with ones
/// boost::compute::fill(vec.begin(), vec.end(), 1, queue);
/// \endcode
///
/// \see \ref array "array<T, N>", buffer
template<class T, class Alloc = buffer_allocator<T> >
class vector
{
public:
    typedef T value_type;
    typedef Alloc allocator_type;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;
    typedef detail::buffer_value<T> reference;
    typedef const detail::buffer_value<T> const_reference;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef buffer_iterator<T> iterator;
    typedef buffer_iterator<T> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    /// Creates an empty vector in \p context.
    explicit vector(const context &context = system::default_context())
        : m_size(0),
          m_allocator(context)
    {
        m_data = m_allocator.allocate(_minimum_capacity());
    }

    /// Creates a vector with space for \p count elements in \p context.
    ///
    /// Note that unlike \c std::vector's constructor, this will not initialize
    /// the values in the container. Either call the vector constructor which
    /// takes a value to initialize with or use the fill() algorithm to set
    /// the initial values.
    ///
    /// For example:
    /// \code
    /// // create a vector on the device with space for ten ints
    /// boost::compute::vector<int> vec(10, context);
    /// \endcode
    explicit vector(size_type count,
                    const context &context = system::default_context())
        : m_size(count),
          m_allocator(context)
    {
        m_data = m_allocator.allocate((std::max)(count, _minimum_capacity()));
    }

    /// Creates a vector with space for \p count elements and sets each equal
    /// to \p value.
    ///
    /// For example:
    /// \code
    /// // creates a vector with four values set to nine (e.g. [9, 9, 9, 9]).
    /// boost::compute::vector<int> vec(4, 9, queue);
    /// \endcode
    vector(size_type count,
           const T &value,
           command_queue &queue = system::default_queue())
        : m_size(count),
          m_allocator(queue.get_context())
    {
        m_data = m_allocator.allocate((std::max)(count, _minimum_capacity()));

        ::boost::compute::fill_n(begin(), count, value, queue);
    }

    /// Creates a vector with space for the values in the range [\p first,
    /// \p last) and copies them into the vector with \p queue.
    ///
    /// For example:
    /// \code
    /// // values on the host
    /// int data[] = { 1, 2, 3, 4 };
    ///
    /// // create a vector of size four and copy the values from data
    /// boost::compute::vector<int> vec(data, data + 4, queue);
    /// \endcode
    template<class InputIterator>
    vector(InputIterator first,
           InputIterator last,
           command_queue &queue = system::default_queue())
        : m_size(detail::iterator_range_size(first, last)),
          m_allocator(queue.get_context())
    {
        m_data = m_allocator.allocate((std::max)(m_size, _minimum_capacity()));

        ::boost::compute::copy(first, last, begin(), queue);
    }

    /// Creates a new vector and copies the values from \p other.
    vector(const vector &other,
           command_queue &queue = system::default_queue())
        : m_size(other.m_size),
          m_allocator(other.m_allocator)
    {
        m_data = m_allocator.allocate((std::max)(m_size, _minimum_capacity()));

        if(!other.empty()){
            if(other.get_buffer().get_context() != queue.get_context()){
                command_queue other_queue = other.default_queue();
                ::boost::compute::copy(other.begin(), other.end(), begin(), other_queue);
                other_queue.finish();
            }
            else {
                ::boost::compute::copy(other.begin(), other.end(), begin(), queue);
                queue.finish();
            }
        }
    }

    /// Creates a new vector and copies the values from \p other.
    template<class OtherAlloc>
    vector(const vector<T, OtherAlloc> &other,
           command_queue &queue = system::default_queue())
        : m_size(other.size()),
          m_allocator(queue.get_context())
    {
        m_data = m_allocator.allocate((std::max)(m_size, _minimum_capacity()));

        if(!other.empty()){
            ::boost::compute::copy(other.begin(), other.end(), begin(), queue);
            queue.finish();
        }
    }

    /// Creates a new vector and copies the values from \p vector.
    template<class OtherAlloc>
    vector(const std::vector<T, OtherAlloc> &vector,
           command_queue &queue = system::default_queue())
        : m_size(vector.size()),
          m_allocator(queue.get_context())
    {
        m_data = m_allocator.allocate((std::max)(m_size, _minimum_capacity()));

        ::boost::compute::copy(vector.begin(), vector.end(), begin(), queue);
    }

    #ifndef BOOST_COMPUTE_NO_HDR_INITIALIZER_LIST
    vector(std::initializer_list<T> list,
           command_queue &queue = system::default_queue())
        : m_size(list.size()),
          m_allocator(queue.get_context())
    {
        m_data = m_allocator.allocate((std::max)(m_size, _minimum_capacity()));

        ::boost::compute::copy(list.begin(), list.end(), begin(), queue);
    }
    #endif // BOOST_COMPUTE_NO_HDR_INITIALIZER_LIST

    vector& operator=(const vector &other)
    {
        if(this != &other){
            command_queue queue = default_queue();
            resize(other.size(), queue);
            ::boost::compute::copy(other.begin(), other.end(), begin(), queue);
            queue.finish();
        }

        return *this;
    }

    template<class OtherAlloc>
    vector& operator=(const vector<T, OtherAlloc> &other)
    {
        command_queue queue = default_queue();
        resize(other.size(), queue);
        ::boost::compute::copy(other.begin(), other.end(), begin(), queue);
        queue.finish();

        return *this;
    }

    template<class OtherAlloc>
    vector& operator=(const std::vector<T, OtherAlloc> &vector)
    {
        command_queue queue = default_queue();
        resize(vector.size(), queue);
        ::boost::compute::copy(vector.begin(), vector.end(), begin(), queue);
        queue.finish();
        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    /// Move-constructs a new vector from \p other.
    vector(vector&& other)
        : m_data(std::move(other.m_data)),
          m_size(other.m_size),
          m_allocator(std::move(other.m_allocator))
    {
        other.m_size = 0;
    }

    /// Move-assigns the data from \p other to \c *this.
    vector& operator=(vector&& other)
    {
        if(capacity() > 0){
            m_allocator.deallocate(m_data, capacity());
        }

        m_data = std::move(other.m_data);
        m_size = other.m_size;
        m_allocator = std::move(other.m_allocator);

        other.m_size = 0;

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the vector object.
    ~vector()
    {
        if(capacity() > 0){
            m_allocator.deallocate(m_data, capacity());
        }
    }

    iterator begin()
    {
        return ::boost::compute::make_buffer_iterator<T>(m_data.get_buffer(), 0);
    }

    const_iterator begin() const
    {
        return ::boost::compute::make_buffer_iterator<T>(m_data.get_buffer(), 0);
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    iterator end()
    {
        return ::boost::compute::make_buffer_iterator<T>(m_data.get_buffer(), m_size);
    }

    const_iterator end() const
    {
        return ::boost::compute::make_buffer_iterator<T>(m_data.get_buffer(), m_size);
    }

    const_iterator cend() const
    {
        return end();
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end() - 1);
    }

    const_reverse_iterator rbegin() const
    {
        return reverse_iterator(end() - 1);
    }

    const_reverse_iterator crbegin() const
    {
        return rbegin();
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin() - 1);
    }

    const_reverse_iterator rend() const
    {
        return reverse_iterator(begin() - 1);
    }

    const_reverse_iterator crend() const
    {
        return rend();
    }

    /// Returns the number of elements in the vector.
    size_type size() const
    {
        return m_size;
    }

    size_type max_size() const
    {
        return m_allocator.max_size();
    }

    /// Resizes the vector to \p size.
    void resize(size_type size, command_queue &queue)
    {
        if(size <= capacity()){
            m_size = size;
        }
        else {
            // allocate new buffer
            pointer new_data =
                m_allocator.allocate(
                    static_cast<size_type>(
                        static_cast<float>(size) * _growth_factor()
                    )
                );

            if(capacity() > 0)
            {
                // copy old values to the new buffer
                ::boost::compute::copy(m_data, m_data + m_size, new_data, queue);

                // free old memory
                m_allocator.deallocate(m_data, capacity());
            }

            // set new data and size
            m_data = new_data;
            m_size = size;
        }
    }

    /// \overload
    void resize(size_type size)
    {
        command_queue queue = default_queue();
        resize(size, queue);
        queue.finish();
    }

    /// Returns \c true if the vector is empty.
    bool empty() const
    {
        return m_size == 0;
    }

    /// Returns the capacity of the vector.
    size_type capacity() const
    {
        if(m_data == pointer()) // null pointer check
        {
            return 0;
        }
        return m_data.get_buffer().size() / sizeof(T);
    }

    void reserve(size_type size, command_queue &queue)
    {
        if(size > max_size()){
            throw std::length_error("vector::reserve");
        }
        if(capacity() < size){
            // allocate new buffer
            pointer new_data =
                m_allocator.allocate(
                    static_cast<size_type>(
                        static_cast<float>(size) * _growth_factor()
                    )
                );

            if(capacity() > 0)
            {
                // copy old values to the new buffer
                ::boost::compute::copy(m_data, m_data + m_size, new_data, queue);

                // free old memory
                m_allocator.deallocate(m_data, capacity());
            }

            // set new data
            m_data = new_data;
        }
    }

    void reserve(size_type size)
    {
        command_queue queue = default_queue();
        reserve(size, queue);
        queue.finish();
    }

    void shrink_to_fit(command_queue &queue)
    {
        pointer old_data = m_data;
        m_data = pointer(); // null pointer
        if(m_size > 0)
        {
            // allocate new buffer
            m_data = m_allocator.allocate(m_size);

            // copy old values to the new buffer
            ::boost::compute::copy(old_data, old_data + m_size, m_data, queue);
        }

        if(capacity() > 0)
        {
            // free old memory
            m_allocator.deallocate(old_data, capacity());
        }
    }

    void shrink_to_fit()
    {
        command_queue queue = default_queue();
        shrink_to_fit(queue);
        queue.finish();
    }

    reference operator[](size_type index)
    {
        return *(begin() + static_cast<difference_type>(index));
    }

    const_reference operator[](size_type index) const
    {
        return *(begin() + static_cast<difference_type>(index));
    }

    reference at(size_type index)
    {
        if(index >= size()){
            BOOST_THROW_EXCEPTION(std::out_of_range("index out of range"));
        }

        return operator[](index);
    }

    const_reference at(size_type index) const
    {
        if(index >= size()){
            BOOST_THROW_EXCEPTION(std::out_of_range("index out of range"));
        }

        return operator[](index);
    }

    reference front()
    {
        return *begin();
    }

    const_reference front() const
    {
        return *begin();
    }

    reference back()
    {
        return *(end() - static_cast<difference_type>(1));
    }

    const_reference back() const
    {
        return *(end() - static_cast<difference_type>(1));
    }

    template<class InputIterator>
    void assign(InputIterator first,
                InputIterator last,
                command_queue &queue)
    {
        // resize vector for new contents
        resize(detail::iterator_range_size(first, last), queue);

        // copy values into the vector
        ::boost::compute::copy(first, last, begin(), queue);
    }

    template<class InputIterator>
    void assign(InputIterator first, InputIterator last)
    {
        command_queue queue = default_queue();
        assign(first, last, queue);
        queue.finish();
    }

    void assign(size_type n, const T &value, command_queue &queue)
    {
        // resize vector for new contents
        resize(n, queue);

        // fill vector with value
        ::boost::compute::fill_n(begin(), n, value, queue);
    }

    void assign(size_type n, const T &value)
    {
        command_queue queue = default_queue();
        assign(n, value, queue);
        queue.finish();
    }

    /// Inserts \p value at the end of the vector (resizing if neccessary).
    ///
    /// Note that calling \c push_back() to insert data values one at a time
    /// is inefficient as there is a non-trivial overhead in performing a data
    /// transfer to the device. It is usually better to store a set of values
    /// on the host (for example, in a \c std::vector) and then transfer them
    /// in bulk using the \c insert() method or the copy() algorithm.
    void push_back(const T &value, command_queue &queue)
    {
        insert(end(), value, queue);
    }

    /// \overload
    void push_back(const T &value)
    {
        command_queue queue = default_queue();
        push_back(value, queue);
        queue.finish();
    }

    void pop_back(command_queue &queue)
    {
        resize(size() - 1, queue);
    }

    void pop_back()
    {
        command_queue queue = default_queue();
        pop_back(queue);
        queue.finish();
    }

    iterator insert(iterator position, const T &value, command_queue &queue)
    {
        if(position == end()){
            resize(m_size + 1, queue);
            position = begin() + position.get_index();
            ::boost::compute::copy_n(&value, 1, position, queue);
        }
        else {
            ::boost::compute::vector<T, Alloc> tmp(position, end(), queue);
            resize(m_size + 1, queue);
            position = begin() + position.get_index();
            ::boost::compute::copy_n(&value, 1, position, queue);
            ::boost::compute::copy(tmp.begin(), tmp.end(), position + 1, queue);
        }

        return position + 1;
    }

    iterator insert(iterator position, const T &value)
    {
        command_queue queue = default_queue();
        iterator iter = insert(position, value, queue);
        queue.finish();
        return iter;
    }

    void insert(iterator position,
                size_type count,
                const T &value,
                command_queue &queue)
    {
        ::boost::compute::vector<T, Alloc> tmp(position, end(), queue);
        resize(size() + count, queue);

        position = begin() + position.get_index();

        ::boost::compute::fill_n(position, count, value, queue);
        ::boost::compute::copy(
            tmp.begin(),
            tmp.end(),
            position + static_cast<difference_type>(count),
            queue
        );
    }

    void insert(iterator position, size_type count, const T &value)
    {
        command_queue queue = default_queue();
        insert(position, count, value, queue);
        queue.finish();
    }

    /// Inserts the values in the range [\p first, \p last) into the vector at
    /// \p position using \p queue.
    template<class InputIterator>
    void insert(iterator position,
                InputIterator first,
                InputIterator last,
                command_queue &queue)
    {
        ::boost::compute::vector<T, Alloc> tmp(position, end(), queue);

        size_type count = detail::iterator_range_size(first, last);
        resize(size() + count, queue);

        position = begin() + position.get_index();

        ::boost::compute::copy(first, last, position, queue);
        ::boost::compute::copy(
            tmp.begin(),
            tmp.end(),
            position + static_cast<difference_type>(count),
            queue
        );
    }

    /// \overload
    template<class InputIterator>
    void insert(iterator position, InputIterator first, InputIterator last)
    {
        command_queue queue = default_queue();
        insert(position, first, last, queue);
        queue.finish();
    }

    iterator erase(iterator position, command_queue &queue)
    {
        return erase(position, position + 1, queue);
    }

    iterator erase(iterator position)
    {
        command_queue queue = default_queue();
        iterator iter = erase(position, queue);
        queue.finish();
        return iter;
    }

    iterator erase(iterator first, iterator last, command_queue &queue)
    {
        if(last != end()){
            ::boost::compute::vector<T, Alloc> tmp(last, end(), queue);
            ::boost::compute::copy(tmp.begin(), tmp.end(), first, queue);
        }

        difference_type count = std::distance(first, last);
        resize(size() - static_cast<size_type>(count), queue);

        return begin() + first.get_index() + count;
    }

    iterator erase(iterator first, iterator last)
    {
        command_queue queue = default_queue();
        iterator iter = erase(first, last, queue);
        queue.finish();
        return iter;
    }

    /// Swaps the contents of \c *this with \p other.
    void swap(vector &other)
    {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
        std::swap(m_allocator, other.m_allocator);
    }

    /// Removes all elements from the vector.
    void clear()
    {
        m_size = 0;
    }

    allocator_type get_allocator() const
    {
        return m_allocator;
    }

    /// Returns the underlying buffer.
    const buffer& get_buffer() const
    {
        return m_data.get_buffer();
    }

    /// \internal_
    ///
    /// Returns a command queue usable to issue commands for the vector's
    /// memory buffer. This is used when a member function is called without
    /// specifying an existing command queue to use.
    command_queue default_queue() const
    {
        const context &context = m_allocator.get_context();
        command_queue queue(context, context.get_device());
        return queue;
    }

private:
    /// \internal_
    BOOST_CONSTEXPR size_type _minimum_capacity() const { return 4; }

    /// \internal_
    BOOST_CONSTEXPR float _growth_factor() const { return 1.5; }

private:
    pointer m_data;
    size_type m_size;
    allocator_type m_allocator;
};

namespace detail {

// set_kernel_arg specialization for vector<T>
template<class T, class Alloc>
struct set_kernel_arg<vector<T, Alloc> >
{
    void operator()(kernel &kernel_, size_t index, const vector<T, Alloc> &vector)
    {
        kernel_.set_arg(index, vector.get_buffer());
    }
};

// for capturing vector<T> with BOOST_COMPUTE_CLOSURE()
template<class T, class Alloc>
struct capture_traits<vector<T, Alloc> >
{
    static std::string type_name()
    {
        return std::string("__global ") + ::boost::compute::type_name<T>() + "*";
    }
};

// meta_kernel streaming operator for vector<T>
template<class T, class Alloc>
meta_kernel& operator<<(meta_kernel &k, const vector<T, Alloc> &vector)
{
  return k << k.get_buffer_identifier<T>(vector.get_buffer());
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_CONTAINER_VECTOR_HPP
