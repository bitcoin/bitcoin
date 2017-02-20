/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: George Schlossnagle <george@omniti.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// this is here to work around a PHP build issue on Windows
#include <iostream>

extern "C"
{
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_db4.h"
}
#ifdef HAVE_MOD_DB4
#include "mod_db4_export.h"
#else
#include "db_cxx.h"
#endif

#ifdef HAVE_MOD_DB4
    #define my_db_create mod_db4_db_create
    #define my_db_env_create mod_db4_db_env_create
#else 
    #define my_db_create db_create
    #define my_db_env_create db_env_create
#endif

#if PHP_MAJOR_VERSION <= 4
unsigned char second_arg_force_ref[] = { 2, BYREF_NONE, BYREF_FORCE };
unsigned char third_arg_force_ref[] = { 3, BYREF_NONE, BYREF_NONE, BYREF_FORCE };
#endif

/* True global resources - no need for thread safety here */
static int le_db;
static int le_dbc;
static int le_db_txn;
static int le_dbenv;

struct php_DB_TXN {
    DB_TXN *db_txn;
    struct my_llist *open_cursors;
    struct my_llist *open_dbs;
};

struct php_DBC {
    DBC *dbc;
    struct php_DB_TXN *parent_txn;
    struct php_DB *parent_db;
};

struct php_DB {
    DB *db;
    struct my_llist *open_cursors;
};

struct php_DB_ENV {
    DB_ENV *dbenv;
};

static void _free_php_db_txn(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    struct php_DB_TXN *pdbtxn = (struct php_DB_TXN *) rsrc->ptr;
    /* should probably iterate over open_cursors */
#ifndef HAVE_MOD_DB4
    if(pdbtxn->db_txn) pdbtxn->db_txn->abort(pdbtxn->db_txn);   
    pdbtxn->db_txn = NULL;
#endif
    if(pdbtxn) efree(pdbtxn);
}

static void _free_php_dbc(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    struct php_DBC *pdbc = (struct php_DBC *) rsrc->ptr;
#ifndef HAVE_MOD_DB4
    pdbc->dbc = NULL;
#endif
    if(pdbc) efree(pdbc);
    rsrc->ptr = NULL;
}

static void _free_php_db(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    struct php_DB *pdb = (struct php_DB *) rsrc->ptr;
#ifndef HAVE_MOD_DB4
    if(pdb->db) pdb->db->close(pdb->db, 0);
    pdb->db = NULL;
#endif
    if(pdb) efree(pdb);
}

static void _free_php_dbenv(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    struct php_DB_ENV *pdb = (struct php_DB_ENV *)rsrc->ptr;
#ifndef HAVE_MOD_DB4
    DbEnv *dbe;
    if(pdb->dbenv) {
        dbe = DbEnv::get_DbEnv(pdb->dbenv);
        if(dbe) dbe->close(0);
        delete dbe;
    }
#endif
    if(pdb) efree(pdb);
}

static zend_class_entry *db_txn_ce;
static zend_class_entry *dbc_ce;
static zend_class_entry *db_ce;
static zend_class_entry *db_env_ce;

/* helpers */
struct my_llist {
    void *data;
    struct my_llist *next;
    struct my_llist *prev;
};

static struct my_llist *my_llist_add(struct my_llist *list, void *data) {
    if(!list) { 
        list = (struct my_llist *)emalloc(sizeof(*list));
        list->data = data;
        list->next = list->prev = NULL;
        return list;
    } else {
        struct my_llist *node;
        node = (struct my_llist *)emalloc(sizeof(*node));
        node->data = data;
        node->next = list;
        node->prev = NULL;
        return node;
    }
}

static struct my_llist *my_llist_del(struct my_llist *list, void *data) {
    struct my_llist *ptr = list;
    if(!ptr) return NULL;
    if(ptr->data == data) { /* special case, first element */
        ptr = ptr->next;
        efree(list);
        return ptr;
    }
    while(ptr) {
        if(data == ptr->data) {
            if(ptr->prev) ptr->prev->next = ptr->next;
            if(ptr->next) ptr->next->prev = ptr->prev;
            efree(ptr);
            break;
        }
        ptr = ptr->next;
    }
    return list;
}

/* {{{ db4_functions[]
 *
 * Every user visible function must have an entry in db4_functions[].
 */
function_entry db4_functions[] = {
    /* PHP_FE(db4_dbenv_create, NULL) */
    {NULL, NULL, NULL}  /* Must be the last line in db4_functions[] */
};
/* }}} */

PHP_MINIT_FUNCTION(db4);
PHP_MSHUTDOWN_FUNCTION(db4);
PHP_RINIT_FUNCTION(db4);
PHP_RSHUTDOWN_FUNCTION(db4);
PHP_MINFO_FUNCTION(db4);

/* {{{ db4_module_entry
 */
zend_module_entry db4_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "db4",
    db4_functions,
    PHP_MINIT(db4),
    PHP_MSHUTDOWN(db4),
    NULL,
    NULL,
    PHP_MINFO(db4),
    "0.9", /* Replace with version number for your extension */
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

/* {{{ class entries 
 */

/* {{{ DB4Txn method forward declarations
 */

zend_class_entry *db_txn_ce_get(void)
{
    return db_txn_ce;
}

ZEND_NAMED_FUNCTION(_wrap_db_txn_abort);
ZEND_NAMED_FUNCTION(_wrap_db_txn_commit);
ZEND_NAMED_FUNCTION(_wrap_db_txn_discard);
ZEND_NAMED_FUNCTION(_wrap_db_txn_id);
ZEND_NAMED_FUNCTION(_wrap_db_txn_set_timeout);
ZEND_NAMED_FUNCTION(_wrap_db_txn_set_name);
ZEND_NAMED_FUNCTION(_wrap_db_txn_get_name);
ZEND_NAMED_FUNCTION(_wrap_new_DbTxn);

static zend_function_entry DbTxn_functions[] = {
        ZEND_NAMED_FE(abort, _wrap_db_txn_abort, NULL)
        ZEND_NAMED_FE(commit, _wrap_db_txn_commit, NULL)
        ZEND_NAMED_FE(discard, _wrap_db_txn_discard, NULL)
        ZEND_NAMED_FE(id, _wrap_db_txn_id, NULL)
        ZEND_NAMED_FE(set_timeout, _wrap_db_txn_set_timeout, NULL)
        ZEND_NAMED_FE(set_name, _wrap_db_txn_set_name, NULL)
        ZEND_NAMED_FE(get_name, _wrap_db_txn_get_name, NULL)
        ZEND_NAMED_FE(db4txn, _wrap_new_DbTxn, NULL)
        { NULL, NULL, NULL}
};
/* }}} */

/* {{{ DB4Cursor method forward declarations
 */

zend_class_entry *dbc_ce_get(void)
{
    return dbc_ce;
}

ZEND_NAMED_FUNCTION(_wrap_dbc_close);
ZEND_NAMED_FUNCTION(_wrap_dbc_count);
ZEND_NAMED_FUNCTION(_wrap_dbc_del);
ZEND_NAMED_FUNCTION(_wrap_dbc_dup);
ZEND_NAMED_FUNCTION(_wrap_dbc_get);
ZEND_NAMED_FUNCTION(_wrap_dbc_put);
ZEND_NAMED_FUNCTION(_wrap_dbc_pget);

#ifdef ZEND_ENGINE_2
ZEND_BEGIN_ARG_INFO(first_and_second_args_force_ref, 0)
    ZEND_ARG_PASS_INFO(1)
    ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(first_and_second_and_third_args_force_ref, 0)
    ZEND_ARG_PASS_INFO(1)
    ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO();

#else
static unsigned char first_and_second_args_force_ref[] = {2, BYREF_FORCE, BYREF_FORCE };
static unsigned char first_and_second_and_third_args_force_ref[] = {3, BYREF_FORCE, BYREF_FORCE, BYREF_FORCE };
#endif

static zend_function_entry Dbc_functions[] = {
        ZEND_NAMED_FE(close, _wrap_dbc_close, NULL)
        ZEND_NAMED_FE(count, _wrap_dbc_count, NULL)
        ZEND_NAMED_FE(del, _wrap_dbc_del, NULL)
        ZEND_NAMED_FE(dup, _wrap_dbc_dup, NULL)
        ZEND_NAMED_FE(get, _wrap_dbc_get, first_and_second_args_force_ref)
        ZEND_NAMED_FE(put, _wrap_dbc_put, NULL)
        ZEND_NAMED_FE(pget, _wrap_dbc_pget, first_and_second_and_third_args_force_ref)
        { NULL, NULL, NULL}
};
/* }}} */

/* {{{ DB4Env method forward declarations 
 */

zend_class_entry *db_env_ce_get(void)
{
    return db_env_ce;
}

