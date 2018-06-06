// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_LAYER_H
#define BITCOIN_VALIDATION_LAYER_H

#include <future>

#include <chainparams.h>
#include <core/async_layer.h>
#include <core/consumerthread.h>
#include <core/producerconsumerqueue.h>
#include <util.h>

class ValidationLayer;
extern std::unique_ptr<ValidationLayer> g_validation_layer;

/**
 * Encapsulates a request to validate an object (currently only a block)
 * Submitted to ValidationLayer for asynchronous validation
 *
 * @see ValidationLayer
 */
template <typename RESPONSE>
class ValidationRequest : public AsyncRequest<RESPONSE>
{
    friend ValidationLayer;

public:
    //! Returns a string identifier (for logging)
    virtual std::string GetId() const override = 0;

protected:
    //! Guts of the validation
    virtual void operator()() override = 0;
};

/**
 * Holds the results of asynchronous block validation
 */
struct BlockValidationResponse {
    //! Is this the first time this block has been validated
    const bool is_new;

    //! Did initial validation pass (a block can still pass initial validation but then later fail to connect to an active chain)
    const bool block_valid;

    BlockValidationResponse(bool _block_valid, bool _is_new)
        : is_new(_is_new), block_valid(_block_valid){};
};

/**
 * Encapsulates a request to validate a block
 */
class BlockValidationRequest : public ValidationRequest<BlockValidationResponse>
{
    friend ValidationLayer;

public:
    //! Returns a block hash
    std::string GetId() const override;

protected:
    //! Does the validation
    void operator()() override;

private:
    BlockValidationRequest(ValidationLayer& validation_layer, const std::shared_ptr<const CBlock> block, bool force_processing, const std::function<void()> on_ready)
        : m_validation_layer(validation_layer), m_block(block), m_force_processing(force_processing), m_on_ready(on_ready){};

    const ValidationLayer& m_validation_layer;

    //! The block to be validated
    const std::shared_ptr<const CBlock> m_block;

    //! Was this block explicitly requested (currently required by ProcessNewBlock)
    const bool m_force_processing;

    //! A callback to invoke when ready
    //! This is a workaround because c++11 does not support multiplexed waiting on futures
    //! In a move to subsequent standards when this behavior is supported this can probably be removed
    const std::function<void()> m_on_ready;
};

/**
 * Public interface to block validation
 *
 * Two apis:
 *  - asynchronous: SubmitForValidation(object) -> future<Response>
 *  - synchronous:  Validate(object) -> Response (just calls SubmitForValidation and blocks on the response)
 *
 * Internally, a validation thread pulls validations requests from a queue, processes them and satisfies the promise
 * with the result of validation.
 */
class ValidationLayer : public AsyncLayer
{
    friend BlockValidationRequest;

public:
    ValidationLayer(const CChainParams& chainparams)
        : AsyncLayer(100), m_chainparams(chainparams) {}
    ~ValidationLayer(){};

    //! Submit a block for asynchronous validation
    std::future<BlockValidationResponse> SubmitForValidation(const std::shared_ptr<const CBlock> block, bool force_processing, std::function<void()> on_ready = []() {});

    //! Submit a block for validation and block on the response
    BlockValidationResponse Validate(const std::shared_ptr<const CBlock> block, bool force_processing);

private:
    //! Internal utility method - sets up and calls ProcessNewBlock
    BlockValidationResponse ValidateInternal(const std::shared_ptr<const CBlock> block, bool force_processing) const;

    const CChainParams& m_chainparams;
};

#endif
