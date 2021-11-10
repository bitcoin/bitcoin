//  boost/timer/timer.hpp  -------------------------------------------------------------//

//  Copyright Beman Dawes 1994-2007, 2011

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_TIMER_TIMER_HPP                  
#define BOOST_TIMER_TIMER_HPP

#include <boost/timer/config.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <cstring>
#include <ostream>

#include <boost/config/abi_prefix.hpp> // must be the last #include

#   if defined(_MSC_VER)
#     pragma warning(push)           // Save warning settings
#     pragma warning(disable : 4251) // disable warning: class 'std::basic_string<_Elem,_Traits,_Ax>'
#   endif                            // needs to have dll-interface...

//--------------------------------------------------------------------------------------//

namespace boost
{
namespace timer
{
  class cpu_timer;
  class auto_cpu_timer;

  typedef boost::int_least64_t nanosecond_type;

  struct cpu_times
  {
    nanosecond_type wall;
    nanosecond_type user;
    nanosecond_type system;

    void clear() { wall = user = system = 0; }
  };
      
  const short         default_places = 6;

  BOOST_TIMER_DECL
  std::string format(const cpu_times& times, short places, const std::string& format); 

  BOOST_TIMER_DECL
  std::string format(const cpu_times& times, short places = default_places); 

//  cpu_timer  -------------------------------------------------------------------------//

  class BOOST_TIMER_DECL cpu_timer
  {
  public:

    //  constructor
    cpu_timer() BOOST_NOEXCEPT                                   { start(); }

    //  observers
    bool          is_stopped() const BOOST_NOEXCEPT              { return m_is_stopped; }
    cpu_times     elapsed() const BOOST_NOEXCEPT;  // does not stop()
    std::string   format(short places, const std::string& format) const
                        { return ::boost::timer::format(elapsed(), places, format); }
    std::string   format(short places = default_places) const
                        { return ::boost::timer::format(elapsed(), places); }
    //  actions
    void          start() BOOST_NOEXCEPT;
    void          stop() BOOST_NOEXCEPT;
    void          resume() BOOST_NOEXCEPT; 

  private:
    cpu_times     m_times;
    bool          m_is_stopped;
  };

//  auto_cpu_timer  --------------------------------------------------------------------//

  class BOOST_TIMER_DECL auto_cpu_timer : public cpu_timer
  {
  public:

    //  Explicit defaults for os are not provided to avoid including <iostream>, which has
    //  high costs even when the standard streams are not actually used. Explicit defaults
    //  for format are not provided to avoid order-of-dynamic-initialization issues with a
    //  std::string.

    explicit auto_cpu_timer(short places = default_places);                          // #1
             auto_cpu_timer(short places, const std::string& format);                // #2
    explicit auto_cpu_timer(const std::string& format);                              // #3
             auto_cpu_timer(std::ostream& os, short places,
                            const std::string& format)                               // #4
                                   : m_places(places), m_os(&os), m_format(format)
                                   { start(); }
    explicit auto_cpu_timer(std::ostream& os, short places = default_places);        // #5
             auto_cpu_timer(std::ostream& os, const std::string& format)             // #6
                                   : m_places(default_places), m_os(&os), m_format(format)
                                   { start(); }

   ~auto_cpu_timer();

   //  observers
   //    not particularly useful to users, but allow testing of constructor
   //    postconditions and ease specification of other functionality without resorting
   //    to "for exposition only" private members.
   std::ostream&       ostream() const       { return *m_os; }
   short               places() const        { return m_places; }
   const std::string&  format_string() const { return m_format; }

    //  actions
    void   report(); 

  private:
    short           m_places;
    std::ostream*   m_os;      // stored as ptr so compiler can generate operator= 
    std::string     m_format;  
  };
   
} // namespace timer
} // namespace boost

#   if defined(_MSC_VER)
#     pragma warning(pop) // restore warning settings.
#   endif 

#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas

#endif  // BOOST_TIMER_TIMER_HPP
