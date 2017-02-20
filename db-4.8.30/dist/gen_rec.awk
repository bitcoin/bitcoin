#!/bin/sh -
#
# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#

# This awk script generates all the log, print, and read routines for the DB
# logging. It also generates a template for the recovery functions (these
# functions must still be edited, but are highly stylized and the initial
# template gets you a fair way along the path).
#
# For a given file prefix.src, we generate a file prefix_auto.c, and a file
# prefix_auto.h that contains:
#
#	external declarations for the file's functions
# 	defines for the physical record types
#	    (logical types are defined in each subsystem manually)
#	structures to contain the data unmarshalled from the log.
#
# This awk script requires that four variables be set when it is called:
#
#	source_file	-- the C source file being created
#	header_file	-- the C #include file being created
#	template_file	-- the template file being created
#
# And stdin must be the input file that defines the recovery setup.
#
# Within each file prefix.src, we use a number of public keywords (documented
# in the reference guide) as well as the following ones which are private to
# DB:
# 	DBPRIVATE	Indicates that a file will be built as part of DB,
#			rather than compiled independently, and so can use
#			DB-private interfaces (such as DB_LOG_NOCOPY).
#	DB		A DB handle.  Logs the dbreg fileid for that handle,
#			and makes the *_log interface take a DB * instead of a
#			DB_ENV *.
#	PGDBT		Just like DBT, only we know it stores a page or page
#			header, so we can byte-swap it (once we write the
#			byte-swapping code, which doesn't exist yet).
#	LOCKS		Just like DBT, but uses a print function for locks.

BEGIN {
	if (source_file == "" ||
	    header_file == "" || template_file == "") {
	    print "Usage: gen_rec.awk requires three variables to be set:"
	    print "\theader_file\t-- the recover #include file being created"
	    print "\tprint_file\t-- the print source file being created"
	    print "\tsource_file\t-- the recover source file being created"
	    print "\ttemplate_file\t-- the template file being created"
	    exit
	}
	FS="[\t ][\t ]*"
	CFILE=source_file
	HFILE=header_file
	PFILE=print_file
	TFILE=template_file

	# These are the variables we use to create code that calls into
	# db routines and/or uses an environment.
	dbprivate = 0
	env_type = "DB_ENV"
	env_var = "dbenv"
	log_call = "dbenv->log_put"

}
/^[ 	]*DBPRIVATE/ {
	dbprivate = 1
	env_type = "ENV"
	env_var = "env"
	log_call = "__log_put"
}
/^[ 	]*PREFIX/ {
	prefix = $2
	num_funcs = 0;

	# Start .c files.
	printf("/* Do not edit: automatically built by gen_rec.awk. */\n\n")\
	    > CFILE
	printf("#include \"db_config.h\"\n") >> CFILE
	if (!dbprivate) {
		printf("#include <errno.h>\n") >> CFILE
		printf("#include <stdlib.h>\n") >> CFILE
		printf("#include <string.h>\n") >> CFILE
		printf("#include \"db.h\"\n") >> CFILE
		printf("#include \"db_int.h\"\n") >> CFILE
		printf("#include \"dbinc/db_swap.h\"\n") >> CFILE
	}

	printf("/* Do not edit: automatically built by gen_rec.awk. */\n\n")\
	    > PFILE
	printf("#include \"db_config.h\"\n\n") >> PFILE
	if (!dbprivate) {
		printf("#include <ctype.h>\n") >> PFILE
		printf("#include <stdio.h>\n") >> PFILE
		printf("#include <stdlib.h>\n") >> PFILE
		printf("#include \"db.h\"\n") >> PFILE
	}

	if (prefix == "__ham")
		printf("#ifdef HAVE_HASH\n") >> PFILE
	if (prefix == "__qam")
		printf("#ifdef HAVE_QUEUE\n") >> PFILE

	# Start .h file, make the entire file conditional.
	printf("/* Do not edit: automatically built by gen_rec.awk. */\n\n")\
	    > HFILE
	printf("#ifndef\t%s_AUTO_H\n#define\t%s_AUTO_H\n", prefix, prefix)\
	    >> HFILE

	# Write recovery template file headers
	if (dbprivate) {
		# This assumes we're doing DB recovery.
		printf("#include \"db_config.h\"\n\n") > TFILE
		printf("#include \"db_int.h\"\n") >> TFILE
		printf("#include \"dbinc/db_page.h\"\n") >> TFILE
		printf("#include \"dbinc/%s.h\"\n", prefix) >> TFILE
		printf("#include \"dbinc/log.h\"\n\n") >> TFILE
	} else {
		printf("#include \"db.h\"\n\n") > TFILE
	}
}
/^[ 	]*INCLUDE/ {
	for (i = 2; i < NF; i++)
		printf("%s ", $i) >> CFILE
	printf("%s\n", $i) >> CFILE
	for (i = 2; i < NF; i++)
		printf("%s ", $i) >> PFILE
	printf("%s\n", $i) >> PFILE
}
/^[ 	]*(BEGIN|BEGIN_COMPAT)/ {
	if (in_begin) {
		print "Invalid format: missing END statement"
		exit
	}
	in_begin = 1;
	is_duplicate = 0;
	is_dbt = 0;
	has_dbp = 0;
	is_uint = 0;
	hdrdbt = "NULL";
	ddbt = "NULL";
	#
	# BEGIN_COMPAT does not need logging function or rec table entry.
	#
	need_log_function = ($1 == "BEGIN");
	is_compat = ($1 == "BEGIN_COMPAT");
	nvars = 0;

	thisfunc = $2;
	dup_thisfunc = $2;
	version = $3;

	rectype = $4;

	make_name(thisfunc, thisfunc, version);
}
/^[	 ]*(DB|ARG|DBT|LOCKS|PGDBT|PGDDBT|POINTER|TIME)/ {
	vars[nvars] = $2;
	types[nvars] = $3;
	modes[nvars] = $1;
	formats[nvars] = $NF;
	for (i = 4; i < NF; i++)
		types[nvars] = sprintf("%s %s", types[nvars], $i);

	if ($1 == "DB") {
		has_dbp = 1;
	}

	if ($1 == "DB" || $1 == "ARG" || $1 == "TIME") {
		sizes[nvars] = sprintf("sizeof(u_int32_t)");
		if ($3 != "u_int32_t")
			is_uint = 1;
	} else if ($1 == "POINTER")
		sizes[nvars] = sprintf("sizeof(*%s)", $2);
	else { # DBT, PGDBT, PGDDBT
		sizes[nvars] =\
		    sprintf("sizeof(u_int32_t) + (%s == NULL ? 0 : %s->size)",\
		    $2, $2);
		is_dbt = 1;
		if ($1 == "PGDBT")
			hdrdbt = vars[nvars];
		else if ($1 == "PGDDBT")
			ddbt = vars[nvars];
	}
	nvars++;
}
/^[	 ]*DUPLICATE/ {
	is_duplicate = 1;
	dup_rectype = $4;
	old_logfunc = logfunc;
	old_funcname = funcname;
	make_name($2, funcname, $3);
	internal_name = sprintf("%s_%s_int", prefix, thisfunc);
	dup_logfunc = logfunc;
	dup_funcname = funcname;
	dup_thisfunc = $2;
	logfunc = old_logfunc;
	funcname = old_funcname;
}
/^[	 ]*END/ {
	if (!in_begin) {
		print "Invalid format: missing BEGIN statement"
		exit;
	}

	# Declare the record type.
	printf("#define\tDB_%s\t%d\n", funcname, rectype) >> HFILE
	if (is_duplicate)
		printf("#define\tDB_%s\t%d\n",\
		    dup_funcname, dup_rectype) >> HFILE

	# Structure declaration.
	printf("typedef struct _%s_args {\n", funcname) >> HFILE

	# Here are the required fields for every structure
	printf("\tu_int32_t type;\n\tDB_TXN *txnp;\n") >> HFILE
	printf("\tDB_LSN prev_lsn;\n") >>HFILE

	# Here are the specified fields.
	for (i = 0; i < nvars; i++) {
		t = types[i];
		if (modes[i] == "POINTER") {
			ndx = index(t, "*");
			t = substr(types[i], 1, ndx - 2);
		}
		printf("\t%s\t%s;\n", t, vars[i]) >> HFILE
	}
	printf("} %s_args;\n\n", funcname) >> HFILE

	# Output the read, log, and print functions (note that we must
	# generate the required read function first, because we use its
	# prototype in the print function).

	read_function();

	if (need_log_function) {
		log_function();
	}
	print_function();

	# Recovery template
	if (dbprivate)
		f = "template/rec_ctemp"
	else
		f = "template/rec_utemp"

	cmd = sprintf(\
    "sed -e s/PREF/%s/ -e s/FUNC/%s/ -e s/DUP/%s/ < template/rec_%s >> %s",
	    prefix, thisfunc, dup_thisfunc,
	    dbprivate ? "ctemp" : "utemp", TFILE)
	system(cmd);

	# Done writing stuff, reset and continue.
	in_begin = 0;
}

