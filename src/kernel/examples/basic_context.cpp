#include <cstdio>
#include "bitcoinkernel.h"

int main()
{
    // Create options for the kernel context
    btck_ContextOptions* opts = btck_context_options_create();
    if (!opts) {
        std::fprintf(stderr, "Failed to create context options\n");
        return 1;
    }

    // Create regtest chain parameters
    btck_ChainParameters* params = btck_chain_parameters_create(btck_ChainType_REGTEST);
    if (!params) {
        std::fprintf(stderr, "Failed to create chain parameters\n");
        btck_context_options_destroy(opts);
        return 1;
    }

    // Set the chain parameters (options keeps an internal copy)
    btck_context_options_set_chainparams(opts, params);
    btck_chain_parameters_destroy(params);

    // Create the kernel context
    btck_Context* ctx = btck_context_create(opts);
    btck_context_options_destroy(opts);
    if (!ctx) {
        std::fprintf(stderr, "Failed to create kernel context\n");
        return 1;
    }

    // Use the context here...
    std::puts("Successfully created Bitcoin kernel context with regtest parameters ✔");

    // Clean up
    btck_context_destroy(ctx);

    std::puts("libbitcoinkernel C API basic usage complete.");
    return 0;
}