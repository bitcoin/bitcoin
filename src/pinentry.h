// Copyright (c) 2011 Denis Roio <jaromil@dyne.org>
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"

#ifdef USE_ASSUAN


#ifndef __PINENTRY_H__
#define __PINENTRY_H__


using namespace std;

// Do not include the definitions for the socket wrapper feature.
#define _ASSUAN_NO_SOCKET_WRAPPER

#include <assuan.h>


// actual public class
class Pin {

 public:
  Pin();
  ~Pin();

  void title(string t); ///< set the dialog title
  void description(string d); ///< set the dialog description
  void prompt(string d); ///< set the dialog description
  void ask();
  string answer;
  //  string get_tty();
  //  string set_tty();

 private:
  void cmd(const char *command);

  assuan_context_t ctx;

  string tty;
  string lang;
};

#endif
#endif
