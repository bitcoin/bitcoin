/* 

 DB_File.xs -- Perl 5 interface to Berkeley DB 

 written by Paul Marquess <pmqs@cpan.org>
 last modified 4th February 2007
 version 1.818

 All comments/suggestions/problems are welcome

     Copyright (c) 1995-2009 Paul Marquess. All rights reserved.
     This program is free software; you can redistribute it and/or
     modify it under the same terms as Perl itself.

 Changes:
	0.1 - 	Initial Release
	0.2 - 	No longer bombs out if dbopen returns an error.
	0.3 - 	Added some support for multiple btree compares
	1.0 - 	Complete support for multiple callbacks added.
	      	Fixed a problem with pushing a value onto an empty list.
	1.01 - 	Fixed a SunOS core dump problem.
		The return value from TIEHASH wasn't set to NULL when
		dbopen returned an error.
	1.02 - 	Use ALIAS to define TIEARRAY.
		Removed some redundant commented code.
		Merged OS2 code into the main distribution.
		Allow negative subscripts with RECNO interface.
		Changed the default flags to O_CREAT|O_RDWR
	1.03 - 	Added EXISTS
	1.04 -  fixed a couple of bugs in hash_cb. Patches supplied by
		Dave Hammen, hammen@gothamcity.jsc.nasa.gov
	1.05 -  Added logic to allow prefix & hash types to be specified via
		Makefile.PL
	1.06 -  Minor namespace cleanup: Localized PrintBtree.
	1.07 -  Fixed bug with RECNO, where bval wasn't defaulting to "\n". 
	1.08 -  No change to DB_File.xs
	1.09 -  Default mode for dbopen changed to 0666
	1.10 -  Fixed fd method so that it still returns -1 for
		in-memory files when db 1.86 is used.
	1.11 -  No change to DB_File.xs
	1.12 -  No change to DB_File.xs
	1.13 -  Tidied up a few casts.     
	1.14 -	Made it illegal to tie an associative array to a RECNO
		database and an ordinary array to a HASH or BTREE database.
	1.50 -  Make work with both DB 1.x or DB 2.x
	1.51 -  Fixed a bug in mapping 1.x O_RDONLY flag to 2.x DB_RDONLY equivalent
	1.52 -  Patch from Gisle Aas <gisle@aas.no> to suppress "use of 
		undefined value" warning with db_get and db_seq.
	1.53 -  Added DB_RENUMBER to flags for recno.
	1.54 -  Fixed bug in the fd method
        1.55 -  Fix for AIX from Jarkko Hietaniemi
        1.56 -  No change to DB_File.xs
        1.57 -  added the #undef op to allow building with Threads support.
	1.58 -  Fixed a problem with the use of sv_setpvn. When the
		size is specified as 0, it does a strlen on the data.
		This was ok for DB 1.x, but isn't for DB 2.x.
        1.59 -  No change to DB_File.xs
        1.60 -  Some code tidy up
        1.61 -  added flagSet macro for DB 2.5.x
		fixed typo in O_RDONLY test.
        1.62 -  No change to DB_File.xs
        1.63 -  Fix to alllow DB 2.6.x to build.
        1.64 -  Tidied up the 1.x to 2.x flags mapping code.
		Added a patch from Mark Kettenis <kettenis@wins.uva.nl>
		to fix a flag mapping problem with O_RDONLY on the Hurd
        1.65 -  Fixed a bug in the PUSH logic.
		Added BOOT check that using 2.3.4 or greater
        1.66 -  Added DBM filter code
        1.67 -  Backed off the use of newSVpvn.
		Fixed DBM Filter code for Perl 5.004.
		Fixed a small memory leak in the filter code.
        1.68 -  fixed backward compatability bug with R_IAFTER & R_IBEFORE
		merged in the 5.005_58 changes
        1.69 -  fixed a bug in push -- DB_APPEND wasn't working properly.
		Fixed the R_SETCURSOR bug introduced in 1.68
		Added a new Perl variable $DB_File::db_ver 
        1.70 -  Initialise $DB_File::db_ver and $DB_File::db_version with 
		GV_ADD|GV_ADDMULT -- bug spotted by Nick Ing-Simmons.
		Added a BOOT check to test for equivalent versions of db.h &
		libdb.a/so.
        1.71 -  Support for Berkeley DB version 3.
		Support for Berkeley DB 2/3's backward compatability mode.
		Rewrote push
        1.72 -  No change to DB_File.xs
        1.73 -  No change to DB_File.xs
        1.74 -  A call to open needed parenthesised to stop it clashing
                with a win32 macro.
		Added Perl core patches 7703 & 7801.
        1.75 -  Fixed Perl core patch 7703.
		Added suppport to allow DB_File to be built with 
		Berkeley DB 3.2 -- btree_compare, btree_prefix and hash_cb
		needed to be changed.
        1.76 -  No change to DB_File.xs
        1.77 -  Tidied up a few types used in calling newSVpvn.
        1.78 -  Core patch 10335, 10372, 10534, 10549, 11051 included.
        1.79 -  NEXTKEY ignores the input key.
                Added lots of casts
        1.800 - Moved backward compatability code into ppport.h.
                Use the new constants code.
        1.801 - No change to DB_File.xs
        1.802 - No change to DB_File.xs
        1.803 - FETCH, STORE & DELETE don't map the flags parameter
                into the equivalent Berkeley DB function anymore.
        1.804 - no change.
        1.805 - recursion detection added to the callbacks
                Support for 4.1.X added.
                Filter code can now cope with read-only $_
        1.806 - recursion detection beefed up.
        1.807 - no change
        1.808 - leak fixed in ParseOpenInfo
        1.809 - no change
        1.810 - no change
        1.811 - no change
        1.812 - no change
        1.813 - no change
        1.814 - no change
        1.814 - C++ casting fixes

*/

#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"  
#include "perl.h"
#include "XSUB.h"

#ifdef _NOT_CORE
#  include "ppport.h"
#endif

/* Mention DB_VERSION_MAJOR_CFG, DB_VERSION_MINOR_CFG, and
   DB_VERSION_PATCH_CFG here so that Configure pulls them all in. */

/* Being the Berkeley DB we prefer the <sys/cdefs.h> (which will be
 * shortly #included by the <db.h>) __attribute__ to the possibly
 * already defined __attribute__, for example by GNUC or by Perl. */

/* #if DB_VERSION_MAJOR_CFG < 2  */
#ifndef DB_VERSION_MAJOR
#    undef __attribute__
#endif

#ifdef COMPAT185
#    include <db_185.h>
#else
#    include <db.h>
#endif

/* Wall starts with 5.7.x */

#if PERL_REVISION > 5 || (PERL_REVISION == 5 && PERL_VERSION >= 7)

/* Since we dropped the gccish definition of __attribute__ we will want
 * to redefine dNOOP, however (so that dTHX continues to work).  Yes,
 * all this means that we can't do attribute checking on the DB_File,
 * boo, hiss. */
#  ifndef DB_VERSION_MAJOR

#    undef  dNOOP
#    define dNOOP extern int Perl___notused

    /* Ditto for dXSARGS. */
#    undef  dXSARGS
#    define dXSARGS				\
	dSP; dMARK;			\
	I32 ax = mark - PL_stack_base + 1;	\
	I32 items = sp - mark

#  endif

/* avoid -Wall; DB_File xsubs never make use of `ix' setup for ALIASes */
#  undef dXSI32
#  define dXSI32 dNOOP

#endif /* Perl >= 5.7 */

#include <fcntl.h> 

/* #define TRACE */

#ifdef TRACE
#    define Trace(x)        printf x
#else
#    define Trace(x)
#endif


#define DBT_clear(x)	Zero(&x, 1, DBT) ;

#ifdef DB_VERSION_MAJOR

#if DB_VERSION_MAJOR == 2
#    define BERKELEY_DB_1_OR_2
#endif

