#include <primitives/dynnft_manager.h>


CNFTManager::CNFTManager() {

}


void CNFTManager::CreateOrOpenDatabase(std::string dataDirectory) {
    sqlite3_initialize();

    std::string dbName = (dataDirectory + std::string("\\nft.db"));
    sqlite3_open_v2(dbName.c_str(), &nftDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    uint32_t tableExists = execScalar("select count(name) from sqlite_master where type = 'table' and name = 'asset_class'");

    if (!tableExists) {
        char* sql = "CREATE TABLE asset_class ("
                    "asset_class_txn_id             TEXT                      NOT NULL,"
                    "asset_class_hash               TEXT                      NOT NULL,"
                    "asset_class_metadata           TEXT                      NOT NULL,"
                    "asset_class_owner              TEXT                      NOT NULL,"
                    "asset_class_count              INTEGER                   NOT NULL");

        sqlite3_exec(nftDB, sql, NULL, NULL, NULL);

        sql = "create index asset_class_owner_idx on asset_class(asset_class_owner)";

        sqlite3_exec(nftDB, sql, NULL, NULL, NULL);


        printf("%s", sqlite3_errmsg(nftDB));
    }

    tableExists = execScalar("select count(name) from sqlite_master where type = 'table' and name = 'asset'");

    if (!tableExists) {
        char* sql = "CREATE TABLE asset ("
                    "asset_txn_id             TEXT                      NOT NULL,"
                    "asset_hash               TEXT                      NOT NULL,"
                    "asset_class_hash         TEXT                      NOT NULL,"
                    "asset_metadata           TEXT                      NOT NULL,"
                    "asset_owner              TEXT                      NOT NULL,"
                    "asset_binary_data        TEXT                      NOT NULL,"
                    "asset_serial             INTEGER                   NOT NULL");

        sqlite3_exec(nftDB, sql, NULL, NULL, NULL);

        sql = "create index asset_owner_idx on asset(asset_owner)";

        sqlite3_exec(nftDB, sql, NULL, NULL, NULL);


        printf("%s", sqlite3_errmsg(nftDB));
    }
}



uint32_t CNFTManager::execScalar(char* sql) {
    uint32_t valInt = -1;

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(nftDB, sql, -1, &stmt, NULL);

    rc = sqlite3_step(stmt);

    if ((rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        valInt = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return valInt;
}

void CNFTManager::addNFTAssetClass(CNFTAssetClass* assetClass) {

}

void CNFTManager::addNFTAsset(CNFTAsset* asset) {

}
