#include "mainoptionspage.h"
#include "optionsmodel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QDataWidgetMapper>
#include <QDebug>

MainOptionsPage::MainOptionsPage(QWidget *parent):
        QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();

    bitcoin_at_startup = new QCheckBox(tr("&Start Bitcoin on window system startup"));
    layout->addWidget(bitcoin_at_startup);

    minimize_to_tray = new QCheckBox(tr("&Minimize to the tray instead of the taskbar"));
    layout->addWidget(minimize_to_tray);

    map_port_upnp = new QCheckBox(tr("Map port using &UPnP"));
    layout->addWidget(map_port_upnp);

    minimize_on_close = new QCheckBox(tr("M&inimize on close"));
    layout->addWidget(minimize_on_close);

    connect_socks4 = new QCheckBox(tr("&Connect through socks4 proxy:"));
    layout->addWidget(connect_socks4);

    QHBoxLayout *proxy_hbox = new QHBoxLayout();
    proxy_hbox->addSpacing(18);
    QLabel *proxy_ip_label = new QLabel(tr("Proxy &IP: "));
    proxy_hbox->addWidget(proxy_ip_label);
    proxy_ip = new QLineEdit();
    proxy_ip->setMaximumWidth(140);
    proxy_ip_label->setBuddy(proxy_ip);
    proxy_hbox->addWidget(proxy_ip);
    QLabel *proxy_port_label = new QLabel(tr("&Port: "));
    proxy_hbox->addWidget(proxy_port_label);
    proxy_port = new QLineEdit();
    proxy_port->setMaximumWidth(55);
    proxy_port_label->setBuddy(proxy_port);
    proxy_hbox->addWidget(proxy_port);
    proxy_hbox->addStretch(1);

    layout->addLayout(proxy_hbox);
    QLabel *fee_help = new QLabel(tr("Optional transaction fee per KB that helps make sure your transactions are processed quickly.  Most transactions are 1KB.  Fee 0.01 recommended."));
    fee_help->setWordWrap(true);
    layout->addWidget(fee_help);

    QHBoxLayout *fee_hbox = new QHBoxLayout();
    fee_hbox->addSpacing(18);
    QLabel *fee_label = new QLabel(tr("Pay transaction &fee"));
    fee_hbox->addWidget(fee_label);
    fee_edit = new QLineEdit();
    fee_edit->setMaximumWidth(70);
    fee_label->setBuddy(fee_edit);
    fee_hbox->addWidget(fee_edit);
    fee_hbox->addStretch(1);

    layout->addLayout(fee_hbox);

    layout->addStretch(1); /* Extra space at bottom */

    setLayout(layout);
}

void MainOptionsPage::setMapper(QDataWidgetMapper *mapper)
{
    /* Map model to widgets */
    mapper->addMapping(bitcoin_at_startup, OptionsModel::StartAtStartup);
    mapper->addMapping(minimize_to_tray, OptionsModel::MinimizeToTray);
    mapper->addMapping(map_port_upnp, OptionsModel::MapPortUPnP);
    mapper->addMapping(minimize_on_close, OptionsModel::MinimizeOnClose);
    mapper->addMapping(connect_socks4, OptionsModel::ConnectSOCKS4);
    mapper->addMapping(proxy_ip, OptionsModel::ProxyIP);
    mapper->addMapping(proxy_port, OptionsModel::ProxyPort);
    mapper->addMapping(fee_edit, OptionsModel::Fee);
}

