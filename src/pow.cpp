// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//2016ブロックごとにターゲットを調整するコードです。

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

/*[概要]
この関数は次のブロックのディフィカルティを返すようです。ブロックヘッダのnBitに設定される値です。
[引数]
`const CBlockIndex* pindexLast` 一つ前のブロックへのインデックス
`const CBlockHeader *pblock` ブロックヘッダへのポインタ。PoWで正解のハッシュ値を見つけたいブロックを指します。
`const Consensus::Params& params` PoW のパラメータです。ここで参照しているparams.powLimit はメインネットでは次の値がセットされています。
`00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff`*/
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    /*`DifficultyAdjustmentInterval()` はPoWの難易度調整を実施するブロック高の間隔であり、 `2週間 / 10分`を計算
    した結果である 2016 を返します。`pindexLast->nHeight+1` つまり次に追加するブロックのブロック高が難易度調整の
    タイミングかどうかを判定しています。難易度調整が必要ない場合にif文のブロックが実行されます。fPowAllowMinDifficultyBlock
    はテストネットのためのモードです。テストネットと regtestとしてノードを起動したときに true がセットされます。なので、この
    ブロックは テストネット、regtest のときだけ実行されます。*/
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

/*その中身の処理です。コメントにある通り、最初のif文 `pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2`
では、次にチェーンにつなぎたいブロックのブロックタイムが前回のブロックから20分以上経っているかを確認しています。
経っている場合は `nProofOfWorkLimit` を返しています。
`nProofOfWorkLimit`はマイニングの難易度の最小値で、chainparams.cppの中で各チェーンのモードごとに以下のように設定されています。

	main net: uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff")
	test net: uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff")
	regtest: uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff")
`nProofOfWorkLimit` はこの設定値をuint32 に変換したものがセットされています。
↓変換処理
`unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();`
*/

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}


unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;


   /*unsigned int nBits]
難易度を表すブロックのフィールドです。このデータを基にブロックハッシュが満たすべき最小値を求めます。*/

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    //ここで、nBits を 256bit の数値型に変換しています。

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
