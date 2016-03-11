// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stat.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(stat_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(stat_testvectors)
{
  //const int numMetrics = 5;
  CStatHistory<int> s1("s1");
  BOOST_CHECK(s1() == 0);
  CStatHistory<unsigned int> s2("s2",STAT_OP_AVE);
  BOOST_CHECK(s2() == 0);
  CStatHistory<uint64_t> s3("s3",STAT_OP_MAX);
  BOOST_CHECK(s3() == 0);
  CStatHistory<double> s4("s4",STAT_OP_MIN);
  BOOST_CHECK(s4() == 0.0);

  CStatHistory<float, MinValMax<float> > s5("s5");

  s5 += 4.5f;
  BOOST_CHECK(s5().min == 4.5f);
  BOOST_CHECK(s5().max == 4.5f);
  BOOST_CHECK(s5().val == 4.5f);
  s5 << 1.5f;
  BOOST_CHECK(s5().min == 4.5f);
  BOOST_CHECK(s5().max == 6.0f);
  BOOST_CHECK(s5().val == 6.0f);

  // check the + over various data types
  s1 += 5;
  BOOST_CHECK(s1() == 5);
  s2 += 10;
  BOOST_CHECK(s2() == 10);
  s3 += 10000;
  BOOST_CHECK(s3() == 10000);
  s4 += 3.3;
  BOOST_CHECK(s4() == 3.3);

  s1 += 15;
  BOOST_CHECK(s1() == 20);
  s2 += 110;
  BOOST_CHECK(s2() == 120);
  s3 += 110000;
  BOOST_CHECK(s3() == 120000);
  s4 += 4.3;
  BOOST_CHECK(s4() == 7.6);

  //statMinInterval=boost::posix_time::milliseconds(50); // boost::posix_time::seconds(1); // Speed things up
  //for (int i=0;i<numMetrics;i++)  doesn't work because there are other global variable stats
  //  stat_io_service.run_one();
  //stat_io_service.poll();
    s1.timeout(boost::system::error_code());
    s2.timeout(boost::system::error_code());
    s3.timeout(boost::system::error_code());
    s4.timeout(boost::system::error_code());
    s5.timeout(boost::system::error_code());

  BOOST_CHECK(s1() == 0);
  for (int i=0;i<30;i++)
    {
    s1 += 5;
    s2 += 10;
    s3 += 10000;
    s4 += 3.3;
    s5 << 2.4;
#if 0  // timing does not trigger accurately enough
    MilliSleep(50);
    for (int j=0;j<numMetrics;j++)
      stat_io_service.run_one();
#endif
    s1.timeout(boost::system::error_code());
    s2.timeout(boost::system::error_code());
    s3.timeout(boost::system::error_code());
    s4.timeout(boost::system::error_code());
    s5.timeout(boost::system::error_code());
    }
  BOOST_CHECK(s1.History(1,0) == 29*5 + 20);  
  BOOST_CHECK(s2.History(1,0) == (29*10 + 120)/30);  
  BOOST_CHECK(s3.History(1,0) == 120000);  
  BOOST_CHECK(s4.History(1,0) == 3.3); 

  statMinInterval=boost::posix_time::milliseconds(10); // boost::posix_time::seconds(1); // Speed things up
  for (int i=0;i<12;i++)
    for (int j=0;j<30;j++)
    {
    s1 += 5;
    s2 += 10;
    s3 += 10000;
    s4 += 3.3;
    s5 << 2.4;
#if 0
    MilliSleep(10);
    for (int k=0;k<numMetrics;k++)
      stat_io_service.run_one();
#endif
    s1.timeout(boost::system::error_code());
    s2.timeout(boost::system::error_code());
    s3.timeout(boost::system::error_code());
    s4.timeout(boost::system::error_code());
    s5.timeout(boost::system::error_code());
    }
  BOOST_CHECK(s1.History(2,0) > 0);  
  BOOST_CHECK(s2.History(2,0) > 0);  
  BOOST_CHECK(s3.History(2,0) == 120000);  
  BOOST_CHECK(s4.History(2,0) == 3.3); 

  int array[15];
  
  BOOST_CHECK(s1.Series(0, array, 15) == 15);
  for (int i=0;i<15;i++)
    BOOST_CHECK(array[i] == 5);
}

BOOST_AUTO_TEST_SUITE_END()
