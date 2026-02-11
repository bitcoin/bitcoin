// Copyright (c) 2024-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/mnemonicverificationdialog.h>
#include <qt/forms/ui_mnemonicverificationdialog.h>

#include <qt/guiutil.h>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QMessageBox>

#include <algorithm>

MnemonicVerificationDialog::MnemonicVerificationDialog(const SecureString& mnemonic, QWidget *parent, bool viewOnly) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::MnemonicVerificationDialog),
    m_mnemonic(mnemonic),
    m_view_only(viewOnly)
{
    ui->setupUi(this);

    if (auto w = findChild<QWidget*>("mnemonicGridWidget")) {
        m_gridLayout = qobject_cast<QGridLayout*>(w->layout());
    }

    // Keep minimum small so the page can compress when users scale down
    setMinimumSize(QSize(550, 360));
    resize(minimumSize());

    setWindowTitle(m_view_only ? tr("Your Recovery Phrase") : tr("Save Your Mnemonic"));

    // Words will be parsed on-demand to minimize exposure time in non-secure memory
    // m_words is intentionally left empty initially

    // Trim outer paddings and inter-item spacing to avoid over-padded look
    if (auto mainLayout = findChild<QVBoxLayout*>("verticalLayout")) {
        mainLayout->setContentsMargins(8, 4, 8, 6);
        mainLayout->setSpacing(6);
    }
    if (auto s1 = findChild<QVBoxLayout*>("verticalLayout_step1")) {
        s1->setContentsMargins(8, 4, 8, 6);
        s1->setSpacing(6);
    }
    if (auto s2 = findChild<QVBoxLayout*>("verticalLayout_step2")) {
        s2->setContentsMargins(8, 2, 8, 6);
        s2->setSpacing(4);
        s2->setAlignment(Qt::AlignTop);
    }
    if (ui->formLayout) {
        ui->formLayout->setContentsMargins(0, 0, 0, 0);
        ui->formLayout->setVerticalSpacing(3);
        ui->formLayout->setHorizontalSpacing(8);
    }
    if (ui->buttonBox) {
        ui->buttonBox->setContentsMargins(0, 0, 0, 0);
        ui->buttonBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    }

    // Prefer compact default; we will adjust per-step to sizeHint
    ui->stackedWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    // Ensure buttonBox is hidden initially (will be shown in step2)
    ui->buttonBox->hide();
    setupStep1();
    adjustSize();
    m_defaultSize = size();

    // Connections
    connect(ui->showMnemonicButton, &QPushButton::clicked, this, &MnemonicVerificationDialog::onShowMnemonicClicked);
    connect(ui->hideMnemonicButton, &QPushButton::clicked, this, &MnemonicVerificationDialog::onHideMnemonicClicked);

    if (!m_view_only) {
        connect(ui->writtenDownCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
            if (checked && m_has_ever_revealed) setupStep2();
        });
        connect(ui->word1Edit, &QLineEdit::textChanged, this, &MnemonicVerificationDialog::onWord1Changed);
        connect(ui->word2Edit, &QLineEdit::textChanged, this, &MnemonicVerificationDialog::onWord2Changed);
        connect(ui->word3Edit, &QLineEdit::textChanged, this, &MnemonicVerificationDialog::onWord3Changed);
    }

    // Button box
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(m_view_only ? tr("Close") : tr("Continue"));
    GUIUtil::handleCloseWindowShortcut(this);
}

MnemonicVerificationDialog::~MnemonicVerificationDialog()
{
    delete ui;
}

