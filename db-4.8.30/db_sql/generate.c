/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * This compilation unit contains functions related to generating the
 * storage layer implementation in "C".
 */
#include "generation.h"

static int header_indent_level = 0;
static int code_indent_level = 0;

static FILE *header_file; /* stream for generated include file */
static FILE *code_file;   /* stream for generated bdb code */

#define	SEPARATE_HEADER (header_file != code_file)

void generate_test(FILE *tfile, char *hfilename);
void generate_verification(FILE *vfile, char *hfilename);

static void check_constraints();
static void generate_config_defines();
static void generate_schema_structs();
static void generate_iteration_callback_typedefs();
static void generate_environment();
static void generate_db_creation_functions();
static void generate_db_removal_functions();
static void generate_insertion_functions();
static void generate_fetch_functions();
static void generate_deletion_functions();
static void generate_full_iterations();
static void generate_index_functions();
static void generate_initialization();
static void pr_header(char *, ...);
static void pr_header_comment(char *, ...);
static void pr_code(char *, ...);
static void pr_code_comment(char *, ...);

/*
 * Emit a printf-formatted string into the main code file
 */
static void
pr_code(char *fmt, ...)
{
	va_list ap;
	char *s;
	static int enable_indent = 1;

	s = prepare_string(fmt,
	    enable_indent ? code_indent_level : 0,
	    0);

	va_start(ap, fmt);
	vfprintf(code_file, s, ap);
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
 * Emit a formatted comment into the main code file.
 */
static void
pr_code_comment(char *fmt, ...)
{
	va_list ap;
	char *s;

	s = prepare_string(fmt, code_indent_level, 1);

	va_start(ap, fmt);
	vfprintf(code_file, s, ap);
	va_end(ap);
}

/*
 * Emit a printf-formatted string into the header file.
 */
static void
pr_header(char *fmt, ...)
{
	va_list ap;
	char *s;
	static int enable_indent = 1;

	s = prepare_string(fmt,
	    enable_indent ? header_indent_level : 0,
	    0);

	va_start(ap, fmt);
	vfprintf(header_file, s, ap);
	va_end(ap);

	/*
	 * if the last char emitted was a newline, enable indentation
	 * for the next time
	 */
	if (s[strlen(s) - 1] == '\n')
		enable_indent = 1;
	else
		enable_indent = 0;
}

/*
 * Emit a formatted comment into the header file.
 */
static void
pr_header_comment(char *fmt, ...)
{
	va_list ap;
	char *s;

	s = prepare_string(fmt, header_indent_level, 1);

	va_start(ap, fmt);
	vfprintf(header_file, s, ap);
	va_end(ap);
}

static void
generate_config_defines()
{
	static char *cache_size_comment =
"Cache size constants, specified in a                                       \n\
hint comment in the original DDL";

	if (the_schema.environment.cache_size > 0) {
		pr_header_comment(cache_size_comment);
		pr_header("#define %s_CACHE_SIZE_GIGA %lu\n",
		    upcase_env_name(&the_schema.environment),
		    the_schema.environment.cache_size / GIGA);
		pr_header("#define %s_CACHE_SIZE_BYTES %lu\n\n",
		    upcase_env_name(&the_schema.environment),
		    the_schema.environment.cache_size % GIGA);
	}
}

static void
generate_environment() {
	static char *create_env_function_template_part1 =
"                                                                           \n\
int create_%s_env(DB_ENV **envpp)                                           \n\
{                                                                           \n\
 int ret, flags;                                                            \n\
 char *env_name = \"./%s\";                                                 \n\
									    \n\
 if ((ret = db_env_create(envpp, 0)) != 0) {                                \n\
  fprintf(stderr, \"db_env_create: %%s\", db_strerror(ret));                \n\
  return 1;                                                                 \n\
 }                                                                          \n\
									    \n\
 (*envpp)->set_errfile(*envpp, stderr);                                     \n\
 flags = DB_CREATE | DB_INIT_MPOOL;                                         \n\
									    \n\
";
	static char *create_env_function_template_part2 =
"                                                                           \n\
 if ((ret = (*envpp)->open(*envpp, env_name, flags, 0)) != 0) {             \n\
  (*envpp)->err(*envpp, ret, \"DB_ENV->open: %%s\", env_name);              \n\
  return 1;                                                                 \n\
 }                                                                          \n\
 return 0;                                                                  \n\
}                                                                           \n\
									    \n\
";
	static char *set_cachesize_template =
" (*envpp)->set_cachesize(*envpp,                                           \n\
 %s_CACHE_SIZE_GIGA,                                                        \n\
 %s_CACHE_SIZE_BYTES, 1);                                                   \n\
									    \n\
";

	pr_code(create_env_function_template_part1,
	    the_schema.environment.name,
	    the_schema.environment.name);

	if (the_schema.environment.cache_size > 0) {
		pr_code(set_cachesize_template,
		    upcase_env_name(&the_schema.environment),
		    upcase_env_name(&the_schema.environment));
	}

	pr_code(create_env_function_template_part2);

	if (SEPARATE_HEADER) {
		pr_header_comment(
			"The environment creation/initialization function");

		pr_header("int create_%s_env(DB_ENV **envpp);\n\n",
		    the_schema.environment.name);
	}
}

/*
 * This entity operation functions is called by the entity iterator
 * when generating the db creation functions.
 */
static void
db_create_enter_entop(ENTITY *e)
{
	static char *create_db_template =
"int create_%s_database(DB_ENV *envp, DB **dbpp)                            \n\
{		                                                            \n\
 return create_database(envp, \"%s.db\", dbpp,                              \n\
  DB_CREATE, %s, 0, %s);                                                    \n\
}                                                                           \n\
									    \n\
";
	pr_code(create_db_template, e->name, e->name, e->dbtype,
	    (strcmp(e->dbtype, "DB_BTREE") == 0 ?
		custom_comparator_for_type(e->primary_key->type) : "NULL"));

	if (SEPARATE_HEADER)
		pr_header(
		    "int create_%s_database(DB_ENV *envp, DB **dbpp);\n\n",
		    e->name);
}

static void
generate_db_creation_functions()
{

	static char *comparator_function_comment =
"These are custom comparator functions for integer keys.  They are          \n\
needed to make integers sort well on little-endian architectures,           \n\
such as x86. cf. discussion of btree comparators in 'Getting Started        \n\
with Data Storage' manual.";
	static char *comparator_functions =
"static int                                                                 \n\
compare_int(DB *dbp, const DBT *a, const DBT *b)                            \n\
{                                                                           \n\
 int ai, bi;                                                                \n\
									    \n\
 memcpy(&ai, a->data, sizeof(int));                                         \n\
 memcpy(&bi, b->data, sizeof(int));                                         \n\
 return (ai - bi);                                                          \n\
}                                                                           \n\
									    \n\
int                                                                         \n\
compare_long(DB *dbp, const DBT *a, const DBT *b)                           \n\
{                                                                           \n\
 long ai, bi;                                                               \n\
									    \n\
 memcpy(&ai, a->data, sizeof(long));                                        \n\
 memcpy(&bi, b->data, sizeof(long));                                        \n\
 return (ai - bi);                                                          \n\
}                                                                           \n\
									    \n\
";
	static char *create_db_function_comment =
"A generic function for creating and opening a database";

	static char *generic_create_db_function =
"int                                                                        \n\
create_database(DB_ENV *envp,                                               \n\
 char *db_name,                                                             \n\
 DB **dbpp,                                                                 \n\
 int flags,                                                                 \n\
 DBTYPE type,                                                               \n\
 int moreflags,                                                             \n\
 int (*comparator)(DB *, const DBT *, const DBT *))                         \n\
{                                                                           \n\
 int ret;                                                                   \n\
 FILE *errfilep;                                                            \n\
									    \n\
 if ((ret = db_create(dbpp, envp, 0)) != 0) {                               \n\
  envp->err(envp, ret, \"db_create\");                                      \n\
  return ret;                                                               \n\
 }                                                                          \n\
									    \n\
 if (moreflags != 0)                                                        \n\
  (*dbpp)->set_flags(*dbpp, moreflags);                                     \n\
									    \n\
 if (comparator != NULL)                                                    \n\
  (*dbpp)->set_bt_compare(*dbpp, comparator);                               \n\
									    \n\
 envp->get_errfile(envp, &errfilep);                                        \n\
 (*dbpp)->set_errfile(*dbpp, errfilep);                                     \n\
 if ((ret = (*dbpp)->open(*dbpp, NULL, db_name,                             \n\
  NULL, type, flags, 0644)) != 0) {                                         \n\
  (*dbpp)->err(*dbpp, ret, \"DB->open: %%s\", db_name);                     \n\
  return ret;                                                               \n\
 }                                                                          \n\
									    \n\
 return 0;                                                                  \n\
}                                                                           \n\
									    \n\
";

	pr_code_comment(comparator_function_comment);
	pr_code(comparator_functions);
	pr_code_comment(create_db_function_comment);
	pr_code(generic_create_db_function);

	pr_header_comment("Database creation/initialization functions");
	iterate_over_entities(&db_create_enter_entop,
	    NULL,
	    NULL);
}

/*
 * This entity operation function is called by the entity iterator
 * when generating the db removal functions.
 */
static void
db_remove_enter_entop(ENTITY *e)
{
	static char *remove_db_template =
"                                                                           \n\
int remove_%s_database(DB_ENV *envp)                                        \n\
{                                                                           \n\
 return envp->dbremove(envp, NULL, \"%s.db\", NULL, 0);                     \n\
}                                                                           \n\
									    \n\
";
	pr_code(remove_db_template, e->name, e->name);

	if (SEPARATE_HEADER)
		pr_header("int remove_%s_database(DB_ENV *envp);\n\n",
		    e->name);
}

static void
generate_db_removal_functions() {

	pr_header_comment("Database removal functions");
	iterate_over_entities(&db_remove_enter_entop,
	    NULL,
	    NULL);
}

/*
 * This group of entity and attribute operation functions are
 * called by the attribute iterator when defining constants for any
 * array type attributes.
 */
static void
array_dim_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	COMPQUIET(first, 0);
	COMPQUIET(last, 0);

	if (is_array(a->type))
		pr_header("#define %s %d\n", array_dim_name(e, a),
		    a->type->array_dim);

}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when defining structs
 * corresponding to the entities.
 */
