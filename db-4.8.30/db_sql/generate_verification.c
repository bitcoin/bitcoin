/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * This compilation unit contains functions related to the generation
 * of a test for the generated storage layer.
 */
#include "generation.h"

extern int maxbinsz; /* defined in buildpt.c */

#define	CASENUM 19
#define	CTYPENUM 4

char * data_set[CASENUM][CTYPENUM] = {
    {"\"0\"", "\"zero\"", "0", "0"},
    {"\"1\"", "\"one\"", "1", "1.1"},
    {"\"2\"", "\"two\"", "2", "2.2"},
    {"\"3\"", "\"three\"", "3", "3.3"},
    {"\"4\"", "\"four\"", "4", "4.4"},
    {"\"5\"", "\"five\"", "5", "5.5"},
    {"\"6\"", "\"six\"", "6", "6.6"},
    {"\"7\"", "\"seven\"", "7", "7.7"},
    {"\"8\"", "\"eight\"", "8", "8.8"},
    {"\"9\"", "\"nine\"", "9", "9.9"},
    {"\"A\"", "\"eleven\"", "11", "11.1"},
    {"\"B\"", "\"twenty-two\"", "22", "22.2"},
    {"\"C\"", "\"thirty-three\"", "33", "33.3"},
    {"\"D\"", "\"forty-four\"", "44", "44.4"},
    {"\"E\"", "\"fifty-five\"", "55", "55.5"},
    {"\"F\"", "\"sixty-six\"", "66", "66.6"},
    {"\"G\"", "\"seventy-seven\"", "77", "77.7"},
    {"\"H\"", "\"eighty-eight\"", "88", "88.8"},
    {"\"I\"", "\"ninety-nine\"", "99", "99.9"}};

static FILE *test_file;   /* stream for generated test code */
static int round = 0;
static int test_indent_level = 0;

void generate_verification(FILE *, char *);
static void pr_test(char *, ...);
static void pr_test_comment(char *, ...);
static void callback_function_enter_entop(ENTITY *);
static void callback_function_exit_entop(ENTITY *);
static void callback_function_attrop(ENTITY *, ATTRIBUTE *, int, int);
static void declare_record_instances_enter_entop(ENTITY *);
static void define_records_enter_entop(ENTITY *);
static void define_records_exit_entop(ENTITY *);
static void define_records_fields_attrop(ENTITY *, ATTRIBUTE *, int, int);

static void initialize_database_enter_entop(ENTITY *);
static void initialize_index_enter_entop(DB_INDEX *);
static void insertion_test_enter_entop(ENTITY *);
static void insertion_test_exit_entop(ENTITY *);
static void insertion_test_attrop(ENTITY *, ATTRIBUTE *, int, int);
static void retrieval_test_enter_entop(ENTITY *);
static void retrieval_test_exit_entop(ENTITY *);
static void retrieval_test_attrop(ENTITY *, ATTRIBUTE *, int, int);
static void invoke_full_iteration_enter_entop(ENTITY *);
static void invoke_full_iteration_exit_entop(ENTITY *);
static void invoke_query_iteration_idxop(DB_INDEX *);
static void deletion_test_enter_entop(ENTITY *);
static void deletion_test_exit_entop(ENTITY *);
static void close_secondary_test_idxop(DB_INDEX *);
static void close_primary_test_enter_entop(ENTITY *);
static void remove_secondary_test_idxop(DB_INDEX *);
static void remove_primary_test_enter_entop(ENTITY *);

/*
 * generate_verification is the test entry point in this module.  It
 * orchestrates the generation of the C code for tests.
 */
