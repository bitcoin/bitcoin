// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <algorithm>
#include <cassert>
#include <ios>
#include <optional>
#include <vector>

/** Deserialization of a CScriptWitness behaves exactly like that of a
 *  std::vector<std::vector<unsigned char>>, and its accessors and serialization agree with that
 *  vector. */
FUZZ_TARGET(script_witness_deserialize)
{
    // Deserialize the input as a vector of vector of unsigned chars.
    size_t stack_left{0};
    std::optional<std::vector<std::vector<unsigned char>>> stack;
    {
        SpanReader reader{buffer};
        try {
            std::vector<std::vector<unsigned char>> tmp;
            reader >> tmp;
            stack = std::move(tmp);
            stack_left = reader.size();
        } catch (const std::ios_base::failure&) {
        }
    }

    // Start over, and deserialize as a CScriptWitness.
    std::optional<CScriptWitness> witness;
    size_t witness_left{0};
    {
        SpanReader reader{buffer};
        try {
            CScriptWitness tmp;
            reader >> tmp;
            witness = std::move(tmp);
            witness_left = reader.size();
        } catch (const std::ios_base::failure&) {
        }
    }

    // Check whether deserialization success matches between them.
    assert(stack.has_value() == witness.has_value());

    // If deserialization succeeded (in both), compare equivalence in behavior.
    if (!stack) return;
    // Make sure the number of consumed bytes matches.
    assert(stack_left == witness_left);
    // Check equivalence after converting CScriptWitness to vector of vectors.
    assert(witness->ToVectors() == *stack);
    // Check size/empty.
    assert(witness->size() == stack->size());
    assert(witness->empty() == stack->empty());
    // Check equivalence after converting vector of vectors to CScriptWitness.
    assert(CScriptWitness{*stack} == *witness);
    if (!stack->empty()) {
        // Compare first and last element.
        assert(std::ranges::equal(witness->front(), stack->front()));
        assert(std::ranges::equal(witness->back(), stack->back()));
        // A copy has the same last element.
        assert(std::ranges::equal(CScriptWitness{*witness}.back(), stack->back()));
        // Compare elements accessed by index: all of them if there are few (as indexing may take time linear in the
        // index), and otherwise those at positions read from the rest of the input, after the serialized witness.
        if (stack->size() <= 64) {
            for (size_t i = 0; i < stack->size(); ++i) {
                assert(std::ranges::equal((*witness)[i], (*stack)[i]));
            }
        } else {
            FuzzedDataProvider provider{buffer.data() + buffer.size() - witness_left, witness_left};
            LIMITED_WHILE(provider.remaining_bytes() > 0, 64) {
                const auto i = provider.ConsumeIntegralInRange<size_t>(0, stack->size() - 1);
                assert(std::ranges::equal((*witness)[i], (*stack)[i]));
            }
        }
    }

    // Check reserialization.
    std::vector<unsigned char> ser_stack, ser_witness;
    VectorWriter{ser_stack, 0, *stack};
    VectorWriter{ser_witness, 0, *witness};
    assert(ser_stack == ser_witness);
    assert(::GetSerializeSize(*witness) == ser_stack.size());
    // Sanity check memory usage.
    assert(witness->DynamicMemoryUsage() >= (stack->empty() ? 0 : ser_stack.size()));
}
