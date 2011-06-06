// Copyright (c) 2011 Luke Dashjr
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <boost/algorithm/string.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/tokenizer.hpp>

#include "headers.h"

using namespace boost::interprocess;
using namespace boost;
using namespace std;

extern CMainFrame* pframeMain;

void ipcHandleURI_wx(const string strAddress, int64 nAmount)
{
    CSendDialog dialogSend(pframeMain, strAddress, nAmount);
    dialogSend.ShowModal();
}

const wxChar *ipcHandleURI_i(const string strURI)
{
    if (!boost::iequals(strURI.substr(0, 8), "bitcoin:"))
        return wxT("URI doesn't begin with 'bitcoin:'");
    string strAddress;
    int64 nAmount = 0;
    size_t nPos;

    nPos = strURI.find('?');
    if (nPos == string::npos)
    {
        strAddress = strURI.substr(8);
    }
    else
    {
        // Parse query string
        strAddress = strURI.substr(8, nPos - 8);
        char_separator<char> sepParams(";?&");
        tokenizer< char_separator<char> > tokParams(strURI.substr(nPos + 1), sepParams);
        string strKey, strValue;
        BOOST_FOREACH(string strKey, tokParams)
        {
            nPos = strKey.find('=');
            if (nPos == string::npos)
                strValue = "1";
            else
                strValue = strKey.substr(nPos + 1);
                strKey = strKey.substr(0, nPos);
            if (strKey == "amount")
                ParseMoney(strValue, nAmount);
        }
    }

    UIThreadCall(bind(ipcHandleURI_wx, strAddress, nAmount));

    return NULL;
}

void ipcHandleURI(const string strURI) {
    const wxChar *strError = ipcHandleURI_i(strURI);
    if (!strError)
        return;
    UIThreadCall(bind(wxMessageBox, strError, wxT("Error"), wxICON_ERROR | wxOK, (wxWindow*)NULL, -1, -1));
}

void ipcShutdown();

void ipcThread(void* parg)
{
    message_queue* mqIPC = (message_queue*)parg;
    char strBuf[256+1];
    size_t nSize;
    unsigned int nPriority;
    while (true)
    {
        mqIPC->receive(&strBuf, sizeof(strBuf), nSize, nPriority);
        strBuf[nSize] = '\0';
        ipcHandleURI(strBuf);
    }
    ipcShutdown();
}

int ipcInit()
{
    message_queue* mqIPC;
    try {
        mqIPC = new message_queue(open_or_create, "Bitcoin", 2, 256);
    }
    catch (interprocess_exception &ex) {
        return 1;
    }
    if (!CreateThread(ipcThread, mqIPC))
    {
        delete mqIPC;
        return 2;
    }
    return 0;
}

void ipcShutdown()
{
    message_queue::remove("Bitcoin");
}