ZEND_NAMED_FUNCTION(_wrap_new_DbEnv);
ZEND_NAMED_FUNCTION(_wrap_db_env_close);
ZEND_NAMED_FUNCTION(_wrap_db_env_dbremove);
ZEND_NAMED_FUNCTION(_wrap_db_env_dbrename);
ZEND_NAMED_FUNCTION(_wrap_db_env_get_encrypt_flags);
ZEND_NAMED_FUNCTION(_wrap_db_env_open);
ZEND_NAMED_FUNCTION(_wrap_db_env_remove);
ZEND_NAMED_FUNCTION(_wrap_db_env_set_data_dir);
ZEND_NAMED_FUNCTION(_wrap_db_env_set_encrypt);
ZEND_NAMED_FUNCTION(_wrap_db_env_txn_begin);
ZEND_NAMED_FUNCTION(_wrap_db_env_txn_checkpoint);

static zend_function_entry DbEnv_functions[] = {
        ZEND_NAMED_FE(db4env, _wrap_new_DbEnv, NULL)
        ZEND_NAMED_FE(close, _wrap_db_env_close, NULL)
        ZEND_NAMED_FE(dbremove, _wrap_db_env_dbremove, NULL)
        ZEND_NAMED_FE(dbrename, _wrap_db_env_dbrename, NULL)
        ZEND_NAMED_FE(get_encrypt, _wrap_db_env_get_encrypt_flags, NULL)
        ZEND_NAMED_FE(open, _wrap_db_env_open, NULL)
        ZEND_NAMED_FE(remove, _wrap_db_env_remove, NULL)
        ZEND_NAMED_FE(set_data_dir, _wrap_db_env_set_data_dir, NULL)
        ZEND_NAMED_FE(set_encrypt, _wrap_db_env_set_encrypt, NULL)
        ZEND_NAMED_FE(txn_begin, _wrap_db_env_txn_begin, NULL)
        ZEND_NAMED_FE(txn_checkpoint, _wrap_db_env_txn_checkpoint, NULL)
        { NULL, NULL, NULL}
};

/* }}} */

/* {{{ DB4 method forward declarations
 */

zend_class_entry *db_ce_get(void)
{
    return db_ce;
}

ZEND_NAMED_FUNCTION(_wrap_new_db4);
ZEND_NAMED_FUNCTION(_wrap_db_open);
ZEND_NAMED_FUNCTION(_wrap_db_close);
ZEND_NAMED_FUNCTION(_wrap_db_del);
ZEND_NAMED_FUNCTION(_wrap_db_get);
ZEND_NAMED_FUNCTION(_wrap_db_get_encrypt_flags);
ZEND_NAMED_FUNCTION(_wrap_db_pget);
ZEND_NAMED_FUNCTION(_wrap_db_get_type);
ZEND_NAMED_FUNCTION(_wrap_db_join);
ZEND_NAMED_FUNCTION(_wrap_db_put);
ZEND_NAMED_FUNCTION(_wrap_db_set_encrypt);
ZEND_NAMED_FUNCTION(_wrap_db_stat);
ZEND_NAMED_FUNCTION(_wrap_db_sync);
ZEND_NAMED_FUNCTION(_wrap_db_truncate);
ZEND_NAMED_FUNCTION(_wrap_db_cursor);

static zend_function_entry Db4_functions[] = {
        ZEND_NAMED_FE(db4, _wrap_new_db4, NULL)
        ZEND_NAMED_FE(open, _wrap_db_open, NULL)
        ZEND_NAMED_FE(close, _wrap_db_close, NULL)
        ZEND_NAMED_FE(cursor, _wrap_db_cursor, NULL)
        ZEND_NAMED_FE(del, _wrap_db_del, NULL)
        ZEND_NAMED_FE(get, _wrap_db_get, NULL)
        ZEND_NAMED_FE(get_encrypt_flags, _wrap_db_get_encrypt_flags, NULL)
        ZEND_NAMED_FE(pget, _wrap_db_pget, second_arg_force_ref)
        ZEND_NAMED_FE(get_type, _wrap_db_get_type, NULL)
        ZEND_NAMED_FE(join, _wrap_db_join, NULL)
        ZEND_NAMED_FE(put, _wrap_db_put, NULL)
        ZEND_NAMED_FE(set_encrypt, _wrap_db_set_encrypt, NULL)
        ZEND_NAMED_FE(stat, _wrap_db_stat, NULL) 
        ZEND_NAMED_FE(sync, _wrap_db_sync, NULL)
        ZEND_NAMED_FE(truncate, _wrap_db_truncate, NULL)
        { NULL, NULL, NULL}
};
/* }}} */
/* }}} */

#ifdef COMPILE_DL_DB4
#ifdef PHP_WIN32
#include "zend_arg_defs.c"
#endif
BEGIN_EXTERN_C()
ZEND_GET_MODULE(db4)
END_EXTERN_C()
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
PHP_INI_END()
*/
/* }}} */

