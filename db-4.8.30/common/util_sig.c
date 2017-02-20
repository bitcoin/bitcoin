/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

static int	interrupt;
static void	set_signal __P((int, int));
static void	signal_handler __P((int));

/*
 * signal_handler --
 *	Interrupt signal handler.
 */
static void
signal_handler(signo)
	int signo;
{
#ifndef HAVE_SIGACTION
	/* Assume signal() is unreliable and reset it, first thing. */
	set_signal(signo, 0);
#endif
	/* Some systems don't pass in the correct signal value -- check. */
	if ((interrupt = signo) == 0)
		interrupt = SIGINT;
}

/*
 * set_signal
 */
static void
set_signal(s, is_dflt)
	int s, is_dflt;
{
	/*
	 * Use sigaction if it's available, otherwise use signal().
	 */
#ifdef HAVE_SIGACTION
	struct sigaction sa, osa;

	sa.sa_handler = is_dflt ? SIG_DFL : signal_handler;
	(void)sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	(void)sigaction(s, &sa, &osa);
#else
	(void)signal(s, is_dflt ? SIG_DFL : signal_handler);
#endif
}

/*
 * __db_util_siginit --
 *
 * PUBLIC: void __db_util_siginit __P((void));
 */
void
__db_util_siginit()
{
	/*
	 * Initialize the set of signals for which we want to clean up.
	 * Generally, we try not to leave the shared regions locked if
	 * we can.
	 */
#ifdef SIGHUP
	set_signal(SIGHUP, 0);
#endif
#ifdef SIGINT
	set_signal(SIGINT, 0);
#endif
#ifdef SIGPIPE
	set_signal(SIGPIPE, 0);
#endif
#ifdef SIGTERM
	set_signal(SIGTERM, 0);
#endif
}

/*
 * __db_util_interrupted --
 *	Return if interrupted.
 *
 * PUBLIC: int __db_util_interrupted __P((void));
 */
int
__db_util_interrupted()
{
	return (interrupt != 0);
}

/*
 * __db_util_sigresend --
 *
 * PUBLIC: void __db_util_sigresend __P((void));
 */
void
__db_util_sigresend()
{
	/* Resend any caught signal. */
	if (interrupt != 0) {
		set_signal(interrupt, 1);

		(void)raise(interrupt);
		/* NOTREACHED */
	}
}
