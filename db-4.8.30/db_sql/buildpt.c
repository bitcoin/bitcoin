/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * The functions in this compilation unit are those related to
 * building the schema model during the parsing of a SQL source file.
 * Most of the functions that have name beginning with "sqlite3..."
 * are invoked by the sqlite parser as it recognizes bits of syntax.
 */

#include <ctype.h>

#include "db_sql.h"

DB_SCHEMA the_schema;
PARSE_PROGRESS the_parse_progress;
int maxbinsz; /* keeps track of the largest binary field */

/* 
 * Extract a name from sqlite token structure.  Returns allocated memory.
 */

static char *
name_from_token(t, pParse)
	Token *t;
	Parse *pParse;
{
	char *s;

	if (t == NULL || t->n <= 0) {
		sqlite3ErrorMsg(pParse, 
		    "Extracting name from a null or empty token");
		return NULL;
	}

	s = calloc(1, t->n + 1);
	if (s == NULL) {
		sqlite3ErrorMsg(pParse, "Malloc failed");
		return NULL;
	}
			    
	memcpy(s, (char*)t->z, t->n);
	sqlite3Dequote(s);

	return s;
}

/*
 * Allocate, populate, and return memory for an ENTITY struct.  If
 * there is a malloc failure, set an error message in the Parse
 * object and return NULL.
 */ 
static ENTITY *
make_entity(name, pParse)
        char *name;
        Parse *pParse;
{
	ENTITY *entity;

	entity = calloc(1, sizeof(ENTITY));
	if (entity == NULL) {
		sqlite3ErrorMsg(pParse, "Malloc failed");
		return NULL;
	}

	entity->name = name;
	entity->line_number = line_number;
	entity->dbtype = "DB_BTREE";
	return entity;
}

/*
 * Allocate, populate, and return memory for an ATTRIBUTE struct.  If
 * there is a malloc failure, set an error message in the Parse
 * object and return NULL.
 */ 
static ATTRIBUTE *
make_attribute(name, pParse)
        char *name;
        Parse *pParse;
{
	ATTRIBUTE *attribute;

	attribute = calloc(1, sizeof(ATTRIBUTE));
	if (attribute == NULL) {
		sqlite3ErrorMsg(pParse, "Malloc failed");
		return NULL;
	}

	attribute->name = name;
	return attribute;
}

/*
 * Allocate, populate, and return memory for an ATTR_TYPE struct.  If
 * there is a malloc failure, set an error message in the Parse
 * object and return NULL.
 */ 
static ATTR_TYPE *
make_attrtype(token, ctype, dimension, is_array, is_string, pParse)
        char *token;
        char *ctype;
        int dimension;
	int is_array;
	int is_string;
	Parse *pParse;
{
	ATTR_TYPE *type;

        type = calloc(1, sizeof(ATTR_TYPE));
	if (type == NULL) {
		sqlite3ErrorMsg(pParse, "Malloc failed");
		return NULL;
	}

	type->sql_token = token;
	type->c_type = ctype;
	type->array_dim = dimension;
	type->is_array = is_array;
	type->is_string = is_string;
	return type;
}

/*
 * Allocate, populate, and return memory for an DB_INDEX struct.  If
 * there is a malloc failure, set the error message in the Parse
 * object and return NULL.
 */ 
static DB_INDEX *
make_index(name, primary, attribute, pParse)
        char *name;
	ENTITY *primary;
	ATTRIBUTE *attribute;
	Parse *pParse;
{
	DB_INDEX *idx;

	idx = calloc(1, sizeof(DB_INDEX));
	if (idx == NULL) {
		sqlite3ErrorMsg(pParse, "Malloc failed");
		return NULL;
	}

	idx->name = name;
	idx->primary = primary;
	idx->attribute = attribute;
        idx->line_number = line_number;
	idx->dbtype = "DB_BTREE";

	return idx;
}

static void
add_entity(entity)
	ENTITY *entity;
{
	if (the_schema.entities_tail == NULL)
		the_schema.entities_tail = the_schema.entities_head = entity;
	else {
		the_schema.entities_tail->next = entity;
		the_schema.entities_tail = entity;
	}
}

static void
add_index(idx)
	DB_INDEX *idx;
{
	if (the_schema.indexes_tail == NULL)
		the_schema.indexes_tail = the_schema.indexes_head = idx;
	else {
		the_schema.indexes_tail->next = idx;
		the_schema.indexes_tail = idx;
	}
}

