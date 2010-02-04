// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

void ThreadRequestProductDetails(void* parg);
void ThreadRandSendTest(void* parg);
bool GetStartOnSystemStartup();
void SetStartOnSystemStartup(bool fAutoStart);



DEFINE_EVENT_TYPE(wxEVT_UITHREADCALL)
DEFINE_EVENT_TYPE(wxEVT_REPLY1)
DEFINE_EVENT_TYPE(wxEVT_REPLY2)
DEFINE_EVENT_TYPE(wxEVT_REPLY3)

CMainFrame* pframeMain = NULL;
CMyTaskBarIcon* ptaskbaricon = NULL;
CServer* pserver = NULL;
map<string, string> mapAddressBook;
bool fRandSendTest = false;
void RandSend();
extern int g_isPainting;
bool fClosedToTray = false;

// Settings
int fShowGenerated = true;
int fMinimizeToTray = true;
int fMinimizeOnClose = true;







//////////////////////////////////////////////////////////////////////////////
//
// Util
//

void HandleCtrlA(wxKeyEvent& event)
{
    // Ctrl-a select all
    wxTextCtrl* textCtrl = (wxTextCtrl*)event.GetEventObject();
    if (event.GetModifiers() == wxMOD_CONTROL && event.GetKeyCode() == 'A')
        textCtrl->SetSelection(-1, -1);
    event.Skip();
}

bool Is24HourTime()
{
    //char pszHourFormat[256];
    //pszHourFormat[0] = '\0';
    //GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ITIME, pszHourFormat, 256);
    //return (pszHourFormat[0] != '0');
    return true;
}

string DateStr(int64 nTime)
{
    // Can only be used safely here in the UI
    return (string)wxDateTime((time_t)nTime).FormatDate();
}

string DateTimeStr(int64 nTime)
{
    // Can only be used safely here in the UI
    wxDateTime datetime((time_t)nTime);
    if (Is24HourTime())
        return (string)datetime.Format("%x %H:%M");
    else
        return (string)datetime.Format("%x ") + itostr((datetime.GetHour() + 11) % 12 + 1) + (string)datetime.Format(":%M %p");
}

wxString GetItemText(wxListCtrl* listCtrl, int nIndex, int nColumn)
{
    // Helper to simplify access to listctrl
    wxListItem item;
    item.m_itemId = nIndex;
    item.m_col = nColumn;
    item.m_mask = wxLIST_MASK_TEXT;
    if (!listCtrl->GetItem(item))
        return "";
    return item.GetText();
}

int InsertLine(wxListCtrl* listCtrl, const wxString& str0, const wxString& str1)
{
    int nIndex = listCtrl->InsertItem(listCtrl->GetItemCount(), str0);
    listCtrl->SetItem(nIndex, 1, str1);
    return nIndex;
}

int InsertLine(wxListCtrl* listCtrl, const wxString& str0, const wxString& str1, const wxString& str2, const wxString& str3, const wxString& str4)
{
    int nIndex = listCtrl->InsertItem(listCtrl->GetItemCount(), str0);
    listCtrl->SetItem(nIndex, 1, str1);
    listCtrl->SetItem(nIndex, 2, str2);
    listCtrl->SetItem(nIndex, 3, str3);
    listCtrl->SetItem(nIndex, 4, str4);
    return nIndex;
}

int InsertLine(wxListCtrl* listCtrl, void* pdata, const wxString& str0, const wxString& str1, const wxString& str2, const wxString& str3, const wxString& str4)
{
    int nIndex = listCtrl->InsertItem(listCtrl->GetItemCount(), str0);
    listCtrl->SetItemPtrData(nIndex, (wxUIntPtr)pdata);
    listCtrl->SetItem(nIndex, 1, str1);
    listCtrl->SetItem(nIndex, 2, str2);
    listCtrl->SetItem(nIndex, 3, str3);
    listCtrl->SetItem(nIndex, 4, str4);
    return nIndex;
}

void SetSelection(wxListCtrl* listCtrl, int nIndex)
{
    int nSize = listCtrl->GetItemCount();
    long nState = (wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
    for (int i = 0; i < nSize; i++)
        listCtrl->SetItemState(i, (i == nIndex ? nState : 0), nState);
}

int GetSelection(wxListCtrl* listCtrl)
{
    int nSize = listCtrl->GetItemCount();
    for (int i = 0; i < nSize; i++)
        if (listCtrl->GetItemState(i, wxLIST_STATE_FOCUSED))
            return i;
    return -1;
}

string HtmlEscape(const char* psz, bool fMultiLine=false)
{
    int len = 0;
    for (const char* p = psz; *p; p++)
    {
             if (*p == '<') len += 4;
        else if (*p == '>') len += 4;
        else if (*p == '&') len += 5;
        else if (*p == '"') len += 6;
        else if (*p == ' ' && p > psz && p[-1] == ' ' && p[1] == ' ') len += 6;
        else if (*p == '\n' && fMultiLine) len += 5;
        else
            len++;
    }
    string str;
    str.reserve(len);
    for (const char* p = psz; *p; p++)
    {
             if (*p == '<') str += "&lt;";
        else if (*p == '>') str += "&gt;";
        else if (*p == '&') str += "&amp;";
        else if (*p == '"') str += "&quot;";
        else if (*p == ' ' && p > psz && p[-1] == ' ' && p[1] == ' ') str += "&nbsp;";
        else if (*p == '\n' && fMultiLine) str += "<br>\n";
        else
            str += *p;
    }
    return str;
}

string HtmlEscape(const string& str, bool fMultiLine=false)
{
    return HtmlEscape(str.c_str(), fMultiLine);
}

void CalledMessageBox(const string& message, const string& caption, int style, wxWindow* parent, int x, int y, int* pnRet, bool* pfDone)
{
    *pnRet = wxMessageBox(message, caption, style, parent, x, y);
    *pfDone = true;
}

int ThreadSafeMessageBox(const string& message, const string& caption, int style, wxWindow* parent, int x, int y)
{
    if (mapArgs.count("-noui"))
        return wxOK;

#ifdef __WXMSW__
    return wxMessageBox(message, caption, style, parent, x, y);
#else
    if (wxThread::IsMain())
    {
        return wxMessageBox(message, caption, style, parent, x, y);
    }
    else
    {
        int nRet = 0;
        bool fDone = false;
        UIThreadCall(bind(CalledMessageBox, message, caption, style, parent, x, y, &nRet, &fDone));
        while (!fDone)
            Sleep(100);
        return nRet;
    }
#endif
}










//////////////////////////////////////////////////////////////////////////////
//
// Custom events
//
// If this code gets used again, it should be replaced with something like UIThreadCall

set<void*> setCallbackAvailable;
CCriticalSection cs_setCallbackAvailable;

void AddCallbackAvailable(void* p)
{
    CRITICAL_BLOCK(cs_setCallbackAvailable)
        setCallbackAvailable.insert(p);
}

void RemoveCallbackAvailable(void* p)
{
    CRITICAL_BLOCK(cs_setCallbackAvailable)
        setCallbackAvailable.erase(p);
}

bool IsCallbackAvailable(void* p)
{
    CRITICAL_BLOCK(cs_setCallbackAvailable)
        return setCallbackAvailable.count(p);
    return false;
}

template<typename T>
void AddPendingCustomEvent(wxEvtHandler* pevthandler, int nEventID, const T pbeginIn, const T pendIn)
{
    // Need to rewrite with something like UIThreadCall
    // I'm tired of maintaining this hack that's only called by unfinished unused code.
    assert(("Unimplemented", 0));
    //if (!pevthandler)
    //    return;
    //
    //const char* pbegin = (pendIn != pbeginIn) ? &pbeginIn[0] : NULL;
    //const char* pend = pbegin + (pendIn - pbeginIn) * sizeof(pbeginIn[0]);
    //wxCommandEvent event(nEventID);
    //wxString strData(wxChar(0), (pend - pbegin) / sizeof(wxChar) + 1);
    //memcpy(&strData[0], pbegin, pend - pbegin);
    //event.SetString(strData);
    //event.SetInt(pend - pbegin);
    //
    //pevthandler->AddPendingEvent(event);
}

template<class T>
void AddPendingCustomEvent(wxEvtHandler* pevthandler, int nEventID, const T& obj)
{
    CDataStream ss;
    ss << obj;
    AddPendingCustomEvent(pevthandler, nEventID, ss.begin(), ss.end());
}

void AddPendingReplyEvent1(void* pevthandler, CDataStream& vRecv)
{
    if (IsCallbackAvailable(pevthandler))
        AddPendingCustomEvent((wxEvtHandler*)pevthandler, wxEVT_REPLY1, vRecv.begin(), vRecv.end());
}

void AddPendingReplyEvent2(void* pevthandler, CDataStream& vRecv)
{
    if (IsCallbackAvailable(pevthandler))
        AddPendingCustomEvent((wxEvtHandler*)pevthandler, wxEVT_REPLY2, vRecv.begin(), vRecv.end());
}

void AddPendingReplyEvent3(void* pevthandler, CDataStream& vRecv)
{
    if (IsCallbackAvailable(pevthandler))
        AddPendingCustomEvent((wxEvtHandler*)pevthandler, wxEVT_REPLY3, vRecv.begin(), vRecv.end());
}

CDataStream GetStreamFromEvent(const wxCommandEvent& event)
{
    wxString strData = event.GetString();
    const char* pszBegin = strData.c_str();
    return CDataStream(pszBegin, pszBegin + event.GetInt(), SER_NETWORK);
}







//////////////////////////////////////////////////////////////////////////////
//
// CMainFrame
//

CMainFrame::CMainFrame(wxWindow* parent) : CMainFrameBase(parent)
{
    Connect(wxEVT_UITHREADCALL, wxCommandEventHandler(CMainFrame::OnUIThreadCall), NULL, this);

    // Init
    fRefreshListCtrl = false;
    fRefreshListCtrlRunning = false;
    fOnSetFocusAddress = false;
    fRefresh = false;
    m_choiceFilter->SetSelection(0);
    double dResize = 1.0;
#ifdef __WXMSW__
    SetIcon(wxICON(bitcoin));
#else
    SetIcon(bitcoin16_xpm);
    wxFont fontTmp = m_staticText41->GetFont();
    fontTmp.SetFamily(wxFONTFAMILY_TELETYPE);
    m_staticTextBalance->SetFont(fontTmp);
    m_staticTextBalance->SetSize(140, 17);
    // & underlines don't work on the toolbar buttons on gtk
    m_toolBar->ClearTools();
    m_toolBar->AddTool(wxID_BUTTONSEND, "Send Coins", wxBitmap(send20_xpm), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    m_toolBar->AddTool(wxID_BUTTONRECEIVE, "Address Book", wxBitmap(addressbook20_xpm), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    m_toolBar->Realize();
    // resize to fit ubuntu's huge default font
    dResize = 1.20;
    SetSize(dResize * GetSize().GetWidth(), 1.1 * GetSize().GetHeight());
#endif
    m_staticTextBalance->SetLabel(FormatMoney(GetBalance()) + "  ");
    m_listCtrl->SetFocus();
    ptaskbaricon = new CMyTaskBarIcon();

    // Init column headers
    int nDateWidth = DateTimeStr(1229413914).size() * 6 + 8;
    if (!strstr(DateTimeStr(1229413914).c_str(), "2008"))
        nDateWidth += 12;
    m_listCtrl->InsertColumn(0, "",             wxLIST_FORMAT_LEFT,  dResize * 0);
    m_listCtrl->InsertColumn(1, "",             wxLIST_FORMAT_LEFT,  dResize * 0);
    m_listCtrl->InsertColumn(2, "Status",       wxLIST_FORMAT_LEFT,  dResize * 110);
    m_listCtrl->InsertColumn(3, "Date",         wxLIST_FORMAT_LEFT,  dResize * nDateWidth);
    m_listCtrl->InsertColumn(4, "Description",  wxLIST_FORMAT_LEFT,  dResize * 409 - nDateWidth);
    m_listCtrl->InsertColumn(5, "Debit",        wxLIST_FORMAT_RIGHT, dResize * 79);
    m_listCtrl->InsertColumn(6, "Credit",       wxLIST_FORMAT_RIGHT, dResize * 79);

    //m_listCtrlProductsSent->InsertColumn(0, "Category",      wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlProductsSent->InsertColumn(1, "Title",         wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlProductsSent->InsertColumn(2, "Description",   wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlProductsSent->InsertColumn(3, "Price",         wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlProductsSent->InsertColumn(4, "",              wxLIST_FORMAT_LEFT,  100);

    //m_listCtrlOrdersSent->InsertColumn(0, "Time",          wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlOrdersSent->InsertColumn(1, "Price",         wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlOrdersSent->InsertColumn(2, "",              wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlOrdersSent->InsertColumn(3, "",              wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlOrdersSent->InsertColumn(4, "",              wxLIST_FORMAT_LEFT,  100);

    //m_listCtrlOrdersReceived->InsertColumn(0, "Time",            wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlOrdersReceived->InsertColumn(1, "Price",           wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlOrdersReceived->InsertColumn(2, "Payment Status",  wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlOrdersReceived->InsertColumn(3, "",                wxLIST_FORMAT_LEFT,  100);
    //m_listCtrlOrdersReceived->InsertColumn(4, "",                wxLIST_FORMAT_LEFT,  100);

    // Init status bar
    int pnWidths[3] = { -100, 88, 290 };
#ifndef __WXMSW__
    pnWidths[1] = pnWidths[1] * 1.1 * dResize;
    pnWidths[2] = pnWidths[2] * 1.1 * dResize;
#endif
    m_statusBar->SetFieldsCount(3, pnWidths);

    // Fill your address text box
    vector<unsigned char> vchPubKey;
    if (CWalletDB("r").ReadDefaultKey(vchPubKey))
        m_textCtrlAddress->SetValue(PubKeyToAddress(vchPubKey));

    // Fill listctrl with wallet transactions
    RefreshListCtrl();
}

CMainFrame::~CMainFrame()
{
    pframeMain = NULL;
    delete ptaskbaricon;
    ptaskbaricon = NULL;
    delete pserver;
    pserver = NULL;
}

void ExitTimeout(void* parg)
{
#ifdef __WXMSW__
    Sleep(5000);
    ExitProcess(0);
#endif
}

void Shutdown(void* parg)
{
    static CCriticalSection cs_Shutdown;
    static bool fTaken;
    bool fFirstThread;
    CRITICAL_BLOCK(cs_Shutdown)
    {
        fFirstThread = !fTaken;
        fTaken = true;
    }
    static bool fExit;
    if (fFirstThread)
    {
        fShutdown = true;
        nTransactionsUpdated++;
        DBFlush(false);
        StopNode();
        DBFlush(true);
        CreateThread(ExitTimeout, NULL);
        Sleep(10);
        printf("Bitcoin exiting\n\n");
        fExit = true;
        exit(0);
    }
    else
    {
        while (!fExit)
            Sleep(500);
        Sleep(100);
        ExitThread(0);
    }
}

void CMainFrame::OnClose(wxCloseEvent& event)
{
    if (fMinimizeOnClose && event.CanVeto() && !IsIconized())
    {
        // Divert close to minimize
        event.Veto();
        fClosedToTray = true;
        Iconize(true);
    }
    else
    {
        Destroy();
        CreateThread(Shutdown, NULL);
    }
}

void CMainFrame::OnIconize(wxIconizeEvent& event)
{
    // Hide the task bar button when minimized.
    // Event is sent when the frame is minimized or restored.
    // wxWidgets 2.8.9 doesn't have IsIconized() so there's no way
    // to get rid of the deprecated warning.  Just ignore it.
    if (!event.Iconized())
        fClosedToTray = false;
#ifndef __WXMSW__
    // Tray is not reliable on Linux gnome
    fClosedToTray = false;
#endif
    if (fMinimizeToTray && event.Iconized())
        fClosedToTray = true;
    Show(!fClosedToTray);
    ptaskbaricon->Show(fMinimizeToTray || fClosedToTray);
}

void CMainFrame::OnMouseEvents(wxMouseEvent& event)
{
    RandAddSeed();
    RAND_add(&event.m_x, sizeof(event.m_x), 0.25);
    RAND_add(&event.m_y, sizeof(event.m_y), 0.25);
}

void CMainFrame::OnListColBeginDrag(wxListEvent& event)
{
     // Hidden columns not resizeable
     if (event.GetColumn() <= 1 && !fDebug)
        event.Veto();
}

int CMainFrame::GetSortIndex(const string& strSort)
{
#ifdef __WXMSW__
    return 0;
#else
    // The wx generic listctrl implementation used on GTK doesn't sort,
    // so we have to do it ourselves.  Remember, we sort in reverse order.
    // In the wx generic implementation, they store the list of items
    // in a vector, so indexed lookups are fast, but inserts are slower
    // the closer they are to the top.
    int low = 0;
    int high = m_listCtrl->GetItemCount();
    while (low < high)
    {
        int mid = low + ((high - low) / 2);
        if (strSort.compare(m_listCtrl->GetItemText(mid).c_str()) >= 0)
            high = mid;
        else
            low = mid + 1;
    }
    return low;
#endif
}

void CMainFrame::InsertLine(bool fNew, int nIndex, uint256 hashKey, string strSort, const wxString& str2, const wxString& str3, const wxString& str4, const wxString& str5, const wxString& str6)
{
    string str0 = strSort;
    long nData = *(long*)&hashKey;

    // Find item
    if (!fNew && nIndex == -1)
    {
        while ((nIndex = m_listCtrl->FindItem(nIndex, nData)) != -1)
            if (GetItemText(m_listCtrl, nIndex, 1) == hashKey.ToString())
                break;
    }

    // fNew is for blind insert, only use if you're sure it's new
    if (fNew || nIndex == -1)
    {
        nIndex = m_listCtrl->InsertItem(GetSortIndex(strSort), str0);
    }
    else
    {
        // If sort key changed, must delete and reinsert to make it relocate
        if (GetItemText(m_listCtrl, nIndex, 0) != str0)
        {
            m_listCtrl->DeleteItem(nIndex);
            nIndex = m_listCtrl->InsertItem(GetSortIndex(strSort), str0);
        }
    }

    m_listCtrl->SetItem(nIndex, 1, hashKey.ToString());
    m_listCtrl->SetItem(nIndex, 2, str2);
    m_listCtrl->SetItem(nIndex, 3, str3);
    m_listCtrl->SetItem(nIndex, 4, str4);
    m_listCtrl->SetItem(nIndex, 5, str5);
    m_listCtrl->SetItem(nIndex, 6, str6);
    m_listCtrl->SetItemData(nIndex, nData);
}

bool CMainFrame::DeleteLine(uint256 hashKey)
{
    long nData = *(long*)&hashKey;

    // Find item
    int nIndex = -1;
    while ((nIndex = m_listCtrl->FindItem(nIndex, nData)) != -1)
        if (GetItemText(m_listCtrl, nIndex, 1) == hashKey.ToString())
            break;

    if (nIndex != -1)
        m_listCtrl->DeleteItem(nIndex);

    return nIndex != -1;
}

string FormatTxStatus(const CWalletTx& wtx)
{
    // Status
    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < 500000000)
            return strprintf("Open for %d blocks", nBestHeight - wtx.nLockTime);
        else
            return strprintf("Open until %s", DateTimeStr(wtx.nLockTime).c_str());
    }
    else
    {
        int nDepth = wtx.GetDepthInMainChain();
        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
            return strprintf("%d/offline?", nDepth);
        else if (nDepth < 6)
            return strprintf("%d/unconfirmed", nDepth);
        else
            return strprintf("%d confirmations", nDepth);
    }
}

string SingleLine(const string& strIn)
{
    string strOut;
    bool fOneSpace = false;
    foreach(int c, strIn)
    {
        if (isspace(c))
        {
            fOneSpace = true;
        }
        else if (c > ' ')
        {
            if (fOneSpace && !strOut.empty())
                strOut += ' ';
            strOut += c;
            fOneSpace = false;
        }
    }
    return strOut;
}

bool CMainFrame::InsertTransaction(const CWalletTx& wtx, bool fNew, int nIndex)
{
    int64 nTime = wtx.nTimeDisplayed = wtx.GetTxTime();
    int64 nCredit = wtx.GetCredit();
    int64 nDebit = wtx.GetDebit();
    int64 nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    string strStatus = FormatTxStatus(wtx);
    map<string, string> mapValue = wtx.mapValue;
    wtx.nLinesDisplayed = 1;
    nListViewUpdated++;

    // Filter
    if (wtx.IsCoinBase())
    {
        // Don't show generated coin until confirmed by at least one block after it
        // so we don't get the user's hopes up until it looks like it's probably accepted.
        //
        // It is not an error when generated blocks are not accepted.  By design,
        // some percentage of blocks, like 10% or more, will end up not accepted.
        // This is the normal mechanism by which the network copes with latency.
        //
        // We display regular transactions right away before any confirmation
        // because they can always get into some block eventually.  Generated coins
        // are special because if their block is not accepted, they are not valid.
        //
        if (wtx.GetDepthInMainChain() < 2)
        {
            wtx.nLinesDisplayed = 0;
            return false;
        }

        // View->Show Generated
        if (!fShowGenerated)
            return false;
    }

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    string strSort = strprintf("%010d-%01d-%010u",
        (pindex ? pindex->nHeight : INT_MAX),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived);

    // Insert line
    if (nNet > 0 || wtx.IsCoinBase())
    {
        //
        // Credit
        //
        string strDescription;

        if (wtx.IsCoinBase())
        {
            // Coinbase
            strDescription = "Generated";
            if (nCredit == 0)
            {
                int64 nUnmatured = 0;
                foreach(const CTxOut& txout, wtx.vout)
                    nUnmatured += txout.GetCredit();
                if (wtx.IsInMainChain())
                {
                    strDescription = strprintf("Generated (%s matures in %d more blocks)", FormatMoney(nUnmatured).c_str(), wtx.GetBlocksToMaturity());

                    // Check if the block was requested by anyone
                    if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                        strDescription = "Generated - Warning: This block was not received by any other nodes and will probably not be accepted!";
                }
                else
                {
                    strDescription = "Generated (not accepted)";
                }
            }
        }
        else if (!mapValue["from"].empty() || !mapValue["message"].empty())
        {
            // Online transaction
            if (!mapValue["from"].empty())
                strDescription += "From: " + mapValue["from"];
            if (!mapValue["message"].empty())
            {
                if (!strDescription.empty())
                    strDescription += " - ";
                strDescription += mapValue["message"];
            }
        }
        else
        {
            // Offline transaction
            foreach(const CTxOut& txout, wtx.vout)
            {
                if (txout.IsMine())
                {
                    vector<unsigned char> vchPubKey;
                    if (ExtractPubKey(txout.scriptPubKey, true, vchPubKey))
                    {
                        string strAddress = PubKeyToAddress(vchPubKey);
                        if (mapAddressBook.count(strAddress))
                        {
                            //strDescription += "Received payment to ";
                            //strDescription += "Received with address ";
                            strDescription += "From: unknown, To: ";
                            strDescription += strAddress;
                            /// The labeling feature is just too confusing, so I hid it
                            /// by putting it at the end where it runs off the screen.
                            /// It can still be seen by widening the column, or in the
                            /// details dialog.
                            if (!mapAddressBook[strAddress].empty())
                                strDescription += " (" + mapAddressBook[strAddress] + ")";
                        }
                    }
                    break;
                }
            }
        }

        InsertLine(fNew, nIndex, hash, strSort,
                   strStatus,
                   nTime ? DateTimeStr(nTime) : "",
                   SingleLine(strDescription),
                   "",
                   FormatMoney(nNet, true));
    }
    else
    {
        bool fAllFromMe = true;
        foreach(const CTxIn& txin, wtx.vin)
            fAllFromMe = fAllFromMe && txin.IsMine();

        bool fAllToMe = true;
        foreach(const CTxOut& txout, wtx.vout)
            fAllToMe = fAllToMe && txout.IsMine();

        if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            int64 nValue = wtx.vout[0].nValue;
            InsertLine(fNew, nIndex, hash, strSort,
                       strStatus,
                       nTime ? DateTimeStr(nTime) : "",
                       "Payment to yourself",
                       "",
                       "");
            /// issue: can't tell which is the payment and which is the change anymore
            //           FormatMoney(nNet - nValue, true),
            //           FormatMoney(nValue, true));
        }
        else if (fAllFromMe)
        {
            //
            // Debit
            //
            int64 nTxFee = nDebit - wtx.GetValueOut();
            wtx.nLinesDisplayed = 0;
            for (int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.vout[nOut];
                if (txout.IsMine())
                    continue;

                string strAddress;
                if (!mapValue["to"].empty())
                {
                    // Online transaction
                    strAddress = mapValue["to"];
                }
                else
                {
                    // Offline transaction
                    uint160 hash160;
                    if (ExtractHash160(txout.scriptPubKey, hash160))
                        strAddress = Hash160ToAddress(hash160);
                }

                string strDescription = "To: ";
                if (mapAddressBook.count(strAddress) && !mapAddressBook[strAddress].empty())
                    strDescription += mapAddressBook[strAddress] + " ";
                strDescription += strAddress;
                if (!mapValue["message"].empty())
                {
                    if (!strDescription.empty())
                        strDescription += " - ";
                    strDescription += mapValue["message"];
                }

                int64 nValue = txout.nValue;
                if (nOut == 0 && nTxFee > 0)
                    nValue += nTxFee;

                InsertLine(fNew, nIndex, hash, strprintf("%s-%d", strSort.c_str(), nOut),
                           strStatus,
                           nTime ? DateTimeStr(nTime) : "",
                           SingleLine(strDescription),
                           FormatMoney(-nValue, true),
                           "");
                wtx.nLinesDisplayed++;
            }
        }
        else
        {
            //
            // Mixed debit transaction, can't break down payees
            //
            bool fAllMine = true;
            foreach(const CTxOut& txout, wtx.vout)
                fAllMine = fAllMine && txout.IsMine();
            foreach(const CTxIn& txin, wtx.vin)
                fAllMine = fAllMine && txin.IsMine();

            InsertLine(fNew, nIndex, hash, strSort,
                       strStatus,
                       nTime ? DateTimeStr(nTime) : "",
                       "",
                       FormatMoney(nNet, true),
                       "");
        }
    }

    return true;
}

