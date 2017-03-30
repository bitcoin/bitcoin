#ifndef RPCCLIENT_H
#define RPCCLIENT_H

#include <QDialog>
QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QAuthenticator;
class QNetworkReply;
QT_END_NAMESPACE

class RpcClient
{
public:
   

    explicit RpcClient();
    ~RpcClient();

	void setEndpoint(const QString &endPoint);
    void setUsername(const QString &username);
	void setPassword(const QString &password);
	
private:
	QString m_endpoint;
    QString m_username;
    QString m_password;

public:
	void sendRequest(QNetworkAccessManager *nam, const QString &request, const QString &param="", const QString &param1="");
};

#endif // RPCCLIENT_H
