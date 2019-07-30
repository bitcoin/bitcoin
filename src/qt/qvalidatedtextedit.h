#ifndef QVALIDATEDTEXTEDIT_H
#define QVALIDATEDTEXTEDIT_H

#include <QPlainTextEdit>

/** Text edit that can be marked as "invalid" to show input validation feedback. When marked as invalid,
   it will get a red background until it is focused.
 */
class QValidatedTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit QValidatedTextEdit(QWidget *parent = 0);
    void clear();

protected:
    void focusInEvent(QFocusEvent *evt);

private:
    bool valid;
    QString errorText;

public Q_SLOTS:
    void setValid(bool valid);
    void setErrorText(QString errorText);

private Q_SLOTS:
    void markValid();
};

#endif // QVALIDATEDTEXTEDIT_H