static void
add_attribute(entity, attr)
	ENTITY *entity;
	ATTRIBUTE * attr;
{
	if (entity->attributes_tail == NULL) {
		entity->attributes_head = entity->attributes_tail = attr;
	} else {
		entity->attributes_tail->next = attr;
		entity->attributes_tail = attr;
	}
}

static ENTITY *
get_current_entity()
{
	/*
	 * The entity under construction is always at the tail of the
	 *  entity list
	 */
	ENTITY *entity = the_schema.entities_tail;
	assert(entity);

	return entity;
}

static ATTRIBUTE *
get_current_attribute()
{
	ENTITY *entity;
	ATTRIBUTE *attr;

	/*
	 * The attribute under construction is always at the tail of
	 * the attribute list belonging to the entity at the tail of
	 * the entity list.
	 */
	entity = get_current_entity();
	attr = entity->attributes_tail;
	assert(attr);
	return attr;
}

static ENTITY *
get_entity_by_name(sought_name)
char *sought_name;
{
	ENTITY *e;

	for (e = the_schema.entities_head; e; e = e->next)
		if (strcasecmp(sought_name, e->name) == 0)
			return e;

	return NULL;
}

static ATTRIBUTE *
get_attribute_by_name(in_entity, sought_name)
ENTITY * in_entity;
char *sought_name;
{
	ATTRIBUTE *a;
	for (a = in_entity->attributes_head; a; a = a->next)
		if (strcasecmp(sought_name, a->name) == 0)
			return a;

	return NULL;
}

static DB_INDEX *
get_index_by_name(sought_name)
char *sought_name;
{
	DB_INDEX *idx;

	for (idx = the_schema.indexes_head; idx; idx = idx->next)
		if (strcasecmp(sought_name, idx->name) == 0)
			return idx;

	return NULL;
}

void
sqlite3BeginParse(Parse *pParse, int explainFlag)
{
	COMPQUIET(pParse, NULL);
	COMPQUIET(explainFlag, 0);
	/* no-op */
}

void
bdb_create_database(Token *name, Parse *pParse) {
	if (the_schema.environment.name != NULL)
		sqlite3ErrorMsg(pParse,
		    "Encountered two CREATE DATABASE statements; only one \
is allowed");

        if ((the_schema.environment.name = 
	    name_from_token(name, pParse)) == NULL)
		return;

	the_schema.environment.line_number = line_number;
	the_schema.environment.cache_size = 0;

	the_parse_progress.last_event = PE_ENVIRONMENT;
}


void
sqlite3StartTable(pParse, pName1, pName2, isTemp, isView, isVirtual, noErr)
	Parse *pParse;   /* Parser context */
	Token *pName1;   /* First part of the name of the table or view */
	Token *pName2;   /* Second part of the name of the table or view */
	int isTemp;      /* True if this is a TEMP table */
	int isView;      /* True if this is a VIEW */
	int isVirtual;   /* True if this is a VIRTUAL table */
	int noErr;       /* Do nothing if table already exists */
{
	char *name, *name2;
	ENTITY *entity;
	DB_INDEX *idx;

	COMPQUIET(isTemp, 0);
	COMPQUIET(isView, 0);
	COMPQUIET(isVirtual, 0);
	COMPQUIET(noErr, 0);

	if (the_schema.environment.name == NULL) {
		sqlite3ErrorMsg(pParse,
		    "Please specify CREATE DATABASE before CREATE TABLE");
		return;
	}

	if ((name = name_from_token(pName1, pParse)) == NULL)
		return;

	/*
	 * We don't allow the second pName, for now.  Note for future
	 * reference: if pName2 is set, then pName1 is the database
	 * name, and pname2 is the table name.
	 */
	name2 = NULL;

	if (! (pName2 == NULL || pName2->n == 0) ) {
		name2 = name_from_token(pName2, pParse);
		sqlite3ErrorMsg(pParse,
		    "The table name must be simple: %s.%s",
		    name, name2);
		goto free_allocated_on_error;
	}

	if ((entity = get_entity_by_name(name)) != NULL) {
		sqlite3ErrorMsg(pParse,
		    "Found two declarations of a table named %s, at lines \
%d and %d",
		    name, entity->line_number, line_number);
		goto free_allocated_on_error;
	}

	if ((idx = get_index_by_name(name)) != NULL) {
		sqlite3ErrorMsg(pParse,
		    "The entity named %s on line %d has the same name as \
the index at line %d.  This is not allowed.",
		    name, line_number, idx->line_number);
		goto free_allocated_on_error;
	}

	if ((entity = make_entity(name, pParse)) == NULL)
		goto free_allocated_on_error;

	the_parse_progress.last_event = PE_ENTITY;
	the_parse_progress.last_entity = entity;
	add_entity(entity);

	return;

free_allocated_on_error:
	if (name != NULL) free(name);
	if (name2 != NULL) free(name2);
}

