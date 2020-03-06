#include <boost/test/unit_test.hpp>
#include <gmock/gmock.h>

struct Interface {
  virtual ~Interface() = default;
  virtual void foo() = 0;
};

struct Mock : public Interface {
  ~Mock() override = default;
  MOCK_METHOD0(foo, void());
};

BOOST_AUTO_TEST_SUITE(gmock_tests)

BOOST_AUTO_TEST_CASE(basic_test)
{
  Mock mock;
  EXPECT_CALL(mock, foo()).Times(1);
  mock.foo();
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()