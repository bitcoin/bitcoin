// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "init.h"

#include "core.h"
#include "main.h"
#include "net.h"
#include "script.h"
#include "txdb.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#endif
//////////////////////////////////////////////////////////////////////////////
//
// BitcreditMiner
//

static const unsigned int pSHA256InitState[8] =
{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

static const unsigned int sha256DigestChunkByteSize = 64;
static const unsigned int sha256ResultByteSize = 32;
static const unsigned int alignmentExtraBytes = 16;

/**
 * Padding of passed in data with the padding format SHA256 expects. This is a leading "1", in hex 0x80
 * followed by padding zeros up until the last 4 bytes which is used to set the length of the full
 * message digest. If a full block of 64 bytes isn't passed to SHA256 hashing function, the data
 * is automatically padded.
 */
int static FormatHashBlocks(void* pbuffer, unsigned int len) {
    unsigned char* pdata = (unsigned char*)pbuffer;
    unsigned int numerOfShaChunks = 1 + ((len + 8) / sha256DigestChunkByteSize);
    unsigned char* pend = pdata + sha256DigestChunkByteSize * numerOfShaChunks;
    //Set everything after data to 0
    memset(pdata + len, 0, sha256DigestChunkByteSize * numerOfShaChunks - len);
    //Set to "1", in hex 0x80
    pdata[len] = 0x80;
    //Set length in last 4 bytes
    unsigned int bits = len * 8;
    pend[-1] = (bits >> 0) & 0xff;
    pend[-2] = (bits >> 8) & 0xff;
    pend[-3] = (bits >> 16) & 0xff;
    pend[-4] = (bits >> 24) & 0xff;
    return numerOfShaChunks;
}

class HeaderData {
public:
    int nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint256 hashLinkedBitcoinBlock;
    uint256 hashSigMerkleRoot;
    unsigned int nTime;
    unsigned int nBits;
    unsigned int nNonce;
    uint64_t nTotalMonetaryBase;
    uint64_t nTotalDepositBase;
    uint64_t nDepositAmount;

    int GetSizeOf() {
    	return sizeof(nVersion) +
    				sizeof(hashPrevBlock) +
    				sizeof(hashMerkleRoot) +
    				sizeof(hashLinkedBitcoinBlock) +
    				sizeof(hashSigMerkleRoot) +
    				sizeof(nTime) +
    				sizeof(nBits) +
    				sizeof(nNonce) +
    				sizeof(nTotalMonetaryBase) +
    				sizeof(nTotalDepositBase) +
    				sizeof(nDepositAmount);
    }
};

class MiningStructure {
public:
    HeaderData headerData;
    unsigned char pchPadding0[sha256DigestChunkByteSize];
    uint256 hash;
    unsigned char pchPadding1[sha256DigestChunkByteSize];

    void InitData(Credits_CBlock* pblock, unsigned int &shaChunksForHeader, unsigned int &shaChunksForHash) {
       headerData.nVersion       = pblock->nVersion;
       headerData.hashPrevBlock  = pblock->hashPrevBlock;
       headerData.hashMerkleRoot = pblock->hashMerkleRoot;
       headerData.hashLinkedBitcoinBlock         = pblock->hashLinkedBitcoinBlock;
       headerData.hashSigMerkleRoot         = pblock->hashSigMerkleRoot;
       headerData.nTime          = pblock->nTime;
       headerData.nBits          = pblock->nBits;
       headerData.nNonce         = pblock->nNonce;
       headerData.nTotalMonetaryBase         = pblock->nTotalMonetaryBase;
       headerData.nTotalDepositBase         = pblock->nTotalDepositBase;
       headerData.nDepositAmount         = pblock->nDepositAmount;

        shaChunksForHeader = FormatHashBlocks(&headerData, headerData.GetSizeOf());
        shaChunksForHash = FormatHashBlocks(&hash, sizeof(hash));
    }
};

/**
 * This function first byte reverses the data before it is transformed
 * with SHA-256.
 */
void SHA256Transform(void* pstate, void* pinput, const void* pinit)
{
    SHA256_CTX ctx;
    unsigned char data[sha256DigestChunkByteSize];

    SHA256_Init(&ctx);

    for (int i = 0; i < 16; i++)
        ((uint32_t*)data)[i] = ByteReverse(((uint32_t*)pinput)[i]);

    for (int i = 0; i < 8; i++)
        ctx.h[i] = ((uint32_t*)pinit)[i];

    SHA256_Update(&ctx, data, sizeof(data));
    for (int i = 0; i < 8; i++)
        ((uint32_t*)pstate)[i] = ctx.h[i];
}

// Some explaining would be appreciated
class COrphan
{
public:
    const Credits_CTransaction* ptx;
    set<uint256> setDependsOn;
    double dPriority;
    double dFeePerKb;

    COrphan(const Credits_CTransaction* ptxIn)
    {
        ptx = ptxIn;
        dPriority = dFeePerKb = 0;
    }

    void print() const
    {
        LogPrintf("COrphan(hash=%s, dPriority=%.1f, dFeePerKb=%.1f)\n",
               ptx->GetHash().ToString(), dPriority, dFeePerKb);
        BOOST_FOREACH(uint256 hash, setDependsOn)
            LogPrintf("   setDependsOn %s\n", hash.ToString());
    }
};


uint64_t bitcredit_nLastBlockTx = 0;
uint64_t bitcredit_nLastBlockSize = 0;

// We want to sort transactions by priority and fee, so:
typedef boost::tuple<double, double, const Credits_CTransaction*> TxPriority;
class TxPriorityCompare
{
    bool byFee;
public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) { }
    bool operator()(const TxPriority& a, const TxPriority& b)
    {
        if (byFee)
        {
            if (a.get<1>() == b.get<1>())
                return a.get<0>() < b.get<0>();
            return a.get<1>() < b.get<1>();
        }
        else
        {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

bool VerifyDepositSignatures (std::string prefix, Credits_CBlock *pblock) {
	const COutPoint coinbaseOutPoint(pblock->vtx[0].GetHash(), 0);

    //Test the newly created signature
    unsigned int flags = SCRIPT_VERIFY_NOCACHE | SCRIPT_VERIFY_P2SH;

	for (unsigned int i = 1; i < pblock->vtx.size(); i++) {
		if (pblock->vtx[i].IsDeposit()) {
			Credits_CTransaction &txDeposit = pblock->vtx[i];
			Credits_CTxIn & txDepositIn = txDeposit.vin[0];
			const CScript &scriptSig = txDepositIn.scriptSig;

			//If coinbase is referenced, find that for validation, otherwise look in bitcredit_coins
			const CScript * scriptPubKey = NULL;
			if(txDepositIn.prevout == coinbaseOutPoint) {
		        scriptPubKey = &pblock->vtx[0].vout[0].scriptPubKey;
			} else {
				const Credits_CCoins &coinsSpent = bitcredit_pcoinsTip->Credits_GetCoins(txDepositIn.prevout.hash);
				assert(coinsSpent.IsAvailable(txDepositIn.prevout.n));
				scriptPubKey = &coinsSpent.vout[txDepositIn.prevout.n].scriptPubKey;
			}

			if (!Bitcredit_VerifyScript(scriptSig, *scriptPubKey, txDeposit, 0, flags, 0)) {
				LogPrintf("\n\n%s: ERROR: Verification of signature failed!!!!!\n\n", prefix);
				return false;
			}
		} else {
			break;
		}
	}

    LogPrintf("%s: Verification of signature OK\n", prefix);
    return true;
}

bool RecalculateCoinbaseDeposit(Credits_CBlock *pblock, const uint256 &oldCoinBaseHash, CKeyStore * bitcreditKeystore, CKeyStore * depositKeystore, const bool& coinbaseDepositDisabled) {
	const Credits_CTransaction& txCoinbase = pblock->vtx[0];

    CKeyStore * tmpKeyStore = bitcreditKeystore;
    if(bitcredit_pwalletMain->IsLocked() && !coinbaseDepositDisabled) {
    	tmpKeyStore = depositKeystore;
    }

	for (unsigned int i = 1; i < pblock->vtx.size(); i++) {
		if (pblock->vtx[i].IsDeposit()) {
			Credits_CTransaction &txDeposit = pblock->vtx[i];
			Credits_CTxIn & txDepositIn = txDeposit.vin[0];

			if(txDepositIn.prevout.hash == oldCoinBaseHash) {
				txDepositIn.prevout = COutPoint(txCoinbase.GetHash(), 0);

			    if (!Credits_SignSignature(*tmpKeyStore, txCoinbase, txDeposit, 0)) {
			        LogPrintf("ERROR: deposit transaction could not sign coinbase\n");
			        return false;
			    }
			}
		}
	}

    return true;
}

struct Miner_CompareValueOnly
{
    bool operator()(const pair<int64_t, const Credits_CTransaction*>& t1,
                    const pair<int64_t, const Credits_CTransaction*>& t2) const
    {
        return t1.first < t2.first;
    }
};

static void ApproximateBestSubset(vector<pair<int64_t, const Credits_CTransaction*> >vValue,  int64_t nTotalLower, int64_t nTargetValue,
                                  vector<char>& vfBest, int64_t& nBest, int iterations = 1000)
{
    vector<char> vfIncluded;

    vfBest.assign(vValue.size(), true);
    nBest = nTotalLower;

    seed_insecure_rand();

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        int64_t nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < vValue.size(); i++)
            {
                //The solver here uses a randomized algorithm,
                //the randomness serves no real security purpose but is just
                //needed to prevent degenerate behavior and it is important
                //that the rng fast. We do not use a constant random sequence,
                //because there may be some privacy improvement by making
                //the selection random.
                if (nPass == 0 ? insecure_rand()&1 : !vfIncluded[i])
                {
                    nTotal += vValue[i].first;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= vValue[i].first;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

bool SelectDepositTxs(int64_t nTargetValue, vector<Credits_CTransaction> &vCoins,
		set<const Credits_CTransaction*>& setCoinsRet)
{
    setCoinsRet.clear();

    // The value closest to the target value but larger
    pair<int64_t, const Credits_CTransaction*> coinLowestLarger;
    coinLowestLarger.first = std::numeric_limits<int64_t>::max();
    coinLowestLarger.second = NULL;
    // List of values less than target
    vector<pair<int64_t, const Credits_CTransaction*> > vValue;
    int64_t nTotalLower = 0;

    random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

    for(unsigned int i= 0; i < vCoins.size();i++)
    {
        const int64_t n = vCoins[i].GetDepositValueOut();

        pair<int64_t,const Credits_CTransaction*> coin = make_pair(n,&vCoins[i]);

        if (n == nTargetValue)
        {
            setCoinsRet.insert(coin.second);
            return true;
        }
        else if (n < nTargetValue + CENT)
        {
            vValue.push_back(coin);
            nTotalLower += n;
        }
        else if (n < coinLowestLarger.first)
        {
            coinLowestLarger = coin;
        }
    }

    if (nTotalLower == nTargetValue)
    {
        for (unsigned int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.insert(vValue[i].second);
        }
        return true;
    }

    if (nTotalLower < nTargetValue)
    {
        if (coinLowestLarger.second == NULL)
            return false;
        setCoinsRet.insert(coinLowestLarger.second);

        return true;
    }

    // Solve subset sum by stochastic approximation
    sort(vValue.rbegin(), vValue.rend(), Miner_CompareValueOnly());
    vector<char> vfBest;
    int64_t nBest;

    ApproximateBestSubset(vValue, nTotalLower, nTargetValue, vfBest, nBest, 1000);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + CENT)
    	ApproximateBestSubset(vValue, nTotalLower, nTargetValue + CENT, vfBest, nBest, 1000);

    // If we have a bigger coin and (either the stochastic approximation didn't find a good solution,
    //                                   or the next bigger coin is closer), return the bigger coin
    if (coinLowestLarger.second &&
        ((nBest != nTargetValue && nBest < nTargetValue + CENT) || coinLowestLarger.first <= nBest))
    {
        setCoinsRet.insert(coinLowestLarger.second);
    }
    else {
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
            {
                setCoinsRet.insert(vValue[i].second);
            }

        LogPrintf("SelectDepositTxs() best subset: ");
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                LogPrintf("%s ", FormatMoney(vValue[i].first));
        LogPrintf("total %s\n", FormatMoney(nBest));
    }

    return true;
}

void SelectLargestDepositTxs(int64_t nTargetValue, vector<Credits_CTransaction> &vCoins, set<const Credits_CTransaction*>& setCoinsRet, unsigned int nMaxTxs) {
    setCoinsRet.clear();

    vector<pair<int64_t, const Credits_CTransaction*> > vValue;
    for(unsigned int i= 0; i < vCoins.size();i++) {
        const int64_t n = vCoins[i].GetDepositValueOut();
        pair<int64_t,const Credits_CTransaction*> coin = make_pair(n,&vCoins[i]);
		vValue.push_back(coin);
    }

    sort(vValue.rbegin(), vValue.rend(), Miner_CompareValueOnly());
    //Check to see that sorting is done as we assume. This can be removed once test is written.
    if(vValue.size() > 1) {
    	assert(vValue[0].first >= vValue[1].first);
    }

    int64_t totalDeposit = 0;
    unsigned int i = 0;
    while(setCoinsRet.size() < nMaxTxs && i < vValue.size()) {
		setCoinsRet.insert(vValue[i].second);

		totalDeposit += vValue[i].first;
		if(totalDeposit >= nTargetValue) {
			break;
		}
		i++;
    }
}

Credits_CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyCoinbase, const CScript& scriptPubKeyDeposit, const CScript& scriptPubKeyDepositChange, CPubKey& pubKeySigningDeposit, CKeyStore * keystore, Credits_CWallet* pdepositWallet, bool coinbaseDepositDisabled)
{
    // Create new block
    auto_ptr<Credits_CBlockTemplate> pblocktemplate(new Credits_CBlockTemplate());
    if(!pblocktemplate.get()) {
        LogPrintf("\n\nERROR! Could not allocate new block template.\n\n");
    	return NULL;
    }
    Credits_CBlock *pblock = &pblocktemplate->block; // pointer for convenience

    // Create coinbase tx
    Credits_CTransaction txCoinbaseNew(TX_TYPE_COINBASE);
    txCoinbaseNew.vout.resize(1);
    txCoinbaseNew.vout[0].scriptPubKey = scriptPubKeyCoinbase;
    txCoinbaseNew.vin.resize(1);
    txCoinbaseNew.vin[0].prevout.SetNull();
    // Add our coinbase tx as first transaction
    pblock->vtx.push_back(txCoinbaseNew);
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOps.push_back(-1); // updated at end

    // Largest block you're willing to create:
    unsigned int nBlockMaxSize = GetArg("-blockmaxsize", BITCREDIT_DEFAULT_BLOCK_MAX_SIZE);
    // Limit to betweeen 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max((unsigned int)1000, std::min((unsigned int)(BITCREDIT_MAX_BLOCK_SIZE-1000), nBlockMaxSize));

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    unsigned int nBlockPrioritySize = GetArg("-blockprioritysize", BITCREDIT_DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    unsigned int nBlockMinSize = GetArg("-blockminsize", BITCREDIT_DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize = std::min(nBlockMaxSize, nBlockMinSize);

    // Collect memory pool transactions into the block
    int64_t nFees = 0;
    {
        LOCK2(credits_mainState.cs_main, credits_mempool.cs);
        Credits_CBlockIndex* pindexPrev = (Credits_CBlockIndex*)credits_chainActive.Tip();

        //Verify linked block
        Bitcoin_CBlockIndex* pprevLinkedBlock;
        int nPrevLinkedHeight;
        {
        LOCK(bitcoin_mainState.cs_main);
        const map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.find(pindexPrev->hashLinkedBitcoinBlock);
        if (mi == bitcoin_mapBlockIndex.end()) {
            LogPrintf("\n\nERROR! Linked bitcoin block %s not found in bitcoin blockchain!\n\n", pindexPrev->hashLinkedBitcoinBlock.GetHex());
            return NULL;
        }
        pprevLinkedBlock = (*mi).second;
        nPrevLinkedHeight = pprevLinkedBlock->nHeight;
        }

        //Set up the new reference to the ->bitcoin<- blockchain
        //Make sure that we are referencing a block deep enough. When the acceptance of credits blocks is done we look
        //10000 blocks down. Therefore we set our reference to 10000  + 1000 + 144.
        //144 is the approximate number of blocks in 24 hours and also the "unknown"
        //number of blocks that may cause the function Bitcoin_IsInitialBlockDownload to be false.
        //Bitcoin_IsInitialBlockDownload is used as the general  function indicating that the downloaded bitcoin blockchain
        //is ready to be used by the credits system.
        {
        LOCK(bitcoin_mainState.cs_main);
        const Bitcoin_CBlockIndex * activeBlockAtHeight = bitcoin_chainActive[nPrevLinkedHeight];
        if(*activeBlockAtHeight->phashBlock != *pprevLinkedBlock->phashBlock) {
            LogPrintf("\n\nERROR! Referenced bitcoin block is not the same as block in active chain. Active chain has probably changed. Hashes are\n%s\n%s \n\n", activeBlockAtHeight->phashBlock->GetHex(), pprevLinkedBlock->phashBlock->GetHex());
            return NULL;
        }
        int acceptDepth = Bitcredit_Params().AcceptDepthLinkedBitcoinBlock();
        int nAcceptHeight = bitcoin_chainActive.Tip()->nHeight - (acceptDepth + (acceptDepth / 10)+ 24*6);
        if(nAcceptHeight < 0) {
    	    nAcceptHeight = 0;
        }
        if(nAcceptHeight < nPrevLinkedHeight) {
        	nAcceptHeight = nPrevLinkedHeight;
        }
        const Bitcoin_CBlockIndex * hashLinkedBitcoinBlockIndex = bitcoin_chainActive[nAcceptHeight];

        Credits_CCoinsViewCache credits_view(*bitcredit_pcoinsTip, true);

        const boost::filesystem::path tmpClaimDirPath = GetDataDir() / ".tmp" / Bitcoin_GetNewClaimTmpDbId();
        {
			//This is a throw away chainstate db + undo vector
        	std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > vBlockUndoClaims;
			Bitcoin_CClaimCoinsViewDB bitcoin_pclaimcoinsdbviewtmp(GetDataDir() / ".tmp" / Bitcoin_GetNewClaimTmpDbId(), Bitcoin_GetCoinDBCacheSize(), false, true, false, true);
			Bitcoin_CClaimCoinsViewCache claim_viewtmp(*bitcoin_pclaimCoinsTip, bitcoin_pclaimcoinsdbviewtmp, bitcoin_nClaimCoinCacheFlushSize, true);

			//Pre-setup hashLinkedBitcoinBlock to be able to use it with indexDummy
			pblock->hashLinkedBitcoinBlock  = *hashLinkedBitcoinBlockIndex->phashBlock;
	        Credits_CBlockIndex indexDummy(*pblock);
	        indexDummy.pprev = pindexPrev;
	        indexDummy.nHeight = pindexPrev->nHeight + 1;

			CValidationState state;
			if(!Bitcoin_AlignClaimTip(pindexPrev, &indexDummy, claim_viewtmp, state, false, vBlockUndoClaims)) {
				LogPrintf("\n\nERROR: Miner: Bitcoin_AlignClaimTip: Failed to move claim tip to %s\n", hashLinkedBitcoinBlockIndex->GetBlockHash().GetHex());
				return NULL;
			}

			// Priority order to process transactions
			list<COrphan> vOrphan; // list memory doesn't move
			map<uint256, vector<COrphan*> > mapDependers;
			bool fPrintPriority = GetBoolArg("-printpriority", false);

			// This vector will be sorted into a priority queue:
			vector<TxPriority> vecPriority;
			vecPriority.reserve(credits_mempool.mapTx.size());
			for (map<uint256, Bitcredit_CTxMemPoolEntry>::iterator mi = credits_mempool.mapTx.begin();
				 mi != credits_mempool.mapTx.end(); ++mi)
			{
				const Credits_CTransaction& tx = mi->second.GetTx();
				if (tx.IsCoinBase() || tx.IsDeposit() || !Credits_IsFinalTx(tx, pindexPrev->nHeight + 1))
					continue;

				if(tx.IsStandard()) {
					COrphan* porphan = NULL;
					double dPriority = 0;
					int64_t nTotalIn = 0;
					bool fMissingInputs = false;
					BOOST_FOREACH(const Credits_CTxIn& txin, tx.vin)
					{
						// Read prev transaction
						if (!credits_view.Credits_HaveCoins(txin.prevout.hash))
						{
							// This should never happen; all transactions in the memory
							// pool should connect to either transactions in the chain
							// or other transactions in the memory pool.
							if (!credits_mempool.mapTx.count(txin.prevout.hash))
							{
								LogPrintf("ERROR: mempool transaction missing input\n");
								if (fDebug) assert("mempool transaction missing input" == 0);
								fMissingInputs = true;
								if (porphan)
									vOrphan.pop_back();
								break;
							}

							// Has to wait for dependencies
							if (!porphan)
							{
								// Use list for automatic deletion
								vOrphan.push_back(COrphan(&tx));
								porphan = &vOrphan.back();
							}
							mapDependers[txin.prevout.hash].push_back(porphan);
							porphan->setDependsOn.insert(txin.prevout.hash);
							nTotalIn += credits_mempool.mapTx[txin.prevout.hash].GetTx().vout[txin.prevout.n].nValue;
							continue;
						}
						const Credits_CCoins &coins = credits_view.Credits_GetCoins(txin.prevout.hash);

						int64_t nValueIn = coins.vout[txin.prevout.n].nValue;
						nTotalIn += nValueIn;

						const int nConf = pindexPrev->nHeight - coins.nHeight + 1;
						dPriority += (double)nValueIn * nConf;
					}
					if (fMissingInputs) continue;

					// Priority is sum(valuein * age) / modified_txsize
					unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, CREDITS_PROTOCOL_VERSION);
					dPriority = tx.ComputePriority(dPriority, nTxSize);

					// This is a more accurate fee-per-kilobyte than is used by the client code, because the
					// client code rounds up the size to the nearest 1K. That's good, because it gives an
					// incentive to create smaller transactions.
					double dFeePerKb =  double(nTotalIn-tx.GetValueOut()) / (double(nTxSize)/1000.0);

					if (porphan)
					{
						porphan->dPriority = dPriority;
						porphan->dFeePerKb = dFeePerKb;
					}
					else
						vecPriority.push_back(TxPriority(dPriority, dFeePerKb, &mi->second.GetTx()));
				} else if(tx.IsClaim()) {
					//COrphan* porphan = NULL;
					double dPriority = 0;
					int64_t nTotalIn = 0;
					bool fMissingInputs = false;
					BOOST_FOREACH(const Credits_CTxIn& txin, tx.vin)
					{
						// Read prev transaction
						if (!claim_viewtmp.Claim_HaveCoins(txin.prevout.hash))
						{
							// This should never happen; all transactions in the memory
							// pool should connect to either transactions in the chain
							// or other transactions in the memory pool.
	//						if (!bitcoin_mempool.mapTx.count(txin.prevout.hash))
	//						{
								LogPrintf("ERROR: transaction missing input\n");
								if (fDebug) assert("transaction missing input" == 0);
								fMissingInputs = true;
	//							if (porphan)
	//								vOrphan.pop_back();
								break;
	//						}
	//
	//						// Has to wait for dependencies
	//						if (!porphan)
	//						{
	//							// Use list for automatic deletion
	//							vOrphan.push_back(COrphan(&tx));
	//							porphan = &vOrphan.back();
	//						}
	//						mapDependers[txin.prevout.hash].push_back(porphan);
	//						porphan->setDependsOn.insert(txin.prevout.hash);
	//						nTotalIn += bitcoin_mempool.mapTx[txin.prevout.hash].GetTx().vout[txin.prevout.n].nValue;
	//						continue;
						}

						const Bitcoin_CClaimCoins &coins = claim_viewtmp.Claim_GetCoins(txin.prevout.hash);

						if(!coins.HasClaimable(txin.prevout.n)) {
							LogPrintf("ERROR: transaction missing input\n");
							if (fDebug) assert("transaction missing input" == 0);
							fMissingInputs = true;
	//							if (porphan)
	//								vOrphan.pop_back();
							break;
						}

						int64_t nValueIn = coins.vout[txin.prevout.n].nValueClaimable;
						nTotalIn += nValueIn;

						int nConf = nPrevLinkedHeight - coins.nHeight + 1;
						dPriority += (double)nValueIn * nConf;
					}
					if (fMissingInputs) continue;

					// Priority is sum(valuein * age) / modified_txsize
					unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, CREDITS_PROTOCOL_VERSION);
					dPriority = tx.ComputePriority(dPriority, nTxSize);

					// This is a more accurate fee-per-kilobyte than is used by the client code, because the
					// client code rounds up the size to the nearest 1K. That's good, because it gives an
					// incentive to create smaller transactions.
					double dFeePerKb =  double(nTotalIn-tx.GetValueOut()) / (double(nTxSize)/1000.0);

	//				if (porphan)
	//				{
	//					porphan->dPriority = dPriority;
	//					porphan->dFeePerKb = dFeePerKb;
	//				}
	//				else
						vecPriority.push_back(TxPriority(dPriority, dFeePerKb, &mi->second.GetTx()));
				}
			}

			//Setup the block used for updating the deposit base due to long enough block chain
			Credits_CBlockIndex* pBlockToTrim = NULL;
			Credits_CBlock trimBlock;
			unsigned int currentHeight = pindexPrev->nHeight + 1 ;
			if(currentHeight > BITCREDIT_DEPOSIT_CUTOFF_DEPTH) {
				//Find trim block in active chain
				pBlockToTrim = credits_chainActive[currentHeight - BITCREDIT_DEPOSIT_CUTOFF_DEPTH - 1];
				// Read block from disk.
				if (!Credits_ReadBlockFromDisk(trimBlock, pBlockToTrim)) {
					LogPrintf("Failed to read trim block");
					return NULL;
				}
			}
			//These two updates the deposit base
			int64_t nResurrectedDepositBase = 0;
			int64_t nTrimmedDepositBase = 0;

			// Collect transactions into block
			uint64_t nBlockSize = 1000;
			uint64_t nBlockTx = 0;
			int nBlockSigOps = 100;
			bool fSortedByFee = (nBlockPrioritySize <= 0);

			TxPriorityCompare comparer(fSortedByFee);
			std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);

			int64_t nClaimedCoinsForBlock = 0;
			while (!vecPriority.empty())
			{
				// Take highest priority transaction off the priority queue:
				double dPriority = vecPriority.front().get<0>();
				double dFeePerKb = vecPriority.front().get<1>();
				const Credits_CTransaction& tx = *(vecPriority.front().get<2>());

				std::pop_heap(vecPriority.begin(), vecPriority.end(), comparer);
				vecPriority.pop_back();

				// Size limits
				unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, CREDITS_PROTOCOL_VERSION);
				if (nBlockSize + nTxSize >= nBlockMaxSize)
					continue;

				// Legacy limits on sigOps:
				unsigned int nTxSigOps = Bitcredit_GetLegacySigOpCount(tx);
				if (nBlockSigOps + nTxSigOps >= BITCREDIT_MAX_BLOCK_SIGOPS)
					continue;

				// Skip free transactions if we're past the minimum block size:
				if (fSortedByFee && (dFeePerKb < Credits_CTransaction::nMinRelayTxFee) && (nBlockSize + nTxSize >= nBlockMinSize))
					continue;

				// Prioritize by fee once past the priority size or we run out of high-priority
				// transactions:
				if (!fSortedByFee &&
					((nBlockSize + nTxSize >= nBlockPrioritySize) || !Credits_AllowFree(dPriority)))
				{
					fSortedByFee = true;
					comparer = TxPriorityCompare(fSortedByFee);
					std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);
				}

				int64_t nTxFees = 0;
				if(tx.IsStandard()) {
					if (!credits_view.Credits_HaveInputs(tx))
						continue;

					nTxFees = credits_view.Credits_GetValueIn(tx)-tx.GetValueOut();
				} else if(tx.IsClaim()) {
					if (!claim_viewtmp.Claim_HaveInputs(tx))
						continue;

					nTxFees = claim_viewtmp.Claim_GetValueIn(tx)-tx.GetValueOut();
				}

				nTxSigOps += Bitcredit_GetP2SHSigOpCount(tx, credits_view, claim_viewtmp);
				if (nBlockSigOps + nTxSigOps >= BITCREDIT_MAX_BLOCK_SIGOPS)
					continue;

				int64_t nClaimedCoins = 0;
				// Note that flags: we don't want to set mempool/IsStandard()
				// policy here, but we still have to ensure that the block we
				// create only contains transactions that are valid in new blocks.
				CValidationState state;
				if (!Credits_CheckInputs(tx, state, credits_view, claim_viewtmp, nClaimedCoins, true, MANDATORY_SCRIPT_VERIFY_FLAGS))
					continue;

				if(!Bitcredit_CheckClaimsAreInBounds(claim_viewtmp, nClaimedCoinsForBlock + nClaimedCoins, nAcceptHeight)) {
					continue;
				}
				nClaimedCoinsForBlock += nClaimedCoins;

				UpdateResurrectedDepositBase(pBlockToTrim, tx, nResurrectedDepositBase, credits_view);

				Credits_CTxUndo txundoUnused;
				uint256 hash = tx.GetHash();
				Bitcredit_UpdateCoins(tx, state, credits_view, claim_viewtmp, txundoUnused, pindexPrev->nHeight+1, hash);

				// Added
				pblock->vtx.push_back(tx);
				pblocktemplate->vTxFees.push_back(nTxFees);
				pblocktemplate->vTxSigOps.push_back(nTxSigOps);
				nBlockSize += nTxSize;
				++nBlockTx;
				nBlockSigOps += nTxSigOps;
				nFees += nTxFees;

				if (fPrintPriority)
				{
					LogPrintf("priority %.1f feeperkb %.1f txid %s\n",
						   dPriority, dFeePerKb, tx.GetHash().ToString());
				}

				// Add transactions that depend on this one to the priority queue
				if (mapDependers.count(hash))
				{
					BOOST_FOREACH(COrphan* porphan, mapDependers[hash])
					{
						if (!porphan->setDependsOn.empty())
						{
							porphan->setDependsOn.erase(hash);
							if (porphan->setDependsOn.empty())
							{
								vecPriority.push_back(TxPriority(porphan->dPriority, porphan->dFeePerKb, porphan->ptx));
								std::push_heap(vecPriority.begin(), vecPriority.end(), comparer);
							}
						}
					}
				}
			}

			UpdateTrimmedDepositBase(pBlockToTrim, trimBlock, nTrimmedDepositBase, credits_view);

			//These values do not include coinbase and deposit tx. Should they?
			bitcredit_nLastBlockTx = nBlockTx;
			bitcredit_nLastBlockSize = nBlockSize;
			LogPrintf("CreateNewBlock(): total size %u\n", nBlockSize);

			const uint64_t maxBlockSubsidyIncFee = Bitcredit_GetMaxBlockSubsidy(pindexPrev->nTotalMonetaryBase) + nFees;
			pblock->vtx[0].vout[0].nValue = maxBlockSubsidyIncFee;

			const uint64_t requiredDepositLevel = Bitcredit_GetRequiredDeposit(pindexPrev->nTotalDepositBase);

			if(!coinbaseDepositDisabled) {
				uint256 tmpCoinbaseHash(0);
				// Create deposit tx
				const int64_t nSubsidy = pblock->vtx[0].vout[0].nValue;
				Credits_CTransaction txDeposit(TX_TYPE_DEPOSIT);
				txDeposit.vout.resize(1);
				txDeposit.vout[0].scriptPubKey = scriptPubKeyDeposit;
				txDeposit.vout[0].nValue = nSubsidy;
				txDeposit.vin.resize(1);
				txDeposit.vin[0] = Credits_CTxIn(COutPoint(tmpCoinbaseHash, 0));
				txDeposit.signingKeyId = pubKeySigningDeposit.GetID();
				// Add our deposit tx as second transaction
				pblock->vtx.insert(pblock->vtx.begin() + 1, txDeposit);
				pblocktemplate->vTxFees.insert(pblocktemplate->vTxFees.begin()+1, 0); // Deposits can not have tx fees
				pblocktemplate->vTxSigOps.insert(pblocktemplate->vTxSigOps.begin()+1, -1); // updated at end
				if(!RecalculateCoinbaseDeposit(pblock, tmpCoinbaseHash, keystore, pdepositWallet, coinbaseDepositDisabled)) {
		            LogPrintf("\n\nERROR! Coinbase deposit could not be recalcualted\n\n");
					return NULL;
				}
				if(!VerifyDepositSignatures("CreateNewBlock", pblock)) {
		            LogPrintf("\n\nERROR! Deposit signatures could not be verified.\n\n");
					return NULL;
				}
			}

			//If the required deposit level is larger than the subsidy, already generated coins needs to be thrown into the mix
			if(coinbaseDepositDisabled || requiredDepositLevel > maxBlockSubsidyIncFee) {
				const uint64_t nRequiredExtraDeposit = requiredDepositLevel - (coinbaseDepositDisabled ? 0 : maxBlockSubsidyIncFee);

				{
					LOCK(credits_mainState.cs_main);

					//Create a set of deposit txs to choose from
					vector<const Credits_CWalletTx*> vDepositWalletTxs;
					pdepositWallet->GetAllWalletTxs(vDepositWalletTxs);
					LogPrintf("Wallet contains %d transactions\n", vDepositWalletTxs.size());

					vector<Credits_CTransaction> vDepositTxs;
					BOOST_FOREACH(const Credits_CWalletTx* walletTx, vDepositWalletTxs)
					{
						const Credits_CWalletTx &tx = *walletTx;

						//Do some basic sanity checks. Failure should probably
						//occur here, as it does, since deposit wallet should never contain faulty deposits
						assert(Credits_IsFinalTx(tx));
						assert(tx.IsValidDeposit());
						for (unsigned int i = 0; i < tx.vout.size(); i++) {
							assert(tx.vout[i].nValue > 0);
						}

						//Do input verifications here. If validation fails the deposit is skipped instead of failure occurring
						//The reason is that a deposit wallet could be distributed between several different nodes and
						//another node may use the deposit in a block. The deposit is not removed form here since the block
						//may be disconnected through a chain rollback
						int64_t nClaimedCoins = 0;
						CValidationState state;
						if (!Credits_CheckInputs(tx, state, credits_view, claim_viewtmp, nClaimedCoins, true, MANDATORY_SCRIPT_VERIFY_FLAGS))
							continue;

						vDepositTxs.push_back(tx);
					}
					LogPrintf("Found %d deposit transactions available for mining\n", vDepositTxs.size());

					// Choose deposit transactions to use
					set<const Credits_CTransaction*>  setExtraDepositTxs;
					if (!SelectDepositTxs(nRequiredExtraDeposit, vDepositTxs, setExtraDepositTxs))
					{
						if(coinbaseDepositDisabled) {
							SelectLargestDepositTxs(nRequiredExtraDeposit, vDepositTxs, setExtraDepositTxs, 10);

							if(setExtraDepositTxs.size() == 0) {
								LogPrintf("\n\nERROR! No deposits could be found for mining. \n\n");
								return NULL;
							}
						}
					}

					if(setExtraDepositTxs.size() >= 10) {
						LogPrintf("\n\nERROR! Too many extra deposits in mining \n\n");
						return NULL;
					}

					BOOST_FOREACH(const Credits_CTransaction* tx, setExtraDepositTxs)
					{
						//Insert all new deposit transaction at position after coinbase
						if(pblock->vtx.size() == 1) {
							pblock->vtx.push_back(*tx);
							pblocktemplate->vTxFees.push_back(0); // Deposits can not have tx fees
							pblocktemplate->vTxSigOps.push_back(-1); // updated at end
						} else {
							const unsigned int insertAt = 1;
							pblock->vtx.insert(pblock->vtx.begin() + insertAt, *tx);
							pblocktemplate->vTxFees.insert(pblocktemplate->vTxFees.begin()+insertAt, 0); // Deposits can not have tx fees
							pblocktemplate->vTxSigOps.insert(pblocktemplate->vTxSigOps.begin()+insertAt, -1); // updated at end
						}
					}
				}
			}

			const uint64_t nDepositAmount = pblock->GetDepositAmount();
			if(nDepositAmount == 0) {
				LogPrintf("\n\nERROR! No deposit has been provided for block. No mining can be done. \n\n");
				return NULL;
			} else if(nDepositAmount < requiredDepositLevel){
				//This logic determines whether the subsidy needs to be lowered.
				//When half the monetary base has been mined, a lowered reward is enforced if not enough deposits have been provided.
				//This is in addition to the higher difficulty that will be enforced on too small deposits as well.
				const int64_t nAllowedBlockSubsidy = Bitcredit_GetAllowedBlockSubsidy(pindexPrev->nTotalMonetaryBase, nDepositAmount, pindexPrev->nTotalDepositBase);
				if(nAllowedBlockSubsidy < Bitcredit_GetMaxBlockSubsidy(pindexPrev->nTotalMonetaryBase)) {
					if(coinbaseDepositDisabled) {
						//Lower the subsidy since deposit is not high enough
						pblock->vtx[0].vout[0].nValue = nAllowedBlockSubsidy + nFees;
					} else {
						LogPrintf("\n\nERROR! Deposit amount sum must be at least the required level when coinbase as deposit is enabled. Deposit amount is: %d, Required: %d.\n\n", nDepositAmount, requiredDepositLevel);
						return NULL;
					}
				}
			} else if(nDepositAmount > requiredDepositLevel){
				//If we have enough for deposit, add rest as change
				if(!coinbaseDepositDisabled) {
					const uint256 coinbaseHash = pblock->vtx[0].GetHash();

					for (unsigned int i = 1; i < pblock->vtx.size(); i++) {
						Credits_CTransaction& txDeposit = pblock->vtx[i];

						if(!txDeposit.IsDeposit()) {
							break;
						}

						if(txDeposit.vin[0].prevout.hash == coinbaseHash) {
							const int64_t nCoinbaseDepositToChange = nDepositAmount - requiredDepositLevel;
							const int64_t nCoinbaseDeposit = txDeposit.vout[0].nValue;

							if(nCoinbaseDepositToChange < nCoinbaseDeposit) {
								txDeposit.vout.resize(2);
								txDeposit.vout[0].scriptPubKey = scriptPubKeyDeposit;
								txDeposit.vout[0].nValue = nCoinbaseDeposit - nCoinbaseDepositToChange;
								txDeposit.vout[1].scriptPubKey = scriptPubKeyDepositChange;
								txDeposit.vout[1].nValue = nCoinbaseDepositToChange;
							}

							break;
						}
					}

				}
			}

			// Fill in header
			pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
			Bitcredit_UpdateTime(*pblock, pindexPrev);
			pblock->nBits          = Bitcredit_GetNextWorkRequired(pindexPrev, pblock);
			pblock->nNonce         = 0;

			const int64_t nMonetaryBaseChange = (pblock->vtx[0].vout[0].nValue - nFees) + nClaimedCoinsForBlock;
			pblock->nTotalMonetaryBase = pindexPrev->nTotalMonetaryBase + nMonetaryBaseChange;
			pblock->nTotalDepositBase = pindexPrev->nTotalDepositBase + nMonetaryBaseChange - nTrimmedDepositBase + nResurrectedDepositBase;
			pblock->nDepositAmount = pblock->GetDepositAmount();
			pblock->hashLinkedBitcoinBlock  = *hashLinkedBitcoinBlockIndex->phashBlock;
        }
    	TryRemoveDirectory(tmpClaimDirPath);
        }

        const uint256 oldCoinbaseHash = pblock->vtx[0].GetHash();
        pblock->vtx[0].vin[0].scriptSig = CScript() << OP_0 << OP_0;

        pblock->RecalcLockHashAndMerkleRoot();

        if(!RecalculateCoinbaseDeposit(pblock, oldCoinbaseHash, keystore, pdepositWallet, coinbaseDepositDisabled)) {
            LogPrintf("\n\nERROR! CreateNewBlock() : Failed to recalculate coinbase as deposit. Miner exiting. \n\n");
        	return NULL;
        }
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();
        if(!pblock->UpdateSignatures(*pdepositWallet)) {
            LogPrintf("\n\nERROR! CreateNewBlock() : Failed to update signatures. Miner exiting. \n\n");
        	return NULL;
        }
        pblock->hashSigMerkleRoot = pblock->BuildSigMerkleTree();

        pblocktemplate->vTxFees[0] = -nFees;
        for (unsigned int i = 0; i < pblock->vtx.size(); ++i) {
        	if(pblock->vtx[i].IsCoinBase() || pblock->vtx[i].IsDeposit()) {
        		pblocktemplate->vTxSigOps[i] = Bitcredit_GetLegacySigOpCount(pblock->vtx[i]);
        	}
		}

        Credits_CBlockIndex indexDummy(*pblock);
        indexDummy.pprev = pindexPrev;
        indexDummy.nHeight = pindexPrev->nHeight + 1;

        Credits_CCoinsViewCache credits_viewNew(*bitcredit_pcoinsTip, true);

        const boost::filesystem::path tmpClaimDirPath = GetDataDir() / ".tmp" / Bitcoin_GetNewClaimTmpDbId();
        {
			//This is a throw away chainstate db + undo vector
        	std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > vBlockUndoClaims;
			Bitcoin_CClaimCoinsViewDB bitcoin_pclaimcoinsdbviewtmp(tmpClaimDirPath, Bitcoin_GetCoinDBCacheSize(), false, true, false, true);
			Bitcoin_CClaimCoinsViewCache claim_viewNew(*bitcoin_pclaimCoinsTip, bitcoin_pclaimcoinsdbviewtmp, bitcoin_nClaimCoinCacheFlushSize, true);

			CValidationState state;
			if (!Bitcredit_ConnectBlock(*pblock, state, &indexDummy, credits_viewNew, claim_viewNew, false, vBlockUndoClaims, true)) {
				LogPrintf("\n\nERROR! CreateNewBlock() : ConnectBlock failed when validating new block template. No mining can be done. Miner exiting. \n\n");
				return NULL;
			}
        }
    	TryRemoveDirectory(tmpClaimDirPath);
    }

    return pblocktemplate.release();
}