/* {{{ php_db4_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_db4_init_globals(zend_db4_globals *db4_globals)
{
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(db4)
{
    /* If you have INI entries, uncomment these lines 
    ZEND_INIT_MODULE_GLOBALS(db4, php_db4_init_globals, NULL);
    REGISTER_INI_ENTRIES();
    */
    static zend_class_entry _db_txn_ce;
    static zend_class_entry _dbc_ce;
    static zend_class_entry _db_ce;
    static zend_class_entry _db_env_ce;

    INIT_CLASS_ENTRY(_db_txn_ce, "db4txn", DbTxn_functions);
    db_txn_ce = zend_register_internal_class(&_db_txn_ce TSRMLS_CC);

    INIT_CLASS_ENTRY(_dbc_ce, "db4cursor", Dbc_functions);
    dbc_ce = zend_register_internal_class(&_dbc_ce TSRMLS_CC);

    INIT_CLASS_ENTRY(_db_ce, "db4", Db4_functions);
    db_ce = zend_register_internal_class(&_db_ce TSRMLS_CC);

    INIT_CLASS_ENTRY(_db_env_ce, "db4env", DbEnv_functions);
    db_env_ce = zend_register_internal_class(&_db_env_ce TSRMLS_CC);

    le_db_txn = zend_register_list_destructors_ex(_free_php_db_txn, NULL, "Db4Txn", module_number); 
    le_dbc = zend_register_list_destructors_ex(_free_php_dbc, NULL, "Db4Cursor", module_number);    
    le_db = zend_register_list_destructors_ex(_free_php_db, NULL, "Db4", module_number);    
    le_dbenv = zend_register_list_destructors_ex(_free_php_dbenv, NULL, "Db4Env", module_number);   

    REGISTER_LONG_CONSTANT("DB_VERSION_MAJOR", DB_VERSION_MAJOR, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_VERSION_MINOR", DB_VERSION_MINOR, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_VERSION_PATCH", DB_VERSION_PATCH, CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("DB_VERSION_STRING", DB_VERSION_STRING, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MAX_PAGES", DB_MAX_PAGES, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MAX_RECORDS", DB_MAX_RECORDS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DBT_APPMALLOC", DB_DBT_APPMALLOC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DBT_ISSET", DB_DBT_ISSET, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DBT_MALLOC", DB_DBT_MALLOC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DBT_PARTIAL", DB_DBT_PARTIAL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DBT_REALLOC", DB_DBT_REALLOC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DBT_USERMEM", DB_DBT_USERMEM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DBT_DUPOK", DB_DBT_DUPOK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_CREATE", DB_CREATE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_CXX_NO_EXCEPTIONS", DB_CXX_NO_EXCEPTIONS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_FORCE", DB_FORCE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOMMAP", DB_NOMMAP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RDONLY", DB_RDONLY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RECOVER", DB_RECOVER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MULTIVERSION", DB_MULTIVERSION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_TXN_SNAPSHOT", DB_TXN_SNAPSHOT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_THREAD", DB_THREAD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_TRUNCATE", DB_TRUNCATE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_TXN_NOSYNC", DB_TXN_NOSYNC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_TXN_NOT_DURABLE", DB_TXN_NOT_DURABLE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_USE_ENVIRON", DB_USE_ENVIRON, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_USE_ENVIRON_ROOT", DB_USE_ENVIRON_ROOT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_AUTO_COMMIT", DB_AUTO_COMMIT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DIRTY_READ", DB_READ_UNCOMMITTED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DEGREE_2", DB_READ_COMMITTED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_READ_COMMITTED", DB_READ_COMMITTED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_READ_UNCOMMITTED", DB_READ_UNCOMMITTED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NO_AUTO_COMMIT", DB_NO_AUTO_COMMIT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RPCCLIENT", DB_RPCCLIENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_INIT_CDB", DB_INIT_CDB, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_INIT_LOCK", DB_INIT_LOCK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_INIT_LOG", DB_INIT_LOG, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_INIT_MPOOL", DB_INIT_MPOOL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_INIT_REP", DB_INIT_REP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_INIT_TXN", DB_INIT_TXN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_JOINENV", DB_JOINENV, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCKDOWN", DB_LOCKDOWN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_PRIVATE", DB_PRIVATE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RECOVER_FATAL", DB_RECOVER_FATAL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SYSTEM_MEM", DB_SYSTEM_MEM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_EXCL", DB_EXCL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_FCNTL_LOCKING", DB_FCNTL_LOCKING, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RDWRMASTER", DB_RDWRMASTER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_WRITEOPEN", DB_WRITEOPEN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_TXN_NOWAIT", DB_TXN_NOWAIT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_TXN_SYNC", DB_TXN_SYNC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_ENCRYPT_AES", DB_ENCRYPT_AES, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_CDB_ALLDB", DB_CDB_ALLDB, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DIRECT_DB", DB_DIRECT_DB, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOLOCKING", DB_NOLOCKING, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOPANIC", DB_NOPANIC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_OVERWRITE", DB_OVERWRITE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_PANIC_ENVIRONMENT", DB_PANIC_ENVIRONMENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REGION_INIT", DB_REGION_INIT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_TIME_NOTGRANTED", DB_TIME_NOTGRANTED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_TXN_WRITE_NOSYNC", DB_TXN_WRITE_NOSYNC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_YIELDCPU", DB_YIELDCPU, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_UPGRADE", DB_UPGRADE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_VERIFY", DB_VERIFY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DIRECT", DB_DIRECT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_EXTENT", DB_EXTENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_ODDFILESIZE", DB_ODDFILESIZE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_CHKSUM", DB_CHKSUM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DUP", DB_DUP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DUPSORT", DB_DUPSORT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_ENCRYPT", DB_ENCRYPT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RECNUM", DB_RECNUM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RENUMBER", DB_RENUMBER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REVSPLITOFF", DB_REVSPLITOFF, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SNAPSHOT", DB_SNAPSHOT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_STAT_CLEAR", DB_STAT_CLEAR, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_JOIN_NOSORT", DB_JOIN_NOSORT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_AGGRESSIVE", DB_AGGRESSIVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOORDERCHK", DB_NOORDERCHK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_ORDERCHKONLY", DB_ORDERCHKONLY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_PR_PAGE", DB_PR_PAGE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_PR_RECOVERYTEST", DB_PR_RECOVERYTEST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_PRINTABLE", DB_PRINTABLE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SALVAGE", DB_SALVAGE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_NOBUFFER", DB_REP_NOBUFFER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_PERMANENT", DB_REP_PERMANENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCKVERSION", DB_LOCKVERSION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_FILE_ID_LEN", DB_FILE_ID_LEN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_NORUN", DB_LOCK_NORUN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_DEFAULT", DB_LOCK_DEFAULT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_EXPIRE", DB_LOCK_EXPIRE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_MAXLOCKS", DB_LOCK_MAXLOCKS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_MINLOCKS", DB_LOCK_MINLOCKS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_MINWRITE", DB_LOCK_MINWRITE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_OLDEST", DB_LOCK_OLDEST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_RANDOM", DB_LOCK_RANDOM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_YOUNGEST", DB_LOCK_YOUNGEST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_NOWAIT", DB_LOCK_NOWAIT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_RECORD", DB_LOCK_RECORD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_SET_TIMEOUT", DB_LOCK_SET_TIMEOUT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_SWITCH", DB_LOCK_SWITCH, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_UPGRADE", DB_LOCK_UPGRADE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_HANDLE_LOCK", DB_HANDLE_LOCK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RECORD_LOCK", DB_RECORD_LOCK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_PAGE_LOCK", DB_PAGE_LOCK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOGVERSION", DB_LOGVERSION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOGOLDVER", DB_LOGOLDVER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOGMAGIC", DB_LOGMAGIC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_ARCH_ABS", DB_ARCH_ABS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_ARCH_DATA", DB_ARCH_DATA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_ARCH_LOG", DB_ARCH_LOG, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_ARCH_REMOVE", DB_ARCH_REMOVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_FLUSH", DB_FLUSH, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOG_CHKPNT", DB_LOG_CHKPNT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOG_COMMIT", DB_LOG_COMMIT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOG_NOCOPY", DB_LOG_NOCOPY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOG_NOT_DURABLE", DB_LOG_NOT_DURABLE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOG_WRNOSYNC", DB_LOG_WRNOSYNC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_user_BEGIN", DB_user_BEGIN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_debug_FLAG", DB_debug_FLAG, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOG_DISK", DB_LOG_DISK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOG_LOCKED", DB_LOG_LOCKED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOG_SILENT_ERR", DB_LOG_SILENT_ERR, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MPOOL_CREATE", DB_MPOOL_CREATE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MPOOL_LAST", DB_MPOOL_LAST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MPOOL_NEW", DB_MPOOL_NEW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MPOOL_DIRTY", DB_MPOOL_DIRTY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MPOOL_DISCARD", DB_MPOOL_DISCARD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MPOOL_NOFILE", DB_MPOOL_NOFILE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MPOOL_UNLINK", DB_MPOOL_UNLINK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_TXNVERSION", DB_TXNVERSION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_GID_SIZE", DB_GID_SIZE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_EID_BROADCAST", DB_EID_BROADCAST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_EID_INVALID", DB_EID_INVALID, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_CLIENT", DB_REP_CLIENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_MASTER", DB_REP_MASTER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RENAMEMAGIC", DB_RENAMEMAGIC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_BTREEVERSION", DB_BTREEVERSION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_BTREEOLDVER", DB_BTREEOLDVER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_BTREEMAGIC", DB_BTREEMAGIC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_HASHVERSION", DB_HASHVERSION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_HASHOLDVER", DB_HASHOLDVER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_HASHMAGIC", DB_HASHMAGIC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_QAMVERSION", DB_QAMVERSION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_QAMOLDVER", DB_QAMOLDVER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_QAMMAGIC", DB_QAMMAGIC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_AFTER", DB_AFTER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_APPEND", DB_APPEND, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_BEFORE", DB_BEFORE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_CONSUME", DB_CONSUME, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_CONSUME_WAIT", DB_CONSUME_WAIT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_CURRENT", DB_CURRENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_FAST_STAT", DB_FAST_STAT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_FIRST", DB_FIRST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_GET_BOTH", DB_GET_BOTH, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_GET_BOTHC", DB_GET_BOTHC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_GET_BOTH_RANGE", DB_GET_BOTH_RANGE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_GET_RECNO", DB_GET_RECNO, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_JOIN_ITEM", DB_JOIN_ITEM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_KEYFIRST", DB_KEYFIRST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_KEYLAST", DB_KEYLAST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LAST", DB_LAST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NEXT", DB_NEXT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NEXT_DUP", DB_NEXT_DUP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NEXT_NODUP", DB_NEXT_NODUP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NODUPDATA", DB_NODUPDATA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOOVERWRITE", DB_NOOVERWRITE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOSYNC", DB_NOSYNC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_POSITION", DB_POSITION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_PREV", DB_PREV, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_PREV_NODUP", DB_PREV_NODUP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SET", DB_SET, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SET_LOCK_TIMEOUT", DB_SET_LOCK_TIMEOUT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SET_RANGE", DB_SET_RANGE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SET_RECNO", DB_SET_RECNO, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SET_TXN_NOW", DB_SET_TXN_NOW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SET_TXN_TIMEOUT", DB_SET_TXN_TIMEOUT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_UPDATE_SECONDARY", DB_UPDATE_SECONDARY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_WRITECURSOR", DB_WRITECURSOR, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_WRITELOCK", DB_WRITELOCK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_OPFLAGS_MASK", DB_OPFLAGS_MASK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MULTIPLE", DB_MULTIPLE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_MULTIPLE_KEY", DB_MULTIPLE_KEY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RMW", DB_RMW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DONOTINDEX", DB_DONOTINDEX, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_KEYEMPTY", DB_KEYEMPTY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_KEYEXIST", DB_KEYEXIST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_DEADLOCK", DB_LOCK_DEADLOCK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_LOCK_NOTGRANTED", DB_LOCK_NOTGRANTED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOSERVER", DB_NOSERVER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOSERVER_HOME", DB_NOSERVER_HOME, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOSERVER_ID", DB_NOSERVER_ID, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_NOTFOUND", DB_NOTFOUND, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_OLD_VERSION", DB_OLD_VERSION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_PAGE_NOTFOUND", DB_PAGE_NOTFOUND, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_DUPMASTER", DB_REP_DUPMASTER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_HANDLE_DEAD", DB_REP_HANDLE_DEAD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_HOLDELECTION", DB_REP_HOLDELECTION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_ISPERM", DB_REP_ISPERM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_NEWMASTER", DB_REP_NEWMASTER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_NEWSITE", DB_REP_NEWSITE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_NOTPERM", DB_REP_NOTPERM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_REP_UNAVAIL", DB_REP_UNAVAIL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_RUNRECOVERY", DB_RUNRECOVERY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_SECONDARY_BAD", DB_SECONDARY_BAD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_VERIFY_BAD", DB_VERIFY_BAD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_VERB_DEADLOCK", DB_VERB_DEADLOCK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_VERB_RECOVERY", DB_VERB_RECOVERY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_VERB_REPLICATION", DB_VERB_REPLICATION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_VERB_WAITSFOR", DB_VERB_WAITSFOR, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DB_DBM_HSEARCH", DB_DBM_HSEARCH, CONST_CS | CONST_PERSISTENT);
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(db4)
{
    /* uncomment this line if you have INI entries
    UNREGISTER_INI_ENTRIES();
    */
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(db4)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "db4 support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
    */
}
/* }}} */


/* {{{ resource accessors 
 */
void setDbEnv(zval *z, DB_ENV *dbenv TSRMLS_DC)
{
    long rsrc_id;
    struct php_DB_ENV *pdb = (struct php_DB_ENV *) emalloc(sizeof(*pdb));
    pdb->dbenv = dbenv;
    rsrc_id = zend_register_resource(NULL, pdb, le_dbenv);
    zend_list_addref(rsrc_id);
    add_property_resource(z, "_dbenv_ptr", rsrc_id);
}

DB_ENV *php_db4_getDbEnvFromObj(zval *z TSRMLS_DC)
{
    struct php_DB_ENV *pdb;
    zval **rsrc;
    if(zend_hash_find(HASH_OF(z), "_dbenv_ptr", sizeof("_dbenv_ptr"), 
          (void **) &rsrc) == SUCCESS) 
    {
        pdb = (struct php_DB_ENV *) zend_fetch_resource(rsrc TSRMLS_CC, -1, "Db4Env", NULL, 1, le_dbenv);
        return pdb->dbenv;
    }
    return NULL;
}

struct php_DB_ENV *php_db4_getPhpDbEnvFromObj(zval *z TSRMLS_DC)
{
    struct php_DB_ENV *pdb;
    zval **rsrc;
    if(zend_hash_find(HASH_OF(z), "_dbenv_ptr", sizeof("_dbenv_ptr"), 
          (void **) &rsrc) == SUCCESS) 
    {
        pdb = (struct php_DB_ENV *) zend_fetch_resource(rsrc TSRMLS_CC, -1, "Db4Env", NULL, 1, le_dbenv);
        return pdb;
    }
    return NULL;
}

#define getDbEnvFromThis(a)        \
do { \
  zval *_self = getThis(); \
  if(!_self) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "must be called as a method"); \
    RETURN_FALSE; \
  } \
  (a) = php_db4_getDbEnvFromObj(_self TSRMLS_CC); \
  if(!(a)) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "not a valid db4Env object"); \
    RETURN_FALSE; \
  } \
} while(0)

