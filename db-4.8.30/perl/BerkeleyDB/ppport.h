/* This file is Based on output from 
 * Perl/Pollution/Portability Version 2.0000 */

#ifndef _P_P_PORTABILITY_H_
#define _P_P_PORTABILITY_H_

#ifndef PERL_REVISION
#   ifndef __PATCHLEVEL_H_INCLUDED__
#       include "patchlevel.h"
#   endif
#   ifndef PERL_REVISION
#   define PERL_REVISION    (5)
        /* Replace: 1 */
#       define PERL_VERSION PATCHLEVEL
#       define PERL_SUBVERSION  SUBVERSION
        /* Replace PERL_PATCHLEVEL with PERL_VERSION */
        /* Replace: 0 */
#   endif
#endif

#define PERL_BCDVERSION ((PERL_REVISION * 0x1000000L) + (PERL_VERSION * 0x1000L) + PERL_SUBVERSION)

#ifndef ERRSV
#   define ERRSV perl_get_sv("@",FALSE)
#endif

#if (PERL_VERSION < 4) || ((PERL_VERSION == 4) && (PERL_SUBVERSION <= 5))
/* Replace: 1 */
#   define PL_Sv        Sv
#   define PL_compiling compiling
#   define PL_copline   copline
#   define PL_curcop    curcop
#   define PL_curstash  curstash
#   define PL_defgv     defgv
#   define PL_dirty     dirty
#   define PL_hints     hints
#   define PL_na        na
#   define PL_perldb    perldb
#   define PL_rsfp_filters  rsfp_filters
#   define PL_rsfp      rsfp
#   define PL_stdingv   stdingv
#   define PL_sv_no     sv_no
#   define PL_sv_undef  sv_undef
#   define PL_sv_yes    sv_yes
/* Replace: 0 */
#endif

#ifndef pTHX
#    define pTHX
#    define pTHX_
#    define aTHX
#    define aTHX_
#endif         

#ifndef PTR2IV
#    define PTR2IV(d)   (IV)(d)
#endif
 
#ifndef INT2PTR
#    define INT2PTR(any,d)      (any)(d)
#endif

#ifndef dTHR
#  ifdef WIN32
#   define dTHR extern int Perl___notused
#  else
#   define dTHR extern int errno
#  endif
#endif

#ifndef boolSV
#   define boolSV(b) ((b) ? &PL_sv_yes : &PL_sv_no)
#endif

#ifndef gv_stashpvn
#   define gv_stashpvn(str,len,flags) gv_stashpv(str,flags)
#endif

#ifndef newSVpvn
#   define newSVpvn(data,len) ((len) ? newSVpv ((data), (len)) : newSVpv ("", 0))
#endif

#ifndef newRV_inc
/* Replace: 1 */
#   define newRV_inc(sv) newRV(sv)
/* Replace: 0 */
#endif

#ifndef SvGETMAGIC
#  define SvGETMAGIC(x)                  STMT_START { if (SvGMAGICAL(x)) mg_get(x); } STMT_END
#endif


/* DEFSV appears first in 5.004_56 */
#ifndef DEFSV
#  define DEFSV GvSV(PL_defgv)
#endif

#ifndef SAVE_DEFSV
#    define SAVE_DEFSV SAVESPTR(GvSV(PL_defgv))
#endif

#ifndef newRV_noinc
#  ifdef __GNUC__
#    define newRV_noinc(sv)               \
      ({                                  \
          SV *nsv = (SV*)newRV(sv);       \
          SvREFCNT_dec(sv);               \
          nsv;                            \
      })
