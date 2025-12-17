/* This module defines primitives and jets that are specific to the Bitcoin application for Simplicity.
 */
#ifndef SIMPLICITY_BITCOIN_BITCOINJETS_H
#define SIMPLICITY_BITCOIN_BITCOINJETS_H

#include "../jets.h"

/* Jets for the Bitcoin application of Simplicity. */
bool simplicity_bitcoin_version(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_lock_time(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_prev_outpoint(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_value(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_script_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_sequence(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_annex_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_script_sig_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_output_value(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_output_script_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_fee(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_total_input_value(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_total_output_value(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_script_cmr(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_transaction_id(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_current_index(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_current_prev_outpoint(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_current_value(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_current_script_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_current_sequence(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_current_annex_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_current_script_sig_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tapleaf_version(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tappath(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_internal_key(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_num_inputs(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_num_outputs(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tx_is_final(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tx_lock_height(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tx_lock_time(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tx_lock_distance(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tx_lock_duration(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_check_lock_height(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_check_lock_time(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_check_lock_distance(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_check_lock_duration(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_build_tapleaf_simplicity(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_build_tapbranch(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_build_taptweak(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_outpoint_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_annex_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_output_values_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_output_scripts_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_outputs_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_output_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_outpoints_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_values_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_scripts_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_utxos_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_utxo_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_sequences_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_annexes_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_script_sigs_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_inputs_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_input_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tx_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tapleaf_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tappath_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_tap_env_hash(frameItem* dst, frameItem src, const txEnv* env);
bool simplicity_bitcoin_sig_all_hash(frameItem* dst, frameItem src, const txEnv* env);

#endif
