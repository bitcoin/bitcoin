/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2007-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

// TpcbUI.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include "resource.h"
#include "TpcbExample.h"
#include <commctrl.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE       hInst;      // The current instance
HWND            hWndDlgMain;    // Handle to the main dialog window.
HWND            hWndFrame;  // Handle to the main window.
TpcbExample     *tpcb;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass (HINSTANCE, LPTSTR);
BOOL                InitInstance    (HINSTANCE, int);
LRESULT CALLBACK    WndProc         (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    MainDialog      (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    InitDialog      (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    AdvancedDialog  (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    RunDialog       (HWND, UINT, WPARAM, LPARAM);
BOOL                GetHomeDirectory(HWND, BOOL);
BOOL                RecursiveDirRemove(wchar_t *);

int WINAPI WinMain( HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR    lpCmdLine,
                    int       nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;

    hWndDlgMain = NULL;
    // Initialize the tpcb object.
    tpcb = new TpcbExample();

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_WCE_TPCB);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    It is important to call this function so that the application
//    will get 'well formed' small icons associated with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
    WNDCLASS    wc;

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = (WNDPROC) WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WCE_TPCB));
    wc.hCursor          = 0;
    wc.hbrBackground    = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName     = 0;
    wc.lpszClassName    = szWindowClass;

    return RegisterClass(&wc);
}

//
//  FUNCTION: InitInstance(HANDLE, int)
//
//  PURPOSE: Saves instance handle and creates main window
//
//  COMMENTS:
//
//    In this function, we save the instance handle in a global variable and
//    create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    TCHAR   szTitle[MAX_LOADSTRING];    // The title bar text
    TCHAR   szWindowClass[MAX_LOADSTRING];  // The window class name

    hInst = hInstance;  // Store instance handle in our global variable
    // Initialize global strings
    LoadString(hInstance, IDC_WCE_TPCB, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance, szWindowClass);
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

    hWndFrame = CreateWindow(szWindowClass, szTitle,
        WS_VISIBLE | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (!hWndFrame)
    {
        return FALSE;
    }

    ShowWindow(hWndDlgMain, nCmdShow);
    UpdateWindow(hWndDlgMain);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hDlg;

    switch (message)
    {
        case WM_COMMAND:
            DefWindowProc(hWnd, message, wParam, lParam);
            break;
        case WM_CREATE:
            hDlg = CreateDialog(hInst,
                MAKEINTRESOURCE(IDD_MAINDIALOG), hWnd,
                (DLGPROC)MainDialog);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Message handler for the Main dialog box
LRESULT CALLBACK MainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rt;
    HWND    hCurrentRadioButton;
    wchar_t wdirname[MAX_PATH], msg[1024], *title;
    int ret, valid, ntxns, written;

    switch (message)
    {
        case WM_INITDIALOG:
            // maximize the dialog.
            GetClientRect(GetParent(hDlg), &rt);
            SetWindowPos(hDlg, HWND_TOP, 0, 0, rt.right, rt.bottom,
                SWP_SHOWWINDOW);
            CheckRadioButton(hDlg, IDC_MEDIUM_RADIO,
                IDC_SMALL_RADIO, IDC_SMALL_RADIO);
            SetDlgItemText(hDlg, IDC_HOME_EDIT,
                tpcb->getHomeDirW(wdirname, MAX_PATH));
            SetDlgItemInt(hDlg, IDC_TXN_EDIT, 1000, 0);

            SetWindowText(hDlg, L"BDB TPCB Example app");
            ShowWindow(hDlg, SW_SHOWNORMAL);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_INIT_BUTTON ||
                LOWORD(wParam) == IDC_RUN_BUTTON ) {
                hCurrentRadioButton = GetDlgItem(hDlg,
                    IDC_SMALL_RADIO);
                if(BST_CHECKED ==
                    SendMessage(hCurrentRadioButton,
                    BM_GETCHECK, NULL, NULL)) {
                    tpcb->accounts = 500;
                    tpcb->branches = 10;
                    tpcb->tellers  = 50;
                    tpcb->history  = 5000;
                }
                hCurrentRadioButton = GetDlgItem(hDlg,
                    IDC_MEDIUM_RADIO);
                if(BST_CHECKED ==
                    SendMessage(hCurrentRadioButton,
                    BM_GETCHECK, NULL, NULL)) {
                    tpcb->accounts = 1000;
                    tpcb->branches = 10;
                    tpcb->tellers  = 100;
                    tpcb->history  = 10000;
                }
                hCurrentRadioButton = GetDlgItem(hDlg,
                    IDC_LARGE_RADIO);
                if(BST_CHECKED ==
                    SendMessage(hCurrentRadioButton,
                    BM_GETCHECK, NULL, NULL)) {
                    tpcb->accounts = 100000;
                    tpcb->branches = 10;
                    tpcb->tellers  = 100;
                    tpcb->history  = 259200;
                }
                EnableWindow(GetDlgItem(hDlg, IDC_INIT_BUTTON),
                    FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RUN_BUTTON),
                    FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_ADV_BUTTON),
                    FALSE);
            }
            if (LOWORD(wParam) == IDC_ADV_BUTTON) {
                CreateDialog(hInst,
                    MAKEINTRESOURCE(IDD_ADVANCEDDIALOG), hDlg,
                    (DLGPROC)AdvancedDialog);
            } else if (LOWORD(wParam) == IDC_INIT_BUTTON) {
                // Close the environment first.
                // In case this is a re-initialization.
                tpcb->closeEnv();
                GetHomeDirectory(hDlg, TRUE);
                tpcb->createEnv(0);
                ret = tpcb->populate();
            } else if (LOWORD(wParam) == IDC_RUN_BUTTON) {
                GetHomeDirectory(hDlg, FALSE);
                if (GetFileAttributes(
                    tpcb->getHomeDirW(wdirname, MAX_PATH)) !=
                    FILE_ATTRIBUTE_DIRECTORY) {
                    _snwprintf(msg, 1024,
L"Target directory: %s does not exist, or is not a directory.\nMake sure the "
L"directory name is correct, and that you ran the initialization phase.",
                        wdirname);
                    MessageBox(hDlg, msg, L"Error", MB_OK);
                    EnableWindow(GetDlgItem(hDlg,
                        IDC_INIT_BUTTON), TRUE);
                    EnableWindow(GetDlgItem(hDlg,
                        IDC_RUN_BUTTON), TRUE);
                    EnableWindow(GetDlgItem(hDlg,
                        IDC_ADV_BUTTON), TRUE);
                    return FALSE;
                }
                // TODO: Check for an empty directory?
                ntxns = GetDlgItemInt(hDlg, IDC_TXN_EDIT,
                    &valid, FALSE);
                if (valid == FALSE) {
                    MessageBox(hDlg,
                    L"Invalid number in transaction field.",
                        L"Error", MB_OK);
                    EnableWindow(GetDlgItem(hDlg,
                        IDC_INIT_BUTTON), TRUE);
                    EnableWindow(GetDlgItem(hDlg,
                        IDC_RUN_BUTTON), TRUE);
                    EnableWindow(GetDlgItem(hDlg,
                        IDC_ADV_BUTTON), TRUE);
                    return FALSE;
                }
                tpcb->createEnv(0);
                ret = tpcb->run(ntxns);
            } else if (LOWORD(wParam) == IDC_EXIT_BUTTON) {
                tpcb->closeEnv();
                EndDialog(hDlg, LOWORD(wParam));
                DestroyWindow(hDlg);
                DestroyWindow(hWndFrame);
                return FALSE;
            }
            if (LOWORD(wParam) == IDC_INIT_BUTTON ||
                LOWORD(wParam) == IDC_RUN_BUTTON ) {
                if (ret == 0)
                    title = L"Results";
                else
                    title = L"Error message";
                written = MultiByteToWideChar(CP_ACP, NULL,
                    tpcb->msgString, strlen(tpcb->msgString),
                    msg, sizeof(msg)/sizeof(msg[0]));
                msg[written] = L'\0';
                MessageBox(hDlg, msg, title, MB_OK);
                EnableWindow(GetDlgItem(hDlg, IDC_INIT_BUTTON), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_RUN_BUTTON),
                    TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_ADV_BUTTON),
                    TRUE);
            }
            break;
        case WM_DESTROY:
            // Same functionality as WM_COMMAND->IDC_EXIT_BUTTON
            tpcb->closeEnv();
            EndDialog(hDlg, LOWORD(wParam));
            DestroyWindow(hDlg);
            DestroyWindow(hWndFrame);
            return FALSE;
        default:
            return DefWindowProc(hDlg, message, wParam, lParam);
    }
    return TRUE;
}

