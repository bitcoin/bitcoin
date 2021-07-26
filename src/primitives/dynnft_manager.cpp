#include <primitives/dynnft_manager.h>
#include <crypto/aes.h>



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

    {
        LOCK(requestLock);
        std::map<std::string,time_t>::iterator i = requestAssetClass.find(assetClass->hash);
        if (i != requestAssetClass.end())
            requestAssetClass.erase(i);
    }


}

void CNFTManager::addNFTAsset(CNFTAsset* asset) {


    std::string key = gArgs.GetArg("-nftdbkey", "");

    AES256Encrypt enc((const unsigned char*)key.data());
    std::string encryptedData;
    encryptedData.resize(asset->binaryData.size() + 32);
    enc.Encrypt((unsigned char*)encryptedData.data(), (const unsigned char*)asset->binaryData.data());

    /*
    AES256Decrypt dec(key.data());
    dec.Decrypt(buf.data(), buf.data());
    */


    std::string sql = "insert into asset (asset_txn_id, asset_hash, asset_class_hash, asset_metadata, asset_owner, asset_binary_data, asset_serial) values (@txn, @hash, @meta, @owner, @count, @bin, @ser)";

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(nftDB, sql.c_str(), -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, asset->txnID.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 2, asset->hash.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 3, asset->assetClassHash.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 4, asset->metaData.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 5, asset->owner.c_str(), -1, NULL);
    sqlite3_bind_blob(stmt, 6, encryptedData.c_str(), encryptedData.size(), NULL);
    sqlite3_bind_int64(stmt, 7, asset->serial);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);



}


bool CNFTManager::assetClassInDatabase(std::string assetClassHash) {

    std::string sql = "select count(asset_class_hash) from asset_class where asset_class_hash = @1";

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(nftDB, sql.c_str(), -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, assetClassHash.c_str(), -1, NULL);

    int rc = sqlite3_step(stmt);

    int valInt = 0;
    if ((rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        valInt = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return (valInt > 0);
}

bool CNFTManager::assetInDatabase(std::string assetHash) {
    std::string sql = "select count(asset_hash) from asset where asset_hash = @1";

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(nftDB, sql.c_str(), -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, assetHash.c_str(), -1, NULL);

    int rc = sqlite3_step(stmt);

    int valInt = 0;
    if ((rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        valInt = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return (valInt > 0);
}


void CNFTManager::queueAssetClassRequest(std::string hash) {

    {
        LOCK(requestLock);
        time_t now;
        time(&now);
        requestAssetClass.emplace(hash, now);
    }
}

void CNFTManager::queueAssetRequest(std::string hash) {

    {
        LOCK(requestLock);
        time_t now;
        time(&now);
        requestAsset.emplace(hash, now);
    }
}




bool CNFTManager::assetClassInCache(std::string hash) {
    bool result = false;
    {
        LOCK(cacheLock);
        if (assetClassCache.count(hash))
            result = true;
    }

    return result;
}

bool CNFTManager::assetInCache(std::string hash) {
    bool result = false;
    {
        LOCK(cacheLock);
        if (assetCache.count(hash))
            result = true;
    }

    return result;
}

void CNFTManager::addAssetClassToCache(CNFTAssetClass* assetClass) {
    LOCK(cacheLock);
    //only store up to 100 asset classes, if exceeded, remove least recently used first
    if (assetClassCache.size() >= 100) {
        std::string lruHash;
        time_t lruTime = ULLONG_MAX;
        std::map<std::string, time_t>::iterator i = lastCacheAccessAssetClass.begin();
        while (i != lastCacheAccessAssetClass.end()) {
            if (i->second < lruTime) {
                lruTime = i->second;
                lruHash = i->first;
            }
            i++;
        }
        CNFTAssetClass* tmp = assetClassCache.at(lruHash);
        assetClassCache.erase(lruHash);
        delete tmp;
    }

    assetClassCache.emplace(assetClass->hash, assetClass);
    time_t now;
    time(&now);
    lastCacheAccessAssetClass.emplace(assetClass->hash, now);

}


void CNFTManager::addAssetToCache(CNFTAsset* asset) {

    LOCK(cacheLock);
    //only store up to 100 assets, if exceeded, remove least recently used first
    if (assetCache.size() >= 100) {
        std::string lruHash;
        time_t lruTime = ULLONG_MAX;
        std::map<std::string, time_t>::iterator i = lastCacheAccessAsset.begin();
        while (i != lastCacheAccessAsset.end()) {
            if (i->second < lruTime) {
                lruTime = i->second;
                lruHash = i->first;
            }
            i++;
        }
        CNFTAsset* tmp = assetCache.at(lruHash);
        assetCache.erase(lruHash);
        delete tmp;
    }

    assetCache.emplace(asset->hash, asset);
    time_t now;
    time(&now);
    lastCacheAccessAsset.emplace(asset->hash, now);

}



CNFTAssetClass* CNFTManager::retrieveAssetClassFromCache(std::string hash) {
    LOCK(cacheLock);
    CNFTAssetClass* result = NULL;

    if (assetClassCache.count(hash) > 0) {
        result = assetClassCache.at(hash);
        time_t now;
        time(&now);
        lastCacheAccessAssetClass.at(hash) = now;
    }

    return result;
}


CNFTAsset* CNFTManager::retrieveAssetFromCache(std::string hash) {
    LOCK(cacheLock);
    CNFTAsset* result = NULL;

    if (assetCache.count(hash) > 0) {
        result = assetCache.at(hash);
        time_t now;
        time(&now);
        lastCacheAccessAsset.at(hash) = now;
    }

    return result;
}


CNFTAssetClass* CNFTManager::retrieveAssetClassFromDatabase(std::string hash)
{

    CNFTAssetClass* result = NULL;

    std::string sql = "select asset_class_txn_id, asset_class_hash, asset_class_metadata, asset_class_owner, asset_class_count from asset_class where asset_class_hash = @hash";
    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(nftDB, sql.c_str(), -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, NULL);
    int rc = sqlite3_step(stmt);
    if ((rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        const char* cAssetClassTxnID = (const char*)sqlite3_column_text(stmt, 0);
        const char* cAssetClassHash = (const char*)sqlite3_column_text(stmt, 1);
        const char* cAssetClassMetaData = (const char*)sqlite3_column_text(stmt, 2);
        const char* cAssetClassOwner = (const char*)sqlite3_column_text(stmt, 3);
        UINT64 iCount = sqlite3_column_int64(stmt, 4);

        result = new CNFTAssetClass();
        result->txnID = std::string(cAssetClassTxnID);
        result->hash = std::string(cAssetClassHash);
        result->metaData = std::string(cAssetClassMetaData);
        result->owner = std::string(cAssetClassOwner);
        result->maxCount = iCount;
    }

    sqlite3_finalize(stmt);

    if (result != NULL)
        addAssetClassToCache(result);

    return result;
   
}
