// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#endif



DEFINE_EVENT_TYPE(wxEVT_CROSSTHREADCALL)
DEFINE_EVENT_TYPE(wxEVT_REPLY1)
DEFINE_EVENT_TYPE(wxEVT_REPLY2)
DEFINE_EVENT_TYPE(wxEVT_REPLY3)
DEFINE_EVENT_TYPE(wxEVT_TABLEADDED)
DEFINE_EVENT_TYPE(wxEVT_TABLEUPDATED)
DEFINE_EVENT_TYPE(wxEVT_TABLEDELETED)

CMainFrame* pframeMain = NULL;
map<string, string> mapAddressBook;


void ThreadRequestProductDetails(void* parg);
void ThreadRandSendTest(void* parg);
bool fRandSendTest = false;
void RandSend();
extern int g_isPainting;






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
    return (string)wxDateTime((time_t)nTime).FormatDate();
}

string DateTimeStr(int64 nTime)
{
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

void AddToMyProducts(CProduct product)
{
    CProduct& productInsert = mapMyProducts[product.GetHash()];
    productInsert = product;
    InsertLine(pframeMain->m_listCtrlProductsSent, &productInsert,
                product.mapValue["category"],
                product.mapValue["title"].substr(0, 100),
                product.mapValue["description"].substr(0, 100),
                product.mapValue["price"],
                "");
}






//////////////////////////////////////////////////////////////////////////////
//
// Custom events
//

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
    if (!pevthandler)
        return;

    const char* pbegin = (pendIn != pbeginIn) ? &pbeginIn[0] : NULL;
    const char* pend = pbegin + (pendIn - pbeginIn) * sizeof(pbeginIn[0]);
    wxCommandEvent event(nEventID);
    wxString strData(wxChar(0), (pend - pbegin) / sizeof(wxChar) + 1);
    memcpy(&strData[0], pbegin, pend - pbegin);
    event.SetString(strData);
    event.SetInt(pend - pbegin);

    pevthandler->AddPendingEvent(event);
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
    return CDataStream(strData.begin(), strData.begin() + event.GetInt(), SER_NETWORK);
}







//////////////////////////////////////////////////////////////////////////////
//
// CMainFrame
//

CMainFrame::CMainFrame(wxWindow* parent) : CMainFrameBase(parent)
{
    Connect(wxEVT_CROSSTHREADCALL, wxCommandEventHandler(CMainFrame::OnCrossThreadCall), NULL, this);

    // Init
    fRefreshListCtrl = false;
    fRefreshListCtrlRunning = false;
    fOnSetFocusAddress = false;
    pindexBestLast = NULL;
    m_choiceFilter->SetSelection(0);
    m_staticTextBalance->SetLabel(FormatMoney(GetBalance()) + "  ");
    m_listCtrl->SetFocus();
    SetIcon(wxICON(bitcoin));
    m_menuOptions->Check(wxID_OPTIONSGENERATEBITCOINS, fGenerateBitcoins);

    // Init toolbar with transparency masked bitmaps
    m_toolBar->ClearTools();

    //// shouldn't have to do mask separately anymore, bitmap alpha support added in wx 2.8.9,
    wxBitmap bmpSend(wxT("send20"), wxBITMAP_TYPE_RESOURCE);
    bmpSend.SetMask(new wxMask(wxBitmap(wxT("send20mask"), wxBITMAP_TYPE_RESOURCE)));
    m_toolBar->AddTool(wxID_BUTTONSEND, wxT("&Send Coins"), bmpSend, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);

    wxBitmap bmpAddressBook(wxT("addressbook20"), wxBITMAP_TYPE_RESOURCE);
    bmpAddressBook.SetMask(new wxMask(wxBitmap(wxT("addressbook20mask"), wxBITMAP_TYPE_RESOURCE)));
    m_toolBar->AddTool(wxID_BUTTONRECEIVE, wxT("&Address Book"), bmpAddressBook, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);

    m_toolBar->Realize();

    // Init column headers
    int nDateWidth = DateTimeStr(1229413914).size() * 6 + 8;
    m_listCtrl->InsertColumn(0, "",             wxLIST_FORMAT_LEFT,     0);
    m_listCtrl->InsertColumn(1, "",             wxLIST_FORMAT_LEFT,     0);
    m_listCtrl->InsertColumn(2, "Status",       wxLIST_FORMAT_LEFT,    90);
    m_listCtrl->InsertColumn(3, "Date",         wxLIST_FORMAT_LEFT,   nDateWidth);
    m_listCtrl->InsertColumn(4, "Description",  wxLIST_FORMAT_LEFT,   409 - nDateWidth);
    m_listCtrl->InsertColumn(5, "Debit",        wxLIST_FORMAT_RIGHT,   79);
    m_listCtrl->InsertColumn(6, "Credit",       wxLIST_FORMAT_RIGHT,   79);

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
    int pnWidths[3] = { -100, 81, 286 };
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
}