// Message handler for the Advanced dialog box
LRESULT CALLBACK AdvancedDialog(HWND hDlg, UINT message,
                WPARAM wParam, LPARAM lParam)
{
    RECT rt;
    HWND hCurrentCheckBox;
    int currentInt, valid;

    switch (message)
    {
        case WM_INITDIALOG:
            GetClientRect(GetParent(hDlg), &rt);
            SetWindowPos(hDlg, HWND_TOP, 0, 0, rt.right, rt.bottom,
                SWP_SHOWWINDOW);
            if (tpcb->fast_mode == 0) {
                hCurrentCheckBox =
                    GetDlgItem(hDlg, IDC_FASTMODE_CHECK);
                SendMessage(hCurrentCheckBox, BM_SETCHECK,
                    BST_CHECKED, 0);
            }
            if (tpcb->verbose == 1) {
                hCurrentCheckBox =
                    GetDlgItem(hDlg, IDC_VERBOSE_CHECK);
                SendMessage(hCurrentCheckBox, BM_SETCHECK,
                    BST_CHECKED, 0);
            }
            if (tpcb->cachesize != 0) {
                SetDlgItemInt(hDlg, IDC_CACHE_EDIT,
                    tpcb->cachesize/1024, FALSE);
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_DONE_BUTTON) {
                hCurrentCheckBox =
                    GetDlgItem(hDlg, IDC_FASTMODE_CHECK);
                if(BST_CHECKED == SendMessage(hCurrentCheckBox,
                    BM_GETCHECK, NULL, NULL))
                    tpcb->fast_mode = 0;
                else
                    tpcb->fast_mode = 1;
                hCurrentCheckBox =
                    GetDlgItem(hDlg, IDC_VERBOSE_CHECK);
                if(BST_CHECKED == SendMessage(hCurrentCheckBox,
                    BM_GETCHECK, NULL, NULL))
                    tpcb->verbose = 1;
                else
                    tpcb->verbose = 0;
                currentInt = GetDlgItemInt(hDlg,
                    IDC_RANDOM_EDIT, &valid, FALSE);
                if (valid != FALSE)
                    tpcb->rand_seed = currentInt;
                currentInt = GetDlgItemInt(hDlg,
                    IDC_CACHE_EDIT, &valid, FALSE);
                if (valid != FALSE) {
                    if (currentInt < 20) {
                        MessageBox(hDlg,
                            L"Min cache size is 20kb.",
                            L"Error", MB_OK);
                        return FALSE;
                    }
                    tpcb->cachesize = currentInt*1024;
                }
                EndDialog(hDlg, LOWORD(wParam));
                DestroyWindow(hDlg);
            }
            break;
        default:
            return DefWindowProc(hDlg, message, wParam, lParam);
    }
    return TRUE;
}

