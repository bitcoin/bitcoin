//  boost progress.hpp header file  ------------------------------------------//

//  Copyright Beman Dawes 1994-99.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/timer for documentation.

//  Revision History
//   1 Dec 01  Add leading progress display strings (suggested by Toon Knapen)
//  20 May 01  Introduce several static_casts<> to eliminate warning messages
//             (Fixed by Beman, reported by Herve Bronnimann)
//  12 Jan 01  Change to inline implementation to allow use without library
//             builds. See docs for more rationale. (Beman Dawes) 
//  22 Jul 99  Name changed to .hpp
//  16 Jul 99  Second beta
//   6 Jul 99  Initial boost version

#ifndef BOOST_PROGRESS_HPP
#define BOOST_PROGRESS_HPP

#include <boost/config/header_deprecated.hpp>
BOOST_HEADER_DEPRECATED( "the facilities in <boost/timer/timer.hpp> or <boost/timer/progress_display.hpp>" )

#include <boost/timer.hpp>
#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>  // for uintmax_t
#include <iostream>           // for ostream, cout, etc
#include <string>             // for string

namespace boost {

//  progress_timer  ----------------------------------------------------------//

//  A progress_timer behaves like a timer except that the destructor displays
//  an elapsed time message at an appropriate place in an appropriate form.

class progress_timer : public timer, private noncopyable
{
  
 public:
  explicit progress_timer( std::ostream & os = std::cout )
     // os is hint; implementation may ignore, particularly in embedded systems
     : timer(), noncopyable(), m_os(os) {}
  ~progress_timer()
  {
  //  A) Throwing an exception from a destructor is a Bad Thing.
  //  B) The progress_timer destructor does output which may throw.
  //  C) A progress_timer is usually not critical to the application.
  //  Therefore, wrap the I/O in a try block, catch and ignore all exceptions.
    try
    {
      // use istream instead of ios_base to workaround GNU problem (Greg Chicares)
      std::istream::fmtflags old_flags = m_os.setf( std::istream::fixed,
                                                   std::istream::floatfield );
      std::streamsize old_prec = m_os.precision( 2 );
      m_os << elapsed() << " s\n" // "s" is System International d'Unites std
                        << std::endl;
      m_os.flags( old_flags );
      m_os.precision( old_prec );
    }

    catch (...) {} // eat any exceptions
  } // ~progress_timer

 private:
  std::ostream & m_os;
};


//  progress_display  --------------------------------------------------------//

//  progress_display displays an appropriate indication of 
//  progress at an appropriate place in an appropriate form.

// NOTE: (Jan 12, 2001) Tried to change unsigned long to boost::uintmax_t, but
// found some compilers couldn't handle the required conversion to double.
// Reverted to unsigned long until the compilers catch up. 

class progress_display : private noncopyable
{
 public:
  explicit progress_display( unsigned long expected_count_,
                             std::ostream & os = std::cout,
                             const std::string & s1 = "\n", //leading strings
                             const std::string & s2 = "",
                             const std::string & s3 = "" )
   // os is hint; implementation may ignore, particularly in embedded systems
   : noncopyable(), m_os(os), m_s1(s1), m_s2(s2), m_s3(s3) { restart(expected_count_); }

  void           restart( unsigned long expected_count_ )
  //  Effects: display appropriate scale
  //  Postconditions: count()==0, expected_count()==expected_count_
  {
    _count = _next_tic_count = _tic = 0;
    _expected_count = expected_count_;

    m_os << m_s1 << "0%   10   20   30   40   50   60   70   80   90   100%\n"
         << m_s2 << "|----|----|----|----|----|----|----|----|----|----|"
         << std::endl  // endl implies flush, which ensures display
         << m_s3;
    if ( !_expected_count ) _expected_count = 1;  // prevent divide by zero
  } // restart

  unsigned long  operator+=( unsigned long increment )
  //  Effects: Display appropriate progress tic if needed.
  //  Postconditions: count()== original count() + increment
  //  Returns: count().
  {
    if ( (_count += increment) >= _next_tic_count ) { display_tic(); }
    return _count;
  }

  unsigned long  operator++()           { return operator+=( 1 ); }
  unsigned long  count() const          { return _count; }
  unsigned long  expected_count() const { return _expected_count; }

  private:
  std::ostream &     m_os;  // may not be present in all imps
  const std::string  m_s1;  // string is more general, safer than 
  const std::string  m_s2;  //  const char *, and efficiency or size are
  const std::string  m_s3;  //  not issues

  unsigned long _count, _expected_count, _next_tic_count;
  unsigned int  _tic;
  void display_tic()
  {
    // use of floating point ensures that both large and small counts
    // work correctly.  static_cast<>() is also used several places
    // to suppress spurious compiler warnings. 
    unsigned int tics_needed = static_cast<unsigned int>((static_cast<double>(_count)
        / static_cast<double>(_expected_count)) * 50.0);
    do { m_os << '*' << std::flush; } while ( ++_tic < tics_needed );
    _next_tic_count = 
      static_cast<unsigned long>((_tic/50.0) * static_cast<double>(_expected_count));
    if ( _count == _expected_count ) {
      if ( _tic < 51 ) m_os << '*';
      m_os << std::endl;
      }
  } // display_tic
};

} // namespace boost

#endif  // BOOST_PROGRESS_HPP