#if DB_VERSION_MAJOR > 3 || (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2)
#    define AT_LEAST_DB_3_2
#endif

#if DB_VERSION_MAJOR > 3 || (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3)
#    define AT_LEAST_DB_3_3
#endif

#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1)
#    define AT_LEAST_DB_4_1
#endif

#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 3)
#    define AT_LEAST_DB_4_3
#endif

#ifdef AT_LEAST_DB_3_3
#   define WANT_ERROR
#endif

/* map version 2 features & constants onto their version 1 equivalent */

#ifdef DB_Prefix_t
#    undef DB_Prefix_t
#endif
#define DB_Prefix_t	size_t

#ifdef DB_Hash_t
#    undef DB_Hash_t
#endif
#define DB_Hash_t	u_int32_t

/* DBTYPE stays the same */
/* HASHINFO, RECNOINFO and BTREEINFO  map to DB_INFO */
#if DB_VERSION_MAJOR == 2
    typedef DB_INFO	INFO ;
#else /* DB_VERSION_MAJOR > 2 */
#    define DB_FIXEDLEN	(0x8000)
#endif /* DB_VERSION_MAJOR == 2 */

/* version 2 has db_recno_t in place of recno_t	*/
typedef db_recno_t	recno_t;


#define R_CURSOR        DB_SET_RANGE
#define R_FIRST         DB_FIRST
#define R_IAFTER        DB_AFTER
#define R_IBEFORE       DB_BEFORE
#define R_LAST          DB_LAST
#define R_NEXT          DB_NEXT
#define R_NOOVERWRITE   DB_NOOVERWRITE
#define R_PREV          DB_PREV

#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 5
#  define R_SETCURSOR	0x800000
#else
#  define R_SETCURSOR	(-100)
#endif

#define R_RECNOSYNC     0
#define R_FIXEDLEN	DB_FIXEDLEN
#define R_DUP		DB_DUP


#define db_HA_hash 	h_hash
#define db_HA_ffactor	h_ffactor
#define db_HA_nelem	h_nelem
#define db_HA_bsize	db_pagesize
#define db_HA_cachesize	db_cachesize
#define db_HA_lorder	db_lorder

#define db_BT_compare	bt_compare
#define db_BT_prefix	bt_prefix
#define db_BT_flags	flags
#define db_BT_psize	db_pagesize
#define db_BT_cachesize	db_cachesize
#define db_BT_lorder	db_lorder
#define db_BT_maxkeypage
#define db_BT_minkeypage


#define db_RE_reclen	re_len
#define db_RE_flags	flags
#define db_RE_bval	re_pad
#define db_RE_bfname	re_source
#define db_RE_psize	db_pagesize
#define db_RE_cachesize	db_cachesize
#define db_RE_lorder	db_lorder

#define TXN	NULL,

#define do_SEQ(db, key, value, flag)	(db->cursor->c_get)(db->cursor, &key, &value, flag)


#define DBT_flags(x)	x.flags = 0
#define DB_flags(x, v)	x |= v 

#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 5
#    define flagSet(flags, bitmask)	((flags) & (bitmask))
#else
#    define flagSet(flags, bitmask)	(((flags) & DB_OPFLAGS_MASK) == (bitmask))
#endif

#else /* db version 1.x */

#define BERKELEY_DB_1
#define BERKELEY_DB_1_OR_2

typedef union INFO {
        HASHINFO 	hash ;
        RECNOINFO 	recno ;
        BTREEINFO 	btree ;
      } INFO ;


#ifdef mDB_Prefix_t 
#  ifdef DB_Prefix_t
#    undef DB_Prefix_t
#  endif
#  define DB_Prefix_t	mDB_Prefix_t 
#endif

#ifdef mDB_Hash_t
#  ifdef DB_Hash_t
#    undef DB_Hash_t
#  endif
#  define DB_Hash_t	mDB_Hash_t
#endif

#define db_HA_hash 	hash.hash
#define db_HA_ffactor	hash.ffactor
#define db_HA_nelem	hash.nelem
#define db_HA_bsize	hash.bsize
#define db_HA_cachesize	hash.cachesize
#define db_HA_lorder	hash.lorder

#define db_BT_compare	btree.compare
#define db_BT_prefix	btree.prefix
#define db_BT_flags	btree.flags
#define db_BT_psize	btree.psize
#define db_BT_cachesize	btree.cachesize
#define db_BT_lorder	btree.lorder
#define db_BT_maxkeypage btree.maxkeypage
#define db_BT_minkeypage btree.minkeypage

#define db_RE_reclen	recno.reclen
#define db_RE_flags	recno.flags
#define db_RE_bval	recno.bval
#define db_RE_bfname	recno.bfname
#define db_RE_psize	recno.psize
#define db_RE_cachesize	recno.cachesize
#define db_RE_lorder	recno.lorder

#define TXN	

#define do_SEQ(db, key, value, flag)	(db->dbp->seq)(db->dbp, &key, &value, flag)
#define DBT_flags(x)	
#define DB_flags(x, v)	
#define flagSet(flags, bitmask)        ((flags) & (bitmask))

#endif /* db version 1 */



#define db_DELETE(db, key, flags)       ((db->dbp)->del)(db->dbp, TXN &key, 0)
#define db_STORE(db, key, value, flags) ((db->dbp)->put)(db->dbp, TXN &key, &value, 0)
#define db_FETCH(db, key, flags)        ((db->dbp)->get)(db->dbp, TXN &key, &value, 0)

#define db_sync(db, flags)              ((db->dbp)->sync)(db->dbp, flags)
#define db_get(db, key, value, flags)   ((db->dbp)->get)(db->dbp, TXN &key, &value, flags)

#ifdef DB_VERSION_MAJOR
#define db_DESTROY(db)                  (!db->aborted && ( db->cursor->c_close(db->cursor),\
					  (db->dbp->close)(db->dbp, 0) ))
#define db_close(db)			((db->dbp)->close)(db->dbp, 0)
#define db_del(db, key, flags)          (flagSet(flags, R_CURSOR) 					\
						? ((db->cursor)->c_del)(db->cursor, 0)		\
						: ((db->dbp)->del)(db->dbp, NULL, &key, flags) )

#else /* ! DB_VERSION_MAJOR */

#define db_DESTROY(db)                  (!db->aborted && ((db->dbp)->close)(db->dbp))
#define db_close(db)			((db->dbp)->close)(db->dbp)
#define db_del(db, key, flags)          ((db->dbp)->del)(db->dbp, &key, flags)
#define db_put(db, key, value, flags)   ((db->dbp)->put)(db->dbp, &key, &value, flags)

#endif /* ! DB_VERSION_MAJOR */


#define db_seq(db, key, value, flags)   do_SEQ(db, key, value, flags)

typedef struct {
	DBTYPE	type ;
	DB * 	dbp ;
	SV *	compare ;
	bool	in_compare ;
	SV *	prefix ;
	bool	in_prefix ;
	SV *	hash ;
	bool	in_hash ;
	bool	aborted ;
	int	in_memory ;
#ifdef BERKELEY_DB_1_OR_2
	INFO 	info ;
#endif	
#ifdef DB_VERSION_MAJOR
	DBC *	cursor ;
#endif
	SV *    filter_fetch_key ;
	SV *    filter_store_key ;
	SV *    filter_fetch_value ;
	SV *    filter_store_value ;
	int     filtering ;

	} DB_File_type;

typedef DB_File_type * DB_File ;
typedef DBT DBTKEY ;

#define my_sv_setpvn(sv, d, s) sv_setpvn(sv, (s ? d : (const char *)""), s)

#define OutputValue(arg, name)  					\
	{ if (RETVAL == 0) {						\
	      SvGETMAGIC(arg) ;          				\
	      my_sv_setpvn(arg, (const char *)name.data, name.size) ;			\
	      TAINT;                                       		\
	      SvTAINTED_on(arg);                                       	\
	      SvUTF8_off(arg);                                       	\
	      DBM_ckFilter(arg, filter_fetch_value,"filter_fetch_value") ; 	\
	  }								\
	}

