/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * This compilation unit contains functions related to the generation
 * of a simple smoke test for the generated storage layer.
 */
#include "generation.h"

extern int maxbinsz; /* defined in buildpt.c */

static FILE *test_file;   /* stream for generated test code */
static int test_indent_level = 0;

static void pr_test(char *, ...);
static void pr_test_comment(char *, ...);
static void callback_function_enter_entop(ENTITY *);
static void callback_function_exit_entop(ENTITY *);
static void callback_function_attrop(ENTITY *, ATTRIBUTE *, int, int);
static void declare_record_instances_enter_entop(ENTITY *);
static void initialize_database_enter_entop(ENTITY *);
static void initialize_index_enter_entop(DB_INDEX *);
static void insertion_test_enter_entop(ENTITY *);
static void insertion_test_exit_entop(ENTITY *);
static void insertion_test_attrop(ENTITY *, ATTRIBUTE *, int, int);
static void retrieval_test_enter_entop(ENTITY *);
static void retrieval_test_exit_entop(ENTITY *);
static void retrieval_test_attrop(ENTITY *, ATTRIBUTE *, int, int);
static void invoke_full_iteration_enter_entop(ENTITY *);
static void invoke_full_iteration_exit_entop(ENTITY *e);
static void invoke_query_iteration_idxop(DB_INDEX *);
static void deletion_test_enter_entop(ENTITY *);
static void deletion_test_exit_entop(ENTITY *e);
static void close_secondary_test_idxop(DB_INDEX *);
static void close_primary_test_enter_entop(ENTITY *);
static void remove_secondary_test_idxop(DB_INDEX *);
static void remove_primary_test_enter_entop(ENTITY *);

/*
 * Generate_test is the sole entry point in this module.  It
 * orchestrates the generation of the C code for the smoke test.
 */
