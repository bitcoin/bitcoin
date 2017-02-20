#
# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#

BEGIN {
	if (source_file == "" || header_file == "") {
	    print "Usage: gen_msg.awk requires these variables to be set:";
	    print "\theader_file\t-- the message #include file being created";
	    print "\tsource_file\t-- the message source file being created";
	    exit;
	}
	CFILE=source_file;
	HFILE=header_file;
	maxmsg = 0;
}
/^[	]*PREFIX/ {
	prefix = $2;

	# Start .c files.
	printf("/* Do not edit: automatically built by gen_msg.awk. */\n\n") \
	    > CFILE
	printf("#include \"db_config.h\"\n\n") >> CFILE

	# Start .h file, make the entire file conditional.
	printf("/* Do not edit: automatically built by gen_msg.awk. */\n\n") \
	    > HFILE
	printf("#ifndef\t%s_AUTO_H\n#define\t%s_AUTO_H\n\n", prefix, prefix) \
	    >> HFILE;
	printf("/*\n") >> HFILE;
	printf(" * Message sizes are simply the sum of field sizes (not\n") \
	    >> HFILE;
	printf(" * counting variable size parts, when DBTs are present),\n") \
	    >> HFILE;
	printf(" * and may be different from struct sizes due to padding.\n") \
	    >> HFILE;
	printf(" */\n") >> HFILE;
}
/^[	]*INCLUDE/ {
	for (i = 2; i < NF; i++)
		printf("%s ", $i) >> CFILE;
	printf("%s\n", $i) >> CFILE;
}
/^[	]*BEGIN_MSG/ {
	if (in_begin) {
		print "Invalid format: missing END statement";
		exit;
	}
	in_begin = 1;
	nvars = 0;
	thismsg = $2;
	for (i = 2; i<= NF; i++) {
		if ($i == "alloc")
			alloc = 1;
		else if ($i == "check_length")
			check_length = 1;
		else if ($i == "version")
			version = 1;
	}

	base_name = sprintf("%s_%s", prefix, thismsg);
	typedef_name = sprintf("%s_args", base_name);
	msg_size_name = toupper(sprintf("%s_SIZE", base_name));
	max_name = toupper(sprintf("%s_MAXMSG_SIZE", prefix));
}
/^[	]*ARG/ {
	vars[nvars] = $2;
	types[nvars] = $3;
	if (types[nvars] == "DBT")
		has_dbt = 1;
	nvars++;
}
/^[	]*END/ {
	if (!in_begin) {
		print "Invalid format: missing BEGIN statement";
		exit;
	}
	if (nvars == 0) {
		printf("%s needs at least one field\n", thismsg);
		exit;
	}

	sum = 0;
	for (i = 0; i < nvars; i++)
		sum += type_length(types[i]);
	printf("#define\t%s\t%d\n", msg_size_name, sum) >> HFILE;
	if (sum > maxmsg)
		maxmsg = sum;

	printf("typedef struct _%s {\n", typedef_name) >> HFILE;
	for (i = 0; i < nvars; i++) {
		if (types[i] == "DB_LSN" || types[i] == "DBT")
			printf("\t%s\t\t%s;\n", types[i], vars[i]) >> HFILE;
		else
			printf("\t%s\t%s;\n", types[i], vars[i]) >> HFILE;
	}
	printf("} %s;\n\n", typedef_name) >> HFILE;

	emit_marshal();
	emit_unmarshal();
 
	# Reinitialize variables for next time.
	in_begin = 0;
	alloc = 0;
	check_length = 0;
	version = 0;
	has_dbt = 0;
}
END {
	# End the conditional for the HFILE
	printf("#define\t%s\t%d\n", max_name, maxmsg) >> HFILE;
	printf("#endif\n") >> HFILE;
}

# Length of fixed part of message.  Does not count variable-length data portion
# of DBT.
#
function type_length(type)
{
	if (type == "DB_LSN")
		return (8);
	if (type == "DBT" || type == "u_int32_t" || type == "db_pgno_t")
		return (4);
	if (type == "u_int16_t")
		return (2);
	printf("unknown field type: %s", type);
	exit(1);
}

