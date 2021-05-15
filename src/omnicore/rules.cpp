/**
 * @file rules.cpp
 *
 * This file contains consensus rules and restrictions.
 */

#include <omnicore/rules.h>

#include <omnicore/activation.h>
#include <omnicore/consensushash.h>
#include <omnicore/dbtxlist.h>
#include <omnicore/log.h>
#include <omnicore/omnicore.h>
#include <omnicore/notifications.h>
#include <omnicore/utilsbitcoin.h>
#include <omnicore/version.h>

#include <chainparams.h>
#include <validation.h>
#include <script/standard.h>
#include <uint256.h>
#include <ui_interface.h>

#include <stdint.h>
#include <limits>
#include <string>
#include <vector>

namespace mastercore
{
/**
 * Returns a mapping of transaction types, and the blocks at which they are enabled.
 */
std::vector<TransactionRestriction> CConsensusParams::GetRestrictions() const
{
    const TransactionRestriction vTxRestrictions[] =
    { //  transaction type                    version        allow 0  activation block
      //  ----------------------------------  -------------  -------  ------------------
        { OMNICORE_MESSAGE_TYPE_ALERT,        0xFFFF,        true,    MSC_ALERT_BLOCK    },
        { OMNICORE_MESSAGE_TYPE_ACTIVATION,   0xFFFF,        true,    MSC_ALERT_BLOCK    },
        { OMNICORE_MESSAGE_TYPE_DEACTIVATION, 0xFFFF,        true,    MSC_ALERT_BLOCK    },

        { MSC_TYPE_SIMPLE_SEND,               MP_TX_PKT_V0,  false,   MSC_SEND_BLOCK     },

        { MSC_TYPE_TRADE_OFFER,               MP_TX_PKT_V0,  false,   MSC_DEX_BLOCK      },
        { MSC_TYPE_TRADE_OFFER,               MP_TX_PKT_V1,  false,   MSC_DEX_BLOCK      },
        { MSC_TYPE_ACCEPT_OFFER_BTC,          MP_TX_PKT_V0,  false,   MSC_DEX_BLOCK      },

        { MSC_TYPE_CREATE_PROPERTY_FIXED,     MP_TX_PKT_V0,  false,   MSC_SP_BLOCK       },
        { MSC_TYPE_CREATE_PROPERTY_VARIABLE,  MP_TX_PKT_V0,  false,   MSC_SP_BLOCK       },
        { MSC_TYPE_CREATE_PROPERTY_VARIABLE,  MP_TX_PKT_V1,  false,   MSC_SP_BLOCK       },
        { MSC_TYPE_CLOSE_CROWDSALE,           MP_TX_PKT_V0,  false,   MSC_SP_BLOCK       },

        { MSC_TYPE_CREATE_PROPERTY_MANUAL,    MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_GRANT_PROPERTY_TOKENS,     MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_REVOKE_PROPERTY_TOKENS,    MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_CHANGE_ISSUER_ADDRESS,     MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_ENABLE_FREEZING,           MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_DISABLE_FREEZING,          MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_FREEZE_PROPERTY_TOKENS,    MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_UNFREEZE_PROPERTY_TOKENS,  MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },

        { MSC_TYPE_ADD_DELEGATE,              MP_TX_PKT_V0,  false,   MSC_DELEGATED_ISSUANCE_BLOCK },
        { MSC_TYPE_REMOVE_DELEGATE,           MP_TX_PKT_V0,  false,   MSC_DELEGATED_ISSUANCE_BLOCK },

        { MSC_TYPE_SEND_TO_OWNERS,            MP_TX_PKT_V0,  false,   MSC_STO_BLOCK      },
        { MSC_TYPE_SEND_TO_OWNERS,            MP_TX_PKT_V1,  false,   MSC_STOV1_BLOCK    },

        { MSC_TYPE_METADEX_TRADE,             MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },
        { MSC_TYPE_METADEX_CANCEL_PRICE,      MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },
        { MSC_TYPE_METADEX_CANCEL_PAIR,       MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },
        { MSC_TYPE_METADEX_CANCEL_ECOSYSTEM,  MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },

        { MSC_TYPE_SEND_ALL,                  MP_TX_PKT_V0,  false,   MSC_SEND_ALL_BLOCK },
        
        { MSC_TYPE_ANYDATA,                   MP_TX_PKT_V0,  true,    MSC_ANYDATA_BLOCK },

        { MSC_TYPE_OFFER_ACCEPT_A_BET,        MP_TX_PKT_V0,  false,   MSC_BET_BLOCK      },

        { MSC_TYPE_SEND_NONFUNGIBLE,          MP_TX_PKT_V0,  false,   MSC_NONFUNGIBLE_BLOCK  },
        { MSC_TYPE_NONFUNGIBLE_DATA,          MP_TX_PKT_V0,  false,   MSC_NONFUNGIBLE_BLOCK  },
    };

    const size_t nSize = sizeof(vTxRestrictions) / sizeof(vTxRestrictions[0]);

    return std::vector<TransactionRestriction>(vTxRestrictions, vTxRestrictions + nSize);
}

/**
 * Returns an empty vector of consensus checkpoints.
 *
 * This method should be overwritten by the child classes, if needed.
 */
std::vector<ConsensusCheckpoint> CConsensusParams::GetCheckpoints() const
{
    return std::vector<ConsensusCheckpoint>();
}

/**
 * Returns an empty vector of transaction checkpoints.
 *
 * This method should be overwritten by the child classes, if needed.
 */
std::vector<TransactionCheckpoint> CConsensusParams::GetTransactions() const
{
    return std::vector<TransactionCheckpoint>();
}

/**
 * Returns consensus checkpoints for mainnet, used to verify transaction processing.
 */
std::vector<ConsensusCheckpoint> CMainConsensusParams::GetCheckpoints() const
{
    // block height, block hash and consensus hash
    const ConsensusCheckpoint vCheckpoints[] = {
        { 250000, uint256S("000000000000003887df1f29024b06fc2200b55f8af8f35453d7be294df2d214"),
                  uint256S("c2e1e0f3cf3c49d8ee08bd45ad39be27eb400041d6288864ee144892449c97df") },
        { 260000, uint256S("000000000000001fb91fbcebaaba0e2d926f04908d798a8b598c3bd962951080"),
                  uint256S("cfe2c574a9f969cfa26f23d3a0a7b3c3f416b50e7fb7b2adffe4524a4a7b0992") },
        { 270000, uint256S("0000000000000002a775aec59dc6a9e4bb1c025cf1b8c2195dd9dc3998c827c5"),
                  uint256S("46daa1df4cea9a1edc9624091d94839203239502bafcc3a20df2fee1a446cf42") },
        { 280000, uint256S("0000000000000001c091ada69f444dc0282ecaabe4808ddbb2532e5555db0c03"),
                  uint256S("4739e5d00fc94e079428cd5a29421df4de3f2b3a4903990a162d8afdd2605fd9") },
        { 290000, uint256S("0000000000000000fa0b2badd05db0178623ebf8dd081fe7eb874c26e27d0b3b"),
                  uint256S("51cb4219ae68cc4bf5bd835a1fadec4c4e587c3653304dfdff8109ea0795bfcb") },
        { 300000, uint256S("000000000000000082ccf8f1557c5d40b21edabb18d2d691cfbf87118bac7254"),
                  uint256S("3bc727b74dd660ac080c974af67bb627f5a059b82935839a37ce3309af68b7be") },
        { 310000, uint256S("0000000000000000125a28cc9e9209ddb75718f599a8039f6c9e7d9f1fb021e0"),
                  uint256S("1a1923a644bee373649e01a0a825daae8d0b862a3488e7f5b092599862169fb7") },
        { 320000, uint256S("000000000000000015aab005b28a326ade60f07515c33517ea5cb598f28fb7ea"),
                  uint256S("30f169f9bff9296157b6d116560af485dc6bccabaf827393d4683823e9dca1f4") },
        { 330000, uint256S("00000000000000000faabab19f17c0178c754dbed023e6c871dcaf74159c5f02"),
                  uint256S("52cb759cf37cf8aa25c14f988e1515b966e40cd29a310b8faff6cab0bfe0112e") },
        { 340000, uint256S("00000000000000000d9b2508615d569e18f00c034d71474fc44a43af8d4a5003"),
                  uint256S("1e21df8610d5ed32645df4e1aacebbb423e4ebe7097015a2392070bb53b3bdf1") },
        { 350000, uint256S("0000000000000000053cf64f0400bb38e0c4b3872c38795ddde27acb40a112bb"),
                  uint256S("05e89e25bb86688aac26bc796084638f10bc7564e391eb5c31e07e26f952f92f") },
        { 360000, uint256S("00000000000000000ca6e07cf681390ff888b7f96790286a440da0f2b87c8ea6"),
                  uint256S("0ffa97ffd5ac83030d50fbb23e0e953ff3717aa0b5181734e782a62ac39925af") },
        { 370000, uint256S("000000000000000002cad3026f68357229dd6eaa6bcef6fe5166e1e53b039b8c"),
                  uint256S("4cce696e822f390fc83a730095d39f5cca5121398829c087dd0c92154e1fb83c") },
        { 380000, uint256S("00000000000000000b06cee3cee10d2617e2024a996f5c613f7d786b15a571ff"),
                  uint256S("32b092620f37c02a1ca33acf5b1f3752642b23e8089ffc4ff0ae401ed41aa9d7") },
        { 390000, uint256S("00000000000000000520000e60b56818523479ada2614806ba17ce0bbe6eaded"),
                  uint256S("ef1812cf6cc1b1b89de173666126744e3f2441bb32c5e28233088f4c8757eb19") },
        { 400000, uint256S("000000000000000004ec466ce4732fe6f1ed1cddc2ed4b328fff5224276e3f6f"),
                  uint256S("1e8949f29a5250c5819d96fd46e632d145e0c667dafd4478598ebb2bb1d5ba84") },
        { 410000, uint256S("0000000000000000060d7ea100ecb75c0a4dc482d05ff19ddaa8046b4b80a458"),
                  uint256S("428a0cce4fe10f2e9874aba3882729149ef1db6e721e16204f65ba5ffb727827") },
        { 420000, uint256S("000000000000000002cce816c0ab2c5c269cb081896b7dcb34b8422d6b74ffa1"),
                  uint256S("1ca6c6f7f31ff7705a0336140485338abcbadf27e4bfdb3484b900b0b4673bba") },
        { 430000, uint256S("000000000000000001868b2bb3a285f3cc6b33ea234eb70facf4dcdf22186b87"),
                  uint256S("758b6850a3fdd86194d20f4c7f3bbbe66c38f78722c242e2ecefaaa42eda6a15") },
        { 440000, uint256S("0000000000000000038cc0f7bcdbb451ad34a458e2d535764f835fdeb896f29b"),
                  uint256S("94e3e045b846b35226c1b7c9399992515094e227fd195626e3875ad812b44e7a") },
        { 450000, uint256S("0000000000000000014083723ed311a461c648068af8cef8a19dcd620c07a20b"),
                  uint256S("ed4a1b81afd4662089e9310b5bec98e77cfb6a70a6f679ba365aed24d3d5e71d") },
        { 460000, uint256S("000000000000000000ef751bbce8e744ad303c47ece06c8d863e4d417efc258c"),
                  uint256S("a5740ebaad24b87ffb1bbd35ea64a3cda6c58e777a513acec51623fed8287d2d") },
        { 470000, uint256S("0000000000000000006c539c722e280a0769abd510af0073430159d71e6d7589"),
                  uint256S("eb68d4ed7b0a84dbf75802d717a754a4cc0c2ad48955e3ea89900493b8101845") },
        { 480000, uint256S("000000000000000001024c5d7a766b173fc9dbb1be1a4dc7e039e631fd96a8b1"),
                  uint256S("500cc23ba7af3a1fc5d4d910e0c375df515f967b0b9b71871cf38dfc34fb5334") },
        { 490000, uint256S("000000000000000000de069137b17b8d5a3dfbd5b145b2dcfb203f15d0c4de90"),
                  uint256S("37248b5d4ec7c2c6ff4fd2aa98a4092c46f2c38a19f9241bffb47157cfb76e4f") },
        { 500000, uint256S("00000000000000000024fb37364cbf81fd49cc2d51c09c75c35433c3a1945d04"),
                  uint256S("748c629e4df688b71f01a1c1461d2adaf0dc5f825b91c4505f6949bf5723cdc7") },
        { 510000, uint256S("000000000000000000152678f83ec36b6951ed3f7e1cc3b04c5828cab8017329"),
                  uint256S("41a7aa0b395c7b422a1edcbe6e8e7a915dad2f9a560ca768853ff137fcc05841") },
        { 520000, uint256S("0000000000000000000d26984c0229c9f6962dc74db0a6d525f2f1640396f69c"),
                  uint256S("3cc8a806a4541c0fe758180bbe6ee51033afbef170c82c94c26649bca57b89c6") },
        { 562708, uint256S("0000000000000000001e8a2aeca536b0ee2b97ae85f83980f00a329dd93ac557"),
                  uint256S("f06698346373e7e7df1fa4227f45695d05fcf26096e7200499af23644a1ed762") },
    };

    const size_t nSize = sizeof(vCheckpoints) / sizeof(vCheckpoints[0]);

    return std::vector<ConsensusCheckpoint>(vCheckpoints, vCheckpoints + nSize);
}

/**
 * Returns transactions checkpoints for mainnet, used to verify DB consistency.
 */
std::vector<TransactionCheckpoint> CMainConsensusParams::GetTransactions() const
{
    // block height, transaction hash
    const TransactionCheckpoint vTransactions[] = {
        { 306906, uint256S("b7c66175a99ca0e7b1691905d50a46165adb7a8012d9ec5e1ecf8239f859df6d") },
        { 306906, uint256S("a59ac18eda590dfe9f3671f99f3cec5679e95ea4d1478d5313e2b4706307537b") },
        { 306912, uint256S("ce56ee84abd20bd1386d66e5ab9e3661a9bfad45a7d46490cae2241bbebf91df") },
        { 306913, uint256S("bbdb911227c033697e92d6828d43b15f99bbc24c97147be0aed12dd98f362c28") },
        { 307290, uint256S("69604c02f219136245d309b722816d6f4f77ccead60ded6976219f9a0becceff") },
        { 307403, uint256S("201c38a5b819c870830ddd7583d446f05a12c6b01ec3690e3acf33e49fe8d91f") },
        { 307620, uint256S("7ddac86217f6fc37a11f7de3b8edb78b6f7d10e93cece8f6bf0565ddc95acc76") },
        { 307897, uint256S("22b4ee94c5a31a082e1b8bc77e79a54783262b19f957d6603f16876397546e58") },
        { 308666, uint256S("84fc343c89218faa9f6f31a9ee266837e19d9bc1319737ae66fa2d1fd095d271") },
        { 309030, uint256S("f3493671fe79a9dd5af871ac1f9b715db8e895c80584031091c626e43a73d4e0") },
        { 309031, uint256S("eda3d2bbb8125397f4d4909ea25d845dc451e8a3206035bf0d736bb3ece5d758") },
        { 309052, uint256S("a4abfc7a10a262bec4a9be830fa784cbb712cfa7d19fb28e854c3d7d7ea50719") },
        { 310003, uint256S("6d373615f4938cc9dfacf213ba0f33ec1b63961a61b4869ea69e08c938671367") },
        { 310114, uint256S("17e48d57fe979aa821b30c14edf76b5203f5dc72847ab9350c068870551d6a5d") },
        { 310121, uint256S("a67609d7941c8e7dc33fd1e4a286f7407e6139c61bec3247d20f546e0b3c2e7f") },
        { 310132, uint256S("21aa754a425c7748688a57bb409cf6942a3eb7b087b32e29254fae62e5dbbe71") },
        { 310313, uint256S("18e955f7e8aa7efa2f07768c9916967cea1821c197a2d9f815a51358b82a549d") },
        { 310502, uint256S("6ab48c227a3aed94df462ab2628a87b566d9bc985b27fd8d3e86f52794e3d735") },
        { 310577, uint256S("1b9d32711dd896d18b083c1d3d12f2ba661c5cbcdc2122a224ecfb462a58eb92") },
        { 310985, uint256S("1d8af5bfaf69b3cea356501f681c698017b60d1aef3d1dcdb2e72454ae682add") },
        { 312967, uint256S("5b922dad64c5c4241c694401d627d6c0e33033215f1fb375aa21b435fa032fce") },
        { 313118, uint256S("e152ffe783d54f7dbf2ab846d3c1904037b26788e48617621e1fdefe151be42a") },
        { 313538, uint256S("b01d1594a7e2083ebcd428706045df003f290c4dc7bd6d77c93df9fcca68232f") },
        { 314110, uint256S("189c959a37ecac2045ba3631b87e151e09506e6a492e867bbcc6c89e3bbc91e7") },
        { 314126, uint256S("7244beac4e5165b3386728f2cd4c203dd6144b6676853e5012a87b75135468b0") },
        { 315312, uint256S("8fbd96005aba5671daf8288f89df8026a7ce4782a0bb411937537933956b827b") },
        { 420430, uint256S("64ab528aa6407033ac4925601f132d1ce3bca47ec140d27e7f5a9bad0f37cbc6") },
        { 420551, uint256S("5b81b20508c98a9bda3de68d6b3872f52efd02ebe2cb9b114cfb3a3225f33dd1") },
    };

    const size_t nSize = sizeof(vTransactions) / sizeof(vTransactions[0]);

    return std::vector<TransactionCheckpoint>(vTransactions, vTransactions + nSize);
}

/**
 * Constructor for mainnet consensus parameters.
 */
CMainConsensusParams::CMainConsensusParams()
{
    // Exodus related:
    exodusBonusPerWeek = 0.10;
    exodusDeadline = 1377993600;
    exodusReward = 100;
    GENESIS_BLOCK = 249498;
    LAST_EXODUS_BLOCK = 255365;
    // Notice range for feature activations:
    MIN_ACTIVATION_BLOCKS = 2048;  // ~2 weeks
    MAX_ACTIVATION_BLOCKS = 12288; // ~12 weeks
    // Waiting period for enabling freezing
    OMNI_FREEZE_WAIT_PERIOD = 4096; // ~4 weeks
    // Script related:
    PUBKEYHASH_BLOCK = 0;
    SCRIPTHASH_BLOCK = 322000;
    MULTISIG_BLOCK = 0;
    NULLDATA_BLOCK = 395000;
    // Transaction restrictions:
    MSC_ALERT_BLOCK = 0;
    MSC_SEND_BLOCK = 249498;
    MSC_DEX_BLOCK = 290630;
    MSC_SP_BLOCK = 297110;
    MSC_MANUALSP_BLOCK = 323230;
    MSC_STO_BLOCK = 342650;
    MSC_METADEX_BLOCK = 400000;
    MSC_SEND_ALL_BLOCK = 395000;
    MSC_BET_BLOCK = 999999;
    MSC_STOV1_BLOCK = 999999;
    MSC_ANYDATA_BLOCK = 0;
    MSC_NONFUNGIBLE_BLOCK = 999999;
    MSC_DELEGATED_ISSUANCE_BLOCK = 999999;
    // Other feature activations:
    GRANTEFFECTS_FEATURE_BLOCK = 394500;
    DEXMATH_FEATURE_BLOCK = 395000;
    SPCROWDCROSSOVER_FEATURE_BLOCK = 395000;
    TRADEALLPAIRS_FEATURE_BLOCK = 438500;
    FEES_FEATURE_BLOCK = 999999;
    FREEZENOTICE_FEATURE_BLOCK = 999999;
    FREEDEX_FEATURE_BLOCK = 999999;
}

/**
 * Constructor for testnet consensus parameters.
 */
CTestNetConsensusParams::CTestNetConsensusParams()
{
    // Exodus related:
    exodusBonusPerWeek = 0.00;
    exodusDeadline = 1377993600;
    exodusReward = 100;
    GENESIS_BLOCK = 263000;
    LAST_EXODUS_BLOCK = std::numeric_limits<int>::max();
    // Notice range for feature activations:
    MIN_ACTIVATION_BLOCKS = 0;
    MAX_ACTIVATION_BLOCKS = 999999;
    // Waiting period for enabling freezing
    OMNI_FREEZE_WAIT_PERIOD = 0;
    // Script related:
    PUBKEYHASH_BLOCK = 0;
    SCRIPTHASH_BLOCK = 0;
    MULTISIG_BLOCK = 0;
    NULLDATA_BLOCK = 0;
    // Transaction restrictions:
    MSC_ALERT_BLOCK = 0;
    MSC_SEND_BLOCK = 0;
    MSC_DEX_BLOCK = 0;
    MSC_SP_BLOCK = 0;
    MSC_MANUALSP_BLOCK = 0;
    MSC_STO_BLOCK = 0;
    MSC_METADEX_BLOCK = 0;
    MSC_SEND_ALL_BLOCK = 0;
    MSC_BET_BLOCK = 999999;
    MSC_STOV1_BLOCK = 0;
    MSC_ANYDATA_BLOCK = 0;
    MSC_NONFUNGIBLE_BLOCK = 0;
    MSC_DELEGATED_ISSUANCE_BLOCK = 0;
    // Other feature activations:
    GRANTEFFECTS_FEATURE_BLOCK = 0;
    DEXMATH_FEATURE_BLOCK = 0;
    SPCROWDCROSSOVER_FEATURE_BLOCK = 0;
    TRADEALLPAIRS_FEATURE_BLOCK = 0;
    FEES_FEATURE_BLOCK = 0;
    FREEZENOTICE_FEATURE_BLOCK = 0;
    FREEDEX_FEATURE_BLOCK = 0;
}

/**
 * Constructor for regtest consensus parameters.
 */
CRegTestConsensusParams::CRegTestConsensusParams()
{
    // Exodus related:
    exodusBonusPerWeek = 0.00;
    exodusDeadline = 1377993600;
    exodusReward = 100;
    GENESIS_BLOCK = 101;
    LAST_EXODUS_BLOCK = std::numeric_limits<int>::max();
    // Notice range for feature activations:
    MIN_ACTIVATION_BLOCKS = 5;
    MAX_ACTIVATION_BLOCKS = 10;
    // Waiting period for enabling freezing
    OMNI_FREEZE_WAIT_PERIOD = 10;
    // Script related:
    PUBKEYHASH_BLOCK = 0;
    SCRIPTHASH_BLOCK = 0;
    MULTISIG_BLOCK = 0;
    NULLDATA_BLOCK = 0;
    // Transaction restrictions:
    MSC_ALERT_BLOCK = 0;
    MSC_SEND_BLOCK = 0;
    MSC_DEX_BLOCK = 0;
    MSC_SP_BLOCK = 0;
    MSC_MANUALSP_BLOCK = 0;
    MSC_STO_BLOCK = 0;
    MSC_METADEX_BLOCK = 0;
    MSC_SEND_ALL_BLOCK = 0;
    MSC_BET_BLOCK = 999999;
    MSC_STOV1_BLOCK = 999999;
    MSC_ANYDATA_BLOCK = 0;
    MSC_NONFUNGIBLE_BLOCK = 0;
    MSC_DELEGATED_ISSUANCE_BLOCK = 0;
    // Other feature activations:
    GRANTEFFECTS_FEATURE_BLOCK = 999999;
    DEXMATH_FEATURE_BLOCK = 999999;
    SPCROWDCROSSOVER_FEATURE_BLOCK = 999999;
    TRADEALLPAIRS_FEATURE_BLOCK = 999999;
    FEES_FEATURE_BLOCK = 999999;
    FREEZENOTICE_FEATURE_BLOCK = 999999;
    FREEDEX_FEATURE_BLOCK = 999999;
}

//! Consensus parameters for mainnet
static CMainConsensusParams mainConsensusParams;
//! Consensus parameters for testnet
static CTestNetConsensusParams testNetConsensusParams;
//! Consensus parameters for regtest mode
static CRegTestConsensusParams regTestConsensusParams;

/**
 * Returns consensus parameters for the given network.
 */
CConsensusParams& ConsensusParams(const std::string& network)
{
    if (network == "main") {
        return mainConsensusParams;
    }
    if (network == "test") {
        return testNetConsensusParams;
    }
    if (network == "regtest") {
        return regTestConsensusParams;
    }
    // Fallback:
    return mainConsensusParams;
}

/**
 * Returns currently active consensus parameter.
 */
const CConsensusParams& ConsensusParams()
{
    const std::string& network = Params().NetworkIDString();

    return ConsensusParams(network);
}

/**
 * Returns currently active mutable consensus parameter.
 */
CConsensusParams& MutableConsensusParams()
{
    const std::string& network = Params().NetworkIDString();

    return ConsensusParams(network);
}

/**
 * Resets consensus parameters.
 */
void ResetConsensusParams()
{
    mainConsensusParams = CMainConsensusParams();
    testNetConsensusParams = CTestNetConsensusParams();
    regTestConsensusParams = CRegTestConsensusParams();
}

/**
 * Checks, if the script type is allowed as input.
 */
bool IsAllowedInputType(int whichType, int nBlock)
{
    const CConsensusParams& params = ConsensusParams();

    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return (params.PUBKEYHASH_BLOCK <= nBlock);

        case TX_SCRIPTHASH:
            return (params.SCRIPTHASH_BLOCK <= nBlock);
    }

