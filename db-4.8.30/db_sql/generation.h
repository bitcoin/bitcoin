/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * This include file contains declarations shared between the code
 * generation modules.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

#include "db_sql.h"

/* Function pointer types to be invoked by the entity and attribute iterators */

typedef void (*enter_entity_operation)(ENTITY *e);
typedef void (*exit_entity_operation)(ENTITY *e);
typedef void (*attribute_operation)(ENTITY *e, ATTRIBUTE *a, 
    int first, int last);
typedef void (*index_operation)(DB_INDEX *idx);

/* The entity and attribute iterator function prototypes */

void iterate_over_entities(enter_entity_operation e_enter_op,
    exit_entity_operation e_exit_op, 
    attribute_operation a_op);
void iterate_over_attributes(ENTITY *e, attribute_operation a_op);
void iterate_over_indexes(index_operation i_op);

/* Other common utilities used by the code generators */

char *prepare_string(const char *in, int indent_level, int is_comment);
int is_array(ATTR_TYPE *t);
int is_string(ATTR_TYPE *t);
char *custom_comparator_for_type(ATTR_TYPE *t);
char *array_dim_name(ENTITY *e, ATTRIBUTE *a);
char *decl_name(ENTITY *e, ATTRIBUTE *a);
char *serialized_length_name(ENTITY *e);
char *upcase_env_name(ENVIRONMENT_INFO *envp);

#define INDENT_MULTIPLIER 1  /* How many chars per indent level */
#define INDENT_CHAR '\t'     /* Char to use for indenting */