void CMainFrame::RefreshListCtrl()
{
    fRefreshListCtrl = true;
    ::wxWakeUpIdle();
}

void CMainFrame::OnIdle(wxIdleEvent& event)
{
    if (fRefreshListCtrl)
    {
        // Collect list of wallet transactions and sort newest first
        bool fEntered = false;
        vector<pair<unsigned int, uint256> > vSorted;
        TRY_CRITICAL_BLOCK(cs_mapWallet)
        {
            printf("RefreshListCtrl starting\n");
            fEntered = true;
            fRefreshListCtrl = false;
            vWalletUpdated.clear();

            // Do the newest transactions first
            vSorted.reserve(mapWallet.size());
            for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            {
                const CWalletTx& wtx = (*it).second;
                unsigned int nTime = UINT_MAX - wtx.GetTxTime();
                vSorted.push_back(make_pair(nTime, (*it).first));
            }
            m_listCtrl->DeleteAllItems();
        }
        if (!fEntered)
            return;

        sort(vSorted.begin(), vSorted.end());

        // Fill list control
        for (int i = 0; i < vSorted.size();)
        {
            if (fShutdown)
                return;
            bool fEntered = false;
            TRY_CRITICAL_BLOCK(cs_mapWallet)
            {
                fEntered = true;
                uint256& hash = vSorted[i++].second;
                map<uint256, CWalletTx>::iterator mi = mapWallet.find(hash);
                if (mi != mapWallet.end())
                    InsertTransaction((*mi).second, true);
            }
            if (!fEntered || i == 100 || i % 500 == 0)
                wxYield();
        }

        printf("RefreshListCtrl done\n");

        // Update transaction total display
        MainFrameRepaint();
    }
    else
    {
        // Check for time updates
        static int64 nLastTime;
        if (GetTime() > nLastTime + 30)
        {
            TRY_CRITICAL_BLOCK(cs_mapWallet)
            {
                nLastTime = GetTime();
                for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
                {
                    CWalletTx& wtx = (*it).second;
                    if (wtx.nTimeDisplayed && wtx.nTimeDisplayed != wtx.GetTxTime())
                        InsertTransaction(wtx, false);
                }
            }
        }
    }
}

void CMainFrame::RefreshStatusColumn()
{
    static int nLastTop;
    static CBlockIndex* pindexLastBest;
    static unsigned int nLastRefreshed;

    int nTop = max((int)m_listCtrl->GetTopItem(), 0);
    if (nTop == nLastTop && pindexLastBest == pindexBest)
        return;

    TRY_CRITICAL_BLOCK(cs_mapWallet)
    {
        int nStart = nTop;
        int nEnd = min(nStart + 100, m_listCtrl->GetItemCount());

        if (pindexLastBest == pindexBest && nLastRefreshed == nListViewUpdated)
        {
            // If no updates, only need to do the part that moved onto the screen
            if (nStart >= nLastTop && nStart < nLastTop + 100)
                nStart = nLastTop + 100;
            if (nEnd >= nLastTop && nEnd < nLastTop + 100)
                nEnd = nLastTop;
        }
        nLastTop = nTop;
        pindexLastBest = pindexBest;
        nLastRefreshed = nListViewUpdated;

        for (int nIndex = nStart; nIndex < min(nEnd, m_listCtrl->GetItemCount()); nIndex++)
        {
            uint256 hash((string)GetItemText(m_listCtrl, nIndex, 1));
            map<uint256, CWalletTx>::iterator mi = mapWallet.find(hash);
            if (mi == mapWallet.end())
            {
                printf("CMainFrame::RefreshStatusColumn() : tx not found in mapWallet\n");
                continue;
            }
            CWalletTx& wtx = (*mi).second;
            if (wtx.IsCoinBase() || wtx.GetTxTime() != wtx.nTimeDisplayed)
            {
                if (!InsertTransaction(wtx, false, nIndex))
                    m_listCtrl->DeleteItem(nIndex--);
            }
            else
                m_listCtrl->SetItem(nIndex, 2, FormatTxStatus(wtx));
        }
    }
}

void CMainFrame::OnPaint(wxPaintEvent& event)
{
    if (fRefresh)
    {
        fRefresh = false;
        Refresh();
    }
    event.Skip();
}


unsigned int nNeedRepaint = 0;
unsigned int nLastRepaint = 0;
int64 nLastRepaintTime = 0;
int64 nRepaintInterval = 500;

void ThreadDelayedRepaint(void* parg)
{
    while (!fShutdown)
    {
        if (nLastRepaint != nNeedRepaint && GetTimeMillis() - nLastRepaintTime >= nRepaintInterval)
        {
            nLastRepaint = nNeedRepaint;
            if (pframeMain)
            {
                printf("DelayedRepaint\n");
                wxPaintEvent event;
                pframeMain->fRefresh = true;
                pframeMain->GetEventHandler()->AddPendingEvent(event);
            }
        }
        Sleep(nRepaintInterval);
    }
}

void MainFrameRepaint()
{
    // This is called by network code that shouldn't access pframeMain
    // directly because it could still be running after the UI is closed.
    if (pframeMain)
    {
        // Don't repaint too often
        static int64 nLastRepaintRequest;
        if (GetTimeMillis() - nLastRepaintRequest < 100)
        {
            nNeedRepaint++;
            return;
        }
        nLastRepaintRequest = GetTimeMillis();

        printf("MainFrameRepaint\n");
        wxPaintEvent event;
        pframeMain->fRefresh = true;
        pframeMain->GetEventHandler()->AddPendingEvent(event);
    }
}

