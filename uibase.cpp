///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "uibase.h"

#include "xpm/about.xpm"
#include "xpm/addressbook20.xpm"
#include "xpm/check.xpm"
#include "xpm/send20.xpm"

///////////////////////////////////////////////////////////////////////////

CMainFrameBase::CMainFrameBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	m_menubar = new wxMenuBar( 0 );
	m_menuFile = new wxMenu();
	wxMenuItem* m_menuFileExit;
	m_menuFileExit = new wxMenuItem( m_menuFile, wxID_EXIT, wxString( _("E&xit") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuFileExit );
	
	m_menubar->Append( m_menuFile, _("&File") );
	
	m_menuOptions = new wxMenu();
	wxMenuItem* m_menuOptionsGenerateBitcoins;
	m_menuOptionsGenerateBitcoins = new wxMenuItem( m_menuOptions, wxID_OPTIONSGENERATEBITCOINS, wxString( _("&Generate Coins") ) , wxEmptyString, wxITEM_CHECK );
	m_menuOptions->Append( m_menuOptionsGenerateBitcoins );
	
	wxMenuItem* m_menuOptionsChangeYourAddress;
	m_menuOptionsChangeYourAddress = new wxMenuItem( m_menuOptions, wxID_ANY, wxString( _("&Your Receiving Addresses...") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuOptions->Append( m_menuOptionsChangeYourAddress );
	
	wxMenuItem* m_menuOptionsOptions;
	m_menuOptionsOptions = new wxMenuItem( m_menuOptions, wxID_PREFERENCES, wxString( _("&Options...") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuOptions->Append( m_menuOptionsOptions );
	
	m_menubar->Append( m_menuOptions, _("&Settings") );
	
	m_menuHelp = new wxMenu();
	wxMenuItem* m_menuHelpAbout;
	m_menuHelpAbout = new wxMenuItem( m_menuHelp, wxID_ABOUT, wxString( _("&About...") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuHelp->Append( m_menuHelpAbout );
	
	m_menubar->Append( m_menuHelp, _("&Help") );
	
	this->SetMenuBar( m_menubar );
	
	m_toolBar = this->CreateToolBar( wxTB_FLAT|wxTB_HORZ_TEXT, wxID_ANY );
	m_toolBar->SetToolBitmapSize( wxSize( 20,20 ) );
	m_toolBar->SetToolSeparation( 1 );
	m_toolBar->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	m_toolBar->AddTool( wxID_BUTTONSEND, _("Send Coins"), wxBitmap( send20_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_toolBar->AddTool( wxID_BUTTONRECEIVE, _("Address Book"), wxBitmap( addressbook20_xpm ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_toolBar->Realize();
	
	m_statusBar = this->CreateStatusBar( 1, wxST_SIZEGRIP, wxID_ANY );
	m_statusBar->SetBackgroundColour( wxColour( 240, 240, 240 ) );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer2->Add( 0, 2, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer85;
	bSizer85 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText32 = new wxStaticText( this, wxID_ANY, _("Your Bitcoin Address:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	bSizer85->Add( m_staticText32, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_textCtrlAddress = new wxTextCtrl( this, wxID_TEXTCTRLADDRESS, wxEmptyString, wxDefaultPosition, wxSize( 340,-1 ), wxTE_READONLY );
	bSizer85->Add( m_textCtrlAddress, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_buttonNew = new wxButton( this, wxID_BUTTONNEW, _(" &New... "), wxDefaultPosition, wxSize( -1,-1 ), wxBU_EXACTFIT );
	bSizer85->Add( m_buttonNew, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonCopy = new wxButton( this, wxID_BUTTONCOPY, _(" &Copy to Clipboard "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizer85->Add( m_buttonCopy, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	bSizer85->Add( 0, 0, 0, wxEXPAND, 5 );
	
	bSizer2->Add( bSizer85, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer66;
	bSizer66 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText41 = new wxStaticText( this, wxID_ANY, _("Balance:"), wxDefaultPosition, wxSize( -1,15 ), 0 );
	m_staticText41->Wrap( -1 );
	bSizer66->Add( m_staticText41, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_staticTextBalance = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 120,15 ), wxALIGN_RIGHT|wxST_NO_AUTORESIZE );
	m_staticTextBalance->Wrap( -1 );
	m_staticTextBalance->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_staticTextBalance->SetBackgroundColour( wxColour( 255, 255, 255 ) );
	
	bSizer66->Add( m_staticTextBalance, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	bSizer3->Add( bSizer66, 1, wxEXPAND|wxALL, 5 );
	
	
	bSizer3->Add( 0, 0, 0, wxEXPAND, 5 );
	
	wxString m_choiceFilterChoices[] = { _(" All"), _(" Sent"), _(" Received"), _(" In Progress") };
	int m_choiceFilterNChoices = sizeof( m_choiceFilterChoices ) / sizeof( wxString );
	m_choiceFilter = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxSize( 110,-1 ), m_choiceFilterNChoices, m_choiceFilterChoices, 0 );
	m_choiceFilter->SetSelection( 0 );
	m_choiceFilter->Hide();
	
	bSizer3->Add( m_choiceFilter, 0, wxALIGN_BOTTOM|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	bSizer2->Add( bSizer3, 0, wxEXPAND, 5 );
	
	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panel9 = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_listCtrlAll = new wxListCtrl( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_SORT_HEADER|wxLC_REPORT|wxLC_SORT_DESCENDING );
	bSizer11->Add( m_listCtrlAll, 1, wxEXPAND, 5 );
	
	m_panel9->SetSizer( bSizer11 );
	m_panel9->Layout();
	bSizer11->Fit( m_panel9 );
	m_notebook->AddPage( m_panel9, _("All Transactions"), true );
	m_panel91 = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer111;
	bSizer111 = new wxBoxSizer( wxVERTICAL );
	
	m_listCtrlSentReceived = new wxListCtrl( m_panel91, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_SORT_HEADER|wxLC_REPORT|wxLC_SORT_DESCENDING );
	bSizer111->Add( m_listCtrlSentReceived, 1, wxEXPAND, 5 );
	
	m_panel91->SetSizer( bSizer111 );
	m_panel91->Layout();
	bSizer111->Fit( m_panel91 );
	m_notebook->AddPage( m_panel91, _("Sent/Received"), false );
	m_panel92 = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer112;
	bSizer112 = new wxBoxSizer( wxVERTICAL );
	
	m_listCtrlSent = new wxListCtrl( m_panel92, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_SORT_HEADER|wxLC_REPORT|wxLC_SORT_DESCENDING );
	bSizer112->Add( m_listCtrlSent, 1, wxEXPAND, 5 );
	
	m_panel92->SetSizer( bSizer112 );
	m_panel92->Layout();
	bSizer112->Fit( m_panel92 );
	m_notebook->AddPage( m_panel92, _("Sent"), false );
	m_panel93 = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer113;
	bSizer113 = new wxBoxSizer( wxVERTICAL );
	
	m_listCtrlReceived = new wxListCtrl( m_panel93, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_SORT_HEADER|wxLC_REPORT|wxLC_SORT_DESCENDING );
	bSizer113->Add( m_listCtrlReceived, 1, wxEXPAND, 5 );
	
	m_panel93->SetSizer( bSizer113 );
	m_panel93->Layout();
	bSizer113->Fit( m_panel93 );
	m_notebook->AddPage( m_panel93, _("Received"), false );
	
	bSizer2->Add( m_notebook, 1, wxEXPAND, 5 );
	
	this->SetSizer( bSizer2 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CMainFrameBase::OnClose ) );
	this->Connect( wxEVT_ICONIZE, wxIconizeEventHandler( CMainFrameBase::OnIconize ) );
	this->Connect( wxEVT_IDLE, wxIdleEventHandler( CMainFrameBase::OnIdle ) );
	this->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_MOTION, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Connect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaint ) );
	this->Connect( m_menuFileExit->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuFileExit ) );
	this->Connect( m_menuOptionsGenerateBitcoins->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuOptionsGenerate ) );
	this->Connect( m_menuOptionsGenerateBitcoins->GetId(), wxEVT_UPDATE_UI, wxUpdateUIEventHandler( CMainFrameBase::OnUpdateUIOptionsGenerate ) );
	this->Connect( m_menuOptionsChangeYourAddress->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuOptionsChangeYourAddress ) );
	this->Connect( m_menuOptionsOptions->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuOptionsOptions ) );
	this->Connect( m_menuHelpAbout->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuHelpAbout ) );
	this->Connect( wxID_BUTTONSEND, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CMainFrameBase::OnButtonSend ) );
	this->Connect( wxID_BUTTONRECEIVE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CMainFrameBase::OnButtonAddressBook ) );
	m_textCtrlAddress->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CMainFrameBase::OnKeyDown ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_MOTION, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( CMainFrameBase::OnSetFocusAddress ), NULL, this );
	m_buttonNew->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMainFrameBase::OnButtonNew ), NULL, this );
	m_buttonCopy->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMainFrameBase::OnButtonCopy ), NULL, this );
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( CMainFrameBase::OnNotebookPageChanged ), NULL, this );
	m_listCtrlAll->Connect( wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, wxListEventHandler( CMainFrameBase::OnListColBeginDrag ), NULL, this );
	m_listCtrlAll->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CMainFrameBase::OnListItemActivated ), NULL, this );
	m_listCtrlAll->Connect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaintListCtrl ), NULL, this );
	m_listCtrlSentReceived->Connect( wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, wxListEventHandler( CMainFrameBase::OnListColBeginDrag ), NULL, this );
	m_listCtrlSentReceived->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CMainFrameBase::OnListItemActivated ), NULL, this );
	m_listCtrlSentReceived->Connect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaintListCtrl ), NULL, this );
	m_listCtrlSent->Connect( wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, wxListEventHandler( CMainFrameBase::OnListColBeginDrag ), NULL, this );
	m_listCtrlSent->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CMainFrameBase::OnListItemActivated ), NULL, this );
	m_listCtrlSent->Connect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaintListCtrl ), NULL, this );
	m_listCtrlReceived->Connect( wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, wxListEventHandler( CMainFrameBase::OnListColBeginDrag ), NULL, this );
	m_listCtrlReceived->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CMainFrameBase::OnListItemActivated ), NULL, this );
	m_listCtrlReceived->Connect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaintListCtrl ), NULL, this );
}