void IncrementExtraNonce(Credits_CBlock* pblock, Credits_CBlockIndex* pindexPrev, unsigned int& nExtraNonce, CKeyStore * bitcreditKeystore, CKeyStore * depositKeystore, const bool& coinbaseDepositDisabled)
{
    const uint256 oldCoinbaseHash = pblock->vtx[0].GetHash();

    // The n value of the outpoint of the coinbase is used as extra nonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    pblock->vtx[0].vin[0].prevout.n = nExtraNonce;

	if(!RecalculateCoinbaseDeposit(pblock, oldCoinbaseHash, bitcreditKeystore, depositKeystore, coinbaseDepositDisabled)) {
		LogPrintf("\n\nERROR! IncrementExtraNonce() : Failed to recalculate coinbase as deposit. Miner exiting. \n\n");
		assert(false);
	}

    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
    if(!pblock->UpdateSignatures(*depositKeystore)) {
        LogPrintf("\n\nERROR! IncrementExtraNonce() : Failed to update signatures. Miner exiting. \n\n");
    	assert(false);
    }
    pblock->hashSigMerkleRoot = pblock->BuildSigMerkleTree();
}


void FormatHashBuffers(Credits_CBlock* pblock, char* pmidstate, char* pdata, char* phash)
{
    //
    // Pre-build hash buffers
    //
    MiningStructure tmp;
    memset(&tmp, 0, sizeof(tmp));
    unsigned int shaChunksForHeader;
    unsigned int shaChunksForHash;
    tmp.InitData(pblock, shaChunksForHeader, shaChunksForHash);

    // Byte swap all the input buffer
    //This may be unnecessary since the buffer is swapped back and forth for no apparent reason
    for (unsigned int i = 0; i < sizeof(tmp)/4; i++)
        ((unsigned int*)&tmp)[i] = ByteReverse(((unsigned int*)&tmp)[i]);

    // Precalc the first half of the first hash, which stays constant
    SHA256Transform(pmidstate, &tmp.headerData, pSHA256InitState);

    memcpy(pdata, &tmp.headerData, shaChunksForHeader * sha256DigestChunkByteSize);
    memcpy(phash, &tmp.hash, shaChunksForHash * sha256DigestChunkByteSize);
}