#  else
#    if defined(CRIPPLED_CC) || defined(USE_THREADS)
static SV * newRV_noinc (SV * sv)
{
          SV *nsv = (SV*)newRV(sv);       
          SvREFCNT_dec(sv);               
          return nsv;                     
}
#    else
#      define newRV_noinc(sv)    \
        ((PL_Sv=(SV*)newRV(sv), SvREFCNT_dec(sv), (SV*)PL_Sv)
#    endif
#  endif
#endif

/* Provide: newCONSTSUB */

/* newCONSTSUB from IO.xs is in the core starting with 5.004_63 */
#if (PERL_VERSION < 4) || ((PERL_VERSION == 4) && (PERL_SUBVERSION < 63))

#if defined(NEED_newCONSTSUB)
static
#else
extern void newCONSTSUB _((HV * stash, char * name, SV *sv));
#endif

#if defined(NEED_newCONSTSUB) || defined(NEED_newCONSTSUB_GLOBAL)
void
newCONSTSUB(stash,name,sv)
HV *stash;
char *name;
SV *sv;
{
    U32 oldhints = PL_hints;
    HV *old_cop_stash = PL_curcop->cop_stash;
    HV *old_curstash = PL_curstash;
    line_t oldline = PL_curcop->cop_line;
    PL_curcop->cop_line = PL_copline;

    PL_hints &= ~HINT_BLOCK_SCOPE;
    if (stash)
        PL_curstash = PL_curcop->cop_stash = stash;

    newSUB(

#if (PERL_VERSION < 3) || ((PERL_VERSION == 3) && (PERL_SUBVERSION < 22))
     /* before 5.003_22 */
        start_subparse(),
#else
#  if (PERL_VERSION == 3) && (PERL_SUBVERSION == 22)
     /* 5.003_22 */
            start_subparse(0),
#  else
     /* 5.003_23  onwards */
            start_subparse(FALSE, 0),
#  endif
#endif

        newSVOP(OP_CONST, 0, newSVpv(name,0)),
        newSVOP(OP_CONST, 0, &PL_sv_no),   /* SvPV(&PL_sv_no) == "" -- GMB */
        newSTATEOP(0, Nullch, newSVOP(OP_CONST, 0, sv))
    );

    PL_hints = oldhints;
    PL_curcop->cop_stash = old_cop_stash;
    PL_curstash = old_curstash;
    PL_curcop->cop_line = oldline;
}
#endif

#endif /* newCONSTSUB */


#ifndef START_MY_CXT

/*
 * Boilerplate macros for initializing and accessing interpreter-local
 * data from C.  All statics in extensions should be reworked to use
 * this, if you want to make the extension thread-safe.  See ext/re/re.xs
 * for an example of the use of these macros.
 *
 * Code that uses these macros is responsible for the following:
 * 1. #define MY_CXT_KEY to a unique string, e.g. "DynaLoader_guts"
 * 2. Declare a typedef named my_cxt_t that is a structure that contains
 *    all the data that needs to be interpreter-local.
 * 3. Use the START_MY_CXT macro after the declaration of my_cxt_t.
 * 4. Use the MY_CXT_INIT macro such that it is called exactly once
 *    (typically put in the BOOT: section).
 * 5. Use the members of the my_cxt_t structure everywhere as
 *    MY_CXT.member.
 * 6. Use the dMY_CXT macro (a declaration) in all the functions that
 *    access MY_CXT.
 */

#if defined(MULTIPLICITY) || defined(PERL_OBJECT) || \
    defined(PERL_CAPI)    || defined(PERL_IMPLICIT_CONTEXT)

/* This must appear in all extensions that define a my_cxt_t structure,
 * right after the definition (i.e. at file scope).  The non-threads
 * case below uses it to declare the data as static. */
#define START_MY_CXT

#if PERL_REVISION == 5 && \
    (PERL_VERSION < 4 || (PERL_VERSION == 4 && PERL_SUBVERSION < 68 ))
/* Fetches the SV that keeps the per-interpreter data. */
#define dMY_CXT_SV \
    SV *my_cxt_sv = perl_get_sv(MY_CXT_KEY, FALSE)
#else /* >= perl5.004_68 */
#define dMY_CXT_SV \
    SV *my_cxt_sv = *hv_fetch(PL_modglobal, MY_CXT_KEY,     \
                  sizeof(MY_CXT_KEY)-1, TRUE)
#endif /* < perl5.004_68 */

/* This declaration should be used within all functions that use the
 * interpreter-local data. */
#define dMY_CXT \
    dMY_CXT_SV;                         \
    my_cxt_t *my_cxtp = INT2PTR(my_cxt_t*,SvUV(my_cxt_sv))

/* Creates and zeroes the per-interpreter data.
 * (We allocate my_cxtp in a Perl SV so that it will be released when
 * the interpreter goes away.) */
#define MY_CXT_INIT \
    dMY_CXT_SV;                         \
    /* newSV() allocates one more than needed */            \
    my_cxt_t *my_cxtp = (my_cxt_t*)SvPVX(newSV(sizeof(my_cxt_t)-1));\
    Zero(my_cxtp, 1, my_cxt_t);                 \
    sv_setuv(my_cxt_sv, PTR2UV(my_cxtp))

/* This macro must be used to access members of the my_cxt_t structure.
 * e.g. MYCXT.some_data */
#define MY_CXT      (*my_cxtp)

/* Judicious use of these macros can reduce the number of times dMY_CXT
 * is used.  Use is similar to pTHX, aTHX etc. */
#define pMY_CXT     my_cxt_t *my_cxtp
#define pMY_CXT_    pMY_CXT,
#define _pMY_CXT    ,pMY_CXT
#define aMY_CXT     my_cxtp
#define aMY_CXT_    aMY_CXT,
#define _aMY_CXT    ,aMY_CXT

#else /* single interpreter */

#ifndef NOOP
#  define NOOP (void)0
#endif

#ifdef HASATTRIBUTE
#  define PERL_UNUSED_DECL __attribute__((unused))
#else
#  define PERL_UNUSED_DECL
#endif    

#ifndef dNOOP
#  define dNOOP extern int Perl___notused PERL_UNUSED_DECL
#endif

#define START_MY_CXT    static my_cxt_t my_cxt;
#define dMY_CXT_SV  dNOOP
#define dMY_CXT     dNOOP
#define MY_CXT_INIT NOOP
#define MY_CXT      my_cxt

#define pMY_CXT     void
#define pMY_CXT_
#define _pMY_CXT
#define aMY_CXT
#define aMY_CXT_
#define _aMY_CXT

#endif 

#endif /* START_MY_CXT */


#if 1
#ifdef DBM_setFilter
#undef DBM_setFilter
#undef DBM_ckFilter
#endif
#endif

#ifndef DBM_setFilter

/* 
   The DBM_setFilter & DBM_ckFilter macros are only used by 
   the *DB*_File modules 
*/

#define DBM_setFilter(db_type,code)             \
    {                           \
        if (db_type)                    \
            RETVAL = sv_mortalcopy(db_type) ;       \
        ST(0) = RETVAL ;                    \
        if (db_type && (code == &PL_sv_undef)) {        \
                SvREFCNT_dec(db_type) ;             \
            db_type = NULL ;                \
        }                           \
        else if (code) {                    \
            if (db_type)                    \
                sv_setsv(db_type, code) ;           \
            else                        \
                db_type = newSVsv(code) ;           \
        }                               \
    }

#define DBM_ckFilter(arg,type,name)             \
    if (db->type) {                     \
        /* printf("Filtering %s\n", name); */       \
        if (db->filtering) {                \
            croak("recursion detected in %s", name) ;   \
        }                                   \
        ENTER ;                     \
        SAVETMPS ;                      \
        SAVEINT(db->filtering) ;                \
        db->filtering = TRUE ;              \
        SAVESPTR(DEFSV) ;                   \
        if (name[7] == 's')                 \
            arg = newSVsv(arg);             \
        DEFSV = arg ;                   \
        SvTEMP_off(arg) ;                   \
        PUSHMARK(SP) ;                  \
        PUTBACK ;                       \
        (void) perl_call_sv(db->type, G_DISCARD);       \
        arg = DEFSV ;                   \
        SPAGAIN ;                       \
        PUTBACK ;                       \
        FREETMPS ;                      \
        LEAVE ;                     \
        if (name[7] == 's'){                \
            arg = sv_2mortal(arg);              \
        }                           \
        SvOKp(arg);                     \
    }

#endif /* DBM_setFilter */

#endif /* _P_P_PORTABILITY_H_ */