void MnemonicVerificationDialog::setupStep1()
{
    ui->stackedWidget->setCurrentIndex(0);
    buildMnemonicGrid(false);
    ui->hideMnemonicButton->hide();
    ui->showMnemonicButton->show();
    ui->writtenDownCheckbox->setEnabled(false);
    ui->writtenDownCheckbox->setChecked(false);
    m_mnemonic_revealed = false;

    // In view-only mode, hide the checkbox and show buttonBox immediately
    if (m_view_only) {
        ui->writtenDownCheckbox->hide();
        ui->buttonBox->show();
        // Hide Cancel button in view-only mode - only Close button is needed
        if (QAbstractButton* cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel)) {
            cancelBtn->hide();
        }
    } else {
        ui->writtenDownCheckbox->show();
        ui->buttonBox->hide();
    }

    // Restore original minimum size (in case we came back from Step 2)
    setMinimumSize(QSize(550, 360));

    // Restore to default size if we have it (when coming back from Step 2)
    if (m_defaultSize.isValid() && !m_defaultSize.isEmpty()) {
        resize(m_defaultSize);
    } else {
        // Compact to content on first load
        adjustSize();
    }

    // Set warning and instruction text with themed colors
    // Font sizes and weights are defined in general.css
    if (m_view_only) {
        ui->warningLabel->setText(
            tr("WARNING: Never share your recovery phrase with anyone. Store it securely offline."));
        ui->warningLabel->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR));
        ui->instructionLabel->setText(
            tr("These words can restore your wallet. Keep them safe and private."));
    } else {
        ui->warningLabel->setText(
            tr("WARNING: If you lose your mnemonic seed phrase, you will lose access to your wallet forever. Write it down in a safe place and never share it with anyone."));
        ui->warningLabel->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR));
        ui->instructionLabel->setText(
            tr("Please write down these words in order. You will need them to restore your wallet."));
    }

    // Reduce extra padding to avoid an over-padded look
    if (auto outer = findChild<QVBoxLayout*>("verticalLayout_step1")) {
        outer->setContentsMargins(12, 6, 12, 6);
        outer->setSpacing(6);
    }
    if (auto hb = findChild<QHBoxLayout*>("horizontalLayout_buttons")) {
        hb->setContentsMargins(0, 4, 0, 0);
        hb->setSpacing(10);
    }
}

