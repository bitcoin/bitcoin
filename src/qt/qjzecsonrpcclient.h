#ifndef ZECHTTPCLIENT_H
#define ZECHTTPCLIENT_H

#include "qjsonhttpclient.h"
QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
QT_END_NAMESPACE

class ZecHttpClient
{
    Q_OBJECT

public:
 
    explicit ZecHttpClient();
    ~ZecHttpClient();
	void sendRequest(QNetworkAccessManager *nam, const QString &request, const QString &param="");
	void sendRawTxRequest(QNetworkAccessManager *nam, const QString &request, const QString &param);
private:
	HttpClient m_client;

};

#endif // ZECHTTPCLIENT_H
