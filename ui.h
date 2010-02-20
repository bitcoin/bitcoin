// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

DECLARE_EVENT_TYPE(wxEVT_UITHREADCALL, -1)


extern map<string, string> mapArgs;

// Settings
extern int fShowGenerated;
extern int fMinimizeToTray;
extern int fMinimizeOnClose;



void HandleCtrlA(wxKeyEvent& event);
string FormatTxStatus(const CWalletTx& wtx);
void UIThreadCall(boost::function0<void>);
void MainFrameRepaint();
void Shutdown(void* parg);
int ThreadSafeMessageBox(const string& message, const string& caption="Message", int style=wxOK, wxWindow* parent=NULL, int x=-1, int y=-1);
bool ThreadSafeAskFee(int64 nFeeRequired, const string& strCaption, wxWindow* parent);






class CMainFrame : public CMainFrameBase
{
protected:
    // Event handlers
    void OnClose(wxCloseEvent& event);
    void OnIconize(wxIconizeEvent& event);
    void OnMouseEvents(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event) { HandleCtrlA(event); }
    void OnIdle(wxIdleEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnPaintListCtrl(wxPaintEvent& event);
    void OnMenuFileExit(wxCommandEvent& event);
    void OnMenuViewShowGenerated(wxCommandEvent& event);
    void OnUpdateUIViewShowGenerated(wxUpdateUIEvent& event);
    void OnMenuOptionsGenerate(wxCommandEvent& event);
    void OnUpdateUIOptionsGenerate(wxUpdateUIEvent& event);
    void OnMenuOptionsChangeYourAddress(wxCommandEvent& event);
    void OnMenuOptionsOptions(wxCommandEvent& event);
    void OnMenuHelpAbout(wxCommandEvent& event);
    void OnButtonSend(wxCommandEvent& event);
    void OnButtonAddressBook(wxCommandEvent& event);
    void OnSetFocusAddress(wxFocusEvent& event);
    void OnMouseEventsAddress(wxMouseEvent& event);
    void OnButtonNew(wxCommandEvent& event);
    void OnButtonCopy(wxCommandEvent& event);
    void OnListColBeginDrag(wxListEvent& event);
    void OnListItemActivated(wxListEvent& event);
    void OnListItemActivatedProductsSent(wxListEvent& event);
    void OnListItemActivatedOrdersSent(wxListEvent& event);
    void OnListItemActivatedOrdersReceived(wxListEvent& event);
	
public:
    /** Constructor */
    CMainFrame(wxWindow* parent);
    ~CMainFrame();

    // Custom
    bool fRefreshListCtrl;
    bool fRefreshListCtrlRunning;
    bool fOnSetFocusAddress;
    unsigned int nListViewUpdated;
    bool fRefresh;

    void OnUIThreadCall(wxCommandEvent& event);
    int GetSortIndex(const string& strSort);
    void InsertLine(bool fNew, int nIndex, uint256 hashKey, string strSort, const wxString& str1, const wxString& str2, const wxString& str3, const wxString& str4, const wxString& str5);
    bool DeleteLine(uint256 hashKey);
    bool InsertTransaction(const CWalletTx& wtx, bool fNew, int nIndex=-1);
    void RefreshListCtrl();
    void RefreshStatusColumn();
};




class CTxDetailsDialog : public CTxDetailsDialogBase
{
protected:
    // Event handlers
    void OnButtonOK(wxCommandEvent& event);

public:
    /** Constructor */
    CTxDetailsDialog(wxWindow* parent, CWalletTx wtx);

    // State
    CWalletTx wtx;
};



class COptionsDialog : public COptionsDialogBase
{
protected:
    // Event handlers
    void OnListBox(wxCommandEvent& event);
    void OnKillFocusTransactionFee(wxFocusEvent& event);
    void OnCheckBoxLimitProcessors(wxCommandEvent& event);
    void OnCheckBoxUseProxy(wxCommandEvent& event);
    void OnKillFocusProxy(wxFocusEvent& event);

    void OnButtonOK(wxCommandEvent& event);
    void OnButtonCancel(wxCommandEvent& event);
    void OnButtonApply(wxCommandEvent& event);

public:
    /** Constructor */
    COptionsDialog(wxWindow* parent);

    // Custom
    bool fTmpStartOnSystemStartup;
    bool fTmpMinimizeOnClose;
    void SelectPage(int nPage);
    CAddress GetProxyAddr();
};



class CAboutDialog : public CAboutDialogBase
{
protected:
    // Event handlers
    void OnButtonOK(wxCommandEvent& event);

public:
    /** Constructor */
    CAboutDialog(wxWindow* parent);
};



class CSendDialog : public CSendDialogBase
{
protected:
    // Event handlers
    void OnKeyDown(wxKeyEvent& event) { HandleCtrlA(event); }
    void OnTextAddress(wxCommandEvent& event);
    void OnKillFocusAmount(wxFocusEvent& event);
    void OnButtonAddressBook(wxCommandEvent& event);
    void OnButtonPaste(wxCommandEvent& event);
    void OnButtonSend(wxCommandEvent& event);
    void OnButtonCancel(wxCommandEvent& event);
	
public:
    /** Constructor */
    CSendDialog(wxWindow* parent, const wxString& strAddress="");

