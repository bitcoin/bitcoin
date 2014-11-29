#ifndef MULTISIGADDRESSENTRY_H
#define MULTISIGADDRESSENTRY_H

#include <QFrame>


class WalletModel;

namespace Ui
{
    class MultisigAddressEntry;
}

class MultisigAddressEntry : public QFrame
{
    Q_OBJECT;

  public:
    explicit MultisigAddressEntry(QWidget *parent = 0);
    ~MultisigAddressEntry();
    void setModel(WalletModel *model);
    bool validate();
    QString getPubkey();

    public slots:
    void setRemoveEnabled(bool enabled);
    void clear();

  signals:
    void removeEntry(MultisigAddressEntry *entry);

  private:
    Ui::MultisigAddressEntry *ui;
    WalletModel *model;

  private slots:
    void on_pubkey_textChanged(const QString &pubkey);
    void on_pasteButton_clicked();
    void on_deleteButton_clicked();
    void on_address_textChanged(const QString &address);
    void on_addressBookButton_clicked();
};

#endif // MULTISIGADDRESSENTRY_H