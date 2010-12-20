// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#endif



DEFINE_EVENT_TYPE(wxEVT_UITHREADCALL)

CMainFrame* pframeMain = NULL;
CMyTaskBarIcon* ptaskbaricon = NULL;
bool fClosedToTray = false;
wxLocale g_locale;









//////////////////////////////////////////////////////////////////////////////
//
// Util
//

void HandleCtrlA(wxKeyEvent& event)
{
    // Ctrl-a select all
    event.Skip();
    wxTextCtrl* textCtrl = (wxTextCtrl*)event.GetEventObject();
    if (event.GetModifiers() == wxMOD_CONTROL && event.GetKeyCode() == 'A')
        textCtrl->SetSelection(-1, -1);
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

void SetItemTextColour(wxListCtrl* listCtrl, int nIndex, const wxColour& colour)
{
    // Repaint on Windows is more flickery if the colour has ever been set,
    // so don't want to set it unless it's different.  Default colour has
    // alpha 0 transparent, so our colours don't match using operator==.
    wxColour c1 = listCtrl->GetItemTextColour(nIndex);
    if (!c1.IsOk())
        c1 = wxColour(0,0,0);
    if (colour.Red() != c1.Red() || colour.Green() != c1.Green() || colour.Blue() != c1.Blue())
        listCtrl->SetItemTextColour(nIndex, colour);
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
#ifdef __WXMSW__
    return wxMessageBox(message, caption, style, parent, x, y);
#else
    if (wxThread::IsMain() || fDaemon)
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

bool ThreadSafeAskFee(int64 nFeeRequired, const string& strCaption, wxWindow* parent)
{
    if (nFeeRequired < CENT || nFeeRequired <= nTransactionFee || fDaemon)
        return true;
    string strMessage = strprintf(
        _("This transaction is over the size limit.  You can still send it for a fee of %s, "
          "which goes to the nodes that process your transaction and helps to support the network.  "
          "Do you want to pay the fee?"),
        FormatMoney(nFeeRequired).c_str());
    return (ThreadSafeMessageBox(strMessage, strCaption, wxYES_NO, parent) == wxYES);
}

void CalledSetStatusBar(const string& strText, int nField)
{
    if (nField == 0 && GetWarnings("statusbar") != "")
        return;
    if (pframeMain && pframeMain->m_statusBar)
        pframeMain->m_statusBar->SetStatusText(strText, nField);
}

void SetDefaultReceivingAddress(const string& strAddress)
{
    // Update main window address and database
    if (pframeMain == NULL)
        return;
    if (strAddress != pframeMain->m_textCtrlAddress->GetValue())
    {
        uint160 hash160;
        if (!AddressToHash160(strAddress, hash160))
            return;
        if (!mapPubKeys.count(hash160))
            return;
        CWalletDB().WriteDefaultKey(mapPubKeys[hash160]);
        pframeMain->m_textCtrlAddress->SetValue(strAddress);
    }
}










//////////////////////////////////////////////////////////////////////////////
//
// CMainFrame
//

CMainFrame::CMainFrame(wxWindow* parent) : CMainFrameBase(parent)
{
    Connect(wxEVT_UITHREADCALL, wxCommandEventHandler(CMainFrame::OnUIThreadCall), NULL, this);

    // Set initially selected page
    wxNotebookEvent event;
    event.SetSelection(0);
    OnNotebookPageChanged(event);
    m_notebook->ChangeSelection(0);

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
    SetIcon(bitcoin80_xpm);
    SetBackgroundColour(m_toolBar->GetBackgroundColour());
    wxFont fontTmp = m_staticText41->GetFont();
    fontTmp.SetFamily(wxFONTFAMILY_TELETYPE);
    m_staticTextBalance->SetFont(fontTmp);
    m_staticTextBalance->SetSize(140, 17);
    // resize to fit ubuntu's huge default font
    dResize = 1.22;
    SetSize(dResize * GetSize().GetWidth(), 1.15 * GetSize().GetHeight());
#endif
    m_staticTextBalance->SetLabel(FormatMoney(GetBalance()) + "  ");
    m_listCtrl->SetFocus();
    ptaskbaricon = new CMyTaskBarIcon();
#ifdef __WXMAC_OSX__
    // Mac automatically moves wxID_EXIT, wxID_PREFERENCES and wxID_ABOUT
    // to their standard places, leaving these menus empty.
    GetMenuBar()->Remove(2); // remove Help menu
    GetMenuBar()->Remove(0); // remove File menu
#endif

    // Init column headers
    int nDateWidth = DateTimeStr(1229413914).size() * 6 + 8;
    if (!strstr(DateTimeStr(1229413914).c_str(), "2008"))
        nDateWidth += 12;
#ifdef __WXMAC_OSX__
    nDateWidth += 5;
    dResize -= 0.01;
#endif
    wxListCtrl* pplistCtrl[] = {m_listCtrlAll, m_listCtrlSentReceived, m_listCtrlSent, m_listCtrlReceived};
    foreach(wxListCtrl* p, pplistCtrl)
    {
        p->InsertColumn(0, "",               wxLIST_FORMAT_LEFT,  dResize * 0);
        p->InsertColumn(1, "",               wxLIST_FORMAT_LEFT,  dResize * 0);
        p->InsertColumn(2, _("Status"),      wxLIST_FORMAT_LEFT,  dResize * 112);
        p->InsertColumn(3, _("Date"),        wxLIST_FORMAT_LEFT,  dResize * nDateWidth);
        p->InsertColumn(4, _("Description"), wxLIST_FORMAT_LEFT,  dResize * 409 - nDateWidth);
        p->InsertColumn(5, _("Debit"),       wxLIST_FORMAT_RIGHT, dResize * 79);
        p->InsertColumn(6, _("Credit"),      wxLIST_FORMAT_RIGHT, dResize * 79);
    }

    // Init status bar
    int pnWidths[3] = { -100, 88, 300 };
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
}

void CMainFrame::OnNotebookPageChanged(wxNotebookEvent& event)
{
    event.Skip();
    nPage = event.GetSelection();
    if (nPage == ALL)
    {
        m_listCtrl = m_listCtrlAll;
        fShowGenerated = true;
        fShowSent = true;
        fShowReceived = true;
    }
    else if (nPage == SENTRECEIVED)
    {
        m_listCtrl = m_listCtrlSentReceived;
        fShowGenerated = false;
        fShowSent = true;
        fShowReceived = true;
    }
    else if (nPage == SENT)
    {
        m_listCtrl = m_listCtrlSent;
        fShowGenerated = false;
        fShowSent = true;
        fShowReceived = false;
    }
    else if (nPage == RECEIVED)
    {
        m_listCtrl = m_listCtrlReceived;
        fShowGenerated = false;
        fShowSent = false;
        fShowReceived = true;
    }
    RefreshListCtrl();
    m_listCtrl->SetFocus();
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
    event.Skip();
    // Hide the task bar button when minimized.
    // Event is sent when the frame is minimized or restored.
    // wxWidgets 2.8.9 doesn't have IsIconized() so there's no way
    // to get rid of the deprecated warning.  Just ignore it.
    if (!event.Iconized())
        fClosedToTray = false;
#if defined(__WXGTK__) || defined(__WXMAC_OSX__)
    if (GetBoolArg("-minimizetotray")) {
#endif
    // The tray icon sometimes disappears on ubuntu karmic
    // Hiding the taskbar button doesn't work cleanly on ubuntu lucid
    // Reports of CPU peg on 64-bit linux
    if (fMinimizeToTray && event.Iconized())
        fClosedToTray = true;
    Show(!fClosedToTray);
    ptaskbaricon->Show(fMinimizeToTray || fClosedToTray);
#if defined(__WXGTK__) || defined(__WXMAC_OSX__)
    }
#endif
}

void CMainFrame::OnMouseEvents(wxMouseEvent& event)
{
    event.Skip();
    RandAddSeed();
    RAND_add(&event.m_x, sizeof(event.m_x), 0.25);
    RAND_add(&event.m_y, sizeof(event.m_y), 0.25);
}

void CMainFrame::OnListColBeginDrag(wxListEvent& event)
{
    // Hidden columns not resizeable
    if (event.GetColumn() <= 1 && !fDebug)
        event.Veto();
    else
        event.Skip();
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

void CMainFrame::InsertLine(bool fNew, int nIndex, uint256 hashKey, string strSort, const wxColour& colour, const wxString& str2, const wxString& str3, const wxString& str4, const wxString& str5, const wxString& str6)
{
    strSort = " " + strSort;       // leading space to workaround wx2.9.0 ubuntu 9.10 bug
    long nData = *(long*)&hashKey; //  where first char of hidden column is displayed

    // Find item
    if (!fNew && nIndex == -1)
    {
        string strHash = " " + hashKey.ToString();
        while ((nIndex = m_listCtrl->FindItem(nIndex, nData)) != -1)
            if (GetItemText(m_listCtrl, nIndex, 1) == strHash)
                break;
    }

    // fNew is for blind insert, only use if you're sure it's new
    if (fNew || nIndex == -1)
    {
        nIndex = m_listCtrl->InsertItem(GetSortIndex(strSort), strSort);
    }
    else
    {
        // If sort key changed, must delete and reinsert to make it relocate
        if (GetItemText(m_listCtrl, nIndex, 0) != strSort)
        {
            m_listCtrl->DeleteItem(nIndex);
            nIndex = m_listCtrl->InsertItem(GetSortIndex(strSort), strSort);
        }
    }

    m_listCtrl->SetItem(nIndex, 1, " " + hashKey.ToString());
    m_listCtrl->SetItem(nIndex, 2, str2);
    m_listCtrl->SetItem(nIndex, 3, str3);
    m_listCtrl->SetItem(nIndex, 4, str4);
    m_listCtrl->SetItem(nIndex, 5, str5);
    m_listCtrl->SetItem(nIndex, 6, str6);
    m_listCtrl->SetItemData(nIndex, nData);
    SetItemTextColour(m_listCtrl, nIndex, colour);
}

bool CMainFrame::DeleteLine(uint256 hashKey)
{
    long nData = *(long*)&hashKey;

    // Find item
    int nIndex = -1;
    string strHash = " " + hashKey.ToString();
    while ((nIndex = m_listCtrl->FindItem(nIndex, nData)) != -1)
        if (GetItemText(m_listCtrl, nIndex, 1) == strHash)
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
            return strprintf(_("Open for %d blocks"), nBestHeight - wtx.nLockTime);
        else
            return strprintf(_("Open until %s"), DateTimeStr(wtx.nLockTime).c_str());
    }
    else
    {
        int nDepth = wtx.GetDepthInMainChain();
        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
            return strprintf(_("%d/offline?"), nDepth);
        else if (nDepth < 6)
            return strprintf(_("%d/unconfirmed"), nDepth);
        else
            return strprintf(_("%d confirmations"), nDepth);
    }
}

string SingleLine(const string& strIn)
{
    string strOut;
    bool fOneSpace = false;
    foreach(unsigned char c, strIn)
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
    int64 nCredit = wtx.GetCredit(true);
    int64 nDebit = wtx.GetDebit();
    int64 nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    string strStatus = FormatTxStatus(wtx);
    bool fConfirmed = wtx.fConfirmedDisplayed = wtx.IsConfirmed();
    wxColour colour = (fConfirmed ? wxColour(0,0,0) : wxColour(128,128,128));
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
            // Generated
            strDescription = _("Generated");
            if (nCredit == 0)
            {
                int64 nUnmatured = 0;
                foreach(const CTxOut& txout, wtx.vout)
                    nUnmatured += txout.GetCredit();
                if (wtx.IsInMainChain())
                {
                    strDescription = strprintf(_("Generated (%s matures in %d more blocks)"), FormatMoney(nUnmatured).c_str(), wtx.GetBlocksToMaturity());

                    // Check if the block was requested by anyone
                    if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                        strDescription = _("Generated - Warning: This block was not received by any other nodes and will probably not be accepted!");
                }
                else
                {
                    strDescription = _("Generated (not accepted)");
                }
            }
        }
        else if (!mapValue["from"].empty() || !mapValue["message"].empty())
        {
            // Received by IP connection
            if (!fShowReceived)
                return false;
            if (!mapValue["from"].empty())
                strDescription += _("From: ") + mapValue["from"];
            if (!mapValue["message"].empty())
            {
                if (!strDescription.empty())
                    strDescription += " - ";
                strDescription += mapValue["message"];
            }
        }
        else
        {
            // Received by Bitcoin Address
            if (!fShowReceived)
                return false;
            foreach(const CTxOut& txout, wtx.vout)
            {
                if (txout.IsMine())
                {
                    vector<unsigned char> vchPubKey;
                    if (ExtractPubKey(txout.scriptPubKey, true, vchPubKey))
                    {
                        CRITICAL_BLOCK(cs_mapAddressBook)
                        {
                            //strDescription += _("Received payment to ");
                            //strDescription += _("Received with address ");
                            strDescription += _("Received with: ");
                            string strAddress = PubKeyToAddress(vchPubKey);
                            map<string, string>::iterator mi = mapAddressBook.find(strAddress);
                            if (mi != mapAddressBook.end() && !(*mi).second.empty())
                            {
                                string strLabel = (*mi).second;
                                strDescription += strAddress.substr(0,12) + "... ";
                                strDescription += "(" + strLabel + ")";
                            }
                            else
                                strDescription += strAddress;
                        }
                    }
                    break;
                }
            }
        }

        string strCredit = FormatMoney(nNet, true);
        if (!fConfirmed)
            strCredit = "[" + strCredit + "]";

        InsertLine(fNew, nIndex, hash, strSort, colour,
                   strStatus,
                   nTime ? DateTimeStr(nTime) : "",
                   SingleLine(strDescription),
                   "",
                   strCredit);
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
            int64 nChange = wtx.GetChange();
            InsertLine(fNew, nIndex, hash, strSort, colour,
                       strStatus,
                       nTime ? DateTimeStr(nTime) : "",
                       _("Payment to yourself"),
                       FormatMoney(-(nDebit - nChange), true),
                       FormatMoney(nCredit - nChange, true));
        }
        else if (fAllFromMe)
        {
            //
            // Debit
            //
            if (!fShowSent)
                return false;

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
                    // Sent to IP
                    strAddress = mapValue["to"];
                }
                else
                {
                    // Sent to Bitcoin Address
                    uint160 hash160;
                    if (ExtractHash160(txout.scriptPubKey, hash160))
                        strAddress = Hash160ToAddress(hash160);
                }

                string strDescription = _("To: ");
                CRITICAL_BLOCK(cs_mapAddressBook)
                    if (mapAddressBook.count(strAddress) && !mapAddressBook[strAddress].empty())
                        strDescription += mapAddressBook[strAddress] + " ";
                strDescription += strAddress;
                if (!mapValue["message"].empty())
                {
                    if (!strDescription.empty())
                        strDescription += " - ";
                    strDescription += mapValue["message"];
                }
                else if (!mapValue["comment"].empty())
                {
                    if (!strDescription.empty())
                        strDescription += " - ";
                    strDescription += mapValue["comment"];
                }

                int64 nValue = txout.nValue;
                if (nTxFee > 0)
                {
                    nValue += nTxFee;
                    nTxFee = 0;
                }

                InsertLine(fNew, nIndex, hash, strprintf("%s-%d", strSort.c_str(), nOut), colour,
                           strStatus,
                           nTime ? DateTimeStr(nTime) : "",
                           SingleLine(strDescription),
                           FormatMoney(-nValue, true),
                           "");
                nIndex = -1;
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

            InsertLine(fNew, nIndex, hash, strSort, colour,
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
            if (wtx.IsCoinBase() ||
                wtx.GetTxTime() != wtx.nTimeDisplayed ||
                wtx.IsConfirmed() != wtx.fConfirmedDisplayed)
            {
                if (!InsertTransaction(wtx, false, nIndex))
                    m_listCtrl->DeleteItem(nIndex--);
            }
            else
            {
                m_listCtrl->SetItem(nIndex, 2, FormatTxStatus(wtx));
            }
        }
    }
}