#define OutputKey(arg, name)	 					\
	{ if (RETVAL == 0) 						\
	  { 								\
		SvGETMAGIC(arg) ;          				\
		if (db->type != DB_RECNO) {				\
		    my_sv_setpvn(arg, (const char *)name.data, name.size); 		\
		}							\
		else 							\
		    sv_setiv(arg, (I32)*(I32*)name.data - 1); 		\
	      TAINT;                                       		\
	      SvTAINTED_on(arg);                                       	\
	      SvUTF8_off(arg);                                       	\
	      DBM_ckFilter(arg, filter_fetch_key,"filter_fetch_key") ; 	\
	  } 								\
	}

#define my_SvUV32(sv) ((u_int32_t)SvUV(sv))

#ifdef CAN_PROTOTYPE
extern void __getBerkeleyDBInfo(void);
#endif

/* Internal Global Data */

#define MY_CXT_KEY "DB_File::_guts" XS_VERSION

typedef struct {
    recno_t	x_Value; 
    recno_t	x_zero;
    DB_File	x_CurrentDB;
    DBTKEY	x_empty;
} my_cxt_t;

START_MY_CXT

#define Value		(MY_CXT.x_Value)
#define zero		(MY_CXT.x_zero)
#define CurrentDB	(MY_CXT.x_CurrentDB)
#define empty		(MY_CXT.x_empty)

#define ERR_BUFF "DB_File::Error"

#ifdef DB_VERSION_MAJOR

static int
#ifdef CAN_PROTOTYPE
db_put(DB_File db, DBTKEY key, DBT value, u_int flags)
#else
db_put(db, key, value, flags)
DB_File		db ;
DBTKEY		key ;
DBT		value ;
u_int		flags ;
#endif
{
    int status ;

    if (flagSet(flags, R_IAFTER) || flagSet(flags, R_IBEFORE)) {
        DBC * temp_cursor ;
	DBT l_key, l_value;
        
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
        if (((db->dbp)->cursor)(db->dbp, NULL, &temp_cursor) != 0)
#else
        if (((db->dbp)->cursor)(db->dbp, NULL, &temp_cursor, 0) != 0)
#endif
	    return (-1) ;

	memset(&l_key, 0, sizeof(l_key));
	l_key.data = key.data;
	l_key.size = key.size;
	memset(&l_value, 0, sizeof(l_value));
	l_value.data = value.data;
	l_value.size = value.size;

	if ( temp_cursor->c_get(temp_cursor, &l_key, &l_value, DB_SET) != 0) {
	    (void)temp_cursor->c_close(temp_cursor);
	    return (-1);
	}

	status = temp_cursor->c_put(temp_cursor, &key, &value, flags);
	(void)temp_cursor->c_close(temp_cursor);
	    
        return (status) ;
    }	
    
    
    if (flagSet(flags, R_CURSOR)) {
	return ((db->cursor)->c_put)(db->cursor, &key, &value, DB_CURRENT);
    }

    if (flagSet(flags, R_SETCURSOR)) {
	if ((db->dbp)->put(db->dbp, NULL, &key, &value, 0) != 0)
		return -1 ;
        return ((db->cursor)->c_get)(db->cursor, &key, &value, DB_SET_RANGE);
    
    }

    return ((db->dbp)->put)(db->dbp, NULL, &key, &value, flags) ;

}

#endif /* DB_VERSION_MAJOR */

static void
tidyUp(DB_File db)
{
    db->aborted = TRUE ;
}


static int
#ifdef AT_LEAST_DB_3_2

#ifdef CAN_PROTOTYPE
btree_compare(DB * db, const DBT *key1, const DBT *key2)
#else
btree_compare(db, key1, key2)
DB * db ;
const DBT * key1 ;
const DBT * key2 ;
#endif /* CAN_PROTOTYPE */

#else /* Berkeley DB < 3.2 */

#ifdef CAN_PROTOTYPE
btree_compare(const DBT *key1, const DBT *key2)
#else
btree_compare(key1, key2)
const DBT * key1 ;
const DBT * key2 ;
#endif

#endif

{
#ifdef dTHX
    dTHX;
#endif    
    dSP ;
    dMY_CXT ;
    void * data1, * data2 ;
    int retval ;
    int count ;
    

    if (CurrentDB->in_compare) {
        tidyUp(CurrentDB);
        croak ("DB_File btree_compare: recursion detected\n") ;
    }

    data1 = (char *) key1->data ;
    data2 = (char *) key2->data ;

#ifndef newSVpvn
    /* As newSVpv will assume that the data pointer is a null terminated C 
       string if the size parameter is 0, make sure that data points to an 
       empty string if the length is 0
    */
    if (key1->size == 0)
        data1 = "" ; 
    if (key2->size == 0)
        data2 = "" ;
#endif	

    ENTER ;
    SAVETMPS;
    SAVESPTR(CurrentDB);
    CurrentDB->in_compare = FALSE;
    SAVEINT(CurrentDB->in_compare);
    CurrentDB->in_compare = TRUE;

    PUSHMARK(SP) ;
    EXTEND(SP,2) ;
    PUSHs(sv_2mortal(newSVpvn((const char*)data1,key1->size)));
    PUSHs(sv_2mortal(newSVpvn((const char*)data2,key2->size)));
    PUTBACK ;

    count = perl_call_sv(CurrentDB->compare, G_SCALAR); 

    SPAGAIN ;

    if (count != 1){
        tidyUp(CurrentDB);
        croak ("DB_File btree_compare: expected 1 return value from compare sub, got %d\n", count) ;
    }

    retval = POPi ;

    PUTBACK ;
    FREETMPS ;
    LEAVE ;

    return (retval) ;

}

static DB_Prefix_t
#ifdef AT_LEAST_DB_3_2

#ifdef CAN_PROTOTYPE
btree_prefix(DB * db, const DBT *key1, const DBT *key2)
#else
btree_prefix(db, key1, key2)
Db * db ;
const DBT * key1 ;
const DBT * key2 ;
#endif

#else /* Berkeley DB < 3.2 */

#ifdef CAN_PROTOTYPE
btree_prefix(const DBT *key1, const DBT *key2)
#else
btree_prefix(key1, key2)
const DBT * key1 ;
const DBT * key2 ;
#endif

#endif
{
#ifdef dTHX
    dTHX;
#endif    
    dSP ;
    dMY_CXT ;
    char * data1, * data2 ;
    int retval ;
    int count ;
    
    if (CurrentDB->in_prefix){
        tidyUp(CurrentDB);
        croak ("DB_File btree_prefix: recursion detected\n") ;
    }

    data1 = (char *) key1->data ;
    data2 = (char *) key2->data ;

#ifndef newSVpvn
    /* As newSVpv will assume that the data pointer is a null terminated C 
       string if the size parameter is 0, make sure that data points to an 
       empty string if the length is 0
    */
    if (key1->size == 0)
        data1 = "" ;
    if (key2->size == 0)
        data2 = "" ;
#endif	

    ENTER ;
    SAVETMPS;
    SAVESPTR(CurrentDB);
    CurrentDB->in_prefix = FALSE;
    SAVEINT(CurrentDB->in_prefix);
    CurrentDB->in_prefix = TRUE;

    PUSHMARK(SP) ;
    EXTEND(SP,2) ;
    PUSHs(sv_2mortal(newSVpvn(data1,key1->size)));
    PUSHs(sv_2mortal(newSVpvn(data2,key2->size)));
    PUTBACK ;

    count = perl_call_sv(CurrentDB->prefix, G_SCALAR); 

    SPAGAIN ;

    if (count != 1){
        tidyUp(CurrentDB);
        croak ("DB_File btree_prefix: expected 1 return value from prefix sub, got %d\n", count) ;
    }
 
    retval = POPi ;
 
    PUTBACK ;
    FREETMPS ;
    LEAVE ;

    return (retval) ;
}