    // Custom
    bool fEnabledPrev;
    string strFromSave;
    string strMessageSave;
};



class CSendingDialog : public CSendingDialogBase
{
public:
    // Event handlers
    void OnClose(wxCloseEvent& event);
    void OnButtonOK(wxCommandEvent& event);
    void OnButtonCancel(wxCommandEvent& event);
    void OnPaint(wxPaintEvent& event);
	
public:
    /** Constructor */
    CSendingDialog(wxWindow* parent, const CAddress& addrIn, int64 nPriceIn, const CWalletTx& wtxIn);
    ~CSendingDialog();

    // State
    CAddress addr;
    int64 nPrice;
    CWalletTx wtx;
    wxDateTime start;
    char pszStatus[10000];
    bool fCanCancel;
    bool fAbort;
    bool fSuccess;
    bool fUIDone;
    bool fWorkDone;

    void Close();
    void Repaint();
    bool Status();
    bool Status(const string& str);
    bool Error(const string& str);
    void StartTransfer();
    void OnReply2(CDataStream& vRecv);
    void OnReply3(CDataStream& vRecv);
};

void SendingDialogStartTransfer(void* parg);
void SendingDialogOnReply2(void* parg, CDataStream& vRecv);
void SendingDialogOnReply3(void* parg, CDataStream& vRecv);



class CAddressBookDialog : public CAddressBookDialogBase
{
protected:
    // Event handlers
    void OnNotebookPageChanged(wxNotebookEvent& event);
    void OnListEndLabelEdit(wxListEvent& event);
    void OnListItemSelected(wxListEvent& event);
    void OnListItemActivated(wxListEvent& event);
    void OnButtonDelete(wxCommandEvent& event);
    void OnButtonCopy(wxCommandEvent& event);
    void OnButtonEdit(wxCommandEvent& event);
    void OnButtonNew(wxCommandEvent& event);
    void OnButtonOK(wxCommandEvent& event);
    void OnButtonCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

public:
    /** Constructor */
    CAddressBookDialog(wxWindow* parent, const wxString& strInitSelected, int nPageIn, bool fDuringSendIn);

    // Custom
    enum
    {
        SENDING = 0,
        RECEIVING = 1,
    };
    int nPage;
    wxListCtrl* m_listCtrl;
    bool fDuringSend;
    wxString GetAddress();
    wxString GetSelectedAddress();
    wxString GetSelectedSendingAddress();
    wxString GetSelectedReceivingAddress();
    bool CheckIfMine(const string& strAddress, const string& strTitle);
};



class CGetTextFromUserDialog : public CGetTextFromUserDialogBase
{
protected:
    // Event handlers
    void OnButtonOK(wxCommandEvent& event)     { EndModal(true); }
    void OnButtonCancel(wxCommandEvent& event) { EndModal(false); }
    void OnClose(wxCloseEvent& event)          { EndModal(false); }

    void OnKeyDown(wxKeyEvent& event)
    {
        if (event.GetKeyCode() == '\r' || event.GetKeyCode() == WXK_NUMPAD_ENTER)
            EndModal(true);
        else
            HandleCtrlA(event);
    }

public:
    /** Constructor */
    CGetTextFromUserDialog(wxWindow* parent,
                           const string& strCaption,
                           const string& strMessage1,
                           const string& strValue1="",
                           const string& strMessage2="",
                           const string& strValue2="") : CGetTextFromUserDialogBase(parent, wxID_ANY, strCaption)
    {
        int x = GetSize().GetWidth();
        int y = GetSize().GetHeight();
        m_staticTextMessage1->SetLabel(strMessage1);
        m_textCtrl1->SetValue(strValue1);
        y += wxString(strMessage1).Freq('\n') * 14;
        if (!strMessage2.empty())
        {
            m_staticTextMessage2->Show(true);
            m_staticTextMessage2->SetLabel(strMessage2);
            m_textCtrl2->Show(true);
            m_textCtrl2->SetValue(strValue2);
            y += 46 + wxString(strMessage2).Freq('\n') * 14;
        }
        if (!fWindows)
        {
            x *= 1.14;
            y *= 1.14;
        }
        SetSize(x, y);
    }

    // Custom
    string GetValue()  { return (string)m_textCtrl1->GetValue(); }
    string GetValue1() { return (string)m_textCtrl1->GetValue(); }
    string GetValue2() { return (string)m_textCtrl2->GetValue(); }
};



class CMyTaskBarIcon : public wxTaskBarIcon
{
protected:
    // Event handlers
    void OnLeftButtonDClick(wxTaskBarIconEvent& event);
    void OnMenuRestore(wxCommandEvent& event);
    void OnMenuOptions(wxCommandEvent& event);
    void OnUpdateUIGenerate(wxUpdateUIEvent& event);
    void OnMenuGenerate(wxCommandEvent& event);
    void OnMenuExit(wxCommandEvent& event);

public:
    CMyTaskBarIcon() : wxTaskBarIcon()
    {
        Show(true);
    }

    void Show(bool fShow=true);
    void Hide();
    void Restore();
    void UpdateTooltip();
    virtual wxMenu* CreatePopupMenu();

DECLARE_EVENT_TABLE()
};
