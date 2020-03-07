// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include "base58.h"
#include "platform/nf-token/nf-tokens-manager.h"

namespace
{
    struct NfTokenManagerFixture
    {
        NfTokenManagerFixture()
        {
            CMutableTransaction fakeMutTx;
            fakeMutTx.nVersion = static_cast<int16_t>(3);
            fakeMutTx.nType = static_cast<int16_t>(TRANSACTION_NF_TOKEN_REGISTER);
            fakeMutTx.extraPayload = {'a', 'b', 'c'};

            m_fakeTx.reset(new CTransaction(fakeMutTx));

            m_fakeBlockHeader.nTime = 1551768613;
            m_fakeBlockHeader.nBits = 1500;
            m_fakeBlockHeader.nNonce = 89465165;

            m_fakeBlockIdx.reset(new CBlockIndex(m_fakeBlockHeader));
            auto blockHash = m_fakeBlockHeader.GetHash();
            m_fakeBlockIdx->phashBlock = &blockHash;
            m_fakeBlockIdx->nHeight = 101;

            m_nfTokensManager.UpdateBlockTip(m_fakeBlockIdx.get());
        }

        ~NfTokenManagerFixture()
        {
        }

        Platform::NfTokensManager m_nfTokensManager;
        std::unique_ptr<CTransaction> m_fakeTx;
        CBlockHeader m_fakeBlockHeader;
        std::unique_ptr<CBlockIndex> m_fakeBlockIdx;
    };
}

