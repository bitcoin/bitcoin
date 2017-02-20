/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2007-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef __NEWRES_H__
#define __NEWRES_H__

#if !defined(UNDER_CE)
    #define UNDER_CE _WIN32_WCE
#endif

#if defined(_WIN32_WCE)
    #if !defined(WCEOLE_ENABLE_DIALOGEX)
        #define DIALOGEX DIALOG DISCARDABLE
    #endif
    #include <commctrl.h>
    #define  SHMENUBAR RCDATA
    #if defined(WIN32_PLATFORM_PSPC) && (_WIN32_WCE >= 300)
        #include <aygshell.h>
    #else
        #define I_IMAGENONE     (-2)
        #define NOMENU          0xFFFF
        #define IDS_SHNEW       1

        #define IDM_SHAREDNEW        10
        #define IDM_SHAREDNEWDEFAULT 11
    #endif
#endif // _WIN32_WCE

#ifdef RC_INVOKED
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
    #include "winuser.h"           // extract from windows header
#endif
#endif

#ifdef IDC_STATIC
#undef IDC_STATIC
#endif
#define IDC_STATIC      (-1)

#endif //__NEWRES_H__