void
sqlite3AddColumn(Parse *pParse, Token *pName)
{
	ENTITY *entity;
	ATTRIBUTE *attr;
	char *name;

	if ((name = name_from_token(pName, pParse)) == NULL)
		return;

	entity = get_current_entity();

	if ((attr = get_attribute_by_name(entity, name)) != NULL) {
		sqlite3ErrorMsg(pParse,
		    "The table %s contains two columns with the same name %s; \
this is not allowed.",
		    entity->name, name);
		goto free_allocated_on_error;
	}

	if ((attr = make_attribute(name, pParse)) == NULL)
		goto free_allocated_on_error;
  
	the_parse_progress.last_event = PE_ATTRIBUTE;
	the_parse_progress.last_attribute = attr;
	add_attribute(entity, attr);

	return;

free_allocated_on_error:
	if (name != NULL) free(name);
}

static void
delete_spaces(char *s) {
	char *p;

        p = s;
	while(*p) {
		if (!isspace(*p))
			*s++ = *p;
		*p++;
	}
	*s = *p;
}

/*
 * This function maps SQL types into the closest equivalent
 * available using plain C-language types.
 */
static ATTR_TYPE *
map_sql_type(pParse, token)
        Parse *pParse;
	char *token;
{
        int dimension, scale, is_array, is_string;
	size_t len;
	char *p, *q;
	ATTR_TYPE *type;
	char *t;
	char *ctype;

        ctype = NULL;
	type = NULL;
	scale = 0;
	len = strlen(token) + 1;

	/*
	 * Make a copy of the original token, so that we can modify it
	 * to remove spaces, and break it up into multiple strings by
	 * inserting null characters.
	 */
	t = malloc(len);
	if (t == NULL) {
		sqlite3ErrorMsg(pParse, "Malloc failed");
		return NULL;
	}

	memcpy(t, token, len);

	dimension = 0;
	is_array = 0;
	is_string = 0;
  
	delete_spaces(t); /* Remove any spaces that t might contain. */

	/*
	 * If t contains a parenthesis, then it is a SQL type that has
	 * either a dimension, such as VARCHAR(255); or a precision,
	 * and possibly a scale, such as NUMBER(15, 2).  We need to
	 * extract these values.  Sqlite's parser does not decompose
	 * these tokens for us.
	 */

	if ((p = strchr(t, '(')) != NULL) {
		/*
		 * Split the token into two strings by replacing the
		 * parenthesis with a null character.  The pointer t
		 * is now a simple type name, and p points to the
		 * first parenthesized parameter.
		 */
		*p++ = '\0';

		/*
		 * There should be a right paren somewhere.  
		 * Actually, the parser probably guarantees this at least.
		 */
		if ((q = strchr(p, ')')) == NULL) {
			sqlite3ErrorMsg(pParse,
			    "Missing right parenthesis in type expression %s\n",
			    token);
			goto free_allocated_on_error;
		}

		/*
		 * null the paren, to make the parameter list a
		 * null-terminated string
		 */
		*q = '\0';

		/*
		 * Is there a comma in the parameter list? If so,
		 * we'll isolate the first parameter
		 */
		if ((q = strchr(p, ',')) != NULL)
			*q++ = '\0';  /* q now points to the second param */

		dimension = atoi(p);

		if (dimension == 0) {
			sqlite3ErrorMsg(pParse,
			    "Non-numeric or zero size parameter in type \
expression %s\n",
			    token);
			goto free_allocated_on_error;
		}
		/* If there was a second parameter, parse it */
		scale = 0;
		if (q != NULL && *q != '\0') {
			if (strchr(q, ',') == NULL && isdigit(*q)) {
				scale = atoi(q);
			} else {
				sqlite3ErrorMsg(pParse,
				    "Unexpected value for second parameter in \
type expression %s\n",
				    token);
				goto free_allocated_on_error;
			}
		}
	}

	/* OK, now the simple mapping. */

	q = t;
	while(*q) {
		*q = tolower(*q);
		q++;
	}

	if (strcmp(t, "bin") == 0) {
		ctype = "char";
		is_array = 1;
	} else if (strcmp(t, "varbin") == 0) {
		ctype = "char";
		is_array = 1;
	} else if (strcmp(t, "char") == 0) {
		ctype = "char";
		is_array = 1;
		is_string = 1;
	} else if (strcmp(t, "varchar") == 0) {
		ctype = "char";
		is_array = 1;
		is_string = 1;
	} else if (strcmp(t, "varchar2") == 0) {
		ctype = "char";
		is_array = 1;
		is_string = 1;
	} else if (strcmp(t, "bit") == 0)
		ctype = "char";
	else if (strcmp(t, "tinyint") == 0)
		ctype = "char";
	else if (strcmp(t, "smallint") == 0)
		ctype = "short";
	else if (strcmp(t, "integer") == 0)
		ctype = "int";
	else if (strcmp(t, "int") == 0)
		ctype = "int";
	else if (strcmp(t, "bigint") == 0)
		ctype = "long";
	else if (strcmp(t, "real") == 0)
		ctype = "float";
	else if (strcmp(t, "double") == 0)
		ctype = "double";
	else if (strcmp(t, "float") == 0)
		ctype = "double";
	else if (strcmp(t, "decimal") == 0)
		ctype = "double";
	else if (strcmp(t, "numeric") == 0)
		ctype = "double";
	else if (strcmp(t, "number") == 0 ) {
		/*
		 * Oracle Number type: if scale is zero, it's an integer.
		 * Otherwise, call it a floating point
		 */
		if (scale == 0) {
			/* A 10 digit decimal needs a long */
			if (dimension < 9)
				ctype = "int";
			else
				ctype = "long";
		} else {
			if (dimension < 7 )
				ctype = "float";
			else
				ctype = "double";
		}
	}
	else {
		sqlite3ErrorMsg(pParse,
		    "Unsupported type %s\n",
		    token);
		goto free_allocated_on_error;
	}

	if (is_array) {
		if (dimension < 1) {
			sqlite3ErrorMsg(pParse,
			    "Zero dimension not allowed for %s", 
			    token);
			goto free_allocated_on_error;
		}
		if ((!is_string) && dimension > maxbinsz)
			maxbinsz = dimension;
	}

	if (is_string && dimension < 2)
		fprintf(stderr, 
		    "Warning: dimension of string \"%s %s\" \
is too small to hold a null-terminated string.",
		    get_current_attribute()->name, token);

	type = make_attrtype(token, ctype, dimension, is_array,
	    is_string, pParse);

free_allocated_on_error:
	free(t);

	/* Type will be NULL unless make_attrtype was called, and succeeded. */
	return(type);
}

