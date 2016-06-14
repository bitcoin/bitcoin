#ifndef ADDEDITADRENALINENODE_H
#define ADDEDITADRENALINENODE_H

#include <QDialog>

namespace Ui {
class AddEditAdrenalineNode;
}


class AddEditAdrenalineNode : public QDialog
{
    Q_OBJECT

public:
    explicit AddEditAdrenalineNode(QWidget *parent = 0);
    ~AddEditAdrenalineNode();

protected:

private slots:
    void on_okButton_clicked();
    void on_cancelButton_clicked();

signals:

private:
    Ui::AddEditAdrenalineNode *ui;
};

#endif // ADDEDITADRENALINENODE_H
