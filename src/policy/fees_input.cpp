// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees_input.h>

#include <chainparams.h>
#include <clientversion.h>
#include <policy/fees.h>
#include <streams.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/time.h>
#include <univalue.h>

#include <fstream>

namespace {
UniValue TxLog(const uint256& hash, unsigned int height, CAmount fee, uint32_t size)
{
    UniValue tx(UniValue::VOBJ);
    tx.pushKV("hash", hash.ToString());
    tx.pushKV("height", int(height));
    tx.pushKV("fee", fee);
    tx.pushKV("size", uint64_t(size));
    return tx;
}
} // namespace

FeeEstInput::FeeEstInput(CBlockPolicyEstimator& estimator) : m_estimator(estimator) {}

bool FeeEstInput::open(const fs::path& estimation_filepath, const fs::path& log_filepath) {
    if (!writeLog(log_filepath)) return false;
    m_estimation_filepath = estimation_filepath;

    // If the fee estimation file is present, read recorded estimations
    if (!readData(m_estimation_filepath)) {
        LogPrintf("Failed to read fee estimates from %s. Continue anyway.\n", fs::PathToString(m_estimation_filepath));
    }

    return true;
}

bool FeeEstInput::close()
{
    m_estimator.FlushUnconfirmed();

    if (!writeData(m_estimation_filepath)) {
        LogPrintf("Failed to write fee estimates to %s. Continue anyway.\n", fs::PathToString(m_estimation_filepath));
    }
    return writeLog({});
}

void FeeEstInput::processTx(const uint256& hash, unsigned int height, CAmount fee, uint32_t size, bool valid)
{
    m_estimator.processTx(hash, height, fee, size, valid);
    if (m_log) {
        UniValue value(UniValue::VOBJ);
        value.pushKV("tx", TxLog(hash, height, fee, size));
        value.pushKV("valid", UniValue(valid));
        value.pushKV("time", GetTime());
        *m_log << value.write() << std::endl;
    }
}

void FeeEstInput::processBlock(unsigned int block_height, const AddTxsFn& add_txs)
{
    UniValue json_txs{UniValue::VARR};
    m_estimator.processBlock(block_height, [&](const AddTxFn& add_tx) {
        add_txs([&](const uint256& hash, unsigned int height, CAmount fee, uint32_t size) {
            add_tx(hash, height, fee, size);
            if (m_log) json_txs.push_back(TxLog(hash, height, fee, size));
        });
    });
    if (m_log) {
        UniValue json(UniValue::VOBJ);
        UniValue json_block(UniValue::VOBJ);
        json_block.pushKV("height", int64_t(block_height));
        json.pushKV("block", json_block);
        json.pushKV("txs", std::move(json_txs));
        json.pushKV("time", GetTime());
        *m_log << json.write() << std::endl;
    }
}

void FeeEstInput::removeTx(const uint256& hash, bool in_block)
{
    m_estimator.removeTx(hash, in_block);

    if (m_log) {
        UniValue value(UniValue::VOBJ);
        UniValue removeTx(UniValue::VOBJ);
        removeTx.pushKV("hash", hash.ToString());
        removeTx.pushKV("inBlock", UniValue(in_block));
        value.pushKV("removeTx", removeTx);
        value.pushKV("time", GetTime());
        *m_log << value.write() << std::endl;
    }
}

bool FeeEstInput::writeData(const fs::path& filepath)
{
    AutoFile file{fsbridge::fopen(filepath, "wb")};
    if (file.IsNull()) return false;
    m_estimator.Write(file);

    if (m_log) {
        UniValue value(UniValue::VOBJ);
        UniValue flush(UniValue::VARR);
        value.pushKV("flush", flush);
        value.pushKV("time", GetTime());
        *m_log << value.write() << std::endl;
    }
    return true;
}