    return false;
}

/**
 * Checks, if the script type qualifies as output.
 */
bool IsAllowedOutputType(int whichType, int nBlock)
{
    const CConsensusParams& params = ConsensusParams();

    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return (params.PUBKEYHASH_BLOCK <= nBlock);

        case TX_SCRIPTHASH:
            return (params.SCRIPTHASH_BLOCK <= nBlock);

        case TX_MULTISIG:
            return (params.MULTISIG_BLOCK <= nBlock);

        case TX_NULL_DATA:
            return (params.NULLDATA_BLOCK <= nBlock);
    }

    return false;
}

/**
 * Activates a feature at a specific block height, authorization has already been validated.
 *
 * Note: Feature activations are consensus breaking.  It is not permitted to activate a feature within
 *       the next 2048 blocks (roughly 2 weeks), nor is it permitted to activate a feature further out
 *       than 12288 blocks (roughly 12 weeks) to ensure sufficient notice.
 *       This does not apply for activation during initialization (where loadingActivations is set true).
 */
bool ActivateFeature(uint16_t featureId, int activationBlock, uint32_t minClientVersion, int transactionBlock)
{
    PrintToLog("Feature activation requested (ID %d to go active as of block: %d)\n", featureId, activationBlock);

    const CConsensusParams& params = ConsensusParams();

    // check activation block is allowed
    if ((activationBlock < (transactionBlock + params.MIN_ACTIVATION_BLOCKS)) ||
        (activationBlock > (transactionBlock + params.MAX_ACTIVATION_BLOCKS))) {
            PrintToLog("Feature activation of ID %d refused due to notice checks\n", featureId);
            return false;
    }

    // check whether the feature is already active
    if (IsFeatureActivated(featureId, transactionBlock)) {
        PrintToLog("Feature activation of ID %d refused as the feature is already live\n", featureId);
        return false;
    }

    // check feature is recognized and activation is successful
    std::string featureName = GetFeatureName(featureId);
    bool supported = OMNICORE_VERSION >= minClientVersion;
    switch (featureId) {
        case FEATURE_CLASS_C:
            MutableConsensusParams().NULLDATA_BLOCK = activationBlock;
        break;
        case FEATURE_METADEX:
            MutableConsensusParams().MSC_METADEX_BLOCK = activationBlock;
        break;
        case FEATURE_BETTING:
            MutableConsensusParams().MSC_BET_BLOCK = activationBlock;
        break;
        case FEATURE_GRANTEFFECTS:
            MutableConsensusParams().GRANTEFFECTS_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_DEXMATH:
            MutableConsensusParams().DEXMATH_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_SENDALL:
            MutableConsensusParams().MSC_SEND_ALL_BLOCK = activationBlock;
        break;
        case FEATURE_SPCROWDCROSSOVER:
            MutableConsensusParams().SPCROWDCROSSOVER_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_TRADEALLPAIRS:
            MutableConsensusParams().TRADEALLPAIRS_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_FEES:
            MutableConsensusParams().FEES_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_STOV1:
            MutableConsensusParams().MSC_STOV1_BLOCK = activationBlock;
        break;
        case FEATURE_FREEZENOTICE:
            MutableConsensusParams().FREEZENOTICE_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_FREEDEX:
            MutableConsensusParams().FREEDEX_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_NONFUNGIBLE:
            MutableConsensusParams().MSC_NONFUNGIBLE_BLOCK = activationBlock;
        break;
        case FEATURE_DELEGATEDISSUANCE:
            MutableConsensusParams().MSC_DELEGATED_ISSUANCE_BLOCK = activationBlock;
        break;
        default:
            supported = false;
        break;
    }

    PrintToLog("Feature activation of ID %d processed. %s will be enabled at block %d.\n", featureId, featureName, activationBlock);
    AddPendingActivation(featureId, activationBlock, minClientVersion, featureName);

    if (!supported) {
        PrintToLog("WARNING!!! AS OF BLOCK %d THIS CLIENT WILL BE OUT OF CONSENSUS AND WILL AUTOMATICALLY SHUTDOWN.\n", activationBlock);
        std::string alertText = strprintf("Your client must be updated and will shutdown at block %d (unsupported feature %d ('%s') activated)\n",
                                          activationBlock, featureId, featureName);
        AddAlert("omnicore", ALERT_BLOCK_EXPIRY, activationBlock, alertText);
        DoWarning(alertText);
    }

    return true;
}