void
generate_test(tfile, hfilename)
	FILE *tfile;
	char *hfilename;
{
	test_file = tfile;

	pr_test_comment(
"Simple test for a Berkeley DB implementation                               \n\
generated from SQL DDL by db_sql                                            \n\
");

	pr_test("\n#include \"%s\"\n\n", hfilename);

	if (maxbinsz != 0) {
        	pr_test_comment("Test data for raw binary types");
        	pr_test("#define MAXBINSZ %d\n", maxbinsz);
        	pr_test("char binary_data[MAXBINSZ];\n\n");
        	pr_test_comment("A very simple binary comparison function");
		pr_test(
"char * compare_binary(char *p, int len)                                    \n\
{                                                                           \n\
 if (memcmp(p, binary_data, len) == 0)                                      \n\
  return \"*binary values match*\";                                         \n\
 return \"*binary values don't match*\";                                    \n\
}                                                                           \n\
                                                                            \n\
");
	}

	pr_test_comment(
"These are the iteration callback functions.  One is defined per            \n\
database(table).  They are used for both full iterations and for            \n\
secondary index queries.  When a retrieval returns multiple records,        \n\
as in full iteration over an entire database, one of these functions        \n\
is called for each record found");

	iterate_over_entities(&callback_function_enter_entop,
	    &callback_function_exit_entop,
	    &callback_function_attrop);

	pr_test(
"                                                                           \n\
main(int argc, char **argv)                                                 \n\
{                                                                           \n\
 int i;                                                                     \n\
 int ret; 								    \n\
									    \n\
");
	test_indent_level++;
	iterate_over_entities(&declare_record_instances_enter_entop,
	    NULL,
	    NULL);
	    
	iterate_over_entities(&initialize_database_enter_entop,
	    NULL,
	    NULL);

	iterate_over_indexes(&initialize_index_enter_entop);

	pr_test("\n");
	
	if (maxbinsz != 0) {
		pr_test_comment(
                    "Fill the binary test data with random values");
		pr_test(
"for (i = 0; i < MAXBINSZ; i++) binary_data[i] = rand();\n\n");
	}

	pr_test_comment(
"Use the convenience method to initialize the environment.                  \n\
The initializations for each entity and environment can be                  \n\
done discretely if you prefer, but this is the easy way.");
	pr_test(
"ret = initialize_%s_environment();                                         \n\
if (ret != 0){                                                              \n\
printf(\"Initialize error\");                                               \n\
return ret;                                                                 \n\
}                                                                           \n\
                                                                            \n\
",
		    the_schema.environment.name);

	pr_test_comment(
"Now that everything is initialized, insert a single                        \n\
record into each database, using the ...insert_fields                       \n\
functions.  These functions take each field of the                          \n\
record as a separate argument");

	iterate_over_entities(&insertion_test_enter_entop,
	    &insertion_test_exit_entop,
	    &insertion_test_attrop);

	pr_test("\n");
	pr_test_comment(
"Next, retrieve the records just inserted, looking them up                  \n\
by their key values");

	iterate_over_entities(&retrieval_test_enter_entop,
	    &retrieval_test_exit_entop,
	    &retrieval_test_attrop);

	pr_test("\n");
	pr_test_comment(
"Now try iterating over every record, using the ...full_iteration           \n\
functions for each database.  For each record found, the                    \n\
appropriate ...iteration_callback_test function will be invoked             \n\
(these are defined above).");

	iterate_over_entities(&invoke_full_iteration_enter_entop,
	    &invoke_full_iteration_exit_entop,
	    NULL);

	pr_test("\n");
	pr_test_comment(
"For the secondary indexes, query for the known keys.  This also            \n\
results in the ...iteration_callback_test function's being called           \n\
for each record found.");

	iterate_over_indexes(&invoke_query_iteration_idxop);

	pr_test("\n");
	pr_test_comment(
"Now delete a record from each database using its primary key.");

	iterate_over_entities(&deletion_test_enter_entop,
	    &deletion_test_exit_entop,
	    NULL);

	pr_test("\n");
	test_indent_level--;
        pr_test("exit_error:\n");
        test_indent_level++;
	
	pr_test_comment("Close the secondary index databases");
	iterate_over_indexes(&close_secondary_test_idxop);

	pr_test("\n");
	pr_test_comment("Close the primary databases");
	iterate_over_entities(&close_primary_test_enter_entop,
	    NULL,
	    NULL);

	pr_test("\n");
	pr_test_comment("Delete the secondary index databases");
	iterate_over_indexes(&remove_secondary_test_idxop);

	pr_test("\n");
	pr_test_comment("Delete the primary databases");
	iterate_over_entities(&remove_primary_test_enter_entop,
	    NULL,
	    NULL);

	pr_test("\n");
	pr_test_comment("Finally, close the environment");
	pr_test("%s_envp->close(%s_envp, 0);\n",
	    the_schema.environment.name, the_schema.environment.name);

        pr_test("return ret;\n");
	test_indent_level--;
	pr_test("}\n");
}

/*
 * Emit a printf-formatted string into the test code file.
 */
static void
pr_test(char *fmt, ...)
{
	va_list ap;
	char *s;
	static int enable_indent = 1;

	s = prepare_string(fmt, 
	    enable_indent ? test_indent_level : 0,
	    0);

	va_start(ap, fmt);
	vfprintf(test_file, s, ap);
	va_end(ap);

        /*
	 * If the last char emitted was a newline, enable indentation
	 * for the next time.
	 */
	if (s[strlen(s) - 1] == '\n')
		enable_indent = 1;
	else
		enable_indent = 0;
}

/*
 * Emit a formatted comment into the test file.
 */
static void
pr_test_comment(char *fmt, ...)
{
	va_list ap;
	char *s;

	s = prepare_string(fmt, test_indent_level, 1);

	va_start(ap, fmt);
	vfprintf(test_file, s, ap);
	va_end(ap);
}

/*
 * Return the appropriate printf format string for the given type.
 */
static char *
format_string_for_type(ATTR_TYPE *t)
{
	char *c_type = t->c_type;

	if (is_array(t)) {
		return ("%s");
	} else if (strcmp(c_type, "char") == 0 ||
	    strcmp(c_type, "short") == 0 ||
	    strcmp(c_type, "int") == 0) {
		return("%d");
	} else  if (strcmp(c_type, "long") == 0) {
		return("%ld");
	} else if (strcmp(c_type, "float") == 0) {
		return("%f");
	} else if (strcmp(c_type, "double") == 0) {
		return("%lf");
	} else {
		fprintf(stderr,
		    "Unexpected C type in schema: %s", c_type);
		assert(0);
	}
	return NULL; /*NOTREACHED*/
}

