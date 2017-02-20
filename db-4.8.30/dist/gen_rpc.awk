#
# $Id$
# Awk script for generating client/server RPC code.
#
# This awk script generates most of the RPC routines for DB client/server
# use. It also generates a template for server and client procedures.  These
# functions must still be edited, but are highly stylized and the initial
# template gets you a fair way along the path).
#
# This awk script requires that these variables be set when it is called:
#
#	major		-- Major version number
#	minor		-- Minor version number
#	gidsize		-- size of GIDs
#	client_file	-- the C source file being created for client code
#	ctmpl_file	-- the C template file being created for client code
#	server_file	-- the C source file being created for server code
#	stmpl_file	-- the C template file being created for server code
#	xdr_file	-- the XDR message file created
#
# And stdin must be the input file that defines the RPC setup.
BEGIN {
	if (major == "" || minor == "" || gidsize == "" ||
	    client_file == "" || ctmpl_file == "" ||
	    server_file == "" || stmpl_file == "" || xdr_file == "") {
		print "Usage: gen_rpc.awk requires these variables be set:"
		print "\tmajor\t-- Major version number"
		print "\tminor\t-- Minor version number"
		print "\tgidsize\t-- GID size"
		print "\tclient_file\t-- the client C source file being created"
		print "\tctmpl_file\t-- the client template file being created"
		print "\tserver_file\t-- the server C source file being created"
		print "\tstmpl_file\t-- the server template file being created"
		print "\txdr_file\t-- the XDR message file being created"
		error = 1; exit
	}

	FS="\t\t*"
	CFILE=client_file
	printf("/* Do not edit: automatically built by gen_rpc.awk. */\n") \
	    > CFILE

	TFILE = ctmpl_file
	printf("/* Do not edit: automatically built by gen_rpc.awk. */\n") \
	    > TFILE

	SFILE = server_file
	printf("/* Do not edit: automatically built by gen_rpc.awk. */\n") \
	    > SFILE

	# Server procedure template.
	PFILE = stmpl_file
	XFILE = xdr_file
	printf("/* Do not edit: automatically built by gen_rpc.awk. */\n") \
	    > XFILE
	nendlist = 1;

	# Output headers
	general_headers()

	# Put out the actual illegal and no-server functions.
	illegal_functions(CFILE)
}
END {
	if (error == 0) {
		printf("program DB_RPC_SERVERPROG {\n") >> XFILE
		printf("\tversion DB_RPC_SERVERVERS {\n") >> XFILE

		for (i = 1; i < nendlist; ++i)
			printf("\t\t%s;\n", endlist[i]) >> XFILE

		printf("\t} = %d%03d;\n", major, minor) >> XFILE
		printf("} = 351457;\n") >> XFILE

		obj_init("DB", "dbp", obj_db, CFILE)
		obj_init("DBC", "dbc", obj_dbc, CFILE)
		obj_init("DB_ENV", "dbenv", obj_dbenv, CFILE)
		obj_init("DB_TXN", "txn", obj_txn, CFILE)
	}
}