/**
 * Deactivates a feature immediately, authorization has already been validated.
 *
 * Note: There is no notice period for feature deactivation as:
 *       # It is reserved for emergency use in the event an exploit is found
 *       # No client upgrade is required
 *       # No action is required by users
 */
bool DeactivateFeature(uint16_t featureId, int transactionBlock)
{
    PrintToLog("Immediate feature deactivation requested (ID %d)\n", featureId);

    if (!IsFeatureActivated(featureId, transactionBlock)) {
        PrintToLog("Feature deactivation of ID %d refused as the feature is not yet live\n", featureId);
        return false;
    }

    std::string featureName = GetFeatureName(featureId);
    switch (featureId) {
        case FEATURE_CLASS_C:
            MutableConsensusParams().NULLDATA_BLOCK = 999999;
        break;
        case FEATURE_METADEX:
            MutableConsensusParams().MSC_METADEX_BLOCK = 999999;
        break;
        case FEATURE_BETTING:
            MutableConsensusParams().MSC_BET_BLOCK = 999999;
        break;
        case FEATURE_GRANTEFFECTS:
            MutableConsensusParams().GRANTEFFECTS_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_DEXMATH:
            MutableConsensusParams().DEXMATH_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_SENDALL:
            MutableConsensusParams().MSC_SEND_ALL_BLOCK = 999999;
        break;
        case FEATURE_SPCROWDCROSSOVER:
            MutableConsensusParams().SPCROWDCROSSOVER_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_TRADEALLPAIRS:
            MutableConsensusParams().TRADEALLPAIRS_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_FEES:
            MutableConsensusParams().FEES_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_STOV1:
            MutableConsensusParams().MSC_STOV1_BLOCK = 999999;
        break;
        case FEATURE_FREEZENOTICE:
            MutableConsensusParams().FREEZENOTICE_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_FREEDEX:
            MutableConsensusParams().FREEDEX_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_NONFUNGIBLE:
            MutableConsensusParams().MSC_NONFUNGIBLE_BLOCK = 999999;
        break;
        case FEATURE_DELEGATEDISSUANCE:
            MutableConsensusParams().MSC_DELEGATED_ISSUANCE_BLOCK = 999999;
        break;
        default:
            return false;
        break;
    }

    PrintToLog("Feature deactivation of ID %d processed. %s has been disabled.\n", featureId, featureName);

    std::string alertText = strprintf("An emergency deactivation of feature ID %d (%s) has occurred.", featureId, featureName);
    AddAlert("omnicore", ALERT_BLOCK_EXPIRY, transactionBlock + 1024, alertText);
    DoWarning(alertText);

    return true;
}