CMainFrameBase::~CMainFrameBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CMainFrameBase::OnClose ) );
	this->Disconnect( wxEVT_ICONIZE, wxIconizeEventHandler( CMainFrameBase::OnIconize ) );
	this->Disconnect( wxEVT_IDLE, wxIdleEventHandler( CMainFrameBase::OnIdle ) );
	this->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_MIDDLE_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_MOTION, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( CMainFrameBase::OnMouseEvents ) );
	this->Disconnect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaint ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuFileExit ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuOptionsGenerate ) );
	this->Disconnect( wxID_ANY, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( CMainFrameBase::OnUpdateUIOptionsGenerate ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuOptionsChangeYourAddress ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuOptionsOptions ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMainFrameBase::OnMenuHelpAbout ) );
	this->Disconnect( wxID_BUTTONSEND, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CMainFrameBase::OnButtonSend ) );
	this->Disconnect( wxID_BUTTONRECEIVE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CMainFrameBase::OnButtonAddressBook ) );
	m_textCtrlAddress->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( CMainFrameBase::OnKeyDown ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_MIDDLE_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_MOTION, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( CMainFrameBase::OnMouseEventsAddress ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( CMainFrameBase::OnSetFocusAddress ), NULL, this );
	m_buttonNew->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMainFrameBase::OnButtonNew ), NULL, this );
	m_buttonCopy->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMainFrameBase::OnButtonCopy ), NULL, this );
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( CMainFrameBase::OnNotebookPageChanged ), NULL, this );
	m_listCtrlAll->Disconnect( wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, wxListEventHandler( CMainFrameBase::OnListColBeginDrag ), NULL, this );
	m_listCtrlAll->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CMainFrameBase::OnListItemActivated ), NULL, this );
	m_listCtrlAll->Disconnect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaintListCtrl ), NULL, this );
	m_listCtrlSentReceived->Disconnect( wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, wxListEventHandler( CMainFrameBase::OnListColBeginDrag ), NULL, this );
	m_listCtrlSentReceived->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CMainFrameBase::OnListItemActivated ), NULL, this );
	m_listCtrlSentReceived->Disconnect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaintListCtrl ), NULL, this );
	m_listCtrlSent->Disconnect( wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, wxListEventHandler( CMainFrameBase::OnListColBeginDrag ), NULL, this );
	m_listCtrlSent->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CMainFrameBase::OnListItemActivated ), NULL, this );
	m_listCtrlSent->Disconnect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaintListCtrl ), NULL, this );
	m_listCtrlReceived->Disconnect( wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, wxListEventHandler( CMainFrameBase::OnListColBeginDrag ), NULL, this );
	m_listCtrlReceived->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CMainFrameBase::OnListItemActivated ), NULL, this );
	m_listCtrlReceived->Disconnect( wxEVT_PAINT, wxPaintEventHandler( CMainFrameBase::OnPaintListCtrl ), NULL, this );
}