#ifdef BERKELEY_DB_1
#    define HASH_CB_SIZE_TYPE size_t
#else
#    define HASH_CB_SIZE_TYPE u_int32_t
#endif

static DB_Hash_t
#ifdef AT_LEAST_DB_3_2

#ifdef CAN_PROTOTYPE
hash_cb(DB * db, const void *data, u_int32_t size)
#else
hash_cb(db, data, size)
DB * db ;
const void * data ;
HASH_CB_SIZE_TYPE size ;
#endif

#else /* Berkeley DB < 3.2 */

#ifdef CAN_PROTOTYPE
hash_cb(const void *data, HASH_CB_SIZE_TYPE size)
#else
hash_cb(data, size)
const void * data ;
HASH_CB_SIZE_TYPE size ;
#endif

#endif
{
#ifdef dTHX
    dTHX;
#endif    
    dSP ;
    dMY_CXT;
    int retval = 0;
    int count ;

    if (CurrentDB->in_hash){
        tidyUp(CurrentDB);
        croak ("DB_File hash callback: recursion detected\n") ;
    }

#ifndef newSVpvn
    if (size == 0)
        data = "" ;
#endif	

     /* DGH - Next two lines added to fix corrupted stack problem */
    ENTER ;
    SAVETMPS;
    SAVESPTR(CurrentDB);
    CurrentDB->in_hash = FALSE;
    SAVEINT(CurrentDB->in_hash);
    CurrentDB->in_hash = TRUE;

    PUSHMARK(SP) ;


    XPUSHs(sv_2mortal(newSVpvn((char*)data,size)));
    PUTBACK ;

    count = perl_call_sv(CurrentDB->hash, G_SCALAR); 

    SPAGAIN ;

    if (count != 1){
        tidyUp(CurrentDB);
        croak ("DB_File hash_cb: expected 1 return value from hash sub, got %d\n", count) ;
    }

    retval = POPi ;

    PUTBACK ;
    FREETMPS ;
    LEAVE ;

    return (retval) ;
}

#ifdef WANT_ERROR

static void
#ifdef AT_LEAST_DB_4_3
db_errcall_cb(const DB_ENV* dbenv, const char * db_errpfx, const char * buffer)
#else
db_errcall_cb(const char * db_errpfx, char * buffer)
#endif
{
#ifdef dTHX
    dTHX;
#endif    
    SV * sv = perl_get_sv(ERR_BUFF, FALSE) ;
    if (sv) {
        if (db_errpfx)
            sv_setpvf(sv, "%s: %s", db_errpfx, buffer) ;
        else
            sv_setpv(sv, buffer) ;
    }
} 
#endif

#if defined(TRACE) && defined(BERKELEY_DB_1_OR_2)

static void
#ifdef CAN_PROTOTYPE
PrintHash(INFO *hash)
#else
PrintHash(hash)
INFO * hash ;
#endif
{
    printf ("HASH Info\n") ;
    printf ("  hash      = %s\n", 
		(hash->db_HA_hash != NULL ? "redefined" : "default")) ;
    printf ("  bsize     = %d\n", hash->db_HA_bsize) ;
    printf ("  ffactor   = %d\n", hash->db_HA_ffactor) ;
    printf ("  nelem     = %d\n", hash->db_HA_nelem) ;
    printf ("  cachesize = %d\n", hash->db_HA_cachesize) ;
    printf ("  lorder    = %d\n", hash->db_HA_lorder) ;

}

static void
#ifdef CAN_PROTOTYPE
PrintRecno(INFO *recno)
#else
PrintRecno(recno)
INFO * recno ;
#endif
{
    printf ("RECNO Info\n") ;
    printf ("  flags     = %d\n", recno->db_RE_flags) ;
    printf ("  cachesize = %d\n", recno->db_RE_cachesize) ;
    printf ("  psize     = %d\n", recno->db_RE_psize) ;
    printf ("  lorder    = %d\n", recno->db_RE_lorder) ;
    printf ("  reclen    = %lu\n", (unsigned long)recno->db_RE_reclen) ;
    printf ("  bval      = %d 0x%x\n", recno->db_RE_bval, recno->db_RE_bval) ;
    printf ("  bfname    = %d [%s]\n", recno->db_RE_bfname, recno->db_RE_bfname) ;
}

static void
#ifdef CAN_PROTOTYPE
PrintBtree(INFO *btree)
#else
PrintBtree(btree)
INFO * btree ;
#endif
{
    printf ("BTREE Info\n") ;
    printf ("  compare    = %s\n", 
		(btree->db_BT_compare ? "redefined" : "default")) ;
    printf ("  prefix     = %s\n", 
		(btree->db_BT_prefix ? "redefined" : "default")) ;
    printf ("  flags      = %d\n", btree->db_BT_flags) ;
    printf ("  cachesize  = %d\n", btree->db_BT_cachesize) ;
    printf ("  psize      = %d\n", btree->db_BT_psize) ;
#ifndef DB_VERSION_MAJOR
    printf ("  maxkeypage = %d\n", btree->db_BT_maxkeypage) ;
    printf ("  minkeypage = %d\n", btree->db_BT_minkeypage) ;
#endif
    printf ("  lorder     = %d\n", btree->db_BT_lorder) ;
}

#else

#define PrintRecno(recno)
#define PrintHash(hash)
#define PrintBtree(btree)

#endif /* TRACE */


static I32
#ifdef CAN_PROTOTYPE
GetArrayLength(pTHX_ DB_File db)
#else
GetArrayLength(db)
DB_File db ;
#endif
{
    DBT		key ;
    DBT		value ;
    int		RETVAL ;

    DBT_clear(key) ;
    DBT_clear(value) ;
    RETVAL = do_SEQ(db, key, value, R_LAST) ;
    if (RETVAL == 0)
        RETVAL = *(I32 *)key.data ;
    else /* No key means empty file */
        RETVAL = 0 ;

    return ((I32)RETVAL) ;
}

static recno_t
#ifdef CAN_PROTOTYPE
GetRecnoKey(pTHX_ DB_File db, I32 value)
#else
GetRecnoKey(db, value)
DB_File  db ;
I32      value ;
#endif
{
    if (value < 0) {
	/* Get the length of the array */
	I32 length = GetArrayLength(aTHX_ db) ;

	/* check for attempt to write before start of array */
	if (length + value + 1 <= 0) {
            tidyUp(db);
	    croak("Modification of non-creatable array value attempted, subscript %ld", (long)value) ;
	}

	value = length + value + 1 ;
    }
    else
        ++ value ;

    return value ;
}


