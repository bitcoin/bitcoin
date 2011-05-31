#ifndef MAINOPTIONSPAGE_H
#define MAINOPTIONSPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
class QCheckBox;
class QLineEdit;
QT_END_NAMESPACE

class OptionsModel;

class MainOptionsPage : public QWidget
{
    Q_OBJECT
public:
    explicit MainOptionsPage(QWidget *parent=0);

    void setMapper(QDataWidgetMapper *mapper);
private:
    QCheckBox *bitcoin_at_startup;
    QCheckBox *minimize_to_tray;
    QCheckBox *map_port_upnp;
    QCheckBox *minimize_on_close;
    QCheckBox *connect_socks4;
    QLineEdit *proxy_ip;
    QLineEdit *proxy_port;
    QLineEdit *fee_edit;

signals:

public slots:

};

#endif // MAINOPTIONSPAGE_H
