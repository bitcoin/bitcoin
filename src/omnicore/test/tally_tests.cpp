#include "omnicore/tally.h"

#include "test/test_bitcoin.h"

#include <stdint.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(omnicore_tally_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(empty_tally)
{
    CMPTally tally;
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, BALANCE));
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, SELLOFFER_RESERVE));
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, ACCEPT_RESERVE));
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, PENDING));
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, METADEX_RESERVE));
    // TallyType out of range:
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, static_cast<TallyType>(5)));
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, static_cast<TallyType>(6)));
    // TallyType out of range:
    BOOST_CHECK(!tally.updateMoney(0, 1, static_cast<TallyType>(5)));
    BOOST_CHECK(!tally.updateMoney(0, 1, static_cast<TallyType>(6)));

    BOOST_CHECK_EQUAL(0, tally.init());
    BOOST_CHECK_EQUAL(0, tally.next());

    BOOST_CHECK_EQUAL(0, tally.getMoneyAvailable(0));
    BOOST_CHECK_EQUAL(0, tally.getMoneyReserved(0));
    BOOST_CHECK_EQUAL(0, tally.getMoneyAvailable(55));
    BOOST_CHECK_EQUAL(0, tally.getMoneyReserved(55));
}

BOOST_AUTO_TEST_CASE(filled_tally)
{
    CMPTally tally;
    BOOST_CHECK(!tally.updateMoney(0, 0, BALANCE));
    BOOST_CHECK(!tally.updateMoney(0, 0, SELLOFFER_RESERVE));
    BOOST_CHECK(!tally.updateMoney(0, 0, ACCEPT_RESERVE));
    BOOST_CHECK(!tally.updateMoney(0, 0, PENDING));
    BOOST_CHECK(!tally.updateMoney(0, 0, METADEX_RESERVE));

    BOOST_CHECK_EQUAL(0, tally.getMoney(0, BALANCE));
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, SELLOFFER_RESERVE));
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, ACCEPT_RESERVE));
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, PENDING));
    BOOST_CHECK_EQUAL(0, tally.getMoney(0, METADEX_RESERVE));

    BOOST_CHECK_EQUAL(0, tally.getMoneyAvailable(0));
    BOOST_CHECK_EQUAL(0, tally.getMoneyReserved(0));

    BOOST_CHECK(tally.updateMoney(0, 1, BALANCE));
    BOOST_CHECK(tally.updateMoney(0, 100, SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(1, int64_t(9223372036854775807LL), ACCEPT_RESERVE));
    BOOST_CHECK(tally.updateMoney(2, (-int64_t(9223372036854775807LL)-1), PENDING));
    BOOST_CHECK(tally.updateMoney(5, int64_t(4294967296L), METADEX_RESERVE));

    BOOST_CHECK_EQUAL(tally.getMoney(0, BALANCE), 1);
    BOOST_CHECK_EQUAL(tally.getMoney(0, SELLOFFER_RESERVE), 100);
    BOOST_CHECK_EQUAL(tally.getMoney(0, ACCEPT_RESERVE), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(0, PENDING), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(0, METADEX_RESERVE), 0);

    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(0), 1);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(0), 100);
    
    BOOST_CHECK_EQUAL(tally.getMoney(1, BALANCE), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(1, SELLOFFER_RESERVE), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(1, ACCEPT_RESERVE), int64_t(9223372036854775807LL));
    BOOST_CHECK_EQUAL(tally.getMoney(1, PENDING), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(1, METADEX_RESERVE), 0);

    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(1), 0);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(1), int64_t(9223372036854775807LL));

    BOOST_CHECK_EQUAL(tally.getMoney(2, BALANCE), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(2, SELLOFFER_RESERVE), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(2, ACCEPT_RESERVE), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(2, PENDING), (-int64_t(9223372036854775807LL)-1));
    BOOST_CHECK_EQUAL(tally.getMoney(2, METADEX_RESERVE), 0);

    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(2), (-int64_t(9223372036854775807LL)-1));
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(2), 0);

    BOOST_CHECK_EQUAL(tally.getMoney(5, BALANCE), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(5, SELLOFFER_RESERVE), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(5, ACCEPT_RESERVE), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(5, PENDING), 0);
    BOOST_CHECK_EQUAL(tally.getMoney(5, METADEX_RESERVE), int64_t(4294967296L));

    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(5), 0);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(5), int64_t(4294967296L));

    /**
     * Note:
     * The internal iterator must be replaced via init(),
     * after inserting a new entry via updateMoney().
     */
    BOOST_CHECK_EQUAL(0, tally.init());
    BOOST_CHECK_EQUAL(0, tally.next());
    BOOST_CHECK_EQUAL(1, tally.next());
    BOOST_CHECK_EQUAL(2, tally.next());
    BOOST_CHECK_EQUAL(5, tally.next());
    BOOST_CHECK_EQUAL(0, tally.init());
    BOOST_CHECK_EQUAL(0, tally.next());
    BOOST_CHECK_EQUAL(1, tally.next());
}

