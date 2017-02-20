/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * These are common utility functions used by the code-generating
 * modules.
 */
#include "generation.h"

static int
is_tab_or_space(c)
	int c;
{
	return c == '\t' || c == ' ';
}

/*
 * Copy a string, truncating whitespace that precedes a newline.  The
 * copy is stored in a static buffer and returned.  Several customized
 * printf wrappers (in generate.c and generate_test.c) use this
 * function to truncate the long format strings that are used to make
 * code-generation functions in those file slightly more readable.
 * This also inserts the appropriate indentation string.
 */
char *
prepare_string(in, indent_level, is_comment)
	const char *in;
	int indent_level;
	int is_comment;
{
#define ttw_bufsiz 10240	
	static char out[ttw_bufsiz];
	int in_i, out_i, tmp_i;
	int indentation;

	indentation = indent_level * INDENT_MULTIPLIER;
	in_i = 0;

	for (out_i = 0; out_i < indentation; out_i++)
		out[out_i] = INDENT_CHAR;

	if (is_comment) {
		out[out_i++] = '/';
		out[out_i++] = '*';
		out[out_i++] = '\n';
	}

	while (out_i < ttw_bufsiz) {
		if (is_tab_or_space(in[in_i])) {
			for (tmp_i = in_i;
			     in[tmp_i] != '\0' && is_tab_or_space(in[tmp_i]); 
			     tmp_i++)
				;
			if (in[tmp_i] == '\n')
				in_i = tmp_i;
		}

		/* insert indentation after newline, unless we're at the end */
		if ((out_i == 0 || out[out_i - 1] == '\n') && in[in_i] != '\0'){
			for (tmp_i = out_i + indentation;
			     out_i < tmp_i; 
			     out_i++) {
				assert(out_i < ttw_bufsiz);
				out[out_i] = INDENT_CHAR;
			}
			if (is_comment) {
				out[out_i++] = ' ';
				out[out_i++] = '*';
				out[out_i++] = ' ';
			} else {
				/* interpret spaces at the beginning of a line
				   as further indentation levels.
				   (1 space = 1 indentation level)
				*/
				for (;
				     in[in_i] == ' ';
				     in_i++)
					for(tmp_i = out_i + INDENT_MULTIPLIER;
					    out_i < tmp_i;
					    out_i++) {
						assert(out_i < ttw_bufsiz);
						out[out_i] = INDENT_CHAR;
					}
			}
		}

		if ((out[out_i++] = in[in_i++]) == '\0') {
			if (is_comment) {
				out_i--; /* back up to the null just assigned */
				/* if the last char isn't \n, add one */
				if (out_i > 0 && out[out_i-1] != '\n')
					out[out_i++] = '\n';
				/* insert the indentation, as before */
				for (tmp_i = out_i + indentation;
				     out_i < tmp_i;
				     out_i++) {
					assert(out_i < ttw_bufsiz);
					out[out_i] = INDENT_CHAR;
				}
				assert(out_i + 5 < ttw_bufsiz);
				out[out_i++] = ' ';
				out[out_i++] = '*';
				out[out_i++] = '/';
				out[out_i++] = '\n';
				out[out_i++] = '\0';
			}
			return out;
		}
	}
	assert(0); /* if this goes off, we exceeded ttw_bufsiz */
	return NULL;
}

/*
 * These entity/attribute iterator functions invoke the given
 * functions at appropriate times during the iteration.
 */
void
iterate_over_attributes(e, a_op)
	ENTITY *e;
	attribute_operation a_op;
{
	ATTRIBUTE *a;

	for (a = e->attributes_head; a; a = a->next)
		if (a_op)
			(*a_op)(e, a, a == e->attributes_head, a->next == NULL);
}

void
iterate_over_entities(e_enter_op, e_exit_op, a_op)
	enter_entity_operation e_enter_op;
	exit_entity_operation e_exit_op;
	attribute_operation a_op;
{
	ENTITY *e;

	for (e = the_schema.entities_head; e; e = e->next) {

		if (e_enter_op)
			(*e_enter_op)(e);

		if (a_op)
			iterate_over_attributes(e, a_op);

		if (e_exit_op)
			(*e_exit_op)(e);

	}
}