/^[	 ]*LOCAL/ {
	# LOCAL methods are ones where we don't override the handle
	# method for RPC, nor is it illegal -- it's just satisfied
	# locally.
	next;
}
/^[	 ]*NOFUNC/ {
	++obj_indx;

	# NOFUNC methods are illegal on the RPC client.
	if ($2 ~ "^db_")
		obj_illegal(obj_db, "dbp", $2, $3)
	else if ($2 ~ "^dbc_")
		obj_illegal(obj_dbc, "dbc", $2, $3)
	else if ($2 ~ "^env_")
		obj_illegal(obj_dbenv, "dbenv", $2, $3)
	else if ($2 ~ "^txn_")
		obj_illegal(obj_txn, "txn", $2, $3)
	else {
		print "unexpected handle prefix: " $2
		error = 1; exit
	}
	next;
}
/^[	 ]*BEGIN/ {
	++obj_indx;

	name = $2;
	link_only = ret_code = 0
	if ($3 == "LINKONLY")
		link_only = 1
	else if ($3 == "RETCODE")
		ret_code = 1

	funcvars = 0;
	newvars = 0;
	nvars = 0;
	rvars = 0;
	xdr_free = 0;

	db_handle = 0;
	dbc_handle = 0;
	dbt_handle = 0;
	env_handle = 0;
	mp_handle = 0;
	txn_handle = 0;
}
/^[	 ]*ARG/ {
	rpc_type[nvars] = $2;
	c_type[nvars] = $3;
	pr_type[nvars] = $3;
	args[nvars] = $4;
	func_arg[nvars] = 0;
	if (rpc_type[nvars] == "LIST") {
		list_type[nvars] = $5;
	} else
		list_type[nvars] = 0;

	if (c_type[nvars] == "DBT *")
		dbt_handle = 1;
	else if (c_type[nvars] == "DB_ENV *") {
		ctp_type[nvars] = "CT_ENV";
		env_handle = 1;
		env_idx = nvars;

		if (nvars == 0)
			obj_func("dbenv", obj_dbenv);
	} else if (c_type[nvars] == "DB *") {
		ctp_type[nvars] = "CT_DB";
		if (db_handle != 1) {
			db_handle = 1;
			db_idx = nvars;
		}

		if (nvars == 0)
			obj_func("dbp", obj_db);
	} else if (c_type[nvars] == "DBC *") {
		ctp_type[nvars] = "CT_CURSOR";
		dbc_handle = 1;
		dbc_idx = nvars;

		if (nvars == 0)
			obj_func("dbc", obj_dbc);
	} else if (c_type[nvars] == "DB_TXN *") {
		ctp_type[nvars] = "CT_TXN";
		txn_handle = 1;
		txn_idx = nvars;

		if (nvars == 0)
			obj_func("txn", obj_txn);
	}

	++nvars;
}
/^[	 ]*FUNCPROT/ {
	pr_type[nvars] = $2;
}
/^[	 ]*FUNCARG/ {
	rpc_type[nvars] = "IGNORE";
	c_type[nvars] = $2;
	args[nvars] = sprintf("func%d", funcvars);
	func_arg[nvars] = 1;
	++funcvars;
	++nvars;
}
/^[	 ]*RET/ {
	ret_type[rvars] = $2;
	retc_type[rvars] = $3;
	retargs[rvars] = $4;
	if (ret_type[rvars] == "LIST" || ret_type[rvars] == "DBT") {
		xdr_free = 1;
	}
	if (ret_type[rvars] == "LIST") {
		retlist_type[rvars] = $5;
	} else
		retlist_type[rvars] = 0;
	ret_isarg[rvars] = 0;

	++rvars;
}
/^[	 ]*ARET/ {
	ret_type[rvars] = $2;
	rpc_type[nvars] = "IGNORE";
	retc_type[rvars] = $3;
	c_type[nvars] = sprintf("%s *", $3);
	pr_type[nvars] = c_type[nvars];
	retargs[rvars] = $4;
	args[nvars] = sprintf("%sp", $4);
	if (ret_type[rvars] == "LIST" || ret_type[rvars] == "DBT") {
		xdr_free = 1;
	}
	func_arg[nvars] = 0;
	if (ret_type[nvars] == "LIST") {
		retlist_type[rvars] =  $5;
		list_type[nvars] = $5;
	} else {
		retlist_type[rvars] = 0;
		list_type[nvars] = 0;
	}
	ret_isarg[rvars] = 1;

	++nvars;
	++rvars;
}
/^[	 ]*END/ {
	#
	# =====================================================
	# LINKONLY -- just reference the function, that's all.
	#
	if (link_only)
		next;

	#
	# =====================================================
	# XDR messages.
	#
	printf("\n") >> XFILE
	printf("struct __%s_msg {\n", name) >> XFILE
	for (i = 0; i < nvars; ++i) {
		if (rpc_type[i] == "LIST") {
			if (list_type[i] == "GID") {
				printf("\topaque %s<>;\n", args[i]) >> XFILE
			} else {
				printf("\tunsigned int %s<>;\n", args[i]) >> XFILE
			}
		}
		if (rpc_type[i] == "ID") {
			printf("\tunsigned int %scl_id;\n", args[i]) >> XFILE
		}
		if (rpc_type[i] == "STRING") {
			printf("\tstring %s<>;\n", args[i]) >> XFILE
		}
		if (rpc_type[i] == "GID") {
			printf("\topaque %s[%d];\n", args[i], gidsize) >> XFILE
		}
		if (rpc_type[i] == "INT") {
			printf("\tunsigned int %s;\n", args[i]) >> XFILE
		}
		if (rpc_type[i] == "DBT") {
			printf("\tunsigned int %sdlen;\n", args[i]) >> XFILE
			printf("\tunsigned int %sdoff;\n", args[i]) >> XFILE
			printf("\tunsigned int %sulen;\n", args[i]) >> XFILE
			printf("\tunsigned int %sflags;\n", args[i]) >> XFILE
			printf("\topaque %sdata<>;\n", args[i]) >> XFILE
		}
	}
	printf("};\n") >> XFILE

	printf("\n") >> XFILE
	#
	# Generate the reply message
	#
	printf("struct __%s_reply {\n", name) >> XFILE
	printf("\t/* num return vars: %d */\n", rvars) >> XFILE
	printf("\tint status;\n") >> XFILE
	for (i = 0; i < rvars; ++i) {
		if (ret_type[i] == "ID") {
			printf("\tunsigned int %scl_id;\n", retargs[i]) >> XFILE
		}
		if (ret_type[i] == "STRING") {
			printf("\tstring %s<>;\n", retargs[i]) >> XFILE
		}
		if (ret_type[i] == "INT") {
			printf("\tunsigned int %s;\n", retargs[i]) >> XFILE
		}
		if (ret_type[i] == "DBL") {
			printf("\tdouble %s;\n", retargs[i]) >> XFILE
		}
		if (ret_type[i] == "DBT") {
			printf("\topaque %sdata<>;\n", retargs[i]) >> XFILE
		}
		if (ret_type[i] == "LIST") {
			if (retlist_type[i] == "GID") {
				printf("\topaque %s<>;\n", retargs[i]) >> XFILE
			} else {
				printf("\tunsigned int %s<>;\n", retargs[i]) >> XFILE
			}
		}
	}
	printf("};\n") >> XFILE

	endlist[nendlist] = \
	    sprintf("__%s_reply __DB_%s(__%s_msg) = %d", \
		name, name, name, nendlist);
	nendlist++;
	#
	# =====================================================
	# Server functions.
	#
	# First spit out PUBLIC prototypes for server functions.
	#
	printf("__%s_reply *\n", name) >> SFILE
	printf("__db_%s_%d%03d__SVCSUFFIX__(msg, req)\n", \
	    name, major, minor) >> SFILE
	printf("\t__%s_msg *msg;\n", name) >> SFILE;
	printf("\tstruct svc_req *req;\n", name) >> SFILE;
	printf("{\n") >> SFILE
	printf("\tstatic __%s_reply reply; /* must be static */\n", \
	    name) >> SFILE
	if (xdr_free) {
		printf("\tstatic int __%s_free = 0; /* must be static */\n\n", \
		    name) >> SFILE
	}
	printf("\tCOMPQUIET(req, NULL);\n", name) >> SFILE
	if (xdr_free) {
		printf("\tif (__%s_free)\n", name) >> SFILE
		printf("\t\txdr_free((xdrproc_t)xdr___%s_reply, (void *)&reply);\n", \
		    name) >> SFILE
		printf("\t__%s_free = 0;\n", name) >> SFILE
		printf("\n\t/* Reinitialize allocated fields */\n") >> SFILE
		for (i = 0; i < rvars; ++i) {
			if (ret_type[i] == "LIST") {
				printf("\treply.%s.%s_val = NULL;\n", \
				    retargs[i], retargs[i]) >> SFILE
			}
			if (ret_type[i] == "DBT") {
				printf("\treply.%sdata.%sdata_val = NULL;\n", \
				    retargs[i], retargs[i]) >> SFILE
			}
		}
	}

	need_out = 0;
	#
	# Compose server proc to call.  Decompose message components as args.
	#
	printf("\n\t__%s_proc(", name) >> SFILE
	sep = "";
	for (i = 0; i < nvars; ++i) {
		if (rpc_type[i] == "IGNORE") {
			continue;
		}
		if (rpc_type[i] == "ID") {
			printf("%smsg->%scl_id", sep, args[i]) >> SFILE
		}
		if (rpc_type[i] == "STRING") {
			printf("%s(*msg->%s == '\\0') ? NULL : msg->%s", \
			    sep, args[i], args[i]) >> SFILE
		}
		if (rpc_type[i] == "GID") {
			printf("%s(u_int8_t *)msg->%s", sep, args[i]) >> SFILE
		}
		if (rpc_type[i] == "INT") {
			printf("%smsg->%s", sep, args[i]) >> SFILE
		}
		if (rpc_type[i] == "LIST") {
			printf("%smsg->%s.%s_val", \
			    sep, args[i], args[i]) >> SFILE
			printf("%smsg->%s.%s_len", \
			    sep, args[i], args[i]) >> SFILE
		}
		if (rpc_type[i] == "DBT") {
			printf("%smsg->%sdlen", sep, args[i]) >> SFILE
			sep = ",\n\t    ";
			printf("%smsg->%sdoff", sep, args[i]) >> SFILE
			printf("%smsg->%sulen", sep, args[i]) >> SFILE
			printf("%smsg->%sflags", sep, args[i]) >> SFILE
			printf("%smsg->%sdata.%sdata_val", \
			    sep, args[i], args[i]) >> SFILE
			printf("%smsg->%sdata.%sdata_len", \
			    sep, args[i], args[i]) >> SFILE
		}
		sep = ",\n\t    ";
	}
	printf("%s&reply", sep) >> SFILE
	if (xdr_free)
		printf("%s&__%s_free);\n", sep, name) >> SFILE
	else
		printf(");\n\n") >> SFILE
	if (need_out) {
		printf("\nout:\n") >> SFILE
	}
	printf("\treturn (&reply);\n") >> SFILE
	printf("}\n\n") >> SFILE

	#
	# =====================================================
	# Generate Procedure Template Server code
	#
	# Spit out comment, prototype, function name and arg list.
	printf("/* BEGIN __%s_proc */\n", name) >> PFILE
	delete p;
	pi = 1;
	p[pi++] = sprintf("void __%s_proc __P((", name);
	p[pi++] = "";
	for (i = 0; i < nvars; ++i) {
		if (rpc_type[i] == "IGNORE")
			continue;
		if (rpc_type[i] == "ID") {
			p[pi++] = "long";
			p[pi++] = ", ";
		}
		if (rpc_type[i] == "STRING") {
			p[pi++] = "char *";
			p[pi++] = ", ";
		}
		if (rpc_type[i] == "GID") {
			p[pi++] = "u_int8_t *";
			p[pi++] = ", ";
		}
		if (rpc_type[i] == "INT") {
			p[pi++] = "u_int32_t";
			p[pi++] = ", ";
		}
		if (rpc_type[i] == "INTRET") {
			p[pi++] = "u_int32_t *";
			p[pi++] = ", ";
		}
		if (rpc_type[i] == "LIST" && list_type[i] == "GID") {
			p[pi++] = "u_int8_t *";
			p[pi++] = ", ";
			p[pi++] = "u_int32_t";
			p[pi++] = ", ";
		}
		if (rpc_type[i] == "LIST" && list_type[i] == "INT") {
			p[pi++] = "u_int32_t *";
			p[pi++] = ", ";
			p[pi++] = "u_int32_t";
			p[pi++] = ", ";
		}
		if (rpc_type[i] == "LIST" && list_type[i] == "ID") {
			p[pi++] = "u_int32_t *";
			p[pi++] = ", ";
			p[pi++] = "u_int32_t";
			p[pi++] = ", ";
		}
		if (rpc_type[i] == "DBT") {
			p[pi++] = "u_int32_t";
			p[pi++] = ", ";
			p[pi++] = "u_int32_t";
			p[pi++] = ", ";
			p[pi++] = "u_int32_t";
			p[pi++] = ", ";
			p[pi++] = "u_int32_t";
			p[pi++] = ", ";
			p[pi++] = "void *";
			p[pi++] = ", ";
			p[pi++] = "u_int32_t";
			p[pi++] = ", ";
		}
	}
	p[pi++] = sprintf("__%s_reply *", name);
	if (xdr_free) {
		p[pi++] = ", ";
		p[pi++] = "int *));";
	} else {
		p[pi++] = "";
		p[pi++] = "));";
	}
	p[pi++] = "";

	printf("void\n") >> PFILE
	printf("__%s_proc(", name) >> PFILE
	sep = "";
	argcount = 0;
	for (i = 0; i < nvars; ++i) {
		argcount++;
		split_lines();
		if (argcount == 0) {
			sep = "";
		}
		if (rpc_type[i] == "IGNORE")
			continue;
		if (rpc_type[i] == "ID") {
			printf("%s%scl_id", sep, args[i]) >> PFILE
		}
		if (rpc_type[i] == "STRING") {
			printf("%s%s", sep, args[i]) >> PFILE
		}
		if (rpc_type[i] == "GID") {
			printf("%s%s", sep, args[i]) >> PFILE
		}
		if (rpc_type[i] == "INT") {
			printf("%s%s", sep, args[i]) >> PFILE
		}
		if (rpc_type[i] == "INTRET") {
			printf("%s%s", sep, args[i]) >> PFILE
		}
		if (rpc_type[i] == "LIST") {
			printf("%s%s", sep, args[i]) >> PFILE
			argcount++;
			split_lines();
			if (argcount == 0) {
				sep = "";
			} else {
				sep = ", ";
			}
			printf("%s%slen", sep, args[i]) >> PFILE
		}
		if (rpc_type[i] == "DBT") {
			printf("%s%sdlen", sep, args[i]) >> PFILE
			sep = ", ";
			argcount++;
			split_lines();
			if (argcount == 0) {
				sep = "";
			} else {
				sep = ", ";
			}
			printf("%s%sdoff", sep, args[i]) >> PFILE
			argcount++;
			split_lines();
			if (argcount == 0) {
				sep = "";
			} else {
				sep = ", ";
			}
			printf("%s%sulen", sep, args[i]) >> PFILE
			argcount++;
			split_lines();
			if (argcount == 0) {
				sep = "";
			} else {
				sep = ", ";
			}
			printf("%s%sflags", sep, args[i]) >> PFILE
			argcount++;
			split_lines();
			if (argcount == 0) {
				sep = "";
			} else {
				sep = ", ";
			}
			printf("%s%sdata", sep, args[i]) >> PFILE
			argcount++;
			split_lines();
			if (argcount == 0) {
				sep = "";
			} else {
				sep = ", ";
			}
			printf("%s%ssize", sep, args[i]) >> PFILE
		}
		sep = ", ";
	}
	printf("%sreplyp",sep) >> PFILE
	if (xdr_free) {
		printf("%sfreep)\n",sep) >> PFILE
	} else {
		printf(")\n") >> PFILE
	}
	#
	# Spit out arg types/names;
	#
	for (i = 0; i < nvars; ++i) {
		if (rpc_type[i] == "ID") {
			printf("\tunsigned int %scl_id;\n", args[i]) >> PFILE
		}
		if (rpc_type[i] == "STRING") {
			printf("\tchar *%s;\n", args[i]) >> PFILE
		}
		if (rpc_type[i] == "GID") {
			printf("\tu_int8_t *%s;\n", args[i]) >> PFILE
		}
		if (rpc_type[i] == "INT") {
			printf("\tu_int32_t %s;\n", args[i]) >> PFILE
		}
		if (rpc_type[i] == "LIST" && list_type[i] == "GID") {
			printf("\tu_int8_t * %s;\n", args[i]) >> PFILE
		}
		if (rpc_type[i] == "LIST" && list_type[i] == "INT") {
			printf("\tu_int32_t * %s;\n", args[i]) >> PFILE
			printf("\tu_int32_t %ssize;\n", args[i]) >> PFILE
		}
		if (rpc_type[i] == "LIST" && list_type[i] == "ID") {
			printf("\tu_int32_t * %s;\n", args[i]) >> PFILE
		}
		if (rpc_type[i] == "LIST") {
			printf("\tu_int32_t %slen;\n", args[i]) >> PFILE
		}
		if (rpc_type[i] == "DBT") {
			printf("\tu_int32_t %sdlen;\n", args[i]) >> PFILE
			printf("\tu_int32_t %sdoff;\n", args[i]) >> PFILE
			printf("\tu_int32_t %sulen;\n", args[i]) >> PFILE
			printf("\tu_int32_t %sflags;\n", args[i]) >> PFILE
			printf("\tvoid *%sdata;\n", args[i]) >> PFILE
			printf("\tu_int32_t %ssize;\n", args[i]) >> PFILE
		}
	}
	printf("\t__%s_reply *replyp;\n",name) >> PFILE
	if (xdr_free) {
		printf("\tint * freep;\n") >> PFILE
	}

	printf("/* END __%s_proc */\n", name) >> PFILE

	#
	# Function body
	#
	printf("{\n") >> PFILE
	printf("\tint ret;\n") >> PFILE
	for (i = 0; i < nvars; ++i) {
		if (rpc_type[i] == "ID") {
			printf("\t%s %s;\n", c_type[i], args[i]) >> PFILE
			printf("\tct_entry *%s_ctp;\n", args[i]) >> PFILE
		}
	}
	printf("\n") >> PFILE
	for (i = 0; i < nvars; ++i) {
		if (rpc_type[i] == "ID") {
			printf("\tACTIVATE_CTP(%s_ctp, %scl_id, %s);\n", \
			    args[i], args[i], ctp_type[i]) >> PFILE
			printf("\t%s = (%s)%s_ctp->ct_anyp;\n", \
			    args[i], c_type[i], args[i]) >> PFILE
		}
	}
	printf("\n\t/*\n\t * XXX Code goes here\n\t */\n\n") >> PFILE
	printf("\treplyp->status = ret;\n") >> PFILE
	printf("\treturn;\n") >> PFILE
	printf("}\n\n") >> PFILE

	#
	# =====================================================
	# Generate Client code
	#
	# Spit out PUBLIC prototypes.
	#
	delete p;
	pi = 1;
	p[pi++] = sprintf("int __dbcl_%s __P((", name);
	p[pi++] = "";
	for (i = 0; i < nvars; ++i) {
		p[pi++] = pr_type[i];
		p[pi++] = ", ";
	}
	p[pi - 1] = "";
	p[pi] = "));";
	proto_format(p, CFILE);

	#
	# Spit out function name/args.
	#
	printf("int\n") >> CFILE
	printf("__dbcl_%s(", name) >> CFILE
	sep = "";
	for (i = 0; i < nvars; ++i) {
		printf("%s%s", sep, args[i]) >> CFILE
		sep = ", ";
	}
	printf(")\n") >> CFILE

	for (i = 0; i < nvars; ++i)
		if (func_arg[i] == 0)
			printf("\t%s %s;\n", c_type[i], args[i]) >> CFILE
		else
			printf("\t%s;\n", c_type[i]) >> CFILE

	printf("{\n") >> CFILE
	printf("\tCLIENT *cl;\n") >> CFILE
	printf("\t__%s_msg msg;\n", name) >> CFILE
	printf("\t__%s_reply *replyp = NULL;\n", name) >> CFILE;
	printf("\tint ret;\n") >> CFILE
	if (!env_handle)
		printf("\tDB_ENV *dbenv;\n") >> CFILE
	#
	# If we are managing a list, we need a few more vars.
	#
	for (i = 0; i < nvars; ++i) {
		if (rpc_type[i] == "LIST") {
			printf("\t%s %sp;\n", c_type[i], args[i]) >> CFILE
			printf("\tint %si;\n", args[i]) >> CFILE
			if (list_type[i] == "GID")
				printf("\tu_int8_t ** %sq;\n", args[i]) >> CFILE
			else
				printf("\tu_int32_t * %sq;\n", args[i]) >> CFILE
		}
	}

	printf("\n") >> CFILE
	printf("\tret = 0;\n") >> CFILE
	if (!env_handle) {
		if (db_handle)
			printf("\tdbenv = %s->dbenv;\n", args[db_idx]) >> CFILE
		else if (dbc_handle)
			printf("\tdbenv = %s->dbenv;\n", \
			    args[dbc_idx]) >> CFILE
		else if (txn_handle)
			printf("\tdbenv = %s->mgrp->env->dbenv;\n", \
			    args[txn_idx]) >> CFILE
		else
			printf("\tdbenv = NULL;\n") >> CFILE
		printf("\tif (dbenv == NULL || !RPC_ON(dbenv))\n") >> CFILE
		printf("\t\treturn (__dbcl_noserver(NULL));\n") >> CFILE
	} else {
		printf("\tif (%s == NULL || !RPC_ON(%s))\n", \
		    args[env_idx], args[env_idx]) >> CFILE
		printf("\t\treturn (__dbcl_noserver(%s));\n", \
		    args[env_idx]) >> CFILE
	}
	printf("\n") >> CFILE

	printf("\tcl = (CLIENT *)%s->cl_handle;\n\n", \
	    env_handle ? args[env_idx] : "dbenv") >> CFILE

	#
	# If there is a function arg, check that it is NULL
	#
	for (i = 0; i < nvars; ++i) {
		if (func_arg[i] != 1)
			continue;
		printf("\tif (%s != NULL) {\n", args[i]) >> CFILE
		if (!env_handle) {
			printf("\t\t__db_errx(dbenv->env, ") >> CFILE
		} else {
			printf(\
			    "\t\t__db_errx(%s->env, ", args[env_idx]) >> CFILE
		}
		printf("\"User functions not supported in RPC\");\n") >> CFILE
		printf("\t\treturn (EINVAL);\n\t}\n") >> CFILE
	}

	#
	# Compose message components
	#
	for (i = 0; i < nvars; ++i) {
		if (rpc_type[i] == "ID") {
			# We don't need to check for a NULL DB_ENV *, because
			# we already checked for it.  I frankly couldn't care
			# less, but lint gets all upset at the wasted cycles.
			if (c_type[i] != "DB_ENV *") {
				printf("\tif (%s == NULL)\n", args[i]) >> CFILE
				printf("\t\tmsg.%scl_id = 0;\n\telse\n", \
				    args[i]) >> CFILE
				indent = "\t\t";
			} else
				indent = "\t";
			if (c_type[i] == "DB_TXN *") {
				printf("%smsg.%scl_id = %s->txnid;\n", \
				    indent, args[i], args[i]) >> CFILE
			} else {
				printf("%smsg.%scl_id = %s->cl_id;\n", \
				    indent, args[i], args[i]) >> CFILE
			}
		}
		if (rpc_type[i] == "GID") {
			printf("\tmemcpy(msg.%s, %s, %d);\n", \
			    args[i], args[i], gidsize) >> CFILE
		}
		if (rpc_type[i] == "INT") {
			printf("\tmsg.%s = (u_int)%s;\n",
			    args[i], args[i]) >> CFILE
		}
		if (rpc_type[i] == "STRING") {
			printf("\tif (%s == NULL)\n", args[i]) >> CFILE
			printf("\t\tmsg.%s = \"\";\n", args[i]) >> CFILE
			printf("\telse\n") >> CFILE
			printf("\t\tmsg.%s = (char *)%s;\n", \
			    args[i], args[i]) >> CFILE
		}
		if (rpc_type[i] == "DBT") {
			printf("\tmsg.%sdlen = %s->dlen;\n", \
			    args[i], args[i]) >> CFILE
			printf("\tmsg.%sdoff = %s->doff;\n", \
			    args[i], args[i]) >> CFILE
			printf("\tmsg.%sulen = %s->ulen;\n", \
			    args[i], args[i]) >> CFILE
			printf("\tmsg.%sflags = %s->flags;\n", \
			    args[i], args[i]) >> CFILE
			printf("\tmsg.%sdata.%sdata_val = %s->data;\n", \
			    args[i], args[i], args[i]) >> CFILE
			printf("\tmsg.%sdata.%sdata_len = %s->size;\n", \
			    args[i], args[i], args[i]) >> CFILE
		}
		if (rpc_type[i] == "LIST") {
			printf("\tfor (%si = 0, %sp = %s; *%sp != 0; ", \
			    args[i], args[i], args[i], args[i]) >> CFILE
			printf(" %si++, %sp++)\n\t\t;\n", args[i], args[i]) \
			    >> CFILE

			#
			# If we are an array of ints, *_len is how many
			# elements.  If we are a GID, *_len is total bytes.
			#
			printf("\tmsg.%s.%s_len = (u_int)%si",
			    args[i], args[i], args[i]) >> CFILE
			if (list_type[i] == "GID")
				printf(" * %d;\n", gidsize) >> CFILE
			else
				printf(";\n") >> CFILE
			printf("\tif ((ret = __os_calloc(") >> CFILE
			if (!env_handle)
				printf("dbenv->env,\n") >> CFILE
			else
				printf("%s->env,\n", args[env_idx]) >> CFILE
			printf("\t    msg.%s.%s_len,", \
			    args[i], args[i]) >> CFILE
			if (list_type[i] == "GID")
				printf(" 1,") >> CFILE
			else
				printf(" sizeof(u_int32_t),") >> CFILE
			printf(" &msg.%s.%s_val)) != 0)\n",\
			    args[i], args[i], args[i], args[i]) >> CFILE
			printf("\t\treturn (ret);\n") >> CFILE
			printf("\tfor (%sq = msg.%s.%s_val, %sp = %s; ", \
			    args[i], args[i], args[i], \
			    args[i], args[i]) >> CFILE
			printf("%si--; %sq++, %sp++)\n", \
			    args[i], args[i], args[i]) >> CFILE
			printf("\t\t*%sq = ", args[i]) >> CFILE
			if (list_type[i] == "GID")
				printf("*%sp;\n", args[i]) >> CFILE
			if (list_type[i] == "ID")
				printf("(*%sp)->cl_id;\n", args[i]) >> CFILE
			if (list_type[i] == "INT")
				printf("*%sp;\n", args[i]) >> CFILE
		}
	}

	printf("\n") >> CFILE
	printf("\treplyp = __db_%s_%d%03d(&msg, cl);\n", name, major, minor) \
	    >> CFILE
	for (i = 0; i < nvars; ++i)
		if (rpc_type[i] == "LIST")
			printf("\t__os_free(%s->env, msg.%s.%s_val);\n",
			    env_handle ? args[env_idx] : "dbenv",
			    args[i], args[i]) >> CFILE

	printf("\tif (replyp == NULL) {\n") >> CFILE
	printf("\t\t__db_errx(%s->env, clnt_sperror(cl, \"Berkeley DB\"));\n",
	    env_handle ? args[env_idx] : "dbenv") >> CFILE

	printf("\t\tret = DB_NOSERVER;\n") >> CFILE
	printf("\t\tgoto out;\n") >> CFILE
	printf("\t}\n") >> CFILE

	if (ret_code == 0) {
		printf("\tret = replyp->status;\n") >> CFILE

		#
		# Set any arguments that are returned
		#
		for (i = 0; i < rvars; ++i) {
			if (ret_isarg[i]) {
				printf("\tif (%sp != NULL)\n",
				    retargs[i]) >> CFILE;
				printf("\t\t*%sp = (%s)replyp->%s;\n",
				    retargs[i],
				    retc_type[i], retargs[i]) >> CFILE;
			}
		}
	} else {
		printf("\tret = __dbcl_%s_ret(", name) >> CFILE
		sep = "";
		for (i = 0; i < nvars; ++i) {
			printf("%s%s", sep, args[i]) >> CFILE
			sep = ", ";
		}
		printf("%sreplyp);\n", sep) >> CFILE
	}
	printf("out:\n") >> CFILE
	#
	# Free reply if there was one.
	#
	printf("\tif (replyp != NULL)\n") >> CFILE
	printf("\t\txdr_free((xdrproc_t)xdr___%s_reply,",name) >> CFILE
	printf(" (void *)replyp);\n") >> CFILE
	printf("\treturn (ret);\n") >> CFILE
	printf("}\n\n") >> CFILE

	#
	# Generate Client Template code
	#
	if (ret_code) {
		#
		# If we are doing a list, write prototypes
		#
		delete p;
		pi = 1;
		p[pi++] = sprintf("int __dbcl_%s_ret __P((", name);
		p[pi++] = "";
		for (i = 0; i < nvars; ++i) {
			p[pi++] = pr_type[i];
			p[pi++] = ", ";
		}
		p[pi] = sprintf("__%s_reply *));", name);
		proto_format(p, TFILE);

		printf("int\n") >> TFILE
		printf("__dbcl_%s_ret(", name) >> TFILE
		sep = "";
		for (i = 0; i < nvars; ++i) {
			printf("%s%s", sep, args[i]) >> TFILE
			sep = ", ";
		}
		printf("%sreplyp)\n",sep) >> TFILE

		for (i = 0; i < nvars; ++i)
			if (func_arg[i] == 0)
				printf("\t%s %s;\n", c_type[i], args[i]) \
				    >> TFILE
			else
				printf("\t%s;\n", c_type[i]) >> TFILE
		printf("\t__%s_reply *replyp;\n", name) >> TFILE;
		printf("{\n") >> TFILE
		printf("\tint ret;\n") >> TFILE
		#
		# Local vars in template
		#
		for (i = 0; i < rvars; ++i) {
			if (ret_type[i] == "ID" || ret_type[i] == "STRING" ||
			    ret_type[i] == "INT" || ret_type[i] == "DBL") {
				printf("\t%s %s;\n", \
				    retc_type[i], retargs[i]) >> TFILE
			} else if (ret_type[i] == "LIST") {
				if (retlist_type[i] == "GID")
					printf("\tu_int8_t *__db_%s;\n", \
					    retargs[i]) >> TFILE
				if (retlist_type[i] == "ID" ||
				    retlist_type[i] == "INT")
					printf("\tu_int32_t *__db_%s;\n", \
					    retargs[i]) >> TFILE
			} else {
				printf("\t/* %s %s; */\n", \
				    ret_type[i], retargs[i]) >> TFILE
			}
		}
		#
		# Client return code
		#
		printf("\n") >> TFILE
		printf("\tif (replyp->status != 0)\n") >> TFILE
		printf("\t\treturn (replyp->status);\n") >> TFILE
		for (i = 0; i < rvars; ++i) {
			varname = "";
			if (ret_type[i] == "ID") {
				varname = sprintf("%scl_id", retargs[i]);
			}
			if (ret_type[i] == "STRING") {
				varname =  retargs[i];
			}
			if (ret_type[i] == "INT" || ret_type[i] == "DBL") {
				varname =  retargs[i];
			}
			if (ret_type[i] == "DBT") {
				varname = sprintf("%sdata", retargs[i]);
			}
			if (ret_type[i] == "ID" || ret_type[i] == "STRING" ||
			    ret_type[i] == "INT" || ret_type[i] == "DBL") {
				printf("\t%s = replyp->%s;\n", \
				    retargs[i], varname) >> TFILE
			} else if (ret_type[i] == "LIST") {
				printf("\n\t/*\n") >> TFILE
				printf("\t * XXX Handle list\n") >> TFILE
				printf("\t */\n\n") >> TFILE
			} else {
				printf("\t/* Handle replyp->%s; */\n", \
				    varname) >> TFILE
			}
		}
		printf("\n\t/*\n\t * XXX Code goes here\n\t */\n\n") >> TFILE
		printf("\treturn (replyp->status);\n") >> TFILE
		printf("}\n\n") >> TFILE
	}
}

