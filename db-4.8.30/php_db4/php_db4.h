/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: George Schlossnagle <george@omniti.com>
 */

#ifndef PHP_DB4_H
#define PHP_DB4_H

extern zend_module_entry db4_module_entry;
#define phpext_db4_ptr &db4_module_entry

#ifdef DB4_EXPORTS
#define PHP_DB4_API __declspec(dllexport)
#else
#define PHP_DB4_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include "db.h"

PHP_DB4_API zend_class_entry *db_txn_ce_get(void);
PHP_DB4_API zend_class_entry *dbc_ce_get(void);
PHP_DB4_API zend_class_entry *db_env_ce_get(void);
PHP_DB4_API zend_class_entry *db_ce_get(void);
PHP_DB4_API DB_ENV *php_db4_getDbEnvFromObj(zval *z TSRMLS_DC);
PHP_DB4_API DB *php_db4_getDbFromObj(zval *z TSRMLS_DC);
PHP_DB4_API DB_TXN *php_db4_getDbTxnFromObj(zval *z TSRMLS_DC);
PHP_DB4_API DBC *php_db4_getDbcFromObj(zval *z TSRMLS_DC);
/* 
    Declare any global variables you may need between the BEGIN
    and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(db4)
    long  global_value;
    char *global_string;
ZEND_END_MODULE_GLOBALS(db4)
*/

/* In every utility function you add that needs to use variables 
   in php_db4_globals, call TSRM_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as DB4_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define DB4_G(v) TSRMG(db4_globals_id, zend_db4_globals *, v)
#else
#define DB4_G(v) (db4_globals.v)
#endif

#endif  /* PHP_DB4_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