void CMainFrame::OnPaint(wxPaintEvent& event)
{
    event.Skip();
    if (fRefresh)
    {
        fRefresh = false;
        Refresh();
    }
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
    // Skip lets the listctrl do the paint, we're just hooking the message
    event.Skip();

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
    static string strPrevWarning;
    string strWarning = GetWarnings("statusbar");
    if (strWarning != "")
        m_statusBar->SetStatusText(string("    ") + _(strWarning), 0);
    else if (strPrevWarning != "")
        m_statusBar->SetStatusText("", 0);
    strPrevWarning = strWarning;

    string strGen = "";
    if (fGenerateBitcoins)
        strGen = _("    Generating");
    if (fGenerateBitcoins && vNodes.empty())
        strGen = _("(not connected)");
    m_statusBar->SetStatusText(strGen, 1);

    string strStatus = strprintf(_("     %d connections     %d blocks     %d transactions"), vNodes.size(), nBestHeight, nTransactionCount);
    m_statusBar->SetStatusText(strStatus, 2);

    // Update receiving address
    string strDefaultAddress = PubKeyToAddress(vchDefaultKey);
    if (m_textCtrlAddress->GetValue() != strDefaultAddress)
        m_textCtrlAddress->SetValue(strDefaultAddress);
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
    // Options->Your Receiving Addresses
    CAddressBookDialog dialog(this, "", CAddressBookDialog::RECEIVING, false);
    if (!dialog.ShowModal())
        return;
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
    CAddressBookDialog dialogAddr(this, "", CAddressBookDialog::SENDING, false);
    if (dialogAddr.ShowModal() == 2)
    {
        // Send
        CSendDialog dialogSend(this, dialogAddr.GetSelectedAddress());
        dialogSend.ShowModal();
    }
}

