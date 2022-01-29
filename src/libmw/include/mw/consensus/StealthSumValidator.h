#pragma once

#include <mw/crypto/PublicKeys.h>
#include <mw/exceptions/ValidationException.h>
#include <mw/models/crypto/BlindingFactor.h>
#include <mw/models/tx/TxBody.h>

class StealthSumValidator
{
public:
    //
    // Verifies that stealth excesses balance:
    // 
    // sum(K_s) + sum(K_i) - sum(K_o) = sum(E') + x'*G
    //
    static void Validate(const BlindingFactor& stealth_offset, const TxBody& body)
    {
        std::vector<PublicKey> lhs_keys;

        //
        // sum(K_s) + sum(K_i)
        //
        std::transform(
            body.GetOutputs().cbegin(), body.GetOutputs().cend(), std::back_inserter(lhs_keys),
            [](const Output& output) { return output.GetSenderPubKey(); }
        );

        for (const Input& input : body.GetInputs()) {
            if (!!input.GetInputPubKey()) {
                lhs_keys.push_back(*input.GetInputPubKey());
            }
        }

        PublicKey lhs_total;
        if (!lhs_keys.empty()) {
            lhs_total = PublicKeys::Add(lhs_keys);
        }
        
        //
        // sum(E') + x'*G + sum(K_o)
        //
        std::vector<PublicKey> rhs_keys = body.GetStealthExcesses();

        std::transform(
            body.GetInputs().cbegin(), body.GetInputs().cend(), std::back_inserter(rhs_keys),
            [](const Input& input) { return input.GetOutputPubKey(); }
        );


        if (!stealth_offset.IsZero()) {
            rhs_keys.push_back(PublicKeys::Calculate(stealth_offset.GetBigInt()));
        }

        PublicKey rhs_total;
        if (!rhs_keys.empty()) {
            rhs_total = PublicKeys::Add(rhs_keys);
        }

        // sum(K_s) + sum(K_i) = sum(E') + x'*G + sum(K_o)
        if (lhs_total != rhs_total) {
            ThrowValidation(EConsensusError::STEALTH_SUMS);
        }
    }
};