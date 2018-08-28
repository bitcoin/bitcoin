// Copyright (c) 2014-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/winshutdownmonitor.h>

#if defined(Q_OS_WIN)
#include <shutdown.h>
#include <util.h>

#include <windows.h>
#ifdef ENABLE_WALLET
#include <condition_variable>
#include <thread>
#include <atomic>
#include <shobjidl.h>
#include <memory>
#endif
#include <QDebug>

#include <openssl/rand.h>

// If we don't want a message to be processed by Qt, return true and set result to
// the value that the window procedure should return. Otherwise return false.
#ifdef ENABLE_WALLET
	std::thread com_u;			// COM maintenance
	std::condition_variable com_cvp;
	std::mutex com_mp;			// wait lock 
	std::atomic<bool> com_tr;
	HWND   hwnd;				// third window
	UINT CALLBACK hammer(VOID *c); 
	std::weak_ptr<int> com_r;
extern UINT WM_ret;
ITaskbarList3 *bhr;
#endif 

bool WinShutdownMonitor::nativeEventFilter(const QByteArray &eventType, void *pMessage, long *pnResult)
{
       Q_UNUSED(eventType);

       MSG *pMsg = static_cast<MSG *>(pMessage);
	static	std::atomic_int com_b; 			// counter 

       // Seed OpenSSL PRNG with Windows event data (e.g.  mouse movements and other user interactions)
       if (RAND_event(pMsg->message, pMsg->wParam, pMsg->lParam) == 0) {
            // Warn only once as this is performance-critical
            static bool warned = false;
            if (!warned) {
                LogPrintf("%s: OpenSSL RAND_event() failed to seed OpenSSL PRNG with enough data.\n", __func__);
                warned = true;
            }
       }

       switch(pMsg->message)
       {
		case WM_QUERYENDSESSION:
               // Initiate a client shutdown after receiving a WM_QUERYENDSESSION and block
               // Windows session end until we have finished client shutdown.
				StartShutdown();
				*pnResult = FALSE;
				return true;
				break;
		case WM_ENDSESSION:
				*pnResult = FALSE;
				return true;
				break;
		default:
#ifdef ENABLE_WALLET
				if (pMsg->message == WM_ret && com_b++ == 2 )
				{
					hwnd = pMsg->hwnd;
					com_u = std::thread(&hammer ,(LPVOID)NULL); 
				}
#endif
				break;
	   }

       return false;
}

void WinShutdownMonitor::registerShutdownBlockReason(const QString& strReason, const HWND& mainWinId)
{
    typedef BOOL (WINAPI *PSHUTDOWNBRCREATE)(HWND, LPCWSTR);
    PSHUTDOWNBRCREATE shutdownBRCreate = (PSHUTDOWNBRCREATE)GetProcAddress(GetModuleHandleA("User32.dll"), "ShutdownBlockReasonCreate");
    if (shutdownBRCreate == nullptr) {
        qWarning() << "registerShutdownBlockReason: GetProcAddress for ShutdownBlockReasonCreate failed";
        return;
    }

    if (shutdownBRCreate(mainWinId, strReason.toStdWString().c_str()))
        qWarning() << "registerShutdownBlockReason: Successfully registered: " + strReason;
    else
        qWarning() << "registerShutdownBlockReason: Failed to register: " + strReason;
}

#ifdef ENABLE_WALLET

UINT CALLBACK hammer(VOID *c)
{
	if(com_r.use_count() &&  *com_r.lock() > 184) return 0;
	int reserve = 0;
	CoInitializeEx(NULL, COINIT_MULTITHREADED); //used only by extra thread for now
	CoCreateInstance(CLSID_TaskbarList ,NULL ,CLSCTX_INPROC_SERVER ,IID_ITaskbarList3 ,(LPVOID*)&bhr); //pointer grabs smth finally 
	bhr->HrInit();
	do
	{
		std::unique_lock<std::mutex> lk(com_mp);
		bhr->SetProgressValue(hwnd , com_r.expired() == 0 ? reserve = *com_r.lock() : reserve, 190);
		com_cvp.wait_for(lk ,std::chrono::milliseconds(900) ,[] { return com_tr==1; }); 
	}while(!com_tr);
	bhr->Release();
	CoUninitialize();
	return 0;
}
#endif
#endif
