
#include "qzecjsonrpcclient.h"
#include <QSettings>

ZecRpcClient::ZecRpcClient(const QString& zecEndPoint, const QString& zecRPCLogin, const QString& zecRPCPassword)
{
	QSettings settings;
	m_client.setEndpoint(zecEndPoint.size() > 0? zecEndPoint : settings.value("zecEndPoint", "").toString());
	m_client.setUsername(zecRPCLogin.size() > 0? zecRPCLogin : settings.value("zecRPCLogin", "").toString());
	m_client.setPassword(zecRPCPassword.size() > 0? zecRPCPassword : settings.value("zecRPCPassword", "").toString());
}
void ZecRpcClient::sendRequest(QNetworkAccessManager *nam, const QString &request, const QString &param)
{
	m_client.sendRequest(nam, request, param);
}
void ZecRpcClient::sendRawTxRequest(QNetworkAccessManager *nam, const QString &param)
{
	m_client.sendRequest(nam, "getrawtransaction", param, "1");
}
ZecRpcClient::~ZecRpcClient()
{
}