static DB_File
#ifdef CAN_PROTOTYPE
ParseOpenInfo(pTHX_ int isHASH, char *name, int flags, int mode, SV *sv)
#else
ParseOpenInfo(isHASH, name, flags, mode, sv)
int    isHASH ;
char * name ;
int    flags ;
int    mode ;
SV *   sv ;
#endif
{

#ifdef BERKELEY_DB_1_OR_2 /* Berkeley DB Version 1  or 2 */

    SV **	svp;
    HV *	action ;
    DB_File	RETVAL = (DB_File)safemalloc(sizeof(DB_File_type)) ;
    void *	openinfo = NULL ;
    INFO	* info  = &RETVAL->info ;
    STRLEN	n_a;
    dMY_CXT;

#ifdef TRACE    
    printf("In ParseOpenInfo name=[%s] flags=[%d] mode=[%d] SV NULL=[%d]\n", 
		    name, flags, mode, sv == NULL) ;  
#endif
    Zero(RETVAL, 1, DB_File_type) ;

    /* Default to HASH */
    RETVAL->filtering = 0 ;
    RETVAL->filter_fetch_key = RETVAL->filter_store_key = 
    RETVAL->filter_fetch_value = RETVAL->filter_store_value =
    RETVAL->hash = RETVAL->compare = RETVAL->prefix = NULL ;
    RETVAL->type = DB_HASH ;

     /* DGH - Next line added to avoid SEGV on existing hash DB */
    CurrentDB = RETVAL; 

    /* fd for 1.86 hash in memory files doesn't return -1 like 1.85 */
    RETVAL->in_memory = (name == NULL) ;

    if (sv)
    {
        if (! SvROK(sv) )
            croak ("type parameter is not a reference") ;

        svp  = hv_fetch( (HV*)SvRV(sv), "GOT", 3, FALSE) ;
        if (svp && SvOK(*svp))
            action  = (HV*) SvRV(*svp) ;
	else
	    croak("internal error") ;

        if (sv_isa(sv, "DB_File::HASHINFO"))
        {

	    if (!isHASH)
	        croak("DB_File can only tie an associative array to a DB_HASH database") ;

            RETVAL->type = DB_HASH ;
            openinfo = (void*)info ;
  
            svp = hv_fetch(action, "hash", 4, FALSE); 

            if (svp && SvOK(*svp))
            {
                info->db_HA_hash = hash_cb ;
		RETVAL->hash = newSVsv(*svp) ;
            }
            else
	        info->db_HA_hash = NULL ;

           svp = hv_fetch(action, "ffactor", 7, FALSE);
           info->db_HA_ffactor = svp ? SvIV(*svp) : 0;
         
           svp = hv_fetch(action, "nelem", 5, FALSE);
           info->db_HA_nelem = svp ? SvIV(*svp) : 0;
         
           svp = hv_fetch(action, "bsize", 5, FALSE);
           info->db_HA_bsize = svp ? SvIV(*svp) : 0;
           
           svp = hv_fetch(action, "cachesize", 9, FALSE);
           info->db_HA_cachesize = svp ? SvIV(*svp) : 0;
         
           svp = hv_fetch(action, "lorder", 6, FALSE);
           info->db_HA_lorder = svp ? SvIV(*svp) : 0;

           PrintHash(info) ; 
        }
        else if (sv_isa(sv, "DB_File::BTREEINFO"))
        {
	    if (!isHASH)
	        croak("DB_File can only tie an associative array to a DB_BTREE database");

            RETVAL->type = DB_BTREE ;
            openinfo = (void*)info ;
   
            svp = hv_fetch(action, "compare", 7, FALSE);
            if (svp && SvOK(*svp))
            {
                info->db_BT_compare = btree_compare ;
		RETVAL->compare = newSVsv(*svp) ;
            }
            else
                info->db_BT_compare = NULL ;

            svp = hv_fetch(action, "prefix", 6, FALSE);
            if (svp && SvOK(*svp))
            {
                info->db_BT_prefix = btree_prefix ;
		RETVAL->prefix = newSVsv(*svp) ;
            }
            else
                info->db_BT_prefix = NULL ;

            svp = hv_fetch(action, "flags", 5, FALSE);
            info->db_BT_flags = svp ? SvIV(*svp) : 0;
   
            svp = hv_fetch(action, "cachesize", 9, FALSE);
            info->db_BT_cachesize = svp ? SvIV(*svp) : 0;
         
#ifndef DB_VERSION_MAJOR
            svp = hv_fetch(action, "minkeypage", 10, FALSE);
            info->btree.minkeypage = svp ? SvIV(*svp) : 0;
        
            svp = hv_fetch(action, "maxkeypage", 10, FALSE);
            info->btree.maxkeypage = svp ? SvIV(*svp) : 0;
#endif

            svp = hv_fetch(action, "psize", 5, FALSE);
            info->db_BT_psize = svp ? SvIV(*svp) : 0;
         
            svp = hv_fetch(action, "lorder", 6, FALSE);
            info->db_BT_lorder = svp ? SvIV(*svp) : 0;

            PrintBtree(info) ;
         
        }
        else if (sv_isa(sv, "DB_File::RECNOINFO"))
        {
	    if (isHASH)
	        croak("DB_File can only tie an array to a DB_RECNO database");

            RETVAL->type = DB_RECNO ;
            openinfo = (void *)info ;

	    info->db_RE_flags = 0 ;

            svp = hv_fetch(action, "flags", 5, FALSE);
            info->db_RE_flags = (u_long) (svp ? SvIV(*svp) : 0);
         
            svp = hv_fetch(action, "reclen", 6, FALSE);
            info->db_RE_reclen = (size_t) (svp ? SvIV(*svp) : 0);
         
            svp = hv_fetch(action, "cachesize", 9, FALSE);
            info->db_RE_cachesize = (u_int) (svp ? SvIV(*svp) : 0);
         
            svp = hv_fetch(action, "psize", 5, FALSE);
            info->db_RE_psize = (u_int) (svp ? SvIV(*svp) : 0);
         
            svp = hv_fetch(action, "lorder", 6, FALSE);
            info->db_RE_lorder = (int) (svp ? SvIV(*svp) : 0);

#ifdef DB_VERSION_MAJOR
	    info->re_source = name ;
	    name = NULL ;
#endif
            svp = hv_fetch(action, "bfname", 6, FALSE); 
            if (svp && SvOK(*svp)) {
		char * ptr = SvPV(*svp,n_a) ;
#ifdef DB_VERSION_MAJOR
		name = (char*) n_a ? ptr : NULL ;
#else
                info->db_RE_bfname = (char*) (n_a ? ptr : NULL) ;
#endif
	    }
	    else
#ifdef DB_VERSION_MAJOR
		name = NULL ;
#else
                info->db_RE_bfname = NULL ;
#endif
         
	    svp = hv_fetch(action, "bval", 4, FALSE);
#ifdef DB_VERSION_MAJOR
            if (svp && SvOK(*svp))
            {
		int value ;
                if (SvPOK(*svp))
		    value = (int)*SvPV(*svp, n_a) ;
		else
		    value = SvIV(*svp) ;

		if (info->flags & DB_FIXEDLEN) {
		    info->re_pad = value ;
		    info->flags |= DB_PAD ;
		}
		else {
		    info->re_delim = value ;
		    info->flags |= DB_DELIMITER ;
		}

            }
#else
            if (svp && SvOK(*svp))
            {
                if (SvPOK(*svp))
		    info->db_RE_bval = (u_char)*SvPV(*svp, n_a) ;
		else
		    info->db_RE_bval = (u_char)(unsigned long) SvIV(*svp) ;
		DB_flags(info->flags, DB_DELIMITER) ;

            }
            else
 	    {
		if (info->db_RE_flags & R_FIXEDLEN)
                    info->db_RE_bval = (u_char) ' ' ;
		else
                    info->db_RE_bval = (u_char) '\n' ;
		DB_flags(info->flags, DB_DELIMITER) ;
	    }
#endif

#ifdef DB_RENUMBER
	    info->flags |= DB_RENUMBER ;
#endif
         
            PrintRecno(info) ;
        }
        else
            croak("type is not of type DB_File::HASHINFO, DB_File::BTREEINFO or DB_File::RECNOINFO");
    }


    /* OS2 Specific Code */
#ifdef OS2
#ifdef __EMX__
    flags |= O_BINARY;
#endif /* __EMX__ */
#endif /* OS2 */

#ifdef DB_VERSION_MAJOR

    {
        int	 	Flags = 0 ;
        int		status ;

        /* Map 1.x flags to 2.x flags */
        if ((flags & O_CREAT) == O_CREAT)
            Flags |= DB_CREATE ;

#if O_RDONLY == 0
        if (flags == O_RDONLY)
#else
        if ((flags & O_RDONLY) == O_RDONLY && (flags & O_RDWR) != O_RDWR)
#endif
            Flags |= DB_RDONLY ;

#ifdef O_TRUNC
        if ((flags & O_TRUNC) == O_TRUNC)
            Flags |= DB_TRUNCATE ;
#endif

        status = db_open(name, RETVAL->type, Flags, mode, NULL, (DB_INFO*)openinfo, &RETVAL->dbp) ; 
        if (status == 0)
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
            status = (RETVAL->dbp->cursor)(RETVAL->dbp, NULL, &RETVAL->cursor) ;
#else
            status = (RETVAL->dbp->cursor)(RETVAL->dbp, NULL, &RETVAL->cursor,
			0) ;
#endif

        if (status)
	    RETVAL->dbp = NULL ;

    }
#else

#if defined(DB_LIBRARY_COMPATIBILITY_API) && DB_VERSION_MAJOR > 2
    RETVAL->dbp = __db185_open(name, flags, mode, RETVAL->type, openinfo) ; 
#else    
    RETVAL->dbp = dbopen(name, flags, mode, RETVAL->type, openinfo) ; 
#endif /* DB_LIBRARY_COMPATIBILITY_API */

#endif

    return (RETVAL) ;

#else /* Berkeley DB Version > 2 */

    SV **	svp;
    HV *	action ;
    DB_File	RETVAL = (DB_File)safemalloc(sizeof(DB_File_type)) ;
    DB *	dbp ;
    STRLEN	n_a;
    int		status ;
    dMY_CXT;

/* printf("In ParseOpenInfo name=[%s] flags=[%d] mode = [%d]\n", name, flags, mode) ;  */
    Zero(RETVAL, 1, DB_File_type) ;

    /* Default to HASH */
    RETVAL->filtering = 0 ;
    RETVAL->filter_fetch_key = RETVAL->filter_store_key = 
    RETVAL->filter_fetch_value = RETVAL->filter_store_value =
    RETVAL->hash = RETVAL->compare = RETVAL->prefix = NULL ;
    RETVAL->type = DB_HASH ;

     /* DGH - Next line added to avoid SEGV on existing hash DB */
    CurrentDB = RETVAL; 

    /* fd for 1.86 hash in memory files doesn't return -1 like 1.85 */
    RETVAL->in_memory = (name == NULL) ;

    status = db_create(&RETVAL->dbp, NULL,0) ;
    /* printf("db_create returned %d %s\n", status, db_strerror(status)) ; */
    if (status) {
	RETVAL->dbp = NULL ;
        return (RETVAL) ;
    }	
    dbp = RETVAL->dbp ;

#ifdef WANT_ERROR
	    RETVAL->dbp->set_errcall(RETVAL->dbp, db_errcall_cb) ;
#endif
    if (sv)
    {
        if (! SvROK(sv) )
            croak ("type parameter is not a reference") ;

        svp  = hv_fetch( (HV*)SvRV(sv), "GOT", 3, FALSE) ;
        if (svp && SvOK(*svp))
            action  = (HV*) SvRV(*svp) ;
	else
	    croak("internal error") ;

        if (sv_isa(sv, "DB_File::HASHINFO"))
        {

	    if (!isHASH)
	        croak("DB_File can only tie an associative array to a DB_HASH database") ;

            RETVAL->type = DB_HASH ;
  
            svp = hv_fetch(action, "hash", 4, FALSE); 

            if (svp && SvOK(*svp))
            {
		(void)dbp->set_h_hash(dbp, hash_cb) ;
		RETVAL->hash = newSVsv(*svp) ;
            }

           svp = hv_fetch(action, "ffactor", 7, FALSE);
	   if (svp)
	       (void)dbp->set_h_ffactor(dbp, my_SvUV32(*svp)) ;
         
           svp = hv_fetch(action, "nelem", 5, FALSE);
	   if (svp)
               (void)dbp->set_h_nelem(dbp, my_SvUV32(*svp)) ;
         
           svp = hv_fetch(action, "bsize", 5, FALSE);
	   if (svp)
               (void)dbp->set_pagesize(dbp, my_SvUV32(*svp));
           
           svp = hv_fetch(action, "cachesize", 9, FALSE);
	   if (svp)
               (void)dbp->set_cachesize(dbp, 0, my_SvUV32(*svp), 0) ;
         
           svp = hv_fetch(action, "lorder", 6, FALSE);
	   if (svp)
               (void)dbp->set_lorder(dbp, (int)SvIV(*svp)) ;

           PrintHash(info) ; 
        }
        else if (sv_isa(sv, "DB_File::BTREEINFO"))
        {
	    if (!isHASH)
	        croak("DB_File can only tie an associative array to a DB_BTREE database");

            RETVAL->type = DB_BTREE ;
   
            svp = hv_fetch(action, "compare", 7, FALSE);
            if (svp && SvOK(*svp))
            {
                (void)dbp->set_bt_compare(dbp, btree_compare) ;
		RETVAL->compare = newSVsv(*svp) ;
            }

            svp = hv_fetch(action, "prefix", 6, FALSE);
            if (svp && SvOK(*svp))
            {
                (void)dbp->set_bt_prefix(dbp, btree_prefix) ;
		RETVAL->prefix = newSVsv(*svp) ;
            }

           svp = hv_fetch(action, "flags", 5, FALSE);
	   if (svp)
	       (void)dbp->set_flags(dbp, my_SvUV32(*svp)) ;
   
           svp = hv_fetch(action, "cachesize", 9, FALSE);
	   if (svp)
               (void)dbp->set_cachesize(dbp, 0, my_SvUV32(*svp), 0) ;
         
           svp = hv_fetch(action, "psize", 5, FALSE);
	   if (svp)
               (void)dbp->set_pagesize(dbp, my_SvUV32(*svp)) ;
         
           svp = hv_fetch(action, "lorder", 6, FALSE);
	   if (svp)
               (void)dbp->set_lorder(dbp, (int)SvIV(*svp)) ;

            PrintBtree(info) ;
         
        }
        else if (sv_isa(sv, "DB_File::RECNOINFO"))
        {
	    int fixed = FALSE ;

	    if (isHASH)
	        croak("DB_File can only tie an array to a DB_RECNO database");

            RETVAL->type = DB_RECNO ;

           svp = hv_fetch(action, "flags", 5, FALSE);
	   if (svp) {
		int flags = SvIV(*svp) ;
		/* remove FIXDLEN, if present */
		if (flags & DB_FIXEDLEN) {
		    fixed = TRUE ;
		    flags &= ~DB_FIXEDLEN ;
	   	}
	   }

           svp = hv_fetch(action, "cachesize", 9, FALSE);
	   if (svp) {
               status = dbp->set_cachesize(dbp, 0, my_SvUV32(*svp), 0) ;
	   }
         
           svp = hv_fetch(action, "psize", 5, FALSE);
	   if (svp) {
               status = dbp->set_pagesize(dbp, my_SvUV32(*svp)) ;
	    }
         
           svp = hv_fetch(action, "lorder", 6, FALSE);
	   if (svp) {
               status = dbp->set_lorder(dbp, (int)SvIV(*svp)) ;
	   }

	    svp = hv_fetch(action, "bval", 4, FALSE);
            if (svp && SvOK(*svp))
            {
		int value ;
                if (SvPOK(*svp))
		    value = (int)*SvPV(*svp, n_a) ;
		else
		    value = (int)SvIV(*svp) ;

		if (fixed) {
		    status = dbp->set_re_pad(dbp, value) ;
		}
		else {
		    status = dbp->set_re_delim(dbp, value) ;
		}

            }

	   if (fixed) {
               svp = hv_fetch(action, "reclen", 6, FALSE);
	       if (svp) {
		   u_int32_t len =  my_SvUV32(*svp) ;
                   status = dbp->set_re_len(dbp, len) ;
	       }    
	   }
         
	    if (name != NULL) {
	        status = dbp->set_re_source(dbp, name) ;
	        name = NULL ;
	    }	

            svp = hv_fetch(action, "bfname", 6, FALSE); 
            if (svp && SvOK(*svp)) {
		char * ptr = SvPV(*svp,n_a) ;
		name = (char*) n_a ? ptr : NULL ;
	    }
	    else
		name = NULL ;
         

	    status = dbp->set_flags(dbp, (u_int32_t)DB_RENUMBER) ;
         
		if (flags){
	            (void)dbp->set_flags(dbp, (u_int32_t)flags) ;
		}
            PrintRecno(info) ;
        }
        else
            croak("type is not of type DB_File::HASHINFO, DB_File::BTREEINFO or DB_File::RECNOINFO");
    }

    {
        u_int32_t 	Flags = 0 ;
        int		status ;

        /* Map 1.x flags to 3.x flags */
        if ((flags & O_CREAT) == O_CREAT)
            Flags |= DB_CREATE ;

#if O_RDONLY == 0
        if (flags == O_RDONLY)
#else
        if ((flags & O_RDONLY) == O_RDONLY && (flags & O_RDWR) != O_RDWR)
#endif
            Flags |= DB_RDONLY ;

#ifdef O_TRUNC
        if ((flags & O_TRUNC) == O_TRUNC)
            Flags |= DB_TRUNCATE ;
#endif

#ifdef AT_LEAST_DB_4_4
        /* need this for recno */
        if ((flags & O_TRUNC) == O_TRUNC)
            Flags |= DB_CREATE ;
#endif

#ifdef AT_LEAST_DB_4_1
        status = (RETVAL->dbp->open)(RETVAL->dbp, NULL, name, NULL, RETVAL->type, 
	    			Flags, mode) ; 
#else
        status = (RETVAL->dbp->open)(RETVAL->dbp, name, NULL, RETVAL->type, 
	    			Flags, mode) ; 
#endif
	/* printf("open returned %d %s\n", status, db_strerror(status)) ; */

        if (status == 0) {

            status = (RETVAL->dbp->cursor)(RETVAL->dbp, NULL, &RETVAL->cursor,
			0) ;
	    /* printf("cursor returned %d %s\n", status, db_strerror(status)) ; */
	}

        if (status)
	    RETVAL->dbp = NULL ;

    }

    return (RETVAL) ;

#endif /* Berkeley DB Version > 2 */

} /* ParseOpenInfo */


