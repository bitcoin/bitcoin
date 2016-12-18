
#include "qbtcjsonrpcclient.h"
#include <QSettings>

BtcRpcClient::BtcRpcClient(const QString& btcEndPoint, const QString& btcRPCLogin, const QString& btcRPCPassword)
{
	QSettings settings;
	m_client.setEndpoint(btcEndPoint.size() > 0? btcEndPoint : settings.value("btcEndPoint", "").toString());
	m_client.setUsername(btcRPCLogin.size() > 0? btcRPCLogin : settings.value("btcRPCLogin", "").toString());
	m_client.setPassword(btcRPCPassword.size() > 0? btcRPCPassword : settings.value("btcRPCPassword", "").toString());
}
void BtcRpcClient::sendRequest(QNetworkAccessManager *nam, const QString &request, const QString &param)
{
	m_client.sendRequest(nam, request, param);
}
void BtcRpcClient::sendRawTxRequest(QNetworkAccessManager *nam, const QString &param)
{
	m_client.sendRequest(nam, "getrawtransaction", param, "1");
}
BtcRpcClient::~BtcRpcClient()
{
}