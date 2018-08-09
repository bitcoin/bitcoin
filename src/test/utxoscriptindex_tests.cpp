#include <index/utxoscriptindex.h>
#include <validation.h>
#include <txmempool.h>
#include <consensus/validation.h>

#include <boost/test/unit_test.hpp>
#include <test/test_bitcoin.h>

BOOST_AUTO_TEST_SUITE(utxoscriptindex_tests)

CMutableTransaction createSimpleTx(const uint256& prevHash, const CScript& script, CKey& key)
{
   CMutableTransaction spend_tx;
    spend_tx.nVersion = 1;
    spend_tx.vin.resize(1);
    spend_tx.vin[0].prevout.hash = prevHash;
    spend_tx.vin[0].prevout.n = 0;
    spend_tx.vout.resize(1);
    spend_tx.vout[0].nValue = 11*CENT;
    spend_tx.vout[0].scriptPubKey = script;

    std::vector<unsigned char> vchSig;
    uint256 hash = SignatureHash(script, spend_tx, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_CHECK(key.Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    spend_tx.vin[0].scriptSig << vchSig;

   return spend_tx;
}

CBlockUndo createBlockUndo(const CScript& script)
{
   CBlockUndo bu = CBlockUndo();
   bu.vtxundo.emplace_back();
   bu.vtxundo[0].vprevout.emplace_back();
   bu.vtxundo[0].vprevout[0].out.scriptPubKey = script;

   return bu;
}

BOOST_FIXTURE_TEST_CASE(block_connected_adds_utxo_record, TestChain100Setup)
{
    UtxoScriptIndex utxoscriptindex(1 << 20, false, false);
    CScript scriptPubKey = GetScriptForDestination(coinbaseKey.GetPubKey().GetID());

    SerializableUtxoSet utxos = utxoscriptindex.getUtxosForScript(scriptPubKey, 1);
    BOOST_CHECK_EQUAL(utxos.size(), 0);

    CMutableTransaction spend_tx = createSimpleTx(m_coinbase_txns[0]->GetHash(), scriptPubKey, coinbaseKey);

    CBlock b = CreateAndProcessBlock({spend_tx}, scriptPubKey);
    CBlockUndo bu = createBlockUndo(scriptPubKey);

    utxoscriptindex.BlockConnected(std::make_shared<CBlock>(b),
                                   nullptr,
                                   std::make_shared<CBlockUndo>(bu),
                                   {});
    utxos = utxoscriptindex.getUtxosForScript(scriptPubKey, 1);

    BOOST_CHECK_EQUAL(utxos.size(), 2);
    BOOST_CHECK_EQUAL(b.vtx.size(), 2);

    std::set<std::string> blockTxHashes;
    std::set<std::string> utxoTxHashes;

    blockTxHashes.insert(b.vtx[0]->GetHash().ToString());
    blockTxHashes.insert(b.vtx[1]->GetHash().ToString());

    utxoTxHashes.insert(utxos.begin()->hash.ToString());
    utxoTxHashes.insert((++utxos.begin())->hash.ToString());

    BOOST_CHECK(blockTxHashes == utxoTxHashes);

    utxos = utxoscriptindex.getUtxosForScript(scriptPubKey, 0);
    BOOST_CHECK_EQUAL(utxos.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(no_record_after_connect_and_disconnect, TestChain100Setup)
{
    UtxoScriptIndex utxoscriptindex(1 << 20, false, false);
    CScript scriptPubKey = GetScriptForDestination(coinbaseKey.GetPubKey().GetID());

    CMutableTransaction spend_tx = createSimpleTx(m_coinbase_txns[0]->GetHash(), scriptPubKey, coinbaseKey);

    CBlock b = CreateAndProcessBlock({spend_tx}, scriptPubKey);

    BOOST_CHECK_EQUAL(b.vtx[0]->vout[1].nValue, 0);

    CBlockUndo bu = createBlockUndo(b.vtx[0]->vout[1].scriptPubKey);

    utxoscriptindex.BlockConnected(std::make_shared<CBlock>(b),
                            nullptr,
                            std::make_shared<CBlockUndo>(bu),
                            {});

    utxoscriptindex.BlockDisconnected(std::make_shared<CBlock>(b), std::make_shared<CBlockUndo>(bu));

    SerializableUtxoSet utxos = utxoscriptindex.getUtxosForScript(scriptPubKey, 1);
    BOOST_CHECK_EQUAL(utxos.size(), 0);

    utxos = utxoscriptindex.getUtxosForScript(scriptPubKey, 0);
    BOOST_CHECK_EQUAL(utxos.size(), 0);
}


BOOST_FIXTURE_TEST_CASE(new_mempool_tx_adds_record, TestChain100Setup)
{
    UtxoScriptIndex utxoscriptindex(1 << 20, false, false);
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    CMutableTransaction spend = createSimpleTx(m_coinbase_txns[0]->GetHash(), scriptPubKey, coinbaseKey);
    LOCK(cs_main);

    CValidationState state;
    BOOST_CHECK(AcceptToMemoryPool(mempool, state, MakeTransactionRef(spend), nullptr /* pfMissingInputs */, nullptr, true, 0));

    CTransactionRef ptxn = std::make_shared<CTransaction>(spend);
    utxoscriptindex.TransactionAddedToMempool(ptxn);

    SerializableUtxoSet utxos = utxoscriptindex.getUtxosForScript(scriptPubKey, 0);

    BOOST_CHECK_EQUAL(utxos.size(), 1);
    BOOST_CHECK_EQUAL(utxos.begin()->hash.ToString(), spend.GetHash().ToString());
}

BOOST_AUTO_TEST_SUITE_END()