void CMainFrame::OnPaintListCtrl(wxPaintEvent& event)
{
    if (ptaskbaricon)
        ptaskbaricon->UpdateTooltip();

    //
    // Slower stuff
    //
    static int nTransactionCount;
    bool fPaintedBalance = false;
    if (GetTimeMillis() - nLastRepaintTime >= nRepaintInterval)
    {
        nLastRepaint = nNeedRepaint;
        nLastRepaintTime = GetTimeMillis();

        // Update listctrl contents
        if (!vWalletUpdated.empty())
        {
            TRY_CRITICAL_BLOCK(cs_mapWallet)
            {
                string strTop;
                if (m_listCtrl->GetItemCount())
                    strTop = (string)m_listCtrl->GetItemText(0);
                foreach(uint256 hash, vWalletUpdated)
                {
                    map<uint256, CWalletTx>::iterator mi = mapWallet.find(hash);
                    if (mi != mapWallet.end())
                        InsertTransaction((*mi).second, false);
                }
                vWalletUpdated.clear();
                if (m_listCtrl->GetItemCount() && strTop != (string)m_listCtrl->GetItemText(0))
                    m_listCtrl->ScrollList(0, INT_MIN/2);
            }
        }

        // Balance total
        TRY_CRITICAL_BLOCK(cs_mapWallet)
        {
            fPaintedBalance = true;
            m_staticTextBalance->SetLabel(FormatMoney(GetBalance()) + "  ");

            // Count hidden and multi-line transactions
            nTransactionCount = 0;
            for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            {
                CWalletTx& wtx = (*it).second;
                nTransactionCount += wtx.nLinesDisplayed;
            }
        }
    }
    if (!vWalletUpdated.empty() || !fPaintedBalance)
        nNeedRepaint++;

    // Update status column of visible items only
    RefreshStatusColumn();

    // Update status bar
    string strGen = "";
    if (fGenerateBitcoins)
        strGen = "    Generating";
    if (fGenerateBitcoins && vNodes.empty())
        strGen = "(not connected)";
    m_statusBar->SetStatusText(strGen, 1);

    string strStatus = strprintf("     %d connections     %d blocks     %d transactions", vNodes.size(), nBestHeight + 1, nTransactionCount);
    m_statusBar->SetStatusText(strStatus, 2);

    if (fDebug && GetTime() - nThreadSocketHandlerHeartbeat > 60)
        m_statusBar->SetStatusText("     ERROR: ThreadSocketHandler has stopped", 0);

    // Pass through to listctrl to actually do the paint, we're just hooking the message
    m_listCtrl->Disconnect(wxEVT_PAINT, (wxObjectEventFunction)NULL, NULL, this);
    m_listCtrl->GetEventHandler()->ProcessEvent(event);
    m_listCtrl->Connect(wxEVT_PAINT, wxPaintEventHandler(CMainFrame::OnPaintListCtrl), NULL, this);
}


void UIThreadCall(boost::function0<void> fn)
{
    // Call this with a function object created with bind.
    // bind needs all parameters to match the function's expected types
    // and all default parameters specified.  Some examples:
    //  UIThreadCall(bind(wxBell));
    //  UIThreadCall(bind(wxMessageBox, wxT("Message"), wxT("Title"), wxOK, (wxWindow*)NULL, -1, -1));
    //  UIThreadCall(bind(&CMainFrame::OnMenuHelpAbout, pframeMain, event));
    if (pframeMain)
    {
        wxCommandEvent event(wxEVT_UITHREADCALL);
        event.SetClientData((void*)new boost::function0<void>(fn));
        pframeMain->GetEventHandler()->AddPendingEvent(event);
    }
}

void CMainFrame::OnUIThreadCall(wxCommandEvent& event)
{
    boost::function0<void>* pfn = (boost::function0<void>*)event.GetClientData();
    (*pfn)();
    delete pfn;
}

void CMainFrame::OnMenuFileExit(wxCommandEvent& event)
{
    // File->Exit
    Close(true);
}

void CMainFrame::OnMenuViewShowGenerated(wxCommandEvent& event)
{
    // View->Show Generated
    fShowGenerated = event.IsChecked();
    CWalletDB().WriteSetting("fShowGenerated", fShowGenerated);
    RefreshListCtrl();
}

void CMainFrame::OnUpdateUIViewShowGenerated(wxUpdateUIEvent& event)
{
    event.Check(fShowGenerated);
}

void CMainFrame::OnMenuOptionsGenerate(wxCommandEvent& event)
{
    // Options->Generate Coins
    GenerateBitcoins(event.IsChecked());
}

void CMainFrame::OnUpdateUIOptionsGenerate(wxUpdateUIEvent& event)
{
    event.Check(fGenerateBitcoins);
}

void CMainFrame::OnMenuOptionsChangeYourAddress(wxCommandEvent& event)
{
    // Options->Change Your Address
    OnButtonChange(event);
}

void CMainFrame::OnMenuOptionsOptions(wxCommandEvent& event)
{
    // Options->Options
    COptionsDialog dialog(this);
    dialog.ShowModal();
}

void CMainFrame::OnMenuHelpAbout(wxCommandEvent& event)
{
    // Help->About
    CAboutDialog dialog(this);
    dialog.ShowModal();
}

void CMainFrame::OnButtonSend(wxCommandEvent& event)
{
    // Toolbar: Send
    CSendDialog dialog(this);
    dialog.ShowModal();
}

void CMainFrame::OnButtonAddressBook(wxCommandEvent& event)
{
    // Toolbar: Address Book
    CAddressBookDialog dialogAddr(this, "", false);
    if (dialogAddr.ShowModal() == 2)
    {
        // Send
        CSendDialog dialogSend(this, dialogAddr.GetAddress());
        dialogSend.ShowModal();
    }
}

void CMainFrame::OnSetFocusAddress(wxFocusEvent& event)
{
    // Automatically select-all when entering window
    m_textCtrlAddress->SetSelection(-1, -1);
    fOnSetFocusAddress = true;
    event.Skip();
}

void CMainFrame::OnMouseEventsAddress(wxMouseEvent& event)
{
    if (fOnSetFocusAddress)
        m_textCtrlAddress->SetSelection(-1, -1);
    fOnSetFocusAddress = false;
    event.Skip();
}

void CMainFrame::OnButtonCopy(wxCommandEvent& event)
{
    // Copy address box to clipboard
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(m_textCtrlAddress->GetValue()));
        wxTheClipboard->Close();
    }
}

void CMainFrame::OnButtonChange(wxCommandEvent& event)
{
    CYourAddressDialog dialog(this, string(m_textCtrlAddress->GetValue()));
    if (!dialog.ShowModal())
        return;
    string strAddress = (string)dialog.GetAddress();
    if (strAddress != m_textCtrlAddress->GetValue())
    {
        uint160 hash160;
        if (!AddressToHash160(strAddress, hash160))
            return;
        if (!mapPubKeys.count(hash160))
            return;
        CWalletDB().WriteDefaultKey(mapPubKeys[hash160]);
        m_textCtrlAddress->SetValue(strAddress);
    }
}

void CMainFrame::OnListItemActivated(wxListEvent& event)
{
    uint256 hash((string)GetItemText(m_listCtrl, event.GetIndex(), 1));
    CWalletTx wtx;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(hash);
        if (mi == mapWallet.end())
        {
            printf("CMainFrame::OnListItemActivated() : tx not found in mapWallet\n");
            return;
        }
        wtx = (*mi).second;
    }
    CTxDetailsDialog dialog(this, wtx);
    dialog.ShowModal();
    //CTxDetailsDialog* pdialog = new CTxDetailsDialog(this, wtx);
    //pdialog->Show();
}

void CMainFrame::OnListItemActivatedProductsSent(wxListEvent& event)
{
    CProduct& product = *(CProduct*)event.GetItem().GetData();
    CEditProductDialog* pdialog = new CEditProductDialog(this);
    pdialog->SetProduct(product);
    pdialog->Show();
}

void CMainFrame::OnListItemActivatedOrdersSent(wxListEvent& event)
{
    CWalletTx& order = *(CWalletTx*)event.GetItem().GetData();
    CViewOrderDialog* pdialog = new CViewOrderDialog(this, order, false);
    pdialog->Show();
}

void CMainFrame::OnListItemActivatedOrdersReceived(wxListEvent& event)
{
    CWalletTx& order = *(CWalletTx*)event.GetItem().GetData();
    CViewOrderDialog* pdialog = new CViewOrderDialog(this, order, true);
    pdialog->Show();
}







//////////////////////////////////////////////////////////////////////////////
//
// CTxDetailsDialog
//

CTxDetailsDialog::CTxDetailsDialog(wxWindow* parent, CWalletTx wtx) : CTxDetailsDialogBase(parent)
{
    string strHTML;
    strHTML.reserve(4000);
    strHTML += "<html><font face='verdana, arial, helvetica, sans-serif'>";

    int64 nTime = wtx.GetTxTime();
    int64 nCredit = wtx.GetCredit();
    int64 nDebit = wtx.GetDebit();
    int64 nNet = nCredit - nDebit;



    strHTML += "<b>Status:</b> " + FormatTxStatus(wtx);
    int nRequests = wtx.GetRequestCount();
    if (nRequests != -1)
    {
        if (nRequests == 0)
            strHTML += ", has not been successfully broadcast yet";
        else if (nRequests == 1)
            strHTML += strprintf(", broadcast through %d node", nRequests);
        else
            strHTML += strprintf(", broadcast through %d nodes", nRequests);
    }
    strHTML += "<br>";

    strHTML += "<b>Date:</b> " + (nTime ? DateTimeStr(nTime) : "") + "<br>";


    //
    // From
    //
    if (wtx.IsCoinBase())
    {
        strHTML += "<b>Source:</b> Generated<br>";
    }
    else if (!wtx.mapValue["from"].empty())
    {
        // Online transaction
        if (!wtx.mapValue["from"].empty())
            strHTML += "<b>From:</b> " + HtmlEscape(wtx.mapValue["from"]) + "<br>";
    }
    else
    {
        // Offline transaction
        if (nNet > 0)
        {
            // Credit
            foreach(const CTxOut& txout, wtx.vout)
            {
                if (txout.IsMine())
                {
                    vector<unsigned char> vchPubKey;
                    if (ExtractPubKey(txout.scriptPubKey, true, vchPubKey))
                    {
                        string strAddress = PubKeyToAddress(vchPubKey);
                        if (mapAddressBook.count(strAddress))
                        {
                            strHTML += "<b>From:</b> unknown<br>";
                            strHTML += "<b>To:</b> ";
                            strHTML += HtmlEscape(strAddress);
                            if (!mapAddressBook[strAddress].empty())
                                strHTML += " (yours, label: " + mapAddressBook[strAddress] + ")";
                            else
                                strHTML += " (yours)";
                            strHTML += "<br>";
                        }
                    }
                    break;
                }
            }
        }
    }


    //
    // To
    //
    string strAddress;
    if (!wtx.mapValue["to"].empty())
    {
        // Online transaction
        strAddress = wtx.mapValue["to"];
        strHTML += "<b>To:</b> ";
        if (mapAddressBook.count(strAddress) && !mapAddressBook[strAddress].empty())
            strHTML += mapAddressBook[strAddress] + " ";
        strHTML += HtmlEscape(strAddress) + "<br>";
    }


    //
    // Amount
    //
    if (wtx.IsCoinBase() && nCredit == 0)
    {
        //
        // Coinbase
        //
        int64 nUnmatured = 0;
        foreach(const CTxOut& txout, wtx.vout)
            nUnmatured += txout.GetCredit();
        if (wtx.IsInMainChain())
            strHTML += strprintf("<b>Credit:</b> (%s matures in %d more blocks)<br>", FormatMoney(nUnmatured).c_str(), wtx.GetBlocksToMaturity());
        else
            strHTML += "<b>Credit:</b> (not accepted)<br>";
    }
    else if (nNet > 0)
    {
        //
        // Credit
        //
        strHTML += "<b>Credit:</b> " + FormatMoney(nNet) + "<br>";
    }
    else
    {
        bool fAllFromMe = true;
        foreach(const CTxIn& txin, wtx.vin)
            fAllFromMe = fAllFromMe && txin.IsMine();

        bool fAllToMe = true;
        foreach(const CTxOut& txout, wtx.vout)
            fAllToMe = fAllToMe && txout.IsMine();

        if (fAllFromMe)
        {
            //
            // Debit
            //
            foreach(const CTxOut& txout, wtx.vout)
            {
                if (txout.IsMine())
                    continue;

                if (wtx.mapValue["to"].empty())
                {
                    // Offline transaction
                    uint160 hash160;
                    if (ExtractHash160(txout.scriptPubKey, hash160))
                    {
                        string strAddress = Hash160ToAddress(hash160);
                        strHTML += "<b>To:</b> ";
                        if (mapAddressBook.count(strAddress) && !mapAddressBook[strAddress].empty())
                            strHTML += mapAddressBook[strAddress] + " ";
                        strHTML += strAddress;
                        strHTML += "<br>";
                    }
                }

                strHTML += "<b>Debit:</b> " + FormatMoney(-txout.nValue) + "<br>";
            }

            if (fAllToMe)
            {
                // Payment to self
                /// issue: can't tell which is the payment and which is the change anymore
                //int64 nValue = wtx.vout[0].nValue;
                //strHTML += "<b>Debit:</b> " + FormatMoney(-nValue) + "<br>";
                //strHTML += "<b>Credit:</b> " + FormatMoney(nValue) + "<br>";
            }

            int64 nTxFee = nDebit - wtx.GetValueOut();
            if (nTxFee > 0)
                strHTML += "<b>Transaction fee:</b> " + FormatMoney(-nTxFee) + "<br>";
        }
        else
        {
            //
            // Mixed debit transaction
            //
            foreach(const CTxIn& txin, wtx.vin)
                if (txin.IsMine())
                    strHTML += "<b>Debit:</b> " + FormatMoney(-txin.GetDebit()) + "<br>";
            foreach(const CTxOut& txout, wtx.vout)
                if (txout.IsMine())
                    strHTML += "<b>Credit:</b> " + FormatMoney(txout.GetCredit()) + "<br>";
        }
    }

    strHTML += "<b>Net amount:</b> " + FormatMoney(nNet, true) + "<br>";


    //
    // Message
    //
    if (!wtx.mapValue["message"].empty())
        strHTML += "<br><b>Message:</b><br>" + HtmlEscape(wtx.mapValue["message"], true) + "<br>";

    if (wtx.IsCoinBase())
        strHTML += "<br>Generated coins must wait 120 blocks before they can be spent.  When you generated this block, it was broadcast to the network to be added to the block chain.  If it fails to get into the chain, it will change to \"not accepted\" and not be spendable.  This may occasionally happen if another node generates a block within a few seconds of yours.<br>";


    //
    // Debug view
    //
    if (fDebug)
    {
        strHTML += "<hr><br>debug print<br><br>";
        foreach(const CTxIn& txin, wtx.vin)
            if (txin.IsMine())
                strHTML += "<b>Debit:</b> " + FormatMoney(-txin.GetDebit()) + "<br>";
        foreach(const CTxOut& txout, wtx.vout)
            if (txout.IsMine())
                strHTML += "<b>Credit:</b> " + FormatMoney(txout.GetCredit()) + "<br>";

        strHTML += "<b>Inputs:</b><br>";
        CRITICAL_BLOCK(cs_mapWallet)
        {
            foreach(const CTxIn& txin, wtx.vin)
            {
                COutPoint prevout = txin.prevout;
                map<uint256, CWalletTx>::iterator mi = mapWallet.find(prevout.hash);
                if (mi != mapWallet.end())
                {
                    const CWalletTx& prev = (*mi).second;
                    if (prevout.n < prev.vout.size())
                    {
                        strHTML += HtmlEscape(prev.ToString(), true);
                        strHTML += " &nbsp;&nbsp; " + FormatTxStatus(prev) + ", ";
                        strHTML = strHTML + "IsMine=" + (prev.vout[prevout.n].IsMine() ? "true" : "false") + "<br>";
                    }
                }
            }
        }

        strHTML += "<br><hr><br><b>Transaction:</b><br>";
        strHTML += HtmlEscape(wtx.ToString(), true);
    }



    strHTML += "</font></html>";
    string(strHTML.begin(), strHTML.end()).swap(strHTML);
    m_htmlWin->SetPage(strHTML);
    m_buttonOK->SetFocus();
}

