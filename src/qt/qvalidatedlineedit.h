#ifndef QVALIDATEDLINEEDIT_H
#define QVALIDATEDLINEEDIT_H

#include <QLineEdit>

/** Line edit that can be marked as "invalid" to show input validation feedback. When marked as invalid,
   it will get a red background until it is focused.
 */
class QValidatedLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit QValidatedLineEdit(QWidget *parent = 0);
    void clear();

protected:
    void focusInEvent(QFocusEvent *evt);

private:
    bool valid;

public slots:
    void setValid(bool valid);

private slots:
    void markValid();
};

#endif // QVALIDATEDLINEEDIT_H
