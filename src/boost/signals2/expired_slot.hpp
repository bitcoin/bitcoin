// Boost.Signals2 library

// Copyright Frank Mori Hess 2007-2010.
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_EXPIRED_SLOT_HPP
#define BOOST_SIGNALS2_EXPIRED_SLOT_HPP

#include <boost/smart_ptr/bad_weak_ptr.hpp>

namespace boost
{
  namespace signals2
  {
    class expired_slot: public bad_weak_ptr
    {
    public:
      virtual char const * what() const throw()
      {
        return "boost::signals2::expired_slot";
      }
    };
  }
} // end namespace boost

#endif // BOOST_SIGNALS2_EXPIRED_SLOT_HPP
