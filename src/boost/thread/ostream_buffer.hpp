// (C) Copyright 2013 Vicente J. Botet Escriba
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_THREAD_OSTREAM_BUFFER_HPP
#define BOOST_THREAD_OSTREAM_BUFFER_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <sstream>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  template <typename OStream>
  class ostream_buffer
  {
  public:
    typedef std::basic_ostringstream<typename OStream::char_type, typename OStream::traits_type> stream_type;
    ostream_buffer(OStream& os) :
      os_(os)
    {
    }
    ~ostream_buffer()
    {
      os_ << o_str_.str();
    }
    stream_type& stream()
    {
      return o_str_;
    }
  private:
    OStream& os_;
    stream_type o_str_;
  };

}

#include <boost/config/abi_suffix.hpp>

#endif // header