void MnemonicVerificationDialog::setupStep2()
{
    ui->stackedWidget->setCurrentIndex(1);
    // Parse words for validation (needed in step 2)
    parseWords();

    // Validate mnemonic has at least 3 words before proceeding
    const int wordCount = getWordCount();
    if (wordCount < 3) {
        QMessageBox::critical(this, tr("Invalid Mnemonic"),
            tr("Mnemonic phrase has fewer than 3 words (found %1). Verification cannot proceed.").arg(wordCount));
        reject();
        return;
    }

    generateRandomPositions();

    // Safety check: ensure positions were generated successfully
    if (m_selected_positions.size() < 3) {
        QMessageBox::critical(this, tr("Verification Error"),
            tr("Failed to generate verification positions. Please try again."));
        reject();
        return;
    }

    ui->word1Edit->clear();
    ui->word2Edit->clear();
    ui->word3Edit->clear();
    // Widget sizes are defined in general.css

    ui->word1Label->setText(tr("Word #%1:").arg(m_selected_positions[0]));
    ui->word2Label->setText(tr("Word #%1:").arg(m_selected_positions[1]));
    ui->word3Label->setText(tr("Word #%1:").arg(m_selected_positions[2]));

    ui->word1Status->clear();
    ui->word2Status->clear();
    ui->word3Status->clear();

    ui->buttonBox->show();
    if (QAbstractButton* cancel = ui->buttonBox->button(QDialogButtonBox::Cancel)) {
        cancel->setText(tr("Back"));
    }
    if (QAbstractButton* cont = ui->buttonBox->button(QDialogButtonBox::Ok)) cont->setEnabled(false);

    // Verification label styling is defined in general.css

    // Hide any existing title label if present
    if (auto titleLabel = findChild<QLabel*>("verifyTitleLabel")) {
        titleLabel->hide();
    }

    // Align content toward top and remove any layout spacers expanding height FIRST
    if (auto v = findChild<QVBoxLayout*>("verticalLayout_step2")) {
        v->setAlignment(Qt::AlignTop);
        // Minimize top padding/margin to eliminate gap at top
        QMargins m = v->contentsMargins();
        v->setContentsMargins(m.left(), 2, m.right(), m.bottom());
        for (int i = 0; i < v->count(); ++i) {
            QLayoutItem* it = v->itemAt(i);
            if (it && it->spacerItem()) it->spacerItem()->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
        // Force immediate layout update
        v->invalidate();
        v->update();
    }
    // Also ensure the main dialog layout has minimal top padding
    if (auto mainLayout = findChild<QVBoxLayout*>("verticalLayout")) {
        QMargins m = mainLayout->contentsMargins();
        mainLayout->setContentsMargins(m.left(), 4, m.right(), m.bottom());
        mainLayout->invalidate();
        mainLayout->update();
    }
    // Force widget to recalculate layout immediately
    updateGeometry();

    // Reduce minimums for verify; open exactly at the minimum size AFTER layout is fixed
    setMinimumSize(QSize(460, 280));
    resize(minimumSize());
    adjustSize();
}

void MnemonicVerificationDialog::generateRandomPositions()
{
    m_selected_positions.clear();
    const int n = getWordCount();
    if (n < 3) {
        // Unable to verify; bail out so the dialog can surface an error message upstream.
        return;
    }
    QSet<int> used;
    QRandomGenerator* rng = QRandomGenerator::global();
    while (m_selected_positions.size() < 3) {
        int pos = rng->bounded(1, n + 1);
        if (!used.contains(pos)) { used.insert(pos); m_selected_positions.append(pos); }
    }
    std::sort(m_selected_positions.begin(), m_selected_positions.end());
}

void MnemonicVerificationDialog::onShowMnemonicClicked()
{
    buildMnemonicGrid(true);
    ui->showMnemonicButton->hide();
    ui->hideMnemonicButton->show();
    if (!m_view_only) {
        ui->writtenDownCheckbox->setEnabled(true);
    }
    m_mnemonic_revealed = true;
    m_has_ever_revealed = true;
}

void MnemonicVerificationDialog::onHideMnemonicClicked()
{
    buildMnemonicGrid(false);
    ui->hideMnemonicButton->hide();
    ui->showMnemonicButton->show();
    m_mnemonic_revealed = false;
    // Clear parsed words from memory immediately when hiding
    m_words.clear();
}

void MnemonicVerificationDialog::reject()
{
    // Eagerly clear parsed words to minimize exposure time in memory.
    // They will be re-parsed on demand via parseWords() if needed.
    m_words.clear();
    // close dialog for step-1; return back to step-1 for step-2
    if (ui->stackedWidget->currentIndex() == 0) {
        QDialog::reject();
    } else {
        setupStep1();
    }
}

void MnemonicVerificationDialog::onWord1Changed() { updateWordValidation(); }
void MnemonicVerificationDialog::onWord2Changed() { updateWordValidation(); }
void MnemonicVerificationDialog::onWord3Changed() { updateWordValidation(); }

bool MnemonicVerificationDialog::validateWord(const QString& word, int position)
{
    // Parse words on-demand for validation (minimizes exposure time)
    // Words are kept in memory during step 2 (verification) and step 1 (when revealed)
    // They are only cleared when explicitly hiding in step 1 or on dialog destruction
    std::vector<SecureString> words = parseWords();
    if (position < 1 || position > static_cast<int>(words.size())) {
        return false;
    }
    // Convert SecureString to QString temporarily for comparison
    QString secureWord{QString::fromUtf8(words[position - 1].data(), words[position - 1].size()).toLower()};
    const bool result{word == secureWord};
    // Clear temporary QString immediately
    secureWord.fill(QChar(0));
    secureWord.clear();
    secureWord.squeeze();
    return result;
}

void MnemonicVerificationDialog::updateWordValidation()
{
    const QString t1 = ui->word1Edit->text().trimmed().toLower();
    const QString t2 = ui->word2Edit->text().trimmed().toLower();
    const QString t3 = ui->word3Edit->text().trimmed().toLower();

    const bool ok1 = !t1.isEmpty() && validateWord(t1, m_selected_positions[0]);
    const bool ok2 = !t2.isEmpty() && validateWord(t2, m_selected_positions[1]);
    const bool ok3 = !t3.isEmpty() && validateWord(t3, m_selected_positions[2]);

    // Status labels show checkmarks/X marks with themed colors
    // Font weight is defined in general.css
    auto setStatus = [](QLabel* lbl, bool filled, bool valid) {
        if (!lbl) return;
        if (!filled) { lbl->clear(); lbl->setStyleSheet(""); return; }
        lbl->setText(valid ? "✓" : "✗");
        lbl->setStyleSheet(valid ?
            GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_SUCCESS) :
            GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR));
    };
    setStatus(ui->word1Status, !t1.isEmpty(), ok1);
    setStatus(ui->word2Status, !t2.isEmpty(), ok2);
    setStatus(ui->word3Status, !t3.isEmpty(), ok3);
    if (ui->buttonBox && ui->stackedWidget->currentIndex() == 1) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok1 && ok2 && ok3);
    }
}

