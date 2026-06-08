#include "bitcoinkernel.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
 
void handle_error(void* _, const char* msg, size_t msg_size)
{
    printf("%s\n", msg);
    exit(0);
}
 
int main() {
    btck_ContextOptions* context_options = btck_context_options_create();
    btck_NotificationInterfaceCallbacks notifications = {
        .user_data = NULL,
        .block_tip = NULL,
        .header_tip = NULL,
        .progress = NULL,
        .warning_set = NULL,
        .warning_unset = NULL,
        .flush_error = handle_error,
        .fatal_error = handle_error,
    };
    btck_context_options_set_notifications(context_options, notifications);
    btck_ChainParameters* chainparams = btck_chain_parameters_create(btck_ChainType_MAINNET);
    btck_context_options_set_chainparams(context_options, chainparams);
    btck_chain_parameters_destroy(chainparams);
    btck_Context* context = btck_context_create(context_options);
    btck_context_options_destroy(context_options);
    if (context == NULL) return 1;
 
    const char data_dir[] = ".bitcoin";
    const char blocks_dir[] = ".bitcoin/blocks";
    btck_ChainstateManagerOptions* chainman_options = btck_chainstate_manager_options_create(context, data_dir, sizeof(data_dir) - 1, blocks_dir, sizeof(blocks_dir) - 1);
    if (chainman_options == NULL) return 2;
    btck_ChainstateManager* chainman = btck_chainstate_manager_create(chainman_options);
    if (chainman == NULL) return 3;
    btck_chainstate_manager_options_destroy(chainman_options);
 
    const btck_Chain* chain = btck_chainstate_manager_get_active_chain(chainman);
    const btck_BlockTreeEntry* entry = btck_chain_get_by_height(chain, 0);
    btck_Block* genesis = btck_block_read(chainman, entry);
 
    btck_BlockHash* hash = btck_block_get_hash(genesis);
    unsigned char raw_hash[32];
    btck_block_hash_to_bytes(hash, raw_hash);
 
    unsigned char expected_hash[32] = {
        0x6f, 0xe2, 0x8c, 0x0a, 0xb6, 0xf1, 0xb3, 0x72,
        0xc1, 0xa6, 0xa2, 0x46, 0xae, 0x63, 0xf7, 0x4f,
        0x93, 0x1e, 0x83, 0x65, 0xe1, 0x5a, 0x08, 0x9c,
        0x68, 0xd6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    if (memcmp(raw_hash, expected_hash, 32) != 0) {
        return 4;
    }
 
    btck_block_hash_destroy(hash);
    btck_block_destroy(genesis);
    btck_chainstate_manager_destroy(chainman);
    btck_context_destroy(context);
 
    return 0;
}
