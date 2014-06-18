// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "paymentserver.h"

#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"

#include "base58.h"
#include "ui_interface.h"
#include "wallet.h"

#include <cstdlib>

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileOpenEvent>
#include <QHash>
#include <QList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslCertificate>
#include <QSslError>
#include <QSslSocket>
#include <QStringList>
#include <QTextDocument>

#if QT_VERSION < 0x050000
#include <QUrl>
#else
#include <QUrlQuery>
#endif

using namespace std;
using namespace boost;

const int BITCOIN_IPC_CONNECT_TIMEOUT = 1000; // milliseconds
const QString BITCOIN_IPC_PREFIX("bitcoin:");
const char* BITCOIN_REQUEST_MIMETYPE = "application/bitcoin-paymentrequest";
const char* BITCOIN_PAYMENTACK_MIMETYPE = "application/bitcoin-paymentack";
const char* BITCOIN_PAYMENTACK_CONTENTTYPE = "application/bitcoin-payment";

X509_STORE* PaymentServer::certStore = NULL;
void PaymentServer::freeCertStore()
{
    if (PaymentServer::certStore != NULL)
    {
        X509_STORE_free(PaymentServer::certStore);
        PaymentServer::certStore = NULL;
    }
}

//
// Create a name that is unique for:
//  testnet / non-testnet
//  data directory
//
static QString ipcServerName()
{
    QString name("BitcoinQt");

    // Append a simple hash of the datadir
    // Note that GetDataDir(true) returns a different path
    // for -testnet versus main net
    QString ddir(QString::fromStdString(GetDataDir(true).string()));
    name.append(QString::number(qHash(ddir)));

    return name;
}

//
// We store payment URIs and requests received before
// the main GUI window is up and ready to ask the user
// to send payment.

static QList<QString> savedPaymentRequests;

static void ReportInvalidCertificate(const QSslCertificate& cert)
{
    qDebug() << "ReportInvalidCertificate : Payment server found an invalid certificate: " << cert.subjectInfo(QSslCertificate::CommonName);
}

//
// Load OpenSSL's list of root certificate authorities
//
void PaymentServer::LoadRootCAs(X509_STORE* _store)
{
    if (PaymentServer::certStore == NULL)
        atexit(PaymentServer::freeCertStore);
    else
        freeCertStore();

    // Unit tests mostly use this, to pass in fake root CAs:
    if (_store)
    {
        PaymentServer::certStore = _store;
        return;
    }

    // Normal execution, use either -rootcertificates or system certs:
    PaymentServer::certStore = X509_STORE_new();

    // Note: use "-system-" default here so that users can pass -rootcertificates=""
    // and get 'I don't like X.509 certificates, don't trust anybody' behavior:
    QString certFile = QString::fromStdString(GetArg("-rootcertificates", "-system-"));

    if (certFile.isEmpty())
        return; // Empty store

    QList<QSslCertificate> certList;

    if (certFile != "-system-")
    {
        certList = QSslCertificate::fromPath(certFile);
        // Use those certificates when fetching payment requests, too:
        QSslSocket::setDefaultCaCertificates(certList);
    }
    else
        certList = QSslSocket::systemCaCertificates ();

    int nRootCerts = 0;
    const QDateTime currentTime = QDateTime::currentDateTime();
    foreach (const QSslCertificate& cert, certList)
    {
        if (currentTime < cert.effectiveDate() || currentTime > cert.expiryDate()) {
            ReportInvalidCertificate(cert);
            continue;
        }
#if QT_VERSION >= 0x050000
        if (cert.isBlacklisted()) {
            ReportInvalidCertificate(cert);
            continue;
        }
#endif
        QByteArray certData = cert.toDer();
        const unsigned char *data = (const unsigned char *)certData.data();

        X509* x509 = d2i_X509(0, &data, certData.size());
        if (x509 && X509_STORE_add_cert(PaymentServer::certStore, x509))
        {
            // Note: X509_STORE_free will free the X509* objects when
            // the PaymentServer is destroyed
            ++nRootCerts;
        }
        else
        {
            ReportInvalidCertificate(cert);
            continue;
        }
    }
    qDebug() << "PaymentServer::LoadRootCAs : Loaded " << nRootCerts << " root certificates";

    // Project for another day:
    // Fetch certificate revocation lists, and add them to certStore.
    // Issues to consider:
    //   performance (start a thread to fetch in background?)
    //   privacy (fetch through tor/proxy so IP address isn't revealed)
    //   would it be easier to just use a compiled-in blacklist?
    //    or use Qt's blacklist?
    //   "certificate stapling" with server-side caching is more efficient
}

