// (C) Copyright 2005-2007 Matthias Troyer

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Matthias Troyer

#ifndef BOOST_MPI_BINARY_BUFFER_OPRIMITIVE_HPP
#define BOOST_MPI_BINARY_BUFFER_OPRIMITIVE_HPP

#include <mpi.h>
#include <iostream>
#include <cstddef> // size_t
#include <boost/config.hpp>

#include <boost/serialization/array.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/assert.hpp>
#include <vector>
#include <boost/mpi/allocator.hpp>
#include <boost/mpl/always.hpp>
#include <boost/type_traits/remove_const.hpp>

namespace boost { namespace mpi {

/// serialization using binary copy into a buffer

class BOOST_MPI_DECL binary_buffer_oprimitive
{
public:
    /// the type of the buffer into which the data is packed upon serialization
    typedef std::vector<char, allocator<char> > buffer_type;

    binary_buffer_oprimitive(buffer_type & b, MPI_Comm const &)
     : buffer_(b)
    {
    }

    void const * address() const
    {
      return detail::c_data(buffer_);
    }

    const std::size_t& size() const
    {
      return size_ = buffer_.size();
    }
    
    const std::size_t* size_ptr() const
    {
      return &size();
    }
    
    void save_binary(void const *address, std::size_t count)
    {
      save_impl(address,count);
    }

    // fast saving of arrays
    template<class T>
    void save_array(serialization::array_wrapper<T> const& x, unsigned int /* file_version */)
    {
    
      BOOST_MPL_ASSERT((serialization::is_bitwise_serializable<BOOST_DEDUCED_TYPENAME remove_const<T>::type>));
      if (x.count())
        save_impl(x.address(), x.count()*sizeof(T));
    }

    template<class T>
    void save(serialization::array_wrapper<T> const& x)
    {
      save_array(x,0u);
    }

    typedef serialization::is_bitwise_serializable<mpl::_1> use_array_optimization;

    // default saving of primitives.
    template<class T>
    void save(const T & t)
    {
      BOOST_MPL_ASSERT((serialization::is_bitwise_serializable<BOOST_DEDUCED_TYPENAME remove_const<T>::type>));
      save_impl(&t, sizeof(T));
    }

    template<class CharType>
    void save(const std::basic_string<CharType> &s)
    {
      unsigned int l = static_cast<unsigned int>(s.size());
      save(l);
      save_impl(s.data(),s.size());
    }

private:

    void save_impl(void const * p, int l)
    {
      char const* ptr = reinterpret_cast<char const*>(p);
      buffer_.insert(buffer_.end(),ptr,ptr+l);
    }

  buffer_type& buffer_;
  mutable std::size_t size_;
};

} } // end namespace boost::mpi

#endif // BOOST_MPI_BINARY_BUFFER_OPRIMITIVE_HPP