BOOST_AUTO_TEST_CASE(tally_entry_order)
{
    CMPTally tally;

    BOOST_CHECK(tally.updateMoney(1, 1, BALANCE));
    BOOST_CHECK(tally.updateMoney(4, 1, SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(3, 1, ACCEPT_RESERVE));
    BOOST_CHECK(tally.updateMoney(9, 1, BALANCE));
    BOOST_CHECK(tally.updateMoney(2, -1, PENDING));
    BOOST_CHECK(tally.updateMoney(5, 1, METADEX_RESERVE));
    BOOST_CHECK(tally.updateMoney(9, 3, BALANCE));
    BOOST_CHECK(tally.updateMoney(9, -6, PENDING));
    BOOST_CHECK(tally.updateMoney(8, 1, SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(7, 1, ACCEPT_RESERVE));
    BOOST_CHECK(tally.updateMoney(6, 3, BALANCE));
    BOOST_CHECK(tally.updateMoney(9, 1, SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(9, 2, ACCEPT_RESERVE));
    BOOST_CHECK(tally.updateMoney(9, 3, METADEX_RESERVE));
    BOOST_CHECK(tally.updateMoney(9, 4, METADEX_RESERVE));
    BOOST_CHECK(tally.updateMoney(8, 1, BALANCE));
    BOOST_CHECK(tally.updateMoney(70, 1, BALANCE));
    BOOST_CHECK(tally.updateMoney(4, 1, SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(5, 1, BALANCE));
    BOOST_CHECK(tally.updateMoney(1, 1, BALANCE));
    BOOST_CHECK(tally.updateMoney(6, -2, PENDING));
    BOOST_CHECK(tally.updateMoney(3, 1, ACCEPT_RESERVE));
    BOOST_CHECK(tally.updateMoney(2, 1, METADEX_RESERVE));
    BOOST_CHECK(tally.updateMoney(4, -1, PENDING));
    BOOST_CHECK(tally.updateMoney(2, -1, PENDING));

    BOOST_CHECK_EQUAL(1, tally.init());
    // Begin iterations:
    BOOST_CHECK_EQUAL(1, tally.next());
    BOOST_CHECK_EQUAL(2, tally.next());
    BOOST_CHECK_EQUAL(3, tally.next());
    BOOST_CHECK_EQUAL(4, tally.next());
    BOOST_CHECK_EQUAL(5, tally.next());
    BOOST_CHECK_EQUAL(6, tally.next());
    BOOST_CHECK_EQUAL(7, tally.next());
    BOOST_CHECK_EQUAL(8, tally.next());
    BOOST_CHECK_EQUAL(9, tally.next());
    BOOST_CHECK_EQUAL(70, tally.next());
    // End of tally reached:
    BOOST_CHECK_EQUAL(0, tally.next());

    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(1), 2);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(1), 0);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(2), -2);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(2), 1);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(3), 0);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(3), 2);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(4), -1);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(4), 2);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(5), 1);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(5), 1);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(6), 1);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(6), 0);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(7), 0);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(7), 1);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(8), 1);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(8), 1);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(9), -2);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(9), 10);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(70), 1);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(70), 0);
}

