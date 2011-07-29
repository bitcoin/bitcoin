#ifndef QVALUECOMBOBOX_H
#define QVALUECOMBOBOX_H

#include <QComboBox>

// QComboBox that can be used with QDataWidgetMapper to select
// ordinal values from a model.
class QValueComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged USER true);
public:
    explicit QValueComboBox(QWidget *parent = 0);

    int value() const;
    void setValue(int value);

    // Model role to use as value
    void setRole(int role);

signals:
    void valueChanged();

public slots:

private:
    int role;

private slots:
    void handleSelectionChanged(int idx);
};

#endif // QVALUECOMBOBOX_H