void CTxDetailsDialog::OnButtonOK(wxCommandEvent& event)
{
    Close();
    //Destroy();
}





//////////////////////////////////////////////////////////////////////////////
//
// COptionsDialog
//

COptionsDialog::COptionsDialog(wxWindow* parent) : COptionsDialogBase(parent)
{
    // Set up list box of page choices
    m_listBox->Append("Main");
    //m_listBox->Append("Test 2");
    m_listBox->SetSelection(0);
    SelectPage(0);
#ifndef __WXMSW__
    m_checkBoxMinimizeOnClose->SetLabel("&Minimize on close");
    m_checkBoxStartOnSystemStartup->Enable(false); // not implemented yet
#endif

    // Init values
    m_textCtrlTransactionFee->SetValue(FormatMoney(nTransactionFee));
    m_checkBoxLimitProcessors->SetValue(fLimitProcessors);
    m_spinCtrlLimitProcessors->Enable(fLimitProcessors);
    m_spinCtrlLimitProcessors->SetValue(nLimitProcessors);
    int nProcessors = wxThread::GetCPUCount();
    if (nProcessors < 1)
        nProcessors = 999;
    m_spinCtrlLimitProcessors->SetRange(1, nProcessors);
    m_checkBoxStartOnSystemStartup->SetValue(fTmpStartOnSystemStartup = GetStartOnSystemStartup());
    m_checkBoxMinimizeToTray->SetValue(fMinimizeToTray);
    m_checkBoxMinimizeOnClose->SetValue(fMinimizeOnClose);
    m_checkBoxUseProxy->SetValue(fUseProxy);
    m_textCtrlProxyIP->Enable(fUseProxy);
    m_textCtrlProxyPort->Enable(fUseProxy);
    m_staticTextProxyIP->Enable(fUseProxy);
    m_staticTextProxyPort->Enable(fUseProxy);
    m_textCtrlProxyIP->SetValue(addrProxy.ToStringIP());
    m_textCtrlProxyPort->SetValue(addrProxy.ToStringPort());

    m_buttonOK->SetFocus();
}

void COptionsDialog::SelectPage(int nPage)
{
    m_panelMain->Show(nPage == 0);
    m_panelTest2->Show(nPage == 1);

    m_scrolledWindow->Layout();
    m_scrolledWindow->SetScrollbars(0, 0, 0, 0, 0, 0);
}

void COptionsDialog::OnListBox(wxCommandEvent& event)
{
    SelectPage(event.GetSelection());
}

void COptionsDialog::OnKillFocusTransactionFee(wxFocusEvent& event)
{
    int64 nTmp = nTransactionFee;
    ParseMoney(m_textCtrlTransactionFee->GetValue(), nTmp);
    m_textCtrlTransactionFee->SetValue(FormatMoney(nTmp));
}

void COptionsDialog::OnCheckBoxLimitProcessors(wxCommandEvent& event)
{
    m_spinCtrlLimitProcessors->Enable(event.IsChecked());
}

void COptionsDialog::OnCheckBoxUseProxy(wxCommandEvent& event)
{
    m_textCtrlProxyIP->Enable(event.IsChecked());
    m_textCtrlProxyPort->Enable(event.IsChecked());
    m_staticTextProxyIP->Enable(event.IsChecked());
    m_staticTextProxyPort->Enable(event.IsChecked());
}

CAddress COptionsDialog::GetProxyAddr()
{
    // Be careful about byte order, addr.ip and addr.port are big endian
    CAddress addr(m_textCtrlProxyIP->GetValue() + ":" + m_textCtrlProxyPort->GetValue());
    if (addr.ip == INADDR_NONE)
        addr.ip = addrProxy.ip;
    int nPort = atoi(m_textCtrlProxyPort->GetValue());
    addr.port = htons(nPort);
    if (nPort <= 0 || nPort > USHRT_MAX)
        addr.port = addrProxy.port;
    return addr;
}

void COptionsDialog::OnKillFocusProxy(wxFocusEvent& event)
{
    m_textCtrlProxyIP->SetValue(GetProxyAddr().ToStringIP());
    m_textCtrlProxyPort->SetValue(GetProxyAddr().ToStringPort());
}


void COptionsDialog::OnButtonOK(wxCommandEvent& event)
{
    OnButtonApply(event);
    Close();
}

void COptionsDialog::OnButtonCancel(wxCommandEvent& event)
{
    Close();
}

void COptionsDialog::OnButtonApply(wxCommandEvent& event)
{
    CWalletDB walletdb;

    int64 nPrevTransactionFee = nTransactionFee;
    if (ParseMoney(m_textCtrlTransactionFee->GetValue(), nTransactionFee) && nTransactionFee != nPrevTransactionFee)
        walletdb.WriteSetting("nTransactionFee", nTransactionFee);

    int nPrevMaxProc = (fLimitProcessors ? nLimitProcessors : INT_MAX);
    if (fLimitProcessors != m_checkBoxLimitProcessors->GetValue())
    {
        fLimitProcessors = m_checkBoxLimitProcessors->GetValue();
        walletdb.WriteSetting("fLimitProcessors", fLimitProcessors);
    }
    if (nLimitProcessors != m_spinCtrlLimitProcessors->GetValue())
    {
        nLimitProcessors = m_spinCtrlLimitProcessors->GetValue();
        walletdb.WriteSetting("nLimitProcessors", nLimitProcessors);
    }
    if (fGenerateBitcoins && (fLimitProcessors ? nLimitProcessors : INT_MAX) > nPrevMaxProc)
        GenerateBitcoins(fGenerateBitcoins);

    if (fTmpStartOnSystemStartup != m_checkBoxStartOnSystemStartup->GetValue())
    {
        fTmpStartOnSystemStartup = m_checkBoxStartOnSystemStartup->GetValue();
        SetStartOnSystemStartup(fTmpStartOnSystemStartup);
    }

    if (fMinimizeToTray != m_checkBoxMinimizeToTray->GetValue())
    {
        fMinimizeToTray = m_checkBoxMinimizeToTray->GetValue();
        walletdb.WriteSetting("fMinimizeToTray", fMinimizeToTray);
        ptaskbaricon->Show(fMinimizeToTray || fClosedToTray);
    }

    if (fMinimizeOnClose != m_checkBoxMinimizeOnClose->GetValue())
    {
        fMinimizeOnClose = m_checkBoxMinimizeOnClose->GetValue();
        walletdb.WriteSetting("fMinimizeOnClose", fMinimizeOnClose);
    }

    fUseProxy = m_checkBoxUseProxy->GetValue();
    walletdb.WriteSetting("fUseProxy", fUseProxy);

    addrProxy = GetProxyAddr();
    walletdb.WriteSetting("addrProxy", addrProxy);
}






//////////////////////////////////////////////////////////////////////////////
//
// CAboutDialog
//

CAboutDialog::CAboutDialog(wxWindow* parent) : CAboutDialogBase(parent)
{
    m_staticTextVersion->SetLabel(strprintf("version 0.%d.%d beta", VERSION/100, VERSION%100));

#if !wxUSE_UNICODE
    // Workaround until upgrade to wxWidgets supporting UTF-8
    wxString str = m_staticTextMain->GetLabel();
    if (str.Find('�') != wxNOT_FOUND)
        str.Remove(str.Find('�'), 1);
    m_staticTextMain->SetLabel(str);
#endif
#ifndef __WXMSW__
    // Resize on Linux to make the window fit the text.
    // The text was wrapped manually rather than using the Wrap setting because
    // the wrap would be too small on Linux and it can't be changed at this point.
    wxFont fontTmp = m_staticTextMain->GetFont();
    if (fontTmp.GetPointSize() > 8);
        fontTmp.SetPointSize(8);
    m_staticTextMain->SetFont(fontTmp);
    SetSize(GetSize().GetWidth() + 44, GetSize().GetHeight() - 4);
#endif
}

void CAboutDialog::OnButtonOK(wxCommandEvent& event)
{
    Close();
}






//////////////////////////////////////////////////////////////////////////////
//
// CSendDialog
//

CSendDialog::CSendDialog(wxWindow* parent, const wxString& strAddress) : CSendDialogBase(parent)
{
    // Init
    m_textCtrlAddress->SetValue(strAddress);
    m_choiceTransferType->SetSelection(0);
    m_bitmapCheckMark->Show(false);
    fEnabledPrev = true;
    m_textCtrlAddress->SetFocus();
    //// todo: should add a display of your balance for convenience
#ifndef __WXMSW__
    wxFont fontTmp = m_staticTextInstructions->GetFont();
    if (fontTmp.GetPointSize() > 9);
        fontTmp.SetPointSize(9);
    m_staticTextInstructions->SetFont(fontTmp);
    SetSize(725, 380);
#endif

    // Set Icon
    wxIcon iconSend;
    iconSend.CopyFromBitmap(wxBitmap(send16noshadow_xpm));
    SetIcon(iconSend);

    wxCommandEvent event;
    OnTextAddress(event);

    // Fixup the tab order
    m_buttonPaste->MoveAfterInTabOrder(m_buttonCancel);
    m_buttonAddress->MoveAfterInTabOrder(m_buttonPaste);
    this->Layout();
}

void CSendDialog::OnTextAddress(wxCommandEvent& event)
{
    // Check mark
    bool fBitcoinAddress = IsValidBitcoinAddress(m_textCtrlAddress->GetValue());
    m_bitmapCheckMark->Show(fBitcoinAddress);

    // Grey out message if bitcoin address
    bool fEnable = !fBitcoinAddress;
    m_staticTextFrom->Enable(fEnable);
    m_textCtrlFrom->Enable(fEnable);
    m_staticTextMessage->Enable(fEnable);
    m_textCtrlMessage->Enable(fEnable);
    m_textCtrlMessage->SetBackgroundColour(wxSystemSettings::GetColour(fEnable ? wxSYS_COLOUR_WINDOW : wxSYS_COLOUR_BTNFACE));
    if (!fEnable && fEnabledPrev)
    {
        strFromSave    = m_textCtrlFrom->GetValue();
        strMessageSave = m_textCtrlMessage->GetValue();
        m_textCtrlFrom->SetValue("Will appear as \"From: Unknown\"");
        m_textCtrlMessage->SetValue("Can't include a message when sending to a Bitcoin address");
    }
    else if (fEnable && !fEnabledPrev)
    {
        m_textCtrlFrom->SetValue(strFromSave);
        m_textCtrlMessage->SetValue(strMessageSave);
    }
    fEnabledPrev = fEnable;
}

void CSendDialog::OnKillFocusAmount(wxFocusEvent& event)
{
    // Reformat the amount
    if (m_textCtrlAmount->GetValue().Trim().empty())
        return;
    int64 nTmp;
    if (ParseMoney(m_textCtrlAmount->GetValue(), nTmp))
        m_textCtrlAmount->SetValue(FormatMoney(nTmp));
}

void CSendDialog::OnButtonAddressBook(wxCommandEvent& event)
{
    // Open address book
    CAddressBookDialog dialog(this, m_textCtrlAddress->GetValue(), true);
    if (dialog.ShowModal())
        m_textCtrlAddress->SetValue(dialog.GetAddress());
}

void CSendDialog::OnButtonPaste(wxCommandEvent& event)
{
    // Copy clipboard to address box
    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported(wxDF_TEXT))
        {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            m_textCtrlAddress->SetValue(data.GetText());
        }
        wxTheClipboard->Close();
    }
}

void CSendDialog::OnButtonSend(wxCommandEvent& event)
{
    CWalletTx wtx;
    string strAddress = (string)m_textCtrlAddress->GetValue();

    // Parse amount
    int64 nValue = 0;
    if (!ParseMoney(m_textCtrlAmount->GetValue(), nValue) || nValue <= 0)
    {
        wxMessageBox("Error in amount  ", "Send Coins");
        return;
    }
    if (nValue > GetBalance())
    {
        wxMessageBox("Amount exceeds your balance  ", "Send Coins");
        return;
    }
    if (nValue + nTransactionFee > GetBalance())
    {
        wxMessageBox(string("Total exceeds your balance when the ") + FormatMoney(nTransactionFee) + " transaction fee is included  ", "Send Coins");
        return;
    }

    // Parse bitcoin address
    uint160 hash160;
    bool fBitcoinAddress = AddressToHash160(strAddress, hash160);

    if (fBitcoinAddress)
    {
        // Send to bitcoin address
        CScript scriptPubKey;
        scriptPubKey << OP_DUP << OP_HASH160 << hash160 << OP_EQUALVERIFY << OP_CHECKSIG;

        if (!SendMoney(scriptPubKey, nValue, wtx))
            return;

        wxMessageBox("Payment sent  ", "Sending...");
    }
    else
    {
        // Parse IP address
        CAddress addr(strAddress);
        if (!addr.IsValid())
        {
            wxMessageBox("Invalid address  ", "Send Coins");
            return;
        }

        // Message
        wtx.mapValue["to"] = strAddress;
        wtx.mapValue["from"] = m_textCtrlFrom->GetValue();
        wtx.mapValue["message"] = m_textCtrlMessage->GetValue();

        // Send to IP address
        CSendingDialog* pdialog = new CSendingDialog(this, addr, nValue, wtx);
        if (!pdialog->ShowModal())
            return;
    }

    if (!mapAddressBook.count(strAddress))
        SetAddressBookName(strAddress, "");

    EndModal(true);
}

void CSendDialog::OnButtonCancel(wxCommandEvent& event)
{
    // Cancel
    EndModal(false);
}






//////////////////////////////////////////////////////////////////////////////
//
// CSendingDialog
//

CSendingDialog::CSendingDialog(wxWindow* parent, const CAddress& addrIn, int64 nPriceIn, const CWalletTx& wtxIn) : CSendingDialogBase(NULL) // we have to give null so parent can't destroy us
{
    addr = addrIn;
    nPrice = nPriceIn;
    wtx = wtxIn;
    start = wxDateTime::UNow();
    memset(pszStatus, 0, sizeof(pszStatus));
    fCanCancel = true;
    fAbort = false;
    fSuccess = false;
    fUIDone = false;
    fWorkDone = false;
#ifndef __WXMSW__
    SetSize(1.2 * GetSize().GetWidth(), 1.05 * GetSize().GetHeight());
#endif

    SetTitle(strprintf("Sending %s to %s", FormatMoney(nPrice).c_str(), wtx.mapValue["to"].c_str()));
    m_textCtrlStatus->SetValue("");

    CreateThread(SendingDialogStartTransfer, this);
}

CSendingDialog::~CSendingDialog()
{
    printf("~CSendingDialog()\n");
}

void CSendingDialog::Close()
{
    // Last one out turn out the lights.
    // fWorkDone signals that work side is done and UI thread should call destroy.
    // fUIDone signals that UI window has closed and work thread should call destroy.
    // This allows the window to disappear and end modality when cancelled
    // without making the user wait for ConnectNode to return.  The dialog object
    // hangs around in the background until the work thread exits.
    if (IsModal())
        EndModal(fSuccess);
    else
        Show(false);
    if (fWorkDone)
        Destroy();
    else
        fUIDone = true;
}

void CSendingDialog::OnClose(wxCloseEvent& event)
{
    if (!event.CanVeto() || fWorkDone || fAbort || !fCanCancel)
    {
        Close();
    }
    else
    {
        event.Veto();
        wxCommandEvent cmdevent;
        OnButtonCancel(cmdevent);
    }
}

void CSendingDialog::OnButtonOK(wxCommandEvent& event)
{
    if (fWorkDone)
        Close();
}

void CSendingDialog::OnButtonCancel(wxCommandEvent& event)
{
    if (fCanCancel)
        fAbort = true;
}