/*
 * Return a data literal appropriate for the given type, to use as test data.
 */
static char *
data_value_for_type(ATTR_TYPE *t)
{
	char *c_type = t->c_type;

	if (is_string(t)) {
		/* If the field is really short, use a tiny string */
		if (t->array_dim < 12)
			return("\"n\"");
		return("\"ninety-nine\"");
	} else if (is_array(t)) {
		return("binary_data");
	} else if (strcmp(c_type, "char") == 0 ||
	    strcmp(c_type, "short") == 0 ||
	    strcmp(c_type, "int") == 0 ||
	    strcmp(c_type, "long") == 0) {
		return("99");
	} else if (strcmp(c_type, "float") == 0 ||
	    strcmp(c_type, "double") == 0) {
		return("99.5");
	} else {
		fprintf(stderr,
		    "Unexpected C type in schema: %s", c_type);
		assert(0);
	}
	return NULL; /*NOTREACHED*/
}

/*
 * This entity operation function is called by the attribute iterator
 * when producing test code that declares record instances.
 */
static void
declare_record_instances_enter_entop(ENTITY *e)
{
	pr_test("%s_data %s_record;\n", e->name, e->name);
}

/*
 * This entity operation function is called by the attribute iterator
 * when producing test code that initialized database handle.
 */
static void
initialize_database_enter_entop(ENTITY *e)
{
	pr_test("%s_dbp = NULL;\n", e->name);
}

/*
 * This entity operation function is called by the attribute iterator
 * when producing test code that initialized index handle.
 */
static void
initialize_index_enter_entop(DB_INDEX *idx)
{
	pr_test("%s_dbp = NULL;\n", idx->name);
}

static void
invoke_full_iteration_enter_entop(ENTITY *e)
{
	pr_test(
"ret = %s_full_iteration(%s_dbp, &%s_iteration_callback_test,               \n\
  \"retrieval of %s record through full iteration\");\n",
	    e->name, e->name, e->name, e->name);
}
static void
invoke_full_iteration_exit_entop(ENTITY *e)
{
        COMPQUIET(e, NULL);
	pr_test(
"if (ret != 0){                                                             \n\
 printf(\"Full Iteration Error\\n\");                                       \n\
 goto exit_error;                                                           \n\
}                                                                           \n\
                                                                            \n\
");
}

/*
 * This index operation function is called by the attribute iterator
 * when producing test code that creates the secondary databases.
 */
static void
invoke_query_iteration_idxop(DB_INDEX *idx)
{
	ATTR_TYPE *key_type = idx->attribute->type;

	pr_test("%s_query_iteration(%s_dbp, ",
	    idx->name, idx->name);

	pr_test("%s", data_value_for_type(key_type));
	
	pr_test(
",\n  &%s_iteration_callback_test,                                          \n\
  \"retrieval of %s record through %s query\");\n",
		    idx->primary->name, idx->primary->name, idx->name);
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when generating insertion test code.
 */
static void
insertion_test_enter_entop(ENTITY *e)
{
	pr_test("ret = %s_insert_fields( %s_dbp, ", e->name, e->name);
}
static void
insertion_test_exit_entop(ENTITY *e)
{
	COMPQUIET(e, NULL);
	pr_test(");\n");
        pr_test(
"if (ret != 0){                                                            \n\
 printf(\"Insert error\\n\");                                              \n\
 goto exit_error;                                                          \n\
}                                                                          \n\
                                                                           \n\
");
}
static void
insertion_test_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	COMPQUIET(e, NULL);
	COMPQUIET(first, 0);

	pr_test("%s", data_value_for_type(a->type));
	if (!last)
		pr_test(", ");
}

/*
 * This next group of index and attribute operation functions are
 * called by the attribute iterator when generating the iteration
 * callback function.
 */
