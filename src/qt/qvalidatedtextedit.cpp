#include "qvalidatedtextedit.h"

#include "guiconstants.h"

#include <QMessageBox>
QValidatedTextEdit::QValidatedTextEdit(QWidget *parent) :
    QPlainTextEdit(parent), valid(true)
{}

void QValidatedTextEdit::setValid(bool valid)
{
    setStyleSheet(valid ? "" : STYLE_INVALID);

    if(valid)
    {
        if(toPlainText() == this->errorText)
            setPlainText("");
    }
    else if(toPlainText() == "")
        setPlainText(this->errorText);
}

void QValidatedTextEdit::setErrorText(QString errorText)
{
    this->errorText = errorText;
}

void QValidatedTextEdit::focusInEvent(QFocusEvent *evt)
{
    // Clear invalid flag on focus
    setValid(true);
    QPlainTextEdit::focusInEvent(evt);
}

void QValidatedTextEdit::markValid()
{
    setValid(true);
}

void QValidatedTextEdit::clear()
{
    setValid(true);
    QPlainTextEdit::clear();
}