/**
 * Returns the display name of a feature ID
 */
std::string GetFeatureName(uint16_t featureId)
{
    switch (featureId) {
        case FEATURE_CLASS_C: return "Class C transaction encoding";
        case FEATURE_METADEX: return "Distributed Meta Token Exchange";
        case FEATURE_BETTING: return "Bet transactions";
        case FEATURE_GRANTEFFECTS: return "Remove grant side effects";
        case FEATURE_DEXMATH: return "DEx integer math update";
        case FEATURE_SENDALL: return "Send All transactions";
        case FEATURE_SPCROWDCROSSOVER: return "Disable crowdsale ecosystem crossovers";
        case FEATURE_TRADEALLPAIRS: return "Allow trading all pairs on the Distributed Exchange";
        case FEATURE_FEES: return "Fee system (inc 0.05% fee from trades of non-Omni pairs)";
        case FEATURE_STOV1: return "Cross-property Send To Owners";
        case FEATURE_FREEZENOTICE: return "Activate the waiting period for enabling freezing";
        case FEATURE_FREEDEX: return "Activate trading of any token on the distributed exchange";
        case FEATURE_NONFUNGIBLE: return "Uniquely identifiable tokens";
        case FEATURE_DELEGATEDISSUANCE: return "Activate delegated issuance of tokens";

        default: return "Unknown feature";
    }
}