CTxDetailsDialogBase::CTxDetailsDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer64;
	bSizer64 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer66;
	bSizer66 = new wxBoxSizer( wxVERTICAL );
	
	m_htmlWin = new wxHtmlWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	bSizer66->Add( m_htmlWin, 1, wxALL|wxEXPAND, 5 );
	
	bSizer64->Add( bSizer66, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer65;
	bSizer65 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer65->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bSizer64->Add( bSizer65, 0, wxALIGN_RIGHT, 5 );
	
	this->SetSizer( bSizer64 );
	this->Layout();
	
	// Connect Events
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CTxDetailsDialogBase::OnButtonOK ), NULL, this );
}

CTxDetailsDialogBase::~CTxDetailsDialogBase()
{
	// Disconnect Events
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CTxDetailsDialogBase::OnButtonOK ), NULL, this );
}

COptionsDialogBase::COptionsDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer55;
	bSizer55 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer66;
	bSizer66 = new wxBoxSizer( wxHORIZONTAL );
	
	m_listBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxSize( 110,-1 ), 0, NULL, wxLB_NEEDED_SB|wxLB_SINGLE ); 
	bSizer66->Add( m_listBox, 0, wxEXPAND|wxRIGHT, 5 );
	
	m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_scrolledWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer63;
	bSizer63 = new wxBoxSizer( wxVERTICAL );
	
	m_panelMain = new wxPanel( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer69;
	bSizer69 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer69->Add( 0, 16, 0, wxEXPAND, 5 );
	
	m_staticText32 = new wxStaticText( m_panelMain, wxID_ANY, _("Optional transaction fee you give to the nodes that process your transactions."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	m_staticText32->Hide();
	
	bSizer69->Add( m_staticText32, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxBoxSizer* bSizer56;
	bSizer56 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText31 = new wxStaticText( m_panelMain, wxID_ANY, _("Transaction fee:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	m_staticText31->Hide();
	
	bSizer56->Add( m_staticText31, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrlTransactionFee = new wxTextCtrl( m_panelMain, wxID_TRANSACTIONFEE, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	m_textCtrlTransactionFee->Hide();
	
	bSizer56->Add( m_textCtrlTransactionFee, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer69->Add( bSizer56, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxHORIZONTAL );
	
	m_checkBoxLimitProcessors = new wxCheckBox( m_panelMain, wxID_ANY, _("&Limit coin generation to"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer71->Add( m_checkBoxLimitProcessors, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_spinCtrlLimitProcessors = new wxSpinCtrl( m_panelMain, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), wxSP_ARROW_KEYS, 1, 999, 1 );
	bSizer71->Add( m_spinCtrlLimitProcessors, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText35 = new wxStaticText( m_panelMain, wxID_ANY, _("processors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText35->Wrap( -1 );
	bSizer71->Add( m_staticText35, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	bSizer69->Add( bSizer71, 0, 0, 5 );
	
	m_checkBoxStartOnSystemStartup = new wxCheckBox( m_panelMain, wxID_ANY, _("&Start Bitcoin on system startup"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer69->Add( m_checkBoxStartOnSystemStartup, 0, wxALL, 5 );
	
	m_checkBoxMinimizeToTray = new wxCheckBox( m_panelMain, wxID_ANY, _("&Minimize to the tray instead of the taskbar"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer69->Add( m_checkBoxMinimizeToTray, 0, wxALL, 5 );
	
	m_checkBoxMinimizeOnClose = new wxCheckBox( m_panelMain, wxID_ANY, _("M&inimize to the tray on close"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer69->Add( m_checkBoxMinimizeOnClose, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer102;
	bSizer102 = new wxBoxSizer( wxHORIZONTAL );
	
	m_checkBoxUseProxy = new wxCheckBox( m_panelMain, wxID_ANY, _("&Connect through socks4 proxy: "), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer102->Add( m_checkBoxUseProxy, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer69->Add( bSizer102, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer103;
	bSizer103 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer103->Add( 18, 0, 0, 0, 5 );
	
	m_staticTextProxyIP = new wxStaticText( m_panelMain, wxID_ANY, _("Proxy &IP:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextProxyIP->Wrap( -1 );
	bSizer103->Add( m_staticTextProxyIP, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlProxyIP = new wxTextCtrl( m_panelMain, wxID_PROXYIP, wxEmptyString, wxDefaultPosition, wxSize( 140,-1 ), 0 );
	m_textCtrlProxyIP->SetMaxLength( 15 ); 
	bSizer103->Add( m_textCtrlProxyIP, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextProxyPort = new wxStaticText( m_panelMain, wxID_ANY, _(" &Port:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextProxyPort->Wrap( -1 );
	bSizer103->Add( m_staticTextProxyPort, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlProxyPort = new wxTextCtrl( m_panelMain, wxID_PROXYPORT, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), 0 );
	m_textCtrlProxyPort->SetMaxLength( 5 ); 
	bSizer103->Add( m_textCtrlProxyPort, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer69->Add( bSizer103, 1, wxEXPAND, 5 );
	
	m_panelMain->SetSizer( bSizer69 );
	m_panelMain->Layout();
	bSizer69->Fit( m_panelMain );
	bSizer63->Add( m_panelMain, 0, wxEXPAND, 5 );
	
	m_panelTest2 = new wxPanel( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer64;
	bSizer64 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer64->Add( 0, 16, 0, wxEXPAND, 5 );
	
	m_staticText321 = new wxStaticText( m_panelTest2, wxID_ANY, _("// [don't translate] Test panel 2 for future expansion"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText321->Wrap( -1 );
	bSizer64->Add( m_staticText321, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticText69 = new wxStaticText( m_panelTest2, wxID_ANY, _("// [don't translate] Let's not start multiple pages until the first page is filled up"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText69->Wrap( -1 );
	bSizer64->Add( m_staticText69, 0, wxALL, 5 );
	
	m_panelTest2->SetSizer( bSizer64 );
	m_panelTest2->Layout();
	bSizer64->Fit( m_panelTest2 );
	bSizer63->Add( m_panelTest2, 0, wxEXPAND, 5 );
	
	m_scrolledWindow->SetSizer( bSizer63 );
	m_scrolledWindow->Layout();
	bSizer63->Fit( m_scrolledWindow );
	bSizer66->Add( m_scrolledWindow, 1, wxEXPAND|wxLEFT, 5 );
	
	bSizer55->Add( bSizer66, 1, wxEXPAND|wxALL, 9 );
	
	wxBoxSizer* bSizer58;
	bSizer58 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer58->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer58->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonApply = new wxButton( this, wxID_APPLY, _("&Apply"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer58->Add( m_buttonApply, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bSizer55->Add( bSizer58, 0, wxALIGN_RIGHT, 5 );
	
	this->SetSizer( bSizer55 );
	this->Layout();
	
	// Connect Events
	m_listBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( COptionsDialogBase::OnListBox ), NULL, this );
	m_textCtrlTransactionFee->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( COptionsDialogBase::OnKillFocusTransactionFee ), NULL, this );
	m_checkBoxLimitProcessors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnCheckBoxLimitProcessors ), NULL, this );
	m_checkBoxMinimizeToTray->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnCheckBoxMinimizeToTray ), NULL, this );
	m_checkBoxUseProxy->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnCheckBoxUseProxy ), NULL, this );
	m_textCtrlProxyIP->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( COptionsDialogBase::OnKillFocusProxy ), NULL, this );
	m_textCtrlProxyPort->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( COptionsDialogBase::OnKillFocusProxy ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnButtonCancel ), NULL, this );
	m_buttonApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnButtonApply ), NULL, this );
}

COptionsDialogBase::~COptionsDialogBase()
{
	// Disconnect Events
	m_listBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( COptionsDialogBase::OnListBox ), NULL, this );
	m_textCtrlTransactionFee->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( COptionsDialogBase::OnKillFocusTransactionFee ), NULL, this );
	m_checkBoxLimitProcessors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnCheckBoxLimitProcessors ), NULL, this );
	m_checkBoxMinimizeToTray->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnCheckBoxMinimizeToTray ), NULL, this );
	m_checkBoxUseProxy->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnCheckBoxUseProxy ), NULL, this );
	m_textCtrlProxyIP->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( COptionsDialogBase::OnKillFocusProxy ), NULL, this );
	m_textCtrlProxyPort->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( COptionsDialogBase::OnKillFocusProxy ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnButtonCancel ), NULL, this );
	m_buttonApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( COptionsDialogBase::OnButtonApply ), NULL, this );
}

CAboutDialogBase::CAboutDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer63;
	bSizer63 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap = new wxStaticBitmap( this, wxID_ANY, wxBitmap( about_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer63->Add( m_bitmap, 0, 0, 5 );
	
	wxBoxSizer* bSizer60;
	bSizer60 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer62;
	bSizer62 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer631;
	bSizer631 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer631->Add( 0, 65, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer64;
	bSizer64 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText40 = new wxStaticText( this, wxID_ANY, _("Bitcoin "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText40->Wrap( -1 );
	m_staticText40->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer64->Add( m_staticText40, 0, wxALIGN_BOTTOM|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_staticTextVersion = new wxStaticText( this, wxID_ANY, _("version"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVersion->Wrap( -1 );
	m_staticTextVersion->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer64->Add( m_staticTextVersion, 0, wxALIGN_BOTTOM|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	bSizer631->Add( bSizer64, 0, wxEXPAND, 5 );
	
	
	bSizer631->Add( 0, 4, 0, wxEXPAND, 5 );
	
	m_staticTextMain = new wxStaticText( this, wxID_ANY, _("Copyright (c) 2009-2010 Satoshi Nakamoto.\n\nDistributed under the MIT/X11 software license, see the accompanying file \nlicense.txt or http://www.opensource.org/licenses/mit-license.php.\n\nThis product includes software developed by the OpenSSL Project for use in the \nOpenSSL Toolkit (http://www.openssl.org/) and cryptographic software written by \nEric Young (eay@cryptsoft.com)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMain->Wrap( -1 );
	bSizer631->Add( m_staticTextMain, 0, wxALL, 5 );
	
	
	bSizer631->Add( 0, 0, 0, wxEXPAND, 5 );
	
	bSizer62->Add( bSizer631, 1, wxEXPAND, 5 );
	
	bSizer60->Add( bSizer62, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer61->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer61->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 6 );
	
	bSizer60->Add( bSizer61, 0, wxALIGN_RIGHT|wxEXPAND|wxRIGHT, 2 );
	
	bSizer63->Add( bSizer60, 1, wxEXPAND|wxLEFT, 5 );
	
	this->SetSizer( bSizer63 );
	this->Layout();
	
	// Connect Events
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAboutDialogBase::OnButtonOK ), NULL, this );
}

CAboutDialogBase::~CAboutDialogBase()
{
	// Disconnect Events
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAboutDialogBase::OnButtonOK ), NULL, this );
}

CSendDialogBase::CSendDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer21->Add( 0, 5, 0, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer1->Add( 0, 0, 0, wxEXPAND, 5 );
	
	m_staticTextInstructions = new wxStaticText( this, wxID_ANY, _("Enter a Bitcoin address (e.g. 1NS17iag9jJgTHD1VXjvLCEnZuQ3rJED9L) or IP address (e.g. 123.45.6.7)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInstructions->Wrap( -1 );
	fgSizer1->Add( m_staticTextInstructions, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer47;
	bSizer47 = new wxBoxSizer( wxHORIZONTAL );
	
	bSizer47->SetMinSize( wxSize( 70,-1 ) ); 
	
	bSizer47->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bitmapCheckMark = new wxStaticBitmap( this, wxID_ANY, wxBitmap( check_xpm ), wxDefaultPosition, wxSize( 16,16 ), 0 );
	bSizer47->Add( m_bitmapCheckMark, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText36 = new wxStaticText( this, wxID_ANY, _("Pay &To:"), wxDefaultPosition, wxSize( -1,-1 ), wxALIGN_RIGHT );
	m_staticText36->Wrap( -1 );
	bSizer47->Add( m_staticText36, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	fgSizer1->Add( bSizer47, 1, wxEXPAND|wxLEFT, 5 );
	
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxHORIZONTAL );
	
	m_textCtrlAddress = new wxTextCtrl( this, wxID_TEXTCTRLPAYTO, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer19->Add( m_textCtrlAddress, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxBoxSizer* bSizer66;
	bSizer66 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonPaste = new wxButton( this, wxID_BUTTONPASTE, _("&Paste"), wxDefaultPosition, wxSize( -1,-1 ), wxBU_EXACTFIT );
	bSizer66->Add( m_buttonPaste, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxEXPAND, 5 );
	
	m_buttonAddress = new wxButton( this, wxID_BUTTONADDRESSBOOK, _(" Address &Book..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer66->Add( m_buttonAddress, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxEXPAND, 5 );
	
	bSizer19->Add( bSizer66, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	fgSizer1->Add( bSizer19, 1, wxEXPAND|wxRIGHT, 5 );
	
	m_staticText19 = new wxStaticText( this, wxID_ANY, _("&Amount:"), wxDefaultPosition, wxSize( -1,-1 ), wxALIGN_RIGHT );
	m_staticText19->Wrap( -1 );
	fgSizer1->Add( m_staticText19, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_RIGHT, 5 );
	
	m_textCtrlAmount = new wxTextCtrl( this, wxID_TEXTCTRLAMOUNT, wxEmptyString, wxDefaultPosition, wxSize( 145,-1 ), 0 );
	m_textCtrlAmount->SetMaxLength( 20 ); 
	m_textCtrlAmount->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	fgSizer1->Add( m_textCtrlAmount, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticText20 = new wxStaticText( this, wxID_ANY, _("T&ransfer:"), wxDefaultPosition, wxSize( -1,-1 ), wxALIGN_RIGHT );
	m_staticText20->Wrap( -1 );
	m_staticText20->Hide();
	
	fgSizer1->Add( m_staticText20, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxString m_choiceTransferTypeChoices[] = { _(" Standard") };
	int m_choiceTransferTypeNChoices = sizeof( m_choiceTransferTypeChoices ) / sizeof( wxString );
	m_choiceTransferType = new wxChoice( this, wxID_CHOICETRANSFERTYPE, wxDefaultPosition, wxDefaultSize, m_choiceTransferTypeNChoices, m_choiceTransferTypeChoices, 0 );
	m_choiceTransferType->SetSelection( 0 );
	m_choiceTransferType->Hide();
	
	fgSizer1->Add( m_choiceTransferType, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	fgSizer1->Add( 0, 3, 0, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 0, wxEXPAND, 5 );
	
	bSizer21->Add( fgSizer1, 0, wxEXPAND|wxLEFT, 5 );
	
	wxBoxSizer* bSizer672;
	bSizer672 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer681;
	bSizer681 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextFrom = new wxStaticText( this, wxID_ANY, _("&From:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFrom->Wrap( -1 );
	bSizer681->Add( m_staticTextFrom, 0, wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrlFrom = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer681->Add( m_textCtrlFrom, 0, wxLEFT|wxEXPAND, 5 );
	
	bSizer672->Add( bSizer681, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer21->Add( bSizer672, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer67;
	bSizer67 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer68;
	bSizer68 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextMessage = new wxStaticText( this, wxID_ANY, _("&Message:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMessage->Wrap( -1 );
	bSizer68->Add( m_staticTextMessage, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrlMessage = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	bSizer68->Add( m_textCtrlMessage, 1, wxEXPAND|wxLEFT, 5 );
	
	bSizer67->Add( bSizer68, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer21->Add( bSizer67, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer23->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonSend = new wxButton( this, wxID_BUTTONSEND, _("&Send"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_buttonSend->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer23->Add( m_buttonSend, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer23->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bSizer21->Add( bSizer23, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer21 );
	this->Layout();
	
	// Connect Events
	m_textCtrlAddress->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CSendDialogBase::OnKeyDown ), NULL, this );
	m_textCtrlAddress->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CSendDialogBase::OnTextAddress ), NULL, this );
	m_buttonPaste->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendDialogBase::OnButtonPaste ), NULL, this );
	m_buttonAddress->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendDialogBase::OnButtonAddressBook ), NULL, this );
	m_textCtrlAmount->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CSendDialogBase::OnKeyDown ), NULL, this );
	m_textCtrlAmount->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CSendDialogBase::OnKillFocusAmount ), NULL, this );
	m_textCtrlFrom->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CSendDialogBase::OnKeyDown ), NULL, this );
	m_textCtrlMessage->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CSendDialogBase::OnKeyDown ), NULL, this );
	m_buttonSend->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendDialogBase::OnButtonSend ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendDialogBase::OnButtonCancel ), NULL, this );
}

CSendDialogBase::~CSendDialogBase()
{
	// Disconnect Events
	m_textCtrlAddress->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( CSendDialogBase::OnKeyDown ), NULL, this );
	m_textCtrlAddress->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CSendDialogBase::OnTextAddress ), NULL, this );
	m_buttonPaste->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendDialogBase::OnButtonPaste ), NULL, this );
	m_buttonAddress->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendDialogBase::OnButtonAddressBook ), NULL, this );
	m_textCtrlAmount->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( CSendDialogBase::OnKeyDown ), NULL, this );
	m_textCtrlAmount->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CSendDialogBase::OnKillFocusAmount ), NULL, this );
	m_textCtrlFrom->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( CSendDialogBase::OnKeyDown ), NULL, this );
	m_textCtrlMessage->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( CSendDialogBase::OnKeyDown ), NULL, this );
	m_buttonSend->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendDialogBase::OnButtonSend ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendDialogBase::OnButtonCancel ), NULL, this );
}

CSendingDialogBase::CSendingDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer68;
	bSizer68 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextSending = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,14 ), 0 );
	m_staticTextSending->Wrap( -1 );
	bSizer68->Add( m_staticTextSending, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 8 );
	
	m_textCtrlStatus = new wxTextCtrl( this, wxID_ANY, _("\n\nConnecting..."), wxDefaultPosition, wxDefaultSize, wxTE_CENTRE|wxTE_MULTILINE|wxTE_NO_VSCROLL|wxTE_READONLY|wxNO_BORDER );
	m_textCtrlStatus->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	bSizer68->Add( m_textCtrlStatus, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	wxBoxSizer* bSizer69;
	bSizer69 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer69->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonOK = new wxButton( this, wxID_ANY, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->Enable( false );
	
	bSizer69->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer69->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bSizer68->Add( bSizer69, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer68 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CSendingDialogBase::OnClose ) );
	this->Connect( wxEVT_PAINT, wxPaintEventHandler( CSendingDialogBase::OnPaint ) );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendingDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendingDialogBase::OnButtonCancel ), NULL, this );
}

CSendingDialogBase::~CSendingDialogBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CSendingDialogBase::OnClose ) );
	this->Disconnect( wxEVT_PAINT, wxPaintEventHandler( CSendingDialogBase::OnPaint ) );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendingDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CSendingDialogBase::OnButtonCancel ), NULL, this );
}

CYourAddressDialogBase::CYourAddressDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer68;
	bSizer68 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer68->Add( 0, 5, 0, wxEXPAND, 5 );
	
	m_staticText45 = new wxStaticText( this, wxID_ANY, _("These are your Bitcoin addresses for receiving payments.  You may want to give a different one to each sender so you can keep track of who is paying you.  The highlighted address is displayed in the main window."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText45->Wrap( 590 );
	bSizer68->Add( m_staticText45, 0, wxALL, 5 );
	
	m_listCtrl = new wxListCtrl( this, wxID_LISTCTRL, wxDefaultPosition, wxDefaultSize, wxLC_NO_SORT_HEADER|wxLC_REPORT|wxLC_SORT_ASCENDING );
	bSizer68->Add( m_listCtrl, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer69;
	bSizer69 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer69->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonRename = new wxButton( this, wxID_BUTTONRENAME, _("&Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer69->Add( m_buttonRename, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonNew = new wxButton( this, wxID_BUTTONNEW, _(" &New Address... "), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer69->Add( m_buttonNew, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonCopy = new wxButton( this, wxID_BUTTONCOPY, _(" &Copy to Clipboard "), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer69->Add( m_buttonCopy, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer69->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_buttonCancel->Hide();
	
	bSizer69->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bSizer68->Add( bSizer69, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer68 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CYourAddressDialogBase::OnClose ) );
	m_listCtrl->Connect( wxEVT_COMMAND_LIST_END_LABEL_EDIT, wxListEventHandler( CYourAddressDialogBase::OnListEndLabelEdit ), NULL, this );
	m_listCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CYourAddressDialogBase::OnListItemActivated ), NULL, this );
	m_listCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( CYourAddressDialogBase::OnListItemSelected ), NULL, this );
	m_buttonRename->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonRename ), NULL, this );
	m_buttonNew->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonNew ), NULL, this );
	m_buttonCopy->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonCopy ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonCancel ), NULL, this );
}

CYourAddressDialogBase::~CYourAddressDialogBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CYourAddressDialogBase::OnClose ) );
	m_listCtrl->Disconnect( wxEVT_COMMAND_LIST_END_LABEL_EDIT, wxListEventHandler( CYourAddressDialogBase::OnListEndLabelEdit ), NULL, this );
	m_listCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CYourAddressDialogBase::OnListItemActivated ), NULL, this );
	m_listCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( CYourAddressDialogBase::OnListItemSelected ), NULL, this );
	m_buttonRename->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonRename ), NULL, this );
	m_buttonNew->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonNew ), NULL, this );
	m_buttonCopy->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonCopy ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CYourAddressDialogBase::OnButtonCancel ), NULL, this );
}

