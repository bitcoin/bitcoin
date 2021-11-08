// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_QVALIDATEDLINEEDIT_H
#define BITCOIN_QT_QVALIDATEDLINEEDIT_H

#include <QLineEdit>

/** Line edit that can be marked as "invalid" to show input validation feedback. When marked as invalid,
   it will get a red background until it is focused.
 */
class QValidatedLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit QValidatedLineEdit(QWidget *parent);
    ~QValidatedLineEdit();
    void clear();
    void setCheckValidator(const QValidator *v);
    bool isValid();
    void setWarningValidator(const QValidator *);
    bool hasWarning() const;

protected:
    void focusInEvent(QFocusEvent *evt) override;
    void focusOutEvent(QFocusEvent *evt) override;

private:
    bool valid;
    const QValidator *checkValidator;
    bool m_has_warning{false};
    const QValidator *m_warning_validator{nullptr};

public Q_SLOTS:
    void setText(const QString&);
    void setValid(bool valid, bool with_warning=false, const std::vector<int>&error_locations=std::vector<int>());
    void setEnabled(bool enabled);

Q_SIGNALS:
    void validationDidChange(QValidatedLineEdit *validatedLineEdit);

private Q_SLOTS:
    void markValid();
    void checkValidity();
    bool checkWarning() const;
};

#endif // BITCOIN_QT_QVALIDATEDLINEEDIT_H