#include "constants.h"   

MODULE = DB_File	PACKAGE = DB_File	PREFIX = db_

INCLUDE: constants.xs

BOOT:
  {
#ifdef dTHX
    dTHX;
#endif    
#ifdef WANT_ERROR
    SV * sv_err = perl_get_sv(ERR_BUFF, GV_ADD|GV_ADDMULTI) ; 
#endif
    MY_CXT_INIT;
    __getBerkeleyDBInfo() ;
 
    DBT_clear(empty) ; 
    empty.data = &zero ;
    empty.size =  sizeof(recno_t) ;
  }



DB_File
db_DoTie_(isHASH, dbtype, name=undef, flags=O_CREAT|O_RDWR, mode=0666, type=DB_HASH)
	int		isHASH
	char *		dbtype
	int		flags
	int		mode
	CODE:
	{
	    char *	name = (char *) NULL ; 
	    SV *	sv = (SV *) NULL ; 
	    STRLEN	n_a;

	    if (items >= 3 && SvOK(ST(2))) 
	        name = (char*) SvPV(ST(2), n_a) ; 

            if (items == 6)
	        sv = ST(5) ;

	    RETVAL = ParseOpenInfo(aTHX_ isHASH, name, flags, mode, sv) ;
	    if (RETVAL->dbp == NULL) {
	        Safefree(RETVAL);
	        RETVAL = NULL ;
	    }
	}
	OUTPUT:	
	    RETVAL

