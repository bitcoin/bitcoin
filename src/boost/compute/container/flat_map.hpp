//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_CONTAINER_FLAT_MAP_HPP
#define BOOST_COMPUTE_CONTAINER_FLAT_MAP_HPP

#include <cstddef>
#include <utility>
#include <exception>

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

#include <boost/compute/exception.hpp>
#include <boost/compute/algorithm/find.hpp>
#include <boost/compute/algorithm/lower_bound.hpp>
#include <boost/compute/algorithm/upper_bound.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/functional/get.hpp>
#include <boost/compute/iterator/transform_iterator.hpp>
#include <boost/compute/types/pair.hpp>
#include <boost/compute/detail/buffer_value.hpp>

namespace boost {
namespace compute {

template<class Key, class T>
class flat_map
{
public:
    typedef Key key_type;
    typedef T mapped_type;
    typedef typename ::boost::compute::vector<std::pair<Key, T> > vector_type;
    typedef typename vector_type::value_type value_type;
    typedef typename vector_type::size_type size_type;
    typedef typename vector_type::difference_type difference_type;
    typedef typename vector_type::reference reference;
    typedef typename vector_type::const_reference const_reference;
    typedef typename vector_type::pointer pointer;
    typedef typename vector_type::const_pointer const_pointer;
    typedef typename vector_type::iterator iterator;
    typedef typename vector_type::const_iterator const_iterator;
    typedef typename vector_type::reverse_iterator reverse_iterator;
    typedef typename vector_type::const_reverse_iterator const_reverse_iterator;

    explicit flat_map(const context &context = system::default_context())
        : m_vector(context)
    {
    }

    flat_map(const flat_map<Key, T> &other)
        : m_vector(other.m_vector)
    {
    }

    flat_map<Key, T>& operator=(const flat_map<Key, T> &other)
    {
        if(this != &other){
            m_vector = other.m_vector;
        }

        return *this;
    }

    ~flat_map()
    {
    }

    iterator begin()
    {
        return m_vector.begin();
    }

    const_iterator begin() const
    {
        return m_vector.begin();
    }

    const_iterator cbegin() const
    {
        return m_vector.cbegin();
    }

    iterator end()
    {
        return m_vector.end();
    }

    const_iterator end() const
    {
        return m_vector.end();
    }

    const_iterator cend() const
    {
        return m_vector.cend();
    }

    reverse_iterator rbegin()
    {
        return m_vector.rbegin();
    }

    const_reverse_iterator rbegin() const
    {
        return m_vector.rbegin();
    }

    const_reverse_iterator crbegin() const
    {
        return m_vector.crbegin();
    }

    reverse_iterator rend()
    {
        return m_vector.rend();
    }

    const_reverse_iterator rend() const
    {
        return m_vector.rend();
    }

    const_reverse_iterator crend() const
    {
        return m_vector.crend();
    }

    size_type size() const
    {
        return m_vector.size();
    }

    size_type max_size() const
    {
        return m_vector.max_size();
    }

    bool empty() const
    {
        return m_vector.empty();
    }

    size_type capacity() const
    {
        return m_vector.capacity();
    }

    void reserve(size_type size, command_queue &queue)
    {
        m_vector.reserve(size, queue);
    }

    void reserve(size_type size)
    {
        command_queue queue = m_vector.default_queue();
        reserve(size, queue);
        queue.finish();
    }

    void shrink_to_fit()
    {
        m_vector.shrink_to_fit();
    }

    void clear()
    {
        m_vector.clear();
    }

    std::pair<iterator, bool>
    insert(const value_type &value, command_queue &queue)
    {
        iterator location = upper_bound(value.first, queue);

        if(location != begin()){
            value_type current_value;
            ::boost::compute::copy_n(location - 1, 1, &current_value, queue);
            if(value.first == current_value.first){
                return std::make_pair(location - 1, false);
            }
        }

        m_vector.insert(location, value);
        return std::make_pair(location, true);
    }

    std::pair<iterator, bool> insert(const value_type &value)
    {
        command_queue queue = m_vector.default_queue();
        std::pair<iterator, bool> result = insert(value, queue);
        queue.finish();
        return result;
    }

    iterator erase(const const_iterator &position, command_queue &queue)
    {
        return erase(position, position + 1, queue);
    }

    iterator erase(const const_iterator &position)
    {
        command_queue queue = m_vector.default_queue();
        iterator iter = erase(position, queue);
        queue.finish();
        return iter;
    }

    iterator erase(const const_iterator &first,
                   const const_iterator &last,
                   command_queue &queue)
    {
        return m_vector.erase(first, last, queue);
    }