END {
	# End the conditional for the HFILE
	printf("#endif\n") >> HFILE

	# Print initialization routine; function prototype
	p[1] = sprintf("int %s_init_print %s%s%s", prefix,
	    "__P((", env_type, " *, DB_DISTAB *));");
	p[2] = "";
	proto_format(p, PFILE);

	# Create the routine to call __db_add_recovery(print_fn, id)
	printf("int\n%s_init_print(%s, dtabp)\n",\
	    prefix, env_var) >> PFILE
	printf("\t%s *%s;\n", env_type, env_var) >> PFILE
	printf("\tDB_DISTAB *dtabp;\n{\n") >> PFILE
	# If application-specific, the user will need a prototype for
	# __db_add_recovery, since they won't have DB's.
	if (!dbprivate) {
		printf(\
		    "\tint __db_add_recovery __P((%s *, DB_DISTAB *,\n",\
		    env_type) >> PFILE
		printf(\
	"\t    int (*)(%s *, DBT *, DB_LSN *, db_recops), u_int32_t));\n",\
		    env_type) >> PFILE
	}

	printf("\tint ret;\n\n") >> PFILE
	for (i = 0; i < num_funcs; i++) {
		if (functable[i] == 1)
			continue;
		printf("\tif ((ret = __db_add_recovery%s(%s, ",\
		    dbprivate ? "_int" : "", env_var) >> PFILE
		printf("dtabp,\n") >> PFILE
		printf("\t    %s_print, DB_%s)) != 0)\n",\
		    dupfuncs[i], funcs[i]) >> PFILE
		printf("\t\treturn (ret);\n") >> PFILE
	}
	printf("\treturn (0);\n}\n") >> PFILE
	if (prefix == "__ham")
		printf("#endif /* HAVE_HASH */\n") >> PFILE
	if (prefix == "__qam")
		printf("#endif /* HAVE_QUEUE */\n") >> PFILE

	# We only want to generate *_init_recover functions if this is a
	# DB-private, rather than application-specific, set of recovery
	# functions.  Application-specific recovery functions should be
	# dispatched using the DB_ENV->set_app_dispatch callback rather than
	# a DB dispatch table ("dtab").
	if (!dbprivate)
		exit
	# Everything below here is dbprivate, so it uses ENV instead of DB_ENV
	# Recover initialization routine
	p[1] = sprintf("int %s_init_recover %s", prefix,\
	    "__P((ENV *, DB_DISTAB *));");
	p[2] = "";
	proto_format(p, CFILE);

	# Create the routine to call db_add_recovery(func, id)
	printf("int\n%s_init_recover(env, dtabp)\n", prefix) >> CFILE
	printf("\tENV *env;\n") >> CFILE
	printf("\tDB_DISTAB *dtabp;\n{\n") >> CFILE
	printf("\tint ret;\n\n") >> CFILE
	for (i = 0; i < num_funcs; i++) {
		if (functable[i] == 1)
			continue;
		printf("\tif ((ret = __db_add_recovery_int(env, ") >> CFILE
		printf("dtabp,\n") >> CFILE
		printf("\t    %s_recover, DB_%s)) != 0)\n",\
		    funcs[i], funcs[i]) >> CFILE
		printf("\t\treturn (ret);\n") >> CFILE
	}
	printf("\treturn (0);\n}\n") >> CFILE
}