BOOST_AUTO_TEST_CASE(tally_equality)
{
    CMPTally tally1;
    CMPTally tally2;

    BOOST_CHECK(tally1 == tally2);
    BOOST_CHECK(tally2 == tally1);

    BOOST_CHECK(tally1.updateMoney(4, 5, ACCEPT_RESERVE));
    BOOST_CHECK(tally1.updateMoney(3, 3, METADEX_RESERVE));
    BOOST_CHECK(tally1.updateMoney(3, 7, SELLOFFER_RESERVE));
    BOOST_CHECK(tally1.updateMoney(1, 5, BALANCE));
    BOOST_CHECK(tally1.updateMoney(9, 4, BALANCE));
    BOOST_CHECK(tally1.updateMoney(1, 50, BALANCE));
    BOOST_CHECK(tally1.updateMoney(1, -3, BALANCE));
    BOOST_CHECK(tally1.updateMoney(1, -3, PENDING));

    BOOST_CHECK(tally2.updateMoney(3, 4, SELLOFFER_RESERVE));
    BOOST_CHECK(tally2.updateMoney(1, 20, BALANCE));
    BOOST_CHECK(tally2.updateMoney(4, 5, ACCEPT_RESERVE));
    BOOST_CHECK(tally2.updateMoney(9, 4, BALANCE));
    BOOST_CHECK(tally2.updateMoney(3, 3, SELLOFFER_RESERVE));
    BOOST_CHECK(tally2.updateMoney(1, 5, BALANCE));
    BOOST_CHECK(tally2.updateMoney(3, 3, METADEX_RESERVE));
    BOOST_CHECK(tally2.updateMoney(1, 5, BALANCE));
    BOOST_CHECK(tally2.updateMoney(1, 28, BALANCE));
    BOOST_CHECK(tally2.updateMoney(1, -6, BALANCE));
    BOOST_CHECK(tally2.updateMoney(1, -3, PENDING));

    BOOST_CHECK(tally1 == tally2);
    BOOST_CHECK(tally2 == tally1);

    BOOST_CHECK(tally1.getMoneyAvailable(1) == tally2.getMoneyAvailable(1));
    BOOST_CHECK(tally1.getMoneyAvailable(3) == tally2.getMoneyAvailable(3));
    BOOST_CHECK(tally1.getMoneyAvailable(4) == tally2.getMoneyAvailable(4));
    BOOST_CHECK(tally1.getMoneyAvailable(9) == tally2.getMoneyAvailable(9));
    BOOST_CHECK(tally1.getMoneyAvailable(0) == tally2.getMoneyAvailable(0));

    BOOST_CHECK(tally2.getMoneyReserved(1) == tally1.getMoneyReserved(1));
    BOOST_CHECK(tally2.getMoneyReserved(3) == tally1.getMoneyReserved(3));
    BOOST_CHECK(tally2.getMoneyReserved(4) == tally1.getMoneyReserved(4));
    BOOST_CHECK(tally2.getMoneyReserved(9) == tally1.getMoneyReserved(9));
    BOOST_CHECK(tally2.getMoneyReserved(0) == tally1.getMoneyReserved(0));

    BOOST_CHECK_EQUAL(1, tally1.init());
    BOOST_CHECK_EQUAL(1, tally1.next());
    BOOST_CHECK_EQUAL(3, tally1.next());
    BOOST_CHECK_EQUAL(4, tally1.next());
    BOOST_CHECK_EQUAL(9, tally1.next());
    BOOST_CHECK_EQUAL(0, tally1.next());
    BOOST_CHECK_EQUAL(1, tally2.init());
    BOOST_CHECK_EQUAL(1, tally2.next());
    BOOST_CHECK_EQUAL(3, tally2.next());
    BOOST_CHECK_EQUAL(4, tally2.next());

    BOOST_CHECK(tally1 == tally2);

    BOOST_CHECK(tally1.updateMoney(9, -2, BALANCE));
    BOOST_CHECK(tally1.updateMoney(9, -2, BALANCE));
    BOOST_CHECK(tally2.updateMoney(9, -4, BALANCE));
    BOOST_CHECK(tally1 == tally2);
    BOOST_CHECK(tally1.getMoneyAvailable(9) == 0);

    BOOST_CHECK(tally1.updateMoney(8, 3, METADEX_RESERVE));
    BOOST_CHECK(tally1 != tally2);
    BOOST_CHECK(tally1.getMoneyReserved(8) == 3);

    BOOST_CHECK(tally2.updateMoney(8, 4, METADEX_RESERVE));
    BOOST_CHECK(tally1 != tally2);
    BOOST_CHECK(tally2.getMoneyReserved(8) == 4);

    BOOST_CHECK(tally2.updateMoney(8, -1, METADEX_RESERVE));
    BOOST_CHECK(tally1 == tally2);
    BOOST_CHECK(tally1.getMoneyReserved(8) == 3);

    BOOST_CHECK(tally1.updateMoney(7, 1, BALANCE));
    BOOST_CHECK(tally2.updateMoney(5, 1, BALANCE));
    BOOST_CHECK(tally1 != tally2);

    BOOST_CHECK(tally1.updateMoney(5, 1, BALANCE));
    BOOST_CHECK(tally2.updateMoney(7, 1, BALANCE));
    BOOST_CHECK(tally1 == tally2);

    BOOST_CHECK(tally1.getMoneyAvailable(5) == 1);
    BOOST_CHECK(tally1.getMoneyAvailable(7) == 1);
    BOOST_CHECK(tally2.getMoneyAvailable(5) == 1);
    BOOST_CHECK(tally2.getMoneyAvailable(7) == 1);
}

