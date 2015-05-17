#include "omnicore_qtutils.h"

#include "guiutil.h"

#include <string>

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace mastercore
{

/**
 * Displays a 'transaction sent' message box containing the transaction ID and an extra button to copy txid to clipboard
 */
void PopulateTXSentDialog(const std::string& txidStr)
{
    std::string strSentText = "Your Omni Layer transaction has been sent.\n\nThe transaction ID is:\n\n" + txidStr + "\n\n";
    QMessageBox sentDialog;
    sentDialog.setIcon(QMessageBox::Information);
    sentDialog.setWindowTitle("Transaction broadcast successfully");
    sentDialog.setText(QString::fromStdString(strSentText));
    sentDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::Ok);
    sentDialog.setDefaultButton(QMessageBox::Ok);
    sentDialog.setButtonText( QMessageBox::Yes, "Copy TXID to clipboard" );
    if(sentDialog.exec() == QMessageBox::Yes) GUIUtil::setClipboard(QString::fromStdString(txidStr));
}

/**
 * Displays a simple dialog layout that can be used to provide selectable text to the user
 *
 * Note: used in place of standard dialogs in cases where text selection & copy to clipboard functions are useful
 */
void PopulateSimpleDialog(const std::string& content, const std::string& title, const std::string& tooltip)
{
    QDialog *simpleDlg = new QDialog;
    QLayout *dlgLayout = new QVBoxLayout;
    dlgLayout->setSpacing(12);
    dlgLayout->setMargin(12);
    QTextEdit *dlgTextEdit = new QTextEdit;
    dlgTextEdit->setText(QString::fromStdString(content));
    dlgTextEdit->setStatusTip(QString::fromStdString(tooltip));
    dlgTextEdit->setReadOnly(true);
    dlgTextEdit->setTextInteractionFlags(dlgTextEdit->textInteractionFlags() | Qt::TextSelectableByKeyboard);
    dlgLayout->addWidget(dlgTextEdit);
    QPushButton *closeButton = new QPushButton(QObject::tr("&Close"));
    closeButton->setDefault(true);
    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(closeButton, QDialogButtonBox::AcceptRole);
    dlgLayout->addWidget(buttonBox);
    QObject::connect(buttonBox, SIGNAL(accepted()), simpleDlg, SLOT(accept()));
    simpleDlg->setAttribute(Qt::WA_DeleteOnClose);
    simpleDlg->setWindowTitle(QString::fromStdString(title));
    simpleDlg->setLayout(dlgLayout);
    simpleDlg->resize(700, 360);
    if (simpleDlg->exec() == QDialog::Accepted) { } //do nothing but close
}

/**
 * Strips trailing zeros from a string containing a divisible value
 */
std::string StripTrailingZeros(const std::string& inputStr)
{
    size_t dot = inputStr.find(".");
    std::string outputStr = inputStr; // make a copy we will manipulate and return
    if (dot==std::string::npos) { // could not find a decimal marker, unsafe - return original input string
        return inputStr;
    }
    size_t lastZero = outputStr.find_last_not_of('0') + 1;
    if (lastZero > dot) { // trailing zeros are after decimal marker, safe to remove
        outputStr.erase ( lastZero, std::string::npos );
        if (outputStr.length() > 0) { std::string::iterator it = outputStr.end() - 1; if (*it == '.') { outputStr.erase(it); } } //get rid of trailing dot if needed
    } else { // last non-zero is before the decimal marker, this is a whole number
        outputStr.erase ( dot, std::string::npos );
    }
    return outputStr;
}

/**
 * Truncates a string at n digits and adds "..." to indicate that display is incomplete
 */
std::string TruncateString(const std::string& inputStr, unsigned int length)
{
    if (inputStr.empty()) return "";
    std::string outputStr = inputStr;
    if (length > 0 && inputStr.length() > length) {
        outputStr = inputStr.substr(0, length) + "...";
    }
    return outputStr;
}

/**
 * Variable length find and replace.  Find all iterations of findText within inputStr and replace them
 * with replaceText.
 */
std::string ReplaceStr(const std::string& findText, const std::string& replaceText, const std::string& inputStr)
{
    size_t start_pos = 0;
    std::string outputStr = inputStr;
    while((start_pos = outputStr.find(findText, start_pos)) != std::string::npos) {
        outputStr.replace(start_pos, findText.length(), replaceText);
        start_pos += replaceText.length();
    }
    return outputStr;
}

} // end namespace
