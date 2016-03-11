#include "test_syscoin_services.h"
#include "utiltime.h"
#include "util.h"
#include "amount.h"
#include "rpcserver.h"
#include <memory>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/thread.hpp>


// SYSCOIN testing setup
void StartNodes()
{
	printf("Stopping any test nodes that are running...\n");
	StopNodes();
	if(boost::filesystem::exists(boost::filesystem::system_complete("node1/regtest")))
		boost::filesystem::remove_all(boost::filesystem::system_complete("node1/regtest"));
	MilliSleep(1000);
	if(boost::filesystem::exists(boost::filesystem::system_complete("node2/regtest")))
		boost::filesystem::remove_all(boost::filesystem::system_complete("node2/regtest"));
	MilliSleep(1000);
	if(boost::filesystem::exists(boost::filesystem::system_complete("node3/regtest")))
		boost::filesystem::remove_all(boost::filesystem::system_complete("node3/regtest"));
	printf("Starting 3 nodes in a regtest setup...\n");
	StartNode("node1");
	StartNode("node2");
	StartNode("node3");

}
void StartMainNetNodes()
{
	StopMainNetNodes();
	printf("Starting 2 nodes in mainnet setup...\n");
	StartNode("mainnet1", false);
	StartNode("mainnet2", false);
}
void StopMainNetNodes()
{
	printf("Stopping mainnet1..\n");
	try{
		CallRPC("mainnet1", "stop");
	}
	catch(const runtime_error& error)
	{
	}	
	MilliSleep(3000);
	printf("Stopping mainnet2..\n");
	try{
		CallRPC("mainnet2", "stop");
	}
	catch(const runtime_error& error)
	{
	}	
	printf("Done!\n");
}
void StopNodes()
{
	printf("Stopping node1..\n");
	try{
		CallRPC("node1", "stop");
	}
	catch(const runtime_error& error)
	{
	}	
	MilliSleep(3000);
	printf("Stopping node2..\n");
	try{
		CallRPC("node2", "stop");
	}
	catch(const runtime_error& error)
	{
	}	
	MilliSleep(3000);
	printf("Stopping node3..\n");
	try{
		CallRPC("node3", "stop");
	}
	catch(const runtime_error& error)
	{
	}	
	MilliSleep(3000);
	printf("Done!\n");
}
void StartNode(const string &dataDir, bool regTest)
{
    boost::filesystem::path fpath = boost::filesystem::system_complete("../syscoind");
	string nodePath = fpath.string() + string(" -datadir=") + dataDir;
	if(regTest)
		nodePath += string(" -regtest -debug");
    boost::thread t(runCommand, nodePath);
	printf("Launching %s, waiting 3 seconds before trying to ping...\n", dataDir.c_str());
	MilliSleep(3000);
	while (1)
	{
		try{
			CallRPC(dataDir, "getinfo", regTest);
		}
		catch(const runtime_error& error)
		{
			printf("Waiting for %s to come online, trying again in 5 seconds...\n", dataDir.c_str());
			MilliSleep(5000);
			continue;
		}
		break;
	}
	printf("Done!\n");
}
UniValue CallRPC(const string &dataDir, const string& commandWithArgs, bool regTest)
{
	UniValue val;
	boost::filesystem::path fpath = boost::filesystem::system_complete("../syscoin-cli");
	string path = fpath.string() + string(" -datadir=") + dataDir;
	if(regTest)
		path += string(" -regtest ");
	else
		path += " ";
	path += commandWithArgs;
	string rawJson = CallExternal(path);
    val.read(rawJson);
	if(val.isNull())
		throw runtime_error("Could not parse rpc results");
	return val;
}
int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}
void safe_fclose(FILE* file)
{
      if (file)
         BOOST_VERIFY(0 == fclose(file));
	if(boost::filesystem::exists("cmdoutput.log"))
		boost::filesystem::remove("cmdoutput.log");
}
int runSysCommand(const std::string& strCommand)
{
    int nErr = ::system(strCommand.c_str());
	return nErr;
}
std::string CallExternal(std::string &cmd)
{
	cmd += " > cmdoutput.log || true";
	if(runSysCommand(cmd))
		return string("ERROR");
    boost::shared_ptr<FILE> pipe(fopen("cmdoutput.log", "r"), safe_fclose);
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
	if(fsize(pipe.get()) > 0)
	{
		while (!feof(pipe.get())) {
			if (fgets(buffer, 128, pipe.get()) != NULL)
				result += buffer;
		}
	}
    return result;
}
void GenerateMainNetBlocks(int nBlocks, const string& node)
{
	int targetHeight, newHeight;
	UniValue r;
	string otherNode1 = "mainnet1";
	if(node == "mainnet1")
	{
		otherNode1 = "mainnet2";
	}
	else if(node == "mainnet2")
	{
		otherNode1 = "mainnet1";
	}
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "getinfo"));
	targetHeight = find_value(r.get_obj(), "blocks").get_int() + nBlocks;
	newHeight = 0;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "setgenerate true 1"));
	while(newHeight < targetHeight)
	{
	  MilliSleep(100);
	  BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "getinfo"));
	  newHeight = find_value(r.get_obj(), "blocks").get_int();
	}
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "setgenerate false"));
	BOOST_CHECK(newHeight >= targetHeight);
}
// generate n Blocks, with up to 10 seconds relay time buffer for other nodes to get the blocks.
// may fail if your network is slow or you try to generate too many blocks such that can't relay within 10 seconds
void GenerateBlocks(int nBlocks, const string& node)
{
  int height, newHeight, timeoutCounter;
  UniValue r;
	string otherNode1 = "node2";
	string otherNode2 = "node3";
	if(node == "node2")
	{
		otherNode1 = "node3";
		otherNode2 = "node1";
	}
	else if(node == "node3")
	{
		otherNode1 = "node1";
		otherNode2 = "node2";
	}
  BOOST_CHECK_NO_THROW(r = CallRPC(node, "getinfo"));
  newHeight = find_value(r.get_obj(), "blocks").get_int() + nBlocks;
  const string &sBlocks = strprintf("%d",nBlocks);
  BOOST_CHECK_NO_THROW(r = CallRPC(node, "generate " + sBlocks));
  BOOST_CHECK_NO_THROW(r = CallRPC(node, "getinfo"));
  height = find_value(r.get_obj(), "blocks").get_int();
  BOOST_CHECK(height == newHeight);
  height = 0;
  timeoutCounter = 0;
  while(height != newHeight)
  {
	  MilliSleep(100);
	  BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "getinfo"));
	  height = find_value(r.get_obj(), "blocks").get_int();
	  timeoutCounter++;
	  if(timeoutCounter > 300)
		  break;
  }
  BOOST_CHECK(height == newHeight);
  height = 0;
  timeoutCounter = 0;
  while(height != newHeight)
  {
	  MilliSleep(100);
	  BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "getinfo"));
	  height = find_value(r.get_obj(), "blocks").get_int();
	  timeoutCounter++;
	  if(timeoutCounter > 300)
		  break;
  }
  BOOST_CHECK(height == newHeight);
  height = 0;
  timeoutCounter = 0;
}
void CreateSysRatesIfNotExist()
{
	string data = "{QuratesQu:[{QucurrencyQu:QuUSDQu,QurateQu:2690.1,QuprecisionQu:2},{QucurrencyQu:QuEURQu,QurateQu:2695.2,QuprecisionQu:2},{QucurrencyQu:QuGBPQu,QurateQu:2697.3,QuprecisionQu:2},{QucurrencyQu:QuCADQu,QurateQu:2698.0,QuprecisionQu:2},{QucurrencyQu:QuBTCQu,QurateQu:100000.0,QuprecisionQu:8},{QucurrencyQu:QuSYSQu,QurateQu:1.0,QuprecisionQu:2}]}";
	// should get runtime error if doesnt exist
	try{
		CallRPC("node1", "aliasupdate SYS_RATES " + data);
	}
	catch(const runtime_error& err)
	{
		GenerateBlocks(200);	
		BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew SYS_RATES " + data));
	}
	GenerateBlocks(10);
}
string AliasNew(const string& node, const string& aliasname, const string& aliasdata)
{
	string otherNode1 = "node2";
	string otherNode2 = "node3";
	if(node == "node2")
	{
		otherNode1 = "node3";
		otherNode2 = "node1";
	}
	else if(node == "node3")
	{
		otherNode1 = "node1";
		otherNode2 = "node2";
	}
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasnew " + aliasname + " " + aliasdata));
	string pubkey;
	const UniValue &resultArray = r.get_array();
	pubkey = resultArray[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + aliasname));
	BOOST_CHECK(find_value(r.get_obj(), "name").get_str() == aliasname);
	BOOST_CHECK(find_value(r.get_obj(), "value").get_str() == aliasdata);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_bool() == true);
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "aliasinfo " + aliasname));
	BOOST_CHECK(find_value(r.get_obj(), "name").get_str() == aliasname);
	BOOST_CHECK(find_value(r.get_obj(), "value").get_str() == aliasdata);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_bool() == false);
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "aliasinfo " + aliasname));
	BOOST_CHECK(find_value(r.get_obj(), "name").get_str() == aliasname);
	BOOST_CHECK(find_value(r.get_obj(), "value").get_str() == aliasdata);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_bool() == false);
	return pubkey;
}
void AliasTransfer(const string& node, const string& aliasname, const string& tonode, const string& pubdata, const string& privdata, string pubkey)
{
	UniValue r;
	if(pubkey.size() <= 0)
	{
		UniValue pkr = CallRPC(tonode, "generatepublickey");
		if (pkr.type() != UniValue::VARR)
			throw runtime_error("Could not parse rpc results");

		const UniValue &resultArray = pkr.get_array();
		pubkey = resultArray[0].get_str();		
	}
	r = CallRPC(node, "aliasupdate " + aliasname + " " + pubdata + " " + privdata + " " + pubkey);
	GenerateBlocks(10, tonode);
	GenerateBlocks(10, node);	
	// check its not mine anymore
	r = CallRPC(node, "aliasinfo " + aliasname);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_bool() == false);
	BOOST_CHECK(find_value(r.get_obj(), "value").get_str() == pubdata);
	BOOST_CHECK(find_value(r.get_obj(), "privatevalue").get_str() == "Encrypted for alias owner");
	// check xferred right person and data changed
	r = CallRPC(tonode, "aliasinfo " + aliasname);
	BOOST_CHECK(find_value(r.get_obj(), "value").get_str() == pubdata);
	BOOST_CHECK(find_value(r.get_obj(), "privatevalue").get_str() == privdata);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_bool() == true);
}
void AliasUpdate(const string& node, const string& aliasname, const string& pubdata, const string& privdata)
{
	string otherNode1 = "node2";
	string otherNode2 = "node3";
	if(node == "node2")
	{
		otherNode1 = "node3";
		otherNode2 = "node1";
	}
	else if(node == "node3")
	{
		otherNode1 = "node1";
		otherNode2 = "node2";
	}
	UniValue r;
	r = CallRPC(node, "aliasupdate " + aliasname + " " + pubdata + " " + privdata);
	// ensure mempool blocks second tx until it confirms
	BOOST_CHECK_THROW(CallRPC(node, "aliasupdate " + aliasname + " " + pubdata + " " + privdata), runtime_error);
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + aliasname));
	BOOST_CHECK(find_value(r.get_obj(), "name").get_str() == aliasname);
	BOOST_CHECK(find_value(r.get_obj(), "value").get_str() == pubdata);
	BOOST_CHECK(find_value(r.get_obj(), "privatevalue").get_str() == privdata);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_bool() == true);
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "aliasinfo " + aliasname));
	BOOST_CHECK(find_value(r.get_obj(), "name").get_str() == aliasname);
	BOOST_CHECK(find_value(r.get_obj(), "value").get_str() == pubdata);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_bool() == false);
	BOOST_CHECK(find_value(r.get_obj(), "privatevalue").get_str() == "Encrypted for alias owner");
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "aliasinfo " + aliasname));
	BOOST_CHECK(find_value(r.get_obj(), "name").get_str() == aliasname);
	BOOST_CHECK(find_value(r.get_obj(), "value").get_str() == pubdata);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_bool() == false);
	BOOST_CHECK(find_value(r.get_obj(), "privatevalue").get_str() == "Encrypted for alias owner");
}
const string CertNew(const string& node, const string& alias, const string& title, const string& data, bool privateData)
{
	string otherNode1 = "node2";
	string otherNode2 = "node3";
	if(node == "node2")
	{
		otherNode1 = "node3";
		otherNode2 = "node1";
	}
	else if(node == "node3")
	{
		otherNode1 = "node1";
		otherNode2 = "node2";
	}
	string privateFlag = privateData? "1":"0";
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certnew " + alias + " " + title + " " + data + " " + privateFlag));
	const UniValue &arr = r.get_array();
	string guid = arr[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() == alias);
	BOOST_CHECK(find_value(r.get_obj(), "data").get_str() == data);
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "true");
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == guid);
	if(privateData)
	{
		BOOST_CHECK(find_value(r.get_obj(), "private").get_str() == "Yes");
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() != data);
	}
	else
	{
		BOOST_CHECK(find_value(r.get_obj(), "private").get_str() == "No");
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() == data);
	}

	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "false");
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == guid);
	if(privateData)
	{
		BOOST_CHECK(find_value(r.get_obj(), "private").get_str() == "Yes");
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() != data);
	}
	else
	{
		BOOST_CHECK(find_value(r.get_obj(), "private").get_str() == "No");
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() == data);
	}
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "false");
	return guid;
}
void CertUpdate(const string& node, const string& guid, const string& title, const string& data, bool privateData)
{
	string otherNode1 = "node2";
	string otherNode2 = "node3";
	if(node == "node2")
	{
		otherNode1 = "node3";
		otherNode2 = "node1";
	}
	else if(node == "node3")
	{
		otherNode1 = "node1";
		otherNode2 = "node2";
	}
	UniValue r;
	string privateFlag = privateData? "1":" 0";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certupdate " + guid + " " + title + " " + data + " " +privateFlag));
	// ensure mempool blocks second tx until it confirms
	BOOST_CHECK_THROW(CallRPC(node, "certupdate " + guid + " " + title + " " + data + " " +privateFlag), runtime_error);
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "data").get_str() == data);
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "true");
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	if(privateData)
	{
		BOOST_CHECK(find_value(r.get_obj(), "private").get_str() == "Yes");
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() != data);
	}
	else
	{
		BOOST_CHECK(find_value(r.get_obj(), "private").get_str() == "No");
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() == data);
	}

	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "false");
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	if(privateData)
	{
		BOOST_CHECK(find_value(r.get_obj(), "private").get_str() == "Yes");
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() != data);
	}
	else
	{
		BOOST_CHECK(find_value(r.get_obj(), "private").get_str() == "No");
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() == data);
	}
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "false");
}
void CertTransfer(const string& node, const string& guid, const string& toalias)
{
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certinfo " + guid));
	string data = find_value(r.get_obj(), "data").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certtransfer " + guid + " " + toalias));
	// ensure mempool blocks second tx until it confirms
	BOOST_CHECK_THROW(CallRPC(node, "certtransfer " + guid + " " + toalias), runtime_error);
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certinfo " + guid));
	bool privateData = find_value(r.get_obj(), "private").get_str() == "Yes";
	if(privateData)
	{
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() != data);
	}
	else
	{
		BOOST_CHECK(find_value(r.get_obj(), "data").get_str() == data);
	}
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "false");
}
const string MessageNew(const string& fromnode, const string& tonode, const string& title, const string& data, const string& fromalias, const string& toalias)
{
	string otherNode;
	if(fromnode != "node1" && tonode != "node1")
		otherNode = "node1";
	else if(fromnode != "node2" && tonode != "node2")
		otherNode = "node2";
	else if(fromnode != "node3" && tonode != "node3")
		otherNode = "node3";
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(fromnode, "messagenew " + title + " " + data + " " + fromalias + " " + toalias));
	const UniValue &arr = r.get_array();
	string guid = arr[1].get_str();
	GenerateBlocks(10, fromnode);
	BOOST_CHECK_NO_THROW(r = CallRPC(fromnode, "messageinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "GUID").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "message").get_str() == data);
	BOOST_CHECK(find_value(r.get_obj(), "subject").get_str() == title);
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode, "messageinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "GUID").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "message").get_str() != data);
	BOOST_CHECK(find_value(r.get_obj(), "subject").get_str() == title);
	BOOST_CHECK_NO_THROW(r = CallRPC(tonode, "messageinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "GUID").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "message").get_str() == data);
	BOOST_CHECK(find_value(r.get_obj(), "subject").get_str() == title);
	return guid;
}
const string OfferLink(const string& node, const string& alias, const string& guid, const string& commission, const string& newdescription)
{
	UniValue r;
	string otherNode1 = "node2";
	string otherNode2 = "node3";
	if(node == "node2")
	{
		otherNode1 = "node3";
		otherNode2 = "node1";
	}
	else if(node == "node3")
	{
		otherNode1 = "node1";
		otherNode2 = "node2";
	}
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + guid));
	const string &olddescription = find_value(r.get_obj(), "description").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerlink " + alias + " " + guid + " " + commission + " " + newdescription));
	const UniValue &arr = r.get_array();
	string linkedguid = arr[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + linkedguid));
	if(!newdescription.empty())
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == newdescription);
	else
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == olddescription);

	string exmode = "ON";
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == linkedguid);
	string commissionwithpct = commission + "%";
	BOOST_CHECK(find_value(r.get_obj(), "offerlink_guid").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "offerlink").get_str() == "true");
	BOOST_CHECK(find_value(r.get_obj(), "commission").get_str() == commissionwithpct);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "true");
	// linked offers always exclusive resell mode, cant resell a resell offere
	BOOST_CHECK(find_value(r.get_obj(), "exclusive_resell").get_str() == exmode);
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "offerinfo " + linkedguid));
	if(!newdescription.empty())
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == newdescription);
	else
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == olddescription);
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == linkedguid);
	// if offer is not yours you cannot see the offer link status including commission
	BOOST_CHECK(find_value(r.get_obj(), "offerlink_guid").get_str() == "");
	BOOST_CHECK(find_value(r.get_obj(), "offerlink").get_str() == "false");
	BOOST_CHECK(find_value(r.get_obj(), "commission").get_str() == "0");
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "false");
	BOOST_CHECK(find_value(r.get_obj(), "exclusive_resell").get_str() == exmode);
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "offerinfo " + linkedguid));
	if(!newdescription.empty())
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == newdescription);
	else
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == olddescription);
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == linkedguid);
	BOOST_CHECK(find_value(r.get_obj(), "offerlink_guid").get_str() == "");
	BOOST_CHECK(find_value(r.get_obj(), "offerlink").get_str() == "false");
	BOOST_CHECK(find_value(r.get_obj(), "commission").get_str() == "0");
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "false");
	BOOST_CHECK(find_value(r.get_obj(), "exclusive_resell").get_str() == exmode);
	return linkedguid;
}
const string OfferNew(const string& node, const string& aliasname, const string& category, const string& title, const string& qty, const string& price, const string& description, const string& currency, const string& certguid, const bool exclusiveResell)
{
	string otherNode1 = "node2";
	string otherNode2 = "node3";
	if(node == "node2")
	{
		otherNode1 = "node3";
		otherNode2 = "node1";
	}
	else if(node == "node3")
	{
		otherNode1 = "node1";
		otherNode2 = "node2";
	}
	CreateSysRatesIfNotExist();
	UniValue r;
	string certguidtmp = "0";
	if(!certguid.empty())
		certguidtmp = certguid;
	string offercreatestr = "offernew " + aliasname + " " + category + " " + title + " " + qty + " " + price + " " + description + " " + currency  + " " + certguidtmp;
	if(exclusiveResell == false)
	{
		offercreatestr += " 0";
	}
	BOOST_CHECK_NO_THROW(r = CallRPC(node, offercreatestr));
	const UniValue &arr = r.get_array();
	string guid = arr[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + guid));
	string exmode = "ON";
	if(!exclusiveResell)
		exmode = "OFF";
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == certguid);
	BOOST_CHECK(find_value(r.get_obj(), "exclusive_resell").get_str() == exmode);
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	BOOST_CHECK(find_value(r.get_obj(), "category").get_str() == category);
	BOOST_CHECK(find_value(r.get_obj(), "quantity").get_str() == qty);
	BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == description);
	BOOST_CHECK(find_value(r.get_obj(), "currency").get_str() == currency);
	BOOST_CHECK(find_value(r.get_obj(), "price").get_str() == price);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "true");
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "offerinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == certguid);
	BOOST_CHECK(find_value(r.get_obj(), "exclusive_resell").get_str() == exmode);
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	BOOST_CHECK(find_value(r.get_obj(), "category").get_str() == category);
	BOOST_CHECK(find_value(r.get_obj(), "quantity").get_str() == qty);
	BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == description);
	BOOST_CHECK(find_value(r.get_obj(), "currency").get_str() == currency);
	BOOST_CHECK(find_value(r.get_obj(), "price").get_str() == price);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "false");
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "offerinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == certguid);
	BOOST_CHECK(find_value(r.get_obj(), "exclusive_resell").get_str() == exmode);
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	BOOST_CHECK(find_value(r.get_obj(), "category").get_str() == category);
	BOOST_CHECK(find_value(r.get_obj(), "quantity").get_str() == qty);
	BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == description);
	BOOST_CHECK(find_value(r.get_obj(), "currency").get_str() == currency);
	BOOST_CHECK(find_value(r.get_obj(), "price").get_str() == price);
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "false");
	return guid;
}
const string EscrowNew(const string& node, const string& buyeralias, const string& offerguid, const string& qty, const string& message, const string& arbiteralias, const string& selleralias)
{
	string otherNode1 = "node2";
	string otherNode2 = "node3";
	if(node == "node2")
	{
		otherNode1 = "node3";
		otherNode2 = "node1";
	}
	else if(node == "node3")
	{
		otherNode1 = "node1";
		otherNode2 = "node2";
	}
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrownew " + buyeralias + " " + offerguid + " " + qty + " " + message + " " + arbiteralias));
	const UniValue &arr = r.get_array();
	string guid = arr[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offerguid));
	CAmount offerprice = AmountFromValue(find_value(r.get_obj(), "sysprice"));
	int nQty = atoi(qty.c_str());
	CAmount nTotal = offerprice*nQty;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "escrow").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == offerguid);
	BOOST_CHECK(AmountFromValue(find_value(r.get_obj(), "systotal")) == nTotal);
	BOOST_CHECK(find_value(r.get_obj(), "arbiter").get_str() == arbiteralias);
	BOOST_CHECK(find_value(r.get_obj(), "seller").get_str() == selleralias);
	BOOST_CHECK(find_value(r.get_obj(), "pay_message").get_str() == string("Encrypted for owner of offer"));
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "escrowinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "escrow").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == offerguid);
	BOOST_CHECK(AmountFromValue(find_value(r.get_obj(), "systotal")) == nTotal);
	BOOST_CHECK(find_value(r.get_obj(), "arbiter").get_str() == arbiteralias);
	BOOST_CHECK(find_value(r.get_obj(), "seller").get_str() == selleralias);
	BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "escrowinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "escrow").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == offerguid);
	BOOST_CHECK(AmountFromValue(find_value(r.get_obj(), "systotal")) == nTotal);
	BOOST_CHECK(find_value(r.get_obj(), "arbiter").get_str() == arbiteralias);
	BOOST_CHECK(find_value(r.get_obj(), "seller").get_str() == selleralias);
	return guid;
}
void EscrowRelease(const string& node, const string& guid)
{
	BOOST_CHECK_NO_THROW(CallRPC(node, "escrowrelease " + guid));
	GenerateBlocks(10, node);
}
void EscrowRefund(const string& node, const string& guid)
{
	BOOST_CHECK_NO_THROW(CallRPC(node, "escrowrefund " + guid));
	GenerateBlocks(10, node);
}
const UniValue FindOfferAccept(const string& node, const string& offerguid, const string& acceptguid)
{
	UniValue r, ret;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offeracceptlist " + offerguid));
	UniValue arrayValue = r.get_array();
	for(int i=0;i<arrayValue.size();i++)
	{
		const string &acceptvalueguid = find_value(arrayValue[i].get_obj(), "id").get_str();
		const string &offervalueguid = find_value(arrayValue[i].get_obj(), "offer").get_str();
		if(acceptvalueguid == acceptguid && offervalueguid == offerguid)
		{
			ret = arrayValue[i].get_obj();
			break;
		}

	}
	BOOST_CHECK(!ret.isNull());
	return ret;
}
const UniValue FindOfferLinkedAccept(const string& node, const string& offerguid, const string& acceptguid)
{
	UniValue r, ret;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offeracceptlist " + offerguid));
	UniValue arrayValue = r.get_array();
	for(int i=0;i<arrayValue.size();i++)
	{
		const string &linkedacceptguid = find_value(arrayValue[i].get_obj(), "linkofferaccept").get_str();
		const string &offervalueguid = find_value(arrayValue[i].get_obj(), "offer").get_str();
		if(linkedacceptguid == acceptguid && offervalueguid == offerguid)
		{
			ret = arrayValue[i].get_obj();
			break;
		}

	}
	BOOST_CHECK(!ret.isNull());
	return ret;
}
// not that this is for escrow dealing with non-linked offers
void EscrowClaimRelease(const string& node, const string& guid)
{
	UniValue r;
	BOOST_CHECK_NO_THROW(CallRPC(node, "escrowrelease " + guid));
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	const string &acceptguid = find_value(r.get_obj(), "offeracceptlink").get_str();
	const string &offerguid = find_value(r.get_obj(), "offer").get_str();
	const string &pay_message = find_value(r.get_obj(), "pay_message").get_str();
	// check that you as the seller who claims the release can see the payment message
	BOOST_CHECK(pay_message != string("Encrypted for owner of offer"));
	CAmount escrowtotal = AmountFromValue(find_value(r.get_obj(), "systotal"));
	const UniValue &acceptValue = FindOfferAccept(node, offerguid, acceptguid);
	BOOST_CHECK(find_value(acceptValue, "escrowlink").get_str() == guid);
	CAmount nTotal = AmountFromValue(find_value(acceptValue, "systotal"));
	BOOST_CHECK(nTotal == escrowtotal);
	BOOST_CHECK(find_value(acceptValue, "ismine").get_str() == "true");
	// confirm that the unencrypted messages match from the escrow and the accept
	BOOST_CHECK(find_value(acceptValue, "pay_message").get_str() == pay_message);
}
float GetPriceOfOffer(const float nPrice, const int nDiscountPct, const int nCommission){
	float price = nPrice;
	float fDiscount = nDiscountPct;
	if(nDiscountPct < -99 || nDiscountPct > 99)
		fDiscount = 0;
	// fMarkup is a percentage, commission minus discount
	float fMarkup = nCommission - fDiscount;
	
	// add commission , subtract discount
	fMarkup = price*(fMarkup / 100);
	price = price + fMarkup;
	return price;
}
// not that this is for escrow dealing with linked offers
void EscrowClaimReleaseLink(const string& node, const string& guid, const string& resellernode)
{
	UniValue r;
	BOOST_CHECK_NO_THROW(CallRPC(node, "escrowrelease " + guid));
	GenerateBlocks(100, "node1");
	GenerateBlocks(100, "node2");
	GenerateBlocks(100, "node3");
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	const string &acceptguid = find_value(r.get_obj(), "offeracceptlink").get_str();
	const string &offerguid = find_value(r.get_obj(), "offer").get_str();
	const string &pay_message = find_value(r.get_obj(), "pay_message").get_str();
	// check that you as the seller who claims the release can see the payment message
	BOOST_CHECK(pay_message != string("Encrypted for owner of offer"));
	CAmount escrowtotal = AmountFromValue(find_value(r.get_obj(), "systotal"));
	BOOST_CHECK_NO_THROW(r = CallRPC(resellernode, "offerinfo " + offerguid));
	BOOST_CHECK(find_value(r.get_obj(), "offerlink").get_str() == "true");
	const string &rootofferguid = find_value(r.get_obj(), "offerlink_guid").get_str();
	const string &commissionstr = find_value(r.get_obj(), "commission").get_str();
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "true");

	const UniValue &acceptValue = FindOfferAccept(node, offerguid, acceptguid);
	BOOST_CHECK(find_value(acceptValue, "escrowlink").get_str() == guid);
	CAmount nTotal = AmountFromValue(find_value(acceptValue, "systotal"));
	BOOST_CHECK_EQUAL(nTotal, escrowtotal);
	BOOST_CHECK(find_value(acceptValue, "ismine").get_str() == "false");
	BOOST_CHECK(find_value(acceptValue, "pay_message").get_str() == string("Encrypted for owner of offer"));
	// now get the accept from the resellernode
	const UniValue &acceptReSellerValue = FindOfferAccept(resellernode, offerguid, acceptguid);
	nTotal = AmountFromValue(find_value(acceptReSellerValue, "systotal"));
	const string &discountstr = find_value(acceptReSellerValue, "offer_discount_percentage").get_str();
	BOOST_CHECK_EQUAL(nTotal, escrowtotal);
	BOOST_CHECK(find_value(acceptReSellerValue, "ismine").get_str() == "true");
	BOOST_CHECK(find_value(acceptReSellerValue, "pay_message").get_str() == string("Encrypted for owner of offer"));
	// now get the linked accept from the sellernode
	int discount = atoi(discountstr.c_str());
	int commission = atoi(commissionstr.c_str());
	GenerateBlocks(100, "node1");
	GenerateBlocks(100, "node2");
	GenerateBlocks(100, "node3");
	const UniValue &acceptSellerValue = FindOfferLinkedAccept(node, rootofferguid, acceptguid);
	BOOST_CHECK(find_value(acceptSellerValue, "escrowlink").get_str().empty());
	nTotal = AmountFromValue(find_value(acceptSellerValue, "systotal"));
	if(commission != 0 || discount != 0)
		BOOST_CHECK(nTotal != escrowtotal);
	float calculatedTotal = GetPriceOfOffer(nTotal, discount, commission);
	CAmount calculatedTotalAmount(calculatedTotal);
	
	BOOST_CHECK_EQUAL(calculatedTotalAmount/COIN, escrowtotal/COIN);
	BOOST_CHECK(find_value(acceptSellerValue, "ismine").get_str() == "true");
	BOOST_CHECK(find_value(acceptSellerValue, "pay_message").get_str() == pay_message);
}
BasicSyscoinTestingSetup::BasicSyscoinTestingSetup()
{
}
BasicSyscoinTestingSetup::~BasicSyscoinTestingSetup()
{
}
SyscoinTestingSetup::SyscoinTestingSetup()
{
	StartNodes();
}
SyscoinTestingSetup::~SyscoinTestingSetup()
{
	StopNodes();
	if(boost::filesystem::exists(boost::filesystem::system_complete("node1/regtest")))
		boost::filesystem::remove_all(boost::filesystem::system_complete("node1/regtest"));
	MilliSleep(1000);
	if(boost::filesystem::exists(boost::filesystem::system_complete("node2/regtest")))
		boost::filesystem::remove_all(boost::filesystem::system_complete("node2/regtest"));
	MilliSleep(1000);
	if(boost::filesystem::exists(boost::filesystem::system_complete("node3/regtest")))
		boost::filesystem::remove_all(boost::filesystem::system_complete("node3/regtest"));
}
SyscoinMainNetSetup::SyscoinMainNetSetup()
{
	StartMainNetNodes();
}
SyscoinMainNetSetup::~SyscoinMainNetSetup()
{
	StopMainNetNodes();
}