/**
 * Checks, whether a feature is activated at the given block.
 */
bool IsFeatureActivated(uint16_t featureId, int transactionBlock)
{
    const CConsensusParams& params = ConsensusParams();
    int activationBlock = std::numeric_limits<int>::max();

    switch (featureId) {
        case FEATURE_CLASS_C:
            activationBlock = params.NULLDATA_BLOCK;
            break;
        case FEATURE_METADEX:
            activationBlock = params.MSC_METADEX_BLOCK;
            break;
        case FEATURE_BETTING:
            activationBlock = params.MSC_BET_BLOCK;
            break;
        case FEATURE_GRANTEFFECTS:
            activationBlock = params.GRANTEFFECTS_FEATURE_BLOCK;
            break;
        case FEATURE_DEXMATH:
            activationBlock = params.DEXMATH_FEATURE_BLOCK;
            break;
        case FEATURE_SENDALL:
            activationBlock = params.MSC_SEND_ALL_BLOCK;
            break;
        case FEATURE_SPCROWDCROSSOVER:
            activationBlock = params.SPCROWDCROSSOVER_FEATURE_BLOCK;
            break;
        case FEATURE_TRADEALLPAIRS:
            activationBlock = params.TRADEALLPAIRS_FEATURE_BLOCK;
            break;
        case FEATURE_FEES:
            activationBlock = params.FEES_FEATURE_BLOCK;
            break;
        case FEATURE_STOV1:
            activationBlock = params.MSC_STOV1_BLOCK;
            break;
        case FEATURE_FREEZENOTICE:
            activationBlock = params.FREEZENOTICE_FEATURE_BLOCK;
        break;
        case FEATURE_FREEDEX:
            activationBlock = params.FREEDEX_FEATURE_BLOCK;
        break;
        case FEATURE_NONFUNGIBLE:
            activationBlock = params.MSC_NONFUNGIBLE_BLOCK;
        break;
        case FEATURE_DELEGATEDISSUANCE:
            activationBlock = params.MSC_DELEGATED_ISSUANCE_BLOCK;
        break;
        default:
            return false;
    }

    return (transactionBlock >= activationBlock);
}

