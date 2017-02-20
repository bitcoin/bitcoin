/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

extern DbRecord DbRecord_base;          /* Initialized structure. */

/*
 * Prototypes
 */
extern int  DbRecord_discard(DbRecord *);
extern int  DbRecord_init(const DBT *, const DBT *, DbRecord *);
extern void DbRecord_print(DbRecord *, FILE *);
extern int  DbRecord_read(u_long, DbRecord *);
extern int  DbRecord_search_field_name(char *, char *, OPERATOR);
extern int  DbRecord_search_field_number(u_int, char *, OPERATOR);
extern int  compare_double(DB *, const DBT *, const DBT *);
extern int  compare_string(DB *, const DBT *, const DBT *);
extern int  compare_ulong(DB *, const DBT *, const DBT *);
extern int  csv_env_close(void);
extern int  csv_env_open(const char *, int);
extern int  csv_secondary_close(void);
extern int  csv_secondary_open(void);
extern int  entry_print(void *, size_t, u_int32_t);
extern int  field_cmp_double(void *, void *, OPERATOR);
extern int  field_cmp_re(void *, void *, OPERATOR);
extern int  field_cmp_string(void *, void *, OPERATOR);
extern int  field_cmp_ulong(void *, void *, OPERATOR);
extern int  input_load(input_fmt, u_long);
extern int  query(char *, int *);
extern int  query_interactive(void);
extern int  secondary_callback(DB *, const DBT *, const DBT *, DBT *);
extern int  strtod_err(char *, double *);
extern int  strtoul_err(char *, u_long *);