static void
callback_function_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	COMPQUIET(first, 0);
	COMPQUIET(last, 0);

	pr_test("printf(\"%s->%s: ", e->name, a->name);
	
	pr_test("%s", format_string_for_type(a->type));

	if (is_array(a->type) && !is_string(a->type)) {
		pr_test("\\n\", compare_binary(%s_record->%s, %s));\n", 
		    e->name, a->name, array_dim_name(e, a) );
	} else {
		pr_test("\\n\", %s_record->%s);\n", e->name, a->name);
	}
}
static void
callback_function_enter_entop(ENTITY *e)
{
	pr_test(
"                                                                           \n\
void %s_iteration_callback_test(void *msg, %s_data *%s_record)              \n\
{                                                                           \n\
 printf(\"In iteration callback, message is: %%s\\n\", (char *)msg);\n\n",
	    e->name, e->name, e->name);
	
	test_indent_level++;
}
static void
callback_function_exit_entop(ENTITY *e)
{
	COMPQUIET(e, NULL);

	test_indent_level--;
	pr_test("}\n\n");
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when generating retrieval test code
 */
static void
retrieval_test_enter_entop(ENTITY *e)
{
        pr_test("\nprintf(\"Retrieval of %s record by key\\n\");\n", e->name);
	pr_test("ret = get_%s_data( %s_dbp, %s, &%s_record);\n\n",
	    e->name, e->name, 
	    data_value_for_type(e->primary_key->type), e->name);
}
static void
retrieval_test_exit_entop(ENTITY *e)
{
	COMPQUIET(e, NULL);
        pr_test(
"if (ret != 0)                                                              \n\
{                                                                           \n\
 printf(\"Retrieve error\\n\");                                             \n\
 goto exit_error;                                                           \n\
}                                                                           \n\
                                                                            \n\
");
}

static void
retrieval_test_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	COMPQUIET(first, 0);
	COMPQUIET(last, 0);

	pr_test("printf(\"%s.%s: ", e->name, a->name);

	pr_test("%s", format_string_for_type(a->type));

	if (is_array(a->type) && !is_string(a->type)) {
		pr_test("\\n\", compare_binary(%s_record.%s, %s));\n", 
		    e->name, a->name, array_dim_name(e, a) );
	} else {
		pr_test("\\n\", %s_record.%s);\n", e->name, a->name);
	}
}

/*
 * This entity operation function is
 * called by the attribute iterator when generating deletion test code.
 */
static void
deletion_test_enter_entop(ENTITY *e)
{
        pr_test("ret = delete_%s_key( %s_dbp, %s);\n",
            e->name, e->name, data_value_for_type(e->primary_key->type));
}
static void
deletion_test_exit_entop(ENTITY *e)
{
        COMPQUIET(e, NULL);
	pr_test(
"if (ret != 0) {                                                            \n\
 printf(\"Delete error\\n\");                                               \n\
 goto exit_error;                                                           \n\
}                                                                           \n\
                                                                            \n\
");
}

/* This entity operation function generates primary database closures. */
static void
close_primary_test_enter_entop(ENTITY *e)
{
        pr_test(
"if (%s_dbp != NULL)                                                        \n\
 %s_dbp->close(%s_dbp, 0);                                                  \n\
									    \n\
",
 	    e->name, e->name, e->name);
}

/* This entity operation function generates secondary database closures. */
static void
close_secondary_test_idxop(DB_INDEX *idx)
{
        pr_test(
"if (%s_dbp != NULL)                                                       \n\
 %s_dbp->close(%s_dbp, 0);                                                 \n\
                                                                           \n\
", 
	    idx->name, idx->name, idx->name);
}

/* This entity operation function generates primary database closures. */
static void
remove_primary_test_enter_entop(ENTITY *e)
{
	pr_test("remove_%s_database(%s_envp);\n", e->name,
	    the_schema.environment.name);
}

/* This entity operation function generates secondary database closures. */
static void
remove_secondary_test_idxop(DB_INDEX *idx)
{
	pr_test("remove_%s_index(%s_envp);\n", idx->name,
	    the_schema.environment.name);
}
