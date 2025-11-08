// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MNEMONICVERIFICATIONDIALOG_H
#define BITCOIN_QT_MNEMONICVERIFICATIONDIALOG_H

#include <QDialog>
#include <QStackedWidget>

#include <support/allocators/secure.h>
#include <vector>

namespace Ui {
    class MnemonicVerificationDialog;
}

/** Dialog to verify mnemonic seed phrase by asking user to enter 3 random word positions */
class MnemonicVerificationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MnemonicVerificationDialog(const SecureString& mnemonic, QWidget *parent = nullptr, bool viewOnly = false);
    ~MnemonicVerificationDialog();

    void accept() override;

private Q_SLOTS:
    void onShowMnemonicClicked();
    void onHideMnemonicClicked();
    void onWord1Changed();
    void onWord2Changed();
    void onWord3Changed();
    void onShowMnemonicAgainClicked();

private:
    void setupStep1();
    void setupStep2();
    void generateRandomPositions();
    void updateWordValidation();
    bool validateWord(const QString& word, int position);
    void clearMnemonic();
    void buildMnemonicGrid(bool reveal);
    std::vector<SecureString> parseWords();
    void clearWordsSecurely();
    int getWordCount() const;

    Ui::MnemonicVerificationDialog *ui;
    SecureString m_mnemonic;
    std::vector<SecureString> m_words;
    QList<int> m_selected_positions;
    bool m_mnemonic_revealed;
    bool m_has_ever_revealed{false};
    bool m_view_only{false};
    class QGridLayout* m_gridLayout{nullptr};
    QSize m_defaultSize;
};

#endif // BITCOIN_QT_MNEMONICVERIFICATIONDIALOG_H