BOOST_AUTO_TEST_CASE(tally_overflow)
{
    CMPTally tally;

    BOOST_CHECK(!tally.updateMoney(1, -1, BALANCE));
    BOOST_CHECK(!tally.updateMoney(2, -34242, SELLOFFER_RESERVE));
    BOOST_CHECK(!tally.updateMoney(2, (-int64_t(9223372036854775807LL)-1), ACCEPT_RESERVE));
    BOOST_CHECK(!tally.updateMoney(3, -int64_t(1099511627777L), METADEX_RESERVE));

    BOOST_CHECK(tally.updateMoney(1, int64_t(9223372036854775807LL), BALANCE));
    BOOST_CHECK_EQUAL(tally.getMoney(1, BALANCE), int64_t(9223372036854775807LL));
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(1), int64_t(9223372036854775807LL));

    BOOST_CHECK(!tally.updateMoney(1, 1, BALANCE));
    BOOST_CHECK_EQUAL(tally.getMoney(1, BALANCE), int64_t(9223372036854775807LL));
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(1), int64_t(9223372036854775807LL));

    BOOST_CHECK(tally.updateMoney(1, (-int64_t(9223372036854775807LL)-1), PENDING));
    BOOST_CHECK_EQUAL(tally.getMoney(1, BALANCE), int64_t(9223372036854775807LL));
    BOOST_CHECK_EQUAL(tally.getMoney(1, PENDING), (-int64_t(9223372036854775807LL)-1));
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(1), -1);

    BOOST_CHECK(!tally.updateMoney(1, -1, PENDING));
    BOOST_CHECK_EQUAL(tally.getMoney(1, BALANCE), int64_t(9223372036854775807LL));
    BOOST_CHECK_EQUAL(tally.getMoney(1, PENDING), (-int64_t(9223372036854775807LL)-1));
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(1), -1);

    BOOST_CHECK(tally.updateMoney(1, int64_t(9223372036854775807LL), ACCEPT_RESERVE));
    BOOST_CHECK(tally.updateMoney(2, int64_t(9223372036854775807LL), SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(3, int64_t(9223372036854775807LL), SELLOFFER_RESERVE));
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(1), int64_t(9223372036854775807LL));
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(2), int64_t(9223372036854775807LL));
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(3), int64_t(9223372036854775807LL));

    BOOST_CHECK(!tally.updateMoney(3, int64_t(9223372036854775807LL), SELLOFFER_RESERVE));
    BOOST_CHECK(!tally.updateMoney(3, (-int64_t(9223372036854775807LL)-1), SELLOFFER_RESERVE));
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(3), int64_t(9223372036854775807LL));

    BOOST_CHECK(tally.updateMoney(1, -int64_t(9223372036854775807), ACCEPT_RESERVE));
    BOOST_CHECK(tally.updateMoney(1, 10000001, ACCEPT_RESERVE));
    BOOST_CHECK(tally.updateMoney(1, 20000001, SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(1, 30000001, SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(1, 40000001, SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(1, 50000001, METADEX_RESERVE));
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(1), 150000005);

    BOOST_CHECK(!tally.updateMoney(1, 1, BALANCE));
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(1), -1);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(2), 0);
    BOOST_CHECK_EQUAL(tally.getMoneyAvailable(3), 0);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(1), 150000005);
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(2), int64_t(9223372036854775807LL));
    BOOST_CHECK_EQUAL(tally.getMoneyReserved(3), int64_t(9223372036854775807LL));
}


BOOST_AUTO_TEST_SUITE_END()