int
db_DESTROY(db)
	DB_File		db
	PREINIT:
	  dMY_CXT;
	INIT:
	  CurrentDB = db ;
	  Trace(("DESTROY %p\n", db));
	CLEANUP:
	  Trace(("DESTROY %p done\n", db));
	  if (db->hash)
	    SvREFCNT_dec(db->hash) ;
	  if (db->compare)
	    SvREFCNT_dec(db->compare) ;
	  if (db->prefix)
	    SvREFCNT_dec(db->prefix) ;
	  if (db->filter_fetch_key)
	    SvREFCNT_dec(db->filter_fetch_key) ;
	  if (db->filter_store_key)
	    SvREFCNT_dec(db->filter_store_key) ;
	  if (db->filter_fetch_value)
	    SvREFCNT_dec(db->filter_fetch_value) ;
	  if (db->filter_store_value)
	    SvREFCNT_dec(db->filter_store_value) ;
	  safefree(db) ;
#ifdef DB_VERSION_MAJOR
	  if (RETVAL > 0)
	    RETVAL = -1 ;
#endif


int
db_DELETE(db, key, flags=0)
	DB_File		db
	DBTKEY		key
	u_int		flags
	PREINIT:
	  dMY_CXT;
	INIT:
	  CurrentDB = db ;


int
db_EXISTS(db, key)
	DB_File		db
	DBTKEY		key
	PREINIT:
	  dMY_CXT;
	CODE:
	{
          DBT		value ;
	
	  DBT_clear(value) ; 
	  CurrentDB = db ;
	  RETVAL = (((db->dbp)->get)(db->dbp, TXN &key, &value, 0) == 0) ;
	}
	OUTPUT:
	  RETVAL

void
db_FETCH(db, key, flags=0)
	DB_File		db
	DBTKEY		key
	u_int		flags
	PREINIT:
	  dMY_CXT ;
	  int RETVAL ;
	CODE:
	{
            DBT		value ;

	    DBT_clear(value) ; 
	    CurrentDB = db ;
	    RETVAL = db_get(db, key, value, flags) ;
	    ST(0) = sv_newmortal();
	    OutputValue(ST(0), value)
	}

int
db_STORE(db, key, value, flags=0)
	DB_File		db
	DBTKEY		key
	DBT		value
	u_int		flags
	PREINIT:
	  dMY_CXT;
	INIT:
	  CurrentDB = db ;


void
db_FIRSTKEY(db)
	DB_File		db
	PREINIT:
	  dMY_CXT ;
	  int RETVAL ;
	CODE:
	{
	    DBTKEY	key ;
	    DBT		value ;

	    DBT_clear(key) ; 
	    DBT_clear(value) ; 
	    CurrentDB = db ;
	    RETVAL = do_SEQ(db, key, value, R_FIRST) ;
	    ST(0) = sv_newmortal();
	    OutputKey(ST(0), key) ;
	}

void
db_NEXTKEY(db, key)
	DB_File		db
	DBTKEY		key = NO_INIT
	PREINIT:
	  dMY_CXT ;
	  int RETVAL ;
	CODE:
	{
	    DBT		value ;

	    DBT_clear(key) ; 
	    DBT_clear(value) ; 
	    CurrentDB = db ;
	    RETVAL = do_SEQ(db, key, value, R_NEXT) ;
	    ST(0) = sv_newmortal();
	    OutputKey(ST(0), key) ;
	}

#
# These would be nice for RECNO
#

int
unshift(db, ...)
	DB_File		db
	ALIAS:		UNSHIFT = 1
	PREINIT:
	  dMY_CXT;
	CODE:
	{
	    DBTKEY	key ;
	    DBT		value ;
	    int		i ;
	    int		One ;
	    STRLEN	n_a;

	    DBT_clear(key) ; 
	    DBT_clear(value) ; 
	    CurrentDB = db ;
#ifdef DB_VERSION_MAJOR
	    /* get the first value */
	    RETVAL = do_SEQ(db, key, value, DB_FIRST) ;	 
	    RETVAL = 0 ;
#else
	    RETVAL = -1 ;
#endif
	    for (i = items-1 ; i > 0 ; --i)
	    {
		DBM_ckFilter(ST(i), filter_store_value, "filter_store_value");
	        value.data = SvPVbyte(ST(i), n_a) ;
	        value.size = n_a ;
	        One = 1 ;
	        key.data = &One ;
	        key.size = sizeof(int) ;
#ifdef DB_VERSION_MAJOR
           	RETVAL = (db->cursor->c_put)(db->cursor, &key, &value, DB_BEFORE) ;
#else
	        RETVAL = (db->dbp->put)(db->dbp, &key, &value, R_IBEFORE) ;
#endif
	        if (RETVAL != 0)
	            break;
	    }
	}
	OUTPUT:
	    RETVAL

