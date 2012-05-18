// Copyright (c) 2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_UI_INTERFACE_H
#define BITCOIN_UI_INTERFACE_H

#include <string>
#include "util.h" // for int64

#define wxYES                   0x00000002
#define wxOK                    0x00000004
#define wxNO                    0x00000008
#define wxYES_NO                (wxYES|wxNO)
#define wxCANCEL                0x00000010
#define wxAPPLY                 0x00000020
#define wxCLOSE                 0x00000040
#define wxOK_DEFAULT            0x00000000
#define wxYES_DEFAULT           0x00000000
#define wxNO_DEFAULT            0x00000080
#define wxCANCEL_DEFAULT        0x80000000
#define wxICON_EXCLAMATION      0x00000100
#define wxICON_HAND             0x00000200
#define wxICON_WARNING          wxICON_EXCLAMATION
#define wxICON_ERROR            wxICON_HAND
#define wxICON_QUESTION         0x00000400
#define wxICON_INFORMATION      0x00000800
#define wxICON_STOP             wxICON_HAND
#define wxICON_ASTERISK         wxICON_INFORMATION
#define wxICON_MASK             (0x00000100|0x00000200|0x00000400|0x00000800)
#define wxFORWARD               0x00001000
#define wxBACKWARD              0x00002000
#define wxRESET                 0x00004000
#define wxHELP                  0x00008000
#define wxMORE                  0x00010000
#define wxSETUP                 0x00020000
// Force blocking, modal message box dialog (not just notification)
#define wxMODAL                 0x00040000

/* These UI communication functions are implemented in bitcoin.cpp (for ui) and noui.cpp (no ui) */

extern int ThreadSafeMessageBox(const std::string& message, const std::string& caption, int style=wxOK);
extern bool ThreadSafeAskFee(int64 nFeeRequired, const std::string& strCaption);
extern void ThreadSafeHandleURI(const std::string& strURI);
extern void MainFrameRepaint();
extern void AddressBookRepaint();
extern void QueueShutdown();
extern void InitMessage(const std::string &message);
extern std::string _(const char* psz);

#endif