void MnemonicVerificationDialog::accept()
{
    // In view-only mode, skip verification
    if (!m_view_only) {
        if (!validateWord(ui->word1Edit->text().trimmed().toLower(), m_selected_positions[0]) ||
            !validateWord(ui->word2Edit->text().trimmed().toLower(), m_selected_positions[1]) ||
            !validateWord(ui->word3Edit->text().trimmed().toLower(), m_selected_positions[2])) {
            QMessageBox::warning(this, tr("Verification Failed"), tr("One or more words are incorrect. Please try again."));
            return;
        }
    }
    QDialog::accept();
}

std::vector<SecureString> MnemonicVerificationDialog::parseWords()
{
    // If words are already parsed, reuse them (for step 2 validation or step 1 display)
    if (!m_words.empty()) {
        return m_words;
    }

    // Parse words from secure mnemonic string
    QString mnemonicStr{QString::fromUtf8(m_mnemonic.data(), m_mnemonic.size())};
    QStringList wordList = mnemonicStr.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    // Convert to SecureString vector for secure storage
    m_words.clear();
    m_words.reserve(wordList.size());
    for (const QString& word : wordList) {
        QByteArray utf8 = word.toUtf8();
        SecureString secureWord;
        secureWord.assign(utf8.constData(), utf8.size());
        m_words.push_back(std::move(secureWord));
        utf8.fill(0);
    }

    // Clear the temporary QString immediately after parsing
    mnemonicStr.fill(QChar(0));
    mnemonicStr.clear();
    mnemonicStr.squeeze(); // Release memory
    wordList.clear();

    return m_words;
}

int MnemonicVerificationDialog::getWordCount() const
{
    // Count words without parsing them into vector
    // This avoids storing words in non-secure memory unnecessarily
    if (m_words.empty()) {
        QString mnemonicStr{QString::fromUtf8(m_mnemonic.data(), m_mnemonic.size())};
        QStringList words = mnemonicStr.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        int count = words.size();
        // Clear immediately
        mnemonicStr.clear();
        mnemonicStr.squeeze();
        words.clear();
        return count;
    }
    return m_words.size();
}

void MnemonicVerificationDialog::buildMnemonicGrid(bool reveal)
{
    if (!m_gridLayout) return;

    QLayoutItem* child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    // Parse words only when revealing (when needed for display)
    std::vector<SecureString> words;
    if (reveal) {
        words = parseWords();
    } else {
        // For hidden view, just get count without parsing words
        const int n = getWordCount();
        const int columns = (n >= 24) ? 4 : 3;
        const int rows = (n + columns - 1) / columns;

        // Font styling is defined in general.css
        m_gridLayout->setContentsMargins(6, 2, 6, 8);
        m_gridLayout->setHorizontalSpacing(32);
        m_gridLayout->setVerticalSpacing(7);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < columns; ++c) {
                int idx = r * columns + c; if (idx >= n) break;
                const QString text = QString("%1. •••••••").arg(idx + 1, 2);
                QLabel* lbl = new QLabel(text);
                lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
                m_gridLayout->addWidget(lbl, r, c);
            }
        }

        m_gridLayout->setRowMinimumHeight(rows, 12);
        return;
    }

    // Revealed view - words are already parsed
    const int n = words.size();
    const int columns = (n >= 24) ? 4 : 3;
    const int rows = (n + columns - 1) / columns;

    // Font styling is defined in general.css
    m_gridLayout->setContentsMargins(6, 2, 6, 8);
    m_gridLayout->setHorizontalSpacing(32);
    m_gridLayout->setVerticalSpacing(7);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            int idx = r * columns + c; if (idx >= n) break;
            // Convert SecureString to QString temporarily for display
            QString wordStr{QString::fromUtf8(words[idx].data(), words[idx].size())};
            const QString text = QString("%1. %2").arg(idx + 1, 2).arg(wordStr);
            QLabel* lbl = new QLabel(text);
            lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
            m_gridLayout->addWidget(lbl, r, c);
            // Clear temporary QString immediately after use
            wordStr.fill(QChar(0));
            wordStr.clear();
            wordStr.squeeze();
        }
    }

    m_gridLayout->setRowMinimumHeight(rows, 12);
}