void setDb(zval *z, DB *db TSRMLS_DC)
{
    long rsrc_id;
    struct php_DB *pdb = (struct php_DB *) emalloc(sizeof(*pdb));
    memset(pdb, 0, sizeof(*pdb));
    pdb->db = db;
    rsrc_id = ZEND_REGISTER_RESOURCE(NULL, pdb, le_db);
    add_property_resource(z, "_db_ptr", rsrc_id);
}

struct php_DB *getPhpDbFromObj(zval *z TSRMLS_DC)
{
    struct php_DB *pdb;
    zval **rsrc;
    if(zend_hash_find(HASH_OF(z), "_db_ptr", sizeof("_db_ptr"), (void **) &rsrc) == SUCCESS) {
        pdb = (struct php_DB *) zend_fetch_resource(rsrc TSRMLS_CC, -1, "Db4", NULL, 1, le_db);
        return pdb;
    }
    return NULL;
}

DB *php_db4_getDbFromObj(zval *z TSRMLS_DC)
{
    struct php_DB *pdb;
    zval **rsrc;
    if(zend_hash_find(HASH_OF(z), "_db_ptr", sizeof("_db_ptr"), (void **) &rsrc) == SUCCESS) {
        pdb = (struct php_DB *) zend_fetch_resource(rsrc TSRMLS_CC, -1, "Db4", NULL, 1, le_db);
        return pdb->db;
    }
    return NULL;
}

#define getDbFromThis(a)        \
do { \
  struct php_DB *pdb; \
  zval *_self = getThis(); \
  if(!_self) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "must be called as a method"); \
    RETURN_FALSE; \
  } \
   pdb = getPhpDbFromObj(_self TSRMLS_CC); \
  if(!pdb || !pdb->db) { \
    assert(0); \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "not a valid db4 object"); \
    RETURN_FALSE; \
  } \
  (a) = pdb->db; \
} while(0)

#define getPhpDbFromThis(a)        \
do { \
  struct php_DB *pdb; \
  zval *_self = getThis(); \
  if(!_self) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "must be called as a method"); \
    RETURN_FALSE; \
  } \
   pdb = getPhpDbFromObj(_self TSRMLS_CC); \
  if(!pdb) { \
    assert(0); \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "not a valid db4 object"); \
    RETURN_FALSE; \
  } \
  (a) = pdb; \
} while(0)

void setDbTxn(zval *z, DB_TXN *dbtxn TSRMLS_DC)
{
    long rsrc_id;
    struct php_DB_TXN *txn = (struct php_DB_TXN *) emalloc(sizeof(*txn));
    memset(txn, 0, sizeof(*txn));
    txn->db_txn = dbtxn;
    rsrc_id = ZEND_REGISTER_RESOURCE(NULL, txn, le_db_txn);
    zend_list_addref(rsrc_id);
    add_property_resource(z, "_dbtxn_ptr", rsrc_id);
}

DB_TXN *php_db4_getDbTxnFromObj(zval *z TSRMLS_DC)
{
    struct php_DB_TXN *pdbtxn;
    zval **rsrc;
    if(zend_hash_find(HASH_OF(z), "_dbtxn_ptr", sizeof("_dbtxn_ptr"), 
          (void **) &rsrc) == SUCCESS) 
    {
        pdbtxn = (struct php_DB_TXN *) zend_fetch_resource(rsrc TSRMLS_CC, -1, "Db4Txn", NULL, 1, le_db_txn);
        return pdbtxn->db_txn;
    }
    return NULL;
}

struct php_DB_TXN *getPhpDbTxnFromObj(zval *z TSRMLS_DC)
{
    struct php_DB_TXN *pdbtxn;
    zval **rsrc;
    if(zend_hash_find(HASH_OF(z), "_dbtxn_ptr", sizeof("_dbtxn_ptr"), 
          (void **) &rsrc) == SUCCESS) 
    {
        pdbtxn = (struct php_DB_TXN *) zend_fetch_resource(rsrc TSRMLS_CC, -1, "Db4Txn", NULL, 1, le_db_txn);
        return pdbtxn;
    }
    return NULL;
}

#define getDbTxnFromThis(a)        \
do { \
  zval *_self = getThis(); \
  if(!_self) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "must be called as a method"); \
    RETURN_FALSE; \
  } \
  (a) = php_db4_getDbTxnFromObj(_self TSRMLS_CC); \
  if(!(a)) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "not a valid db4txn object"); \
    RETURN_FALSE; \
  } \
} while(0)

#define getPhpDbTxnFromThis(a)        \
do { \
  zval *_self = getThis(); \
  if(!_self) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "must be called as a method"); \
    RETURN_FALSE; \
  } \
  (a) = getPhpDbTxnFromObj(_self TSRMLS_CC); \
  if(!(a) || !(a)->db_txn) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "not a valid db4txn object"); \
    RETURN_FALSE; \
  } \
} while(0)

void closeDbTxnDependencies(zval *obj TSRMLS_DC) {
    struct php_DB_TXN *pdbtxn = getPhpDbTxnFromObj(obj TSRMLS_CC);
    if(pdbtxn) {
        while(pdbtxn->open_cursors) {
            struct my_llist *el = pdbtxn->open_cursors;
            struct php_DBC *pdbc = (struct php_DBC *) el->data;
            if(pdbc) {
                if(pdbc->dbc) {
                    pdbc->dbc->c_close(pdbc->dbc);
                    pdbc->dbc = NULL;
                }
                pdbc->parent_txn = NULL;
            }
//          efree(el->data);
            pdbtxn->open_cursors = el->next;
            efree(el);
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Attempting to end a transaction without closing it's child cursors.");
        }
        /* should handle open dbs with pending transactions */
    }
}