void CMainFrame::OnSetFocusAddress(wxFocusEvent& event)
{
    // Automatically select-all when entering window
    event.Skip();
    m_textCtrlAddress->SetSelection(-1, -1);
    fOnSetFocusAddress = true;
}

void CMainFrame::OnMouseEventsAddress(wxMouseEvent& event)
{
    event.Skip();
    if (fOnSetFocusAddress)
        m_textCtrlAddress->SetSelection(-1, -1);
    fOnSetFocusAddress = false;
}

void CMainFrame::OnButtonNew(wxCommandEvent& event)
{
    // Ask name
    CGetTextFromUserDialog dialog(this,
        _("New Receiving Address"),
        _("You should use a new address for each payment you receive.\n\nLabel"),
        "");
    if (!dialog.ShowModal())
        return;
    string strName = dialog.GetValue();

    // Generate new key
    string strAddress = PubKeyToAddress(GetKeyFromKeyPool());

    // Save
    SetAddressBookName(strAddress, strName);
    SetDefaultReceivingAddress(strAddress);
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






//////////////////////////////////////////////////////////////////////////////
//
// CTxDetailsDialog
//

CTxDetailsDialog::CTxDetailsDialog(wxWindow* parent, CWalletTx wtx) : CTxDetailsDialogBase(parent)
{
    CRITICAL_BLOCK(cs_mapAddressBook)
    {
        string strHTML;
        strHTML.reserve(4000);
        strHTML += "<html><font face='verdana, arial, helvetica, sans-serif'>";

        int64 nTime = wtx.GetTxTime();
        int64 nCredit = wtx.GetCredit();
        int64 nDebit = wtx.GetDebit();
        int64 nNet = nCredit - nDebit;



        strHTML += _("<b>Status:</b> ") + FormatTxStatus(wtx);
        int nRequests = wtx.GetRequestCount();
        if (nRequests != -1)
        {
            if (nRequests == 0)
                strHTML += _(", has not been successfully broadcast yet");
            else if (nRequests == 1)
                strHTML += strprintf(_(", broadcast through %d node"), nRequests);
            else
                strHTML += strprintf(_(", broadcast through %d nodes"), nRequests);
        }
        strHTML += "<br>";

        strHTML += _("<b>Date:</b> ") + (nTime ? DateTimeStr(nTime) : "") + "<br>";


        //
        // From
        //
        if (wtx.IsCoinBase())
        {
            strHTML += _("<b>Source:</b> Generated<br>");
        }
        else if (!wtx.mapValue["from"].empty())
        {
            // Online transaction
            if (!wtx.mapValue["from"].empty())
                strHTML += _("<b>From:</b> ") + HtmlEscape(wtx.mapValue["from"]) + "<br>";
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
                                strHTML += string() + _("<b>From:</b> ") + _("unknown") + "<br>";
                                strHTML += _("<b>To:</b> ");
                                strHTML += HtmlEscape(strAddress);
                                if (!mapAddressBook[strAddress].empty())
                                    strHTML += _(" (yours, label: ") + mapAddressBook[strAddress] + ")";
                                else
                                    strHTML += _(" (yours)");
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
            strHTML += _("<b>To:</b> ");
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
            strHTML += _("<b>Credit:</b> ");
            if (wtx.IsInMainChain())
                strHTML += strprintf(_("(%s matures in %d more blocks)"), FormatMoney(nUnmatured).c_str(), wtx.GetBlocksToMaturity());
            else
                strHTML += _("(not accepted)");
            strHTML += "<br>";
        }
        else if (nNet > 0)
        {
            //
            // Credit
            //
            strHTML += _("<b>Credit:</b> ") + FormatMoney(nNet) + "<br>";
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
                            strHTML += _("<b>To:</b> ");
                            if (mapAddressBook.count(strAddress) && !mapAddressBook[strAddress].empty())
                                strHTML += mapAddressBook[strAddress] + " ";
                            strHTML += strAddress;
                            strHTML += "<br>";
                        }
                    }

                    strHTML += _("<b>Debit:</b> ") + FormatMoney(-txout.nValue) + "<br>";
                }

                if (fAllToMe)
                {
                    // Payment to self
                    int64 nChange = wtx.GetChange();
                    int64 nValue = nCredit - nChange;
                    strHTML += _("<b>Debit:</b> ") + FormatMoney(-nValue) + "<br>";
                    strHTML += _("<b>Credit:</b> ") + FormatMoney(nValue) + "<br>";
                }

                int64 nTxFee = nDebit - wtx.GetValueOut();
                if (nTxFee > 0)
                    strHTML += _("<b>Transaction fee:</b> ") + FormatMoney(-nTxFee) + "<br>";
            }
            else
            {
                //
                // Mixed debit transaction
                //
                foreach(const CTxIn& txin, wtx.vin)
                    if (txin.IsMine())
                        strHTML += _("<b>Debit:</b> ") + FormatMoney(-txin.GetDebit()) + "<br>";
                foreach(const CTxOut& txout, wtx.vout)
                    if (txout.IsMine())
                        strHTML += _("<b>Credit:</b> ") + FormatMoney(txout.GetCredit()) + "<br>";
            }
        }

        strHTML += _("<b>Net amount:</b> ") + FormatMoney(nNet, true) + "<br>";


        //
        // Message
        //
        if (!wtx.mapValue["message"].empty())
            strHTML += string() + "<br><b>" + _("Message:") + "</b><br>" + HtmlEscape(wtx.mapValue["message"], true) + "<br>";
        if (!wtx.mapValue["comment"].empty())
            strHTML += string() + "<br><b>" + _("Comment:") + "</b><br>" + HtmlEscape(wtx.mapValue["comment"], true) + "<br>";

        if (wtx.IsCoinBase())
            strHTML += string() + "<br>" + _("Generated coins must wait 120 blocks before they can be spent.  When you generated this block, it was broadcast to the network to be added to the block chain.  If it fails to get into the chain, it will change to \"not accepted\" and not be spendable.  This may occasionally happen if another node generates a block within a few seconds of yours.") + "<br>";


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

            strHTML += "<br><b>Transaction:</b><br>";
            strHTML += HtmlEscape(wtx.ToString(), true);

            strHTML += "<br><b>Inputs:</b><br>";
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
        }



        strHTML += "</font></html>";
        string(strHTML.begin(), strHTML.end()).swap(strHTML);
        m_htmlWin->SetPage(strHTML);
        m_buttonOK->SetFocus();
    }
}