#ifdef ENABLE_WALLET
//////////////////////////////////////////////////////////////////////////////
//
// Internal miner
//
double dHashesPerSec = 0.0;
int64_t nHPSTimerStart = 0;

//
// ScanHash scans nonces looking for a hash with at least some zero bits.
// It operates on big endian data.  Caller does the byte reversing.
// All input buffers are 16-byte aligned.  nNonce is usually preserved
// between calls, but periodically or if nNonce is 0xffff0000 or above,
// the block is rebuilt and nNonce starts over at zero.
//
unsigned int static ScanHash_CryptoPP(char* pdata, char* midHash, char* phash, char* pFinalHash, unsigned int& nHashesDone)
{
	unsigned int& nNonce = *(unsigned int*)(pdata + 12);
    for (;;)
    {
        nNonce++;

        // Crypto++ SHA256
        SHA256Transform(phash, pdata, midHash);

        //Second hashing of data hash
        SHA256Transform(pFinalHash, phash, pSHA256InitState);

        // Return the nonce if the hash has at least some zero bits,
        // caller will check if it has enough to reach the target
        //Since the bytes are swapped back and forth it is the 28 positioned byte that will be at the front in reversed hash
        if (((unsigned char*)pFinalHash)[28] == 0) {
            return nNonce;
        }

        // If nothing found after trying for a while, return -1
        if ((nNonce & 0xffff) == 0)
        {
            nHashesDone = 0xffff+1;
            return (unsigned int) -1;
        }
        if ((nNonce & 0xfff) == 0)
            boost::this_thread::interruption_point();
    }
}

