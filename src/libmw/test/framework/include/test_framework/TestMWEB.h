#pragma once

#include <boost/test/unit_test.hpp>
#include <mweb/mweb_db.h>
#include <test/util/setup_common.h>

struct MWEBTestingSetup : public BasicTestingSetup {
    explicit MWEBTestingSetup()
        : BasicTestingSetup(CBaseChainParams::MAIN)
    {
        m_db = std::make_unique<CDBWrapper>(GetDataDir() / "db", 1 << 15);
        m_mweb_db = std::make_shared<MWEB::DBWrapper>(m_db.get());
    }

    virtual ~MWEBTestingSetup() = default;

    mw::DBWrapper::Ptr GetDB() { return m_mweb_db; }

private:
    std::unique_ptr<CDBWrapper> m_db;
    std::shared_ptr<mw::DBWrapper> m_mweb_db;
};