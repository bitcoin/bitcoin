# $Id$

# Try and configure RPC support.
AC_DEFUN(AM_RPC_CONFIGURE, [
	AC_DEFINE(HAVE_RPC)
	AH_TEMPLATE(HAVE_RPC, [Define to 1 if building RPC client/server.])

	# We use the target's rpcgen utility because it may be architecture
	# specific, for example, 32- or 64-bit specific.
	XDR_FILE=$srcdir/../rpc_server/db_server.x

	# Prefer the -C option to rpcgen which generates ANSI C-conformant
	# code.
	RPCGEN="rpcgen -C"
	AC_MSG_CHECKING(["$RPCGEN" build of db_server.h])
	$RPCGEN -h $XDR_FILE > db_server.h 2>/dev/null
	if test $? -ne 0; then
		AC_MSG_RESULT([no])

		# Try rpcgen without the -C option.
		RPCGEN="rpcgen"
		AC_MSG_CHECKING(["$RPCGEN" build of db_server.h])
		$RPCGEN -h $XDR_FILE > db_server.h 2>/dev/null
		if test $? -ne 0; then
			AC_MSG_RESULT([no])
			AC_MSG_ERROR(
			    [Unable to build RPC support: $RPCGEN failed.])
		fi
	fi

	# Some rpcgen programs generate a set of client stubs called something
	# like __db_env_create_4003 and functions on the server to handle the
	# request called something like __db_env_create_4003_svc.  Others
	# expect client and server stubs to both be called __db_env_create_4003.
	#
	# We have to generate code in whichever format rpcgen expects, and the
	# only reliable way to do that is to check what is in the db_server.h
	# file we just created.
	if grep "env_create_[[0-9]]*_svc" db_server.h >/dev/null 2>&1 ; then
		sed 's/__SVCSUFFIX__/_svc/' \
		    < $srcdir/../rpc_server/c/gen_db_server.c > gen_db_server.c
	else
		sed 's/__SVCSUFFIX__//' \
		    < $srcdir/../rpc_server/c/gen_db_server.c > gen_db_server.c
	fi

	AC_MSG_RESULT([yes])

	$RPCGEN -l $XDR_FILE |
	sed -e 's/^#include.*db_server.h.*/#include "db_server.h"/' \
	    -e '1,/^#include/s/^#include/#include "db_config.h"\
&/' > db_server_clnt.c

	$RPCGEN -s tcp $XDR_FILE |
	sed -e 's/^#include.*db_server.h.*/#include "db_server.h"/' \
	    -e 's/^main *()/__dbsrv_main()/' \
	    -e 's/^main *(.*argc.*argv.*)/__dbsrv_main(int argc, char *argv[])/' \
	    -e '/^db_rpc_serverprog/,/^}/{' \
	    -e 's/return;//' \
	    -e 's/^}/__dbsrv_timeout(0);}/' \
	    -e '}' \
	    -e '1,/^#include/s/^#include/#include "db_config.h"\
&/' > db_server_svc.c

	$RPCGEN -c $XDR_FILE |
	sed -e 's/^#include.*db_server.h.*/#include "db_server.h"/' \
	    -e '1,/^#include/s/^#include/#include "db_config.h"\
&/' > db_server_xdr.c

	RPC_SERVER_H=db_server.h
	RPC_CLIENT_OBJS="\$(RPC_CLIENT_OBJS)"
	ADDITIONAL_PROGS="berkeley_db_svc $ADDITIONAL_PROGS"

	# Solaris and HPUX need the nsl library to build RPC.
	AC_CHECK_FUNC(svc_run,,
	    AC_HAVE_LIBRARY(nsl, LIBSO_LIBS="$LIBSO_LIBS -lnsl"))
])