void CTxDetailsDialog::OnButtonOK(wxCommandEvent& event)
{
    EndModal(false);
}






//////////////////////////////////////////////////////////////////////////////
//
// Startup folder
//

#ifdef __WXMSW__
string StartupShortcutPath()
{
    return MyGetSpecialFolderPath(CSIDL_STARTUP, true) + "\\Bitcoin.lnk";
}

bool GetStartOnSystemStartup()
{
    return filesystem::exists(StartupShortcutPath().c_str());
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
            TCHAR pszExePath[MAX_PATH];
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

#elif defined(__WXGTK__)

// Follow the Desktop Application Autostart Spec:
//  http://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html

boost::filesystem::path GetAutostartDir()
{
    namespace fs = boost::filesystem;

    char* pszConfigHome = getenv("XDG_CONFIG_HOME");
    if (pszConfigHome) return fs::path(pszConfigHome) / fs::path("autostart");
    char* pszHome = getenv("HOME");
    if (pszHome) return fs::path(pszHome) / fs::path(".config/autostart");
    return fs::path();
}

boost::filesystem::path GetAutostartFilePath()
{
    return GetAutostartDir() / boost::filesystem::path("bitcoin.desktop");
}

bool GetStartOnSystemStartup()
{
    boost::filesystem::ifstream optionFile(GetAutostartFilePath());
    if (!optionFile.good())
        return false;
    // Scan through file for "Hidden=true":
    string line;
    while (!optionFile.eof())
    {
        getline(optionFile, line);
        if (line.find("Hidden") != string::npos &&
            line.find("true") != string::npos)
            return false;
    }
    optionFile.close();

    return true;
}

void SetStartOnSystemStartup(bool fAutoStart)
{
    if (!fAutoStart)
    {
        unlink(GetAutostartFilePath().native_file_string().c_str());
    }
    else
    {
        char pszExePath[MAX_PATH+1];
        memset(pszExePath, 0, sizeof(pszExePath));
        if (readlink("/proc/self/exe", pszExePath, sizeof(pszExePath)-1) == -1)
            return;

        boost::filesystem::create_directories(GetAutostartDir());

        boost::filesystem::ofstream optionFile(GetAutostartFilePath(), ios_base::out|ios_base::trunc);
        if (!optionFile.good())
        {
            wxMessageBox(_("Cannot write autostart/bitcoin.desktop file"), "Bitcoin");
            return;
        }
        // Write a bitcoin.desktop file to the autostart directory:
        optionFile << "[Desktop Entry]\n";
        optionFile << "Type=Application\n";
        optionFile << "Name=Bitcoin\n";
        optionFile << "Exec=" << pszExePath << "\n";
        optionFile << "Terminal=false\n";
        optionFile << "Hidden=false\n";
        optionFile.close();
    }
}
#else

// TODO: OSX startup stuff; see:
// http://developer.apple.com/mac/library/documentation/MacOSX/Conceptual/BPSystemStartup/Articles/CustomLogin.html

bool GetStartOnSystemStartup() { return false; }
void SetStartOnSystemStartup(bool fAutoStart) { }

#endif






//////////////////////////////////////////////////////////////////////////////
//
// COptionsDialog
//

COptionsDialog::COptionsDialog(wxWindow* parent) : COptionsDialogBase(parent)
{
    // Set up list box of page choices
    m_listBox->Append(_("Main"));
    //m_listBox->Append(_("Test 2"));
    m_listBox->SetSelection(0);
    SelectPage(0);
#ifndef __WXMSW__
    SetSize(1.0 * GetSize().GetWidth(), 1.2 * GetSize().GetHeight());
#endif
#if defined(__WXGTK__) || defined(__WXMAC_OSX__)
    m_checkBoxStartOnSystemStartup->SetLabel(_("&Start Bitcoin on window system startup"));
    if (!GetBoolArg("-minimizetotray"))
    {
        // Minimize to tray is just too buggy on Linux
        fMinimizeToTray = false;
        m_checkBoxMinimizeToTray->SetValue(false);
        m_checkBoxMinimizeToTray->Enable(false);
        m_checkBoxMinimizeOnClose->SetLabel(_("&Minimize on close"));
    }
#endif
#ifdef __WXMAC_OSX__
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
    event.Skip();
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
    event.Skip();
    m_textCtrlProxyIP->SetValue(GetProxyAddr().ToStringIP());
    m_textCtrlProxyPort->SetValue(GetProxyAddr().ToStringPort());
}


void COptionsDialog::OnButtonOK(wxCommandEvent& event)
{
    OnButtonApply(event);
    EndModal(false);
}

void COptionsDialog::OnButtonCancel(wxCommandEvent& event)
{
    EndModal(false);
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
    m_staticTextVersion->SetLabel(strprintf(_("version %s%s BETA"), FormatVersion(VERSION).c_str(), pszSubVer));

    // Change (c) into UTF-8 or ANSI copyright symbol
    wxString str = m_staticTextMain->GetLabel();
#if wxUSE_UNICODE
    str.Replace("(c)", wxString::FromUTF8("\xC2\xA9"));
#else
    str.Replace("(c)", "\xA9");
#endif
    m_staticTextMain->SetLabel(str);
#ifndef __WXMSW__
    // Resize on Linux to make the window fit the text.
    // The text was wrapped manually rather than using the Wrap setting because
    // the wrap would be too small on Linux and it can't be changed at this point.
    wxFont fontTmp = m_staticTextMain->GetFont();
    if (fontTmp.GetPointSize() > 8);
        fontTmp.SetPointSize(8);
    m_staticTextMain->SetFont(fontTmp);
    SetSize(GetSize().GetWidth() + 44, GetSize().GetHeight() + 10);
#endif
}

void CAboutDialog::OnButtonOK(wxCommandEvent& event)
{
    EndModal(false);
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
    event.Skip();
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
        m_textCtrlFrom->SetValue(_("n/a"));
        m_textCtrlMessage->SetValue(_("Can't include a message when sending to a Bitcoin address"));
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
    event.Skip();
    if (m_textCtrlAmount->GetValue().Trim().empty())
        return;
    int64 nTmp;
    if (ParseMoney(m_textCtrlAmount->GetValue(), nTmp))
        m_textCtrlAmount->SetValue(FormatMoney(nTmp));
}

void CSendDialog::OnButtonAddressBook(wxCommandEvent& event)
{
    // Open address book
    CAddressBookDialog dialog(this, m_textCtrlAddress->GetValue(), CAddressBookDialog::SENDING, true);
    if (dialog.ShowModal())
        m_textCtrlAddress->SetValue(dialog.GetSelectedAddress());
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
    static CCriticalSection cs_sendlock;
    TRY_CRITICAL_BLOCK(cs_sendlock)
    {
        CWalletTx wtx;
        string strAddress = (string)m_textCtrlAddress->GetValue();

        // Parse amount
        int64 nValue = 0;
        if (!ParseMoney(m_textCtrlAmount->GetValue(), nValue) || nValue <= 0)
        {
            wxMessageBox(_("Error in amount  "), _("Send Coins"));
            return;
        }
        if (nValue > GetBalance())
        {
            wxMessageBox(_("Amount exceeds your balance  "), _("Send Coins"));
            return;
        }
        if (nValue + nTransactionFee > GetBalance())
        {
            wxMessageBox(string(_("Total exceeds your balance when the ")) + FormatMoney(nTransactionFee) + _(" transaction fee is included  "), _("Send Coins"));
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

            string strError = SendMoney(scriptPubKey, nValue, wtx, true);
            if (strError == "")
                wxMessageBox(_("Payment sent  "), _("Sending..."));
            else if (strError == "ABORTED")
                return; // leave send dialog open
            else
            {
                wxMessageBox(strError + "  ", _("Sending..."));
                EndModal(false);
            }
        }
        else
        {
            // Parse IP address
            CAddress addr(strAddress);
            if (!addr.IsValid())
            {
                wxMessageBox(_("Invalid address  "), _("Send Coins"));
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

        CRITICAL_BLOCK(cs_mapAddressBook)
            if (!mapAddressBook.count(strAddress))
                SetAddressBookName(strAddress, "");

        EndModal(true);
    }
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
    SetSize(1.2 * GetSize().GetWidth(), 1.08 * GetSize().GetHeight());
#endif

    SetTitle(strprintf(_("Sending %s to %s"), FormatMoney(nPrice).c_str(), wtx.mapValue["to"].c_str()));
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
    event.Skip();
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
        strcpy(pszStatus, _("CANCELLED"));
        m_buttonOK->Enable(true);
        m_buttonOK->SetFocus();
        m_buttonCancel->Enable(false);
        m_buttonCancel->SetLabel(_("Cancelled"));
        Close();
        wxMessageBox(_("Transfer cancelled  "), _("Sending..."), wxOK, this);
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
        strcpy(pszStatus, _("CANCELLED"));
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
    Status(string(_("Error: ")) + str);
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
        Error(_("Insufficient funds"));
        return;
    }

    // We may have connected already for product details
    if (!Status(_("Connecting...")))
        return;
    CNode* pnode = ConnectNode(addr, 15 * 60);
    if (!pnode)
    {
        Error(_("Unable to connect"));
        return;
    }

    // Send order to seller, with response going to OnReply2 via event handler
    if (!Status(_("Requesting public key...")))
        return;
    pnode->PushRequest("checkorder", wtx, SendingDialogOnReply2, this);
}

void SendingDialogOnReply2(void* parg, CDataStream& vRecv)
{
    ((CSendingDialog*)parg)->OnReply2(vRecv);
}

void CSendingDialog::OnReply2(CDataStream& vRecv)
{
    if (!Status(_("Received public key...")))
        return;

    CScript scriptPubKey;
    int nRet;
    try
    {
        vRecv >> nRet;
        if (nRet > 0)
        {
            string strMessage;
            if (!vRecv.empty())
                vRecv >> strMessage;
            if (nRet == 2)
                Error(_("Recipient is not accepting transactions sent by IP address"));
            else
                Error(_("Transfer was not accepted"));
            //// todo: enlarge the window and enable a hidden white box to put seller's message
            return;
        }
        vRecv >> scriptPubKey;
    }
    catch (...)
    {
        //// what do we want to do about this?
        Error(_("Invalid response received"));
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
        if (!Status(_("Creating transaction...")))
            return;
        if (nPrice + nTransactionFee > GetBalance())
        {
            Error(_("Insufficient funds"));
            return;
        }
        CReserveKey reservekey;
        int64 nFeeRequired;
        if (!CreateTransaction(scriptPubKey, nPrice, wtx, reservekey, nFeeRequired))
        {
            if (nPrice + nFeeRequired > GetBalance())
                Error(strprintf(_("This is an oversized transaction that requires a transaction fee of %s"), FormatMoney(nFeeRequired).c_str()));
            else
                Error(_("Transaction creation failed"));
            return;
        }

        // Transaction fee
        if (!ThreadSafeAskFee(nFeeRequired, _("Sending..."), this))
        {
            Error(_("Transaction aborted"));
            return;
        }

        // Make sure we're still connected
        CNode* pnode = ConnectNode(addr, 2 * 60 * 60);
        if (!pnode)
        {
            Error(_("Lost connection, transaction cancelled"));
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
        if (!Status(_("Sending payment...")))
            return;

        // Commit
        if (!CommitTransaction(wtx, reservekey))
        {
            Error(_("The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."));
            return;
        }

        // Send payment tx to seller, with response going to OnReply3 via event handler
        CWalletTx wtxSend = wtx;
        wtxSend.fFromMe = false;
        pnode->PushRequest("submitorder", wtxSend, SendingDialogOnReply3, this);

        Status(_("Waiting for confirmation..."));
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
            Error(_("The payment was sent, but the recipient was unable to verify it.\n"
                    "The transaction is recorded and will credit to the recipient,\n"
                    "but the comment information will be blank."));
            return;
        }
    }
    catch (...)
    {
        //// what do we want to do about this?
        Error(_("Payment was sent, but an invalid response was received"));
        return;
    }

    fSuccess = true;
    fWorkDone = true;
    Status(_("Payment completed"));
}






//////////////////////////////////////////////////////////////////////////////
//
// CAddressBookDialog
//

CAddressBookDialog::CAddressBookDialog(wxWindow* parent, const wxString& strInitSelected, int nPageIn, bool fDuringSendIn) : CAddressBookDialogBase(parent)
{
    // Set initially selected page
    wxNotebookEvent event;
    event.SetSelection(nPageIn);
    OnNotebookPageChanged(event);
    m_notebook->ChangeSelection(nPageIn);

    fDuringSend = fDuringSendIn;
    if (!fDuringSend)
        m_buttonCancel->Show(false);

    // Set Icon
    wxIcon iconAddressBook;
    iconAddressBook.CopyFromBitmap(wxBitmap(addressbook16_xpm));
    SetIcon(iconAddressBook);

    // Init column headers
    m_listCtrlSending->InsertColumn(0, _("Name"), wxLIST_FORMAT_LEFT, 200);
    m_listCtrlSending->InsertColumn(1, _("Address"), wxLIST_FORMAT_LEFT, 350);
    m_listCtrlSending->SetFocus();
    m_listCtrlReceiving->InsertColumn(0, _("Label"), wxLIST_FORMAT_LEFT, 200);
    m_listCtrlReceiving->InsertColumn(1, _("Bitcoin Address"), wxLIST_FORMAT_LEFT, 350);
    m_listCtrlReceiving->SetFocus();

    // Fill listctrl with address book data
    CRITICAL_BLOCK(cs_mapKeys)
    CRITICAL_BLOCK(cs_mapAddressBook)
    {
        string strDefaultReceiving = (string)pframeMain->m_textCtrlAddress->GetValue();
        foreach(const PAIRTYPE(string, string)& item, mapAddressBook)
        {
            string strAddress = item.first;
            string strName = item.second;
            uint160 hash160;
            bool fMine = (AddressToHash160(strAddress, hash160) && mapPubKeys.count(hash160));
            wxListCtrl* plistCtrl = fMine ? m_listCtrlReceiving : m_listCtrlSending;
            int nIndex = InsertLine(plistCtrl, strName, strAddress);
            if (strAddress == (fMine ? strDefaultReceiving : string(strInitSelected)))
                plistCtrl->SetItemState(nIndex, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
        }
    }
}

wxString CAddressBookDialog::GetSelectedAddress()
{
    int nIndex = GetSelection(m_listCtrl);
    if (nIndex == -1)
        return "";
    return GetItemText(m_listCtrl, nIndex, 1);
}

wxString CAddressBookDialog::GetSelectedSendingAddress()
{
    int nIndex = GetSelection(m_listCtrlSending);
    if (nIndex == -1)
        return "";
    return GetItemText(m_listCtrlSending, nIndex, 1);
}

wxString CAddressBookDialog::GetSelectedReceivingAddress()
{
    int nIndex = GetSelection(m_listCtrlReceiving);
    if (nIndex == -1)
        return "";
    return GetItemText(m_listCtrlReceiving, nIndex, 1);
}

void CAddressBookDialog::OnNotebookPageChanged(wxNotebookEvent& event)
{
    event.Skip();
    nPage = event.GetSelection();
    if (nPage == SENDING)
        m_listCtrl = m_listCtrlSending;
    else if (nPage == RECEIVING)
        m_listCtrl = m_listCtrlReceiving;
    m_buttonDelete->Show(nPage == SENDING);
    m_buttonCopy->Show(nPage == RECEIVING);
    this->Layout();
    m_listCtrl->SetFocus();
}

void CAddressBookDialog::OnListEndLabelEdit(wxListEvent& event)
{
    // Update address book with edited name
    event.Skip();
    if (event.IsEditCancelled())
        return;
    string strAddress = (string)GetItemText(m_listCtrl, event.GetIndex(), 1);
    SetAddressBookName(strAddress, string(event.GetText()));
    pframeMain->RefreshListCtrl();
}

void CAddressBookDialog::OnListItemSelected(wxListEvent& event)
{
    event.Skip();
    if (nPage == RECEIVING)
        SetDefaultReceivingAddress((string)GetSelectedReceivingAddress());
}

void CAddressBookDialog::OnListItemActivated(wxListEvent& event)
{
    event.Skip();
    if (fDuringSend)
    {
        // Doubleclick returns selection
        EndModal(GetSelectedAddress() != "" ? 2 : 0);
        return;
    }

    // Doubleclick edits item
    wxCommandEvent event2;
    OnButtonEdit(event2);
}

void CAddressBookDialog::OnButtonDelete(wxCommandEvent& event)
{
    if (nPage != SENDING)
        return;
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
        wxTheClipboard->SetData(new wxTextDataObject(GetSelectedAddress()));
        wxTheClipboard->Close();
    }
}

bool CAddressBookDialog::CheckIfMine(const string& strAddress, const string& strTitle)
{
    uint160 hash160;
    bool fMine = (AddressToHash160(strAddress, hash160) && mapPubKeys.count(hash160));
    if (fMine)
        wxMessageBox(_("This is one of your own addresses for receiving payments and cannot be entered in the address book.  "), strTitle);
    return fMine;
}

void CAddressBookDialog::OnButtonEdit(wxCommandEvent& event)
{
    int nIndex = GetSelection(m_listCtrl);
    if (nIndex == -1)
        return;
    string strName = (string)m_listCtrl->GetItemText(nIndex);
    string strAddress = (string)GetItemText(m_listCtrl, nIndex, 1);
    string strAddressOrg = strAddress;

    if (nPage == SENDING)
    {
        // Ask name and address
        do
        {
            CGetTextFromUserDialog dialog(this, _("Edit Address"), _("Name"), strName, _("Address"), strAddress);
            if (!dialog.ShowModal())
                return;
            strName = dialog.GetValue1();
            strAddress = dialog.GetValue2();
        }
        while (CheckIfMine(strAddress, _("Edit Address")));

    }
    else if (nPage == RECEIVING)
    {
        // Ask name
        CGetTextFromUserDialog dialog(this, _("Edit Address Label"), _("Label"), strName);
        if (!dialog.ShowModal())
            return;
        strName = dialog.GetValue();
    }

    // Write back
    if (strAddress != strAddressOrg)
        CWalletDB().EraseName(strAddressOrg);
    SetAddressBookName(strAddress, strName);
    m_listCtrl->SetItem(nIndex, 1, strAddress);
    m_listCtrl->SetItemText(nIndex, strName);
    pframeMain->RefreshListCtrl();
}

void CAddressBookDialog::OnButtonNew(wxCommandEvent& event)
{
    string strName;
    string strAddress;

    if (nPage == SENDING)
    {
        // Ask name and address
        do
        {
            CGetTextFromUserDialog dialog(this, _("Add Address"), _("Name"), strName, _("Address"), strAddress);
            if (!dialog.ShowModal())
                return;
            strName = dialog.GetValue1();
            strAddress = dialog.GetValue2();
        }
        while (CheckIfMine(strAddress, _("Add Address")));
    }
    else if (nPage == RECEIVING)
    {
        // Ask name
        CGetTextFromUserDialog dialog(this,
            _("New Receiving Address"),
            _("You should use a new address for each payment you receive.\n\nLabel"),
            "");
        if (!dialog.ShowModal())
            return;
        strName = dialog.GetValue();

        // Generate new key
        strAddress = PubKeyToAddress(GetKeyFromKeyPool());
    }

    // Add to list and select it
    SetAddressBookName(strAddress, strName);
    int nIndex = InsertLine(m_listCtrl, strName, strAddress);
    SetSelection(m_listCtrl, nIndex);
    m_listCtrl->SetFocus();
    if (nPage == SENDING)
        pframeMain->RefreshListCtrl();
}

void CAddressBookDialog::OnButtonOK(wxCommandEvent& event)
{
    // OK
    EndModal(GetSelectedAddress() != "" ? 1 : 0);
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
        string strTooltip = _("Bitcoin");
        if (fGenerateBitcoins)
            strTooltip = _("Bitcoin - Generating");
        if (fGenerateBitcoins && vNodes.empty())
            strTooltip = _("Bitcoin - (not connected)");

        // Optimization, only update when changed, using char array to be reentrant
        if (strncmp(pszPrevTip, strTooltip.c_str(), sizeof(pszPrevTip)-1) != 0)
        {
            strlcpy(pszPrevTip, strTooltip.c_str(), sizeof(pszPrevTip));
#ifdef __WXMSW__
            // somehow it'll choose the wrong size and scale it down if
            // we use the main icon, so we hand it one with only 16x16
            SetIcon(wxICON(favicon), strTooltip);
#else
            SetIcon(bitcoin80_xpm, strTooltip);
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
    wxCommandEvent event2(wxEVT_COMMAND_MENU_SELECTED, wxID_PREFERENCES);
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
    pmenu->Append(ID_TASKBAR_RESTORE, _("&Open Bitcoin"));
    pmenu->Append(ID_TASKBAR_OPTIONS, _("O&ptions..."));
    pmenu->AppendCheckItem(ID_TASKBAR_GENERATE, _("&Generate Coins"))->Check(fGenerateBitcoins);
#ifndef __WXMAC_OSX__ // Mac has built-in quit menu
    pmenu->AppendSeparator();
    pmenu->Append(ID_TASKBAR_EXIT, _("E&xit"));
#endif
    return pmenu;
}






//////////////////////////////////////////////////////////////////////////////
//
// CMyApp
//

void CreateMainWindow()
{
    pframeMain = new CMainFrame(NULL);
    if (GetBoolArg("-min"))
        pframeMain->Iconize(true);
#if defined(__WXGTK__) || defined(__WXMAC_OSX__)
    if (!GetBoolArg("-minimizetotray"))
        fMinimizeToTray = false;
#endif
    pframeMain->Show(true);  // have to show first to get taskbar button to hide
    if (fMinimizeToTray && pframeMain->IsIconized())
        fClosedToTray = true;
    pframeMain->Show(!fClosedToTray);
    ptaskbaricon->Show(fMinimizeToTray || fClosedToTray);
    CreateThread(ThreadDelayedRepaint, NULL);
}


// Define a new application
class CMyApp : public wxApp
{
public:
    CMyApp(){};
    ~CMyApp(){};
    bool OnInit();
    bool OnInit2();
    int OnExit();

    // Hook Initialize so we can start without GUI
    virtual bool Initialize(int& argc, wxChar** argv);

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

bool CMyApp::Initialize(int& argc, wxChar** argv)
{
    for (int i = 1; i < argc; i++)
        if (!IsSwitchChar(argv[i][0]))
            fCommandLine = true;

    if (!fCommandLine)
    {
        // wxApp::Initialize will remove environment-specific parameters,
        // so it's too early to call ParseParameters yet
        for (int i = 1; i < argc; i++)
        {
            wxString str = argv[i];
            #ifdef __WXMSW__
            if (str.size() >= 1 && str[0] == '/')
                str[0] = '-';
            char pszLower[MAX_PATH];
            strlcpy(pszLower, str.c_str(), sizeof(pszLower));
            strlwr(pszLower);
            str = pszLower;
            #endif
            if (str == "-daemon")
                fDaemon = true;
        }
    }

#ifdef __WXGTK__
    if (fDaemon || fCommandLine)
    {
        // Call the original Initialize while suppressing error messages
        // and ignoring failure.  If unable to initialize GTK, it fails
        // near the end so hopefully the last few things don't matter.
        {
            wxLogNull logNo;
            wxApp::Initialize(argc, argv);
        }

        if (fDaemon)
        {
            // Daemonize
            pid_t pid = fork();
            if (pid < 0)
            {
                fprintf(stderr, "Error: fork() returned %d errno %d\n", pid, errno);
                return false;
            }
            if (pid > 0)
                pthread_exit((void*)0);
        }

        return true;
    }
#endif

    return wxApp::Initialize(argc, argv);
}

bool CMyApp::OnInit()
{
#if defined(__WXMSW__) && defined(__WXDEBUG__) && defined(GUI)
    // Disable malfunctioning wxWidgets debug assertion
    extern int g_isPainting;
    g_isPainting = 10000;
#endif
#ifdef GUI
    wxImage::AddHandler(new wxPNGHandler);
#endif
#if defined(__WXMSW__ ) || defined(__WXMAC_OSX__)
    SetAppName("Bitcoin");
#else
    SetAppName("bitcoin");
#endif
#ifdef __WXMSW__
#if wxUSE_UNICODE
    // Hack to set wxConvLibc codepage to UTF-8 on Windows,
    // may break if wxMBConv_win32 implementation in strconv.cpp changes.
    class wxMBConv_win32 : public wxMBConv
    {
    public:
        long m_CodePage;
        size_t m_minMBCharWidth;
    };
    if (((wxMBConv_win32*)&wxConvLibc)->m_CodePage == CP_ACP)
        ((wxMBConv_win32*)&wxConvLibc)->m_CodePage = CP_UTF8;
#endif
#endif

    // Load locale/<lang>/LC_MESSAGES/bitcoin.mo language file
    g_locale.Init(wxLANGUAGE_DEFAULT, 0);
    g_locale.AddCatalogLookupPathPrefix("locale");
#ifndef __WXMSW__
    g_locale.AddCatalogLookupPathPrefix("/usr/share/locale");
    g_locale.AddCatalogLookupPathPrefix("/usr/local/share/locale");
#endif
    g_locale.AddCatalog("wxstd"); // wxWidgets standard translations, if any
    g_locale.AddCatalog("bitcoin");

    return AppInit(argc, argv);
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
    wxMessageBox(_("Program has crashed and will terminate.  "), "Bitcoin", wxOK | wxICON_ERROR);
}