Credits_CBlockTemplate* CreateNewBlockWithKey(Credits_CReserveKey& coinbaseReservekey, Credits_CReserveKey& depositReserveKey, Credits_CReserveKey& depositChangeReserveKey, Credits_CReserveKey& depositReserveSigningKey, CKeyStore * keystore, Credits_CWallet* pdepositWallet, bool coinbaseDepositDisabled)
{
    CPubKey pubkeyCoinbase;
    if (!coinbaseReservekey.GetReservedKey(pubkeyCoinbase)) {
        LogPrintf("\n\nERROR! Could not get coinbase reserve key.\n\n");
        return NULL;
    }
    CScript scriptPubKeyCoinbase = CScript() << pubkeyCoinbase << OP_CHECKSIG;

    CPubKey pubkeyDeposit;
    if (!depositReserveKey.GetReservedKey(pubkeyDeposit)){
        LogPrintf("\n\nERROR! Could not get deposit reserve key.\n\n");
    	return NULL;
    }
    CScript scriptPubKeyDeposit = CScript() << pubkeyDeposit << OP_CHECKSIG;

    CPubKey pubkeyDepositChange;
    if (!depositChangeReserveKey.GetReservedKey(pubkeyDepositChange)) {
        LogPrintf("\n\nERROR! Could not get deposit change reserve key.\n\n");
    	return NULL;
    }
    CScript scriptPubKeyDepositChange = CScript() << pubkeyDepositChange << OP_CHECKSIG;

    CPubKey pubkeySigningDeposit;
    if (!depositReserveSigningKey.GetReservedKey(pubkeySigningDeposit)) {
        LogPrintf("\n\nERROR! Could not get deposit signing reserve key.\n\n");
    	return NULL;
    }

    return CreateNewBlock(scriptPubKeyCoinbase, scriptPubKeyDeposit, scriptPubKeyDepositChange, pubkeySigningDeposit, keystore, pdepositWallet, coinbaseDepositDisabled);
}

