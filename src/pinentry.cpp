// Copyright (c) 2011 Denis Roio <jaromil@dyne.org>
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"

#ifdef USE_ASSUAN

#include "pinentry.h"

#define SECURE_EXEC_PATH "/usr/bin"


// callbacks
static void *assuan_data_cb_arg;
static void *assuan_inquire_cb_arg;
static void *assuan_status_cb_arg;
gpg_error_t assuan_data_cb(void * ctx, const void * msg, size_t len)
{
  // msg contains input password
  // len contains its length
  //  fprintf(stderr,"data cb: %s (%u) - %s\n", msg, len, (char*)_data_cb_arg);
  Pin *pin = (Pin*)assuan_data_cb_arg;

  if(!pin)
    return GPG_ERR_NO_DATA;
  else
    pin->answer = (char*)msg;
  
  return 0;
}
gpg_error_t assuan_inquire_cb(void* ctx, const char *msg)
{ // nop
  return 0;
}
gpg_error_t assuan_status_cb(void* ctx, const char *msg)
{ // nop
  return 0;
}


Pin::Pin() {
  gpg_error_t res;
  pid_t pid;
  int flags;
  const char *argv[2];

  gpg_err_init();
  res = assuan_new (&ctx);
  if(res)
    throw runtime_error(strprintf("pinentry initialisation: %s", gpg_strerror(res)));

  assuan_set_assuan_log_prefix("Pin: ");

  // needed esp. for ncurses
  lang = strprintf("OPTION lc-ctype=%s",getenv("LANG"));
  tty = strprintf("OPTION ttyname=%s",getenv("TTY"));

  flags = 0x0;
  argv[0] = "bitcoind"; // fake argv
  argv[1] = NULL;
  res = assuan_pipe_connect (ctx, SECURE_EXEC_PATH"/pinentry",
			     argv, NULL, NULL, NULL, flags);
  if(res)
    throw runtime_error(strprintf("pinentry pipe forking: %s", gpg_strerror(res)));

  pid = assuan_get_pid(ctx);
  if(pid == ASSUAN_INVALID_PID)
    throw runtime_error(strprintf("pinentry not running: %s", gpg_strerror(res)));

  cmd(tty.c_str());
  cmd(lang.c_str());
}

Pin::~Pin()
{
  assuan_release(ctx);
}

void Pin::title(string t)
{ 
  string tt = strprintf("SETTITLE %s",t.c_str());
  cmd(tt.c_str());
}
void Pin::description(string d)
{
  string dd = strprintf("SETDESC %s",d.c_str());
  cmd(dd.c_str());
}
void Pin::prompt(string p) 
{
  string pp = strprintf("SETPROMPT %s",p.c_str());
  cmd(pp.c_str());
}

void Pin::ask()
{
  assuan_data_cb_arg = this;
  cmd("GETPIN");
}

void Pin::cmd(const char *command) {
  gpg_error_t res;
  res = assuan_transact(ctx, command,
			&assuan_data_cb, assuan_data_cb_arg,
			&assuan_inquire_cb, assuan_inquire_cb_arg,
			&assuan_status_cb, assuan_status_cb_arg);
  if(res)
    throw runtime_error(strprintf("assuan_transaction: %s", gpg_strerror(res)));
}

#endif