function general_headers()
{
	printf("#include \"db_config.h\"\n") >> CFILE
	printf("\n") >> CFILE
	printf("#include \"db_int.h\"\n") >> CFILE
	printf("#ifdef HAVE_SYSTEM_INCLUDE_FILES\n") >> CFILE
	printf("#include <rpc/rpc.h>\n") >> CFILE
	printf("#endif\n") >> CFILE
	printf("#include \"db_server.h\"\n") >> CFILE
	printf("#include \"dbinc/txn.h\"\n") >> CFILE
	printf("#include \"dbinc_auto/rpc_client_ext.h\"\n") >> CFILE
	printf("\n") >> CFILE

	printf("#include \"db_config.h\"\n") >> TFILE
	printf("\n") >> TFILE
	printf("#include \"db_int.h\"\n") >> TFILE
	printf("#include \"dbinc/txn.h\"\n") >> TFILE
	printf("\n") >> TFILE

	printf("#include \"db_config.h\"\n") >> SFILE
	printf("\n") >> SFILE
	printf("#include \"db_int.h\"\n") >> SFILE
	printf("#ifdef HAVE_SYSTEM_INCLUDE_FILES\n") >> SFILE
	printf("#include <rpc/rpc.h>\n") >> SFILE
	printf("#endif\n") >> SFILE
	printf("#include \"db_server.h\"\n") >> SFILE
	printf("#include \"dbinc/db_server_int.h\"\n") >> SFILE
	printf("#include \"dbinc_auto/rpc_server_ext.h\"\n") >> SFILE
	printf("\n") >> SFILE

	printf("#include \"db_config.h\"\n") >> PFILE
	printf("\n") >> PFILE
	printf("#include \"db_int.h\"\n") >> PFILE
	printf("#ifdef HAVE_SYSTEM_INCLUDE_FILES\n") >> PFILE
	printf("#include <rpc/rpc.h>\n") >> PFILE
	printf("#endif\n") >> PFILE
	printf("#include \"db_server.h\"\n") >> PFILE
	printf("#include \"dbinc/db_server_int.h\"\n") >> PFILE
	printf("\n") >> PFILE
}

