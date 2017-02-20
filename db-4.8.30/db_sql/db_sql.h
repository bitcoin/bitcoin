/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * Db_sql is a utility program that translates a schema description
 * written in a SQL Data Definition Language dialect into C code that
 * implements the schema using Berkeley DB.
 *
 * This include file declares the data structures used to model the
 * schema; along with macro definitions and function prototypes used
 * throughout the program.
 */

#include "sqlite/sqliteInt.h"

/*
 * Forward structure declarations and typedefs.
 */
struct __db_schema;        typedef struct __db_schema DB_SCHEMA;
struct __parse_progress;   typedef struct __parse_progress PARSE_PROGRESS;
struct __environment_info; typedef struct __environment_info ENVIRONMENT_INFO;
struct __db_entity;        typedef struct __db_entity ENTITY;
struct __db_attribute;     typedef struct __db_attribute ATTRIBUTE;
struct __db_attr_type;     typedef struct __db_attr_type ATTR_TYPE;
struct __db_index;         typedef struct __db_index DB_INDEX;

/*
 * Information about the BDB environment comes from a CREATE
 * DATABASE statement in the input DDL.
 */
struct __environment_info {
    char   *name;
    char   *uppercase_name;        /* A copy of the name in all caps. */
    int    line_number;
    unsigned long int cache_size;
};

/*
 * DDL statements are parsed into a tree of data structures
 * rooted in the global instance of this structure.
 */
struct __db_schema {
        ENVIRONMENT_INFO environment;
    ENTITY          *entities_head; /* head of the list of Entities */
    ENTITY          *entities_tail; /* the tail of that list */
    DB_INDEX        *indexes_head;  /* head of the list of DBIndexes */
    DB_INDEX        *indexes_tail;  /* tail of the list of DBIndexes */
};

/*
 * An ENTITY describes a table from the DDL.
 */
struct __db_entity {
    ENTITY          *next;
    char        *name; /* the name of this Entity */
    ATTRIBUTE   *attributes_head;
    ATTRIBUTE   *attributes_tail;
    ATTRIBUTE   *primary_key;
    char            *serialized_length_name; /* filled in when needed */
    int             line_number; /* line where this entity is declared */
    char            *dbtype;
};

/*
 * ATTRIBUTE is a fancy name for a column description
 */
struct __db_attribute {
    ATTRIBUTE   *next;
    char        *name; /* the name of this Attribute */
    ATTR_TYPE   *type; /* the given type, simply a copy of the tokens */
    int     is_autoincrement;
    char            *array_dim_name; /* filled in when needed */
    char            *decl_name; /* name with array dimension, when needed */
};

/*
 * ATTR_TYPE describes the type of an attribute
 */
struct __db_attr_type {
    char        *sql_token; /* the original type string from the sql */
    char        *c_type;    /* the mapped c-language type */
    int     array_dim;  /* The array size, if there is one */
    int             is_array;   /* boolean, tells if the c type is array */
        int             is_string;  /* boolean, tells if it's a char* string */
};


/*
 * DBIndex is what you get from CREATE_INDEX, it describes a secondary database
 */
struct __db_index {
    DB_INDEX        *next;
    char            *name;      /* the name of the index/secondary */
    ENTITY          *primary;   /* primary database this index refers to */
    ATTRIBUTE       *attribute; /* the indexed field */
    int             line_number; /* line where this index is declared */
    char            *dbtype;
};

/*
 * PARSE_PROGRESS is just a little housekeeping info about what was the
 * last thing parsed.
 */
enum parse_event {
    PE_NONE = 0, PE_ENVIRONMENT =1, PE_ENTITY = 2, 
    PE_ATTRIBUTE = 3, PE_INDEX = 4
};

struct __parse_progress {
    enum parse_event last_event;
    ENTITY           *last_entity;
        ATTRIBUTE        *last_attribute;
        DB_INDEX         *last_index;
};

extern DB_SCHEMA the_schema;
extern PARSE_PROGRESS the_parse_progress;
extern int line_number;
extern int debug;
extern char *me;

#define MAX_SQL_LENGTH 1000000000

#define KILO (1024)
#define MEGA (KILO * KILO)
#define GIGA (MEGA * KILO)
#define TERA (GIGA * KILO)

#if defined (_WIN32) && !defined(__MINGW_H)
#define snprintf sprintf_s
#define strdup _strdup
#define strconcat(dest, size, src) strcat_s(dest, size, src)
#define strnconcat(dest, size, src, count) strncat_s(dest, size, src, count)
#else
#define strconcat(dest, size, src) strcat(dest, src)
#define strnconcat(dest, size, src, count) strncat(dest, src, count)
#endif

/* Windows lacks strcasecmp; how convenient that sqlite has its own */
#define strcasecmp sqlite3StrICmp

extern void generate(FILE *, FILE *, FILE *, FILE *, char *);
extern void generate_test(FILE *, char *);
extern int do_parse(const char *, char **);
extern void parse_hint_comment(Token *);
extern void setString(char **, ...);
extern void preparser(void *, int,Token, Parse *);
extern void bdb_create_database(Token *, Parse *);

/*
 * To avoid warnings on unused functionarguments and variables, we
 * write and then read the variable.
 */
#define COMPQUIET(n, v) do {                            \
    (n) = (v);                              \
    (n) = (n);                              \
} while (0)