CAddressBookDialogBase::CAddressBookDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer58;
	bSizer58 = new wxBoxSizer( wxVERTICAL );
	
	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelSending = new wxPanel( m_notebook, wxID_PANELSENDING, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer68;
	bSizer68 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer68->Add( 0, 0, 0, wxEXPAND, 5 );
	
	m_staticText55 = new wxStaticText( m_panelSending, wxID_ANY, _("Bitcoin Address"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText55->Wrap( -1 );
	m_staticText55->Hide();
	
	bSizer68->Add( m_staticText55, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_listCtrlSending = new wxListCtrl( m_panelSending, wxID_LISTCTRLSENDING, wxDefaultPosition, wxDefaultSize, wxLC_NO_SORT_HEADER|wxLC_REPORT|wxLC_SORT_ASCENDING );
	bSizer68->Add( m_listCtrlSending, 1, wxALL|wxEXPAND, 5 );
	
	m_panelSending->SetSizer( bSizer68 );
	m_panelSending->Layout();
	bSizer68->Fit( m_panelSending );
	m_notebook->AddPage( m_panelSending, _("Sending"), false );
	m_panelReceiving = new wxPanel( m_notebook, wxID_PANELRECEIVING, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer681;
	bSizer681 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer681->Add( 0, 0, 0, wxEXPAND, 5 );
	
	m_staticText45 = new wxStaticText( m_panelReceiving, wxID_ANY, _("These are your Bitcoin addresses for receiving payments.  You can give a different one to each sender to keep track of who is paying you.  The highlighted address will be displayed in the main window."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText45->Wrap( 570 );
	bSizer681->Add( m_staticText45, 0, wxTOP|wxRIGHT|wxLEFT, 6 );
	
	
	bSizer681->Add( 0, 2, 0, wxEXPAND, 5 );
	
	m_listCtrlReceiving = new wxListCtrl( m_panelReceiving, wxID_LISTCTRLRECEIVING, wxDefaultPosition, wxDefaultSize, wxLC_NO_SORT_HEADER|wxLC_REPORT|wxLC_SORT_ASCENDING );
	bSizer681->Add( m_listCtrlReceiving, 1, wxALL|wxEXPAND, 5 );
	
	m_panelReceiving->SetSizer( bSizer681 );
	m_panelReceiving->Layout();
	bSizer681->Fit( m_panelReceiving );
	m_notebook->AddPage( m_panelReceiving, _("Receiving"), true );
	
	bSizer58->Add( m_notebook, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer69;
	bSizer69 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer69->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonDelete = new wxButton( this, wxID_BUTTONDELETE, _("&Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer69->Add( m_buttonDelete, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonCopy = new wxButton( this, wxID_BUTTONCOPY, _(" &Copy to Clipboard "), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer69->Add( m_buttonCopy, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonEdit = new wxButton( this, wxID_BUTTONEDIT, _("&Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer69->Add( m_buttonEdit, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonNew = new wxButton( this, wxID_BUTTONNEW, _(" &New Address... "), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer69->Add( m_buttonNew, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer69->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer69->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bSizer58->Add( bSizer69, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer58 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CAddressBookDialogBase::OnClose ) );
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( CAddressBookDialogBase::OnNotebookPageChanged ), NULL, this );
	m_listCtrlSending->Connect( wxEVT_COMMAND_LIST_END_LABEL_EDIT, wxListEventHandler( CAddressBookDialogBase::OnListEndLabelEdit ), NULL, this );
	m_listCtrlSending->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CAddressBookDialogBase::OnListItemActivated ), NULL, this );
	m_listCtrlSending->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( CAddressBookDialogBase::OnListItemSelected ), NULL, this );
	m_listCtrlReceiving->Connect( wxEVT_COMMAND_LIST_END_LABEL_EDIT, wxListEventHandler( CAddressBookDialogBase::OnListEndLabelEdit ), NULL, this );
	m_listCtrlReceiving->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CAddressBookDialogBase::OnListItemActivated ), NULL, this );
	m_listCtrlReceiving->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( CAddressBookDialogBase::OnListItemSelected ), NULL, this );
	m_buttonDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonDelete ), NULL, this );
	m_buttonCopy->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonCopy ), NULL, this );
	m_buttonEdit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonEdit ), NULL, this );
	m_buttonNew->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonNew ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonCancel ), NULL, this );
}