function log_function()
{
	log_prototype(logfunc, 0);
	if (is_duplicate) {
		log_prototype(dup_logfunc, 0);
		log_prototype(internal_name, 1);
	}

	# Function declaration
	log_funcdecl(logfunc, 0);
	if (is_duplicate) {
		log_callint(funcname);
		log_funcdecl(dup_logfunc, 0);
		log_callint(dup_funcname);
		log_funcdecl(internal_name, 1);
	}

	# Function body and local decls
	printf("{\n") >> CFILE
	printf("\tDBT logrec;\n") >> CFILE
	printf("\tDB_LSN *lsnp, null_lsn, *rlsnp;\n") >> CFILE
	if (dbprivate) {
		printf("\tDB_TXNLOGREC *lr;\n") >> CFILE
		if (has_dbp)
			printf("\t%s *%s;\n",
			    env_type, env_var) >> CFILE
	} else {
		printf("\tENV *env;\n") >> CFILE
	}
	printf("\tu_int32_t ") >> CFILE
	if (is_dbt)
		printf("zero, ") >> CFILE
	if (is_uint)
		printf("uinttmp, ") >> CFILE
	printf("rectype, txn_num;\n") >> CFILE
	printf("\tu_int npad;\n") >> CFILE
	printf("\tu_int8_t *bp;\n") >> CFILE
	printf("\tint ") >> CFILE
	if (dbprivate) {
		printf("is_durable, ") >> CFILE
	}
	printf("ret;\n\n") >> CFILE

	# Initialization
	if (dbprivate)
		printf("\tCOMPQUIET(lr, NULL);\n\n") >> CFILE

	if (has_dbp)
		printf("\t%s = dbp->%s;\n", env_var, env_var) >> CFILE
	if (!dbprivate)
		printf("\tenv = dbenv->env;\n") >> CFILE
	printf("\trlsnp = ret_lsnp;\n") >> CFILE
	if (is_duplicate)
		printf("\trectype = type;\n") >> CFILE
	else
		printf("\trectype = DB_%s;\n", funcname) >> CFILE
	printf("\tnpad = 0;\n") >> CFILE
	printf("\tret = 0;\n\n") >> CFILE

	if (dbprivate) {
		printf("\tif (LF_ISSET(DB_LOG_NOT_DURABLE)") >> CFILE
		if (has_dbp) {
			printf(" ||\n\t    ") >> CFILE
			printf("F_ISSET(dbp, DB_AM_NOT_DURABLE)) {\n") >> CFILE
		} else {
			printf(") {\n") >> CFILE
		}
		printf("\t\tif (txnp == NULL)\n") >> CFILE
		printf("\t\t\treturn (0);\n") >> CFILE
		printf("\t\tis_durable = 0;\n") >> CFILE
		printf("\t} else\n") >> CFILE
		printf("\t\tis_durable = 1;\n\n") >> CFILE
	}
	printf("\tif (txnp == NULL) {\n") >> CFILE
	printf("\t\ttxn_num = 0;\n") >> CFILE
	printf("\t\tlsnp = &null_lsn;\n") >> CFILE
	printf("\t\tnull_lsn.file = null_lsn.offset = 0;\n") >> CFILE
	printf("\t} else {\n") >> CFILE
	if (dbprivate && logfunc != "__db_debug") {
		printf(\
    "\t\tif (TAILQ_FIRST(&txnp->kids) != NULL &&\n") >> CFILE
		printf("\t\t    (ret = __txn_activekids(") >> CFILE
		printf("env, rectype, txnp)) != 0)\n") >> CFILE
		printf("\t\t\treturn (ret);\n") >> CFILE
	}
	printf("\t\t/*\n\t\t * We need to assign begin_lsn while ") >> CFILE
	printf("holding region mutex.\n") >> CFILE
	printf("\t\t * That assignment is done inside the ") >> CFILE
	printf("DbEnv->log_put call,\n\t\t * ") >> CFILE
	printf("so pass in the appropriate memory location to be ") >> CFILE
	printf("filled\n\t\t * in by the log_put code.\n\t\t */\n")  >> CFILE
	printf("\t\tDB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);\n") >> CFILE
	printf("\t\ttxn_num = txnp->txnid;\n") >> CFILE
	printf("\t}\n\n") >> CFILE

	# If we're logging a DB handle, make sure we have a log
	# file ID for it.
	db_handle_id_function(modes, nvars);

	# Malloc
	printf("\tlogrec.size = ") >> CFILE
	printf("sizeof(rectype) + ") >> CFILE
	printf("sizeof(txn_num) + sizeof(DB_LSN)") >> CFILE
	for (i = 0; i < nvars; i++)
		printf("\n\t    + %s", sizes[i]) >> CFILE
	printf(";\n") >> CFILE
	if (dbprivate) {
		printf("\tif (CRYPTO_ON(env)) {\n") >> CFILE
		printf(\
	"\t\tnpad = env->crypto_handle->adj_size(logrec.size);\n") >> CFILE
		printf("\t\tlogrec.size += npad;\n\t}\n\n") >> CFILE

		printf("\tif (is_durable || txnp == NULL) {\n") >> CFILE
		printf("\t\tif ((ret =\n\t\t    __os_malloc(env, ") >> CFILE
		printf("logrec.size, &logrec.data)) != 0)\n") >> CFILE
		printf("\t\t\treturn (ret);\n") >> CFILE
		printf("\t} else {\n") >> CFILE
		write_malloc("\t\t",
		    "lr", "logrec.size + sizeof(DB_TXNLOGREC)", CFILE)
		printf("#ifdef DIAGNOSTIC\n") >> CFILE
		printf("\t\tif ((ret =\n\t\t    __os_malloc(env, ") >> CFILE
		printf("logrec.size, &logrec.data)) != 0) {\n") >> CFILE
		printf("\t\t\t__os_free(env, lr);\n") >> CFILE
		printf("\t\t\treturn (ret);\n") >> CFILE
		printf("\t\t}\n") >> CFILE
		printf("#else\n") >> CFILE
		printf("\t\tlogrec.data = lr->data;\n") >> CFILE
		printf("#endif\n") >> CFILE
		printf("\t}\n") >> CFILE
	} else {
		write_malloc("\t", "logrec.data", "logrec.size", CFILE)
		printf("\tbp = logrec.data;\n\n") >> CFILE
	}
	printf("\tif (npad > 0)\n") >> CFILE
	printf("\t\tmemset((u_int8_t *)logrec.data + logrec.size ") >> CFILE
	printf("- npad, 0, npad);\n\n") >> CFILE
	printf("\tbp = logrec.data;\n\n") >> CFILE

	# Copy args into buffer
	printf("\tLOGCOPY_32(env, bp, &rectype);\n") >> CFILE
	printf("\tbp += sizeof(rectype);\n\n") >> CFILE
	printf("\tLOGCOPY_32(env, bp, &txn_num);\n") >> CFILE
	printf("\tbp += sizeof(txn_num);\n\n") >> CFILE
	printf("\tLOGCOPY_FROMLSN(env, bp, lsnp);\n") >> CFILE
	printf("\tbp += sizeof(DB_LSN);\n\n") >> CFILE

	for (i = 0; i < nvars; i++) {
		if (modes[i] == "ARG" || modes[i] == "TIME") {
			if (types[i] == "u_int32_t") {
				printf("\tLOGCOPY_32(env, bp, &%s);\n",
				    vars[i]) >> CFILE
				printf("\tbp += sizeof(%s);\n\n",
				    vars[i]) >> CFILE
			} else {
				printf("\tuinttmp = (u_int32_t)%s;\n",
				    vars[i]) >> CFILE
				printf("\tLOGCOPY_32(env,") >> CFILE
				printf("bp, &uinttmp);\n") >> CFILE
				printf("\tbp += sizeof(uinttmp);\n\n") >> CFILE
			}
		} else if (modes[i] == "DBT" || modes[i] == "LOCKS" ||\
		    modes[i] == "PGDBT" || modes[i] == "PGDDBT") {
			printf("\tif (%s == NULL) {\n", vars[i]) >> CFILE
			printf("\t\tzero = 0;\n") >> CFILE
			printf("\t\tLOGCOPY_32(env, bp, &zero);\n") >> CFILE
			printf("\t\tbp += sizeof(u_int32_t);\n") >> CFILE
			printf("\t} else {\n") >> CFILE
			printf("\t\tLOGCOPY_32(env, bp, &%s->size);\n",
			    vars[i]) >> CFILE
			printf("\t\tbp += sizeof(%s->size);\n", vars[i])\
			    >> CFILE
			printf("\t\tmemcpy(bp, %s->data, %s->size);\n",\
			    vars[i], vars[i]) >> CFILE
			if (modes[i] == "PGDBT" && !is_compat) {
				printf("\t\tif (LOG_SWAPPED(env))\n") >> CFILE
				printf(\
			"\t\t\tif ((ret = __db_pageswap(dbp,\n") >> CFILE
				printf(\
	       "\t\t\t    (PAGE *)bp, (size_t)%s->size, (DBT *)%s, 0)) != 0)\n",
				    vars[i], ddbt) >> CFILE
				printf("\t\t\t\treturn (ret);\n") >> CFILE
			} else if (modes[i] == "PGDDBT") {
				printf("\t\tif (LOG_SWAPPED(env) && ") >> CFILE
				printf("F_ISSET(%s, DB_DBT_APPMALLOC))\n",
				    ddbt) >> CFILE
				printf("\t\t\t__os_free(env, %s->data);\n",
				    ddbt) >> CFILE
			}
			printf("\t\tbp += %s->size;\n", vars[i]) >> CFILE
			printf("\t}\n\n") >> CFILE
		} else if (modes[i] == "DB") {
			printf(\
			    "\tuinttmp = (u_int32_t)dbp->log_filename->id;\n")\
			    >> CFILE
			printf("\tLOGCOPY_32(env, bp, &uinttmp);\n") >> CFILE
			printf("\tbp += sizeof(uinttmp);\n\n") >> CFILE
		} else { # POINTER
			printf("\tif (%s != NULL)", vars[i]) >> CFILE
			if (has_dbp && types[i] == "DB_LSN *") {
				printf(" {\n\t\tif (txnp != NULL) {\n")\
				    >> CFILE
				printf(\
	"\t\t\tLOG *lp = env->lg_handle->reginfo.primary;\n") >> CFILE
				printf(\
	"\t\t\tif (LOG_COMPARE(%s, &lp->lsn) >= 0 && (ret =\n", vars[i])\
				    >> CFILE
				printf(\
	"\t\t\t    __log_check_page_lsn(env, dbp, %s)) != 0)\n", vars[i])\
				    >> CFILE
				printf("\t\t\t\treturn (ret);\n") >> CFILE
				printf("\t\t}") >> CFILE
			}
			printf("\n\t\tLOGCOPY_FROMLSN(env, bp, %s);\n",
			    vars[i]) >> CFILE
			if (has_dbp && types[i] == "DB_LSN *")
				printf("\t} else\n") >> CFILE
			else
				printf("\telse\n") >> CFILE
			printf("\t\tmemset(bp, 0, %s);\n", sizes[i]) >> CFILE
			printf("\tbp += %s;\n\n", sizes[i]) >> CFILE
		}
	}

	# Error checking.  User code won't have DB_ASSERT available, but
	# this is a pretty unlikely assertion anyway, so we just leave it out
	# rather than requiring assert.h.
	if (dbprivate) {
		printf("\tDB_ASSERT(env,\n") >> CFILE
		printf("\t    (u_int32_t)(bp - (u_int8_t *)") >> CFILE
		printf("logrec.data) <= logrec.size);\n\n") >> CFILE
		# Save the log record off in the txn's linked list,
		# or do log call.
		# We didn't call the crypto alignment function when
		# we created this log record (because we don't have
		# the right header files to find the function), so
		# we have to copy the log record to make sure the
		# alignment is correct.
		printf("\tif (is_durable || txnp == NULL) {\n") >> CFILE
		# Output the log record and update the return LSN.
		printf("\t\tif ((ret = __log_put(env, rlsnp,") >> CFILE
		printf("(DBT *)&logrec,\n") >> CFILE
		printf("\t\t    flags | DB_LOG_NOCOPY)) == 0") >> CFILE
		printf(" && txnp != NULL) {\n") >> CFILE
		printf("\t\t\t*lsnp = *rlsnp;\n") >> CFILE

		printf("\t\t\tif (rlsnp != ret_lsnp)\n") >> CFILE
		printf("\t\t\t\t *ret_lsnp = *rlsnp;\n") >> CFILE
		printf("\t\t}\n\t} else {\n") >> CFILE
		printf("\t\tret = 0;\n") >> CFILE
		printf("#ifdef DIAGNOSTIC\n") >> CFILE

		# Add the debug bit if we are logging a ND record.
		printf("\t\t/*\n") >> CFILE
		printf("\t\t * Set the debug bit if we are") >> CFILE
		printf(" going to log non-durable\n") >> CFILE
		printf("\t\t * transactions so they will be ignored") >> CFILE
		printf(" by recovery.\n") >> CFILE
		printf("\t\t */\n") >> CFILE
		printf("\t\tmemcpy(lr->data, logrec.data, ") >> CFILE
		printf("logrec.size);\n") >> CFILE
		printf("\t\trectype |= DB_debug_FLAG;\n") >> CFILE
		printf("\t\tLOGCOPY_32(") >> CFILE
		printf("env, logrec.data, &rectype);\n\n") >> CFILE
		# Output the log record.
		printf("\t\tif (!IS_REP_CLIENT(env))\n") >> CFILE
		printf("\t\t\tret = __log_put(env,\n") >> CFILE
		printf("\t\t\t    rlsnp, (DBT *)&logrec, ") >> CFILE
		printf("flags | DB_LOG_NOCOPY);\n") >> CFILE
		printf("#endif\n") >> CFILE
		# Add a ND record to the txn list.
		printf("\t\tSTAILQ_INSERT_HEAD(&txnp") >> CFILE
		printf("->logs, lr, links);\n") >> CFILE
		printf("\t\tF_SET((TXN_DETAIL *)") >> CFILE
		printf("txnp->td, TXN_DTL_INMEMORY);\n") >> CFILE
		# Update the return LSN.
		printf("\t\tLSN_NOT_LOGGED(*ret_lsnp);\n") >> CFILE
		printf("\t}\n\n") >> CFILE
	} else {
		printf("\tif ((ret = dbenv->log_put(dbenv, rlsnp,") >> CFILE
		printf(" (DBT *)&logrec,\n") >> CFILE
		printf("\t    flags | DB_LOG_NOCOPY)) == 0") >> CFILE
		printf(" && txnp != NULL) {\n") >> CFILE

               	# Update the transactions last_lsn.
		printf("\t\t*lsnp = *rlsnp;\n") >> CFILE
		printf("\t\tif (rlsnp != ret_lsnp)\n") >> CFILE
		printf("\t\t\t *ret_lsnp = *rlsnp;\n") >> CFILE
		printf("\t}\n") >> CFILE
	}

	# If out of disk space log writes may fail.  If we are debugging
	# that print out which records did not make it to disk.
	printf("#ifdef LOG_DIAGNOSTIC\n") >> CFILE
	printf("\tif (ret != 0)\n") >> CFILE
	printf("\t\t(void)%s_print(%s,\n", funcname, env_var) >> CFILE
	printf("\t\t    (DBT *)&logrec, ret_lsnp, ") >> CFILE
	printf("DB_TXN_PRINT%s);\n#endif\n\n",\
	    dbprivate ? ", NULL" : "") >> CFILE
	# Free and return
	if (dbprivate) {
		printf("#ifdef DIAGNOSTIC\n") >> CFILE
		write_free("\t", "logrec.data", CFILE)
		printf("#else\n") >> CFILE
		printf("\tif (is_durable || txnp == NULL)\n") >> CFILE
		write_free("\t\t", "logrec.data", CFILE)
		printf("#endif\n") >> CFILE
	} else {
		write_free("\t", "logrec.data", CFILE)
	}

	printf("\treturn (ret);\n}\n\n") >> CFILE
}