void
sqlite3AddColumnType(pParse, pType)
	Parse *pParse;
	Token *pType;
{
	char *type;
	ATTRIBUTE *attr;

        if ((type = name_from_token(pType, pParse)) == NULL)
		return;

	attr = get_current_attribute();
  
	attr->type = map_sql_type(pParse, type);
}
  
void
sqlite3AddPrimaryKey(pParse, pList, onError, autoInc, sortOrder)
	Parse *pParse;    /* Parsing context */
	ExprList *pList;  /* List of field names to be indexed */
	int onError;      /* What to do with a uniqueness conflict */
	int autoInc;      /* True if the AUTOINCREMENT keyword is present */
	int sortOrder;    /* SQLITE_SO_ASC or SQLITE_SO_DESC */
{
	ENTITY *entity;
	ATTRIBUTE *attr;

	COMPQUIET(pParse, NULL);
	COMPQUIET(pList, NULL);
	COMPQUIET(onError, 0);
	COMPQUIET(autoInc, 0);
	COMPQUIET(sortOrder, 0);

	entity = get_current_entity();
	attr = get_current_attribute();
	entity->primary_key = attr;
}

/*
 * Add a new element to the end of an expression list.  If pList is
 * initially NULL, then create a new expression list.
 */
ExprList *
sqlite3ExprListAppend(pParse, pList, pExpr, pName)
        Parse *pParse;          /* Parsing context */
	ExprList *pList;        /* List to which to append. Might be NULL */
	Expr *pExpr;            /* Expression to be appended */
	Token *pName;           /* AS keyword for the expression */
{
	if( pList == NULL ) {
		pList = calloc(1, sizeof(ExprList));
		if (pList == NULL) {
			sqlite3ErrorMsg(pParse, "Malloc failed");
			return NULL;
		}
	}
	if (pList->nAlloc <= pList->nExpr) {
		struct ExprList_item *a;
		int n = pList->nAlloc*2 + 4;
		a = realloc(pList->a, n * sizeof(pList->a[0]));
		if( a == NULL ) {
			sqlite3ErrorMsg(pParse, "Malloc failed");
			return NULL;
		}
		pList->a = a;
		pList->nAlloc = n;
	}

	if( pExpr || pName ){
		struct ExprList_item *pItem = &pList->a[pList->nExpr++];
		memset(pItem, 0, sizeof(*pItem));
		if ((pItem->zName = name_from_token(pName, pParse)) == NULL) {
			pList->nExpr --; /* undo allocation of pItem */
			return pList;
		}
		pItem->pExpr = pExpr;
	}
	return pList;
}
  
