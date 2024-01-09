// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/qvaluecombobox.h>

QValueComboBox::QValueComboBox(QWidget* parent)
    : QComboBox(parent)
{
    connect(this, qOverload<int>(&QComboBox::currentIndexChanged), this, &QValueComboBox::handleSelectionChanged);
}

QVariant QValueComboBox::value() const
{
    return itemData(currentIndex(), role);
}

void QValueComboBox::setValue(const QVariant &value)
{
    setCurrentIndex(findData(value, role));
}

void QValueComboBox::setRole(int _role)
{
    this->role = _role;
}

void QValueComboBox::handleSelectionChanged(int idx)
{
    Q_EMIT valueChanged();
}