/**
 * Checks, if the transaction type and version is supported and enabled.
 *
 * In the test ecosystem, transactions, which are known to the client are allowed
 * without height restriction.
 *
 * Certain transactions use a property identifier of 0 (= BTC) as wildcard, which
 * must explicitly be allowed.
 */
bool IsTransactionTypeAllowed(int txBlock, uint32_t txProperty, uint16_t txType, uint16_t version)
{
    const std::vector<TransactionRestriction>& vTxRestrictions = ConsensusParams().GetRestrictions();

    for (std::vector<TransactionRestriction>::const_iterator it = vTxRestrictions.begin(); it != vTxRestrictions.end(); ++it)
    {
        const TransactionRestriction& entry = *it;
        if (entry.txType != txType || entry.txVersion != version) {
            continue;
        }
        // a property identifier of 0 (= BTC) may be used as wildcard
        if (OMNI_PROPERTY_BTC == txProperty && !entry.allowWildcard) {
            continue;
        }
        // transactions are not restricted in the test ecosystem
        if (isTestEcosystemProperty(txProperty)) {
            return true;
        }
        if (txBlock >= entry.activationBlock) {
            return true;
        }
    }

    return false;
}

/**
 * Compares a supplied block, block hash and consensus hash against a hardcoded list of checkpoints.
 */