#
# illegal_functions --
#	Output general illegal-call functions
function illegal_functions(OUTPUT)
{
	printf("static int __dbcl_dbp_illegal __P((DB *));\n") >> OUTPUT
	printf("static int __dbcl_noserver __P((DB_ENV *));\n") >> OUTPUT
	printf("static int __dbcl_txn_illegal __P((DB_TXN *));\n") >> OUTPUT
	# If we ever need an "illegal" function for a DBC method.
	# printf("static int __dbcl_dbc_illegal __P((DBC *));\n") >> OUTPUT
	printf("\n") >> OUTPUT

	printf("static int\n") >> OUTPUT
	printf("__dbcl_noserver(dbenv)\n") >> OUTPUT
	printf("\tDB_ENV *dbenv;\n") >> OUTPUT
	printf("{\n\t__db_errx(dbenv == NULL ? NULL : dbenv->env,") >> OUTPUT
	printf(\
	    "\n\t    \"No Berkeley DB RPC server environment\");\n") >> OUTPUT
	printf("\treturn (DB_NOSERVER);\n") >> OUTPUT
	printf("}\n\n") >> OUTPUT

	printf("/*\n") >> OUTPUT
	printf(" * __dbcl_dbenv_illegal --\n") >> OUTPUT
	printf(" *	DB_ENV method not supported under RPC.\n") >> OUTPUT
	printf(" *\n") >> OUTPUT
	printf(" * PUBLIC: int __dbcl_dbenv_illegal __P((DB_ENV *));\n")\
	    >> OUTPUT
	printf(" */\n") >> OUTPUT
	printf("int\n") >> OUTPUT
	printf("__dbcl_dbenv_illegal(dbenv)\n") >> OUTPUT
	printf("\tDB_ENV *dbenv;\n") >> OUTPUT
	printf("{\n\t__db_errx(dbenv == NULL ? NULL : dbenv->env,") >> OUTPUT
	printf("\n\t    \"Interface not supported by ") >> OUTPUT
	printf("Berkeley DB RPC client environments\");\n") >> OUTPUT
	printf("\treturn (DB_OPNOTSUP);\n") >> OUTPUT
	printf("}\n\n") >> OUTPUT
	printf("/*\n") >> OUTPUT
	printf(" * __dbcl_dbp_illegal --\n") >> OUTPUT
	printf(" *	DB method not supported under RPC.\n") >> OUTPUT
	printf(" */\n") >> OUTPUT
	printf("static int\n") >> OUTPUT
	printf("__dbcl_dbp_illegal(dbp)\n") >> OUTPUT
	printf("\tDB *dbp;\n") >> OUTPUT
	printf("{\n\treturn (__dbcl_dbenv_illegal(dbp->dbenv));\n") >> OUTPUT
	printf("}\n\n") >> OUTPUT
	printf("/*\n") >> OUTPUT
	printf(" * __dbcl_txn_illegal --\n") >> OUTPUT
	printf(" *	DB_TXN method not supported under RPC.\n") >> OUTPUT
	printf(" */\n") >> OUTPUT
	printf("static int\n__dbcl_txn_illegal(txn)\n") >> OUTPUT
	printf("\tDB_TXN *txn;\n") >> OUTPUT
	printf("{\n\treturn (__dbcl_dbenv_illegal(txn->mgrp->env->dbenv));\n")\
	    >> OUTPUT
	printf("}\n\n") >> OUTPUT
	# If we ever need an "illegal" function for a DBC method.
	# printf("static int\n") >> OUTPUT
	# printf("__dbcl_dbc_illegal(dbc)\n") >> OUTPUT
	# printf("\tDBC *dbc;\n") >> OUTPUT
	# printf("{\n\treturn (__dbcl_dbenv_illegal(dbc->dbenv));\n") \
	#    >> OUTPUT
	# printf("}\n\n") >> OUTPUT
}

