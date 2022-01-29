#pragma once

#include <mw/common/Macros.h>
#include <mw/crypto/Pedersen.h>
#include <mw/models/block/Header.h>
#include <mw/models/tx/Transaction.h>
#include <mw/consensus/Aggregation.h>
#include <mw/mmr/MMR.h>
#include <mw/mmr/LeafSet.h>

#include <test_framework/models/MinedBlock.h>
#include <test_framework/models/Tx.h>
#include <test_framework/TestLeafSet.h>

#include <mweb/mweb_db.h>

TEST_NAMESPACE

class Miner
{
public:
    Miner(const FilePath& datadir)
        : m_datadir(datadir) { }

    MinedBlock MineBlock(const uint64_t height, const std::vector<Tx>& txs = {})
    {
        mw::Transaction::CPtr pTransaction = std::make_shared<const mw::Transaction>();
        if (!txs.empty()) {
            std::vector<mw::Transaction::CPtr> transactions;
            std::transform(
                txs.cbegin(), txs.cend(),
                std::back_inserter(transactions),
                [](const Tx& tx) { return tx.GetTransaction(); }
            );
            pTransaction = Aggregation::Aggregate(transactions);
        }

        auto kernel_offset = pTransaction->GetKernelOffset();
        if (!m_blocks.empty()) {
            kernel_offset = Pedersen::AddBlindingFactors({ kernel_offset, m_blocks.back().GetKernelOffset() });
        }

        auto stealth_offset = pTransaction->GetStealthOffset();

        auto kernelMMR = GetKernelMMR(pTransaction->GetKernels());
        auto outputMMR = GetOutputMMR(pTransaction->GetOutputs());
        auto pLeafSet = GetLeafSet(pTransaction->GetInputs(), pTransaction->GetOutputs());

        auto pHeader = std::make_shared<mw::Header>(
            height,
            outputMMR->Root(),
            kernelMMR->Root(),
            pLeafSet->Root(),
            std::move(kernel_offset),
            std::move(stealth_offset),
            outputMMR->GetNumLeaves(),
            kernelMMR->GetNumLeaves()
        );

        MinedBlock minedBlock(
            std::make_shared<mw::Block>(pHeader, pTransaction->GetBody()),
            txs
        );
        m_blocks.push_back(minedBlock);

        return minedBlock;
    }

    void Rewind(const size_t nextHeight)
    {
        m_blocks.erase(m_blocks.begin() + nextHeight, m_blocks.end());
    }

private:
    IMMR::Ptr GetKernelMMR(const std::vector<Kernel>& tx_kernels)
    {
        MemMMR::Ptr pKernelMMR = std::make_shared<MemMMR>();
        for (const Kernel& kernel : tx_kernels) {
            pKernelMMR->Add(kernel.Serialized());
        }

        return pKernelMMR;
    }

    IMMR::Ptr GetOutputMMR(const std::vector<Output>& additionalOutputs = {})
    {
        std::vector<Output> outputs;
        for (const auto& block : m_blocks) {
            const auto& blockOutputs = block.GetBlock()->GetOutputs();
            std::copy(blockOutputs.cbegin(), blockOutputs.cend(), std::back_inserter(outputs));
        }

        std::copy(additionalOutputs.cbegin(), additionalOutputs.cend(), std::back_inserter(outputs));

        MemMMR::Ptr pOutputMMR = std::make_shared<MemMMR>();
        for (const Output& output : outputs) {
            pOutputMMR->Add(output.GetOutputID());
        }

        return pOutputMMR;
    }

    TestLeafSet::Ptr GetLeafSet(const std::vector<Input>& additionalInputs = {}, const std::vector<Output>& additionalOutputs = {})
    {
        std::vector<TestLeafSet::BlockInfo> blockInfos;
        for (const MinedBlock& block : m_blocks) {
            TestLeafSet::BlockInfo blockInfo;
            for (const Input& input : block.GetBlock()->GetInputs()) {
                blockInfo.inputs.push_back(input.GetCommitment());
            }

            for (const Output& output : block.GetBlock()->GetOutputs()) {
                blockInfo.outputs.push_back(output.GetCommitment());
            }

            blockInfos.push_back(blockInfo);
        }

        TestLeafSet::BlockInfo blockInfo;
        for (const Input& input : additionalInputs) {
            blockInfo.inputs.push_back(input.GetCommitment());
        }

        for (const Output& output : additionalOutputs) {
            blockInfo.outputs.push_back(output.GetCommitment());
        }

        blockInfos.push_back(blockInfo);

        return TestLeafSet::Create(m_datadir, blockInfos);
    }

    FilePath m_datadir;
    std::vector<MinedBlock> m_blocks;
};

END_NAMESPACE