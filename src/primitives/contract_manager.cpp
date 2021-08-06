#include <primitives/contract_manager.h>

CContractManager::CContractManager() {

    contractDB = NULL;

}



void CContractManager::CreateOrOpenDatabase(std::string dataDirectory)
{

    //TODO - error checking

    sqlite3_initialize();

#ifdef WIN32
    std::string dbName = (dataDirectory + std::string("\\contract.db"));
#else
    std::string dbName = (dataDirectory + std::string("/contract.db"));
#endif

    sqlite3_open_v2(dbName.c_str(), &contractDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    uint32_t tableExists = execScalar("select count(name) from sqlite_master where type = 'table' and name = 'contract'");

    if (!tableExists) {

        char* sql = "CREATE TABLE contract ("
                    "contract_txn_id         TEXT                      NOT NULL,"
                    "contract_block_hash     TEXT                      NOT NULL,"
                    "contract_addr           TEXT                      NOT NULL,"
                    "contract_code           TEXT                      NOT NULL,"
                    "contract_balance        INTEGER                   NOT NULL)";

        sqlite3_exec(contractDB, sql, NULL, NULL, NULL);

        sql = "create index contract_contract_addr on contract(contract_addr)";

        sqlite3_exec(contractDB, sql, NULL, NULL, NULL);


        printf("%s", sqlite3_errmsg(contractDB));
    }




}


//TODO - allow nullable result
uint32_t CContractManager::execScalar(char* sql) {

    uint32_t valInt = -1;

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(contractDB, sql, -1, &stmt, NULL);

    rc = sqlite3_step(stmt);

    if ((rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        valInt = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return valInt;

}


void CContractManager::addContract(CContract* contract) {

    std::string sql = "insert into contract (contract_txn_id, contract_block_hash, contract_addr, contract_code, contract_balance) values (@txn, @height, @addr, @code, @bal)";

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(contractDB, sql.c_str(), -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, contract->contractTxnID.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 2, contract->blockHash.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 3, contract->contractAddress.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 4, contract->code.c_str(), -1, NULL);
    sqlite3_bind_int64(stmt, 5, contract->balance);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);
}