void
sqlite3ExprListCheckLength(pParse, pElist, zObject)
	Parse *pParse;
	ExprList *pElist;
	const char *zObject;
{
	COMPQUIET(pParse, NULL);
	COMPQUIET(pElist, NULL);
	COMPQUIET(zObject, NULL);
	/* no-op */
}

/*
 * Append a new table name to the given SrcList.  Create a new SrcList
 * if need be.  If pList is initially NULL, then create a new
 * src list.
 *
 * We don't have a Parse object here, so we can't use name_from_token,
 * or report an error via the usual Parse.rc mechanism.  Just emit a
 * message on stderr if there is a problem.  Returning NULL from this
 * function will cause the parser to fail, so any message we print
 * here will be followed by the usual syntax error message.
 */

SrcList *
sqlite3SrcListAppend(db, pList, pTable, pDatabase)
        sqlite3 *db;        /* unused */
	SrcList *pList;     /* Append to this SrcList */
	Token *pTable;      /* Table to append */
	Token *pDatabase;   /* Database of the table, not used at this time */
{
	struct SrcList_item *pItem;
	char *table_name;

	COMPQUIET(db, NULL);
	COMPQUIET(pDatabase, NULL);

	table_name = NULL;
	
	if (pList == NULL) {
		pList = calloc(1, sizeof(SrcList));
		if (pList == NULL) {
			fprintf(stderr, "Malloc failure\n");
			return NULL;
		}

		pList->nAlloc = 1;
	}

	if (pList->nSrc >= pList->nAlloc) {
		SrcList *pNew;
		pList->nAlloc *= 2;
		pNew = realloc(pList,
		    sizeof(*pList) + (pList->nAlloc-1)*sizeof(pList->a[0]));
		if (pNew == NULL) {
			fprintf(stderr, "Malloc failure\n");
			return NULL;
		}

		pList = pNew;
	}

	pItem = &pList->a[pList->nSrc];
	memset(pItem, 0, sizeof(pList->a[0]));

	if (pTable == NULL || pTable->n <= 0) {
		fprintf(stderr,
		    "Extracting name from a null or empty token\n");
		return NULL;
	}
	table_name = calloc(1, pTable->n + 1);
	if (table_name == NULL) {
		fprintf(stderr, "Malloc failure\n");
		return NULL;
	}
	
	memcpy(table_name, (char*)pTable->z, pTable->n);
	pItem->zName = table_name;
	pItem->zDatabase = NULL;
	pItem->iCursor = -1;
	pItem->isPopulated = 0;
	pList->nSrc++;

	return pList;
}

/*
 * We parse, and allow, foreign key constraints, but currently do not
 * use them.
 */
