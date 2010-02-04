/*
 * Inter-process calling functionality
 */

#include "headers.h"

wxConnectionBase * CServer::OnAcceptConnection (const wxString &topic) {
    return new CServerConnection;
}

wxConnectionBase * CClient::OnMakeConnection () {
    return new CClientConnection;
}

// For request based handling
const void * CServerConnection::OnRequest (const wxString &topic, const wxString &item, size_t *size, wxIPCFormat format) {
    const char * output;

    if (item == "blockamount") {
        stringstream stream;
        stream << nBestHeight + 1;
        output = stream.str().c_str();
    }
    else
        output = "Unknown identifier";
    
    return output;
}

// For event based handling
bool CClientConnection::OnAdvise (const wxString &topic, const wxString &item, const void *data, size_t size, wxIPCFormat format) {
    return false;
}