function obj_func(v, l)
{
	# Ignore db_create and env_create -- there's got to be something
	# cleaner, but I don't want to rewrite rpc.src right now.
	if (name == "db_create")
		return;
	if (name == "env_create")
		return;

	# Strip off the leading prefix for the method name.
	#
	# There are two method names for cursors, the old and the new.
	#
	# There just has to be something cleaner, but yadda, yadda, yadda.
	len = length(name);
	i = index(name, "_");
	s = substr(name, i + 1, len - i)

	if (v != "dbc" || s == "get_priority" || s == "set_priority")
		o = ""
	else
		o = sprintf(" = %s->c_%s", v, s)
	l[obj_indx] = sprintf("\t%s->%s%s = __dbcl_%s;", v, s, o, name)
}

function obj_illegal(l, handle, method, proto)
{
	# All of the functions return an int, with one exception.  Hack
	# to make that work.
	type = method == "db_get_mpf" ? "DB_MPOOLFILE *" : "int"

	# Strip off the leading prefix for the method name -- there's got to
	# be something cleaner, but I don't want to rewrite rpc.src right now.
	len = length(method);
	i = index(method, "_");

	l[obj_indx] =\
	    sprintf("\t%s->%s =\n\t    (%s (*)(",\
	    handle, substr(method, i + 1, len - i), type)\
	    proto\
	    sprintf("))\n\t    __dbcl_%s_illegal;", handle);
}

