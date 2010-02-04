#ifndef _IPC_H
#define	_IPC_H

class CServer : public wxServer {
public:
    wxConnectionBase * OnAcceptConnection (const wxString &topic);
};

class CClient : public wxClient {
public:
    wxConnectionBase * OnMakeConnection ();
};

class CServerConnection : public wxConnection {
public:
    const void * OnRequest (const wxString &topic, const wxString &item, size_t *size, wxIPCFormat format);
};

class CClientConnection : public wxConnection {
public:
    CClientConnection() : wxConnection() {}
    ~CClientConnection() {}
    
    bool OnAdvise (const wxString &topic, const wxString &item, const void *data, size_t size, wxIPCFormat format);
};

#endif	/* _IPC_H */