    iterator erase(const const_iterator &first, const const_iterator &last)
    {
        command_queue queue = m_vector.default_queue();
        iterator iter = erase(first, last, queue);
        queue.finish();
        return iter;
    }

    size_type erase(const key_type &value, command_queue &queue)
    {
        iterator position = find(value, queue);

        if(position == end()){
            return 0;
        }
        else {
            erase(position, queue);
            return 1;
        }
    }

    iterator find(const key_type &value, command_queue &queue)
    {
        ::boost::compute::get<0> get_key;

        return ::boost::compute::find(
                   ::boost::compute::make_transform_iterator(begin(), get_key),
                   ::boost::compute::make_transform_iterator(end(), get_key),
                   value,
                   queue
               ).base();
    }

    iterator find(const key_type &value)
    {
        command_queue queue = m_vector.default_queue();
        iterator iter = find(value, queue);
        queue.finish();
        return iter;
    }

    const_iterator find(const key_type &value, command_queue &queue) const
    {
        ::boost::compute::get<0> get_key;

        return ::boost::compute::find(
                   ::boost::compute::make_transform_iterator(begin(), get_key),
                   ::boost::compute::make_transform_iterator(end(), get_key),
                   value,
                   queue
               ).base();
    }

    const_iterator find(const key_type &value) const
    {
        command_queue queue = m_vector.default_queue();
        const_iterator iter = find(value, queue);
        queue.finish();
        return iter;
    }

    size_type count(const key_type &value, command_queue &queue) const
    {
        return find(value, queue) != end() ? 1 : 0;
    }

    size_type count(const key_type &value) const
    {
        command_queue queue = m_vector.default_queue();
        size_type result = count(value, queue);
        queue.finish();
        return result;
    }

    iterator lower_bound(const key_type &value, command_queue &queue)
    {
        ::boost::compute::get<0> get_key;

        return ::boost::compute::lower_bound(
                   ::boost::compute::make_transform_iterator(begin(), get_key),
                   ::boost::compute::make_transform_iterator(end(), get_key),
                   value,
                   queue
               ).base();
    }

    iterator lower_bound(const key_type &value)
    {
        command_queue queue = m_vector.default_queue();
        iterator iter = lower_bound(value, queue);
        queue.finish();
        return iter;
    }

    const_iterator lower_bound(const key_type &value, command_queue &queue) const
    {
        ::boost::compute::get<0> get_key;

        return ::boost::compute::lower_bound(
                   ::boost::compute::make_transform_iterator(begin(), get_key),
                   ::boost::compute::make_transform_iterator(end(), get_key),
                   value,
                   queue
               ).base();
    }

    const_iterator lower_bound(const key_type &value) const
    {
        command_queue queue = m_vector.default_queue();
        const_iterator iter = lower_bound(value, queue);
        queue.finish();
        return iter;
    }

    iterator upper_bound(const key_type &value, command_queue &queue)
    {
        ::boost::compute::get<0> get_key;

        return ::boost::compute::upper_bound(
                   ::boost::compute::make_transform_iterator(begin(), get_key),
                   ::boost::compute::make_transform_iterator(end(), get_key),
                   value,
                   queue
               ).base();
    }

    iterator upper_bound(const key_type &value)
    {
        command_queue queue = m_vector.default_queue();
        iterator iter = upper_bound(value, queue);
        queue.finish();
        return iter;
    }

    const_iterator upper_bound(const key_type &value, command_queue &queue) const
    {
        ::boost::compute::get<0> get_key;

        return ::boost::compute::upper_bound(
                   ::boost::compute::make_transform_iterator(begin(), get_key),
                   ::boost::compute::make_transform_iterator(end(), get_key),
                   value,
                   queue
               ).base();
    }

    const_iterator upper_bound(const key_type &value) const
    {
        command_queue queue = m_vector.default_queue();
        const_iterator iter = upper_bound(value, queue);
        queue.finish();
        return iter;
    }

    const mapped_type at(const key_type &key) const
    {
        const_iterator iter = find(key);
        if(iter == end()){
            BOOST_THROW_EXCEPTION(std::out_of_range("key not found"));
        }

        return value_type(*iter).second;
    }

    detail::buffer_value<mapped_type> operator[](const key_type &key)
    {
        iterator iter = find(key);
        if(iter == end()){
            iter = insert(std::make_pair(key, mapped_type())).first;
        }

        size_t index = iter.get_index() * sizeof(value_type) + sizeof(key_type);

        return detail::buffer_value<mapped_type>(m_vector.get_buffer(), index);
    }

private:
    ::boost::compute::vector<std::pair<Key, T> > m_vector;
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_CONTAINER_FLAT_MAP_HPP