CAddressBookDialogBase::~CAddressBookDialogBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CAddressBookDialogBase::OnClose ) );
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( CAddressBookDialogBase::OnNotebookPageChanged ), NULL, this );
	m_listCtrlSending->Disconnect( wxEVT_COMMAND_LIST_END_LABEL_EDIT, wxListEventHandler( CAddressBookDialogBase::OnListEndLabelEdit ), NULL, this );
	m_listCtrlSending->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CAddressBookDialogBase::OnListItemActivated ), NULL, this );
	m_listCtrlSending->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( CAddressBookDialogBase::OnListItemSelected ), NULL, this );
	m_listCtrlReceiving->Disconnect( wxEVT_COMMAND_LIST_END_LABEL_EDIT, wxListEventHandler( CAddressBookDialogBase::OnListEndLabelEdit ), NULL, this );
	m_listCtrlReceiving->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( CAddressBookDialogBase::OnListItemActivated ), NULL, this );
	m_listCtrlReceiving->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( CAddressBookDialogBase::OnListItemSelected ), NULL, this );
	m_buttonDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonDelete ), NULL, this );
	m_buttonCopy->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonCopy ), NULL, this );
	m_buttonEdit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonEdit ), NULL, this );
	m_buttonNew->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonNew ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CAddressBookDialogBase::OnButtonCancel ), NULL, this );
}