//
// Sending to the server is done synchronously, at startup.
// If the server isn't already running, startup continues,
// and the items in savedPaymentRequest will be handled
// when uiReady() is called.
//
// Warning: ipcSendCommandLine() is called early in init,
// so don't use "emit message()", but "QMessageBox::"!
//
bool PaymentServer::ipcParseCommandLine(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        QString arg(argv[i]);
        if (arg.startsWith("-"))
            continue;

        if (arg.startsWith(BITCOIN_IPC_PREFIX, Qt::CaseInsensitive)) // bitcoin: URI
        {
            savedPaymentRequests.append(arg);

            SendCoinsRecipient r;
            if (GUIUtil::parseBitcoinURI(arg, &r))
            {
                CBitcoinAddress address(r.address.toStdString());

                SelectParams(CChainParams::MAIN);
                if (!address.IsValid())
                {
                    SelectParams(CChainParams::TESTNET);
                }
            }
        }
        else if (QFile::exists(arg)) // Filename
        {
            savedPaymentRequests.append(arg);

            PaymentRequestPlus request;
            if (readPaymentRequest(arg, request))
            {
                if (request.getDetails().network() == "main")
                    SelectParams(CChainParams::MAIN);
                else
                    SelectParams(CChainParams::TESTNET);
            }
        }
        else
        {
            // Printing to debug.log is about the best we can do here, the
            // GUI hasn't started yet so we can't pop up a message box.
            qDebug() << "PaymentServer::ipcSendCommandLine : Payment request file does not exist: " << arg;
        }
    }
    return true;
}

//
// Sending to the server is done synchronously, at startup.
// If the server isn't already running, startup continues,
// and the items in savedPaymentRequest will be handled
// when uiReady() is called.
//
bool PaymentServer::ipcSendCommandLine()
{
    bool fResult = false;
    foreach (const QString& r, savedPaymentRequests)
    {
        QLocalSocket* socket = new QLocalSocket();
        socket->connectToServer(ipcServerName(), QIODevice::WriteOnly);
        if (!socket->waitForConnected(BITCOIN_IPC_CONNECT_TIMEOUT))
        {
            delete socket;
            return false;
        }

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);
        out << r;
        out.device()->seek(0);
        socket->write(block);
        socket->flush();

        socket->waitForBytesWritten(BITCOIN_IPC_CONNECT_TIMEOUT);
        socket->disconnectFromServer();
        delete socket;
        fResult = true;
    }

    return fResult;
}

PaymentServer::PaymentServer(QObject* parent, bool startLocalServer) :
    QObject(parent),
    saveURIs(true),
    uriServer(0),
    netManager(0),
    optionsModel(0)
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Install global event filter to catch QFileOpenEvents
    // on Mac: sent when you click bitcoin: links
    // other OSes: helpful when dealing with payment request files (in the future)
    if (parent)
        parent->installEventFilter(this);

    QString name = ipcServerName();

    // Clean up old socket leftover from a crash:
    QLocalServer::removeServer(name);

    if (startLocalServer)
    {
        uriServer = new QLocalServer(this);

        if (!uriServer->listen(name)) {
            // constructor is called early in init, so don't use "emit message()" here
            QMessageBox::critical(0, tr("Payment request error"),
                tr("Cannot start bitcoin: click-to-pay handler"));
        }
        else {
            connect(uriServer, SIGNAL(newConnection()), this, SLOT(handleURIConnection()));
            connect(this, SIGNAL(receivedPaymentACK(QString)), this, SLOT(handlePaymentACK(QString)));
        }
    }
}

PaymentServer::~PaymentServer()
{
    google::protobuf::ShutdownProtobufLibrary();
}

//
// OSX-specific way of handling bitcoin: URIs and
// PaymentRequest mime types
//
bool PaymentServer::eventFilter(QObject *object, QEvent *event)
{
    // clicking on bitcoin: URIs creates FileOpen events on the Mac
    if (event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent*>(event);
        if (!fileEvent->file().isEmpty())
            handleURIOrFile(fileEvent->file());
        else if (!fileEvent->url().isEmpty())
            handleURIOrFile(fileEvent->url().toString());

        return true;
    }

    return QObject::eventFilter(object, event);
}

