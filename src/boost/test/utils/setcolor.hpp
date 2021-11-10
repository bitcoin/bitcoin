//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : contains definition for setcolor iostream manipulator
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_SETCOLOR_HPP
#define BOOST_TEST_UTILS_SETCOLOR_HPP

// Boost.Test
#include <boost/test/detail/config.hpp>

#include <boost/core/ignore_unused.hpp>

// STL
#include <iostream>
#include <cstdio>
#include <cassert>

#include <boost/test/detail/suppress_warnings.hpp>

#ifdef _WIN32
  #include <windows.h>

  #if defined(__MINGW32__) && !defined(COMMON_LVB_UNDERSCORE)
    // mingw badly mimicking windows.h
    #define COMMON_LVB_UNDERSCORE 0x8000
  #endif
#endif

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace utils {

// ************************************************************************** //
// **************                    term_attr                 ************** //
// ************************************************************************** //

struct term_attr { enum _ {
    NORMAL    = 0,
    BRIGHT    = 1,
    DIM       = 2,
    UNDERLINE = 4,
    BLINK     = 5,
    REVERSE   = 7,
    CROSSOUT  = 9
}; };

// ************************************************************************** //
// **************                   term_color                 ************** //
// ************************************************************************** //

struct term_color { enum _ {
    BLACK    = 0,
    RED      = 1,
    GREEN    = 2,
    YELLOW   = 3,
    BLUE     = 4,
    MAGENTA  = 5,
    CYAN     = 6,
    WHITE    = 7,
    ORIGINAL = 9
}; };

// ************************************************************************** //
// **************                    setcolor                  ************** //
// ************************************************************************** //

#ifndef _WIN32
class setcolor {
public:
    typedef int state;

    // Constructor
    explicit    setcolor( bool is_color_output = false,
                          term_attr::_  attr = term_attr::NORMAL,
                          term_color::_ fg   = term_color::ORIGINAL,
                          term_color::_ bg   = term_color::ORIGINAL,
                          state* /* unused */= NULL)
    : m_is_color_output(is_color_output)
    {
        m_command_size = std::sprintf( m_control_command, "%c[%c;3%c;4%cm",
          0x1B,
          static_cast<char>(attr + '0'),
          static_cast<char>(fg + '0'),
          static_cast<char>(bg + '0'));
    }

    explicit    setcolor(bool is_color_output,
                         state* /* unused */)
    : m_is_color_output(is_color_output)
    {
        m_command_size = std::sprintf(m_control_command, "%c[%c;3%c;4%cm",
          0x1B,
          static_cast<char>(term_attr::NORMAL + '0'),
          static_cast<char>(term_color::ORIGINAL + '0'),
          static_cast<char>(term_color::ORIGINAL + '0'));
    }

    friend std::ostream&
    operator<<( std::ostream& os, setcolor const& sc )
    {
       if (sc.m_is_color_output && (&os == &std::cout || &os == &std::cerr)) {
          return os.write( sc.m_control_command, sc.m_command_size );
       }
       return os;
    }

private:
    // Data members
    bool        m_is_color_output;
    char        m_control_command[13];
    int         m_command_size;
};

#else

class setcolor {

protected:
  void set_console_color(std::ostream& os, WORD *attributes = NULL) const {
    if (!m_is_color_output || m_state_saved) {
      return;
    }
    DWORD console_type;
    if (&os == &std::cout) {
      console_type = STD_OUTPUT_HANDLE;
    }
    else if (&os == &std::cerr) {
      console_type =  STD_ERROR_HANDLE;
    }
    else {
      return;
    }
    HANDLE hConsole = GetStdHandle(console_type);

    if(hConsole == INVALID_HANDLE_VALUE || hConsole == NULL )
      return;

    state console_attributes;
    if(attributes != NULL || (m_restore_state && m_s)) {
      if (attributes != NULL) {
        console_attributes = *attributes;
      }
      else {
        console_attributes = *m_s;
        *m_s = state();
      }
      SetConsoleTextAttribute(hConsole, console_attributes);
      return;
    }

    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    console_attributes = consoleInfo.wAttributes;

    if (!m_state_saved && m_s) {
      assert(!m_restore_state);
      // we can save the state only the first time this object is used
      // for modifying the console.
      *m_s = console_attributes;
      m_state_saved = true;
    }

    WORD fg_attr = 0;
    switch(m_fg)
    {
    case term_color::WHITE:
      fg_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
      break;
    case term_color::BLACK:
      fg_attr = 0;
      break;
    case term_color::RED:
      fg_attr = FOREGROUND_RED;
      break;
    case term_color::GREEN:
      fg_attr = FOREGROUND_GREEN;
      break;
    case term_color::CYAN:
      fg_attr = FOREGROUND_GREEN | FOREGROUND_BLUE;
      break;
    case term_color::MAGENTA:
      fg_attr = FOREGROUND_RED | FOREGROUND_BLUE;
      break;
    case term_color::BLUE:
      fg_attr = FOREGROUND_BLUE;
      break;
    case term_color::YELLOW:
      fg_attr = FOREGROUND_RED | FOREGROUND_GREEN;
      break;
    case term_color::ORIGINAL:
    default:
      fg_attr = console_attributes & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
      break;
    }

    WORD bg_attr = 0;
    switch(m_bg)
    {
    case term_color::BLACK:
      bg_attr = 0;
      break;
    case term_color::WHITE:
      bg_attr = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
      break;
    case term_color::RED:
      bg_attr = BACKGROUND_RED;
      break;
    case term_color::GREEN:
      bg_attr = BACKGROUND_GREEN;
      break;
    case term_color::BLUE:
      bg_attr = BACKGROUND_BLUE;
      break;
    case term_color::ORIGINAL:
    default:
      bg_attr = console_attributes & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
      break;
    }

    WORD text_attr = 0;
    switch(m_attr)
    {
    case term_attr::BRIGHT:
      text_attr = FOREGROUND_INTENSITY;
      break;
    case term_attr::UNDERLINE:
      text_attr = COMMON_LVB_UNDERSCORE;
      break;
    default:
      break;
    }

    SetConsoleTextAttribute(hConsole, fg_attr | bg_attr | text_attr);
    return;
  }

public:
  typedef WORD state;

  // Constructor
  explicit    setcolor( 
    bool is_color_output = false,
    term_attr::_  attr = term_attr::NORMAL,
    term_color::_ fg   = term_color::ORIGINAL,
    term_color::_ bg   = term_color::ORIGINAL,
    state* s           = NULL)
  : m_is_color_output(is_color_output)
  , m_attr(attr)
  , m_fg(fg)
  , m_bg(bg)
  , m_s(s)
  , m_restore_state(false)
  , m_state_saved(false)
  {}

  explicit    setcolor(
    bool is_color_output,
    state* s)
  : m_is_color_output(is_color_output)
  , m_attr(term_attr::NORMAL)
  , m_fg(term_color::ORIGINAL)
  , m_bg(term_color::ORIGINAL)
  , m_s(s)
  , m_restore_state(true)
  , m_state_saved(false)
  {}

  friend std::ostream&
    operator<<( std::ostream& os, setcolor const& sc )
  {
    sc.set_console_color(os);
    return os;
  }

private:
  bool m_is_color_output;
  term_attr::_ m_attr;
  term_color::_ m_fg;
  term_color::_ m_bg;
  state* m_s;
  // indicates that the instance has been initialized to restore a previously
  // stored state
  bool m_restore_state; 
  // indicates the first time we pull and set the console information.
  mutable bool m_state_saved;
};

#endif
// ************************************************************************** //
// **************                 scope_setcolor               ************** //
// ************************************************************************** //

struct scope_setcolor {
  scope_setcolor() 
  : m_os( 0 )
  , m_state()
  , m_is_color_output(false)
  {}
  
  explicit    scope_setcolor(
    bool is_color_output,
    std::ostream& os,
    term_attr::_  attr = term_attr::NORMAL,
    term_color::_ fg   = term_color::ORIGINAL,
    term_color::_ bg   = term_color::ORIGINAL )
  : m_os( &os )
  , m_is_color_output(is_color_output)
  {
    os << setcolor(is_color_output, attr, fg, bg, &m_state);
  }

  ~scope_setcolor()
  {
    if (m_os) {
      *m_os << setcolor(m_is_color_output, &m_state);
    }
  }
private:
  scope_setcolor(const scope_setcolor& r);
  scope_setcolor& operator=(const scope_setcolor& r);
  // Data members
  std::ostream* m_os;
  setcolor::state m_state;
  bool m_is_color_output;
};


#define BOOST_TEST_SCOPE_SETCOLOR( is_color_output, os, attr, color )               \
    utils::scope_setcolor const sc(is_color_output, os, utils::attr, utils::color); \
    boost::ignore_unused( sc )                                                      \
/**/

} // namespace utils
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_SETCOLOR_HPP
