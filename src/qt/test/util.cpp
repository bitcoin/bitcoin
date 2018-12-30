#include <qt/callback.h>

#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QString>
#include <QPushButton>
#include <QWidget>

void ConfirmMessage(QString* text, int msec)
{
    QTimer::singleShot(msec, makeCallback([text](Callback* callback) {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (widget->inherits("QMessageBox")) {
                QMessageBox* messageBox = qobject_cast<QMessageBox*>(widget);
                if (text) *text = messageBox->text();
                messageBox->defaultButton()->click();
            }
        }
        delete callback;
    }), SLOT(call()));
}
