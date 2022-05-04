#pragma once

#include <mw/common/Macros.h>
#include <cstddef>
#include <cstdint>

MW_NAMESPACE

/// <summary>
/// Consensus parameters
/// Any change to these will cause a hardfork!
/// </summary>
static constexpr size_t BYTES_PER_WEIGHT = 42;

static constexpr size_t BASE_KERNEL_WEIGHT = 2;
static constexpr size_t STEALTH_EXCESS_WEIGHT = 1;
static constexpr size_t KERNEL_WITH_STEALTH_WEIGHT = BASE_KERNEL_WEIGHT + STEALTH_EXCESS_WEIGHT;

static constexpr size_t BASE_OUTPUT_WEIGHT = 17;
static constexpr size_t STANDARD_OUTPUT_FIELDS_WEIGHT = 1;
static constexpr size_t STANDARD_OUTPUT_WEIGHT = BASE_OUTPUT_WEIGHT + STANDARD_OUTPUT_FIELDS_WEIGHT;

static constexpr size_t MAX_NUM_INPUTS = 50'000;
static constexpr size_t INPUT_BYTES = 196;

static constexpr std::size_t MAX_BLOCK_WEIGHT = 200'000; // Nodes won't accept blocks over this weight
static constexpr std::size_t MAX_MINE_WEIGHT = 20'000; // Miners won't create blocks over this weight. Non-consensus
static constexpr size_t MAX_BLOCK_BYTES = 180 + (3 * 5) +                  // 180 bytes per header and 5 bytes each for input, output, and kernel vec size
                                         (MAX_NUM_INPUTS * INPUT_BYTES) + // 50k inputs at 196 bytes each (ignoring extradata)
                                         (MAX_BLOCK_WEIGHT * 60);     // Ignoring inputs, no tx component is ever more than 60 bytes per unit of weight 

END_NAMESPACE