void setDbc(zval *z, DBC *dbc, struct php_DB_TXN *txn TSRMLS_DC)
{
    long rsrc_id;
    struct php_DBC *pdbc = (struct php_DBC *) emalloc(sizeof(*pdbc));
    memset(pdbc, 0, sizeof(*pdbc));
    pdbc->dbc = dbc;
    if(txn) {
        pdbc->parent_txn = txn;
        txn->open_cursors = my_llist_add(txn->open_cursors, pdbc);
    }
    rsrc_id = zend_register_resource(NULL, pdbc, le_dbc);
    zend_list_addref(rsrc_id);
    add_property_resource(z, "_dbc_ptr", rsrc_id);
}

DBC *php_db4_getDbcFromObj(zval *z TSRMLS_DC)
{
    struct php_DBC *pdbc;
    zval **rsrc;
    if(zend_hash_find(HASH_OF(z), "_dbc_ptr", sizeof("_dbc_ptr"), 
          (void **) &rsrc) == SUCCESS) 
    {
        pdbc = (struct php_DBC *) zend_fetch_resource(rsrc TSRMLS_CC, -1, "Db4Cursor", NULL, 1, le_dbc);
        return pdbc->dbc;
    }
    return NULL;
}

struct php_DBC *getPhpDbcFromObj(zval *z TSRMLS_DC)
{
    struct php_DBC *pdbc;
    zval **rsrc;
    if(zend_hash_find(HASH_OF(z), "_dbc_ptr", sizeof("_dbc_ptr"), 
          (void **) &rsrc) == SUCCESS) 
    {
        pdbc = (struct php_DBC *) zend_fetch_resource(rsrc TSRMLS_CC, -1, "Db4Cursor", NULL, 1, le_dbc);
        return pdbc;
    }
    return NULL;
}

#define getDbcFromThis(a)        \
do { \
  zval *_self = getThis(); \
  if(!_self) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "must be called as a method"); \
    RETURN_FALSE; \
  } \
  (a) = php_db4_getDbcFromObj(_self TSRMLS_CC); \
  if(!(a)) { \
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "not a valid db4Cursor object"); \
    RETURN_FALSE; \
  } \
} while(0)

int closeDbc(zval *obj TSRMLS_DC)
{
    int ret = 0;
    struct php_DBC *pdbc = getPhpDbcFromObj(obj TSRMLS_CC);
    if(pdbc) {
        if(pdbc->parent_txn) {
            pdbc->parent_txn->open_cursors = 
                my_llist_del(pdbc->parent_txn->open_cursors, pdbc);
        }
        ret = pdbc->dbc->c_close(pdbc->dbc);
        pdbc->dbc = NULL;
        pdbc->parent_txn = NULL;
    }
    return ret;
}

/* }}} */

/* {{{ DB4Txn method definitions 
 */

/* {{{ proto bool Db4Txn::abort()
 */
