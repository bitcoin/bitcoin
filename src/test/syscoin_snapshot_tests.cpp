#include "test/test_syscoin_services.h"
#include "data/utxo.json.h"
#include "utiltime.h"
#include "rpcserver.h"
#include <boost/test/unit_test.hpp>
#include <univalue.h>

extern UniValue read_json(const std::string& jsondata);
BOOST_FIXTURE_TEST_SUITE (syscoin_snapshot_tests, SyscoinMainNetSetup)
struct PaymentAmount
{
	std::string address;
	std::string amount;
};
void VerifySnapShot()
{
}
void GenerateSnapShot()
{
}
void GetUTXOs(std::vector<PaymentAmount> &paymentAmounts)
{
    UniValue tests = read_json(std::string(json_tests::utxo, json_tests::utxo + sizeof(json_tests::utxo)));
    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 2) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
		PaymentAmount payment;
        payment.address  = test[0].get_str();
        payment.amount = ValueFromAmount(test[1].get_int64()).write());
		paymentAmounts.push_back(payment);
    }
}
bool IsMainNetAlreadyCreated()
{
	int height;
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "getinfo"));
	height = find_value(r.get_obj(), "blocks").get_int();
	return height > 0;
}
BOOST_AUTO_TEST_CASE (generate_and_verify_snapshot)
{
	std::vector<PaymentAmount> paymentAmounts;
	GetUTXOs(paymentAmounts);
	if(IsMainNetAlreadyCreated())
	{
		VerifySnapShot();
	}
	else
	{
		GenerateSnapShot();
		VerifySnapShot();
	}
}
BOOST_AUTO_TEST_SUITE_END ()