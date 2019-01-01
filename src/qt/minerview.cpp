// Copyright (c) 2019 The BitcoinV Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/minerview.h>

#include <qt/addresstablemodel.h>
#include <qt/bitcoinunits.h>
#include <qt/csvmodelwriter.h>
#include <qt/editaddressdialog.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/sendcoinsdialog.h>
#include <qt/transactiondescdialog.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactionrecord.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>

#include <ui_interface.h>

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QSignalMapper>
#include <QTableView>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QTextEdit>

#include <rpc/server.h>
#include <rpc/client.h>
#include <univalue/include/univalue.h>

#include <pthread.h>
#include <stdio.h>
#include <atomic>
#include <thread>
#include <unistd.h>

MinerView::MinerView(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent), m_minerView(0), m_mining_active{false},
    m_mining_type{STANDARD}
{
    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);

    if (platformStyle->getUseExtraSpacing()) {
        hlayout->setSpacing(5);
        hlayout->addSpacing(26);
    } else {
        hlayout->setSpacing(0);
        hlayout->addSpacing(23);
    }


    m_mine_type = new QComboBox(this);
    if (platformStyle->getUseExtraSpacing()) {
        m_mine_type->setFixedWidth(251);
    } else {
        m_mine_type->setFixedWidth(250);
    }
    m_mine_type->addItem(tr("Mine Standard Block Rewards"), STANDARD);
    m_mine_type->addItem(tr("Mine Random Block Rewards"), RANDOM_BLOCK_REWARD);
    m_mine_type->addItem(tr("Mine JACKPOT Block Rewards"), JACKPOT_BLOCK_REWARD);
    hlayout->addWidget(m_mine_type);



    // Delay before filtering transactions in ms
    static const int input_filter_delay = 200;


    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    //QTableView *view = new QTableView(this);
    m_minerView = new QTextEdit();


    vlayout->addLayout(hlayout);
    vlayout->addWidget(m_minerView);
    vlayout->setSpacing(0);



    m_minerView->installEventFilter(this);


    m_minerView->setObjectName("minerView");

    // Create the button, make "this" the parent
    m_button = new QPushButton("START", this);
    // set size and location of the button
    m_button->setGeometry(QRect(QPoint(100, 100), QSize(200, 50)));
    m_button->setFixedWidth(150);

    hlayout->addWidget(m_button);
 
    // Connect button signal to appropriate slot
    connect(m_button, SIGNAL (released()), this, SLOT (handleButton()));

    connect(m_mine_type, SIGNAL (activated(int)), this, SLOT (handleMineType(int)));

}


std::atomic<bool> stop_mining(false);

void* mining_thread_gui(void* params);
void* mining_thread_gui(void* params)
{
    MinerView::Mining_Type_t mining_type =  *((MinerView::Mining_Type_t*)params);


    std::vector<std::string> strParams;
    bool is_random_enabled = false;

    if ( MinerView::RANDOM_BLOCK_REWARD == mining_type )
    {
        strParams.push_back(std::string("1"));
        is_random_enabled = true;
    }
    else if ( MinerView::JACKPOT_BLOCK_REWARD == mining_type )
    {
        strParams.push_back(std::string("1"));
        strParams.push_back(std::string("10000000"));
        uint32_t million = 1<<20;
        
        strParams.push_back(std::string(std::to_string(million)));
    }
    else
    {
        // default to standard
        strParams.push_back(std::string("1"));
        strParams.push_back(std::string("10000000"));
    }

    UniValue par = RPCConvertValues(std::string("generate"), strParams);

    stop_mining = false;
    JSONRPCRequest req;
    req.params = par;
    req.strMethod = "generate";
    req.URI = "/wallet/";

    uint32_t n = 0;

    while ( true ) {

        // RPC call to do the mining
        ::tableRPC.execute(req);

        std::cout << "running..." << std::endl;


        if (true == stop_mining) {
            break;
        }

        if (is_random_enabled)
        {
            std::vector<std::string> strParams;


            strParams.push_back(std::string("1"));
            strParams.push_back(std::string("10000000"));
            strParams.push_back(std::to_string(1<<n));

            UniValue par = RPCConvertValues(std::string("generate"), strParams);

            req.params = par;
            req.strMethod = "generate";
            req.URI = "/wallet/";

            // don't go more than a million times the block reward, the blockchain won't accept it if you do.
            n = (n+1)%20;
        }

    }


    pthread_exit((void*)(nullptr));

}

void start_mining( JSONRPCRequest* mining_params )
{
    void* status = 0;

    pthread_t tids[1];
    std::cout << "Mining started " << std::endl;

    pthread_create(&tids[0], NULL, mining_thread_gui, (void *)mining_params);

    return;
}


 void MinerView::handleButton()
 {
     
     if (m_mining_active)
     {
        // stop mining
        stop_mining = true;

        // now the user needs to see the START button
        m_button->setText("START");
     }
     else
     {
         // start mining
        if ( RANDOM_BLOCK_REWARD == m_mining_type )
        {
            m_minerView->setText("Mining for a RANDOM BLOCK REWARD");
            m_minerView->show();
        }
        else if ( JACKPOT_BLOCK_REWARD == m_mining_type )
        {
            m_minerView->setText("Mining for the JACKPOT BLOCK REWARD");
            m_minerView->show();
        }
        else
        {
            // default to standard
            m_mining_type = STANDARD;
            m_minerView->setText("Mining for the STANDARD BLOCK REWARD");
            m_minerView->show();
        }

        stop_mining = false;

        // now the user needs to see the STOP button
        m_button->setText("STOP");


        //// Start the thread to do the mining.
        pthread_t tids[1];
        pthread_create(&tids[0], NULL, mining_thread_gui, (void *)&m_mining_type);


     }

     m_mining_active = !m_mining_active;
     
 }

void MinerView::handleMineType(int type)
{
    if (type >=0 && type <= 2)
    {
        m_mining_type = (Mining_Type_t)type;
    }

    return;
}