ZEND_NAMED_FUNCTION(_wrap_db_txn_abort)
{
    struct php_DB_TXN *ptxn;
    zval *self;
    int ret;

    if(ZEND_NUM_ARGS()) {
        WRONG_PARAM_COUNT;
    }
    self = getThis();
    getPhpDbTxnFromThis(ptxn);
    closeDbTxnDependencies(self TSRMLS_CC);
    if((ret = ptxn->db_txn->abort(ptxn->db_txn)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        RETURN_FALSE;
    }
    ptxn->db_txn = NULL;
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool Db4Txn::commit()
 */
ZEND_NAMED_FUNCTION(_wrap_db_txn_commit)
{
    struct php_DB_TXN *ptxn;
    u_int32_t flags = 0;
    int ret;
    zval *self;

    self = getThis();
    getPhpDbTxnFromThis(ptxn);
    closeDbTxnDependencies(self TSRMLS_CC);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &flags) == FAILURE)
    {
        return;
    }
    if((ret = ptxn->db_txn->commit(ptxn->db_txn, flags)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        RETURN_FALSE;
    }
    ptxn->db_txn = NULL;
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool Db4Txn::discard()
 */
ZEND_NAMED_FUNCTION(_wrap_db_txn_discard)
{
    struct php_DB_TXN *ptxn;
    int ret;
    zval *self;

    self = getThis();
    getPhpDbTxnFromThis(ptxn);
    closeDbTxnDependencies(self TSRMLS_CC);
    if(ZEND_NUM_ARGS()) WRONG_PARAM_COUNT;
    if((ret = ptxn->db_txn->discard(ptxn->db_txn, 0)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        RETURN_FALSE;
    }
    ptxn->db_txn = NULL;
    /* FIXME should destroy $self */
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto long Db4Txn::id()
 */
ZEND_NAMED_FUNCTION(_wrap_db_txn_id)
{
    DB_TXN *txn;

    getDbTxnFromThis(txn);
    if(ZEND_NUM_ARGS()) WRONG_PARAM_COUNT;
    RETURN_LONG(txn->id(txn));
}
/* }}} */

/* {{{ proto bool Db4Txn::set_timeout(long $timeout [, long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_db_txn_set_timeout)
{
    DB_TXN *txn;
    u_int32_t flags = 0;
    long timeout;
    int ret;

    getDbTxnFromThis(txn);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l", &timeout, &flags) == FAILURE)
    {
        return;
    }
    if((ret = txn->set_timeout(txn, timeout, flags)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool Db4Txn::set_name(string $name) 
 */
ZEND_NAMED_FUNCTION(_wrap_db_txn_set_name)
{
    DB_TXN *txn;
    char *name;
    int name_len;

    getDbTxnFromThis(txn);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE)
    {
        return;
    }
    txn->set_name(txn, name);
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool Db4Txn::get_name() 
 */
ZEND_NAMED_FUNCTION(_wrap_db_txn_get_name)
{
    DB_TXN *txn;
    const char *name;
    int ret;

    getDbTxnFromThis(txn);
    if(ZEND_NUM_ARGS())
    {
        WRONG_PARAM_COUNT;
    }
    if((ret = txn->get_name(txn, &name)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        RETURN_FALSE;
    }
    RETURN_STRING((char *)name, 1);
}
/* }}} */

/* {{{ private Db4Txn::Db4Txn()
 */
ZEND_NAMED_FUNCTION(_wrap_new_DbTxn)
{
    php_error_docref(NULL TSRMLS_CC, E_ERROR, "DB4Txn objects must be created with Db4Env::begin_txn()");
}
/* }}} */

/* }}} */


/* {{{ DB4 method definitions
 */

/* {{{ proto object DB4::DB4([object $dbenv])
 */
ZEND_NAMED_FUNCTION(_wrap_new_db4)
{
    DB *db;
    DB_ENV *dbenv = NULL;
    zval *dbenv_obj = NULL;
    zval *self;
    int ret;

    self = getThis();
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|O",
                             &dbenv_obj, db_env_ce) == FAILURE) 
    {
        return;
    }
    if(dbenv_obj) {
        dbenv = php_db4_getDbEnvFromObj(dbenv_obj TSRMLS_CC);
        zval_add_ref(&dbenv_obj);
        add_property_zval(self, "dbenv", dbenv_obj);
    }
    if((ret = my_db_create(&db, dbenv, 0)) != 0) {
        php_error_docref(NULL TSRMLS_CC,
             E_WARNING, "error occurred during open");
        RETURN_FALSE;
    }
    setDb(self, db TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool DB4::open([object $txn [, string $file [, string $database [, long $flags [, long $mode]]]]]) 
 */
ZEND_NAMED_FUNCTION(_wrap_db_open)
{
    DB *db = NULL;
    DB_TXN *dbtxn = NULL;
    zval *dbtxn_obj = NULL;
    char *file = NULL, *database = NULL;
    long filelen = 0, databaselen = 0;
    DBTYPE type = DB_BTREE;
    u_int32_t flags = DB_CREATE;
    int mode = 0;
    int ret;
    
    zval *self;
    self = getThis();
    getDbFromThis(db);
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|O!sslll",
                             &dbtxn_obj, db_txn_ce,
                             &file, &filelen,
                             &database, &databaselen,
                             &type, &flags, &mode) == FAILURE) 
    {
        return;
    }
    if(dbtxn_obj) {
        dbtxn = php_db4_getDbTxnFromObj(dbtxn_obj TSRMLS_CC);
    }
    add_property_string(self, "file", file, 1);
    add_property_string(self, "database", database, 1);
    if(strcmp(file, "") == 0) file = NULL;
    if(strcmp(database, "") == 0) database = NULL;
    /* add type and other introspection data */
    if((ret = db->open(db, dbtxn, file, database, type, flags, mode)) == 0) {
        RETURN_TRUE;
    }
    else {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        add_property_string(self, "lastError", db_strerror(ret), 1);
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto bool DB4::close()
 */
ZEND_NAMED_FUNCTION(_wrap_db_close)
{
    struct php_DB *pdb = NULL;
    getPhpDbFromThis(pdb);

    if(ZEND_NUM_ARGS()) {
        WRONG_PARAM_COUNT;
    }
    if(pdb && pdb->db) {
      pdb->db->close(pdb->db, 0);
      pdb->db = NULL;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool DB4::del(string $key [, object $txn])
 */
ZEND_NAMED_FUNCTION(_wrap_db_del)
{
    DB *db = NULL;
    DB_TXN *txn = NULL;
    zval *txn_obj = NULL;
    u_int32_t flags;
    DBT key;
    char *keyname;
    int keylen;

    getDbFromThis(db);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|O", &keyname, &keylen, 
                             &txn_obj, db_txn_ce) == FAILURE) 
    {
        return;
    }
    if(txn_obj) {
        getDbTxnFromThis(txn);
        flags = 0;
    }
    memset(&key, 0, sizeof(DBT));
    key.data = keyname;
    key.size = keylen;
    RETURN_LONG(db->del(db, txn, &key, flags));
}
/* }}} */

/* {{{ proto string DB4::get(string $key [,object $txn [, long flags]])
 */
ZEND_NAMED_FUNCTION(_wrap_db_get)
{
    DB *db = NULL;
    DB_TXN *txn = NULL;
    zval *txn_obj = NULL;
    DBT key, value;
    char *keyname;
    int keylen;
    u_int32_t flags = 0;

    getDbFromThis(db);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|Ol", &keyname, &keylen, 
                             &txn_obj, db_txn_ce, &flags) == FAILURE) 
    {
        return;
    }
    if(txn_obj) {
        txn = php_db4_getDbTxnFromObj(txn_obj TSRMLS_CC);
    }
    memset(&key, 0, sizeof(DBT));
    key.data = keyname;
    key.size = keylen;
    memset(&value, 0, sizeof(DBT));
    if(db->get(db, txn, &key, &value, flags) == 0) {
        RETURN_STRINGL((char *)value.data, value.size, 1);
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto string DB4::pget(string $key, string &$pkey [,object $txn [, long flags]])
 */
ZEND_NAMED_FUNCTION(_wrap_db_pget)
{
    DB *db = NULL;
    DB_TXN *txn = NULL;
    zval *txn_obj = NULL;
    DBT key, value, pkey;
    char *keyname;
    int keylen;
    zval *z_pkey;
    u_int32_t flags = 0;

    getDbFromThis(db);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|Ol", 
                             &keyname, &keylen, &z_pkey,
                             &txn_obj, db_txn_ce, &flags) == FAILURE) 
    {
        return;
    }
    if(txn_obj) {
        txn = php_db4_getDbTxnFromObj(txn_obj TSRMLS_CC);
    }
    memset(&key, 0, sizeof(DBT));
    key.data = keyname;
    key.size = keylen;
    memset(&pkey, 0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));
    if(db->pget(db, txn, &key, &pkey, &value, flags) == 0) {
        if(Z_STRLEN_P(z_pkey) == 0) {
            Z_STRVAL_P(z_pkey) = (char *) emalloc(pkey.size);
        } else {
            Z_STRVAL_P(z_pkey) = (char *) erealloc(Z_STRVAL_P(z_pkey), pkey.size);
        }
        memcpy(Z_STRVAL_P(z_pkey), pkey.data, pkey.size);
        Z_STRLEN_P(z_pkey) = pkey.size;
        RETURN_STRINGL((char *)value.data, value.size, 1);
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto string DB4::get_type() 
 */
ZEND_NAMED_FUNCTION(_wrap_db_get_type)
{
    DB *db = NULL;
    DBTYPE type;

    getDbFromThis(db);
    if(db->get_type(db, &type)) { 
        RETURN_FALSE; 
    }
    switch(type) {
        case DB_BTREE:
            RETURN_STRING("DB_BTREE", 1);
            break;
        case DB_HASH:
            RETURN_STRING("DB_HASH", 1);
            break;
        case DB_RECNO:
            RETURN_STRING("DB_RECNO", 1);
            break;
        case DB_QUEUE:
            RETURN_STRING("DB_QUEUE", 1);
            break;
        default:
            RETURN_STRING("UNKNOWN", 1);
            break;
    }
}
/* }}} */

/* {{{ proto bool DB4::set_encrypt(string $password [, long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_db_set_encrypt)
{
    DB *db = NULL;
    char *pass;
    long passlen;
    u_int32_t flags = 0;
    getDbFromThis(db);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &pass, &passlen,
                             &flags) == FAILURE)
    {
        return;
    }
    RETURN_BOOL(db->set_encrypt(db, pass, flags)?0:1);
}
/* }}} */

/* {{{ proto int DB4::get_encrypt_flags()
 */
ZEND_NAMED_FUNCTION(_wrap_db_get_encrypt_flags)
{
    DB *db = NULL;

    getDbFromThis(db);
    u_int32_t flags = 0;
    if (db->get_encrypt_flags(db, &flags) != 0)
            RETURN_FALSE;
    RETURN_LONG(flags);
}
/* }}} */

/* {{{ proto array DB4::stat([object $txn [, long flags]])
 */
ZEND_NAMED_FUNCTION(_wrap_db_stat)
{
    DB *db = NULL;
    DB_TXN *txn = NULL;
    zval *txn_obj = NULL;
    DBTYPE type;
    u_int32_t flags = 0;

    getDbFromThis(db);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zl", &txn_obj, db_txn_ce, &flags) == FAILURE) {
        return;
    }
    if(db->get_type(db, &type)) { 
        RETURN_FALSE; 
    }
    if(txn_obj) {
        txn = php_db4_getDbTxnFromObj(txn_obj TSRMLS_CC);
    }
    switch(type) {
#define ADD_STAT_LONG(a)  add_assoc_long(return_value, #a, sb.a)
        case DB_HASH: 
        {
            DB_HASH_STAT sb;
            if(db->stat(db, txn, (void *)&sb, flags)) {
                RETURN_FALSE;
            }
            array_init(return_value);
            if(flags & DB_FAST_STAT) {
                ADD_STAT_LONG(hash_magic);
                ADD_STAT_LONG(hash_version);
                ADD_STAT_LONG(hash_nkeys);
                ADD_STAT_LONG(hash_ndata);
                ADD_STAT_LONG(hash_pagesize);
                ADD_STAT_LONG(hash_ffactor);
                ADD_STAT_LONG(hash_buckets);
            }
            ADD_STAT_LONG(hash_free);
            ADD_STAT_LONG(hash_bfree);
            ADD_STAT_LONG(hash_bigpages);
            ADD_STAT_LONG(hash_bfree);
            ADD_STAT_LONG(hash_overflows);
            ADD_STAT_LONG(hash_ovfl_free);
            ADD_STAT_LONG(hash_dup);
            ADD_STAT_LONG(hash_dup_free);
        }
            break;
        case DB_BTREE:
        case DB_RECNO:
        {
            DB_BTREE_STAT sb;
            if(db->stat(db, txn, (void *)&sb, flags)) {
                RETURN_FALSE;
            }
            array_init(return_value);
            if(flags & DB_FAST_STAT) {
                ADD_STAT_LONG(bt_magic);
                ADD_STAT_LONG(bt_version);
                ADD_STAT_LONG(bt_nkeys);
                ADD_STAT_LONG(bt_ndata);
                ADD_STAT_LONG(bt_pagesize);
                ADD_STAT_LONG(bt_minkey);
                ADD_STAT_LONG(bt_re_len);
                ADD_STAT_LONG(bt_re_pad);
            }
            ADD_STAT_LONG(bt_levels);
            ADD_STAT_LONG(bt_int_pg);
            ADD_STAT_LONG(bt_leaf_pg);
            ADD_STAT_LONG(bt_dup_pg);
            ADD_STAT_LONG(bt_over_pg);
            ADD_STAT_LONG(bt_free);
            ADD_STAT_LONG(bt_int_pgfree);
            ADD_STAT_LONG(bt_leaf_pgfree);
            ADD_STAT_LONG(bt_dup_pgfree);
            ADD_STAT_LONG(bt_over_pgfree);
        }
            break;
        case DB_QUEUE:
        {
            DB_QUEUE_STAT sb;
            if(db->stat(db, txn, (void *)&sb, flags)) {
                RETURN_FALSE;
            }
            array_init(return_value);
            if(flags & DB_FAST_STAT) {
                ADD_STAT_LONG(qs_magic);
                ADD_STAT_LONG(qs_version);
                ADD_STAT_LONG(qs_nkeys);
                ADD_STAT_LONG(qs_ndata);
                ADD_STAT_LONG(qs_pagesize);
                ADD_STAT_LONG(qs_extentsize);
                ADD_STAT_LONG(qs_re_len);
                ADD_STAT_LONG(qs_re_pad);
                ADD_STAT_LONG(qs_first_recno);
                ADD_STAT_LONG(qs_cur_recno);
            }
            ADD_STAT_LONG(qs_pages);
            ADD_STAT_LONG(qs_pgfree);
            break;
        }
        default:
            RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto DBCursor DB4::join(array $curslist [, long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_db_join) 
{ 
    DB *db = NULL;
    DBC *dbcp;
    DBC **curslist;
    zval *z_array;
    HashTable *array;
    HashPosition pos;
    zval **z_cursor;
    int num_cursors, rv, i;

    u_int32_t flags = 0;

    getDbFromThis(db);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|l", 
                             &z_array, &flags) == FAILURE) 
    {
        return;
    }
    array = HASH_OF(z_array);
    num_cursors = zend_hash_num_elements(array);
    curslist = (DBC **) calloc(sizeof(DBC *), num_cursors + 1);
    for(zend_hash_internal_pointer_reset_ex(array, &pos), i=0;
        zend_hash_get_current_data_ex(array, (void **) &z_cursor, &pos) == SUCCESS;
        zend_hash_move_forward_ex(array, &pos), i++) {
        curslist[i] = php_db4_getDbcFromObj(*z_cursor TSRMLS_CC);
    }
    rv = db->join(db, curslist, &dbcp, flags);
    free(curslist);
    if(rv) {
        RETURN_FALSE;
    } else {
        object_init_ex(return_value, dbc_ce);
        setDbc(return_value, dbcp, NULL TSRMLS_CC);
    }
}
/* }}} */

/* {{{ proto bool DB4::put(string $key, string $value [, object $txn [, long flags]])
 */
ZEND_NAMED_FUNCTION(_wrap_db_put)
{
    DB *db = NULL;
    DB_TXN *txn = NULL;
    zval *txn_obj = NULL;
    DBT key, value;
    char *keyname, *dataname;
    int keylen, datalen;
    int ret;
    zval *self;
    long flags = 0;

    self = getThis();
    getDbFromThis(db);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|Ol", &keyname, &keylen, 
                             &dataname, &datalen, &txn_obj, db_txn_ce, &flags) == FAILURE) 
    {
        return;
    }
    if(txn_obj) {
        txn = php_db4_getDbTxnFromObj(txn_obj TSRMLS_CC);
    }
    memset(&key, 0, sizeof(DBT));
    key.data = keyname;
    key.size = keylen;
    memset(&value, 0, sizeof(DBT));
    value.data = dataname;
    value.size = datalen;
    if((ret = db->put(db, txn, &key, &value, flags)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        add_property_string(self, "lastError", db_strerror(ret), 1);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool DB4::sync()
 */
ZEND_NAMED_FUNCTION(_wrap_db_sync)
{
    DB *db = NULL;
    getDbFromThis(db);
    if(ZEND_NUM_ARGS()) {
        WRONG_PARAM_COUNT;
    }
    db->sync(db, 0);
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool DB4::truncate([object $txn [, long $flags]])
 */
ZEND_NAMED_FUNCTION(_wrap_db_truncate)
{
    DB *db = NULL;
    DB_TXN *txn = NULL;
    zval *txn_obj = NULL;
    long flags = DB_AUTO_COMMIT;
    u_int32_t countp;

    getDbFromThis(db);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|Ol", 
                             &txn_obj, db_txn_ce, &flags) == FAILURE) 
    {
        return;
    }
    if(txn_obj) {
        txn = php_db4_getDbTxnFromObj(txn_obj TSRMLS_CC);
        flags = 0;
    }
    if(db->truncate(db, txn, &countp, flags) == 0) {
        RETURN_LONG(countp);
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto DB4Cursor DB4::cursor([object $txn [, long flags]])
 */
ZEND_NAMED_FUNCTION(_wrap_db_cursor)
{
    DB *db;
    DB_TXN *txn = NULL;
    zval *txn_obj = NULL, *self;
    DBC *cursor = NULL;
    u_int32_t flags = 0;
    int ret;

    self = getThis();
    getDbFromThis(db);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|Ol", &txn_obj, db_txn_ce, &flags) == FAILURE) 
    {
        return;
    }
    if(txn_obj) {
        txn = php_db4_getDbTxnFromObj(txn_obj TSRMLS_CC);
    }
    if((ret = db->cursor(db, txn, &cursor, flags)) != 0 ) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        add_property_string(self, "lastError", db_strerror(ret), 1);
        RETURN_FALSE;
    }
    else {
        object_init_ex(return_value, dbc_ce);
        setDbc(return_value, cursor, txn_obj?getPhpDbTxnFromObj(txn_obj TSRMLS_CC):NULL TSRMLS_CC);
    }
        
}
/* }}} */

/* }}} end DB4 method definitions */

/* {{{ DB4Cursor method definitions 
 */

/* {{{ proto bool Db4Cursor::close()
 */
ZEND_NAMED_FUNCTION(_wrap_dbc_close)
{
    DBC *dbc;
    int ret;
    zval *self;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) return;
    self = getThis();
    getDbcFromThis(dbc);
    if((ret = closeDbc(self TSRMLS_CC)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto long Db4Cursor::count()
 */
ZEND_NAMED_FUNCTION(_wrap_dbc_count)
{
    DBC *dbc;
    db_recno_t count;
    int ret;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) return;
    getDbcFromThis(dbc);
    if((ret = dbc->c_count(dbc, &count, 0)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        RETURN_FALSE;
    }
    RETURN_LONG(count);
}
/* }}} */

/* {{{ proto bool Db4Cursor::del()
 */
ZEND_NAMED_FUNCTION(_wrap_dbc_del)
{
    DBC *dbc;
    int ret;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) return;
    getDbcFromThis(dbc);
    if((ret = dbc->c_del(dbc, 0)) != 0) {
        if(ret != DB_KEYEMPTY) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        }
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto object Db4Cursor::dup([long $flags]) 
 */
ZEND_NAMED_FUNCTION(_wrap_dbc_dup)
{
    DBC *dbc, *newdbc;
    u_int32_t flags = 0;
    int ret;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &flags) == FAILURE) return;
    getDbcFromThis(dbc);
    if((ret = dbc->c_dup(dbc, &newdbc, flags)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        RETURN_FALSE;
    }
    object_init_ex(return_value, dbc_ce);
    /* FIXME should pass in dbc's parent txn */
    setDbc(return_value, newdbc, NULL TSRMLS_CC);
}
/* }}} */

/* {{{ proto string Db4Cursor::get(string &$key, string &$data [, long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_dbc_get)
{
    DBC *dbc;
    DBT key, value;
    zval *zkey, *zvalue;
    u_int32_t flags = DB_NEXT;
    zval *self;
    int ret;
    
    self = getThis();
    getDbcFromThis(dbc);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z/z/|l", &zkey, &zvalue, &flags) == FAILURE) 
    {
        return;
    }
    memset(&key, 0, sizeof(DBT));
    key.data = Z_STRVAL_P(zkey);
    key.size = Z_STRLEN_P(zkey);
    memset(&value, 0, sizeof(DBT));
    value.data = Z_STRVAL_P(zvalue);
    value.size = Z_STRLEN_P(zvalue);
    if((ret = dbc->c_get(dbc, &key, &value, flags)) == 0) {
        zval_dtor(zkey); ZVAL_STRINGL(zkey, (char *) key.data, key.size, 1);
        zval_dtor(zvalue); ZVAL_STRINGL(zvalue, (char *) value.data, value.size, 1);
        RETURN_LONG(0);
    } 
    if(ret != DB_NOTFOUND) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        add_property_string(self, "lastError", db_strerror(ret), 1);
    }
    RETURN_LONG(1);
}
/* }}} */

/* {{{ proto string Db4Cursor::pget(string &$key, string &$pkey, string &$data [, long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_dbc_pget)
{
    DBC *dbc;
    DBT key, pkey, value;
    zval *zkey, *zvalue, *zpkey;
    u_int32_t flags = DB_NEXT;
    zval *self;
    int ret;
    
    self = getThis();
    getDbcFromThis(dbc);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z/z/z/|l", &zkey, &zpkey, &zvalue, &flags) == FAILURE) 
    {
        return;
    }
    memset(&key, 0, sizeof(DBT));
    key.data = Z_STRVAL_P(zkey);
    key.size = Z_STRLEN_P(zkey);
    memset(&pkey, 0, sizeof(DBT));
    pkey.data = Z_STRVAL_P(zpkey);
    pkey.size = Z_STRLEN_P(zpkey);
    memset(&value, 0, sizeof(DBT));
    value.data = Z_STRVAL_P(zvalue);
    value.size = Z_STRLEN_P(zvalue);
    if((ret = dbc->c_pget(dbc, &key, &pkey, &value, flags)) == 0) {
        zval_dtor(zkey); ZVAL_STRINGL(zkey, (char *) key.data, key.size, 1);
        zval_dtor(zpkey); ZVAL_STRINGL(zpkey, (char *) pkey.data, pkey.size, 1);
        zval_dtor(zvalue); ZVAL_STRINGL(zvalue, (char *) value.data, value.size, 1);
        RETURN_LONG(0);
    } 
    if(ret != DB_NOTFOUND) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        add_property_string(self, "lastError", db_strerror(ret), 1);
    }
    RETURN_LONG(1);
}
/* }}} */

/* {{{ proto bool Db4Cursor::put(string $key, string $data [, long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_dbc_put)
{
    DBC *dbc;
    DBT key, value;
    char *keyname, *dataname;
    int keylen, datalen;
    u_int32_t flags = 0;
    int ret;

    getDbcFromThis(dbc);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &keyname, &keylen, 
          &dataname, &datalen, &flags) == FAILURE) 
    {
        return;
    }
    memset(&key, 0, sizeof(DBT));
    key.data = keyname;
    key.size = keylen;
    memset(&value, 0, sizeof(DBT));
    value.data = dataname;
    value.size = datalen;
    if((ret = dbc->c_put(dbc, &key, &value, flags)) == 0) {
        RETURN_TRUE;
    }
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
    RETURN_FALSE;
    
}
/* }}} */

/* }}} */

/* {{{ DB4Env method definitions
 */

/* {{{ php_db4_error ( zend_error wrapper )
 */

void php_db4_error(const DB_ENV *dp, const char *errpfx, const char *msg)
{
    TSRMLS_FETCH();
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s %s\n", errpfx, msg);
}
/* }}} */

/* {{{ proto object DB4Env::Db4Env([long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_new_DbEnv) 
{
    DB_ENV *dbenv;
    u_int32_t flags = 0;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &flags) == FAILURE)
    {
        return;
    }
    if(my_db_env_create(&dbenv, flags) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "bad things here: %s:%d\n", __FILE__, __LINE__);
        RETURN_FALSE;
    }
#ifndef HAVE_MOD_DB4
    DbEnv::wrap_DB_ENV(dbenv);
#endif  
    dbenv->set_errcall(dbenv, php_db4_error);
    setDbEnv(this_ptr, dbenv TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool DB4Env::close([long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_close) 
{
    struct php_DB_ENV *pdb;
    DbEnv *dbe;
    u_int32_t flags = 0;
    
    pdb = php_db4_getPhpDbEnvFromObj(getThis() TSRMLS_CC);
    if(!pdb || !pdb->dbenv) { 
      RETURN_FALSE;
    }
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &flags) == FAILURE) {
        RETURN_FALSE;
    }
    dbe = DbEnv::get_DbEnv(pdb->dbenv);
    dbe->close(flags);
    pdb->dbenv = NULL;
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool DB4Env::dbremove(object $txn, string $file [, string $database [, long flags]])
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_dbremove) 
{
    DB_ENV *dbenv;
    DB_TXN *txn;
    zval *txn_obj;
    char *filename=NULL, *database=NULL;
    int filenamelen, databaselen;
    u_int32_t flags = 0;

    getDbEnvFromThis(dbenv);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O!s|sl", &txn_obj, db_txn_ce, 
         &filename, &filenamelen, &database, &databaselen, &flags) == FAILURE) 
    {
        return;
    }
    if(txn_obj) {
        txn = php_db4_getDbTxnFromObj(txn_obj TSRMLS_CC);
        flags = 0;
    }
    if(dbenv->dbremove(dbenv, txn, filename, database, flags) == 0) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool DB4Env::dbrename(object $txn, string $file, string $database, string $newdatabase [, long flags])
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_dbrename) 
{
    DB_ENV *dbenv;
    DB_TXN *txn;
    zval *txn_obj;
    char *filename=NULL, *database=NULL, *newname=NULL;
    int filenamelen, databaselen, newnamelen;
    u_int32_t flags = 0;

    getDbEnvFromThis(dbenv);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O!sss|l", &txn_obj, db_txn_ce, 
         &filename, &filenamelen, &database, &databaselen, 
         &newname, &newnamelen, &flags) == FAILURE) 
    {
        return;
    }
    if(txn_obj) {
        txn = php_db4_getDbTxnFromObj(txn_obj TSRMLS_CC);
        flags = 0;
    }
    if(dbenv->dbrename(dbenv, txn, filename, database, newname, flags) == 0) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool DB4Env::open(string $home [, long flags [, long mode]])
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_open) 
{
    DB_ENV *dbenv;
    zval *self;
    char *home = NULL;
    long  homelen;
    u_int32_t flags = DB_CREATE  | DB_INIT_LOCK | DB_INIT_LOG | \
            DB_INIT_MPOOL | DB_INIT_TXN ;
    int mode = 0666;
    int ret;

    getDbEnvFromThis(dbenv);
    self = getThis();
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!ll", &home, &homelen, 
        &flags, &mode) == FAILURE)
    {
        return;
    }
    if((ret = dbenv->open(dbenv, home, flags, mode) != 0)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "open(%s, %d, %o) failed: %s (%d) %s:%d\n", home, flags, mode, strerror(ret), ret, __FILE__, __LINE__);
        RETURN_FALSE;
    }
    if(home) add_property_stringl(self, "home", home, homelen, 1);
}
/* }}} */

/* {{{ proto bool DB4Env::remove(string $home [, long flags])
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_remove)
{
    DB_ENV *dbenv;
    DbEnv *dbe;
    zval *self;
    char *home;
    long homelen;
    u_int32_t flags = 0;
    self = getThis();
    getDbEnvFromThis(dbenv);
    dbe = DbEnv::get_DbEnv(dbenv);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &home, &homelen, &flags) == FAILURE)
    {
        return;
    }
    RETURN_BOOL(dbe->remove(home, flags)?0:1);
}
/* }}} */

/* {{{ proto bool DB4Env::set_data_dir(string $dir)
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_set_data_dir)
{
    DB_ENV *dbenv;
    zval *self;
    char *dir;
    long dirlen;
    self = getThis();
    getDbEnvFromThis(dbenv);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &dir, &dirlen) == FAILURE)
    {
        return;
    }
    RETURN_BOOL(dbenv->set_data_dir(dbenv, dir)?0:1);
}
/* }}} */

/* {{{ proto bool DB4Env::set_encrypt(string $password [, long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_set_encrypt)
{
    DB_ENV *dbenv;
    zval *self;
    char *pass;
    long passlen;
    u_int32_t flags = 0;
    self = getThis();
    getDbEnvFromThis(dbenv);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &pass, &passlen,
                             &flags) == FAILURE)
    {
        return;
    }
    RETURN_BOOL(dbenv->set_encrypt(dbenv, pass, flags)?0:1);
}
/* }}} */

/* {{{ proto int DB4Env::get_encrypt_flags()
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_get_encrypt_flags)
{
    DB_ENV *dbenv;
    zval *self;
    u_int32_t flags = 0;
    self = getThis();
    getDbEnvFromThis(dbenv);
    if (dbenv->get_encrypt_flags(dbenv, &flags) != 0)
            RETURN_FALSE;
    RETURN_LONG(flags);
}
/* }}} */

/* {{{ proto object Db4Env::txn_begin([object $parent_txn [, long $flags]])
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_txn_begin) 
{
    DB_ENV *dbenv;
    DB_TXN *txn, *parenttxn = NULL;
    zval *self;
    zval *cursor_array;
    zval *parenttxn_obj = NULL;
    u_int32_t flags = 0;
    int ret;

    self = getThis();
    getDbEnvFromThis(dbenv);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|Ol", &parenttxn_obj, db_txn_ce, 
         &flags) == FAILURE) 
    {
        return;
    }
    if(parenttxn_obj) {
        parenttxn = php_db4_getDbTxnFromObj(parenttxn_obj TSRMLS_CC);
    }
    if((ret = dbenv->txn_begin(dbenv, parenttxn, &txn, flags)) != 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", db_strerror(ret));
        add_property_string(self, "lastError", db_strerror(ret), 1);
        RETURN_FALSE;
    }
    object_init_ex(return_value, db_txn_ce);
    MAKE_STD_ZVAL(cursor_array);
    array_init(cursor_array);
    add_property_zval(return_value, "openCursors", cursor_array);
    setDbTxn(return_value, txn TSRMLS_CC);
}
/* }}} */

/* {{{ Db4Env::txn_checkpoint(long $kbytes, long $minutes [, long $flags])
 */
ZEND_NAMED_FUNCTION(_wrap_db_env_txn_checkpoint)
{
    DB_ENV *dbenv;
    zval *self;
    u_int32_t kbytes = 0;
    u_int32_t mins = 0;
    u_int32_t flags = 0;
    int ret;

    self = getThis();
    getDbEnvFromThis(dbenv);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll|l", &kbytes, &mins, &flags) == FAILURE) 
    {
        return;
    }
    if((ret = dbenv->txn_checkpoint(dbenv, kbytes, mins, flags)) != 0) {
        add_property_string(self, "lastError", db_strerror(ret), 1);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* }}} end db4env */

/*
 * Local variables:
 * tab-width: 4
  c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
