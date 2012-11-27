#include "importprivatekeydialog.h"
#include "ui_importprivatekeydialog.h"
#include "walletmodel.h"
#include "progressdialog.h"
#include "base58.h"
 #include <QValidator>
#include "version.h"
using namespace std;


PrivateKeyValidator::PrivateKeyValidator(QObject *parent) : QValidator(parent){
    sBase58= string("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz");
    parentObject=parent;
};

QValidator::State PrivateKeyValidator::validate (QString & input, int & pos ) const
{
    if (input.isEmpty()) {
        return Intermediate;
    }

    for ( int i = 0 ; i < input.length(); i++)
    {
       if (sBase58.find(input[i].toAscii())==string::npos)
           return Invalid;
    }

    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(input.toStdString());

    if (fGood) {
        return Acceptable;
    }

    return Intermediate;

}

ImportPrivateKeyDialog::ImportPrivateKeyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportPrivateKeyDialog)
{
    ui->setupUi(this);
    QValidator *validator = new PrivateKeyValidator(this);
    ui->privateKeyEdit1->setValidator(validator);
    //QObject::connect(validator, SIGNAL(keyValidityChanged(bool)),
    //                 this, SLOT(setOKEnabled(bool)));
}

void ImportPrivateKeyDialog::setModel(WalletModel *walletmodel)
{
    model=walletmodel;
}

ImportPrivateKeyDialog::~ImportPrivateKeyDialog()
{
    delete ui;
}

void ImportPrivateKeyDialog::on_buttonBox_accepted()
{
    //Import the key...
    string strSecret = ui->privateKeyEdit1->text().toStdString();
    string strLabel = ui->addressLabelEdit->text().toStdString();

    //TODO handle errors returned.
    ProgressDialog prgdlg(this);
    prgdlg.setModal (true );
    prgdlg.show();
    prgdlg.raise();
    prgdlg.activateWindow();
    connect(model, SIGNAL(ScanWalletTransactionsProgress(int)), &prgdlg, SLOT(UpdateProgress(int)));
    model->ImportPrivateKey(strSecret,strLabel);
    disconnect(model, SIGNAL(ScanWalletTransactionsProgress(int)), &prgdlg, SLOT(UpdateProgress(int)));
    prgdlg.hide();
    close();
}

void ImportPrivateKeyDialog::on_privateKeyEdit1_textChanged(const QString &arg1)
{
    //TODO - check if wallet encrypted and locked
    ui->buttonBox->setEnabled(
        ui->privateKeyEdit1->hasAcceptableInput ());
}
