// (C) Copyright 2005 Matthias Troyer

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Matthias Troyer

#ifndef BOOST_MPI_PACKED_OPRIMITIVE_HPP
#define BOOST_MPI_PACKED_OPRIMITIVE_HPP

#include <boost/mpi/config.hpp>
#include <cstddef> // size_t
#include <boost/config.hpp>
#include <boost/mpi/datatype.hpp>
#include <boost/mpi/exception.hpp>
#include <boost/mpi/detail/antiques.hpp>
#include <boost/serialization/array.hpp>
#include <boost/assert.hpp>
#include <vector>
#include <boost/mpi/allocator.hpp>

namespace boost { namespace mpi {

/// serialization using MPI::Pack

class BOOST_MPI_DECL packed_oprimitive
{
public:
    /// the type of the buffer into which the data is packed upon serialization
    typedef std::vector<char, allocator<char> > buffer_type;

    packed_oprimitive(buffer_type & b, MPI_Comm const & comm)
         : buffer_(b),
           comm(comm)
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
          save_impl(address,MPI_BYTE,count);
        }

    // fast saving of arrays
    template<class T>
    void save_array(serialization::array_wrapper<T> const& x, unsigned int /* file_version */)
    {
        if (x.count())
          save_impl(x.address(), get_mpi_datatype(*x.address()), x.count());
    }

    typedef is_mpi_datatype<mpl::_1> use_array_optimization;

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
    friend class archive::save_access;
protected:
#else
public:
#endif

    // default saving of primitives.
    template<class T>
    void save(const T & t)
    {
      save_impl(&t, get_mpi_datatype<T>(t), 1);
    }

    template<class CharType>
    void save(const std::basic_string<CharType> &s)
    {
      unsigned int l = static_cast<unsigned int>(s.size());
      save(l);
      if (l)
        save_impl(s.data(),get_mpi_datatype(CharType()),s.size());
    }

private:

    void save_impl(void const * p, MPI_Datatype t, int l)
    {
      // allocate enough memory
      int memory_needed;
      BOOST_MPI_CHECK_RESULT(MPI_Pack_size,(l,t,comm,&memory_needed));

      int position = buffer_.size();
      buffer_.resize(position + memory_needed);

      // pack the data into the buffer
      BOOST_MPI_CHECK_RESULT(MPI_Pack,
                             (const_cast<void*>(p),l,t, 
                              detail::c_data(buffer_),
                              buffer_.size(), 
                              &position,comm));
      // reduce the buffer size if needed
      BOOST_ASSERT(std::size_t(position) <= buffer_.size());
      if (std::size_t(position) < buffer_.size())
          buffer_.resize(position);
    }

    static buffer_type::value_type* get_data(buffer_type& b)
    {
      return detail::c_data(b);
    }

  buffer_type& buffer_;
  mutable std::size_t size_;
  MPI_Comm comm;
};

} } // end namespace boost::mpi

#endif // BOOST_MPI_PACKED_OPRIMITIVE_HPP