void CSendingDialog::OnPaint(wxPaintEvent& event)
{
    if (strlen(pszStatus) > 130)
        m_textCtrlStatus->SetValue(string("\n") + pszStatus);
    else
        m_textCtrlStatus->SetValue(string("\n\n") + pszStatus);
    m_staticTextSending->SetFocus();
    if (!fCanCancel)
        m_buttonCancel->Enable(false);
    if (fWorkDone)
    {
        m_buttonOK->Enable(true);
        m_buttonOK->SetFocus();
        m_buttonCancel->Enable(false);
    }
    if (fAbort && fCanCancel && IsShown())
    {
        strcpy(pszStatus, "CANCELLED");
        m_buttonOK->Enable(true);
        m_buttonOK->SetFocus();
        m_buttonCancel->Enable(false);
        m_buttonCancel->SetLabel("Cancelled");
        Close();
        wxMessageBox("Transfer cancelled  ", "Sending...", wxOK, this);
    }
    event.Skip();
}


//
// Everything from here on is not in the UI thread and must only communicate
// with the rest of the dialog through variables and calling repaint.
//

void CSendingDialog::Repaint()
{
    Refresh();
    wxPaintEvent event;
    GetEventHandler()->AddPendingEvent(event);
}

bool CSendingDialog::Status()
{
    if (fUIDone)
    {
        Destroy();
        return false;
    }
    if (fAbort && fCanCancel)
    {
        memset(pszStatus, 0, 10);
        strcpy(pszStatus, "CANCELLED");
        Repaint();
        fWorkDone = true;
        return false;
    }
    return true;
}

bool CSendingDialog::Status(const string& str)
{
    if (!Status())
        return false;

    // This can be read by the UI thread at any time,
    // so copy in a way that can be read cleanly at all times.
    memset(pszStatus, 0, min(str.size()+1, sizeof(pszStatus)));
    strlcpy(pszStatus, str.c_str(), sizeof(pszStatus));

    Repaint();
    return true;
}

bool CSendingDialog::Error(const string& str)
{
    fCanCancel = false;
    fWorkDone = true;
    Status(string("Error: ") + str);
    return false;
}

void SendingDialogStartTransfer(void* parg)
{
    ((CSendingDialog*)parg)->StartTransfer();
}

void CSendingDialog::StartTransfer()
{
    // Make sure we have enough money
    if (nPrice + nTransactionFee > GetBalance())
    {
        Error("You don't have enough money");
        return;
    }

    // We may have connected already for product details
    if (!Status("Connecting..."))
        return;
    CNode* pnode = ConnectNode(addr, 15 * 60);
    if (!pnode)
    {
        Error("Unable to connect");
        return;
    }

    // Send order to seller, with response going to OnReply2 via event handler
    if (!Status("Requesting public key..."))
        return;
    pnode->PushRequest("checkorder", wtx, SendingDialogOnReply2, this);
}

void SendingDialogOnReply2(void* parg, CDataStream& vRecv)
{
    ((CSendingDialog*)parg)->OnReply2(vRecv);
}

void CSendingDialog::OnReply2(CDataStream& vRecv)
{
    if (!Status("Received public key..."))
        return;

    CScript scriptPubKey;
    int nRet;
    try
    {
        vRecv >> nRet;
        if (nRet > 0)
        {
            string strMessage;
            vRecv >> strMessage;
            Error("Transfer was not accepted");
            //// todo: enlarge the window and enable a hidden white box to put seller's message
            return;
        }
        vRecv >> scriptPubKey;
    }
    catch (...)
    {
        //// what do we want to do about this?
        Error("Invalid response received");
        return;
    }

    // Pause to give the user a chance to cancel
    while (wxDateTime::UNow() < start + wxTimeSpan(0, 0, 0, 2 * 1000))
    {
        Sleep(200);
        if (!Status())
            return;
    }

    CRITICAL_BLOCK(cs_main)
    {
        // Pay
        if (!Status("Creating transaction..."))
            return;
        if (nPrice + nTransactionFee > GetBalance())
        {
            Error("You don't have enough money");
            return;
        }
        CKey key;
        int64 nFeeRequired;
        if (!CreateTransaction(scriptPubKey, nPrice, wtx, key, nFeeRequired))
        {
            if (nPrice + nFeeRequired > GetBalance())
                Error(strprintf("This is an oversized transaction that requires a transaction fee of %s", FormatMoney(nFeeRequired).c_str()));
            else
                Error("Transaction creation failed");
            return;
        }

        // Make sure we're still connected
        CNode* pnode = ConnectNode(addr, 2 * 60 * 60);
        if (!pnode)
        {
            Error("Lost connection, transaction cancelled");
            return;
        }

        // Last chance to cancel
        Sleep(50);
        if (!Status())
            return;
        fCanCancel = false;
        if (fAbort)
        {
            fCanCancel = true;
            if (!Status())
                return;
            fCanCancel = false;
        }
        if (!Status("Sending payment..."))
            return;

        // Commit
        if (!CommitTransactionSpent(wtx, key))
        {
            Error("Error finalizing payment");
            return;
        }

        // Send payment tx to seller, with response going to OnReply3 via event handler
        pnode->PushRequest("submitorder", wtx, SendingDialogOnReply3, this);

        // Accept and broadcast transaction
        if (!wtx.AcceptTransaction())
            printf("ERROR: CSendingDialog : wtxNew.AcceptTransaction() %s failed\n", wtx.GetHash().ToString().c_str());
        wtx.RelayWalletTransaction();

        Status("Waiting for confirmation...");
        MainFrameRepaint();
    }
}

void SendingDialogOnReply3(void* parg, CDataStream& vRecv)
{
    ((CSendingDialog*)parg)->OnReply3(vRecv);
}

void CSendingDialog::OnReply3(CDataStream& vRecv)
{
    int nRet;
    try
    {
        vRecv >> nRet;
        if (nRet > 0)
        {
            Error("The payment was sent, but the recipient was unable to verify it.\n"
                  "The transaction is recorded and will credit to the recipient,\n"
                  "but the comment information will be blank.");
            return;
        }
    }
    catch (...)
    {
        //// what do we want to do about this?
        Error("Payment was sent, but an invalid response was received");
        return;
    }

    fSuccess = true;
    fWorkDone = true;
    Status("Payment completed");
}






//////////////////////////////////////////////////////////////////////////////
//
// CYourAddressDialog
//

CYourAddressDialog::CYourAddressDialog(wxWindow* parent, const string& strInitSelected) : CYourAddressDialogBase(parent)
{
    // Init column headers
    m_listCtrl->InsertColumn(0, "Label", wxLIST_FORMAT_LEFT, 200);
    m_listCtrl->InsertColumn(1, "Bitcoin Address", wxLIST_FORMAT_LEFT, 350);
    m_listCtrl->SetFocus();

    // Fill listctrl with address book data
    CRITICAL_BLOCK(cs_mapKeys)
    {
        foreach(const PAIRTYPE(string, string)& item, mapAddressBook)
        {
            string strAddress = item.first;
            string strName = item.second;
            uint160 hash160;
            bool fMine = (AddressToHash160(strAddress, hash160) && mapPubKeys.count(hash160));
            if (fMine)
            {
                int nIndex = InsertLine(m_listCtrl, strName, strAddress);
                if (strAddress == strInitSelected)
                    m_listCtrl->SetItemState(nIndex, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
            }
        }
    }
}

wxString CYourAddressDialog::GetAddress()
{
    int nIndex = GetSelection(m_listCtrl);
    if (nIndex == -1)
        return "";
    return GetItemText(m_listCtrl, nIndex, 1);
}

void CYourAddressDialog::OnListEndLabelEdit(wxListEvent& event)
{
    // Update address book with edited name
    if (event.IsEditCancelled())
        return;
    string strAddress = (string)GetItemText(m_listCtrl, event.GetIndex(), 1);
    SetAddressBookName(strAddress, string(event.GetText()));
    pframeMain->RefreshListCtrl();
}

void CYourAddressDialog::OnListItemSelected(wxListEvent& event)
{
}

void CYourAddressDialog::OnListItemActivated(wxListEvent& event)
{
    // Doubleclick edits item
    wxCommandEvent event2;
    OnButtonRename(event2);
}

void CYourAddressDialog::OnButtonRename(wxCommandEvent& event)
{
    // Ask new name
    int nIndex = GetSelection(m_listCtrl);
    if (nIndex == -1)
        return;
    string strName = (string)m_listCtrl->GetItemText(nIndex);
    string strAddress = (string)GetItemText(m_listCtrl, nIndex, 1);
    CGetTextFromUserDialog dialog(this, "Edit Address Label", "New Label", strName);
    if (!dialog.ShowModal())
        return;
    strName = dialog.GetValue();

    // Change name
    SetAddressBookName(strAddress, strName);
    m_listCtrl->SetItemText(nIndex, strName);
    pframeMain->RefreshListCtrl();
}

void CYourAddressDialog::OnButtonNew(wxCommandEvent& event)
{
    // Ask name
    CGetTextFromUserDialog dialog(this, "New Bitcoin Address", "Label", "");
    if (!dialog.ShowModal())
        return;
    string strName = dialog.GetValue();

    // Generate new key
    string strAddress = PubKeyToAddress(GenerateNewKey());
    SetAddressBookName(strAddress, strName);

    // Add to list and select it
    int nIndex = InsertLine(m_listCtrl, strName, strAddress);
    SetSelection(m_listCtrl, nIndex);
    m_listCtrl->SetFocus();
}

void CYourAddressDialog::OnButtonCopy(wxCommandEvent& event)
{
    // Copy address box to clipboard
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(GetAddress()));
        wxTheClipboard->Close();
    }
}

void CYourAddressDialog::OnButtonOK(wxCommandEvent& event)
{
    // OK
    EndModal(true);
}

void CYourAddressDialog::OnButtonCancel(wxCommandEvent& event)
{
    // Cancel
    EndModal(false);
}

void CYourAddressDialog::OnClose(wxCloseEvent& event)
{
    // Close
    EndModal(false);
}






//////////////////////////////////////////////////////////////////////////////
//
// CAddressBookDialog
//

CAddressBookDialog::CAddressBookDialog(wxWindow* parent, const wxString& strInitSelected, bool fSendingIn) : CAddressBookDialogBase(parent)
{
    fSending = fSendingIn;
    if (!fSending)
        m_buttonCancel->Show(false);

    // Init column headers
    m_listCtrl->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 200);
    m_listCtrl->InsertColumn(1, "Address", wxLIST_FORMAT_LEFT, 350);
    m_listCtrl->SetFocus();

    // Set Icon
    wxIcon iconAddressBook;
    iconAddressBook.CopyFromBitmap(wxBitmap(addressbook16_xpm));
    SetIcon(iconAddressBook);

    // Fill listctrl with address book data
    CRITICAL_BLOCK(cs_mapKeys)
    {
        foreach(const PAIRTYPE(string, string)& item, mapAddressBook)
        {
            string strAddress = item.first;
            string strName = item.second;
            uint160 hash160;
            bool fMine = (AddressToHash160(strAddress, hash160) && mapPubKeys.count(hash160));
            if (!fMine)
            {
                int nIndex = InsertLine(m_listCtrl, strName, strAddress);
                if (strAddress == strInitSelected)
                    m_listCtrl->SetItemState(nIndex, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
            }
        }
    }
}

wxString CAddressBookDialog::GetAddress()
{
    int nIndex = GetSelection(m_listCtrl);
    if (nIndex == -1)
        return "";
    return GetItemText(m_listCtrl, nIndex, 1);
}

void CAddressBookDialog::OnListEndLabelEdit(wxListEvent& event)
{
    // Update address book with edited name
    if (event.IsEditCancelled())
        return;
    string strAddress = (string)GetItemText(m_listCtrl, event.GetIndex(), 1);
    SetAddressBookName(strAddress, string(event.GetText()));
    pframeMain->RefreshListCtrl();
}

void CAddressBookDialog::OnListItemSelected(wxListEvent& event)
{
}

void CAddressBookDialog::OnListItemActivated(wxListEvent& event)
{
    if (fSending)
    {
        // Doubleclick returns selection
        EndModal(GetAddress() != "" ? 2 : 0);
    }
    else
    {
        // Doubleclick edits item
        wxCommandEvent event2;
        OnButtonEdit(event2);
    }
}

bool CAddressBookDialog::CheckIfMine(const string& strAddress, const string& strTitle)
{
    uint160 hash160;
    bool fMine = (AddressToHash160(strAddress, hash160) && mapPubKeys.count(hash160));
    if (fMine)
        wxMessageBox("This is one of your own addresses for receiving payments and cannot be entered in the address book.  ", strTitle);
    return fMine;
}

void CAddressBookDialog::OnButtonEdit(wxCommandEvent& event)
{
    // Ask new name
    int nIndex = GetSelection(m_listCtrl);
    if (nIndex == -1)
        return;
    string strName = (string)m_listCtrl->GetItemText(nIndex);
    string strAddress = (string)GetItemText(m_listCtrl, nIndex, 1);
    string strAddressOrg = strAddress;
    do
    {
        CGetTextFromUserDialog dialog(this, "Edit Address", "Name", strName, "Address", strAddress);
        if (!dialog.ShowModal())
            return;
        strName = dialog.GetValue1();
        strAddress = dialog.GetValue2();
    }
    while (CheckIfMine(strAddress, "Edit Address"));

    // Change name
    if (strAddress != strAddressOrg)
        CWalletDB().EraseName(strAddressOrg);
    SetAddressBookName(strAddress, strName);
    m_listCtrl->SetItem(nIndex, 1, strAddress);
    m_listCtrl->SetItemText(nIndex, strName);
    pframeMain->RefreshListCtrl();
}

void CAddressBookDialog::OnButtonNew(wxCommandEvent& event)
{
    // Ask name
    string strName;
    string strAddress;
    do
    {
        CGetTextFromUserDialog dialog(this, "New Address", "Name", strName, "Address", strAddress);
        if (!dialog.ShowModal())
            return;
        strName = dialog.GetValue1();
        strAddress = dialog.GetValue2();
    }
    while (CheckIfMine(strAddress, "New Address"));

    // Add to list and select it
    SetAddressBookName(strAddress, strName);
    int nIndex = InsertLine(m_listCtrl, strName, strAddress);
    SetSelection(m_listCtrl, nIndex);
    m_listCtrl->SetFocus();
    pframeMain->RefreshListCtrl();
}

void CAddressBookDialog::OnButtonDelete(wxCommandEvent& event)
{
    for (int nIndex = m_listCtrl->GetItemCount()-1; nIndex >= 0; nIndex--)
    {
        if (m_listCtrl->GetItemState(nIndex, wxLIST_STATE_SELECTED))
        {
            string strAddress = (string)GetItemText(m_listCtrl, nIndex, 1);
            CWalletDB().EraseName(strAddress);
            m_listCtrl->DeleteItem(nIndex);
        }
    }
    pframeMain->RefreshListCtrl();
}

void CAddressBookDialog::OnButtonCopy(wxCommandEvent& event)
{
    // Copy address box to clipboard
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(GetAddress()));
        wxTheClipboard->Close();
    }
}

void CAddressBookDialog::OnButtonOK(wxCommandEvent& event)
{
    // OK
    EndModal(GetAddress() != "" ? 1 : 0);
}

void CAddressBookDialog::OnButtonCancel(wxCommandEvent& event)
{
    // Cancel
    EndModal(0);
}

void CAddressBookDialog::OnClose(wxCloseEvent& event)
{
    // Close
    EndModal(0);
}






//////////////////////////////////////////////////////////////////////////////
//
// CProductsDialog
//

bool CompareIntStringPairBestFirst(const pair<int, string>& item1, const pair<int, string>& item2)
{
    return (item1.first > item2.first);
}