function log_prototype(fname, with_type)
{
	# Write the log function; function prototype
	pi = 1;
	p[pi++] = sprintf("int %s_log", fname);
	p[pi++] = " ";
	if (has_dbp) {
		p[pi++] = "__P((DB *";
	} else {
		p[pi++] = sprintf("__P((%s *", env_type);
	}
	p[pi++] = ", DB_TXN *, DB_LSN *, u_int32_t";
	for (i = 0; i < nvars; i++) {
		if (modes[i] == "DB")
			continue;
		p[pi++] = ", ";
		p[pi++] = sprintf("%s%s%s", (modes[i] == "DBT" ||
		    modes[i] == "LOCKS" || modes[i] == "PGDBT" ||
		    modes[i] == "PGDDBT") ? "const " : "", types[i],
		    (modes[i] == "DBT" || modes[i] == "LOCKS" ||
		    modes[i] == "PGDBT" || modes[i] == "PGDDBT") ? " *" : "");
	}

	# If this is a logging call with type, add the type here.
	if (with_type)
		p[pi++] = ", u_int32_t";

	p[pi++] = "";
	p[pi++] = "));";
	p[pi++] = "";
	proto_format(p, CFILE);
}

# If we're logging a DB handle, make sure we have a log
# file ID for it.
function db_handle_id_function(modes, n)
{
	for (i = 0; i < n; i++)
		if (modes[i] == "DB") {
			# We actually log the DB handle's fileid; from
			# that ID we're able to acquire an open handle
			# at recovery time.
			printf(\
		    "\tDB_ASSERT(env, dbp->log_filename != NULL);\n")\
			    >> CFILE
			printf("\tif (dbp->log_filename->id == ")\
			    >> CFILE
			printf("DB_LOGFILEID_INVALID &&\n\t    ")\
			    >> CFILE
			printf("(ret = __dbreg_lazy_id(dbp)) != 0)\n")\
			    >> CFILE
			printf("\t\treturn (ret);\n\n") >> CFILE
			break;
		}
}