BOOST_FIXTURE_TEST_SUITE(NfTokenManagerTest, NfTokenManagerFixture)

    BOOST_AUTO_TEST_CASE(AddValidNfToken_GetNfToken)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        auto nftWeakPtr = m_nfTokensManager.GetNfToken(25, registeredTokenId);
        BOOST_CHECK(!nftWeakPtr.expired());

        auto nftSharedPtr = nftWeakPtr.lock();

        BOOST_CHECK_EQUAL(25, nftSharedPtr->tokenProtocolId);
        BOOST_CHECK_EQUAL("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1", nftSharedPtr->tokenId.ToString());
        BOOST_CHECK_EQUAL("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6", CBitcoinAddress(nftSharedPtr->tokenOwnerKeyId).ToString());
        BOOST_CHECK_EQUAL("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT", CBitcoinAddress(nftSharedPtr->metadataAdminKeyId).ToString());
        BOOST_CHECK_EQUAL("btc 2025 prediction", std::string(nftSharedPtr->metadata.begin(), nftSharedPtr->metadata.end()));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_Contains)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        BOOST_CHECK(m_nfTokensManager.Contains(25, registeredTokenId));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_ContainsAtRegistrationHeight)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        BOOST_CHECK(m_nfTokensManager.Contains(25, registeredTokenId, 101));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_ContainsHigherThen_RegistrationHeight)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        BOOST_CHECK(m_nfTokensManager.Contains(25, registeredTokenId, 201));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_NotContainsAtHeight)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        BOOST_CHECK(!m_nfTokensManager.Contains(25, registeredTokenId, 100));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_GetNfTokenIndex)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        auto nftIndex = m_nfTokensManager.GetNfTokenIndex(25, registeredTokenId);

        BOOST_CHECK_EQUAL(25, nftIndex->nfToken->tokenProtocolId);
        BOOST_CHECK_EQUAL("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1", nftIndex->nfToken->tokenId.ToString());
        BOOST_CHECK_EQUAL("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6", CBitcoinAddress(nftIndex->nfToken->tokenOwnerKeyId).ToString());
        BOOST_CHECK_EQUAL("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT", CBitcoinAddress(nftIndex->nfToken->metadataAdminKeyId).ToString());
        BOOST_CHECK_EQUAL("btc 2025 prediction", std::string(nftIndex->nfToken->metadata.begin(), nftIndex->nfToken->metadata.end()));

        BOOST_CHECK_EQUAL("1682d7f74e5977beac6b19d136ed601e539ed4d604f15ce4c961955110ed5d21", nftIndex->regTxHash.ToString());
        BOOST_CHECK_EQUAL(101, nftIndex->blockIndex->nHeight);

    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_OwnerOf)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        auto ownerId = m_nfTokensManager.OwnerOf(25, registeredTokenId);
        BOOST_CHECK_EQUAL("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6", CBitcoinAddress(ownerId).ToString());
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_BalanceOf)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));

        BOOST_CHECK_EQUAL(1, m_nfTokensManager.BalanceOf(25, nfToken.tokenOwnerKeyId));
        BOOST_CHECK_EQUAL(1, m_nfTokensManager.BalanceOf(nfToken.tokenOwnerKeyId));

        Platform::NfToken nfToken2;
        nfToken2.tokenProtocolId = 116546815;
        ownerAddress.GetKeyID(nfToken2.tokenOwnerKeyId);
        adminAddress.GetKeyID(nfToken2.metadataAdminKeyId);
        nfToken2.tokenId.SetHex("ef51e0d401537dc8f3a3394f5fb992819d493a77bd20e7638dc24321bf4ecfdb");
        std::string metadataStr2("another prediction");
        nfToken2.metadata.assign(metadataStr2.begin(), metadataStr2.end());

        CMutableTransaction fakeMutTx2;
        fakeMutTx2.nVersion = static_cast<int16_t>(3);
        fakeMutTx2.nType = static_cast<int16_t>(TRANSACTION_NF_TOKEN_REGISTER);
        fakeMutTx2.extraPayload = {'d', 'e', 'f'};

        CTransaction fakeTx2(fakeMutTx2);

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken2, fakeTx2, m_fakeBlockIdx.get()));

        BOOST_CHECK_EQUAL(1, m_nfTokensManager.BalanceOf(25, nfToken.tokenOwnerKeyId));
        BOOST_CHECK_EQUAL(1, m_nfTokensManager.BalanceOf(116546815, nfToken.tokenOwnerKeyId));
        BOOST_CHECK_EQUAL(2, m_nfTokensManager.BalanceOf(nfToken.tokenOwnerKeyId));

        Platform::NfToken nfToken3;
        nfToken3.tokenProtocolId = 25;
        ownerAddress.GetKeyID(nfToken3.tokenOwnerKeyId);
        adminAddress.GetKeyID(nfToken3.metadataAdminKeyId);
        nfToken3.tokenId.SetHex("5027a08676f392c326d4cb96c938a22e55c92a6c20ededc84a1f13fe3ba1b880");
        std::string metadataStr3("another prediction 2");
        nfToken3.metadata.assign(metadataStr3.begin(), metadataStr3.end());

        CMutableTransaction fakeMutTx3;
        fakeMutTx3.nVersion = static_cast<int16_t>(3);
        fakeMutTx3.nType = static_cast<int16_t>(TRANSACTION_NF_TOKEN_REGISTER);
        fakeMutTx3.extraPayload = {'x', 'y', 'z'};

        CTransaction fakeTx3(fakeMutTx3);

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken3, fakeTx3, m_fakeBlockIdx.get()));

        BOOST_CHECK_EQUAL(2, m_nfTokensManager.BalanceOf(25, nfToken.tokenOwnerKeyId));
        BOOST_CHECK_EQUAL(1, m_nfTokensManager.BalanceOf(116546815, nfToken.tokenOwnerKeyId));
        BOOST_CHECK_EQUAL(3, m_nfTokensManager.BalanceOf(nfToken.tokenOwnerKeyId));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_NfTokenIdsOf)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));

        auto protocolOwnerIds = m_nfTokensManager.NfTokenIdsOf(25, nfToken.tokenOwnerKeyId);
        auto ownerIds = m_nfTokensManager.NfTokenIdsOf(nfToken.tokenOwnerKeyId);

        BOOST_CHECK_EQUAL(1, protocolOwnerIds.size());
        BOOST_CHECK_EQUAL(1, ownerIds.size());

        BOOST_CHECK_EQUAL("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1", protocolOwnerIds.front().ToString());
        BOOST_CHECK_EQUAL("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1", ownerIds.front().ToString());


        Platform::NfToken nfToken2;
        nfToken2.tokenProtocolId = 116546815;
        ownerAddress.GetKeyID(nfToken2.tokenOwnerKeyId);
        adminAddress.GetKeyID(nfToken2.metadataAdminKeyId);
        nfToken2.tokenId.SetHex("ef51e0d401537dc8f3a3394f5fb992819d493a77bd20e7638dc24321bf4ecfdb");
        std::string metadataStr2("another prediction");
        nfToken2.metadata.assign(metadataStr2.begin(), metadataStr2.end());

        CMutableTransaction fakeMutTx2;
        fakeMutTx2.nVersion = static_cast<int16_t>(3);
        fakeMutTx2.nType = static_cast<int16_t>(TRANSACTION_NF_TOKEN_REGISTER);
        fakeMutTx2.extraPayload = {'d', 'e', 'f'};

        CTransaction fakeTx2(fakeMutTx2);

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken2, fakeTx2, m_fakeBlockIdx.get()));

        protocolOwnerIds = m_nfTokensManager.NfTokenIdsOf(25, nfToken.tokenOwnerKeyId);
        auto protocolOwnerIds2 = m_nfTokensManager.NfTokenIdsOf(116546815, nfToken.tokenOwnerKeyId);
        ownerIds = m_nfTokensManager.NfTokenIdsOf(nfToken.tokenOwnerKeyId);

        BOOST_CHECK_EQUAL(1, protocolOwnerIds.size());
        BOOST_CHECK_EQUAL(1, protocolOwnerIds2.size());
        BOOST_CHECK_EQUAL(2, ownerIds.size());

        BOOST_CHECK_EQUAL("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1", protocolOwnerIds.front().ToString());
        BOOST_CHECK_EQUAL("ef51e0d401537dc8f3a3394f5fb992819d493a77bd20e7638dc24321bf4ecfdb", protocolOwnerIds2.front().ToString());

        std::sort(ownerIds.begin(), ownerIds.end());
        BOOST_CHECK_EQUAL("ef51e0d401537dc8f3a3394f5fb992819d493a77bd20e7638dc24321bf4ecfdb", ownerIds[0].ToString());
        BOOST_CHECK_EQUAL("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1", ownerIds[1].ToString());


        Platform::NfToken nfToken3;
        nfToken3.tokenProtocolId = 25;
        ownerAddress.GetKeyID(nfToken3.tokenOwnerKeyId);
        adminAddress.GetKeyID(nfToken3.metadataAdminKeyId);
        nfToken3.tokenId.SetHex("5027a08676f392c326d4cb96c938a22e55c92a6c20ededc84a1f13fe3ba1b880");
        std::string metadataStr3("another prediction 2");
        nfToken3.metadata.assign(metadataStr3.begin(), metadataStr3.end());

        CMutableTransaction fakeMutTx3;
        fakeMutTx3.nVersion = static_cast<int16_t>(3);
        fakeMutTx3.nType = static_cast<int16_t>(TRANSACTION_NF_TOKEN_REGISTER);
        fakeMutTx3.extraPayload = {'x', 'y', 'z'};

        CTransaction fakeTx3(fakeMutTx3);

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken3, fakeTx3, m_fakeBlockIdx.get()));

        BOOST_CHECK_EQUAL(2, m_nfTokensManager.BalanceOf(25, nfToken.tokenOwnerKeyId));
        BOOST_CHECK_EQUAL(1, m_nfTokensManager.BalanceOf(116546815, nfToken.tokenOwnerKeyId));
        BOOST_CHECK_EQUAL(3, m_nfTokensManager.BalanceOf(nfToken.tokenOwnerKeyId));

        protocolOwnerIds = m_nfTokensManager.NfTokenIdsOf(25, nfToken.tokenOwnerKeyId);
        protocolOwnerIds2 = m_nfTokensManager.NfTokenIdsOf(116546815, nfToken.tokenOwnerKeyId);
        ownerIds = m_nfTokensManager.NfTokenIdsOf(nfToken.tokenOwnerKeyId);

        BOOST_CHECK_EQUAL(2, protocolOwnerIds.size());
        BOOST_CHECK_EQUAL(1, protocolOwnerIds2.size());
        BOOST_CHECK_EQUAL(3, ownerIds.size());

        std::sort(protocolOwnerIds.begin(), protocolOwnerIds.end());
        BOOST_CHECK_EQUAL("5027a08676f392c326d4cb96c938a22e55c92a6c20ededc84a1f13fe3ba1b880", protocolOwnerIds[0].ToString());
        BOOST_CHECK_EQUAL("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1", protocolOwnerIds[1].ToString());
        BOOST_CHECK_EQUAL("ef51e0d401537dc8f3a3394f5fb992819d493a77bd20e7638dc24321bf4ecfdb", protocolOwnerIds2.front().ToString());

        std::sort(ownerIds.begin(), ownerIds.end());
        BOOST_CHECK_EQUAL("5027a08676f392c326d4cb96c938a22e55c92a6c20ededc84a1f13fe3ba1b880", ownerIds[0].ToString());
        BOOST_CHECK_EQUAL("ef51e0d401537dc8f3a3394f5fb992819d493a77bd20e7638dc24321bf4ecfdb", ownerIds[1].ToString());
        BOOST_CHECK_EQUAL("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1", ownerIds[2].ToString());
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_TotalSupply)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));

        BOOST_CHECK_EQUAL(1, m_nfTokensManager.TotalSupply());
        BOOST_CHECK_EQUAL(1, m_nfTokensManager.TotalSupply(25));

        Platform::NfToken nfToken2;
        nfToken2.tokenProtocolId = 116546815;
        ownerAddress.GetKeyID(nfToken2.tokenOwnerKeyId);
        adminAddress.GetKeyID(nfToken2.metadataAdminKeyId);
        nfToken2.tokenId.SetHex("ef51e0d401537dc8f3a3394f5fb992819d493a77bd20e7638dc24321bf4ecfdb");
        std::string metadataStr2("another prediction");
        nfToken2.metadata.assign(metadataStr2.begin(), metadataStr2.end());

        CMutableTransaction fakeMutTx2;
        fakeMutTx2.nVersion = static_cast<int16_t>(3);
        fakeMutTx2.nType = static_cast<int16_t>(TRANSACTION_NF_TOKEN_REGISTER);
        fakeMutTx2.extraPayload = {'d', 'e', 'f'};

        CTransaction fakeTx2(fakeMutTx2);

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken2, fakeTx2, m_fakeBlockIdx.get()));

        BOOST_CHECK_EQUAL(2, m_nfTokensManager.TotalSupply());
        BOOST_CHECK_EQUAL(1, m_nfTokensManager.TotalSupply(25));
        BOOST_CHECK_EQUAL(1, m_nfTokensManager.TotalSupply(116546815));

        Platform::NfToken nfToken3;
        nfToken3.tokenProtocolId = 25;
        ownerAddress.GetKeyID(nfToken3.tokenOwnerKeyId);
        adminAddress.GetKeyID(nfToken3.metadataAdminKeyId);
        nfToken3.tokenId.SetHex("5027a08676f392c326d4cb96c938a22e55c92a6c20ededc84a1f13fe3ba1b880");
        std::string metadataStr3("another prediction 2");
        nfToken3.metadata.assign(metadataStr3.begin(), metadataStr3.end());

        CMutableTransaction fakeMutTx3;
        fakeMutTx3.nVersion = static_cast<int16_t>(3);
        fakeMutTx3.nType = static_cast<int16_t>(TRANSACTION_NF_TOKEN_REGISTER);
        fakeMutTx3.extraPayload = {'x', 'y', 'z'};

        CTransaction fakeTx3(fakeMutTx3);

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken3, fakeTx3, m_fakeBlockIdx.get()));

        BOOST_CHECK_EQUAL(3, m_nfTokensManager.TotalSupply());
        BOOST_CHECK_EQUAL(2, m_nfTokensManager.TotalSupply(25));
        BOOST_CHECK_EQUAL(1, m_nfTokensManager.TotalSupply(116546815));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_Delete)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        BOOST_CHECK(m_nfTokensManager.Delete(25, registeredTokenId));

        BOOST_CHECK(!m_nfTokensManager.Contains(25, registeredTokenId));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_DeleteAtHeight)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        BOOST_CHECK(m_nfTokensManager.Delete(25, registeredTokenId, 101));

        BOOST_CHECK(!m_nfTokensManager.Contains(25, registeredTokenId));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_DeleteAtHeight_BeforeRegistration)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        BOOST_CHECK(!m_nfTokensManager.Delete(25, registeredTokenId, 50));

        BOOST_CHECK(m_nfTokensManager.Contains(25, registeredTokenId));
    }

    BOOST_AUTO_TEST_CASE(AddValidNfToken_DeleteAtHeight_AfterRegistration)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        BOOST_CHECK(m_nfTokensManager.Delete(25, registeredTokenId, 150));

        BOOST_CHECK(!m_nfTokensManager.Contains(25, registeredTokenId));
    }

    BOOST_AUTO_TEST_CASE(AddDupNfToken)
    {
        Platform::NfToken nfToken;
        nfToken.tokenProtocolId = 25;
        CBitcoinAddress ownerAddress("CRWYEwUogioqsQhNrGfj17Y2zNpcmcr8CKS6");
        ownerAddress.GetKeyID(nfToken.tokenOwnerKeyId);
        CBitcoinAddress adminAddress("CRWZLkZ5eYJ2cASp8iVgo1kLGCn3qxzLemxT");
        adminAddress.GetKeyID(nfToken.metadataAdminKeyId);
        nfToken.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr("btc 2025 prediction");
        nfToken.metadata.assign(metadataStr.begin(), metadataStr.end());

        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken, *m_fakeTx, m_fakeBlockIdx.get()));
        uint256 registeredTokenId;
        registeredTokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");

        BOOST_CHECK(m_nfTokensManager.Contains(25, registeredTokenId));


        Platform::NfToken nfToken2;
        nfToken2.tokenProtocolId = 116546815;
        ownerAddress.GetKeyID(nfToken2.tokenOwnerKeyId);
        adminAddress.GetKeyID(nfToken2.metadataAdminKeyId);
        nfToken2.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr2("same btc 2025 prediction for new proto");
        nfToken2.metadata.assign(metadataStr2.begin(), metadataStr2.end());

        CMutableTransaction fakeMutTx2;
        fakeMutTx2.nVersion = static_cast<int16_t>(3);
        fakeMutTx2.nType = static_cast<int16_t>(TRANSACTION_NF_TOKEN_REGISTER);
        fakeMutTx2.extraPayload = {'d', 'e', 'f'};

        CTransaction fakeTx2(fakeMutTx2);

        // Add same nft id to another protocol -> success
        BOOST_CHECK(m_nfTokensManager.AddNfToken(nfToken2, fakeTx2, m_fakeBlockIdx.get()));


        Platform::NfToken nfToken3;
        nfToken3.tokenProtocolId = 25;
        ownerAddress.GetKeyID(nfToken3.tokenOwnerKeyId);
        adminAddress.GetKeyID(nfToken3.metadataAdminKeyId);
        nfToken3.tokenId.SetHex("4e087c9d24b23910e403eda9c731c5796305721701d568601cb70b9071fd8fe1");
        std::string metadataStr3("same btc 2025 prediction");
        nfToken3.metadata.assign(metadataStr3.begin(), metadataStr3.end());

        CMutableTransaction fakeMutTx3;
        fakeMutTx3.nVersion = static_cast<int16_t>(3);
        fakeMutTx3.nType = static_cast<int16_t>(TRANSACTION_NF_TOKEN_REGISTER);
        fakeMutTx3.extraPayload = {'x', 'y', 'z'};

        CTransaction fakeTx3(fakeMutTx3);

        // Add same nft id to the same protocol -> failure
        BOOST_CHECK(!m_nfTokensManager.AddNfToken(nfToken3, fakeTx3, m_fakeBlockIdx.get()));
    }

BOOST_AUTO_TEST_SUITE_END()