void
generate_verification(vfile, hfilename)
	FILE *vfile;
	char *hfilename;
{
	test_file = vfile;

	pr_test_comment(
"Simple test for a Berkeley DB implementation                               \n\
generated from SQL DDL by db_sql                                            \n\
");
	pr_test("#include <assert.h>\n");
	pr_test("#include <math.h>\n\n");

	pr_test("\n#include \"%s\"\n\n", hfilename);

	if (maxbinsz != 0) {
		pr_test_comment("Test data for raw binary types");
		pr_test("#define MAXBINSZ %d\n\n", maxbinsz);
		pr_test("char binary_data[%d][MAXBINSZ];\n", CASENUM);
	}

	pr_test("int count_iteration;\n\n");

	pr_test_comment("Test data for input record");
	iterate_over_entities(&define_records_enter_entop,
	    &define_records_exit_entop,
	    NULL);

	if (maxbinsz != 0) {
		pr_test_comment("A very simple binary comparison function");
		pr_test(
"int compare_binary(char *p, int len, int dem)                              \n\
{                                                                           \n\
 if (memcmp(p, binary_data[dem], len) == 0) {                               \n\
  return 0;                                                                 \n\
 } else {                                                                   \n\
  return 1;                                                                 \n\
 }                                                                          \n\
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
 int j;                                                                     \n\
 int ret;                                                                   \n\
 int occurence;                                                             \n\
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
		pr_test_comment("Fill the binary test data with random values");
		pr_test(
"for (i = 0; i < %d; i++)                                                   \n\
 for (j = 0; j < MAXBINSZ; j++)                                             \n\
  binary_data[i][j] = rand();                                               \n\
									    \n\
",
	    CASENUM);
	}

	pr_test_comment(
"Use the convenience method to initialize the environment.                  \n\
The initializations for each entity and environment can be                  \n\
done discretely if you prefer, but this is the easy way.");
	pr_test(
"ret = initialize_%s_environment();                                         \n\
if (ret != 0) {                                                             \n\
 printf(\"Initialize error\");                                              \n\
 return ret;                                                                \n\
}                                                                           \n\
									    \n\
",
		    the_schema.environment.name);

	pr_test("\n");
	pr_test_comment(
"Now that everything is initialized, insert some records                    \n\
into each database, retrieve and verify them");

	pr_test("for (i = 0; i < %d; i++) {\n", CASENUM);
	test_indent_level++;
	pr_test_comment(
"First, insert some records into each database using                        \n\
the ...insert_fields functions.  These functions take                       \n\
each field of the record as a separate argument");
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

	test_indent_level--;
	pr_test("}\n");

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

	for (round = 0; round < CASENUM; round++) {
		iterate_over_entities(&deletion_test_enter_entop,
		    &deletion_test_exit_entop,
		    NULL);
	}

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

	pr_test("\n");
	pr_test(
"if (ret == 0)                                                              \n\
 printf(\"*****WELL DONE!*****\\n\");                                       \n\
");

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
 * Return a data literal appropriate for the given type, to use as test value.
 * No duplicate values will be got in this function.
 */
static char *
data_value_for_type(ATTR_TYPE *t)
{
	char *c_type = t->c_type;
	char *bin_array = NULL;
	const char *bin_name = "binary_data[";
	int bin_len = 0;
	int count_bit;
	int quote;

	if (is_string(t)) {
		/* If the field is really short, use a tiny string */
		if (t->array_dim < 12)
			return (data_set[round][0]);
		return (data_set[round][1]);
	} else if (is_array(t)) {
		for (quote = round, count_bit = 1; quote > 0;
		    quote/=10, count_bit++);
		bin_len = strlen(bin_name) + count_bit + 2;
		bin_array = (char *)malloc(bin_len);
		memset(bin_array, 0, bin_len);
		snprintf(bin_array, bin_len, "%s%d%c", bin_name, round, ']');
		return (bin_array);
	} else if (strcmp(c_type, "char") == 0 ||
	    strcmp(c_type, "short") == 0 ||
	    strcmp(c_type, "int") == 0 ||
	    strcmp(c_type, "long") == 0) {
		return (data_set[round][2]);
	} else if (strcmp(c_type, "float") == 0 ||
	    strcmp(c_type, "double") == 0) {
		return (data_set[round][3]);
	} else {
		fprintf(stderr,
		    "Unexpected C type in schema: %s", c_type);
		assert(0);
	}
	return NULL; /*NOTREACHED*/
}

/*
 * This entity operation function is called by the attribute iterator
 * when producing test code that defines record instances.
 */
static void
define_records_enter_entop(ENTITY *e)
{
	pr_test("%s_data %s_record_array[] = {\n",
	    e->name, e->name);
	test_indent_level++;
	for (round = 0; round < CASENUM; round++) {
		pr_test("{");
		iterate_over_attributes(e, &define_records_fields_attrop);
		pr_test("}");
		if (round < CASENUM -1)
			pr_test(",");

		pr_test("\n");
	}
	test_indent_level--;
}
static void
define_records_exit_entop(ENTITY *e)
{
	COMPQUIET(e, NULL);
	pr_test("};\n\n");
}
static void
define_records_fields_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	int get_row_num;

	COMPQUIET(first, 0);

	if ((e->primary_key != a) && round > 9)
		get_row_num = round - 10;
	else
		get_row_num = round;

	if (is_array(a->type) && !is_string(a->type)) {
		pr_test("{0}");
	} else if (is_string(a->type)) {
		if (a->type->array_dim < 12)
		      pr_test(data_set[get_row_num][0]);
		else
		      pr_test(data_set[get_row_num][1]);
	} else if (strcmp(a->type->c_type, "char") == 0 ||
	    strcmp(a->type->c_type, "short") == 0 ||
	    strcmp(a->type->c_type, "int") == 0 ||
	    strcmp(a->type->c_type, "long") == 0) {
		pr_test(data_set[get_row_num][2]);
	} else if (strcmp(a->type->c_type, "float") == 0 ||
	    strcmp(a->type->c_type, "double") == 0) {
		pr_test(data_set[get_row_num][3]);
	} else {
		fprintf(stderr,
		    "Unexpected C type in schema: %s", a->type->c_type);
		assert(0);
	}
	if (!last)
		pr_test(", ");
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
	pr_test("count_iteration = 0;\n");
	pr_test(
"ret = %s_full_iteration(%s_dbp, &%s_iteration_callback_test,               \n\
  \"retrieval of %s record through full iteration\");\n",
	    e->name, e->name, e->name, e->name);
}

static void
invoke_full_iteration_exit_entop(ENTITY *e)
{
	pr_test(
"if (ret != 0) {                                                            \n\
 printf(\"Full Iteration Error\\n\");                                       \n\
 goto exit_error;                                                           \n\
}                                                                           \n\
printf(\"%s full iteration: %%d\\n\", count_iteration);                     \n\
assert(count_iteration == %d);                                              \n\
									    \n\
",
	    e->name, CASENUM);
}

/*
 * This index operation function is called by the attribute iterator
 * when producing test code that creates the secondary databases.
 */
static void
invoke_query_iteration_idxop(DB_INDEX *idx)
{
	ATTRIBUTE *a = idx->attribute;

	pr_test("for (i = 0; i < %d; i++) {\n", CASENUM);
	test_indent_level++;
	pr_test("count_iteration = 0;\n");
	pr_test("%s_query_iteration(%s_dbp, ",
	    idx->name, idx->name);
	pr_test("%s_record_array[i].%s",
	    idx->primary->name, a->name);
	pr_test(
	    ",\n   &%s_iteration_callback_test,                             \n\
   \"retrieval of %s record through %s query\");\n\n",
	    idx->primary->name, idx->primary->name, idx->name);
	pr_test(
"printf(\"%s_record_array[%%d].%s: %%d\\n\", i, count_iteration);\n",
	    idx->primary->name, a->name,
	    idx->primary->name, a->name);

	if (is_array(a->type) && !is_string(a->type)) {
		pr_test("assert(count_iteration == 1);\n");
	} else {
	       pr_test("occurence = 0;\n");
		pr_test("for (j = 0; j < %d; j++) {\n", CASENUM);
		test_indent_level++;

		if (is_string(a->type)) {
			pr_test(
"if (strcmp(%s_record_array[j].%s, %s_record_array[i].%s) == 0) {\n",
			    idx->primary->name, a->name,
			    idx->primary->name, a->name);
		} else if ((strcmp(a->type->c_type, "float") == 0) ||
		    (strcmp(a->type->c_type, "double") == 0)) {
			pr_test(
"if (fabs(%s_record_array[j].%s - %s_record_array[i].%s) <= 0.00001) {\n",
			    idx->primary->name, a->name,
			    idx->primary->name, a->name);
		} else {
			pr_test(
"if (%s_record_array[j].%s == %s_record_array[i].%s) {\n",
			    idx->primary->name, a->name,
			    idx->primary->name, a->name);
		}

		test_indent_level++;
		pr_test("occurence++;\n");
		test_indent_level--;
		pr_test("}\n");
		test_indent_level--;
		pr_test("}\n");
		pr_test("assert(count_iteration == occurence);\n");
	}

	test_indent_level--;
	pr_test("}\n\n");
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when generating insertion test code.
 */
static void
insertion_test_enter_entop(ENTITY *e)
{
	pr_test("ret = %s_insert_fields( %s_dbp, \n", e->name, e->name);
}

static void
insertion_test_exit_entop(ENTITY *e)
{
	pr_test(
"if (ret != 0) {                                                            \n\
 printf(\"ERROR IN INSERT NO.%%d record in %s_dbp\\n\", i);                 \n\
 goto exit_error;                                                           \n\
}                                                                           \n\
									    \n\
",
	     e->name);
}

static void
insertion_test_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	if (first)
		test_indent_level++;

	if (is_array(a->type) && !is_string(a->type))
		pr_test("binary_data[i]");
	else
		pr_test("%s_record_array[i].%s", e->name, a->name);

	if (!last)
		pr_test(", \n");
	else {
	       pr_test(");\n");
	       test_indent_level--;
	}
}

/*
 * This next group of index and attribute operation functions are
 * called by the attribute iterator when generating the iteration
 * callback function.
 */
static void
callback_function_enter_entop(ENTITY *e)
{
	pr_test(
"void %s_iteration_callback_test(void *msg, %s_data *%s_record)             \n\
{                                                                           \n\
",
	    e->name, e->name, e->name);
	test_indent_level++;
	pr_test(
"int i;                                                                     \n\
int same = 0;                                                               \n\
									    \n\
for (i = 0; i < %d; i++) {                                                  \n\
",
	    CASENUM);
	test_indent_level++;
}
static void
callback_function_exit_entop(ENTITY *e)
{
	COMPQUIET(e, NULL);

	pr_test(") {\n");
	test_indent_level--;
	pr_test(
"same = 1;                                                                  \n\
break;                                                                      \n\
");
	test_indent_level--;
	pr_test("}\n");
	test_indent_level--;
	pr_test("}\n\n");
	pr_test(
"if (same == 0)                                                             \n\
 assert(0);                                                                 \n\
else                                                                        \n\
 count_iteration++;                                                         \n\
");

	test_indent_level--;
	pr_test("}\n\n");
}
static void
callback_function_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	if (first)
		pr_test("if (");

	if (is_array(a->type) && !is_string(a->type)) {
		pr_test("(compare_binary(%s_record->%s, %s, i) == 0) ",
		    e->name, a->name, array_dim_name(e, a), a->name);
	} else if (is_string(a->type)) {
		pr_test(
		    "(strcmp(%s_record->%s, %s_record_array[i].%s) == 0) ",
		    e->name, a->name, e->name, a->name);
	} else if ((strcmp(a->type->c_type, "float") == 0) ||
	    (strcmp(a->type->c_type, "double") == 0)) {
		pr_test(
"(fabs(%s_record->%s - %s_record_array[i].%s) <= 0.00001) ",
		    e->name, a->name, e->name, a->name);
	} else {
		pr_test("(%s_record->%s == %s_record_array[i].%s) ",
		    e->name, a->name, e->name, a->name);
	}

	if (!last)
	       pr_test("&&\n");

	if (first)
	       test_indent_level += 2;
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when generating retrieval test code
 */
static void
retrieval_test_enter_entop(ENTITY *e)
{
	pr_test("ret = get_%s_data( %s_dbp, ", e->name, e->name);
	if (is_array(e->primary_key->type) && !is_string(e->primary_key->type))
	       pr_test("binary_data[i], ");
	else
		pr_test("%s_record_array[i].%s, ",
		    e->name, e->primary_key->name);
	pr_test("&%s_record);\n", e->name);
	pr_test(
"if (ret != 0) {                                                            \n\
 printf(\"ERROR IN RETRIEVE NO.%%d record in %s_dbp\\n\", i);               \n\
 goto exit_error;                                                           \n\
}                                                                           \n\
									    \n\
",
	    e->name);
}

static void
retrieval_test_exit_entop(ENTITY *e)
{
	COMPQUIET(e, NULL);
	pr_test("\n");
}

static void
retrieval_test_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	COMPQUIET(first, 0);
	COMPQUIET(last, 0);

	pr_test("assert(");

	if (is_array(a->type) && !is_string(a->type)) {
		pr_test("compare_binary(%s_record.%s, %s, i) == 0);\n",
		    e->name, a->name, array_dim_name(e, a));
	} else if (is_string(a->type)) {
		pr_test(
"strcmp(%s_record.%s, %s_record_array[i].%s) == 0);\n",
		    e->name, a->name, e->name, a->name);
	} else if ((strcmp(a->type->c_type, "float") == 0) ||
	    (strcmp(a->type->c_type, "double") == 0)) {
		pr_test(
"(fabs(%s_record.%s - %s_record_array[i].%s) <= 0.00001));\n",
		    e->name, a->name, e->name, a->name);
	} else {
		pr_test("%s_record.%s == %s_record_array[i].%s);\n",
		    e->name, a->name, e->name, a->name);
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
	ATTRIBUTE *a = e->primary_key;
	char *key;
	char * return_value;
	int i, column;

	key = data_value_for_type(a->type);
	return_value = "0";
	i = 0;
	column = 0;

	if (is_string(a->type)) {
		if (a->type->array_dim < 12)
			column = 0;
		else
			column = 1;
	} else if (is_array(a->type)) {
		column = CTYPENUM;
	} else if (strcmp(a->type->c_type, "char") == 0 ||
	    strcmp(a->type->c_type, "short") == 0 ||
	    strcmp(a->type->c_type, "int") == 0 ||
	    strcmp(a->type->c_type, "long") == 0) {
		column = 2;
	} else if (strcmp(a->type->c_type, "float") == 0 ||
	    strcmp(a->type->c_type, "double") == 0) {
		column = 3;
	} else {
		fprintf(stderr,
		    "Unexpected C type in schema: %s", a->type->c_type);
		assert(0);
	}

	if (column != CTYPENUM) {
		while (i < round) {
			if (strcmp(data_set[i][column], key) == 0) {
				return_value = "DB_NOTFOUND";
				break;
			}
			i++;
		}
	}

	pr_test(
"if (ret == %s)                                                             \n\
 ret = 0;                                                                   \n\
else {                                                                      \n\
 printf(\"ERROR IN DELETE NO.%d record in %s_dbp\\n\");                     \n\
 goto exit_error;                                                           \n\
}\n\n",
	    return_value, round + 1, e->name);
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
"if (%s_dbp != NULL)                                                        \n\
 %s_dbp->close(%s_dbp, 0);                                                  \n\
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