void
iterate_over_indexes(i_op)
	index_operation i_op;
{
	DB_INDEX *idx;

	for (idx = the_schema.indexes_head; idx; idx = idx->next)
		(*i_op)(idx);
}

/*
 * Indicate whether this type is a supported array type.
 */

int
is_array(t)
	ATTR_TYPE *t;
{
	return t->is_array;
}

/*
 * Indicate whether this type is a char * string, whose size can be
 * calculated with strlen.
 */
int
is_string(t)
	ATTR_TYPE *t;
{
	return t->is_string;
}

/*
 * Return the name of the custom comparator for integer types.
 */
char *
custom_comparator_for_type(t)
	ATTR_TYPE *t;
{
	if (strcmp(t->c_type, "int") == 0)
		return ("&compare_int");
	if (strcmp(t->c_type, "long") == 0)
		return ("&compare_long");
	return "NULL";
}

/*
 * Return the name of an array dimension #defined constant.  Stores
 * the name in the ATTRIBUTE structure if it is not already set.
 */
char *
array_dim_name(e, a)
	ENTITY *e;
	ATTRIBUTE *a;
{
	char *s, *format;
	size_t len;

	if (a->array_dim_name != NULL)
		return a->array_dim_name;

	format = "%s_data_%s_length";
	len = strlen(format) - 4; /* subtract the format directives */
	len += strlen(e->name);
	len += strlen(a->name);
	len += 1;                     /* add termination char */
	
	s = malloc(len);
	snprintf(s, len, format, e->name, a->name);

	assert(strlen(s) == len - 1);
	
	a->array_dim_name = s;
	
	/* now capitalize it */
	for (; *s != '\0'; s++)
		*s = toupper(*s);
	
	return a->array_dim_name;
}

char *
decl_name(e, a)
	ENTITY *e;
	ATTRIBUTE *a;
{		
	char *s, *format, *dim_name;
	size_t len;

	if (a->decl_name != NULL)
		return a->decl_name;

	if (!is_array(a->type))
		a->decl_name = a->name;
	else {

		format = "%s[%s]";
		dim_name = array_dim_name(e, a);
		len = strlen(format) - 4; /* subtract format directives */
		len += strlen(a->name);
		len += strlen(dim_name);
		len += 1;

		s = malloc(len);
		snprintf(s, len, format, a->name, dim_name);
		assert(strlen(s) == len - 1);

		a->decl_name = s;
	}

	return a->decl_name;
}

/*
 * Return the name of an serialized length #defined constant.  Stores
 * the name in the ENTITY structure if it is not already set.
 */
char *
serialized_length_name(e)
	ENTITY *e; 
{
	char *s, *format;
	size_t len;

	if (e->serialized_length_name != NULL)
		return e->serialized_length_name;

	format = "%s_data_serialized_length";
	len = strlen(format) - 2; /* subtract the format directives */
	len += strlen(e->name);
	len += 1;                     /* add termination char */
	
	s = malloc(len);
	snprintf(s, len, format, e->name);

	assert(strlen(s) == len - 1);
	
	e->serialized_length_name = s;
	
	/* now capitalize it */
	for (; *s != '\0'; s++)
		*s = toupper(*s);
	
	return e->serialized_length_name;
}

char *
upcase_env_name(envp)
	ENVIRONMENT_INFO *envp;
{
	char *s;
	size_t len;

	if (envp->uppercase_name != NULL)
		return envp->uppercase_name;

	len = strlen(envp->name)+ 1;
	envp->uppercase_name = malloc(len);
	memcpy(envp->uppercase_name, envp->name, len);

	for (s = envp->uppercase_name; *s != '\0'; s++)
		*s = toupper(*s);

	return envp->uppercase_name;
}