CProductsDialog::CProductsDialog(wxWindow* parent) : CProductsDialogBase(parent)
{
    // Init column headers
    m_listCtrl->InsertColumn(0, "Title",  wxLIST_FORMAT_LEFT, 200);
    m_listCtrl->InsertColumn(1, "Price",  wxLIST_FORMAT_LEFT, 80);
    m_listCtrl->InsertColumn(2, "Seller", wxLIST_FORMAT_LEFT, 80);
    m_listCtrl->InsertColumn(3, "Stars",  wxLIST_FORMAT_LEFT, 50);
    m_listCtrl->InsertColumn(4, "Power",  wxLIST_FORMAT_LEFT, 50);

    // Tally top categories
    map<string, int> mapTopCategories;
    CRITICAL_BLOCK(cs_mapProducts)
        for (map<uint256, CProduct>::iterator mi = mapProducts.begin(); mi != mapProducts.end(); ++mi)
            mapTopCategories[(*mi).second.mapValue["category"]]++;

    // Sort top categories
    vector<pair<int, string> > vTopCategories;
    for (map<string, int>::iterator mi = mapTopCategories.begin(); mi != mapTopCategories.end(); ++mi)
        vTopCategories.push_back(make_pair((*mi).second, (*mi).first));
    sort(vTopCategories.begin(), vTopCategories.end(), CompareIntStringPairBestFirst);

    // Fill categories combo box
    int nLimit = 250;
    for (vector<pair<int, string> >::iterator it = vTopCategories.begin(); it != vTopCategories.end() && nLimit-- > 0; ++it)
        m_comboBoxCategory->Append((*it).second);

    // Fill window with initial search
    //wxCommandEvent event;
    //OnButtonSearch(event);
}

void CProductsDialog::OnCombobox(wxCommandEvent& event)
{
    OnButtonSearch(event);
}

bool CompareProductsBestFirst(const CProduct* p1, const CProduct* p2)
{
    return (p1->nAtoms > p2->nAtoms);
}

void CProductsDialog::OnButtonSearch(wxCommandEvent& event)
{
    string strCategory = (string)m_comboBoxCategory->GetValue();
    string strSearch = (string)m_textCtrlSearch->GetValue();

    // Search products
    vector<CProduct*> vProductsFound;
    CRITICAL_BLOCK(cs_mapProducts)
    {
        for (map<uint256, CProduct>::iterator mi = mapProducts.begin(); mi != mapProducts.end(); ++mi)
        {
            CProduct& product = (*mi).second;
            if (product.mapValue["category"].find(strCategory) != -1)
            {
                if (product.mapValue["title"].find(strSearch) != -1 ||
                    product.mapValue["description"].find(strSearch) != -1 ||
                    product.mapValue["seller"].find(strSearch) != -1)
                {
                    vProductsFound.push_back(&product);
                }
            }
        }
    }

    // Sort
    sort(vProductsFound.begin(), vProductsFound.end(), CompareProductsBestFirst);

    // Display
    foreach(CProduct* pproduct, vProductsFound)
    {
        InsertLine(m_listCtrl,
                   pproduct->mapValue["title"],
                   pproduct->mapValue["price"],
                   pproduct->mapValue["seller"],
                   pproduct->mapValue["stars"],
                   itostr(pproduct->nAtoms));
    }
}

void CProductsDialog::OnListItemActivated(wxListEvent& event)
{
    // Doubleclick opens product
    CViewProductDialog* pdialog = new CViewProductDialog(this, m_vProduct[event.GetIndex()]);
    pdialog->Show();
}







//////////////////////////////////////////////////////////////////////////////
//
// CEditProductDialog
//

CEditProductDialog::CEditProductDialog(wxWindow* parent) : CEditProductDialogBase(parent)
{
    m_textCtrlLabel[0 ] = m_textCtrlLabel0;
    m_textCtrlLabel[1 ] = m_textCtrlLabel1;
    m_textCtrlLabel[2 ] = m_textCtrlLabel2;
    m_textCtrlLabel[3 ] = m_textCtrlLabel3;
    m_textCtrlLabel[4 ] = m_textCtrlLabel4;
    m_textCtrlLabel[5 ] = m_textCtrlLabel5;
    m_textCtrlLabel[6 ] = m_textCtrlLabel6;
    m_textCtrlLabel[7 ] = m_textCtrlLabel7;
    m_textCtrlLabel[8 ] = m_textCtrlLabel8;
    m_textCtrlLabel[9 ] = m_textCtrlLabel9;
    m_textCtrlLabel[10] = m_textCtrlLabel10;
    m_textCtrlLabel[11] = m_textCtrlLabel11;
    m_textCtrlLabel[12] = m_textCtrlLabel12;
    m_textCtrlLabel[13] = m_textCtrlLabel13;
    m_textCtrlLabel[14] = m_textCtrlLabel14;
    m_textCtrlLabel[15] = m_textCtrlLabel15;
    m_textCtrlLabel[16] = m_textCtrlLabel16;
    m_textCtrlLabel[17] = m_textCtrlLabel17;
    m_textCtrlLabel[18] = m_textCtrlLabel18;
    m_textCtrlLabel[19] = m_textCtrlLabel19;

    m_textCtrlField[0 ] = m_textCtrlField0;
    m_textCtrlField[1 ] = m_textCtrlField1;
    m_textCtrlField[2 ] = m_textCtrlField2;
    m_textCtrlField[3 ] = m_textCtrlField3;
    m_textCtrlField[4 ] = m_textCtrlField4;
    m_textCtrlField[5 ] = m_textCtrlField5;
    m_textCtrlField[6 ] = m_textCtrlField6;
    m_textCtrlField[7 ] = m_textCtrlField7;
    m_textCtrlField[8 ] = m_textCtrlField8;
    m_textCtrlField[9 ] = m_textCtrlField9;
    m_textCtrlField[10] = m_textCtrlField10;
    m_textCtrlField[11] = m_textCtrlField11;
    m_textCtrlField[12] = m_textCtrlField12;
    m_textCtrlField[13] = m_textCtrlField13;
    m_textCtrlField[14] = m_textCtrlField14;
    m_textCtrlField[15] = m_textCtrlField15;
    m_textCtrlField[16] = m_textCtrlField16;
    m_textCtrlField[17] = m_textCtrlField17;
    m_textCtrlField[18] = m_textCtrlField18;
    m_textCtrlField[19] = m_textCtrlField19;

    m_buttonDel[0 ] = m_buttonDel0;
    m_buttonDel[1 ] = m_buttonDel1;
    m_buttonDel[2 ] = m_buttonDel2;
    m_buttonDel[3 ] = m_buttonDel3;
    m_buttonDel[4 ] = m_buttonDel4;
    m_buttonDel[5 ] = m_buttonDel5;
    m_buttonDel[6 ] = m_buttonDel6;
    m_buttonDel[7 ] = m_buttonDel7;
    m_buttonDel[8 ] = m_buttonDel8;
    m_buttonDel[9 ] = m_buttonDel9;
    m_buttonDel[10] = m_buttonDel10;
    m_buttonDel[11] = m_buttonDel11;
    m_buttonDel[12] = m_buttonDel12;
    m_buttonDel[13] = m_buttonDel13;
    m_buttonDel[14] = m_buttonDel14;
    m_buttonDel[15] = m_buttonDel15;
    m_buttonDel[16] = m_buttonDel16;
    m_buttonDel[17] = m_buttonDel17;
    m_buttonDel[18] = m_buttonDel18;
    m_buttonDel[19] = m_buttonDel19;

    for (int i = 1; i < FIELDS_MAX; i++)
        ShowLine(i, false);

    LayoutAll();
}

void CEditProductDialog::LayoutAll()
{
    m_scrolledWindow->Layout();
    m_scrolledWindow->GetSizer()->Fit(m_scrolledWindow);
    this->Layout();
}

void CEditProductDialog::ShowLine(int i, bool fShow)
{
    m_textCtrlLabel[i]->Show(fShow);
    m_textCtrlField[i]->Show(fShow);
    m_buttonDel[i]->Show(fShow);
}

void CEditProductDialog::OnButtonDel0(wxCommandEvent& event)  { OnButtonDel(event, 0); }
void CEditProductDialog::OnButtonDel1(wxCommandEvent& event)  { OnButtonDel(event, 1); }
void CEditProductDialog::OnButtonDel2(wxCommandEvent& event)  { OnButtonDel(event, 2); }
void CEditProductDialog::OnButtonDel3(wxCommandEvent& event)  { OnButtonDel(event, 3); }
void CEditProductDialog::OnButtonDel4(wxCommandEvent& event)  { OnButtonDel(event, 4); }
void CEditProductDialog::OnButtonDel5(wxCommandEvent& event)  { OnButtonDel(event, 5); }
void CEditProductDialog::OnButtonDel6(wxCommandEvent& event)  { OnButtonDel(event, 6); }
void CEditProductDialog::OnButtonDel7(wxCommandEvent& event)  { OnButtonDel(event, 7); }
void CEditProductDialog::OnButtonDel8(wxCommandEvent& event)  { OnButtonDel(event, 8); }
void CEditProductDialog::OnButtonDel9(wxCommandEvent& event)  { OnButtonDel(event, 9); }
void CEditProductDialog::OnButtonDel10(wxCommandEvent& event) { OnButtonDel(event, 10); }
void CEditProductDialog::OnButtonDel11(wxCommandEvent& event) { OnButtonDel(event, 11); }
void CEditProductDialog::OnButtonDel12(wxCommandEvent& event) { OnButtonDel(event, 12); }
void CEditProductDialog::OnButtonDel13(wxCommandEvent& event) { OnButtonDel(event, 13); }
void CEditProductDialog::OnButtonDel14(wxCommandEvent& event) { OnButtonDel(event, 14); }
void CEditProductDialog::OnButtonDel15(wxCommandEvent& event) { OnButtonDel(event, 15); }
void CEditProductDialog::OnButtonDel16(wxCommandEvent& event) { OnButtonDel(event, 16); }
void CEditProductDialog::OnButtonDel17(wxCommandEvent& event) { OnButtonDel(event, 17); }
void CEditProductDialog::OnButtonDel18(wxCommandEvent& event) { OnButtonDel(event, 18); }
void CEditProductDialog::OnButtonDel19(wxCommandEvent& event) { OnButtonDel(event, 19); }

void CEditProductDialog::OnButtonDel(wxCommandEvent& event, int n)
{
    Freeze();
    int x, y;
    m_scrolledWindow->GetViewStart(&x, &y);
    int i;
    for (i = n; i < FIELDS_MAX-1; i++)
    {
        m_textCtrlLabel[i]->SetValue(m_textCtrlLabel[i+1]->GetValue());
        m_textCtrlField[i]->SetValue(m_textCtrlField[i+1]->GetValue());
        if (!m_buttonDel[i+1]->IsShown())
            break;
    }
    m_textCtrlLabel[i]->SetValue("");
    m_textCtrlField[i]->SetValue("");
    ShowLine(i, false);
    m_buttonAddField->Enable(true);
    LayoutAll();
    m_scrolledWindow->Scroll(0, y);
    Thaw();
}

void CEditProductDialog::OnButtonAddField(wxCommandEvent& event)
{
    for (int i = 0; i < FIELDS_MAX; i++)
    {
        if (!m_buttonDel[i]->IsShown())
        {
            Freeze();
            ShowLine(i, true);
            if (i == FIELDS_MAX-1)
                m_buttonAddField->Enable(false);
            LayoutAll();
            m_scrolledWindow->Scroll(0, 99999);
            Thaw();
            break;
        }
    }
}

void AddToMyProducts(CProduct product)
{
    CProduct& productInsert = mapMyProducts[product.GetHash()];
    productInsert = product;
    //InsertLine(pframeMain->m_listCtrlProductsSent, &productInsert,
    //            product.mapValue["category"],
    //            product.mapValue["title"].substr(0, 100),
    //            product.mapValue["description"].substr(0, 100),
    //            product.mapValue["price"],
    //            "");
}

void CEditProductDialog::OnButtonSend(wxCommandEvent& event)
{
    CProduct product;
    GetProduct(product);

    // Sign the detailed product
    product.vchPubKeyFrom = keyUser.GetPubKey();
    if (!keyUser.Sign(product.GetSigHash(), product.vchSig))
    {
        wxMessageBox("Error digitally signing the product  ");
        return;
    }

    // Save detailed product
    AddToMyProducts(product);

    // Strip down to summary product
    product.mapDetails.clear();
    product.vOrderForm.clear();

    // Sign the summary product
    if (!keyUser.Sign(product.GetSigHash(), product.vchSig))
    {
        wxMessageBox("Error digitally signing the product  ");
        return;
    }

    // Verify
    if (!product.CheckProduct())
    {
        wxMessageBox("Errors found in product  ");
        return;
    }

    // Broadcast
    AdvertStartPublish(pnodeLocalHost, MSG_PRODUCT, 0, product);

    Destroy();
}

void CEditProductDialog::OnButtonPreview(wxCommandEvent& event)
{
    CProduct product;
    GetProduct(product);
    CViewProductDialog* pdialog = new CViewProductDialog(this, product);
    pdialog->Show();
}

void CEditProductDialog::OnButtonCancel(wxCommandEvent& event)
{
    Destroy();
}

void CEditProductDialog::SetProduct(const CProduct& productIn)
{
    CProduct product = productIn;

    m_comboBoxCategory->SetValue(product.mapValue["category"]);
    m_textCtrlTitle->SetValue(product.mapValue["title"]);
    m_textCtrlPrice->SetValue(product.mapValue["price"]);
    m_textCtrlDescription->SetValue(product.mapValue["description"]);
    m_textCtrlInstructions->SetValue(product.mapValue["instructions"]);

    for (int i = 0; i < FIELDS_MAX; i++)
    {
        bool fUsed = i < product.vOrderForm.size();
        m_buttonDel[i]->Show(fUsed);
        m_textCtrlLabel[i]->Show(fUsed);
        m_textCtrlField[i]->Show(fUsed);
        if (!fUsed)
            continue;

        m_textCtrlLabel[i]->SetValue(product.vOrderForm[i].first);
        string strControl = product.vOrderForm[i].second;
        if (strControl.substr(0, 5) == "text=")
            m_textCtrlField[i]->SetValue("");
        else if (strControl.substr(0, 7) == "choice=")
            m_textCtrlField[i]->SetValue(strControl.substr(7));
        else
            m_textCtrlField[i]->SetValue(strControl);
    }
}

void CEditProductDialog::GetProduct(CProduct& product)
{
    // map<string, string> mapValue;
    // vector<pair<string, string> > vOrderForm;

    product.mapValue["category"]     = m_comboBoxCategory->GetValue().Trim();
    product.mapValue["title"]        = m_textCtrlTitle->GetValue().Trim();
    product.mapValue["price"]        = m_textCtrlPrice->GetValue().Trim();
    product.mapValue["description"]  = m_textCtrlDescription->GetValue().Trim();
    product.mapValue["instructions"] = m_textCtrlInstructions->GetValue().Trim();

    for (int i = 0; i < FIELDS_MAX; i++)
    {
        if (m_buttonDel[i]->IsShown())
        {
            string strLabel = (string)m_textCtrlLabel[i]->GetValue().Trim();
            string strControl = (string)m_textCtrlField[i]->GetValue();
            if (strControl.empty())
                strControl = "text=";
            else
                strControl = "choice=" + strControl;
            product.vOrderForm.push_back(make_pair(strLabel, strControl));
        }
    }
}







//////////////////////////////////////////////////////////////////////////////
//
// CViewProductDialog
//

CViewProductDialog::CViewProductDialog(wxWindow* parent, const CProduct& productIn) : CViewProductDialogBase(parent)
{
    Connect(wxEVT_REPLY1, wxCommandEventHandler(CViewProductDialog::OnReply1), NULL, this);
    AddCallbackAvailable(GetEventHandler());

    // Fill display with product summary while waiting for details
    product = productIn;
    UpdateProductDisplay(false);

    m_buttonBack->Enable(false);
    m_buttonNext->Enable(!product.vOrderForm.empty());
    m_htmlWinReviews->Show(true);
    m_scrolledWindow->Show(false);
    this->Layout();

    // Request details from seller
    CreateThread(ThreadRequestProductDetails, new pair<CProduct, wxEvtHandler*>(product, GetEventHandler()));
}