static void
structs_enter_entop(ENTITY *e)
{
	pr_header("typedef struct _%s_data {\n", e->name);
	header_indent_level++;
}

static void
structs_exit_entop(ENTITY *e)
{
	header_indent_level--;
	pr_header("} %s_data;\n\n", e->name);
	pr_header("%s_data %s_data_specimen;\n\n", e->name, e->name);
}

static void
structs_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	COMPQUIET(first, 0);
	COMPQUIET(last, 0);
	pr_header("%s\t%s;\n", a->type->c_type, decl_name(e, a));
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when defining constants for
 * serialized lengths of records.
 */
static void
serialized_length_enter_entop(ENTITY *e)
{
	pr_header("#define %s (", serialized_length_name(e));
}

static void
serialized_length_exit_entop(ENTITY *e)
{
	COMPQUIET(e, NULL);
	pr_header(")\n\n");
}

static void
serialized_length_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	static char *sizeof_template = "%ssizeof(%s_data_specimen.%s)";
	
	if (strcmp(e->primary_key->name, a->name) == 0)
		pr_header("%s0", first ? "" : " + \\\n\t");
	else 
		pr_header(sizeof_template, first ? "" : " + \\\n\t",
		    e->name, a->name);
}

/*
 * Call the iterator twice, once for generating the array dimension
 * #defines, and once for the structs.
 */