CGetTextFromUserDialogBase::CGetTextFromUserDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer79;
	bSizer79 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer81;
	bSizer81 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer81->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextMessage1 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMessage1->Wrap( -1 );
	bSizer81->Add( m_staticTextMessage1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrl1 = new wxTextCtrl( this, wxID_TEXTCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bSizer81->Add( m_textCtrl1, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticTextMessage2 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMessage2->Wrap( -1 );
	m_staticTextMessage2->Hide();
	
	bSizer81->Add( m_staticTextMessage2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrl2 = new wxTextCtrl( this, wxID_TEXTCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_textCtrl2->Hide();
	
	bSizer81->Add( m_textCtrl2, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer81->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer79->Add( bSizer81, 1, wxEXPAND|wxALL, 10 );
	
	wxBoxSizer* bSizer80;
	bSizer80 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer80->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer80->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer80->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bSizer79->Add( bSizer80, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer79 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CGetTextFromUserDialogBase::OnClose ) );
	m_textCtrl1->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CGetTextFromUserDialogBase::OnKeyDown ), NULL, this );
	m_textCtrl2->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CGetTextFromUserDialogBase::OnKeyDown ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CGetTextFromUserDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CGetTextFromUserDialogBase::OnButtonCancel ), NULL, this );
}

CGetTextFromUserDialogBase::~CGetTextFromUserDialogBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CGetTextFromUserDialogBase::OnClose ) );
	m_textCtrl1->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( CGetTextFromUserDialogBase::OnKeyDown ), NULL, this );
	m_textCtrl2->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( CGetTextFromUserDialogBase::OnKeyDown ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CGetTextFromUserDialogBase::OnButtonOK ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CGetTextFromUserDialogBase::OnButtonCancel ), NULL, this );
}
