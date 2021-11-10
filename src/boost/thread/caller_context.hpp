// (C) Copyright 2013,2015 Vicente J. Botet Escriba
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_THREAD_CALL_CONTEXT_HPP
#define BOOST_THREAD_CALL_CONTEXT_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_USES_LOG_THREAD_ID
#include <boost/thread/thread.hpp>
#endif
#include <boost/current_function.hpp>
#include <boost/io/ios_state.hpp>
#include <iomanip>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  struct caller_context_t
  {
    const char * filename;
    unsigned lineno;
    const char * func;
    caller_context_t(const char * filename, unsigned lineno, const char * func) :
      filename(filename), lineno(lineno), func(func)
    {
    }
  };

#define BOOST_CONTEXTOF boost::caller_context_t(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION)

  template <typename OStream>
  OStream& operator<<(OStream& os, caller_context_t const& ctx)
  {
#if defined BOOST_THREAD_USES_LOG_THREAD_ID
    {
      io::ios_flags_saver ifs( os );
      os << std::left << std::setw(14) << boost::this_thread::get_id() << " ";
    }
#endif
    {
      io::ios_flags_saver ifs(os);
      os << std::setw(50) << ctx.filename << "["
         << std::setw(4) << std::right << std::dec<< ctx.lineno << "] ";
#if defined BOOST_THREAD_USES_LOG_CURRENT_FUNCTION
      os << ctx.func << " " ;
#endif
    }
    return os;
  }
}

#include <boost/config/abi_suffix.hpp>

#endif // header