static void
generate_schema_structs()
{
	pr_header_comment("Array size constants.");

	/* Emit the array dimension definitions. */
	iterate_over_entities(NULL,
	    NULL,
	    &array_dim_attrop);

	pr_header_comment("Data structures representing the record layouts");

	/* Emit the structs that represent the schema. */
	iterate_over_entities(&structs_enter_entop,
	    &structs_exit_entop,
	    &structs_attrop);

	pr_header_comment(
"Macros for the maximum length of the                                       \n\
records after serialization.  This is                                       \n\
the maximum size of the data that is stored");

	/* Emit the serialized length #defines. */
	iterate_over_entities(&serialized_length_enter_entop,
	    &serialized_length_exit_entop,
	    &serialized_length_attrop);
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when defining the entity
 * serialization functions.
 */
static void
serialize_function_enter_entop(ENTITY *e)
{
	static char *header_template =
"                                                                           \n\
int serialize_%s_data(%s_data *%sp,					    \n\
 char *buffer,								    \n\
 DBT *key_dbt,								    \n\
 DBT *data_dbt)                                                             \n\
{                                                                           \n\
 size_t len;                                                                \n\
 char *p;                                                                   \n\
									    \n\
 memset(buffer, 0, %s);                                                     \n\
 p = buffer;                                                                \n\
 									    \n\
 key_dbt->data = %s%sp->%s;						    \n\
 key_dbt->size = ";
	pr_code(header_template, e->name, e->name, e->name,  
	    serialized_length_name(e),
	    is_array(e->primary_key->type) ? "" : "&",
	    e->name, e->primary_key->name);
	
	if (is_array(e->primary_key->type) &&
	    !is_string((e->primary_key->type)))
		pr_code(array_dim_name(e, e->primary_key));
	else 
		pr_code("%s(%sp->%s)%s", 
		    is_string(e->primary_key->type) ? "strlen" : "sizeof",
		    e->name, e->primary_key->name,
		    is_string(e->primary_key->type) ? "+ 1" : "");
	
	pr_code(";\n");
	    
	code_indent_level++;
}
static void
serialize_function_exit_entop(ENTITY *e)
{
	COMPQUIET(e, NULL);
	code_indent_level--;
	pr_code(
"                                                                           \n\
 data_dbt->data = buffer;						    \n\
 data_dbt->size = p - buffer;						    \n\
}                                                                           \n\
"
		);
}
static void
serialize_function_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	static char *copy_strlen_template =
"                                                                           \n\
len = strlen(%sp->%s) + 1;                                                  \n\
assert(len <= %s);                                                          \n\
memcpy(p, %sp->%s, len);                                                    \n\
p += len;                                                                   \n\
									    \n\
";

	COMPQUIET(first, 0);
	COMPQUIET(last, 0);

	/* Primary key is not stored in data. */
	if (strcmp(e->primary_key->name, a->name) != 0) {
		/* 
		 * For strings, we store only the length calculated by 
		 * strlen.
		 */ 		 
		if (is_string(a->type)) {
			pr_code(copy_strlen_template,
			    e->name, a->name,
			    array_dim_name(e, a),
			    e->name, a->name);
		} else {
			pr_code("memcpy(p, %s%sp->%s, sizeof(%sp->%s));\n",
			    is_array(a->type) ? "" : "&",
			    e->name, a->name, e->name, a->name);
			pr_code("p += sizeof(%sp->%s);\n", e->name, a->name);
		}
	}
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when defining the entity
 * deserialization functions.
 */
static void
deserialize_function_enter_entop(ENTITY *e)
{
	static char *header_template =
"									    \n\
void deserialize_%s_data(char *buffer, %s_data *%sp)                        \n\
{                                                                           \n\
 size_t len;                                                                \n\
									    \n\
";
	pr_code(header_template, e->name, e->name, e->name,
	    e->name, e->name);
	code_indent_level++;
}

static void
deserialize_function_exit_entop(ENTITY *e)
{
	COMPQUIET(e, NULL);
	code_indent_level--;
	pr_code("}\n\n");
}

static void
deserialize_function_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	/* strings are stored with null termination in the buffer */
	static char *copy_strlen_template =
"len = strlen(buffer) + 1;                                                  \n\
assert(len <= %s);                                                          \n\
memcpy(%sp->%s, buffer, len);                                               \n\
buffer += len;                                                              \n\
									    \n\
";

	COMPQUIET(first, 0);
	COMPQUIET(last, 0);

	if (strcmp(e->primary_key->name, a->name) != 0) {
		if (is_string(a->type)) {
			pr_code (copy_strlen_template,
			    array_dim_name(e, a),
			    e->name, a->name);
		} else {
			pr_code(
			    "memcpy(%s%sp->%s, buffer, sizeof(%sp->%s));\n",
			    is_array(a->type)? "" : "&",
			    e->name, a->name, e->name, a->name);
			pr_code("buffer += sizeof(%sp->%s);\n\n", 
			    e->name, a->name);
		}
	}
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when defining the struct insertion
 * functions.
 */
static void
insert_struct_function_enter_entop(ENTITY *e)
{
	static char *function_template =
"                                                                           \n\
int %s_insert_struct( DB *dbp, %s_data *%sp)                                \n\
{                                                                           \n\
 DBT key_dbt, data_dbt;                                                     \n\
 char serialized_data[%s];                                                  \n\
 int ret;                                                                   \n\
 %s %s%s_key = %sp->%s;                                                     \n\
									    \n\
 memset(&key_dbt, 0, sizeof(key_dbt));                                      \n\
 memset(&data_dbt, 0, sizeof(data_dbt));                                    \n\
									    \n\
 serialize_%s_data(%sp, serialized_data, &key_dbt, &data_dbt);              \n\
									    \n\
 if ((ret = dbp->put(dbp, NULL, &key_dbt, &data_dbt, 0)) != 0) {            \n\
  dbp->err(dbp, ret, \"Inserting key %s\", %s_key);                         \n\
  return -1;                                                                \n\
 }                                                                          \n\
 return 0;                                                                  \n\
}                                                                           \n\
									    \n\
";
	pr_code(function_template, e->name,  
	    e->name, e->name, 
	    serialized_length_name(e),   
	    e->primary_key->type->c_type,  
	    is_array(e->primary_key->type)? "*" : "",
	    e->name,
	    e->name,
	    e->primary_key->name,
	    e->name, e->name,
	    is_array(e->primary_key->type) ? "%s" : "%d",
	    e->name);

	if (SEPARATE_HEADER)
		pr_header(
"int %s_insert_struct(DB *dbp, %s_data *%sp);\n\n",
		    e->name, e->name, e->name);
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when defining the field insertion
 * functions.
 */
static void
insert_fields_list_args_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	/*
	 * This is an extra attribute operation called by
	 * insert_fields_enter_entop to produce the arguments to the
	 * function it defines.
	 */
	COMPQUIET(e, NULL);
	COMPQUIET(first, 0);

	if (SEPARATE_HEADER) {
		pr_header("%s %s%s", a->type->c_type,
		    is_array(a->type)? "*" : "",
		    a->name);
		if (!last) pr_header(",\n");
	}
}

/*
 * This is an extra attribute operation called by
 * insert_fields_enter_entop to produce the arguments to the
 * function it defines.
 */
static void
insert_fields_declare_args_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{

	COMPQUIET(e, NULL);
	COMPQUIET(first, 0);

	pr_code("%s %s%s", a->type->c_type,
	    is_array(a->type)? "*" : "",
	    a->name);
	if (!last)
	       pr_code(",\n");
	else
	       pr_code(")\n");
}

static void
insert_fields_function_enter_entop(ENTITY *e)
{
	if (SEPARATE_HEADER)
		pr_header("int %s_insert_fields(DB * dbp,\n", e->name);
	pr_code("int %s_insert_fields(DB *dbp,\n", e->name);
	code_indent_level+=3; /* push them way over */
	header_indent_level+=3;
	iterate_over_attributes(e, &insert_fields_list_args_attrop);
	if (SEPARATE_HEADER)
		pr_header(");\n\n");
	code_indent_level-=2; /* drop back to +1 */
	header_indent_level -= 3;
	iterate_over_attributes(e, &insert_fields_declare_args_attrop);
	code_indent_level--;
	pr_code("{\n");
	code_indent_level++;
	pr_code("%s_data data;\n", e->name);
}

static void
insert_fields_function_exit_entop(ENTITY *e)
{
	pr_code("return %s_insert_struct(dbp, &data);\n", e->name);
	code_indent_level--;
	pr_code("}\n\n");
}

static void
insert_fields_function_attrop(ENTITY *e, ATTRIBUTE *a, int first, int last)
{
	COMPQUIET(first, 0);
	COMPQUIET(last, 0);

	if (is_string(a->type)) {
		pr_code("assert(strlen(%s) < %s);\n", a->name,
		    array_dim_name(e, a));
		pr_code("strncpy(data.%s, %s, %s);\n",
		    a->name, a->name, array_dim_name(e, a));
	} else if (is_array(a->type)) {
		pr_code("memcpy(data.%s, %s, %s);\n",
		    a->name, a->name, array_dim_name(e, a));
	} else
		pr_code("data.%s = %s;\n", a->name, a->name);
}

static void
generate_insertion_functions()
{

	/* Generate the serializer functions for each entity. */
	iterate_over_entities(&serialize_function_enter_entop,
	    &serialize_function_exit_entop,
	    &serialize_function_attrop);

	/* Generate the de-serializer functions for each entity. */
	iterate_over_entities(&deserialize_function_enter_entop,
	    &deserialize_function_exit_entop,
	    &deserialize_function_attrop);

	/* Generate the struct insertion functions for each entity. */
	if (SEPARATE_HEADER)
		pr_header_comment(
"Functions for inserting records by providing                               \n\
the full corresponding data structure"
			);

	iterate_over_entities(&insert_struct_function_enter_entop,
	    NULL,
	    NULL);

	/* Generate the field insertion functions for each entity. */
	if (SEPARATE_HEADER)
		pr_header_comment(
"Functions for inserting records by providing                               \n\
each field value as a separate argument"
			);

	iterate_over_entities(&insert_fields_function_enter_entop,
	    &insert_fields_function_exit_entop,
	    &insert_fields_function_attrop);
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when defining the simple fetch
 * by key functions.
 */
static void
fetch_function_enter_entop(ENTITY *e)
{
	static char *header_template =
"                                                                           \n\
int get_%s_data(DB *dbp,                                                    \n\
 %s %s%s_key,                                                               \n\
 %s_data *data)                                                             \n\
{                                                                           \n\
 DBT key_dbt, data_dbt;                                                     \n\
 int ret;                                                                   \n\
 %s %scanonical_key = %s_key;                                               \n\
									    \n\
 memset(&key_dbt, 0, sizeof(key_dbt));                                      \n\
 memset(&data_dbt, 0, sizeof(data_dbt));                                    \n\
 memset(data, 0, sizeof(%s_data));					    \n\
									    \n\
 key_dbt.data = %scanonical_key;                                            \n\
 key_dbt.size = ";

	static char *function_template =					
" 									    \n\
 if ((ret = dbp->get(dbp, NULL, &key_dbt, &data_dbt, 0)) != 0) {            \n\
  dbp->err(dbp, ret, \"Retrieving key %s\", %s_key);                        \n\
  return ret;                                                               \n\
 }                                                                          \n\
									    \n\
 assert(data_dbt.size <= %s);                                               \n\
 memcpy(&data->%s, key_dbt.data, sizeof(data->%s));			    \n\
 deserialize_%s_data(data_dbt.data, data);                                  \n\
 return 0;                                                                  \n\
}                                                                           \n\
									    \n\
";
	pr_code(header_template, e->name, 
	    e->primary_key->type->c_type,
	    is_array(e->primary_key->type) ? "*" : "",
	    e->name, 
	    e->name, 
	    e->primary_key->type->c_type,
	    is_array(e->primary_key->type) ? "*" : "",
	    e->name,
	    e->name, 
	    is_array(e->primary_key->type) ? "" : "&"); 
	    
	if (is_array(e->primary_key->type) && !is_string(e->primary_key->type))
		pr_code(array_dim_name(e, e->primary_key));
	else 
		pr_code("%s(canonical_key)%s", 
		    is_string(e->primary_key->type) ? "strlen" : "sizeof",             
		    is_string(e->primary_key->type) ? " + 1" : "");
	
	pr_code(";\n");
	    
	pr_code(function_template, 
	    is_array(e->primary_key->type) ? "%s" : "%d",
	    e->name,
	    serialized_length_name(e),   
	    e->primary_key->name, 
	    e->primary_key->name,
	    e->name); 

	if (SEPARATE_HEADER)
		pr_header(
"int get_%s_data(DB *dbp, %s %s%s_key, %s_data *data);\n\n",
		    e->name, e->primary_key->type->c_type,
		    is_array(e->primary_key->type) ? "*" : "",
		    e->name, e->name);
}

static void
generate_fetch_functions()
{
	if (SEPARATE_HEADER)
		pr_header_comment("Functions for retrieving records by key");

	iterate_over_entities(&fetch_function_enter_entop,
	    NULL,
	    NULL);
}

/*
 * This next group of entity and attribute operation functions are
 * called by the attribute iterator when defining the simple deletion
 * by primary key functions.
 */
static void
deletion_function_enter_entop(ENTITY *e)
{
	static char *header_template =
"                                                                           \n\
int delete_%s_key(DB *dbp, %s %s%s_key)                                     \n\
{                                                                           \n\
 DBT key_dbt;                                                               \n\
 int ret;                                                                   \n\
 %s %scanonical_key = %s_key;                                               \n\
									    \n\
 memset(&key_dbt, 0, sizeof(key_dbt));                                      \n\
									    \n\
 key_dbt.data = %scanonical_key;                                            \n\
 key_dbt.size = ";

	static char *method_template =
"									    \n\
  if ((ret = dbp->del(dbp, NULL, &key_dbt, 0)) != 0) {                      \n\
  dbp->err(dbp, ret, \"deleting key %s\", %s_key);                          \n\
  return ret;                                                               \n\
 }                                                                          \n\
									    \n\
 return 0;                                                                  \n\
}                                                                           \n\
									    \n\
";
	pr_code(header_template, e->name, e->primary_key->type->c_type,
	    is_array(e->primary_key->type) ? "*" : "",
	    e->name, e->primary_key->type->c_type,
	    is_array(e->primary_key->type) ? "*" : "",
	    e->name,
	    is_array(e->primary_key->type) ? "" : "&");
	    
	if (is_array(e->primary_key->type) && !is_string(e->primary_key->type))
		pr_code(array_dim_name(e, e->primary_key));
	else 
		pr_code("%s(canonical_key)%s",	
		    is_string(e->primary_key->type) ? "strlen" : "sizeof",
		    is_string(e->primary_key->type) ? " + 1" : "");
	
	pr_code(";\n");
	
	pr_code(method_template,    
	    is_array(e->primary_key->type) ? "%s" : "%d",
	    e->name);

	if (SEPARATE_HEADER)
		pr_header("int delete_%s_key(DB *dbp, %s %s%s_key);\n\n",
		    e->name, e->primary_key->type->c_type,
		    is_array(e->primary_key->type) ? "*" : "",
		    e->name);
}

static void
generate_deletion_functions()
{
	if (SEPARATE_HEADER)
		pr_header_comment("Functions for deleting records by key");

	iterate_over_entities(&deletion_function_enter_entop,
	    NULL,
	    NULL);
}

/*
 * This entity operation function is called by the attribute iterator when
 * producing header code that declares the global primary database pointers.
 */
static void
declare_extern_database_pointers_enter_entop(ENTITY *e)
{
	pr_header("extern DB *%s_dbp;\n", e->name);
}

/*
 * This entity operation function is called by the attribute iterator
 * when producing code that defines the global primary database
 * pointers.
 */
static void
declare_database_pointers_enter_entop(ENTITY *e)
{
	pr_code("DB *%s_dbp = NULL;\n", e->name);
}

/*
 * This index operation function is called by the attribute iterator when
 * producing header code that declares the global secondary database pointers.
 */
static void
declare_extern_secondary_pointers_idxop(DB_INDEX *idx)
{
	pr_header("extern DB *%s_dbp;\n", idx->name);
}

/*
 * This index operation function is called by the attribute iterator when
 * producing code that defines the global secondary database pointers.
 */
static void
declare_secondary_pointers_idxop(DB_INDEX *idx)
{
	pr_code("DB *%s_dbp = NULL;\n", idx->name);
}

/*
 * This entity operation function is called by the attribute iterator
 * when producing initialization code that creates the primary databases.
 */
static void
create_database_enter_entop(ENTITY *e)
{
	pr_code(
"                                                                           \n\
if (create_%s_database(%s_envp, &%s_dbp) != 0)                              \n\
 goto exit_error;                                                           \n\
",
	    e->name, the_schema.environment.name, e->name);
}

/*
 * This index operation function is called by the attribute iterator when
 * producing initialization code that creates the secondary databases.
 */
static void
create_secondary_idxop(DB_INDEX *idx)
{
	pr_code(
"                                                                           \n\
if (create_%s_secondary(%s_envp, %s_dbp, &%s_dbp) != 0)                     \n\
 goto exit_error;                                                           \n\
",
	    idx->name, the_schema.environment.name,
	    idx->primary->name, idx->name);
}

static void
generate_secondary_key_callback_function(DB_INDEX *idx)
{
	static char *secondary_key_callback_template =
"                                                                           \n\
int %s_callback(DB *dbp,                                                    \n\
 const DBT *key_dbt,                                                        \n\
 const DBT *data_dbt,                                                       \n\
 DBT *secondary_key_dbt)                                                    \n\
{									    \n\
 int ret;                                                                   \n\
 %s_data deserialized_data;                                                 \n\
									    \n\
 memcpy(&deserialized_data.%s, key_dbt->data, key_dbt->size);               \n\
 deserialize_%s_data(data_dbt->data, &deserialized_data);                   \n\
									    \n\
 memset(secondary_key_dbt, 0, sizeof(DBT));                  		    \n\
 secondary_key_dbt->size = %s(deserialized_data.%s)%s;                      \n\
 secondary_key_dbt->data = malloc(secondary_key_dbt->size);                 \n\
 memcpy(secondary_key_dbt->data, %sdeserialized_data.%s,                    \n\
  secondary_key_dbt->size);                                                 \n\
									    \n\
 /* tell the caller to free memory referenced by secondary_key_dbt */       \n\
 secondary_key_dbt->flags = DB_DBT_APPMALLOC;                               \n\
									    \n\
 return 0;                                                                  \n\
}                                                                           \n\
";

	pr_code(secondary_key_callback_template,
	    idx->name, idx->primary->name,
	    idx->primary->primary_key->name,
	    idx->primary->name,
	    is_string(idx->attribute->type) ? "strlen" : "sizeof",
	    idx->attribute->name,
	    is_string(idx->attribute->type) ? " + 1" : "",
	    is_array(idx->attribute->type) ? "" : "&",
	    idx->attribute->name);
}


static void
generate_index_creation_function(DB_INDEX *idx)
{
	static char *index_creation_function_template =
"                                                                           \n\
int create_%s_secondary(DB_ENV *envp,                                       \n\
 DB *primary_dbp,                                                           \n\
 DB **secondary_dbpp)                                                       \n\
{                                                                           \n\
 int ret;                                                                   \n\
 char * secondary_name = \"%s.db\";                                         \n\
									    \n\
 if ((ret = create_database(envp, secondary_name, secondary_dbpp,           \n\
  DB_CREATE, %s, DB_DUPSORT, %s)) != 0)                                     \n\
 return ret;                                                                \n\
									    \n\
 if ((ret = primary_dbp->associate(primary_dbp, NULL, *secondary_dbpp,      \n\
  &%s_callback, DB_CREATE)) != 0) {                                         \n\
  (*secondary_dbpp)->err(*secondary_dbpp, ret,                              \n\
   \"DB->associate: %%s.db\", secondary_name);                              \n\
  return ret;                                                               \n\
 }                                                                          \n\
 return 0;                                                                  \n\
}                                                                           \n\
";

	pr_code(index_creation_function_template,
	    idx->name, idx->name,
	    idx->dbtype,
	    custom_comparator_for_type(idx->attribute->type),
	    idx->name);

	if (SEPARATE_HEADER)
		pr_header(
"int create_%s_secondary(DB_ENV *envp, DB *dbpp, DB **secondary_dbpp);\n\n",
		    idx->name);
}

static void
generate_index_removal_function(DB_INDEX *idx)
{
	pr_code(
"                                                                           \n\
int remove_%s_index(DB_ENV *envp)                                           \n\
{                                                                           \n\
 return envp->dbremove(envp, NULL, \"%s.db\", NULL, 0);                     \n\
}                                                                           \n\
",
	    idx->name, idx->name);

	if (SEPARATE_HEADER)
		pr_header("int remove_%s_index(DB_ENV * envp);\n\n", idx->name);
}

static void
generate_index_query_iteration(DB_INDEX *idx)
{
	static char *query_iteration_template =
"                                                                           \n\
int %s_query_iteration(DB *secondary_dbp,                                   \n\
 %s %s%s_key,                                                               \n\
 %s_iteration_callback user_func,                                           \n\
 void *user_data)                                                           \n\
{                                                                           \n\
 DBT key_dbt, pkey_dbt, data_dbt;					    \n\
 DBC *cursorp;                                                              \n\
 %s_data deserialized_data;                                                 \n\
 int ret;								    \n\
 int flag = DB_SET;                     				    \n\
									    \n\
 memset(&key_dbt, 0, sizeof(key_dbt));                                      \n\
 memset(&pkey_dbt, 0, sizeof(pkey_dbt)); 				    \n\
 memset(&data_dbt, 0, sizeof(data_dbt));                                    \n\
									    \n\
 if ((ret = secondary_dbp->cursor(secondary_dbp, NULL, &cursorp, 0)) != 0) {\n\
  secondary_dbp->err(secondary_dbp, ret, \"creating cursor\");              \n\
  return ret;                                                               \n\
 }                                                                          \n\
									    \n\
 key_dbt.data = %s%s_key;                                                   \n\
 key_dbt.size = %s(%s_key)%s;                                               \n\
 									    \n\
 while((ret = cursorp->pget(cursorp, &key_dbt, &pkey_dbt, &data_dbt, flag)) \n\
  == 0) {								    \n\
  memcpy(&deserialized_data.%s, pkey_dbt.data, sizeof(deserialized_data.%s));\n\
  deserialize_%s_data(data_dbt.data, &deserialized_data);                   \n\
  (*user_func)(user_data, &deserialized_data);                              \n\
  if (flag == DB_SET)							    \n\
   flag = DB_NEXT_DUP;						    	    \n\
 }                                                                          \n\
									    \n\
 if (ret != DB_NOTFOUND) {                                                  \n\
  secondary_dbp->err(secondary_dbp, ret, \"Querying secondary\");           \n\
  return ret;                                                               \n\
 }                                                                          \n\
									    \n\
 cursorp->close(cursorp);                                                   \n\
									    \n\
 return 0;                                                                  \n\
}                                                                           \n\
";
	pr_code(query_iteration_template,
	    idx->name, idx->attribute->type->c_type,
	    is_array(idx->attribute->type) ? "*" : "",
	    idx->name, idx->primary->name, idx->primary->name,
	    is_array(idx->attribute->type) ? "" : "&",
	    idx->name,
	    is_string(idx->attribute->type) ? "strlen" : "sizeof",
	    idx->name, 
	    is_string(idx->attribute->type) ? " + 1" : "",
	    idx->primary->primary_key->name, idx->primary->primary_key->name,
	    idx->primary->name);

	if (SEPARATE_HEADER)
		pr_header(
"int %s_query_iteration(DB *secondary_dbp,                                  \n\
  %s %s%s_key,                                                              \n\
  %s_iteration_callback user_func,                                          \n\
  void *user_data);\n\n",
		    idx->name, idx->attribute->type->c_type,
		    is_array(idx->attribute->type) ? "*" : "",
		    idx->name, idx->primary->name);
}

static void
index_function_idxop(DB_INDEX *idx)
{
	generate_secondary_key_callback_function(idx);
	generate_index_creation_function(idx);
	generate_index_removal_function(idx);
	generate_index_query_iteration(idx);
}

static void
generate_index_functions() {
	if (SEPARATE_HEADER)
		pr_header_comment("Index creation and removal functions");
	iterate_over_indexes(&index_function_idxop);
}

/*
 * Emit the prototype of the user's callback into the header file.
 */
static void
generate_iteration_callback_typedef_enter_entop(ENTITY *e)
{
	pr_header(
		"typedef void (*%s_iteration_callback)(void *, %s_data *);\n\n",
		    e->name, e->name);
}

static void
generate_iteration_callback_typedefs()
{
	pr_header_comment(
"These typedefs are prototypes for the user-written                         \n\
iteration callback functions, which are invoked during                      \n\
full iteration and secondary index queries"
	    );

	iterate_over_entities(&generate_iteration_callback_typedef_enter_entop,
	    NULL,
	    NULL);
}

static void
generate_full_iteration_enter_entop(ENTITY *e)
{
	static char *full_iteration_template =
"                                                                           \n\
int %s_full_iteration(DB *dbp,                                              \n\
 %s_iteration_callback user_func,                                           \n\
 void *user_data)                                                           \n\
{                                                                           \n\
 DBT key_dbt, data_dbt;                                                     \n\
 DBC *cursorp;                                                              \n\
 %s_data deserialized_data;                                                 \n\
 int ret;                                                                   \n\
									    \n\
 memset(&key_dbt, 0, sizeof(key_dbt));                                      \n\
 memset(&data_dbt, 0, sizeof(data_dbt));                                    \n\
									    \n\
 if ((ret = dbp->cursor(dbp, NULL, &cursorp, 0)) != 0) {                    \n\
  dbp->err(dbp, ret, \"creating cursor\");                                  \n\
  return ret;                                                               \n\
 }                                                                          \n\
									    \n\
 while ((ret = cursorp->get(cursorp, &key_dbt, &data_dbt, DB_NEXT)) == 0) { \n\
  memcpy(&deserialized_data.%s, key_dbt.data, sizeof(deserialized_data.%s));\n\
  deserialize_%s_data(data_dbt.data, &deserialized_data);                   \n\
  (*user_func)(user_data, &deserialized_data);                              \n\
 }                                                                          \n\
									    \n\
 if (ret != DB_NOTFOUND) {                                                  \n\
  dbp->err(dbp, ret, \"Full iteration\");                                   \n\
  cursorp->close(cursorp);                                                  \n\
  return ret;                                                               \n\
 }                                                                          \n\
									    \n\
 cursorp->close(cursorp);                                                   \n\
									    \n\
 return 0;                                                                  \n\
}                                                                           \n\
";

	pr_code(full_iteration_template,
	    e->name, e->name, e->name, 
	    e->primary_key->name,
	    e->primary_key->name,
	    e->name);

	if (SEPARATE_HEADER)
		pr_header(
"int %s_full_iteration(DB *dbp,                                             \n\
  %s_iteration_callback user_func,                                          \n\
  void *user_data);\n\n",
		    e->name, e->name);
}

static void
generate_full_iterations()
{
	if (SEPARATE_HEADER)
		pr_header_comment(
"Functions for doing iterations over                                        \n\
an entire primary database");

	iterate_over_entities(&generate_full_iteration_enter_entop,
	    NULL,
	    NULL);

}

static void
generate_initialization()
{
	if (SEPARATE_HEADER) {
		pr_header_comment(
"This convenience method invokes all of the                                 \n\
environment and database creation methods necessary                         \n\
to initialize the complete BDB environment.  It uses                        \n\
the global environment and database pointers declared                       \n\
below.  You may bypass this function and use your own                       \n\
environment and database pointers, if you wish.");

		pr_header("int initialize_%s_environment();\n",
		    the_schema.environment.name);
	}
	pr_header("\nextern DB_ENV * %s_envp;\n", the_schema.environment.name);
	iterate_over_entities(&declare_extern_database_pointers_enter_entop,
	    NULL,
	    NULL);

	pr_code("\nDB_ENV * %s_envp = NULL;\n", the_schema.environment.name);
	iterate_over_entities(&declare_database_pointers_enter_entop,
	    NULL,
	    NULL);

	iterate_over_indexes(&declare_extern_secondary_pointers_idxop);

	iterate_over_indexes(&declare_secondary_pointers_idxop);

	pr_code(
"                                                                           \n\
int initialize_%s_environment()                                             \n\
{                                                                           \n\
 if (create_%s_env(&%s_envp) != 0)                                          \n\
  goto exit_error;                                                          \n\
",
	    the_schema.environment.name,
	    the_schema.environment.name,
	    the_schema.environment.name);

	code_indent_level++;
	iterate_over_entities(&create_database_enter_entop,
	    NULL,
	    NULL);

	iterate_over_indexes(&create_secondary_idxop);

	pr_code ("\nreturn 0;\n\n");
	code_indent_level--;
	pr_code("exit_error:\n");
	code_indent_level++;
	pr_code(
"                                                                           \n\
fprintf(stderr, \"Stopping initialization because of error\\n\");           \n\
return -1;                                                                  \n\
");
	code_indent_level--;
	pr_code("}\n");
}

static void
check_pk_constraint_enter_entop(ENTITY *e)
{
	if (e->primary_key == NULL) {
		fprintf(stderr,
		    "The table \"%s\" (defined near line %d) lacks a \
primary key, which is not allowed.  All tables must have a designated \
primary key.\n",
		    e->name, e->line_number);
		exit(1);
	}
}

static void
check_constraints()
{
	iterate_over_entities(&check_pk_constraint_enter_entop,
	    NULL, NULL);
}

/*
 * Generate is the sole entry point to this module.  It orchestrates
 * the generation of the C code for the storage layer and for the
 * smoke test.
 */
void generate(hfile, cfile, tfile, vfile, hfilename)
	FILE *hfile, *cfile, *tfile, *vfile;
	char *hfilename;
{
	static char *header_intro_comment =
"Header file for a Berkeley DB implementation                               \n\
generated from SQL DDL by db_sql";

	static char *include_stmts =
"#include <sys/types.h>                                                     \n\
#include <sys/stat.h>                                                       \n\
#include <assert.h>                                                         \n\
#include <errno.h>                                                          \n\
#include <stdlib.h>                                                         \n\
#include <string.h>                                                         \n\
#include \"db.h\"                                                           \n\
\n";

	header_file = hfile;
	code_file = cfile;

	check_constraints();

	if (SEPARATE_HEADER)
		pr_header_comment(header_intro_comment);

	pr_header(include_stmts);

	if (SEPARATE_HEADER)
		pr_code("#include \"%s\"\n\n", hfilename);

	generate_config_defines();
	generate_schema_structs();
	generate_iteration_callback_typedefs();
	generate_environment();
	generate_db_creation_functions();
	generate_db_removal_functions();
	generate_insertion_functions();
	generate_fetch_functions();
	generate_deletion_functions();
	generate_full_iterations();
	generate_index_functions();
	generate_initialization();

	if (tfile != NULL)
		generate_test(tfile, hfilename);
	if (vfile != NULL)
		generate_verification(vfile, hfilename);
}

/*
  This is a little emacs-lisp keybinding to help with the
   long line continuations

  Local Variables:
  eval:(defun insert-string-continuation () (interactive)   (let ((spaces-needed (- 76 (current-column))))    (insert-char ?  spaces-needed)    (insert "\\n\\")))
  eval:(local-set-key "\C-cc" 'insert-string-continuation)
  End:
*/