function print_function()
{
	# Write the print function; function prototype
	p[1] = sprintf("int %s_print", funcname);
	p[2] = " ";
	if (dbprivate)
		p[3] = "__P((ENV *, DBT *, DB_LSN *, db_recops, void *));";
	else
		p[3] = "__P((DB_ENV *, DBT *, DB_LSN *, db_recops));";
	p[4] = "";
	proto_format(p, PFILE);

	# Function declaration
	printf("int\n%s_print(%s, ", funcname, env_var) >> PFILE
	printf("dbtp, lsnp, notused2") >> PFILE
	if (dbprivate)
		printf(", notused3") >> PFILE
	printf(")\n") >> PFILE
	printf("\t%s *%s;\n", env_type, env_var) >> PFILE
	printf("\tDBT *dbtp;\n") >> PFILE
	printf("\tDB_LSN *lsnp;\n") >> PFILE
	printf("\tdb_recops notused2;\n") >> PFILE
	if (dbprivate)
		printf("\tvoid *notused3;\n") >> PFILE
	printf("{\n") >> PFILE

	# Locals
	printf("\t%s_args *argp;\n", funcname) >> PFILE
	if (!dbprivate)
		printf("\t%s\n", read_proto) >> PFILE
	for (i = 0; i < nvars; i ++)
		if (modes[i] == "TIME") {
			printf("\tstruct tm *lt;\n") >> PFILE
			printf("\ttime_t timeval;\n") >> PFILE
			printf("\tchar time_buf[CTIME_BUFLEN];\n") >> PFILE
			break;
		}
	for (i = 0; i < nvars; i ++)
		if (modes[i] == "DBT" ||
		    modes[i] == "PGDBT" || modes[i] == "PGDDBT") {
			printf("\tu_int32_t i;\n") >> PFILE
			printf("\tint ch;\n") >> PFILE
			break;
		}
	printf("\tint ret;\n\n") >> PFILE

	# Get rid of complaints about unused parameters.
	printf("\tnotused2 = DB_TXN_PRINT;\n") >> PFILE
	if (dbprivate)
		printf("\tnotused3 = NULL;\n") >> PFILE
	printf("\n") >> PFILE

	# Call read routine to initialize structure
	if (has_dbp) {
		printf("\tif ((ret =\n") >> PFILE
		printf(\
		    "\t    %s_read(%s, NULL, NULL, dbtp->data, &argp)) != 0)\n",
		    funcname, env_var) >> PFILE
	} else {
		printf("\tif ((ret = %s_read(%s, dbtp->data, &argp)) != 0)\n",
		    funcname, env_var) >> PFILE
	}
	printf("\t\treturn (ret);\n") >> PFILE

	# Print values in every record
	printf("\t(void)printf(\n    \"[%%lu][%%lu]%s%%s: ", funcname) >> PFILE
	printf("rec: %%lu txnp %%lx prevlsn [%%lu][%%lu]\\n\",\n") >> PFILE
	printf("\t    (u_long)lsnp->file, (u_long)lsnp->offset,\n") >> PFILE
	printf("\t    (argp->type & DB_debug_FLAG) ? \"_debug\" : \"\",\n")\
	     >> PFILE
	printf("\t    (u_long)argp->type,\n") >> PFILE
	printf("\t    (u_long)argp->txnp->txnid,\n") >> PFILE
	printf("\t    (u_long)argp->prev_lsn.file, ") >> PFILE
	printf("(u_long)argp->prev_lsn.offset);\n") >> PFILE

	# Now print fields of argp
	for (i = 0; i < nvars; i ++) {
		if (modes[i] == "TIME") {
			printf("\ttimeval = (time_t)argp->%s;\n",
			    vars[i]) >> PFILE
			printf("\tlt = localtime(&timeval);\n") >> PFILE
			printf("\t(void)printf(\n\t    \"\\t%s: ",
			    vars[i]) >> PFILE
		} else
			printf("\t(void)printf(\"\\t%s: ", vars[i]) >> PFILE

		if (modes[i] == "DBT" ||
		    modes[i] == "PGDBT" || modes[i] == "PGDDBT") {
			printf("\");\n") >> PFILE
			printf("\tfor (i = 0; i < ") >> PFILE
			printf("argp->%s.size; i++) {\n", vars[i]) >> PFILE
			printf("\t\tch = ((u_int8_t *)argp->%s.data)[i];\n",\
			    vars[i]) >> PFILE
			printf("\t\tprintf(isprint(ch) || ch == 0x0a") >> PFILE
			printf(" ? \"%%c\" : \"%%#x \", ch);\n") >> PFILE
			printf("\t}\n\t(void)printf(\"\\n\");\n") >> PFILE
		} else if (types[i] == "DB_LSN *") {
			printf("[%%%s][%%%s]\\n\",\n",\
			    formats[i], formats[i]) >> PFILE
			printf("\t    (u_long)argp->%s.file,",\
			    vars[i]) >> PFILE
			printf(" (u_long)argp->%s.offset);\n",\
			    vars[i]) >> PFILE
		} else if (modes[i] == "TIME") {
			# Time values are displayed in two ways: the standard
			# string returned by ctime, and in the input format
			# expected by db_recover -t.
			printf(\
	    "%%%s (%%.24s, 20%%02lu%%02lu%%02lu%%02lu%%02lu.%%02lu)\\n\",\n",\
			    formats[i]) >> PFILE
			printf("\t    (long)argp->%s, ", vars[i]) >> PFILE
			printf("__os_ctime(&timeval, time_buf),",\
			    vars[i]) >> PFILE
			printf("\n\t    (u_long)lt->tm_year - 100, ") >> PFILE
			printf("(u_long)lt->tm_mon+1,") >> PFILE
			printf("\n\t    (u_long)lt->tm_mday, ") >> PFILE
			printf("(u_long)lt->tm_hour,") >> PFILE
			printf("\n\t    (u_long)lt->tm_min, ") >> PFILE
			printf("(u_long)lt->tm_sec);\n") >> PFILE
		} else if (modes[i] == "LOCKS") {
			printf("\\n\");\n") >> PFILE
			printf("\t__lock_list_print(env, &argp->locks);\n")\
				>> PFILE
		} else {
			if (formats[i] == "lx")
				printf("0x") >> PFILE
			printf("%%%s\\n\", ", formats[i]) >> PFILE
			if (formats[i] == "lx" || formats[i] == "lu")
				printf("(u_long)") >> PFILE
			if (formats[i] == "ld")
				printf("(long)") >> PFILE
			printf("argp->%s);\n", vars[i]) >> PFILE
		}
	}
	printf("\t(void)printf(\"\\n\");\n") >> PFILE
	write_free("\t", "argp", PFILE);
	printf("\treturn (0);\n") >> PFILE
	printf("}\n\n") >> PFILE
}