bool VerifyCheckpoint(int block, const uint256& blockHash)
{
    // optimization; we only checkpoint every 10,000 blocks - skip any further work if block not a multiple of 10K
    if (block % 10000 != 0) return true;

    const std::vector<ConsensusCheckpoint>& vCheckpoints = ConsensusParams().GetCheckpoints();

    for (std::vector<ConsensusCheckpoint>::const_iterator it = vCheckpoints.begin(); it != vCheckpoints.end(); ++it) {
        const ConsensusCheckpoint& checkpoint = *it;
        if (block != checkpoint.blockHeight) {
            continue;
        }

        if (blockHash != checkpoint.blockHash) {
            PrintToLog("%s(): block hash mismatch - expected %s, received %s\n", __func__, checkpoint.blockHash.GetHex(), blockHash.GetHex());
            return false;
        }

        // only verify if there is a checkpoint to verify against
        uint256 consensusHash = GetConsensusHash();
        if (consensusHash != checkpoint.consensusHash) {
            PrintToLog("%s(): consensus hash mismatch - expected %s, received %s\n", __func__, checkpoint.consensusHash.GetHex(), consensusHash.GetHex());
            return false;
        } else {
            break;
        }
    }

    // either checkpoint matched or we don't have a checkpoint for this block
    return true;
}

/**
 * Checks, if a specific transaction exists in the database.
 */
bool VerifyTransactionExistence(int block)
{
    PrintToLog("%s: verifying existence of historical transactions up to block %d..\n", __func__, block);

    const std::vector<TransactionCheckpoint>& vTransactionss = ConsensusParams().GetTransactions();

    for (std::vector<TransactionCheckpoint>::const_iterator it = vTransactionss.begin(); it != vTransactionss.end(); ++it) {
        const TransactionCheckpoint& checkpoint = *it;
        if (block < checkpoint.blockHeight) {
            continue;
        }

        if (!mastercore::pDbTransactionList->exists(checkpoint.txHash)) {
            PrintToLog("%s: ERROR: failed to find historical transaction %s in block %d\n",
                    __func__, checkpoint.txHash.GetHex(), checkpoint.blockHeight);
            return false;
        }
    }

    return true;
}

} // namespace mastercore
