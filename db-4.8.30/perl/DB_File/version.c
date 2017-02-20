/* 

 version.c -- Perl 5 interface to Berkeley DB 

 written by Paul Marquess <Paul.Marquess@btinternet.com>
 last modified 2nd Jan 2002
 version 1.802

 All comments/suggestions/problems are welcome

     Copyright (c) 1995-2002 Paul Marquess. All rights reserved.
     This program is free software; you can redistribute it and/or
     modify it under the same terms as Perl itself.

 Changes:
        1.71 -  Support for Berkeley DB version 3.
		Support for Berkeley DB 2/3's backward compatability mode.
        1.72 -  No change.
        1.73 -  Added support for threading
        1.74 -  Added Perl core patch 7801.


*/

#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"  
#include "perl.h"
#include "XSUB.h"

#include <db.h>

void
#ifdef CAN_PROTOTYPE
__getBerkeleyDBInfo(void)
#else
__getBerkeleyDBInfo()
#endif
{
#ifdef dTHX	
    dTHX;
#endif    
    SV * version_sv = perl_get_sv("DB_File::db_version", GV_ADD|GV_ADDMULTI) ;
    SV * ver_sv = perl_get_sv("DB_File::db_ver", GV_ADD|GV_ADDMULTI) ;
    SV * compat_sv = perl_get_sv("DB_File::db_185_compat", GV_ADD|GV_ADDMULTI) ;

#ifdef DB_VERSION_MAJOR
    int Major, Minor, Patch ;

    (void)db_version(&Major, &Minor, &Patch) ;

    /* Check that the versions of db.h and libdb.a are the same */
    if (Major != DB_VERSION_MAJOR || Minor != DB_VERSION_MINOR )
		/* || Patch != DB_VERSION_PATCH) */

	croak("\nDB_File was build with libdb version %d.%d.%d,\nbut you are attempting to run it with libdb version %d.%d.%d\n",
		DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH, 
		Major, Minor, Patch) ;
    
    /* check that libdb is recent enough  -- we need 2.3.4 or greater */
    if (Major == 2 && (Minor < 3 || (Minor ==  3 && Patch < 4)))
	croak("DB_File needs Berkeley DB 2.3.4 or greater, you have %d.%d.%d\n",
		 Major, Minor, Patch) ;
 
    {
        char buffer[40] ;
        sprintf(buffer, "%d.%d", Major, Minor) ;
        sv_setpv(version_sv, buffer) ; 
        sprintf(buffer, "%d.%03d%03d", Major, Minor, Patch) ;
        sv_setpv(ver_sv, buffer) ; 
    }
 
#else /* ! DB_VERSION_MAJOR */
    sv_setiv(version_sv, 1) ;
    sv_setiv(ver_sv, 1) ;
#endif /* ! DB_VERSION_MAJOR */

#ifdef COMPAT185
    sv_setiv(compat_sv, 1) ;
#else /* ! COMPAT185 */
    sv_setiv(compat_sv, 0) ;
#endif /* ! COMPAT185 */

}