function read_function()
{
	# Write the read function; function prototype
	if (has_dbp) {
		p[1] = sprintf("int %s_read __P((%s *, DB **, void *, void *,",
		    funcname, env_type);
	} else {
		p[1] = sprintf("int %s_read __P((%s *, void *,",
		    funcname, env_type);
	}
	p[2] = " ";
	p[3] = sprintf("%s_args **));", funcname);
	p[4] = "";
	read_proto = sprintf("%s %s", p[1], p[3]);
	proto_format(p, CFILE);

	# Function declaration
	if (has_dbp) {
		printf("int\n%s_read(%s, dbpp, td, recbuf, argpp)\n",
		    funcname, env_var) >> CFILE
	} else {
		printf("int\n%s_read(%s, recbuf, argpp)\n",
		    funcname, env_var) >> CFILE
	}

	# Now print the parameters
	printf("\t%s *%s;\n", env_type, env_var) >> CFILE
	if (has_dbp) {
		printf("\tDB **dbpp;\n") >> CFILE
		printf("\tvoid *td;\n") >> CFILE
	}
	printf("\tvoid *recbuf;\n") >> CFILE
	printf("\t%s_args **argpp;\n", funcname) >> CFILE

	# Function body and local decls
	printf("{\n\t%s_args *argp;\n", funcname) >> CFILE
	if (is_uint)
		printf("\tu_int32_t uinttmp;\n") >> CFILE
	printf("\tu_int8_t *bp;\n") >> CFILE

	if (dbprivate) {
		# We only use ret in the private malloc case.
		printf("\tint ret;\n\n") >> CFILE
	} else {
		printf("\tENV *env;\n\n") >> CFILE
		printf("\tenv = dbenv->env;\n\n") >> CFILE
	}

	malloc_size = sprintf("sizeof(%s_args) + sizeof(DB_TXN)", funcname)
	write_malloc("\t", "argp", malloc_size, CFILE)

	# Set up the pointers to the DB_TXN *.
	printf("\tbp = recbuf;\n") >> CFILE

	printf("\targp->txnp = (DB_TXN *)&argp[1];\n") >> CFILE
	printf("\tmemset(argp->txnp, 0, sizeof(DB_TXN));\n\n")\
	    >> CFILE
	if (has_dbp)
		printf("\targp->txnp->td = td;\n") >> CFILE

	# First get the record type, prev_lsn, and txnp fields.
	printf("\tLOGCOPY_32(env, &argp->type, bp);\n") >> CFILE
	printf("\tbp += sizeof(argp->type);\n\n") >> CFILE
	printf("\tLOGCOPY_32(env, &argp->txnp->txnid, bp);\n") >> CFILE
	printf("\tbp += sizeof(argp->txnp->txnid);\n\n") >> CFILE
	printf("\tLOGCOPY_TOLSN(env, &argp->prev_lsn, bp);\n") >> CFILE
	printf("\tbp += sizeof(DB_LSN);\n\n") >> CFILE

	# Now get rest of data.
	for (i = 0; i < nvars; i ++) {
		if (modes[i] == "DBT" || modes[i] == "LOCKS" ||
		    modes[i] == "PGDBT" || modes[i] == "PGDDBT") {
			printf("\tmemset(&argp->%s, 0, sizeof(argp->%s));\n",\
			    vars[i], vars[i]) >> CFILE
			printf("\tLOGCOPY_32(env,") >> CFILE
			printf("&argp->%s.size, bp);\n", vars[i]) >> CFILE
			printf("\tbp += sizeof(u_int32_t);\n") >> CFILE
			printf("\targp->%s.data = bp;\n", vars[i]) >> CFILE
			printf("\tbp += argp->%s.size;\n", vars[i]) >> CFILE
			if (modes[i] == "PGDBT" && ddbt == "NULL" &&
			    !is_compat) {
				printf("\tif (LOG_SWAPPED(env) && ") >> CFILE
				printf("dbpp != NULL && ") >> CFILE
				printf("*dbpp != NULL) {\n") >> CFILE
				printf("\t\tint t_ret;\n") >> CFILE
				printf(\
	"\t\tif ((t_ret = __db_pageswap(*dbpp, (PAGE *)argp->%s.data,\n",
				    vars[i]) >> CFILE
				printf(\
		      "\t\t    (size_t)argp->%s.size, NULL, 1)) != 0)\n",
				    vars[i]) >> CFILE
				printf("\t\t\treturn (t_ret);\n") >> CFILE
				printf("\t}\n") >> CFILE
			} else if (modes[i] == "PGDDBT" && !is_compat) {
				printf("\tif (LOG_SWAPPED(env) && ") >> CFILE
				printf("dbpp != NULL && ") >> CFILE
				printf("*dbpp != NULL) {\n") >> CFILE
				printf("\t\tint t_ret;\n") >> CFILE
				printf(\
	"\t\tif ((t_ret = __db_pageswap(*dbpp,\n") >> CFILE
				printf(\
		      "\t\t    (PAGE *)argp->%s.data, (size_t)argp->%s.size,\n",
				     hdrdbt, hdrdbt) >> CFILE
				printf("\t\t    &argp->%s, 1)) != 0)\n",
				    vars[i]) >> CFILE
				printf("\t\t\treturn (t_ret);\n") >> CFILE
				printf("\t}\n") >> CFILE
			}
		} else if (modes[i] == "ARG" || modes[i] == "TIME" ||
		    modes[i] == "DB") {
			if (types[i] == "u_int32_t") {
				printf("\tLOGCOPY_32(env, &argp->%s, bp);\n",
				    vars[i]) >> CFILE
				printf("\tbp += sizeof(argp->%s);\n", vars[i])\
				    >> CFILE
			} else {
				printf("\tLOGCOPY_32(env, &uinttmp, bp);\n")\
				    >> CFILE
				printf("\targp->%s = (%s)uinttmp;\n", vars[i],\
				    types[i]) >> CFILE
				printf("\tbp += sizeof(uinttmp);\n") >> CFILE
			}

			if (modes[i] == "DB") {
				# We can now get the DB handle.
				printf("\tif (dbpp != NULL) {\n") >> CFILE
				printf("\t\t*dbpp = NULL;\n") >> CFILE
				printf(\
			"\t\tret = __dbreg_id_to_db(\n\t\t    ") >> CFILE
				printf("env, argp->txnp, dbpp, argp->%s, 1);\n",
				    vars[i]) >> CFILE
				printf("\t}\n") >> CFILE
			}
		} else if (types[i] == "DB_LSN *") {
			printf("\tLOGCOPY_TOLSN(env, &argp->%s, bp);\n",
			    vars[i]) >> CFILE
			printf("\tbp += sizeof(DB_LSN);\n", vars[i]) >> CFILE
		} else { # POINTER
			printf("\tDB_ASSERT(env, sizeof(argp->%s) == 4);",
			    vars[i]) >> CFILE
			printf("\tLOGCOPY_32(env, &argp->%s, bp);",
			    vars[i]) >> CFILE
			printf("\tbp += sizeof(argp->%s);\n", vars[i]) >> CFILE
		}
		printf("\n") >> CFILE
	}

	printf("\t*argpp = argp;\n") >> CFILE
	if (dbprivate)
		printf("\treturn (ret);\n}\n\n") >> CFILE
	else
		printf("\treturn (0);\n}\n\n") >> CFILE
}