// Utility function to retrieve the directory name
// from the control, and set it in the tpcb object.
// Optionally remove and create requested directory.
BOOL
GetHomeDirectory(HWND hDlg, BOOL init)
{
    wchar_t wdirname[MAX_PATH];
    DWORD attrs;

    if (GetDlgItemText(hDlg, IDC_HOME_EDIT, wdirname, MAX_PATH) == 0)
        tpcb->setHomeDir(TESTDIR);
    else
        tpcb->setHomeDirW(wdirname);

    if (init == TRUE) {
        // Ensure that wdirname holds the correct version:
        tpcb->getHomeDirW(wdirname, MAX_PATH);

        // If the directory exists, ensure that it is empty.
        attrs = GetFileAttributes(wdirname);
        if (attrs == FILE_ATTRIBUTE_DIRECTORY)
            RecursiveDirRemove(wdirname);
        else if (attrs == FILE_ATTRIBUTE_NORMAL)
            DeleteFile(wdirname);
        else if (attrs != 0xFFFFFFFF) {
            // Not a directory or normal file, don't try to remove
            // it, or create a new directory over the top.
            return FALSE;
        }

        // Create the requested directory.
        return CreateDirectory(wdirname, NULL);
    }
    return TRUE;
}

BOOL
RecursiveDirRemove(wchar_t *dirname)
{ 
    HANDLE hFind;  // file handle
    WIN32_FIND_DATA findFileData;

    wchar_t DirPath[MAX_PATH];
    wchar_t FileName[MAX_PATH];

    wcscpy(DirPath, dirname);
    wcscat(DirPath, L"\\*");    // searching all files
    wcscpy(FileName, dirname);
    wcscat(FileName, L"\\");

    MessageBox(hWndDlgMain, L"Cleaning directory.", L"Message", MB_OK);
    // find the first file
    if ((hFind = FindFirstFile(DirPath,&findFileData)) ==
        INVALID_HANDLE_VALUE)
        return FALSE;

    wcscpy(DirPath,FileName);
    MessageBox(hWndDlgMain, L"Found files in directory.",
        L"Message", MB_OK);

    bool bSearch = true;
    do {

        wcscpy(FileName + wcslen(DirPath), findFileData.cFileName);
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // we have found a directory, recurse
            if (!RecursiveDirRemove(FileName))
                break; // directory couldn't be deleted
        } else {
            if (findFileData.dwFileAttributes &
                FILE_ATTRIBUTE_READONLY)
                SetFileAttributes(findFileData.cFileName,
                    FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFile(FileName)) {
                MessageBox(hWndDlgMain, L"Delete failed.",
                    L"Message", MB_OK);
                break; // file couldn't be deleted
            }
        }
    } while (FindNextFile(hFind,&findFileData));

    FindClose(hFind);  // closing file handle
    return RemoveDirectory(dirname); // remove the empty directory
}

// Callback function used to receive error messages from DB
// Needs to have a C calling convention.
// Using this function, since the implementation is presenting
// the error to the user in a message box.
extern "C" {
void tpcb_errcallback(const DB_ENV *, const char *errpfx, const char *errstr)
{
    wchar_t wstr[ERR_STRING_MAX];
    memset(wstr, 0, sizeof(wstr));
    MultiByteToWideChar(CP_ACP, 0, errstr, strlen(errstr),
        wstr, ERR_STRING_MAX-1);
    MessageBox(hWndDlgMain, wstr, L"Error Message", MB_OK);
    exit(1);
}
}