function emit_marshal()
{
	pi = 1;
	if (check_length)
		p[pi++] = "int ";
	else
		p[pi++] = "void ";
	function_name = sprintf("%s_marshal", base_name);
	p[pi++] = function_name;
	p[pi++] = " __P((ENV *, ";
	if (version)
		p[pi++] = "u_int32_t, ";
	p[pi++] = sprintf("%s *, u_int8_t *", typedef_name);
	if (check_length)
		p[pi++] = ", size_t, size_t *";
	p[pi++] = "));";
	proto_format(p, CFILE);

	if (check_length)
		printf("int\n") >> CFILE;
	else
		printf("void\n") >> CFILE;
	printf("%s(env", function_name) >> CFILE;
	if (version)
		printf(", version") >> CFILE;
	printf(", argp, bp") >> CFILE;
	if (check_length)
		printf(", max, lenp") >> CFILE;
	printf(")\n") >> CFILE;

	printf("\tENV *env;\n") >> CFILE;
	if (version)
		printf("\tu_int32_t version;\n") >> CFILE;
	printf("\t%s *argp;\n", typedef_name) >> CFILE;
	printf("\tu_int8_t *bp;\n") >> CFILE;
	if (check_length)
		printf("\tsize_t *lenp, max;\n") >> CFILE;
	printf("{\n") >> CFILE;

	if (version)
		printf("\tint copy_only;\n") >> CFILE;
	if (check_length) {
		printf("\tu_int8_t *start;\n\n") >> CFILE;
		printf("\tif (max < %s", msg_size_name) >> CFILE;
		for (i = 0; i < nvars; i++)
			if (types[i] == "DBT")
				printf("\n\t    + (size_t)argp->%s.size", \
                                    vars[i]) >> CFILE;
		# add in dbt sizes
		printf(")\n") >> CFILE;
		printf("\t\treturn (ENOMEM);\n") >> CFILE;
		printf("\tstart = bp;\n\n") >> CFILE;
	}

	if (version) {
		printf("\tcopy_only = 0;\n") >> CFILE;
		printf("\tif (version < DB_REPVERSION_47)\n") >> CFILE;
		printf("\t\tcopy_only = 1;\n") >> CFILE;
	}
	for (i = 0; i < nvars; i++) {
		if (types[i] == "u_int32_t" || types[i] == "db_pgno_t") {
			if (version) {
				printf("\tif (copy_only) {\n") >> CFILE;
				printf(\
    "\t\tmemcpy(bp, &argp->%s, sizeof(u_int32_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int32_t);\n") >> CFILE;
				printf("\t} else\n\t") >> CFILE;
			}
			printf("\tDB_HTONL_COPYOUT(env, bp, argp->%s);\n", \
                            vars[i]) >> CFILE;
		} else if (types[i] == "u_int16_t") {
			if (version) {
				printf("\tif (copy_only) {\n") >> CFILE;
				printf(\
    "\t\tmemcpy(bp, &argp->%s, sizeof(u_int16_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int16_t);\n") >> CFILE;
				printf("\t} else\n\t") >> CFILE;
			}
			printf("\tDB_HTONS_COPYOUT(env, bp, argp->%s);\n", \
                            vars[i]) >> CFILE;
		} else if (types[i] == "DB_LSN") {
			if (version) {
				printf("\tif (copy_only) {\n") >> CFILE;
				printf(\
    "\t\tmemcpy(bp, &argp->%s.file, sizeof(u_int32_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int32_t);\n") >> CFILE;
				printf(\
    "\t\tmemcpy(bp, &argp->%s.offset, sizeof(u_int32_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int32_t);\n") >> CFILE;
				printf("\t} else {\n\t") >> CFILE;
			}
			printf("\tDB_HTONL_COPYOUT(env, bp, argp->%s.file);\n",\
                            vars[i]) >> CFILE;
			if (version)
				printf("\t") >> CFILE;
			printf( \
                            "\tDB_HTONL_COPYOUT(env, bp, argp->%s.offset);\n", \
                            vars[i]) >> CFILE;
			if (version)
				printf("\t}\n") >> CFILE;
		} else if (types[i] == "DBT") {
			if (version) {
				printf("\tif (copy_only) {\n") >> CFILE;
				printf(\
    "\t\tmemcpy(bp, &argp->%s.size, sizeof(u_int32_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int32_t);\n") >> CFILE;
				printf("\t} else\n\t") >> CFILE;
			}
			printf("\tDB_HTONL_COPYOUT(env, bp, argp->%s.size);\n",\
                            vars[i]) >> CFILE;
			printf("\tif (argp->%s.size > 0) {\n", vars[i]) \
                            >> CFILE;
			printf( \
                            "\t\tmemcpy(bp, argp->%s.data, argp->%s.size);\n", \
                            vars[i], vars[i]) >> CFILE;
			printf("\t\tbp += argp->%s.size;\n", vars[i]) >> CFILE;
			printf("\t}\n") >> CFILE;
		} else {
			printf("unknown field type: %s", types[i]);
			exit(1);
		}
	}

	if (check_length) {
		printf("\n\t*lenp = (size_t)(bp - start);\n") >> CFILE;
		printf("\treturn (0);\n") >> CFILE;
	}
	printf("}\n\n") >> CFILE;
}