# proto_format --
#	Pretty-print a function prototype.
function proto_format(p, fp)
{
	printf("/*\n") >> fp;

	s = "";
	for (i = 1; i in p; ++i)
		s = s p[i];

	t = " * PUBLIC: "
	if (length(s) + length(t) < 80)
		printf("%s%s", t, s) >> fp;
	else {
		split(s, p, "__P");
		len = length(t) + length(p[1]);
		printf("%s%s", t, p[1]) >> fp

		n = split(p[2], comma, ",");
		comma[1] = "__P" comma[1];
		for (i = 1; i <= n; i++) {
			if (len + length(comma[i]) > 70) {
				printf("\n * PUBLIC:    ") >> fp;
				len = 0;
			}
			printf("%s%s", comma[i], i == n ? "" : ",") >> fp;
			len += length(comma[i]) + 2;
		}
	}
	printf("\n */\n") >> fp;
	delete p;
}

function write_malloc(tab, ptr, size, file)
{
	if (dbprivate) {
		print(tab "if ((ret = __os_malloc(env,") >> file
		print(tab "    " size ", &" ptr ")) != 0)") >> file
		print(tab "\treturn (ret);") >> file;
	} else {
		print(tab "if ((" ptr " = malloc(" size ")) == NULL)") >> file
		print(tab "\treturn (ENOMEM);") >> file
	}
}

