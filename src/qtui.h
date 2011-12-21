// Copyright (c) 2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_EXTERNUI_H
#define BITCOIN_EXTERNUI_H

#include <string>
#include <boost/function/function0.hpp>
#include "wallet.h"

typedef void wxWindow;
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

extern int MyMessageBox(const std::string& message, const std::string& caption="Message", int style=wxOK, wxWindow* parent=NULL, int x=-1, int y=-1);
#define wxMessageBox  MyMessageBox
extern int ThreadSafeMessageBox(const std::string& message, const std::string& caption, int style=wxOK, wxWindow* parent=NULL, int x=-1, int y=-1);
extern bool ThreadSafeAskFee(int64 nFeeRequired, const std::string& strCaption, wxWindow* parent);
extern void CalledSetStatusBar(const std::string& strText, int nField);
extern void UIThreadCall(boost::function0<void> fn);
extern void MainFrameRepaint();
extern void InitMessage(const std::string &message);
extern std::string _(const char* psz);

#endif
