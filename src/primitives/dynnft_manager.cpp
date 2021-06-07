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
                    "asset_class_count              INTEGER                   NOT NULL)";

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
                    "asset_binary_data        BLOB                      NOT NULL,"
                    "asset_serial             INTEGER                   NOT NULL)";

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

    std::string sql = "insert into asset_class (asset_class_txn_id, asset_class_hash, asset_class_metadata, asset_class_owner, asset_class_count) values (@txn, @hash, @meta, @owner, @count)";

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(nftDB, sql.c_str(), -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, assetClass->txnID.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 2, assetClass->hash.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 3, assetClass->metaData.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 4, assetClass->owner.c_str(), -1, NULL);
    sqlite3_bind_int64(stmt, 5, assetClass->maxCount);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);

}

void CNFTManager::addNFTAsset(CNFTAsset* asset) {

    std::string sql = "insert into asset (asset_txn_id, asset_hash, asset_class_hash, asset_metadata, asset_owner, asset_binary_data, asset_serial) values (@txn, @hash, @meta, @owner, @count, @bin, @ser)";

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(nftDB, sql.c_str(), -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, asset->txnID.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 2, asset->hash.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 3, asset->assetClassHash.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 4, asset->metaData.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 5, asset->owner.c_str(), -1, NULL);
    sqlite3_bind_blob(stmt, 6, asset->binaryData.c_str(), asset->binaryData.size(), NULL);
    sqlite3_bind_int64(stmt, 7, asset->serial);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);


                /*
                    assert(key.size() == 32);
                assert(in.size() == 16);
                assert(correctout.size() == 16);
                AES256Encrypt enc(key.data());
                buf.resize(correctout.size());
                enc.Encrypt(buf.data(), in.data());
                BOOST_CHECK(buf == correctout);
                AES256Decrypt dec(key.data());
                dec.Decrypt(buf.data(), buf.data());
                BOOST_CHECK(buf == in);
                */

}