void PaymentServer::initNetManager()
{
    if (!optionsModel)
        return;
    if (netManager != NULL)
        delete netManager;

    // netManager is used to fetch paymentrequests given in bitcoin: URIs
    netManager = new QNetworkAccessManager(this);

    QNetworkProxy proxy;

    // Query active proxy (fails if no SOCKS5 proxy)
    if (optionsModel->getProxySettings(proxy)) {
        if (proxy.type() == QNetworkProxy::Socks5Proxy) {
            netManager->setProxy(proxy);

            qDebug() << "PaymentServer::initNetManager : Using SOCKS5 proxy" << proxy.hostName() << ":" << proxy.port();
        }
        else
            qDebug() << "PaymentServer::initNetManager : No active proxy server found.";
    }
    else
        emit message(tr("Net manager warning"),
            tr("Your active proxy doesn't support SOCKS5, which is required for payment requests via proxy."),
            CClientUIInterface::MSG_WARNING);

    connect(netManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(netRequestFinished(QNetworkReply*)));
    connect(netManager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
            this, SLOT(reportSslErrors(QNetworkReply*, const QList<QSslError> &)));
}

void PaymentServer::uiReady()
{
    initNetManager();

    saveURIs = false;
    foreach (const QString& s, savedPaymentRequests)
    {
        handleURIOrFile(s);
    }
    savedPaymentRequests.clear();
}

void PaymentServer::handleURIOrFile(const QString& s)
{
    if (saveURIs)
    {
        savedPaymentRequests.append(s);
        return;
    }

    if (s.startsWith(BITCOIN_IPC_PREFIX, Qt::CaseInsensitive)) // bitcoin: URI
    {
#if QT_VERSION < 0x050000
        QUrl uri(s);
#else
        QUrlQuery uri((QUrl(s)));
#endif
        if (uri.hasQueryItem("r")) // payment request URI
        {
            QByteArray temp;
            temp.append(uri.queryItemValue("r"));
            QString decoded = QUrl::fromPercentEncoding(temp);
            QUrl fetchUrl(decoded, QUrl::StrictMode);

            if (fetchUrl.isValid())
            {
                qDebug() << "PaymentServer::handleURIOrFile : fetchRequest(" << fetchUrl << ")";
                fetchRequest(fetchUrl);
            }
            else
            {
                qDebug() << "PaymentServer::handleURIOrFile : Invalid URL: " << fetchUrl;
                emit message(tr("URI handling"),
                    tr("Payment request fetch URL is invalid: %1").arg(fetchUrl.toString()),
                    CClientUIInterface::ICON_WARNING);
            }

            return;
        }
        else // normal URI
        {
            SendCoinsRecipient recipient;
            if (GUIUtil::parseBitcoinURI(s, &recipient))
            {
                CBitcoinAddress address(recipient.address.toStdString());
                if (!address.IsValid()) {
                    emit message(tr("URI handling"), tr("Invalid payment address %1").arg(recipient.address),
                        CClientUIInterface::MSG_ERROR);
                }
                else
                    emit receivedPaymentRequest(recipient);
            }
            else
                emit message(tr("URI handling"),
                    tr("URI can not be parsed! This can be caused by an invalid Bitcoin address or malformed URI parameters."),
                    CClientUIInterface::ICON_WARNING);

            return;
        }
    }

    if (QFile::exists(s)) // payment request file
    {
        PaymentRequestPlus request;
        SendCoinsRecipient recipient;
        if (!readPaymentRequest(s, request))
        {
            emit message(tr("Payment request file handling"),
                tr("Payment request file can not be read! This can be caused by an invalid payment request file."),
                CClientUIInterface::ICON_WARNING);
        }
        else if (processPaymentRequest(request, recipient))
            emit receivedPaymentRequest(recipient);

        return;
    }
}

void PaymentServer::handleURIConnection()
{
    QLocalSocket *clientConnection = uriServer->nextPendingConnection();

    while (clientConnection->bytesAvailable() < (int)sizeof(quint32))
        clientConnection->waitForReadyRead();

    connect(clientConnection, SIGNAL(disconnected()),
            clientConnection, SLOT(deleteLater()));

    QDataStream in(clientConnection);
    in.setVersion(QDataStream::Qt_4_0);
    if (clientConnection->bytesAvailable() < (int)sizeof(quint16)) {
        return;
    }
    QString msg;
    in >> msg;

    handleURIOrFile(msg);
}

