// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "strong_ptr.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

class my_struct
{
public:
    my_struct(int){}
    my_struct() = default;
    ~my_struct()
    {
        m_valid = false;
    }
    bool valid() const
    {
        return m_valid;
    }
private:
    bool m_valid = true;
};

class Deleter
{
public:
    explicit Deleter(bool& deleted) : m_deleted{deleted}{}

    void operator()(std::nullptr_t)
    {
        m_deleted = true;
    }
    template <typename T>
    void operator()(T* ptr)
    {
        delete ptr;
        m_deleted = true;
    }
private:
    bool& m_deleted;
};

BOOST_FIXTURE_TEST_SUITE(strong_ptr_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(construction)
{
    // typical construction
    strong_ptr<my_struct> strong(new my_struct());
    BOOST_CHECK(strong);
    BOOST_CHECK(strong->valid());
    decay_ptr<my_struct> degraded(std::move(strong));
    BOOST_CHECK(!strong);
    BOOST_CHECK(degraded);
    BOOST_CHECK(degraded.decayed());
    BOOST_CHECK(degraded->valid());
}

BOOST_AUTO_TEST_CASE(deletion)
{
    // test typical deletion
    {
        bool deleted = false;
        {
            strong_ptr<my_struct> strong(new my_struct(), Deleter(deleted));
        }
        BOOST_CHECK(deleted);
    }
    {
        bool deleted = false;
        strong_ptr<my_struct> strong(new my_struct(), Deleter(deleted));
        strong.reset();
        BOOST_CHECK(deleted);
    }
    {
        bool deleted = false;
        strong_ptr<my_struct> strong(new my_struct(), Deleter(deleted));
        decay_ptr<my_struct> degraded(std::move(strong));
        BOOST_CHECK(!deleted);
        degraded.reset();
        BOOST_CHECK(deleted);
    }
    {
        bool deleted = false;
        strong_ptr<my_struct> strong(new my_struct(), Deleter(deleted));
        {
            auto shared = strong.get_shared();
        }
        decay_ptr<my_struct> degraded(std::move(strong));
        BOOST_CHECK(!deleted);
        degraded.reset();
        BOOST_CHECK(deleted);
    }
}

BOOST_AUTO_TEST_CASE(shared_outlives_strong)
{
    {
        bool deleted = false;
        strong_ptr<my_struct> strong(new my_struct(), Deleter(deleted));
        auto shared = strong.get_shared();
        {
            decay_ptr<my_struct> degraded(std::move(strong));
            BOOST_CHECK(!degraded.decayed());
        }
        BOOST_CHECK(!deleted);
        shared.reset();
        BOOST_CHECK(deleted);
    }
}

BOOST_AUTO_TEST_CASE(use_count)
{
        strong_ptr<my_struct> strong(new my_struct());
        auto shared = strong.get_shared();
        BOOST_CHECK(shared.use_count() == 2);
        auto strong2(std::move(strong));
        BOOST_CHECK(shared.use_count() == 2);
        BOOST_CHECK(strong2);
        BOOST_CHECK(!strong);
        strong = std::move(strong2);
        BOOST_CHECK(shared.use_count() == 2);
        BOOST_CHECK(strong);
        BOOST_CHECK(!strong2);
        decay_ptr<my_struct> degraded(std::move(strong));
        BOOST_CHECK(shared.use_count() == 1);
        BOOST_CHECK(!strong);
        BOOST_CHECK(!degraded.decayed());
        shared.reset();
        BOOST_CHECK(degraded.decayed());
}

BOOST_AUTO_TEST_SUITE_END()