function emit_unmarshal()
{
	pi = 1;
	p[pi++] = "int ";
	function_name = sprintf("%s_unmarshal", base_name);
	p[pi++] = function_name;
	p[pi++] = " __P((ENV *, ";
	if (version)
		p[pi++] = sprintf("u_int32_t, ");
	if (alloc)
		p[pi++] = sprintf("%s **, u_int8_t *, ", typedef_name);
	else
		p[pi++] = sprintf("%s *, u_int8_t *, ", typedef_name);
	p[pi++] = sprintf("size_t, u_int8_t **));");
	proto_format(p, CFILE);

	printf("int\n") >> CFILE;
	if (alloc)
		arg_name = "argpp";
	else
		arg_name = "argp";
	printf("%s(env, ", function_name) >> CFILE;
	if (version)
		printf("version, ") >> CFILE;
	printf("%s, bp, ", arg_name) >> CFILE;
	printf("max, nextp)\n") >> CFILE;
	printf("\tENV *env;\n") >> CFILE;
	if (version)
		printf("\tu_int32_t version;\n") >> CFILE;
	if (alloc)
		printf("\t%s **argpp;\n", typedef_name) >> CFILE;
	else
		printf("\t%s *argp;\n", typedef_name) >> CFILE;
	printf("\tu_int8_t *bp;\n") >> CFILE;
	printf("\tsize_t max;\n") >> CFILE;
	printf("\tu_int8_t **nextp;\n") >> CFILE;
	printf("{\n") >> CFILE;
	has_locals = 0;
	if (has_dbt) {
		printf("\tsize_t needed;\n") >> CFILE;
		has_locals = 1;
	}
	if (alloc) {
		printf("\t%s *argp;\n", typedef_name) >> CFILE;
		printf("\tint ret;\n") >> CFILE;
		has_locals = 1;
	}
	if (version) {
		printf("\tint copy_only;\n") >> CFILE;
		has_locals = 1;
	}
	if (has_locals)
		printf("\n") >> CFILE;

	# Check that input byte buffer is long enough.
	#
	if (has_dbt) {
		printf("\tneeded = %s;\n", msg_size_name) >> CFILE;
		printf("\tif (max < needed)\n") >> CFILE;
	} else 
		printf("\tif (max < %s)\n", msg_size_name) >> CFILE;
	printf("\t\tgoto too_few;\n") >> CFILE;

	if (alloc) {
		printf( \
              "\tif ((ret = __os_malloc(env, sizeof(*argp), &argp)) != 0)\n") \
		    >> CFILE;
		printf("\t\treturn (ret);\n\n") >> CFILE;
	}
	if (version) {
		printf("\tcopy_only = 0;\n") >> CFILE;
		printf("\tif (version < DB_REPVERSION_47)\n") >> CFILE;
		printf("\t\tcopy_only = 1;\n") >> CFILE;
	}
	
	for (i = 0; i < nvars; i++) {
		if (types[i] == "u_int32_t" || types[i] == "db_pgno_t") {
			if (version) {
				printf("\tif (copy_only) {\n") >> CFILE;
				printf(\
    "\t\tmemcpy(&argp->%s, bp, sizeof(u_int32_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int32_t);\n") >> CFILE;
				printf("\t} else\n\t") >> CFILE;
			}
			printf("\tDB_NTOHL_COPYIN(env, argp->%s, bp);\n", \
                            vars[i]) >> CFILE;
		} else if (types[i] == "u_int16_t") {
			if (version) {
				printf("\tif (copy_only) {\n") >> CFILE;
				printf(\
    "\t\tmemcpy(&argp->%s, bp, sizeof(u_int16_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int16_t);\n") >> CFILE;
				printf("\t} else\n\t") >> CFILE;
			}
			printf("\tDB_NTOHS_COPYIN(env, argp->%s, bp);\n", \
                            vars[i]) >> CFILE;
		} else if (types[i] == "DB_LSN") {
			if (version) {
				printf("\tif (copy_only) {\n") >> CFILE;
				printf(\
    "\t\tmemcpy(&argp->%s.file, bp, sizeof(u_int32_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int32_t);\n") >> CFILE;
				printf(\
    "\t\tmemcpy(&argp->%s.offset, bp, sizeof(u_int32_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int32_t);\n") >> CFILE;
				printf("\t} else {\n\t") >> CFILE;
			}
			printf("\tDB_NTOHL_COPYIN(env, argp->%s.file, bp);\n", \
                            vars[i]) >> CFILE;
			if (version)
				printf("\t") >> CFILE;
			printf( \
                            "\tDB_NTOHL_COPYIN(env, argp->%s.offset, bp);\n", \
                            vars[i]) >> CFILE;
			if (version)
				printf("\t}\n") >> CFILE;
		} else if (types[i] == "DBT") {
			if (version) {
				printf("\tif (copy_only) {\n") >> CFILE;
				printf(\
    "\t\tmemcpy(&argp->%s.size, bp, sizeof(u_int32_t));\n", vars[i]) >> CFILE;
				printf(\
    "\t\tbp += sizeof(u_int32_t);\n") >> CFILE;
				printf("\t} else\n\t") >> CFILE;
			}
			printf("\tDB_NTOHL_COPYIN(env, argp->%s.size, bp);\n", \
                            vars[i]) >> CFILE;
			printf("\targp->%s.data = bp;\n", vars[i]) >> CFILE;
			printf("\tneeded += (size_t)argp->%s.size;\n", \
                            vars[i]) >> CFILE;
			printf("\tif (max < needed)\n") >> CFILE;
			printf("\t\tgoto too_few;\n") >> CFILE;
			printf("\tbp += argp->%s.size;\n", vars[i]) >> CFILE;
		} else {
			printf("unknown field type: %s", types[i]);
			exit(1);
		}
	}

	printf("\n\tif (nextp != NULL)\n") >> CFILE;
	printf("\t\t*nextp = bp;\n") >> CFILE;
	if (alloc) {
		printf("\t*argpp = argp;\n") >> CFILE;
	}
	printf("\treturn (0);\n\n") >> CFILE;

	printf("too_few:\n") >> CFILE;
	printf("\t__db_errx(env,\n") >> CFILE;
	printf("\t    \"Not enough input bytes to fill a %s message\");\n", \
	    base_name) >> CFILE;
	printf("\treturn (EINVAL);\n") >> CFILE;
	printf("}\n\n") >> CFILE;
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
				printf("\n * PUBLIC:	") >> fp;
				len = 0;
			}
			printf("%s%s", comma[i], i == n ? "" : ",") >> fp;
			len += length(comma[i]) + 2;
		}
	}
	printf("\n */\n") >> fp;
	delete p;
}
