// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/qvalidatedlineedit.h>

#include <qt/bitcoinaddressvalidator.h>
#include <qt/guiconstants.h>

#include <QColor>
#include <QCoreApplication>
#include <QFont>
#include <QInputMethodEvent>
#include <QList>
#include <QTextCharFormat>

QValidatedLineEdit::QValidatedLineEdit(QWidget* parent)
    : QLineEdit(parent)
{
    connect(this, &QValidatedLineEdit::textChanged, this, &QValidatedLineEdit::markValid);
}

QValidatedLineEdit::~QValidatedLineEdit()
{
    delete m_warning_validator;
}

void QValidatedLineEdit::setText(const QString& text)
{
    QLineEdit::setText(text);
    checkValidity();
}

void QValidatedLineEdit::setValid(bool _valid, bool with_warning, const std::vector<int>&error_locations)
{
    if(_valid && this->valid)
    {
        if (with_warning == m_has_warning) {
            return;
        }
    }

    QList<QInputMethodEvent::Attribute> attributes;

    if(_valid)
    {
        m_has_warning = with_warning;
        if (with_warning) {
            setStyleSheet("QValidatedLineEdit { " STYLE_INCORRECT "}");
        } else {
            setStyleSheet("");
        }
    }
    else
    {
        setStyleSheet("QValidatedLineEdit { " STYLE_INVALID "}");
        if (!error_locations.empty()) {
            QTextCharFormat format;
            format.setFontUnderline(true);
            format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
            format.setUnderlineColor(Qt::yellow);
            format.setForeground(Qt::yellow);
            format.setFontWeight(QFont::Bold);
            for (auto error_pos : error_locations) {
                attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, error_pos - cursorPosition(), /*length=*/ 1, format));
            }
        }
    }

    QInputMethodEvent event(QString(), attributes);
    QCoreApplication::sendEvent(this, &event);

    this->valid = _valid;
}

void QValidatedLineEdit::focusInEvent(QFocusEvent *evt)
{
    // Clear invalid flag on focus
    setValid(true);

    QLineEdit::focusInEvent(evt);
}

void QValidatedLineEdit::focusOutEvent(QFocusEvent *evt)
{
    checkValidity();

    QLineEdit::focusOutEvent(evt);
}

void QValidatedLineEdit::markValid()
{
    // As long as a user is typing ensure we display state as valid
    setValid(true);
}

void QValidatedLineEdit::clear()
{
    setValid(true);
    QLineEdit::clear();
}

void QValidatedLineEdit::setEnabled(bool enabled)
{
    if (!enabled)
    {
        // A disabled QValidatedLineEdit should be marked valid
        setValid(true);
    }
    else
    {
        // Recheck validity when QValidatedLineEdit gets enabled
        checkValidity();
    }

    QLineEdit::setEnabled(enabled);
}

void QValidatedLineEdit::checkValidity()
{
    const bool has_warning = checkWarning();
    if (text().isEmpty())
    {
        setValid(true);
    }
    else if (hasAcceptableInput())
    {
        setValid(true, has_warning);

        // Check contents on focus out
        if (checkValidator)
        {
            QString address = text();
            QValidator::State validation_result;
            std::vector<int> error_locations;
            const BitcoinAddressEntryValidator * const address_validator = dynamic_cast<const BitcoinAddressEntryValidator*>(checkValidator);
            if (address_validator) {
                validation_result = address_validator->validate(address, error_locations);
            } else {
                int pos = 0;
                validation_result = checkValidator->validate(address, pos);
                error_locations.push_back(pos);
            }
            if (validation_result == QValidator::Acceptable) {
                setValid(/* valid= */ true, has_warning);
            } else {
                setValid(/* valid= */ false, /* with_warning= */ false, error_locations);
            }
        }
    }
    else
        setValid(false);

    Q_EMIT validationDidChange(this);
}

void QValidatedLineEdit::setCheckValidator(const QValidator *v)
{
    checkValidator = v;
    checkValidity();
}

bool QValidatedLineEdit::isValid()
{
    // use checkValidator in case the QValidatedLineEdit is disabled
    if (checkValidator)
    {
        QString address = text();
        int pos = 0;
        if (checkValidator->validate(address, pos) == QValidator::Acceptable)
            return true;
    }

    return valid;
}

void QValidatedLineEdit::setWarningValidator(const QValidator *v)
{
    delete m_warning_validator;
    m_warning_validator = v;
    checkValidity();
}

bool QValidatedLineEdit::checkWarning() const
{
    if (m_warning_validator && !text().isEmpty()) {
        QString address = text();
        int pos = 0;
        if (m_warning_validator->validate(address, pos) != QValidator::Acceptable) {
            return true;
        }
    }

    return false;
}

bool QValidatedLineEdit::hasWarning() const
{
    return m_has_warning;
}