CViewProductDialog::~CViewProductDialog()
{
    RemoveCallbackAvailable(GetEventHandler());
}

void ThreadRequestProductDetails(void* parg)
{
    // Extract parameters
    pair<CProduct, wxEvtHandler*>* pitem = (pair<CProduct, wxEvtHandler*>*)parg;
    CProduct product = pitem->first;
    wxEvtHandler* pevthandler = pitem->second;
    delete pitem;

    // Connect to seller
    CNode* pnode = ConnectNode(product.addr, 5 * 60);
    if (!pnode)
    {
        CDataStream ssEmpty;
        AddPendingReplyEvent1(pevthandler, ssEmpty);
        return;
    }

    // Request detailed product, with response going to OnReply1 via dialog's event handler
    pnode->PushRequest("getdetails", product.GetHash(), AddPendingReplyEvent1, (void*)pevthandler);
}

void CViewProductDialog::OnReply1(wxCommandEvent& event)
{
    CDataStream ss = GetStreamFromEvent(event);
    if (ss.empty())
    {
        product.mapValue["description"] = "-- CAN'T CONNECT TO SELLER --\n";
        UpdateProductDisplay(true);
        return;
    }

    int nRet;
    CProduct product2;
    try
    {
        ss >> nRet;
        if (nRet > 0)
            throw false;
        ss >> product2;
        if (product2.GetHash() != product.GetHash())
            throw false;
        if (!product2.CheckSignature())
            throw false;
    }
    catch (...)
    {
        product.mapValue["description"] = "-- INVALID RESPONSE --\n";
        UpdateProductDisplay(true);
        return;
    }

    product = product2;
    UpdateProductDisplay(true);
}

bool CompareReviewsBestFirst(const CReview* p1, const CReview* p2)
{
    return (p1->nAtoms > p2->nAtoms);
}