bool PaymentServer::readPaymentRequest(const QString& filename, PaymentRequestPlus& request)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
    {
        qDebug() << "PaymentServer::readPaymentRequest : Failed to open " << filename;
        return false;
    }

    if (f.size() > MAX_PAYMENT_REQUEST_SIZE)
    {
        qDebug() << "PaymentServer::readPaymentRequest : " << filename << " too large";
        return false;
    }

    QByteArray data = f.readAll();

    return request.parse(data);
}

bool PaymentServer::processPaymentRequest(PaymentRequestPlus& request, SendCoinsRecipient& recipient)
{
    if (!optionsModel)
        return false;

    if (request.IsInitialized()) {
        const payments::PaymentDetails& details = request.getDetails();

        // Payment request network matches client network?
        if (details.network() != Params().NetworkIDString())
        {
            emit message(tr("Payment request rejected"), tr("Payment request network doesn't match client network."),
                CClientUIInterface::MSG_ERROR);

            return false;
        }

        // Expired payment request?
        if (details.has_expires() && (int64_t)details.expires() < GetTime())
        {
            emit message(tr("Payment request rejected"), tr("Payment request has expired."),
                CClientUIInterface::MSG_ERROR);

            return false;
        }
    }
    else {
        emit message(tr("Payment request error"), tr("Payment request is not initialized."),
            CClientUIInterface::MSG_ERROR);

        return false;
    }

    recipient.paymentRequest = request;
    recipient.message = GUIUtil::HtmlEscape(request.getDetails().memo());

    request.getMerchant(PaymentServer::certStore, recipient.authenticatedMerchant);

    QList<std::pair<CScript, qint64> > sendingTos = request.getPayTo();
    QStringList addresses;

    foreach(const PAIRTYPE(CScript, qint64)& sendingTo, sendingTos) {
        // Extract and check destination addresses
        CTxDestination dest;
        if (ExtractDestination(sendingTo.first, dest)) {
            // Append destination address
            addresses.append(QString::fromStdString(CBitcoinAddress(dest).ToString()));
        }
        else if (!recipient.authenticatedMerchant.isEmpty()) {
            // Insecure payments to custom bitcoin addresses are not supported
            // (there is no good way to tell the user where they are paying in a way
            // they'd have a chance of understanding).
            emit message(tr("Payment request rejected"),
                tr("Unverified payment requests to custom payment scripts are unsupported."),
                CClientUIInterface::MSG_ERROR);
            return false;
        }

        // Extract and check amounts
        CTxOut txOut(sendingTo.second, sendingTo.first);
        if (txOut.IsDust(CTransaction::minRelayTxFee)) {
            emit message(tr("Payment request error"), tr("Requested payment amount of %1 is too small (considered dust).")
                .arg(BitcoinUnits::formatWithUnit(optionsModel->getDisplayUnit(), sendingTo.second)),
                CClientUIInterface::MSG_ERROR);

            return false;
        }

        recipient.amount += sendingTo.second;
    }
    // Store addresses and format them to fit nicely into the GUI
    recipient.address = addresses.join("<br />");

    if (!recipient.authenticatedMerchant.isEmpty()) {
        qDebug() << "PaymentServer::processPaymentRequest : Secure payment request from " << recipient.authenticatedMerchant;
    }
    else {
        qDebug() << "PaymentServer::processPaymentRequest : Insecure payment request to " << addresses.join(", ");
    }

    return true;
}

void PaymentServer::fetchRequest(const QUrl& url)
{
    QNetworkRequest netRequest;
    netRequest.setAttribute(QNetworkRequest::User, "PaymentRequest");
    netRequest.setUrl(url);
    netRequest.setRawHeader("User-Agent", CLIENT_NAME.c_str());
    netRequest.setRawHeader("Accept", BITCOIN_REQUEST_MIMETYPE);
    netManager->get(netRequest);
}

