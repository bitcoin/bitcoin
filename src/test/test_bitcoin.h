

#include "main.h"
#include "random.h"
#include "txdb.h"
#include "ui_interface.h"
#include "util.h"
#ifdef ENABLE_WALLET
#include "db.h"
#include "wallet.h"
#endif

#include <boost/filesystem.hpp>

struct TestingSetupManager {
	CCoinsViewDB *pcoinsdbview;
    boost::filesystem::path pathTemp;
    boost::thread_group threadGroup;
	bool keepTestEvidence;
	
	TestingSetupManager();
	void CreateBlockChain();
	void DestroyBlockChain();
	void SetupGenesisBlockChain();
	void Start();
	void Stop();
};
	
extern TestingSetupManager testingSetupManager;