function obj_init(obj, v, list, OUTPUT) {
	printf("/*\n") >> OUTPUT
	printf(" * __dbcl_%s_init --\n", v) >> OUTPUT
	printf(" *\tInitialize %s handle methods.\n", obj) >> OUTPUT
	printf(" *\n") >> OUTPUT
	printf(\
	    " * PUBLIC: void __dbcl_%s_init __P((%s *));\n", v, obj) >> OUTPUT
	printf(" */\n") >> OUTPUT
	printf("void\n") >> OUTPUT
	printf("__dbcl_%s_init(%s)\n", v, v) >> OUTPUT
	printf("\t%s *%s;\n", obj, v) >> OUTPUT
	printf("{\n") >> OUTPUT
	for (i = 1; i < obj_indx; ++i) {
		if (i in list)
			print list[i] >> OUTPUT
	}
	printf("\treturn;\n}\n\n") >> OUTPUT
}

#
# split_lines --
#	Add line separators to pretty-print the output.
function split_lines() {
	if (argcount > 3) {
		# Reset the counter, remove any trailing whitespace from
		# the separator.
		argcount = 0;
		sub("[ 	]$", "", sep)

		printf("%s\n\t\t", sep) >> PFILE
	}
}

# proto_format --
#	Pretty-print a function prototype.
function proto_format(p, OUTPUT)
{
	printf("/*\n") >> OUTPUT;

	s = "";
	for (i = 1; i in p; ++i)
		s = s p[i];

	t = " * PUBLIC: "
	if (length(s) + length(t) < 80)
		printf("%s%s", t, s) >> OUTPUT;
	else {
		split(s, p, "__P");
		len = length(t) + length(p[1]);
		printf("%s%s", t, p[1]) >> OUTPUT

		n = split(p[2], comma, ",");
		comma[1] = "__P" comma[1];
		for (i = 1; i <= n; i++) {
			if (len + length(comma[i]) > 75) {
				printf("\n * PUBLIC:     ") >> OUTPUT;
				len = 0;
			}
			printf("%s%s", comma[i], i == n ? "" : ",") >> OUTPUT;
			len += length(comma[i]);
		}
	}
	printf("\n */\n") >> OUTPUT;
}