bool CheckWork(Credits_CBlock* pblock, Credits_CWallet& credits_wallet, Credits_CWallet& deposit_wallet, Credits_CReserveKey& reservekey, Credits_CReserveKey& depositReserveKey, Credits_CReserveKey& depositChangeReserveKey, Credits_CReserveKey& depositReserveSigningKey)
{
    if(!VerifyDepositSignatures("CheckWork", pblock)) {
    	return false;
    }

    if(!Bitcredit_CheckProofOfWork(pblock->GetHash(), pblock->nBits, pblock->nTotalDepositBase, pblock->nDepositAmount)) {
    	return false;
    }

    //// debug print
    LogPrintf("BitcreditMiner:\n");
    LogPrintf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", pblock->GetHash().GetHex(), uint256().SetCompact(pblock->nBits).GetHex());
    pblock->print();
    LogPrintf("generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue));

    // Found a solution
    {
        LOCK(credits_mainState.cs_main);
        if (pblock->hashPrevBlock != credits_chainActive.Tip()->GetBlockHash())
            return error("BitcreditMiner : generated block is stale");

        // Remove key from key pool
        reservekey.KeepKey();
        depositReserveKey.KeepKey();
        depositChangeReserveKey.KeepKey();
        depositReserveSigningKey.KeepKey();

        BOOST_FOREACH(const Credits_CTransaction& tx, pblock->vtx) {
        	if(tx.IsDeposit() && tx.vin[0].prevout != COutPoint(pblock->vtx[0].GetHash(), 0)) {
				uint256 txHash = tx.GetHash();
				if(deposit_wallet.GetWalletTx(txHash) != NULL) {
					deposit_wallet.EraseFromWallet(&credits_wallet, txHash);
				} else {
					LogPrintf("\n\n ERROR: deposit tx used in mining could not be deleted from deposit db: %s\n\n", txHash.GetHex());
				}
        	}
        }

        // Track how many getdata requests this block gets
        {
            LOCK(credits_wallet.cs_wallet);
            credits_wallet.mapRequestCount[pblock->GetHash()] = 0;
        }

        // Process this block the same as if we had received it from another node
        CValidationState state;
        if (!Bitcredit_ProcessBlock(state, NULL, pblock))
            return error("BitcreditMiner : ProcessBlock, block not accepted");
    }

    return true;
}

void static BitcoinMiner(Credits_CWallet *pwallet, Credits_CWallet* pdepositWallet, bool coinbaseDepositDisabled)
{
    LogPrintf("Credits Miner started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("bitcredit-miner");

    // Each thread has its own key and counter
    Credits_CReserveKey reservekey(pwallet);
    if(bitcredit_pwalletMain->IsLocked() && !coinbaseDepositDisabled) {
    	reservekey = Credits_CReserveKey(pdepositWallet);
    }
    Credits_CReserveKey depositReservekey(pwallet);
    Credits_CReserveKey depositChangeReservekey(pwallet);
    Credits_CReserveKey depositReserveSigningKey(pdepositWallet);

    unsigned int nExtraNonce = 0;

    CNetParams * netParams = Credits_NetParams();

    try { while (true) {
    	//Checkpoint that waits until the ->bitcoin<- blockchain is (reasonably) up to date
    	if(Bitcoin_IsInitialBlockDownload()) {
            LogPrintf("Credits Miner waiting for bitcoin blockchain to be up to date.\n");

            MilliSleep(10000);
    		continue;
    	}

        if (Bitcredit_Params().NetworkID() != CChainParams::REGTEST) {
            // Busy-wait for the network to come online so we don't waste time mining
            // on an obsolete chain. In regtest mode we expect to fly solo.
            while (netParams->vNodes.empty()) {
                LogPrintf("Credits Miner waiting for other nodes. No nodes online.\n");

            	MilliSleep(3000);
            }
        }

        //
        // Create new block
        //
        unsigned int nTransactionsUpdatedLast = credits_mempool.GetTransactionsUpdated();
        Credits_CBlockIndex* pindexPrev = (Credits_CBlockIndex*)credits_chainActive.Tip();

        auto_ptr<Credits_CBlockTemplate> pblocktemplate(CreateNewBlockWithKey(reservekey, depositReservekey, depositChangeReservekey, depositReserveSigningKey, pwallet, pdepositWallet, coinbaseDepositDisabled));
        if (!pblocktemplate.get())
            return;
        Credits_CBlock *pblock = &pblocktemplate->block;
        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce, pwallet, pdepositWallet, coinbaseDepositDisabled);

        if(!VerifyDepositSignatures("Bitcreditminer", pblock)) {
        	return;
        }

        LogPrintf("Running BitcreditMiner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
               ::GetSerializeSize(*pblock, SER_NETWORK, CREDITS_PROTOCOL_VERSION));

        //Just used to get chunk size
        MiningStructure tmp;
        memset(&tmp, 0, sizeof(tmp));
	    unsigned int shaChunksForHeader;
	    unsigned int shaChunksForHash;
	    tmp.InitData(pblock, shaChunksForHeader, shaChunksForHash);

        // Pre-build hash buffers
        char pdatabuf[shaChunksForHeader * sha256DigestChunkByteSize + alignmentExtraBytes];
        char* pdata     = alignup<alignmentExtraBytes>(pdatabuf);
        char pmidstatebuf[sha256ResultByteSize + alignmentExtraBytes];
        char* pmidstate = alignup<alignmentExtraBytes>(pmidstatebuf);
        char midstate2buf[sha256ResultByteSize + alignmentExtraBytes];
        char* midstate2 = alignup<alignmentExtraBytes>(midstate2buf);
        char phashbuf[sha256DigestChunkByteSize + alignmentExtraBytes];
        char* phash    = alignup<alignmentExtraBytes>(phashbuf);
        uint256 finalHashBuf[2];
        uint256& finalHash = *alignup<alignmentExtraBytes>(finalHashBuf);

        FormatHashBuffers(pblock, pmidstate, pdata, phash);

        //Precalc the second hashing of three 64 byte chunks that are required to hash the header
    	SHA256Transform(midstate2, pdata + sha256DigestChunkByteSize, pmidstate);

        const unsigned int startPosOfBlockTime = 4 + 32 + 32 + 32 + 32;
        unsigned int& nBlockTime = *(unsigned int*)(pdata + startPosOfBlockTime);
        unsigned int& nBlockBits = *(unsigned int*)(pdata + startPosOfBlockTime + 4);
        unsigned int& nBlockNonce = *(unsigned int*)(pdata + startPosOfBlockTime + 8);

        // Search
        int64_t nStart = GetTime();
        uint256 hashTarget = uint256().SetCompact(pblock->nBits);
        hashTarget = Bitcredit_ReduceByReqDepositLevel(hashTarget, pblock->GetDepositAmount(), pblock->nTotalDepositBase);
        while (true)
        {
            unsigned int nHashesDone = 0;

            // Crypto++ SHA256
            unsigned int nNonceFound = ScanHash_CryptoPP(pdata + sha256DigestChunkByteSize + sha256DigestChunkByteSize, midstate2, phash, (char*)&finalHash, nHashesDone);

            // Check if something found
            if (nNonceFound)
            {
                for (unsigned int i = 0; i < sizeof(finalHash)/4; i++)
                    ((unsigned int*)&finalHash)[i] = ByteReverse(((unsigned int*)&finalHash)[i]);

                if (finalHash <= hashTarget)
                {
                	pblock->nNonce = ByteReverse(nNonceFound);
                    assert(finalHash == pblock->GetHash());

                    SetThreadPriority(THREAD_PRIORITY_NORMAL);
                    CheckWork(pblock, *pwallet, *pdepositWallet, reservekey, depositReservekey, depositChangeReservekey, depositReserveSigningKey);
                    SetThreadPriority(THREAD_PRIORITY_LOWEST);

                    // In regression test mode, stop mining after a block is found. This
                    // allows developers to controllably generate a block on demand.
                    if (Bitcredit_Params().NetworkID() == CChainParams::REGTEST)
                        throw boost::thread_interrupted();

                    break;
                }
            }

            // Meter hashes/sec
            static int64_t nHashCounter;
            if (nHPSTimerStart == 0)
            {
                nHPSTimerStart = GetTimeMillis();
                nHashCounter = 0;
            }
            else
                nHashCounter += nHashesDone;
            if (GetTimeMillis() - nHPSTimerStart > 4000)
            {
                static CCriticalSection cs;
                {
                    LOCK(cs);
                    if (GetTimeMillis() - nHPSTimerStart > 4000)
                    {
                        dHashesPerSec = 1000.0 * nHashCounter / (GetTimeMillis() - nHPSTimerStart);
                        nHPSTimerStart = GetTimeMillis();
                        nHashCounter = 0;
                        static int64_t nLogTime;
                        if (GetTime() - nLogTime > 1 * 20)
                        {
                            nLogTime = GetTime();
                            LogPrintf("hashmeter %6.0f hash/s\n", dHashesPerSec);
                        }
                    }
                }
            }

            // Check for stop or if block needs to be rebuilt
            boost::this_thread::interruption_point();
            if (netParams->vNodes.empty() && Bitcredit_Params().NetworkID() != CChainParams::REGTEST)
                break;
            if (nBlockNonce >= 0xffff0000)
                break;
            if (credits_mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                break;
            if (pindexPrev != credits_chainActive.Tip())
                break;

            // Update nTime every few seconds
            Bitcredit_UpdateTime(*pblock, pindexPrev);
            nBlockTime = ByteReverse(pblock->nTime);
            if (Bitcredit_TestNet())
            {
                // Changing pblock->nTime can change work required on testnet:
                nBlockBits = ByteReverse(pblock->nBits);
                hashTarget.SetCompact(pblock->nBits);
                hashTarget = Bitcredit_ReduceByReqDepositLevel(hashTarget, pblock->GetDepositAmount(), pblock->nTotalDepositBase);
            }
        }
    } }
    catch (boost::thread_interrupted)
    {
        LogPrintf("BitcreditMiner terminated\n");
        throw;
    }
}

void GenerateBitcredits(bool fGenerate, Credits_CWallet* pwallet, Credits_CWallet* pdepositWallet, int nThreads, bool coinbaseDepositDisabled)
{
    static boost::thread_group* minerThreads = NULL;

    if (nThreads < 0) {
        if (Bitcredit_Params().NetworkID() == CChainParams::REGTEST)
            nThreads = 1;
        else
            nThreads = boost::thread::hardware_concurrency();
    }

    if (minerThreads != NULL)
    {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (nThreads == 0 || !fGenerate)
        return;

    minerThreads = new boost::thread_group();
    for (int i = 0; i < nThreads; i++)
        minerThreads->create_thread(boost::bind(&BitcoinMiner, pwallet, pdepositWallet, coinbaseDepositDisabled));
}

#endif