function write_free(tab, ptr, file)
{
	if (dbprivate) {
		print(tab "__os_free(env, " ptr ");") >> file
	} else {
		print(tab "free(" ptr ");") >> file
	}
}

function make_name(unique_name, dup_name, p_version)
{
	logfunc = sprintf("%s_%s", prefix, unique_name);
	logname[num_funcs] = logfunc;
	if (is_compat) {
		funcname = sprintf("%s_%s_%s", prefix, unique_name, p_version);
	} else {
		funcname = logfunc;
	}

	if (is_duplicate)
		dupfuncs[num_funcs] = dup_name;
	else
		dupfuncs[num_funcs] = funcname;

	funcs[num_funcs] = funcname;
	functable[num_funcs] = is_compat;
	++num_funcs;
}

function log_funcdecl(name, withtype)
{
	# Function declaration
	if (has_dbp) {
		printf("int\n%s_log(dbp, txnp, ret_lsnp, flags",\
		    name) >> CFILE
	} else {
		printf("int\n%s_log(%s, txnp, ret_lsnp, flags",\
		    name, env_var) >> CFILE
	}
	for (i = 0; i < nvars; i++) {
		if (modes[i] == "DB") {
			# We pass in fileids on the dbp, so if this is one,
			# skip it.
			continue;
		}
		printf(",") >> CFILE
		if ((i % 6) == 0)
			printf("\n    ") >> CFILE
		else
			printf(" ") >> CFILE
		printf("%s", vars[i]) >> CFILE
	}

	if (withtype)
		printf(", type") >> CFILE

	printf(")\n") >> CFILE

	# Now print the parameters
	if (has_dbp) {
		printf("\tDB *dbp;\n") >> CFILE
	} else {
		printf("\t%s *%s;\n", env_type, env_var) >> CFILE
	}
	printf("\tDB_TXN *txnp;\n\tDB_LSN *ret_lsnp;\n") >> CFILE
	printf("\tu_int32_t flags;\n") >> CFILE
	for (i = 0; i < nvars; i++) {
		# We just skip for modes == DB.
		if (modes[i] == "DBT" || modes[i] == "LOCKS" ||
		    modes[i] == "PGDBT" || modes[i] == "PGDDBT")
			printf("\tconst %s *%s;\n", types[i], vars[i]) >> CFILE
		else if (modes[i] != "DB")
			printf("\t%s %s;\n", types[i], vars[i]) >> CFILE
	}
	if (withtype)
		printf("\tu_int32_t type;\n") >> CFILE
}

function log_callint(fname)
{
	if (has_dbp) {
		printf("\n{\n\treturn (%s_log(dbp, txnp, ret_lsnp, flags",\
		    internal_name) >> CFILE
	} else {
		printf("\n{\n\treturn (%s_log(%s, txnp, ret_lsnp, flags",\
		    internal_name, env_var) >> CFILE
	}

	for (i = 0; i < nvars; i++) {
		if (modes[i] == "DB") {
			# We pass in fileids on the dbp, so if this is one,
			# skip it.
			continue;
		}
		printf(",") >> CFILE
		if ((i % 6) == 0)
			printf("\n    ") >> CFILE
		else
			printf(" ") >> CFILE
		printf("%s", vars[i]) >> CFILE
	}

	printf(", DB_%s", fname) >> CFILE
	printf("));\n}\n") >> CFILE
}
