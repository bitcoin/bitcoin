#include "test/test_syscoin_services.h"
#include "data/utxo.json.h"
#include "utiltime.h"
#include "rpcserver.h"
#include <boost/test/unit_test.hpp>
#include <univalue.h>
int currentTx = 0;
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
void SendSnapShotPayment(const std::string &strSend)
{
	currentTx++;
	std::string strSendMany = "sendmany \"\" {" + strSend + "}";
	UniValue r;
	
	BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", strSendMany, false));
}
void GenerateSnapShot(const std::vector<PaymentAmount> &paymentAmounts)
{
	// generate snapshot payments and let it mature
	printf("Generating 101 blocks to start the mainnet\n");
	GenerateMainNetBlocks(101, "mainnet1");

	int numberOfTxPerBlock = 250;
	double nTotal  =0;
	std::string sendManyString = "";
	for(int i =0;i<paymentAmounts.size();i++)
	{
		if(sendManyString != "") 
			sendManyString += ",";
		sendManyString += "\\\"" + paymentAmounts[i].address + "\\\":" + paymentAmounts[i].amount;
		nTotal += atof(paymentAmounts[i].amount.c_str());
		if(i != 0 && (i%numberOfTxPerBlock) == 0)
		{
			printf("strSendMany #%d, total %f\n", currentTx, nTotal);
			SendSnapShotPayment(sendManyString);
			GenerateMainNetBlocks(1, "mainnet1");
			sendManyString = "";
			nTotal = 0;
		}
	}
	if(sendManyString != "") 
	{
		SendSnapShotPayment(sendManyString);
		GenerateMainNetBlocks(1, "mainnet1");
	}
	
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
		CAmount amount = test[1].get_int64() / 299.4f;
        payment.amount = ValueFromAmount(amount).write();
		paymentAmounts.push_back(payment);
    }
}
bool IsMainNetAlreadyCreated()
{
	int height;
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "getinfo", false));
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
		GenerateSnapShot(paymentAmounts);
		VerifySnapShot();
	}
}
BOOST_AUTO_TEST_SUITE_END ()