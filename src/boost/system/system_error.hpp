//  Boost system_error.hpp  --------------------------------------------------//

//  Copyright Beman Dawes 2006

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SYSTEM_SYSTEM_ERROR_HPP
#define BOOST_SYSTEM_SYSTEM_ERROR_HPP

#include <boost/system/error_code.hpp>
#include <string>
#include <stdexcept>
#include <cassert>

namespace boost
{
  namespace system
  {
    //  class system_error  ------------------------------------------------------------//

    class BOOST_SYMBOL_VISIBLE system_error : public std::runtime_error
    // BOOST_SYMBOL_VISIBLE is needed by GCC to ensure system_error thrown from a shared
    // library can be caught. See svn.boost.org/trac/boost/ticket/3697
    {
    public:
      explicit system_error( error_code ec )
          : std::runtime_error(""), m_error_code(ec) {}

      system_error( error_code ec, const std::string & what_arg )
          : std::runtime_error(what_arg), m_error_code(ec) {}

      system_error( error_code ec, const char* what_arg )
          : std::runtime_error(what_arg), m_error_code(ec) {}

      system_error( int ev, const error_category & ecat )
          : std::runtime_error(""), m_error_code(ev,ecat) {}

      system_error( int ev, const error_category & ecat,
        const std::string & what_arg )
          : std::runtime_error(what_arg), m_error_code(ev,ecat) {}

      system_error( int ev, const error_category & ecat,
        const char * what_arg )
          : std::runtime_error(what_arg), m_error_code(ev,ecat) {}

      virtual ~system_error() BOOST_NOEXCEPT_OR_NOTHROW {}

      error_code code() const BOOST_NOEXCEPT { return m_error_code; }
      const char * what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE;

    private:
      error_code           m_error_code;
      mutable std::string  m_what;
    };

    //  implementation  ------------------------------------------------------//

    inline const char * system_error::what() const BOOST_NOEXCEPT_OR_NOTHROW
    // see http://www.boost.org/more/error_handling.html for lazy build rationale
    {
      if ( m_what.empty() )
      {
#ifndef BOOST_NO_EXCEPTIONS
        try
#endif
        {
          m_what = this->std::runtime_error::what();
          if ( !m_what.empty() ) m_what += ": ";
          m_what += m_error_code.message();
        }
#ifndef BOOST_NO_EXCEPTIONS
        catch (...) { return std::runtime_error::what(); }
#endif
      }
      return m_what.c_str();
    }

  } // namespace system
} // namespace boost

#endif // BOOST_SYSTEM_SYSTEM_ERROR_HPP