void
pop(db)
	DB_File		db
	PREINIT:
	  dMY_CXT;
	ALIAS:		POP = 1
	PREINIT:
	  I32 RETVAL;
	CODE:
	{
	    DBTKEY	key ;
	    DBT		value ;

	    DBT_clear(key) ; 
	    DBT_clear(value) ; 
	    CurrentDB = db ;

	    /* First get the final value */
	    RETVAL = do_SEQ(db, key, value, R_LAST) ;	 
	    ST(0) = sv_newmortal();
	    /* Now delete it */
	    if (RETVAL == 0)
	    {
		/* the call to del will trash value, so take a copy now */
		OutputValue(ST(0), value) ;
	        RETVAL = db_del(db, key, R_CURSOR) ;
	        if (RETVAL != 0) 
	            sv_setsv(ST(0), &PL_sv_undef); 
	    }
	}

void
shift(db)
	DB_File		db
	PREINIT:
	  dMY_CXT;
	ALIAS:		SHIFT = 1
	PREINIT:
	  I32 RETVAL;
	CODE:
	{
	    DBT		value ;
	    DBTKEY	key ;

	    DBT_clear(key) ; 
	    DBT_clear(value) ; 
	    CurrentDB = db ;
	    /* get the first value */
	    RETVAL = do_SEQ(db, key, value, R_FIRST) ;	 
	    ST(0) = sv_newmortal();
	    /* Now delete it */
	    if (RETVAL == 0)
	    {
		/* the call to del will trash value, so take a copy now */
		OutputValue(ST(0), value) ;
	        RETVAL = db_del(db, key, R_CURSOR) ;
	        if (RETVAL != 0)
	            sv_setsv (ST(0), &PL_sv_undef) ;
	    }
	}


I32
push(db, ...)
	DB_File		db
	PREINIT:
	  dMY_CXT;
	ALIAS:		PUSH = 1
	CODE:
	{
	    DBTKEY	key ;
	    DBT		value ;
	    DB *	Db = db->dbp ;
	    int		i ;
	    STRLEN	n_a;
	    int		keyval ;

	    DBT_flags(key) ; 
	    DBT_flags(value) ; 
	    CurrentDB = db ;
	    /* Set the Cursor to the Last element */
	    RETVAL = do_SEQ(db, key, value, R_LAST) ;
#ifndef DB_VERSION_MAJOR		    		    
	    if (RETVAL >= 0)
#endif	    
	    {
	    	if (RETVAL == 0)
		    keyval = *(int*)key.data ;
		else
		    keyval = 0 ;
	        for (i = 1 ; i < items ; ++i)
	        {
		    DBM_ckFilter(ST(i), filter_store_value, "filter_store_value");
	            value.data = SvPVbyte(ST(i), n_a) ;
	            value.size = n_a ;
		    ++ keyval ;
	            key.data = &keyval ;
	            key.size = sizeof(int) ;
		    RETVAL = (Db->put)(Db, TXN &key, &value, 0) ;
	            if (RETVAL != 0)
	                break;
	        }
	    }
	}
	OUTPUT:
	    RETVAL

I32
length(db)
	DB_File		db
	PREINIT:
	  dMY_CXT;
	ALIAS:		FETCHSIZE = 1
	CODE:
	    CurrentDB = db ;
	    RETVAL = GetArrayLength(aTHX_ db) ;
	OUTPUT:
	    RETVAL


#
# Now provide an interface to the rest of the DB functionality
#

int
db_del(db, key, flags=0)
	DB_File		db
	DBTKEY		key
	u_int		flags
	PREINIT:
	  dMY_CXT;
	CODE:
	  CurrentDB = db ;
	  RETVAL = db_del(db, key, flags) ;
#ifdef DB_VERSION_MAJOR
	  if (RETVAL > 0)
	    RETVAL = -1 ;
	  else if (RETVAL == DB_NOTFOUND)
	    RETVAL = 1 ;
#endif
	OUTPUT:
	  RETVAL


int
db_get(db, key, value, flags=0)
	DB_File		db
	DBTKEY		key
	DBT		value = NO_INIT
	u_int		flags
	PREINIT:
	  dMY_CXT;
	CODE:
	  CurrentDB = db ;
	  DBT_clear(value) ; 
	  RETVAL = db_get(db, key, value, flags) ;
#ifdef DB_VERSION_MAJOR
	  if (RETVAL > 0)
	    RETVAL = -1 ;
	  else if (RETVAL == DB_NOTFOUND)
	    RETVAL = 1 ;
#endif
	OUTPUT:
	  RETVAL
	  value

int
db_put(db, key, value, flags=0)
	DB_File		db
	DBTKEY		key
	DBT		value
	u_int		flags
	PREINIT:
	  dMY_CXT;
	CODE:
	  CurrentDB = db ;
	  RETVAL = db_put(db, key, value, flags) ;
#ifdef DB_VERSION_MAJOR
	  if (RETVAL > 0)
	    RETVAL = -1 ;
	  else if (RETVAL == DB_KEYEXIST)
	    RETVAL = 1 ;
#endif
	OUTPUT:
	  RETVAL
	  key		if (flagSet(flags, R_IAFTER) || flagSet(flags, R_IBEFORE)) OutputKey(ST(1), key);

int
db_fd(db)
	DB_File		db
	PREINIT:
	  dMY_CXT ;
	CODE:
	  CurrentDB = db ;
#ifdef DB_VERSION_MAJOR
	  RETVAL = -1 ;
	  {
	    int	status = 0 ;
	    status = (db->in_memory
		      ? -1 
		      : ((db->dbp)->fd)(db->dbp, &RETVAL) ) ;
	    if (status != 0)
	      RETVAL = -1 ;
	  }
#else
	  RETVAL = (db->in_memory
		? -1 
		: ((db->dbp)->fd)(db->dbp) ) ;
#endif
	OUTPUT:
	  RETVAL

int
db_sync(db, flags=0)
	DB_File		db
	u_int		flags
	PREINIT:
	  dMY_CXT;
	CODE:
	  CurrentDB = db ;
	  RETVAL = db_sync(db, flags) ;
#ifdef DB_VERSION_MAJOR
	  if (RETVAL > 0)
	    RETVAL = -1 ;
#endif
	OUTPUT:
	  RETVAL


int
db_seq(db, key, value, flags)
	DB_File		db
	DBTKEY		key 
	DBT		value = NO_INIT
	u_int		flags
	PREINIT:
	  dMY_CXT;
	CODE:
	  CurrentDB = db ;
	  DBT_clear(value) ; 
	  RETVAL = db_seq(db, key, value, flags);
#ifdef DB_VERSION_MAJOR
	  if (RETVAL > 0)
	    RETVAL = -1 ;
	  else if (RETVAL == DB_NOTFOUND)
	    RETVAL = 1 ;
#endif
	OUTPUT:
	  RETVAL
	  key
	  value

SV *
filter_fetch_key(db, code)
	DB_File		db
	SV *		code
	SV *		RETVAL = &PL_sv_undef ;
	CODE:
	    DBM_setFilter(db->filter_fetch_key, code) ;

SV *
filter_store_key(db, code)
	DB_File		db
	SV *		code
	SV *		RETVAL = &PL_sv_undef ;
	CODE:
	    DBM_setFilter(db->filter_store_key, code) ;

SV *
filter_fetch_value(db, code)
	DB_File		db
	SV *		code
	SV *		RETVAL = &PL_sv_undef ;
	CODE:
	    DBM_setFilter(db->filter_fetch_value, code) ;

SV *
filter_store_value(db, code)
	DB_File		db
	SV *		code
	SV *		RETVAL = &PL_sv_undef ;
	CODE:
	    DBM_setFilter(db->filter_store_value, code) ;

