/* This file has been automatically generated. */

{
  int32_t code;
  code = simplicity_decodeUptoMaxInt(stream);
  if (code < 0) return (simplicity_err)code;
  switch (code) {
    case 1:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1: *result = SIG_ALL_HASH; return SIMPLICITY_NO_ERROR;
        case 2: *result = TX_HASH; return SIMPLICITY_NO_ERROR;
        case 3: *result = TAP_ENV_HASH; return SIMPLICITY_NO_ERROR;
        case 4: *result = OUTPUTS_HASH; return SIMPLICITY_NO_ERROR;
        case 5: *result = INPUTS_HASH; return SIMPLICITY_NO_ERROR;
        case 6: *result = ISSUANCES_HASH; return SIMPLICITY_NO_ERROR;
        case 7: *result = INPUT_UTXOS_HASH; return SIMPLICITY_NO_ERROR;
        case 8: *result = OUTPUT_HASH; return SIMPLICITY_NO_ERROR;
        case 9: *result = OUTPUT_AMOUNTS_HASH; return SIMPLICITY_NO_ERROR;
        case 10: *result = OUTPUT_SCRIPTS_HASH; return SIMPLICITY_NO_ERROR;
        case 11: *result = OUTPUT_NONCES_HASH; return SIMPLICITY_NO_ERROR;
        case 12: *result = OUTPUT_RANGE_PROOFS_HASH; return SIMPLICITY_NO_ERROR;
        case 13: *result = OUTPUT_SURJECTION_PROOFS_HASH; return SIMPLICITY_NO_ERROR;
        case 14: *result = INPUT_HASH; return SIMPLICITY_NO_ERROR;
        case 15: *result = INPUT_OUTPOINTS_HASH; return SIMPLICITY_NO_ERROR;
        case 16: *result = INPUT_SEQUENCES_HASH; return SIMPLICITY_NO_ERROR;
        case 17: *result = INPUT_ANNEXES_HASH; return SIMPLICITY_NO_ERROR;
        case 18: *result = INPUT_SCRIPT_SIGS_HASH; return SIMPLICITY_NO_ERROR;
        case 19: *result = ISSUANCE_HASH; return SIMPLICITY_NO_ERROR;
        case 20: *result = ISSUANCE_ASSET_AMOUNTS_HASH; return SIMPLICITY_NO_ERROR;
        case 21: *result = ISSUANCE_TOKEN_AMOUNTS_HASH; return SIMPLICITY_NO_ERROR;
        case 22: *result = ISSUANCE_RANGE_PROOFS_HASH; return SIMPLICITY_NO_ERROR;
        case 23: *result = ISSUANCE_BLINDING_ENTROPY_HASH; return SIMPLICITY_NO_ERROR;
        case 24: *result = INPUT_UTXO_HASH; return SIMPLICITY_NO_ERROR;
        case 25: *result = INPUT_AMOUNTS_HASH; return SIMPLICITY_NO_ERROR;
        case 26: *result = INPUT_SCRIPTS_HASH; return SIMPLICITY_NO_ERROR;
        case 27: *result = TAPLEAF_HASH; return SIMPLICITY_NO_ERROR;
        case 28: *result = TAPPATH_HASH; return SIMPLICITY_NO_ERROR;
        case 29: *result = OUTPOINT_HASH; return SIMPLICITY_NO_ERROR;
        case 30: *result = ASSET_AMOUNT_HASH; return SIMPLICITY_NO_ERROR;
        case 31: *result = NONCE_HASH; return SIMPLICITY_NO_ERROR;
        case 32: *result = ANNEX_HASH; return SIMPLICITY_NO_ERROR;
        case 33: *result = BUILD_TAPLEAF_SIMPLICITY; return SIMPLICITY_NO_ERROR;
        case 34: *result = BUILD_TAPBRANCH; return SIMPLICITY_NO_ERROR;
        case 35: *result = BUILD_TAPTWEAK; return SIMPLICITY_NO_ERROR;
      }
      break;
    case 2:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1: *result = CHECK_LOCK_HEIGHT; return SIMPLICITY_NO_ERROR;
        case 2: *result = CHECK_LOCK_TIME; return SIMPLICITY_NO_ERROR;
        case 3: *result = CHECK_LOCK_DISTANCE; return SIMPLICITY_NO_ERROR;
        case 4: *result = CHECK_LOCK_DURATION; return SIMPLICITY_NO_ERROR;
        case 5: *result = TX_LOCK_HEIGHT; return SIMPLICITY_NO_ERROR;
        case 6: *result = TX_LOCK_TIME; return SIMPLICITY_NO_ERROR;
        case 7: *result = TX_LOCK_DISTANCE; return SIMPLICITY_NO_ERROR;
        case 8: *result = TX_LOCK_DURATION; return SIMPLICITY_NO_ERROR;
        case 9: *result = TX_IS_FINAL; return SIMPLICITY_NO_ERROR;
      }
      break;
    case 3:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1: *result = ISSUANCE; return SIMPLICITY_NO_ERROR;
        case 2: *result = ISSUANCE_ASSET; return SIMPLICITY_NO_ERROR;
        case 3: *result = ISSUANCE_TOKEN; return SIMPLICITY_NO_ERROR;
        case 4: *result = ISSUANCE_ENTROPY; return SIMPLICITY_NO_ERROR;
        case 5: *result = CALCULATE_ISSUANCE_ENTROPY; return SIMPLICITY_NO_ERROR;
        case 6: *result = CALCULATE_ASSET; return SIMPLICITY_NO_ERROR;
        case 7: *result = CALCULATE_EXPLICIT_TOKEN; return SIMPLICITY_NO_ERROR;
        case 8: *result = CALCULATE_CONFIDENTIAL_TOKEN; return SIMPLICITY_NO_ERROR;
        case 9: *result = LBTC_ASSET; return SIMPLICITY_NO_ERROR;
      }
      break;
    case 4:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1: *result = SCRIPT_CMR; return SIMPLICITY_NO_ERROR;
        case 2: *result = INTERNAL_KEY; return SIMPLICITY_NO_ERROR;
        case 3: *result = CURRENT_INDEX; return SIMPLICITY_NO_ERROR;
        case 4: *result = NUM_INPUTS; return SIMPLICITY_NO_ERROR;
        case 5: *result = NUM_OUTPUTS; return SIMPLICITY_NO_ERROR;
        case 6: *result = LOCK_TIME; return SIMPLICITY_NO_ERROR;
        case 7: *result = OUTPUT_ASSET; return SIMPLICITY_NO_ERROR;
        case 8: *result = OUTPUT_AMOUNT; return SIMPLICITY_NO_ERROR;
        case 9: *result = OUTPUT_NONCE; return SIMPLICITY_NO_ERROR;
        case 10: *result = OUTPUT_SCRIPT_HASH; return SIMPLICITY_NO_ERROR;
        case 11: *result = OUTPUT_NULL_DATUM; return SIMPLICITY_NO_ERROR;
        case 12: *result = OUTPUT_IS_FEE; return SIMPLICITY_NO_ERROR;
        case 13: *result = OUTPUT_SURJECTION_PROOF; return SIMPLICITY_NO_ERROR;
        case 14: *result = OUTPUT_RANGE_PROOF; return SIMPLICITY_NO_ERROR;
        case 15: *result = TOTAL_FEE; return SIMPLICITY_NO_ERROR;
        case 16: *result = CURRENT_PEGIN; return SIMPLICITY_NO_ERROR;
        case 17: *result = CURRENT_PREV_OUTPOINT; return SIMPLICITY_NO_ERROR;
        case 18: *result = CURRENT_ASSET; return SIMPLICITY_NO_ERROR;
        case 19: *result = CURRENT_AMOUNT; return SIMPLICITY_NO_ERROR;
        case 20: *result = CURRENT_SCRIPT_HASH; return SIMPLICITY_NO_ERROR;
        case 21: *result = CURRENT_SEQUENCE; return SIMPLICITY_NO_ERROR;
        case 22: *result = CURRENT_ANNEX_HASH; return SIMPLICITY_NO_ERROR;
        case 23: *result = CURRENT_SCRIPT_SIG_HASH; return SIMPLICITY_NO_ERROR;
        case 24: *result = CURRENT_REISSUANCE_BLINDING; return SIMPLICITY_NO_ERROR;
        case 25: *result = CURRENT_NEW_ISSUANCE_CONTRACT; return SIMPLICITY_NO_ERROR;
        case 26: *result = CURRENT_REISSUANCE_ENTROPY; return SIMPLICITY_NO_ERROR;
        case 27: *result = CURRENT_ISSUANCE_ASSET_AMOUNT; return SIMPLICITY_NO_ERROR;
        case 28: *result = CURRENT_ISSUANCE_TOKEN_AMOUNT; return SIMPLICITY_NO_ERROR;
        case 29: *result = CURRENT_ISSUANCE_ASSET_PROOF; return SIMPLICITY_NO_ERROR;
        case 30: *result = CURRENT_ISSUANCE_TOKEN_PROOF; return SIMPLICITY_NO_ERROR;
        case 31: *result = INPUT_PEGIN; return SIMPLICITY_NO_ERROR;
        case 32: *result = INPUT_PREV_OUTPOINT; return SIMPLICITY_NO_ERROR;
        case 33: *result = INPUT_ASSET; return SIMPLICITY_NO_ERROR;
        case 34: *result = INPUT_AMOUNT; return SIMPLICITY_NO_ERROR;
        case 35: *result = INPUT_SCRIPT_HASH; return SIMPLICITY_NO_ERROR;
        case 36: *result = INPUT_SEQUENCE; return SIMPLICITY_NO_ERROR;
        case 37: *result = INPUT_ANNEX_HASH; return SIMPLICITY_NO_ERROR;
        case 38: *result = INPUT_SCRIPT_SIG_HASH; return SIMPLICITY_NO_ERROR;
        case 39: *result = REISSUANCE_BLINDING; return SIMPLICITY_NO_ERROR;
        case 40: *result = NEW_ISSUANCE_CONTRACT; return SIMPLICITY_NO_ERROR;
        case 41: *result = REISSUANCE_ENTROPY; return SIMPLICITY_NO_ERROR;
        case 42: *result = ISSUANCE_ASSET_AMOUNT; return SIMPLICITY_NO_ERROR;
        case 43: *result = ISSUANCE_TOKEN_AMOUNT; return SIMPLICITY_NO_ERROR;
        case 44: *result = ISSUANCE_ASSET_PROOF; return SIMPLICITY_NO_ERROR;
        case 45: *result = ISSUANCE_TOKEN_PROOF; return SIMPLICITY_NO_ERROR;
        case 46: *result = TAPLEAF_VERSION; return SIMPLICITY_NO_ERROR;
        case 47: *result = TAPPATH; return SIMPLICITY_NO_ERROR;
        case 48: *result = VERSION; return SIMPLICITY_NO_ERROR;
        case 49: *result = GENESIS_BLOCK_HASH; return SIMPLICITY_NO_ERROR;
        case 50: *result = TRANSACTION_ID; return SIMPLICITY_NO_ERROR;
      }
      break;
  }
}