void
sqlite3CreateForeignKey(pParse, pFromCol, pTo, pToCol, flags)
	Parse *pParse;       /* Parsing context */
	ExprList *pFromCol;  /* Columns in that reference other table */
	Token *pTo;          /* Name of the other table */
	ExprList *pToCol;    /* Columns in the other table */
	int flags;           /* Conflict resolution algorithms. */
{
	char * s;

	COMPQUIET(flags, 0);

	if (debug) {
		if ((s = name_from_token(pTo, pParse)) == NULL)
			return;
		fprintf(stderr, "Foreign Key Constraint not implemented: \
FromTable %s FromCol %s ToTable %s ToCol %s\n", 
		    get_current_entity()->name, pFromCol->a->zName,
		    s, pToCol->a->zName);
		free(s);
	}
}

void
sqlite3DeferForeignKey(Parse *pParse, int isDeferred)
{
	COMPQUIET(pParse, NULL);
	COMPQUIET(isDeferred, 0);
	/* no-op */
}

void
sqlite3CreateIndex(pParse, pName1, pName2, pTblName, pList, 
    onError, pStart, pEnd, sortOrder, ifNotExist)
	Parse *pParse;     /* All information about this parse */
	Token *pName1;     /* First part of index name. May be NULL */
	Token *pName2;     /* Second part of index name. May be NULL */
	SrcList *pTblName; /* Table to index. Use pParse->pNewTable if 0 */
	ExprList *pList;   /* A list of columns to be indexed */
	int onError;       /* OE_Abort, OE_Ignore, OE_Replace, or OE_None */
	Token *pStart;     /* The CREATE token that begins this statement */
	Token *pEnd;       /* The ")" that closes the CREATE INDEX statement */
	int sortOrder;     /* Sort order of primary key when pList==NULL */
	int ifNotExist;    /* Omit error if index already exists */
{
	ENTITY *e, *extra_entity;
	ATTRIBUTE *a;
	DB_INDEX *idx;
	char *entity_name, *attribute_name, *index_name;

	COMPQUIET(pName2, NULL);
	COMPQUIET(onError, 0);
	COMPQUIET(pStart, NULL);
	COMPQUIET(pEnd, NULL);
	COMPQUIET(sortOrder, 0);
	COMPQUIET(ifNotExist, 0);

	entity_name = pTblName->a->zName;
	attribute_name = pList->a->zName;
	if ((index_name = name_from_token(pName1, pParse)) == NULL)
		return;

	e = get_entity_by_name(entity_name);
	if (e == NULL) {
		sqlite3ErrorMsg(pParse, "Index %s names unknown table %s",
		    index_name, entity_name);
		goto free_allocated_on_error;
	}

	a = get_attribute_by_name(e, attribute_name);
	if (a == NULL) {
		sqlite3ErrorMsg(pParse, 
		    "Index %s names unknown column %s in table %s",
		    index_name, attribute_name, entity_name);
		goto free_allocated_on_error;;
	}

	if ((idx = get_index_by_name(index_name)) != NULL) {
		sqlite3ErrorMsg(pParse,
		    "Found two declarations of an index named %s, \
at lines %d and %d",
		    index_name, idx->line_number, line_number);
		goto free_allocated_on_error;;
	}

	if ((extra_entity = get_entity_by_name(index_name)) != NULL) {
		sqlite3ErrorMsg(pParse,
		    "The index named %s on line %d has the same name as the \
table at line %d.  This is not allowed.",
		    index_name, line_number, 
		    extra_entity->line_number);
		goto free_allocated_on_error;
	}

	if ((idx = make_index(index_name, e, a, pParse)) == NULL)
		goto free_allocated_on_error;

	the_parse_progress.last_event = PE_INDEX;
	the_parse_progress.last_index = idx;
	add_index(idx);

	return;

free_allocated_on_error:
	if (index_name != NULL)
		free(index_name);
}

void sqlite3EndTable(pParse, pCons, pEnd, pSelect)
	Parse *pParse;          /* Parse context */
	Token *pCons;           /* The ',' token after the last column defn. */
	Token *pEnd;            /* The final ')' token in the CREATE TABLE */
	Select *pSelect;        /* Select from a "CREATE ... AS SELECT" */
{
	COMPQUIET(pParse, NULL);
	COMPQUIET(pCons, NULL);
	COMPQUIET(pEnd, NULL);
	COMPQUIET(pSelect, NULL);
	the_parse_progress.last_event = PE_ENTITY;
}

void
sqlite3FinishCoding(pParse)
	Parse *pParse;
{
	COMPQUIET(pParse, NULL);
	/* no-op */
}
