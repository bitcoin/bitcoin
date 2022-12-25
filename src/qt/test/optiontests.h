// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_QT_TEST_OPTIONTESTS_H
#define SYSCOIN_QT_TEST_OPTIONTESTS_H

#include <qt/optionsmodel.h>
#include <univalue.h>
#include <util/settings.h>

#include <QObject>

class OptionTests : public QObject
{
    Q_OBJECT
public:
    explicit OptionTests(interfaces::Node& node);

private Q_SLOTS:
    void init(); // called before each test function execution.
    void migrateSettings();
    void integerGetArgBug();
    void parametersInteraction();
    void extractFilter();

private:
    interfaces::Node& m_node;
    util::Settings m_previous_settings;
};

#endif // SYSCOIN_QT_TEST_OPTIONTESTS_H