void PaymentServer::fetchPaymentACK(CWallet* wallet, SendCoinsRecipient recipient, QByteArray transaction)
{
    const payments::PaymentDetails& details = recipient.paymentRequest.getDetails();
    if (!details.has_payment_url())
        return;

    QNetworkRequest netRequest;
    netRequest.setAttribute(QNetworkRequest::User, "PaymentACK");
    netRequest.setUrl(QString::fromStdString(details.payment_url()));
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader, BITCOIN_PAYMENTACK_CONTENTTYPE);
    netRequest.setRawHeader("User-Agent", CLIENT_NAME.c_str());
    netRequest.setRawHeader("Accept", BITCOIN_PAYMENTACK_MIMETYPE);

    payments::Payment payment;
    payment.set_merchant_data(details.merchant_data());
    payment.add_transactions(transaction.data(), transaction.size());

    // Create a new refund address, or re-use:
    QString account = tr("Refund from %1").arg(recipient.authenticatedMerchant);
    std::string strAccount = account.toStdString();
    set<CTxDestination> refundAddresses = wallet->GetAccountAddresses(strAccount);
    if (!refundAddresses.empty()) {
        CScript s; s.SetDestination(*refundAddresses.begin());
        payments::Output* refund_to = payment.add_refund_to();
        refund_to->set_script(&s[0], s.size());
    }
    else {
        CPubKey newKey;
        if (wallet->GetKeyFromPool(newKey)) {
            LOCK(wallet->cs_wallet); // SetAddressBook
            CKeyID keyID = newKey.GetID();
            wallet->SetAddressBook(keyID, strAccount, "refund");

            CScript s; s.SetDestination(keyID);
            payments::Output* refund_to = payment.add_refund_to();
            refund_to->set_script(&s[0], s.size());
        }
        else {
            // This should never happen, because sending coins should have
            // just unlocked the wallet and refilled the keypool.
            qDebug() << "PaymentServer::fetchPaymentACK : Error getting refund key, refund_to not set";
        }
    }

    int length = payment.ByteSize();
    netRequest.setHeader(QNetworkRequest::ContentLengthHeader, length);
    QByteArray serData(length, '\0');
    if (payment.SerializeToArray(serData.data(), length)) {
        netManager->post(netRequest, serData);
    }
    else {
        // This should never happen, either.
        qDebug() << "PaymentServer::fetchPaymentACK : Error serializing payment message";
    }
}

void PaymentServer::netRequestFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError)
    {
        QString msg = tr("Error communicating with %1: %2")
            .arg(reply->request().url().toString())
            .arg(reply->errorString());

        qDebug() << "PaymentServer::netRequestFinished : " << msg;
        emit message(tr("Payment request error"), msg, CClientUIInterface::MSG_ERROR);
        return;
    }

    QByteArray data = reply->readAll();

    QString requestType = reply->request().attribute(QNetworkRequest::User).toString();
    if (requestType == "PaymentRequest")
    {
        PaymentRequestPlus request;
        SendCoinsRecipient recipient;
        if (!request.parse(data))
        {
            qDebug() << "PaymentServer::netRequestFinished : Error parsing payment request";
            emit message(tr("Payment request error"),
                tr("Payment request can not be parsed!"),
                CClientUIInterface::MSG_ERROR);
        }
        else if (processPaymentRequest(request, recipient))
            emit receivedPaymentRequest(recipient);

        return;
    }
    else if (requestType == "PaymentACK")
    {
        payments::PaymentACK paymentACK;
        if (!paymentACK.ParseFromArray(data.data(), data.size()))
        {
            QString msg = tr("Bad response from server %1")
                .arg(reply->request().url().toString());

            qDebug() << "PaymentServer::netRequestFinished : " << msg;
            emit message(tr("Payment request error"), msg, CClientUIInterface::MSG_ERROR);
        }
        else
        {
            emit receivedPaymentACK(GUIUtil::HtmlEscape(paymentACK.memo()));
        }
    }
}

void PaymentServer::reportSslErrors(QNetworkReply* reply, const QList<QSslError> &errs)
{
    Q_UNUSED(reply);

    QString errString;
    foreach (const QSslError& err, errs) {
        qDebug() << "PaymentServer::reportSslErrors : " << err;
        errString += err.errorString() + "\n";
    }
    emit message(tr("Network request error"), errString, CClientUIInterface::MSG_ERROR);
}

void PaymentServer::setOptionsModel(OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
}

void PaymentServer::handlePaymentACK(const QString& paymentACKMsg)
{
    // currently we don't futher process or store the paymentACK message
    emit message(tr("Payment acknowledged"), paymentACKMsg, CClientUIInterface::ICON_INFORMATION | CClientUIInterface::MODAL);
}