void Shutdown(void* parg)
{
    static CCriticalSection cs_Shutdown;
    CRITICAL_BLOCK(cs_Shutdown)
    {
        fShutdown = true;
        nTransactionsUpdated++;
        DBFlush(false);
        StopNode();
        DBFlush(true);

        printf("Bitcoin exiting\n");
        exit(0);
    }
}

void CMainFrame::OnClose(wxCloseEvent& event)
{
    Destroy();
    _beginthread(Shutdown, 0, NULL);
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

void CMainFrame::InsertLine(bool fNew, int nIndex, uint256 hashKey, string strSort, const wxString& str2, const wxString& str3, const wxString& str4, const wxString& str5, const wxString& str6)
{
    string str0 = strSort;
    long nData = *(long*)&hashKey;

    if (fNew)
    {
        nIndex = m_listCtrl->InsertItem(0, str0);
    }
    else
    {
        if (nIndex == -1)
        {
            // Find item
            while ((nIndex = m_listCtrl->FindItem(nIndex, nData)) != -1)
                if (GetItemText(m_listCtrl, nIndex, 1) == hashKey.ToString())
                    break;
            if (nIndex == -1)
            {
                printf("CMainFrame::InsertLine : Couldn't find item to be updated\n");
                return;
            }
        }

        // If sort key changed, must delete and reinsert to make it relocate
        if (GetItemText(m_listCtrl, nIndex, 0) != str0)
        {
            m_listCtrl->DeleteItem(nIndex);
            nIndex = m_listCtrl->InsertItem(0, str0);
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

string FormatTxStatus(const CWalletTx& wtx)
{
    // Status
    int nDepth = wtx.GetDepthInMainChain();
    if (!wtx.IsFinal())
        return strprintf("Open for %d blocks", nBestHeight - wtx.nLockTime);
    else if (nDepth < 6)
        return strprintf("%d/unconfirmed", nDepth);
    else
        return strprintf("%d blocks", nDepth);
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

void CMainFrame::InsertTransaction(const CWalletTx& wtx, bool fNew, int nIndex)
{
    int64 nTime = wtx.nTimeDisplayed = wtx.GetTxTime();
    int64 nCredit = wtx.GetCredit();
    int64 nDebit = wtx.GetDebit();
    int64 nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    string strStatus = FormatTxStatus(wtx);
    map<string, string> mapValue = wtx.mapValue;

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
                    strDescription += strprintf(" (%s matures in %d more blocks)", FormatMoney(nUnmatured).c_str(), wtx.GetBlocksToMaturity());
                else
                    strDescription += " (not accepted)";
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
                       FormatMoney(nNet - nValue, true),
                       FormatMoney(nValue, true));
        }
        else if (fAllFromMe)
        {
            //
            // Debit
            //
            int64 nTxFee = nDebit - wtx.GetValueOut();
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
}

void CMainFrame::RefreshStatus()
{
    static int nLastTop;
    int nTop = m_listCtrl->GetTopItem();
    if (nTop == nLastTop && pindexBestLast == pindexBest)
        return;

    TRY_CRITICAL_BLOCK(cs_mapWallet)
    {
        int nStart = nTop;
        int nEnd = min(nStart + 100, m_listCtrl->GetItemCount());
        if (pindexBestLast == pindexBest)
        {
            if (nStart >= nLastTop && nStart < nLastTop + 100)
                nStart = nLastTop + 100;
            if (nEnd >= nLastTop && nEnd < nLastTop + 100)
                nEnd = nLastTop;
        }
        nLastTop = nTop;
        pindexBestLast = pindexBest;

        for (int nIndex = nStart; nIndex < nEnd; nIndex++)
        {
            uint256 hash((string)GetItemText(m_listCtrl, nIndex, 1));
            map<uint256, CWalletTx>::iterator mi = mapWallet.find(hash);
            if (mi == mapWallet.end())
            {
                printf("CMainFrame::RefreshStatus() : tx not found in mapWallet\n");
                continue;
            }
            const CWalletTx& wtx = (*mi).second;
            if (wtx.IsCoinBase() || wtx.GetTxTime() != wtx.nTimeDisplayed)
                InsertTransaction(wtx, false, nIndex);
            else
                m_listCtrl->SetItem(nIndex, 2, FormatTxStatus(wtx));
        }
    }
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

void CMainFrame::OnPaint(wxPaintEvent& event)
{
    event.Skip();
}

void CMainFrame::OnPaintListCtrl(wxPaintEvent& event)
{
    // Update listctrl contents
    if (!vWalletUpdated.empty())
    {
        TRY_CRITICAL_BLOCK(cs_mapWallet)
        {
            pair<uint256, bool> item;
            foreach(item, vWalletUpdated)
            {
                bool fNew = item.second;
                map<uint256, CWalletTx>::iterator mi = mapWallet.find(item.first);
                if (mi != mapWallet.end())
                {
                    printf("vWalletUpdated: %s %s\n", (*mi).second.GetHash().ToString().substr(0,6).c_str(), fNew ? "new" : "");
                    InsertTransaction((*mi).second, fNew);
                }
            }
            m_listCtrl->ScrollList(0, INT_MAX);
            vWalletUpdated.clear();
        }
    }

    // Update status column of visible items only
    RefreshStatus();

    // Update status bar
    string strGen = "";
    if (fGenerateBitcoins)
        strGen = "    Generating";
    if (fGenerateBitcoins && vNodes.empty())
        strGen = "(not connected)";
    m_statusBar->SetStatusText(strGen, 1);

    string strStatus = strprintf("     %d connections     %d blocks     %d transactions", vNodes.size(), nBestHeight + 1, m_listCtrl->GetItemCount());
    m_statusBar->SetStatusText(strStatus, 2);

    // Balance total
    TRY_CRITICAL_BLOCK(cs_mapWallet)
        m_staticTextBalance->SetLabel(FormatMoney(GetBalance()) + "  ");

    m_listCtrl->OnPaint(event);
}

void CrossThreadCall(wxCommandEvent& event)
{
    if (pframeMain)
        pframeMain->GetEventHandler()->AddPendingEvent(event);
}

void CrossThreadCall(int nID, void* pdata)
{
    wxCommandEvent event;
    event.SetInt(nID);
    event.SetClientData(pdata);
    if (pframeMain)
        pframeMain->GetEventHandler()->AddPendingEvent(event);
}

void CMainFrame::OnCrossThreadCall(wxCommandEvent& event)
{
    void* pdata = event.GetClientData();
    switch (event.GetInt())
    {
        case UICALL_ADDORDER:
        {
            break;
        }

        case UICALL_UPDATEORDER:
        {
            break;
        }
    }
}

void CMainFrame::OnMenuFileExit(wxCommandEvent& event)
{
    Close(true);
}

void CMainFrame::OnMenuOptionsGenerate(wxCommandEvent& event)
{
    fGenerateBitcoins = event.IsChecked();
    nTransactionsUpdated++;
    CWalletDB().WriteSetting("fGenerateBitcoins", fGenerateBitcoins);

    if (fGenerateBitcoins)
        if (_beginthread(ThreadBitcoinMiner, 0, NULL) == -1)
            printf("Error: _beginthread(ThreadBitcoinMiner) failed\n");

    Refresh();
    wxPaintEvent eventPaint;
    AddPendingEvent(eventPaint);
}

void CMainFrame::OnMenuOptionsChangeYourAddress(wxCommandEvent& event)
{
    OnButtonChange(event);
}

void CMainFrame::OnMenuOptionsOptions(wxCommandEvent& event)
{
    COptionsDialog dialog(this);
    dialog.ShowModal();
}

void CMainFrame::OnMenuHelpAbout(wxCommandEvent& event)
{
    CAboutDialog dialog(this);
    dialog.ShowModal();
}

void CMainFrame::OnButtonSend(wxCommandEvent& event)
{
    /// debug test
    if (fRandSendTest)
    {
        RandSend();
        return;
    }

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

void CMainFrame::OnListItemActivatedAllTransactions(wxListEvent& event)
{
    uint256 hash((string)GetItemText(m_listCtrl, event.GetIndex(), 1));
    CWalletTx wtx;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(hash);
        if (mi == mapWallet.end())
        {
            printf("CMainFrame::OnListItemActivatedAllTransactions() : tx not found in mapWallet\n");
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



    strHTML += "<b>Status:</b> " + FormatTxStatus(wtx) + "<br>";
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
                int64 nValue = wtx.vout[0].nValue;
                strHTML += "<b>Debit:</b> " + FormatMoney(-nValue) + "<br>";
                strHTML += "<b>Credit:</b> " + FormatMoney(nValue) + "<br>";
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
    m_textCtrlTransactionFee->SetValue(FormatMoney(nTransactionFee));
    m_buttonOK->SetFocus();
}

void COptionsDialog::OnKillFocusTransactionFee(wxFocusEvent& event)
{
    int64 nTmp = nTransactionFee;
    ParseMoney(m_textCtrlTransactionFee->GetValue(), nTmp);
    m_textCtrlTransactionFee->SetValue(FormatMoney(nTmp));
}

void COptionsDialog::OnButtonOK(wxCommandEvent& event)
{
    // nTransactionFee
    int64 nPrevTransactionFee = nTransactionFee;
    if (ParseMoney(m_textCtrlTransactionFee->GetValue(), nTransactionFee) && nTransactionFee != nPrevTransactionFee)
        CWalletDB().WriteSetting("nTransactionFee", nTransactionFee);

    Close();
}

void COptionsDialog::OnButtonCancel(wxCommandEvent& event)
{
    Close();
}






//////////////////////////////////////////////////////////////////////////////
//
// CAboutDialog
//

CAboutDialog::CAboutDialog(wxWindow* parent) : CAboutDialogBase(parent)
{
    m_staticTextVersion->SetLabel(strprintf("version 0.%d.%d Alpha", VERSION/100, VERSION%100));

    // Workaround until upgrade to wxWidgets supporting UTF-8
    wxString str = m_staticTextMain->GetLabel();
    if (str.Find('Â') != wxNOT_FOUND)
        str.Remove(str.Find('Â'), 1);
    m_staticTextMain->SetLabel(str);
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
    //// todo: should add a display of your balance for convenience

    // Set Icon
    wxBitmap bmpSend(wxT("send16"), wxBITMAP_TYPE_RESOURCE);
    bmpSend.SetMask(new wxMask(wxBitmap(wxT("send16masknoshadow"), wxBITMAP_TYPE_RESOURCE)));
    wxIcon iconSend;
    iconSend.CopyFromBitmap(bmpSend);
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
        CAddress addr(strAddress.c_str());
        if (addr.ip == 0)
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
    strStatus = "";
    fCanCancel = true;
    fAbort = false;
    fSuccess = false;
    fUIDone = false;
    fWorkDone = false;

    SetTitle(strprintf("Sending %s to %s...", FormatMoney(nPrice).c_str(), wtx.mapValue["to"].c_str()));
    m_textCtrlStatus->SetValue("");

    _beginthread(SendingDialogStartTransfer, 0, this);
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
    if (strStatus.size() > 130)
        m_textCtrlStatus->SetValue(string("\n") + strStatus);
    else
        m_textCtrlStatus->SetValue(string("\n\n") + strStatus);
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
        strStatus = "CANCELLED";
        m_buttonOK->Enable(true);
        m_buttonOK->SetFocus();
        m_buttonCancel->Enable(false);
        m_buttonCancel->SetLabel("Cancelled");
        Close();
        wxMessageBox("Transfer cancelled  ", "Sending...", wxOK, this);
    }
    event.Skip();

    /// debug test
    if (fRandSendTest && fWorkDone && fSuccess)
    {
        Close();
        Sleep(1000);
        RandSend();
    }
}


//
// Everything from here on is not in the UI thread and must only communicate
// with the rest of the dialog through variables and calling repaint.
//

void CSendingDialog::Repaint()
{
    Refresh();
    wxPaintEvent event;
    AddPendingEvent(event);
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
        strStatus = "CANCELLED";
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
    strStatus = str;
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
    CNode* pnode = ConnectNode(addr, 5 * 60);
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

    // Should already be connected
    CNode* pnode = ConnectNode(addr, 5 * 60);
    if (!pnode)
    {
        Error("Lost connection");
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
        int64 nFeeRequired;
        if (!CreateTransaction(scriptPubKey, nPrice, wtx, nFeeRequired))
        {
            if (nPrice + nFeeRequired > GetBalance())
                Error(strprintf("This is an oversized transaction that requires a transaction fee of %s", FormatMoney(nFeeRequired).c_str()));
            else
                Error("Transaction creation failed");
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
        if (!CommitTransactionSpent(wtx))
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
                  "The transaction is recorded and will credit to the recipient if it is valid,\n"
                  "but without comment information.");
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
    wxBitmap bmpAddressBook(wxT("addressbook16"), wxBITMAP_TYPE_RESOURCE);
    bmpAddressBook.SetMask(new wxMask(wxBitmap(wxT("addressbook16mask"), wxBITMAP_TYPE_RESOURCE)));
    wxIcon iconAddressBook;
    iconAddressBook.CopyFromBitmap(bmpAddressBook);
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
    _beginthread(ThreadRequestProductDetails, 0, new pair<CProduct, wxEvtHandler*>(product, GetEventHandler()));
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
    try
    {
        return OnInit2();
    }
    catch (std::exception& e) {
        PrintException(&e, "OnInit()");
    } catch (...) {
        PrintException(NULL, "OnInit()");
    }
    return false;
}

map<string, string> ParseParameters(int argc, char* argv[])
{
    map<string, string> mapArgs;
    for (int i = 0; i < argc; i++)
    {
        char psz[10000];
        strcpy(psz, argv[i]);
        char* pszValue = "";
        if (strchr(psz, '='))
        {
            pszValue = strchr(psz, '=');
            *pszValue++ = '\0';
        }
        strlwr(psz);
        if (psz[0] == '-')
            psz[0] = '/';
        mapArgs[psz] = pszValue;
    }
    return mapArgs;
}

bool CMyApp::OnInit2()
{
#ifdef _MSC_VER
    // Turn off microsoft heap dump noise for now
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFile("NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0));
#endif
#ifdef __WXDEBUG__
    // Disable malfunctioning wxWidgets debug assertion
    g_isPainting = 10000;
#endif

    //// debug print
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Bitcoin version %d, Windows version %08x\n", VERSION, GetVersion());

    //
    // Limit to single instance per user
    // Required to protect the database files if we're going to keep deleting log.*
    //
    wxString strMutexName = wxString("Bitcoin.") + getenv("HOMEPATH");
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

    //
    // Parameters
    //
    wxImage::AddHandler(new wxPNGHandler);
    map<string, string> mapArgs = ParseParameters(argc, argv);

    if (mapArgs.count("/datadir"))
        strSetDataDir = mapArgs["/datadir"];

    if (mapArgs.count("/proxy"))
        addrProxy = CAddress(mapArgs["/proxy"].c_str());

    if (mapArgs.count("/debug"))
        fDebug = true;

    if (mapArgs.count("/dropmessages"))
    {
        nDropMessagesTest = atoi(mapArgs["/dropmessages"]);
        if (nDropMessagesTest == 0)
            nDropMessagesTest = 20;
    }

    if (mapArgs.count("/loadblockindextest"))
    {
        CTxDB txdb("r");
        txdb.LoadBlockIndex();
        PrintBlockTree();
        ExitProcess(0);
    }

    //
    // Load data files
    //
    string strErrors;
    int64 nStart, nEnd;

    printf("Loading addresses...\n");
    QueryPerformanceCounter((LARGE_INTEGER*)&nStart);
    if (!LoadAddresses())
        strErrors += "Error loading addr.dat      \n";
    QueryPerformanceCounter((LARGE_INTEGER*)&nEnd);
    printf(" addresses   %20I64d\n", nEnd - nStart);

    printf("Loading block index...\n");
    QueryPerformanceCounter((LARGE_INTEGER*)&nStart);
    if (!LoadBlockIndex())
        strErrors += "Error loading blkindex.dat      \n";
    QueryPerformanceCounter((LARGE_INTEGER*)&nEnd);
    printf(" block index %20I64d\n", nEnd - nStart);

    printf("Loading wallet...\n");
    QueryPerformanceCounter((LARGE_INTEGER*)&nStart);
    if (!LoadWallet())
        strErrors += "Error loading wallet.dat      \n";
    QueryPerformanceCounter((LARGE_INTEGER*)&nEnd);
    printf(" wallet      %20I64d\n", nEnd - nStart);

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
        OnExit();
        return false;
    }

    // Add wallet transactions that aren't already in a block to mapTransactions
    ReacceptWalletTransactions();

    //
    // Parameters
    //
    if (mapArgs.count("/printblockindex") || mapArgs.count("/printblocktree"))
    {
        PrintBlockTree();
        OnExit();
        return false;
    }

    if (mapArgs.count("/gen"))
    {
        if (mapArgs["/gen"].empty())
            fGenerateBitcoins = true;
        else
            fGenerateBitcoins = atoi(mapArgs["/gen"].c_str());
    }

    //
    // Create the main frame window
    //
    {
        pframeMain = new CMainFrame(NULL);
        pframeMain->Show();

        if (!CheckDiskSpace())
        {
            OnExit();
            return false;
        }

        if (!StartNode(strErrors))
            wxMessageBox(strErrors, "Bitcoin");

        if (fGenerateBitcoins)
            if (_beginthread(ThreadBitcoinMiner, 0, NULL) == -1)
                printf("Error: _beginthread(ThreadBitcoinMiner) failed\n");

        //
        // Tests
        //
        if (argc >= 2 && stricmp(argv[1], "/send") == 0)
        {
            int64 nValue = 1;
            if (argc >= 3)
                ParseMoney(argv[2], nValue);

            string strAddress;
            if (argc >= 4)
                strAddress = argv[3];
            CAddress addr(strAddress.c_str());

            CWalletTx wtx;
            wtx.mapValue["to"] = strAddress;
            wtx.mapValue["from"] = addrLocalHost.ToString();
            wtx.mapValue["message"] = "command line send";

            // Send to IP address
            CSendingDialog* pdialog = new CSendingDialog(pframeMain, addr, nValue, wtx);
            if (!pdialog->ShowModal())
                return false;
        }

        if (mapArgs.count("/randsendtest"))
        {
            if (!mapArgs["/randsendtest"].empty())
                _beginthread(ThreadRandSendTest, 0, new string(mapArgs["/randsendtest"]));
            else
                fRandSendTest = true;
            fDebug = true;
        }
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
        wxLogWarning(_T("Exception %s %s"), typeid(e).name(), e.what());
        Sleep(1000);
        throw;
    }
    catch (...)
    {
        PrintException(NULL, "CMyApp::OnExceptionInMainLoop()");
        wxLogWarning(_T("Unknown exception"));
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
        wxLogWarning(_T("Exception %s %s"), typeid(e).name(), e.what());
        Sleep(1000);
        throw;
    }
    catch (...)
    {
        PrintException(NULL, "CMyApp::OnUnhandledException()");
        wxLogWarning(_T("Unknown exception"));
        Sleep(1000);
        throw;
    }
}

void CMyApp::OnFatalException()
{
    wxMessageBox("Program has crashed and will terminate.  ", "Bitcoin", wxOK | wxICON_ERROR);
}



void MainFrameRepaint()
{
    if (pframeMain)
    {
        printf("MainFrameRepaint()\n");
        wxPaintEvent event;
        pframeMain->Refresh();
        pframeMain->AddPendingEvent(event);
    }
}







// randsendtest to bitcoin address
void ThreadRandSendTest(void* parg)
{
    string strAddress = *(string*)parg;
    uint160 hash160;
    if (!AddressToHash160(strAddress, hash160))
    {
        wxMessageBox(strprintf("ThreadRandSendTest: Bitcoin address '%s' not valid  ", strAddress.c_str()));
        return;
    }

    loop
    {
        Sleep(GetRand(30) * 1000 + 100);

        // Message
        CWalletTx wtx;
        wtx.mapValue["to"] = strAddress;
        wtx.mapValue["from"] = addrLocalHost.ToString();
        static int nRep;
        wtx.mapValue["message"] = strprintf("randsendtest %d\n", ++nRep);

        // Value
        int64 nValue = (GetRand(9) + 1) * 100 * CENT;
        if (GetBalance() < nValue)
        {
            wxMessageBox("Out of money  ");
            return;
        }
        nValue += (nRep % 100) * CENT;

        // Send to bitcoin address
        CScript scriptPubKey;
        scriptPubKey << OP_DUP << OP_HASH160 << hash160 << OP_EQUALVERIFY << OP_CHECKSIG;

        if (!SendMoney(scriptPubKey, nValue, wtx))
            return;
    }
}


// randsendtest to any connected node
void RandSend()
{
    CWalletTx wtx;

    while (vNodes.empty())
        Sleep(1000);
    CAddress addr;
    CRITICAL_BLOCK(cs_vNodes)
        addr = vNodes[GetRand(vNodes.size())]->addr;

    // Message
    wtx.mapValue["to"] = addr.ToString();
    wtx.mapValue["from"] = addrLocalHost.ToString();
    static int nRep;
    wtx.mapValue["message"] = strprintf("randsendtest %d\n", ++nRep);

    // Value
    int64 nValue = (GetRand(999) + 1) * CENT;
    if (GetBalance() < nValue)
    {
        wxMessageBox("Out of money  ");
        return;
    }

    // Send to IP address
    CSendingDialog* pdialog = new CSendingDialog(pframeMain, addr, nValue, wtx);
    if (!pdialog->Show())
        wxMessageBox("ShowModal Failed  ");
}