void CViewProductDialog::UpdateProductDisplay(bool fDetails)
{
    // Product and reviews
    string strHTML;
    strHTML.reserve(4000);
    strHTML += "<html>\n"
               "<head>\n"
               "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n"
               "</head>\n"
               "<body>\n";
    strHTML += "<b>Category:</b> " + HtmlEscape(product.mapValue["category"]) + "<br>\n";
    strHTML += "<b>Title:</b> "    + HtmlEscape(product.mapValue["title"])    + "<br>\n";
    strHTML += "<b>Price:</b> "    + HtmlEscape(product.mapValue["price"])    + "<br>\n";

    if (!fDetails)
        strHTML += "<b>Loading details...</b><br>\n<br>\n";
    else
        strHTML += HtmlEscape(product.mapValue["description"], true) + "<br>\n<br>\n";

    strHTML += "<b>Reviews:</b><br>\n<br>\n";

    if (!product.vchPubKeyFrom.empty())
    {
        CReviewDB reviewdb("r");

        // Get reviews
        vector<CReview> vReviews;
        reviewdb.ReadReviews(product.GetUserHash(), vReviews);

        // Get reviewer's number of atoms
        vector<CReview*> vSortedReviews;
        vSortedReviews.reserve(vReviews.size());
        for (vector<CReview>::reverse_iterator it = vReviews.rbegin(); it != vReviews.rend(); ++it)
        {
            CReview& review = *it;
            CUser user;
            reviewdb.ReadUser(review.GetUserHash(), user);
            review.nAtoms = user.GetAtomCount();
            vSortedReviews.push_back(&review);
        }

        reviewdb.Close();

        // Sort
        stable_sort(vSortedReviews.begin(), vSortedReviews.end(), CompareReviewsBestFirst);

        // Format reviews
        foreach(CReview* preview, vSortedReviews)
        {
            CReview& review = *preview;
            int nStars = atoi(review.mapValue["stars"].c_str());
            if (nStars < 1 || nStars > 5)
                continue;

            strHTML += "<b>" + itostr(nStars) + (nStars == 1 ? " star" : " stars") + "</b>";
            strHTML += " &nbsp;&nbsp;&nbsp; ";
            strHTML += DateStr(atoi64(review.mapValue["date"])) + "<br>\n";
            strHTML += HtmlEscape(review.mapValue["review"], true);
            strHTML += "<br>\n<br>\n";
        }
    }

    strHTML += "</body>\n</html>\n";

    // Shrink capacity to fit
    string(strHTML.begin(), strHTML.end()).swap(strHTML);

    m_htmlWinReviews->SetPage(strHTML);

    ///// need to find some other indicator to use so can allow empty order form
    if (product.vOrderForm.empty())
        return;

    // Order form
    m_staticTextInstructions->SetLabel(product.mapValue["instructions"]);
    for (int i = 0; i < FIELDS_MAX; i++)
    {
        m_staticTextLabel[i] = NULL;
        m_textCtrlField[i] = NULL;
        m_choiceField[i] = NULL;
    }

    // Construct flexgridsizer
    wxBoxSizer* bSizer21 = (wxBoxSizer*)m_scrolledWindow->GetSizer();
    wxFlexGridSizer* fgSizer;
    fgSizer = new wxFlexGridSizer(0, 2, 0, 0);
    fgSizer->AddGrowableCol(1);
    fgSizer->SetFlexibleDirection(wxBOTH);
    fgSizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Construct order form fields
    wxWindow* windowLast = NULL;
    for (int i = 0; i < product.vOrderForm.size(); i++)
    {
        string strLabel = product.vOrderForm[i].first;
        string strControl = product.vOrderForm[i].second;

        if (strLabel.size() < 20)
            strLabel.insert(strLabel.begin(), 20 - strLabel.size(), ' ');

        m_staticTextLabel[i] = new wxStaticText(m_scrolledWindow, wxID_ANY, strLabel, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
        m_staticTextLabel[i]->Wrap(-1);
        fgSizer->Add(m_staticTextLabel[i], 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5);

        if (strControl.substr(0, 5) == "text=")
        {
            m_textCtrlField[i] = new wxTextCtrl(m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
            fgSizer->Add(m_textCtrlField[i], 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5);
            windowLast = m_textCtrlField[i];
        }
        else if (strControl.substr(0, 7) == "choice=")
        {
            vector<string> vChoices;
            ParseString(strControl.substr(7), ',', vChoices);

            wxArrayString arraystring;
            foreach(const string& str, vChoices)
                arraystring.Add(str);

            m_choiceField[i] = new wxChoice(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, arraystring, 0);
            fgSizer->Add(m_choiceField[i], 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
            windowLast = m_choiceField[i];
        }
        else
        {
            m_textCtrlField[i] = new wxTextCtrl(m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
            fgSizer->Add(m_textCtrlField[i], 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5);
            m_staticTextLabel[i]->Show(false);
            m_textCtrlField[i]->Show(false);
        }
    }

    // Insert after instructions and before submit/cancel buttons
    bSizer21->Insert(2, fgSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5);
    m_scrolledWindow->Layout();
    bSizer21->Fit(m_scrolledWindow);
    this->Layout();

    // Fixup the tab order
    m_buttonSubmitForm->MoveAfterInTabOrder(windowLast);
    m_buttonCancelForm->MoveAfterInTabOrder(m_buttonSubmitForm);
    //m_buttonBack->MoveAfterInTabOrder(m_buttonCancelForm);
    //m_buttonNext->MoveAfterInTabOrder(m_buttonBack);
    //m_buttonCancel->MoveAfterInTabOrder(m_buttonNext);
    this->Layout();
}

void CViewProductDialog::GetOrder(CWalletTx& wtx)
{
    wtx.SetNull();
    for (int i = 0; i < product.vOrderForm.size(); i++)
    {
        string strValue;
        if (m_textCtrlField[i])
            strValue = m_textCtrlField[i]->GetValue().Trim();
        else
            strValue = m_choiceField[i]->GetStringSelection();
        wtx.vOrderForm.push_back(make_pair(m_staticTextLabel[i]->GetLabel(), strValue));
    }
}

void CViewProductDialog::OnButtonSubmitForm(wxCommandEvent& event)
{
    m_buttonSubmitForm->Enable(false);
    m_buttonCancelForm->Enable(false);

    CWalletTx wtx;
    GetOrder(wtx);

    CSendingDialog* pdialog = new CSendingDialog(this, product.addr, atoi64(product.mapValue["price"]), wtx);
    if (!pdialog->ShowModal())
    {
        m_buttonSubmitForm->Enable(true);
        m_buttonCancelForm->Enable(true);
        return;
    }
}

void CViewProductDialog::OnButtonCancelForm(wxCommandEvent& event)
{
    Destroy();
}

void CViewProductDialog::OnButtonBack(wxCommandEvent& event)
{
    Freeze();
    m_htmlWinReviews->Show(true);
    m_scrolledWindow->Show(false);
    m_buttonBack->Enable(false);
    m_buttonNext->Enable(!product.vOrderForm.empty());
    this->Layout();
    Thaw();
}

void CViewProductDialog::OnButtonNext(wxCommandEvent& event)
{
    if (!product.vOrderForm.empty())
    {
        Freeze();
        m_htmlWinReviews->Show(false);
        m_scrolledWindow->Show(true);
        m_buttonBack->Enable(true);
        m_buttonNext->Enable(false);
        this->Layout();
        Thaw();
    }
}

void CViewProductDialog::OnButtonCancel(wxCommandEvent& event)
{
    Destroy();
}







//////////////////////////////////////////////////////////////////////////////
//
// CViewOrderDialog
//

CViewOrderDialog::CViewOrderDialog(wxWindow* parent, CWalletTx order, bool fReceived) : CViewOrderDialogBase(parent)
{
    int64 nPrice = (fReceived ? order.GetCredit() : order.GetDebit());

    string strHTML;
    strHTML.reserve(4000);
    strHTML += "<html>\n"
               "<head>\n"
               "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n"
               "</head>\n"
               "<body>\n";
    strHTML += "<b>Time:</b> "   + HtmlEscape(DateTimeStr(order.nTimeReceived)) + "<br>\n";
    strHTML += "<b>Price:</b> "  + HtmlEscape(FormatMoney(nPrice)) + "<br>\n";
    strHTML += "<b>Status:</b> " + HtmlEscape(FormatTxStatus(order)) + "<br>\n";

    strHTML += "<table>\n";
    for (int i = 0; i < order.vOrderForm.size(); i++)
    {
        strHTML += " <tr><td><b>" + HtmlEscape(order.vOrderForm[i].first) + ":</b></td>";
        strHTML += "<td>" + HtmlEscape(order.vOrderForm[i].second) + "</td></tr>\n";
    }
    strHTML += "</table>\n";

    strHTML += "</body>\n</html>\n";

    // Shrink capacity to fit
    // (strings are ref counted, so it may live on in SetPage)
    string(strHTML.begin(), strHTML.end()).swap(strHTML);

    m_htmlWin->SetPage(strHTML);
}

void CViewOrderDialog::OnButtonOK(wxCommandEvent& event)
{
    Destroy();
}







//////////////////////////////////////////////////////////////////////////////
//
// CEditReviewDialog
//

CEditReviewDialog::CEditReviewDialog(wxWindow* parent) : CEditReviewDialogBase(parent)
{
}

void CEditReviewDialog::OnButtonSubmit(wxCommandEvent& event)
{
    if (m_choiceStars->GetSelection() == -1)
    {
        wxMessageBox("Please select a rating  ");
        return;
    }

    CReview review;
    GetReview(review);

    // Sign the review
    review.vchPubKeyFrom = keyUser.GetPubKey();
    if (!keyUser.Sign(review.GetSigHash(), review.vchSig))
    {
        wxMessageBox("Unable to digitally sign the review  ");
        return;
    }

    // Broadcast
    if (!review.AcceptReview())
    {
        wxMessageBox("Save failed  ");
        return;
    }
    RelayMessage(CInv(MSG_REVIEW, review.GetHash()), review);

    Destroy();
}

void CEditReviewDialog::OnButtonCancel(wxCommandEvent& event)
{
    Destroy();
}

void CEditReviewDialog::GetReview(CReview& review)
{
    review.mapValue["time"]   = i64tostr(GetAdjustedTime());
    review.mapValue["stars"]  = itostr(m_choiceStars->GetSelection()+1);
    review.mapValue["review"] = m_textCtrlReview->GetValue();
}







//////////////////////////////////////////////////////////////////////////////
//
// CMyTaskBarIcon
//

enum
{
    ID_TASKBAR_RESTORE = 10001,
    ID_TASKBAR_OPTIONS,
    ID_TASKBAR_GENERATE,
    ID_TASKBAR_EXIT,
};

BEGIN_EVENT_TABLE(CMyTaskBarIcon, wxTaskBarIcon)
    EVT_TASKBAR_LEFT_DCLICK(CMyTaskBarIcon::OnLeftButtonDClick)
    EVT_MENU(ID_TASKBAR_RESTORE, CMyTaskBarIcon::OnMenuRestore)
    EVT_MENU(ID_TASKBAR_OPTIONS, CMyTaskBarIcon::OnMenuOptions)
    EVT_MENU(ID_TASKBAR_GENERATE, CMyTaskBarIcon::OnMenuGenerate)
    EVT_UPDATE_UI(ID_TASKBAR_GENERATE, CMyTaskBarIcon::OnUpdateUIGenerate)
    EVT_MENU(ID_TASKBAR_EXIT, CMyTaskBarIcon::OnMenuExit)
END_EVENT_TABLE()

void CMyTaskBarIcon::Show(bool fShow)
{
    static char pszPrevTip[200];
    if (fShow)
    {
        string strTooltip = "Bitcoin";
        if (fGenerateBitcoins)
            strTooltip = "Bitcoin - Generating";
        if (fGenerateBitcoins && vNodes.empty())
            strTooltip = "Bitcoin - (not connected)";

        // Optimization, only update when changed, using char array to be reentrant
        if (strncmp(pszPrevTip, strTooltip.c_str(), sizeof(pszPrevTip)-1) != 0)
        {
            strlcpy(pszPrevTip, strTooltip.c_str(), sizeof(pszPrevTip));
#ifdef __WXMSW__
            SetIcon(wxICON(bitcoin), strTooltip);
#else
            SetIcon(bitcoin20_xpm, strTooltip);
#endif
        }
    }
    else
    {
        strlcpy(pszPrevTip, "", sizeof(pszPrevTip));
        RemoveIcon();
    }
}

void CMyTaskBarIcon::Hide()
{
    Show(false);
}

void CMyTaskBarIcon::OnLeftButtonDClick(wxTaskBarIconEvent& event)
{
    Restore();
}

void CMyTaskBarIcon::OnMenuRestore(wxCommandEvent& event)
{
    Restore();
}

void CMyTaskBarIcon::OnMenuOptions(wxCommandEvent& event)
{
    // Since it's modal, get the main window to do it
    wxCommandEvent event2(wxEVT_COMMAND_MENU_SELECTED, wxID_MENUOPTIONSOPTIONS);
    pframeMain->GetEventHandler()->AddPendingEvent(event2);
}

void CMyTaskBarIcon::Restore()
{
    pframeMain->Show();
    wxIconizeEvent event(0, false);
    pframeMain->GetEventHandler()->AddPendingEvent(event);
    pframeMain->Iconize(false);
    pframeMain->Raise();
}

void CMyTaskBarIcon::OnMenuGenerate(wxCommandEvent& event)
{
    GenerateBitcoins(event.IsChecked());
}

void CMyTaskBarIcon::OnUpdateUIGenerate(wxUpdateUIEvent& event)
{
    event.Check(fGenerateBitcoins);
}

void CMyTaskBarIcon::OnMenuExit(wxCommandEvent& event)
{
    pframeMain->Close(true);
}

void CMyTaskBarIcon::UpdateTooltip()
{
    if (IsIconInstalled())
        Show(true);
}

wxMenu* CMyTaskBarIcon::CreatePopupMenu()
{
    wxMenu* pmenu = new wxMenu;
    pmenu->Append(ID_TASKBAR_RESTORE, "&Open Bitcoin");
    pmenu->Append(ID_TASKBAR_OPTIONS, "O&ptions...");
    pmenu->AppendCheckItem(ID_TASKBAR_GENERATE, "&Generate Coins")->Check(fGenerateBitcoins);
#ifndef __WXMAC_OSX__ // Mac has built-in quit menu
    pmenu->AppendSeparator();
    pmenu->Append(ID_TASKBAR_EXIT, "E&xit");
#endif
    return pmenu;
}










//////////////////////////////////////////////////////////////////////////////
//
// CMyApp
//

// Define a new application
class CMyApp: public wxApp
{
  public:
    CMyApp(){};
    ~CMyApp(){};
    bool OnInit();
    bool OnInit2();
    int OnExit();

    // 2nd-level exception handling: we get all the exceptions occurring in any
    // event handler here
    virtual bool OnExceptionInMainLoop();

    // 3rd, and final, level exception handling: whenever an unhandled
    // exception is caught, this function is called
    virtual void OnUnhandledException();

    // and now for something different: this function is called in case of a
    // crash (e.g. dereferencing null pointer, division by 0, ...)
    virtual void OnFatalException();
};

IMPLEMENT_APP(CMyApp)

bool CMyApp::OnInit()
{
    bool fRet = false;
    try
    {
        fRet = OnInit2();
    }
    catch (std::exception& e) {
        PrintException(&e, "OnInit()");
    } catch (...) {
        PrintException(NULL, "OnInit()");
    }
    if (!fRet)
        Shutdown(NULL);
    return fRet;
}

bool CMyApp::OnInit2()
{
#ifdef _MSC_VER
    // Turn off microsoft heap dump noise for now
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFile("NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0));
#endif
#if defined(__WXMSW__) && defined(__WXDEBUG__)
    // Disable malfunctioning wxWidgets debug assertion
    g_isPainting = 10000;
#endif
    wxImage::AddHandler(new wxPNGHandler);
#ifdef __WXMSW__
    SetAppName("Bitcoin");
#else
    SetAppName("bitcoin");
    umask(077);
#endif

    //
    // Parameters
    //
    ParseParameters(argc, argv);
    if (mapArgs.count("-?") || mapArgs.count("--help"))
    {
#ifdef __WXMSW__
        string strUsage =
            "Usage: bitcoin [options]\t\t\t\t\t\t\n"
            "Options:\n"
            "  -gen\t\t  Generate coins\n"
            "  -gen=0\t\t  Don't generate coins\n"
            "  -min\t\t  Start minimized\n"
            "  -datadir=<dir>\t  Specify data directory\n"
            "  -proxy=<ip:port>\t  Connect through socks4 proxy\n"
            "  -addnode=<ip>\t  Add a node to connect to\n"
            "  -connect=<ip>\t  Connect only to the specified node\n"
            "  -?\t\t  This help message\n";
        wxMessageBox(strUsage, "Bitcoin", wxOK);
#else
        string strUsage =
            "Usage: bitcoin [options]\n"
            "Options:\n"
            "  -gen              Generate coins\n"
            "  -gen=0            Don't generate coins\n"
            "  -min              Start minimized\n"
            "  -datadir=<dir>    Specify data directory\n"
            "  -proxy=<ip:port>  Connect through socks4 proxy\n"
            "  -addnode=<ip>     Add a node to connect to\n"
            "  -connect=<ip>     Connect only to the specified node\n"
            "  -?                This help message\n";
        fprintf(stderr, "%s", strUsage.c_str());
#endif
        return false;
    }

    if (mapArgs.count("-blockamount")) {
        CClient client;
        wxString hostname = "localhost";
        wxString server = GetDataDir() + "service";
        CClientConnection * pconnection = (CClientConnection *)client.MakeConnection(hostname, server, "ipc test");
        string output = "";
        if (pconnection) {
            char * pbuffer = (char *)pconnection->Request("blockamount");
            while (*pbuffer != '\n') {
                output += *pbuffer;
                pbuffer++;
            }
        }
        else {
            output = "Cannot access Bitcoin. Are you sure the program is running?\n";
        }
        fprintf(stderr, "%s", output.c_str());
        return false;
    }

    if (mapArgs.count("-datadir"))
        strlcpy(pszSetDataDir, mapArgs["-datadir"].c_str(), sizeof(pszSetDataDir));

    if (mapArgs.count("-debug"))
        fDebug = true;

    if (mapArgs.count("-printtodebugger"))
        fPrintToDebugger = true;

    if (!fDebug && !pszSetDataDir[0])
        ShrinkDebugFile();
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Bitcoin version %d%s, OS version %s\n", VERSION, pszSubVer, ((string)wxGetOsDescription()).c_str());

    if (mapArgs.count("-loadblockindextest"))
    {
        CTxDB txdb("r");
        txdb.LoadBlockIndex();
        PrintBlockTree();
        return false;
    }

    //
    // Limit to single instance per user
    // Required to protect the database files if we're going to keep deleting log.*
    //
#ifdef __WXMSW__
    // todo: wxSingleInstanceChecker wasn't working on Linux, never deleted its lock file
    //  maybe should go by whether successfully bind port 8333 instead
    wxString strMutexName = wxString("bitcoin_running.") + getenv("HOMEPATH");
    for (int i = 0; i < strMutexName.size(); i++)
        if (!isalnum(strMutexName[i]))
            strMutexName[i] = '.';
    wxSingleInstanceChecker* psingleinstancechecker = new wxSingleInstanceChecker(strMutexName);
    if (psingleinstancechecker->IsAnotherRunning())
    {
        printf("Existing instance found\n");
        unsigned int nStart = GetTime();
        loop
        {
            // TODO: find out how to do this in Linux, or replace with wxWidgets commands
            // Show the previous instance and exit
            HWND hwndPrev = FindWindow("wxWindowClassNR", "Bitcoin");
            if (hwndPrev)
            {
                if (IsIconic(hwndPrev))
                    ShowWindow(hwndPrev, SW_RESTORE);
                SetForegroundWindow(hwndPrev);
                return false;
            }

            if (GetTime() > nStart + 60)
                return false;

            // Resume this instance if the other exits
            delete psingleinstancechecker;
            Sleep(1000);
            psingleinstancechecker = new wxSingleInstanceChecker(strMutexName);
            if (!psingleinstancechecker->IsAnotherRunning())
                break;
        }
    }
#endif

    // Bind to the port early so we can tell if another instance is already running.
    // This is a backup to wxSingleInstanceChecker, which doesn't work on Linux.
    string strErrors;
    if (!BindListenPort(strErrors))
    {
        wxMessageBox(strErrors, "Bitcoin");
        return false;
    }

    //
    // Load data files
    //
    bool fFirstRun;
    strErrors = "";
    int64 nStart;

    printf("Loading addresses...\n");
    nStart = GetTimeMillis();
    if (!LoadAddresses())
        strErrors += "Error loading addr.dat      \n";
    printf(" addresses   %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Loading block index...\n");
    nStart = GetTimeMillis();
    if (!LoadBlockIndex())
        strErrors += "Error loading blkindex.dat      \n";
    printf(" block index %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Loading wallet...\n");
    nStart = GetTimeMillis();
    if (!LoadWallet(fFirstRun))
        strErrors += "Error loading wallet.dat      \n";
    printf(" wallet      %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Done loading\n");

        //// debug print
        printf("mapBlockIndex.size() = %d\n",   mapBlockIndex.size());
        printf("nBestHeight = %d\n",            nBestHeight);
        printf("mapKeys.size() = %d\n",         mapKeys.size());
        printf("mapPubKeys.size() = %d\n",      mapPubKeys.size());
        printf("mapWallet.size() = %d\n",       mapWallet.size());
        printf("mapAddressBook.size() = %d\n",  mapAddressBook.size());

    if (!strErrors.empty())
    {
        wxMessageBox(strErrors, "Bitcoin");
        return false;
    }

    // Add wallet transactions that aren't already in a block to mapTransactions
    ReacceptWalletTransactions();

    //
    // Parameters
    //
    if (mapArgs.count("-printblockindex") || mapArgs.count("-printblocktree"))
    {
        PrintBlockTree();
        return false;
    }

    if (mapArgs.count("-printblock"))
    {
        string strMatch = mapArgs["-printblock"];
        int nFound = 0;
        for (map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
        {
            uint256 hash = (*mi).first;
            if (strncmp(hash.ToString().c_str(), strMatch.c_str(), strMatch.size()) == 0)
            {
                CBlockIndex* pindex = (*mi).second;
                CBlock block;
                block.ReadFromDisk(pindex);
                block.BuildMerkleTree();
                block.print();
                printf("\n");
                nFound++;
            }
        }
        if (nFound == 0)
            printf("No blocks matching %s were found\n", strMatch.c_str());
        return false;
    }

    if (mapArgs.count("-gen"))
    {
        if (mapArgs["-gen"].empty())
            fGenerateBitcoins = true;
        else
            fGenerateBitcoins = (atoi(mapArgs["-gen"].c_str()) != 0);
    }

    if (mapArgs.count("-proxy"))
    {
        fUseProxy = true;
        addrProxy = CAddress(mapArgs["-proxy"]);
        if (!addrProxy.IsValid())
        {
            wxMessageBox("Invalid -proxy address", "Bitcoin");
            return false;
        }
    }

    if (mapArgs.count("-addnode"))
    {
        foreach(string strAddr, mapMultiArgs["-addnode"])
        {
            CAddress addr(strAddr, NODE_NETWORK);
            addr.nTime = 0; // so it won't relay unless successfully connected
            if (addr.IsValid())
                AddAddress(addr);
        }
    }

    //
    // Create the main frame window
    //
    if (!mapArgs.count("-noui"))
    {
        pframeMain = new CMainFrame(NULL);
        if (mapArgs.count("-min"))
            pframeMain->Iconize(true);
        pframeMain->Show(true);  // have to show first to get taskbar button to hide
        if (fMinimizeToTray && pframeMain->IsIconized())
            fClosedToTray = true;
        pframeMain->Show(!fClosedToTray);
        ptaskbaricon->Show(fMinimizeToTray || fClosedToTray);

        CreateThread(ThreadDelayedRepaint, NULL);
    }

    if (!CheckDiskSpace())
        return false;

    RandAddSeedPerfmon();

    if (!CreateThread(StartNode, NULL))
        wxMessageBox("Error: CreateThread(StartNode) failed", "Bitcoin");

    if (fFirstRun)
        SetStartOnSystemStartup(true);

    pserver = new CServer;
    pserver->Create(GetDataDir() + "service");

    //
    // Tests
    //
#ifdef __WXMSW__
    if (argc >= 2 && stricmp(argv[1], "-send") == 0)
#else
    if (argc >= 2 && strcmp(argv[1], "-send") == 0)
#endif
    {
        int64 nValue = 1;
        if (argc >= 3)
            ParseMoney(argv[2], nValue);

        string strAddress;
        if (argc >= 4)
            strAddress = argv[3];
        CAddress addr(strAddress);

        CWalletTx wtx;
        wtx.mapValue["to"] = strAddress;
        wtx.mapValue["from"] = addrLocalHost.ToString();
        wtx.mapValue["message"] = "command line send";

        // Send to IP address
        CSendingDialog* pdialog = new CSendingDialog(pframeMain, addr, nValue, wtx);
        if (!pdialog->ShowModal())
            return false;
    }

    return true;
}

int CMyApp::OnExit()
{
    Shutdown(NULL);
    return wxApp::OnExit();
}

bool CMyApp::OnExceptionInMainLoop()
{
    try
    {
        throw;
    }
    catch (std::exception& e)
    {
        PrintException(&e, "CMyApp::OnExceptionInMainLoop()");
        wxLogWarning("Exception %s %s", typeid(e).name(), e.what());
        Sleep(1000);
        throw;
    }
    catch (...)
    {
        PrintException(NULL, "CMyApp::OnExceptionInMainLoop()");
        wxLogWarning("Unknown exception");
        Sleep(1000);
        throw;
    }

    return true;
}

void CMyApp::OnUnhandledException()
{
    // this shows how we may let some exception propagate uncaught
    try
    {
        throw;
    }
    catch (std::exception& e)
    {
        PrintException(&e, "CMyApp::OnUnhandledException()");
        wxLogWarning("Exception %s %s", typeid(e).name(), e.what());
        Sleep(1000);
        throw;
    }
    catch (...)
    {
        PrintException(NULL, "CMyApp::OnUnhandledException()");
        wxLogWarning("Unknown exception");
        Sleep(1000);
        throw;
    }
}

void CMyApp::OnFatalException()
{
    wxMessageBox("Program has crashed and will terminate.  ", "Bitcoin", wxOK | wxICON_ERROR);
}





#ifdef __WXMSW__
typedef WINSHELLAPI BOOL (WINAPI *PSHGETSPECIALFOLDERPATHA)(HWND hwndOwner, LPSTR lpszPath, int nFolder, BOOL fCreate);

string MyGetSpecialFolderPath(int nFolder, bool fCreate)
{
    char pszPath[MAX_PATH+100] = "";

    // SHGetSpecialFolderPath is not usually available on NT 4.0
    HMODULE hShell32 = LoadLibrary("shell32.dll");
    if (hShell32)
    {
        PSHGETSPECIALFOLDERPATHA pSHGetSpecialFolderPath =
            (PSHGETSPECIALFOLDERPATHA)GetProcAddress(hShell32, "SHGetSpecialFolderPathA");
        if (pSHGetSpecialFolderPath)
            (*pSHGetSpecialFolderPath)(NULL, pszPath, nFolder, fCreate);
        FreeModule(hShell32);
    }

    // Backup option
    if (pszPath[0] == '\0')
    {
        if (nFolder == CSIDL_STARTUP)
        {
            strcpy(pszPath, getenv("USERPROFILE"));
            strcat(pszPath, "\\Start Menu\\Programs\\Startup");
        }
        else if (nFolder == CSIDL_APPDATA)
        {
            strcpy(pszPath, getenv("APPDATA"));
        }
    }

    return pszPath;
}

string StartupShortcutPath()
{
    return MyGetSpecialFolderPath(CSIDL_STARTUP, true) + "\\Bitcoin.lnk";
}

bool GetStartOnSystemStartup()
{
    return wxFileExists(StartupShortcutPath());
}

void SetStartOnSystemStartup(bool fAutoStart)
{
    // If the shortcut exists already, remove it for updating
    remove(StartupShortcutPath().c_str());

    if (fAutoStart)
    {
        CoInitialize(NULL);

        // Get a pointer to the IShellLink interface.
        IShellLink* psl = NULL;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL,
                                CLSCTX_INPROC_SERVER, IID_IShellLink,
                                reinterpret_cast<void**>(&psl));

        if (SUCCEEDED(hres))
        {
            // Get the current executable path
            char pszExePath[MAX_PATH];
            GetModuleFileName(NULL, pszExePath, sizeof(pszExePath));

            // Set the path to the shortcut target
            psl->SetPath(pszExePath);
            PathRemoveFileSpec(pszExePath);
            psl->SetWorkingDirectory(pszExePath);
            psl->SetShowCmd(SW_SHOWMINNOACTIVE);

            // Query IShellLink for the IPersistFile interface for
            // saving the shortcut in persistent storage.
            IPersistFile* ppf = NULL;
            hres = psl->QueryInterface(IID_IPersistFile,
                                       reinterpret_cast<void**>(&ppf));
            if (SUCCEEDED(hres))
            {
                WCHAR pwsz[MAX_PATH];
                // Ensure that the string is ANSI.
                MultiByteToWideChar(CP_ACP, 0, StartupShortcutPath().c_str(), -1, pwsz, MAX_PATH);
                // Save the link by calling IPersistFile::Save.
                hres = ppf->Save(pwsz, TRUE);
                ppf->Release();
            }
            psl->Release();
        }
        CoUninitialize();
    }
}
#else
bool GetStartOnSystemStartup() { return false; }
void SetStartOnSystemStartup(bool fAutoStart) { }
#endif