bool FeeEstInput::readData(const fs::path& filepath)
{
    AutoFile file{fsbridge::fopen(filepath, "rb")};
    if (file.IsNull() || !m_estimator.Read(file)) return false;

    if (m_log) {
        UniValue value(UniValue::VOBJ);
        std::ifstream data(filepath, std::ifstream::binary);
        value.pushKV(
            "read", HexStr(std::string(std::istreambuf_iterator<char>(data), std::istreambuf_iterator<char>())));
        value.pushKV("time", GetTime());
        *m_log << value.write() << std::endl;
    }
    return true;
}

bool FeeEstInput::writeLog(const fs::path& filepath)
{
    if (m_log) {
        UniValue value(UniValue::VOBJ);
        value.pushKV("stop", Params().NetworkIDString());
        value.pushKV("time", GetTime());
        *m_log << value.write() << std::endl;
    }

    if (filepath.empty()) {
        m_log.reset();
    } else {
        m_log = std::make_unique<std::ofstream>(filepath, std::ofstream::out | std::ofstream::app);
        if (!*m_log) {
            m_log.reset();
            return false;
        }
    }

    if (m_log) {
        UniValue value(UniValue::VOBJ);
        value.pushKV("start", Params().NetworkIDString());
        value.pushKV("time", GetTime());
        *m_log << value.write() << std::endl;
    }

    return true;
}

bool FeeEstInput::readLog(const fs::path& filepath, const std::function<bool(UniValue&)>& filter)
{
    std::ifstream file(filepath);
    if (!file) {
        LogPrintf("%s: Failed to open log file %s\n", __func__, fs::PathToString(filepath));
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        UniValue value;
        if (!value.read(line)) {
            throw std::runtime_error("Failed to parse fee estimate log line.");
        }

        if (filter && !filter(value)) {
            continue;
        }

        const UniValue& tx = value["tx"];
        if (tx.isObject()) {
            m_estimator.processTx(uint256S(tx["hash"].get_str()), tx["height"].getInt<int>(), tx["fee"].getInt<int64_t>(),
                tx["size"].getInt<uint32_t>(), value["valid"].get_bool());
            continue;
        }

        const UniValue& block = value["block"];
        if (block.isObject()) {
            int height = block["height"].getInt<int>();
            m_estimator.processBlock(height, [&](const AddTxFn& add_tx) {
                const auto& block_txs = value["txs"].getValues();
                for (const UniValue& block_tx : block_txs) {
                    add_tx(uint256S(block_tx["hash"].get_str()), block_tx["height"].getInt<int>(),
                        block_tx["fee"].getInt<int64_t>(), block_tx["size"].getInt<uint32_t>());
                }
                return block_txs.size();
            });
            continue;
        }

        const UniValue& removeTx = value["removeTx"];
        if (removeTx.isObject()) {
            m_estimator.removeTx(uint256S(removeTx["hash"].get_str()), removeTx["inBlock"].get_bool());
            continue;
        }

        const UniValue& flush = value["flush"];
        if (flush.isArray()) {
            m_estimator.FlushUnconfirmed();
            continue;
        }

        const UniValue& read = value["read"];
        if (read.isStr()) {
            std::vector<unsigned char> data = ParseHex(read.get_str());
            uint16_t randv = 0;
            GetRandBytes(Span{(unsigned char*)&randv, sizeof(randv)});
            fs::path data_filepath = fs::PathFromString(strprintf("fee_estimates.tmp.%04x", randv));
            CAutoFile(fsbridge::fopen(data_filepath, "wb"), SER_DISK, CLIENT_VERSION).write(MakeByteSpan(data));
            {
                CAutoFile data(fsbridge::fopen(data_filepath, "rb"), SER_DISK, CLIENT_VERSION);
                m_estimator.Read(data);
            }
            fs::remove(data_filepath);
            continue;
        }
    }

    if (file.bad()) {
        LogPrintf("%s: Failure reading log file %s\n", __func__, fs::PathToString(filepath));
        return false;
    }

    return true;
}
