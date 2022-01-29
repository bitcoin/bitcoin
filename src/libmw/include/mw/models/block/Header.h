#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Macros.h>
#include <mw/common/Traits.h>
#include <mw/crypto/Hasher.h>
#include <mw/models/crypto/BlindingFactor.h>
#include <mw/models/crypto/Hash.h>

#include <cstdint>
#include <memory>

MW_NAMESPACE

class Header final :
    public Traits::IPrintable,
    public Traits::ISerializable,
    public Traits::IHashable
{
public:
    using CPtr = std::shared_ptr<const Header>;

    //
    // Constructors
    //
    Header() = default;
    Header(
        const int32_t height,
        mw::Hash outputRoot,
        mw::Hash kernelRoot,
		mw::Hash leafsetRoot,
        BlindingFactor kernelOffset,
        BlindingFactor stealthOffset,
        const uint64_t outputMMRSize,
        const uint64_t kernelMMRSize
    )
        : m_height(height),
        m_outputRoot(std::move(outputRoot)),
        m_kernelRoot(std::move(kernelRoot)),
		m_leafsetRoot(std::move(leafsetRoot)),
        m_kernelOffset(std::move(kernelOffset)),
        m_stealthOffset(std::move(stealthOffset)),
        m_outputMMRSize(outputMMRSize),
        m_kernelMMRSize(kernelMMRSize)
    {
        m_hash = Hashed(*this);
    }

    //
    // Operators
    //
    bool operator==(const Header& rhs) const noexcept { return this->GetHash() == rhs.GetHash(); }
    bool operator!=(const Header& rhs) const noexcept { return this->GetHash() != rhs.GetHash(); }

    //
    // Getters
    //
    int32_t GetHeight() const noexcept { return m_height; }
    const mw::Hash& GetOutputRoot() const noexcept { return m_outputRoot; }
    const mw::Hash& GetKernelRoot() const noexcept { return m_kernelRoot; }
	const mw::Hash& GetLeafsetRoot() const noexcept { return m_leafsetRoot; }
    const BlindingFactor& GetKernelOffset() const noexcept { return m_kernelOffset; }
    const BlindingFactor& GetStealthOffset() const noexcept { return m_stealthOffset; }
    uint64_t GetNumTXOs() const noexcept { return m_outputMMRSize; }
    uint64_t GetNumKernels() const noexcept { return m_kernelMMRSize; }

    //
    // Traits
    //
    const mw::Hash& GetHash() const noexcept final { return m_hash; }

    std::string Format() const final { return GetHash().ToHex(); }

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZABLE(Header, obj)
    {
        READWRITE(VARINT_MODE(obj.m_height, VarIntMode::NONNEGATIVE_SIGNED));
        READWRITE(obj.m_outputRoot);
        READWRITE(obj.m_kernelRoot);
        READWRITE(obj.m_leafsetRoot);
        READWRITE(obj.m_kernelOffset);
        READWRITE(obj.m_stealthOffset);
        READWRITE(VARINT(obj.m_outputMMRSize));
        READWRITE(VARINT(obj.m_kernelMMRSize));
        SER_READ(obj, obj.m_hash = Hashed(obj));
    }

private:
    int32_t m_height;
    mw::Hash m_outputRoot;
    mw::Hash m_kernelRoot;
	mw::Hash m_leafsetRoot;
    BlindingFactor m_kernelOffset;
    BlindingFactor m_stealthOffset;
    uint64_t m_outputMMRSize;
    uint64_t m_kernelMMRSize;

    mw::Hash m_hash;
};

class MutHeader
{
public:
    //
    // Constructors
    //
    MutHeader() = default;
    MutHeader(const Header::CPtr& pHeader)
        : m_height(pHeader->GetHeight()),
          m_outputRoot(pHeader->GetOutputRoot()),
          m_kernelRoot(pHeader->GetKernelRoot()),
          m_leafsetRoot(pHeader->GetLeafsetRoot()),
          m_kernelOffset(pHeader->GetKernelOffset()),
          m_stealthOffset(pHeader->GetStealthOffset()),
          m_numOutputs(pHeader->GetNumTXOs()),
          m_numKernels(pHeader->GetNumKernels()) { }

    MutHeader& SetHeight(const int32_t height) noexcept
    {
        m_height = height;
        return *this;
    }

    MutHeader& SetOutputRoot(const mw::Hash& output_root) noexcept
    {
        m_outputRoot = output_root;
        return *this;
    }

    MutHeader& SetKernelRoot(const mw::Hash& kernel_root) noexcept
    {
        m_kernelRoot = kernel_root;
        return *this;
    }

    MutHeader& SetLeafsetRoot(const mw::Hash& leafset_root) noexcept
    {
        m_leafsetRoot = leafset_root;
        return *this;
    }

    MutHeader& SetKernelOffset(const mw::Hash& kernel_offset) noexcept
    {
        m_kernelOffset = kernel_offset;
        return *this;
    }

    MutHeader& SetStealthOffset(const mw::Hash& stealth_offset) noexcept
    {
        m_stealthOffset = stealth_offset;
        return *this;
    }

    MutHeader& SetNumOutputs(const uint64_t num_outputs) noexcept
    {
        m_numOutputs = num_outputs;
        return *this;
    }

    MutHeader& SetNumKernels(const uint64_t num_kernels) noexcept
    {
        m_numKernels = num_kernels;
        return *this;
    }

    Header::CPtr Build() const
    {
        return std::make_shared<mw::Header>(
            m_height,
            m_outputRoot,
            m_kernelRoot,
            m_leafsetRoot,
            m_kernelOffset,
            m_stealthOffset,
            m_numOutputs,
            m_numKernels
        );
    }

private:
    int32_t m_height;
    mw::Hash m_outputRoot;
    mw::Hash m_kernelRoot;
    mw::Hash m_leafsetRoot;
    BlindingFactor m_kernelOffset;
    BlindingFactor m_stealthOffset;
    uint64_t m_numOutputs;
    uint64_t m_numKernels;
};

END_NAMESPACE