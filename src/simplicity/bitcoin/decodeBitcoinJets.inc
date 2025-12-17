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
        case 6: *result = INPUT_UTXOS_HASH; return SIMPLICITY_NO_ERROR;
        case 7: *result = OUTPUT_HASH; return SIMPLICITY_NO_ERROR;
        case 8: *result = OUTPUT_VALUES_HASH; return SIMPLICITY_NO_ERROR;
        case 9: *result = OUTPUT_SCRIPTS_HASH; return SIMPLICITY_NO_ERROR;
        case 10: *result = INPUT_HASH; return SIMPLICITY_NO_ERROR;
        case 11: *result = INPUT_OUTPOINTS_HASH; return SIMPLICITY_NO_ERROR;
        case 12: *result = INPUT_SEQUENCES_HASH; return SIMPLICITY_NO_ERROR;
        case 13: *result = INPUT_ANNEXES_HASH; return SIMPLICITY_NO_ERROR;
        case 14: *result = INPUT_SCRIPT_SIGS_HASH; return SIMPLICITY_NO_ERROR;
        case 15: *result = INPUT_UTXO_HASH; return SIMPLICITY_NO_ERROR;
        case 16: *result = INPUT_VALUES_HASH; return SIMPLICITY_NO_ERROR;
        case 17: *result = INPUT_SCRIPTS_HASH; return SIMPLICITY_NO_ERROR;
        case 18: *result = TAPLEAF_HASH; return SIMPLICITY_NO_ERROR;
        case 19: *result = TAPPATH_HASH; return SIMPLICITY_NO_ERROR;
        case 20: *result = OUTPOINT_HASH; return SIMPLICITY_NO_ERROR;
        case 21: *result = ANNEX_HASH; return SIMPLICITY_NO_ERROR;
        case 22: *result = BUILD_TAPLEAF_SIMPLICITY; return SIMPLICITY_NO_ERROR;
        case 23: *result = BUILD_TAPBRANCH; return SIMPLICITY_NO_ERROR;
        case 24: *result = BUILD_TAPTWEAK; return SIMPLICITY_NO_ERROR;
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
        case 1: *result = SCRIPT_CMR; return SIMPLICITY_NO_ERROR;
        case 2: *result = INTERNAL_KEY; return SIMPLICITY_NO_ERROR;
        case 3: *result = CURRENT_INDEX; return SIMPLICITY_NO_ERROR;
        case 4: *result = NUM_INPUTS; return SIMPLICITY_NO_ERROR;
        case 5: *result = NUM_OUTPUTS; return SIMPLICITY_NO_ERROR;
        case 6: *result = LOCK_TIME; return SIMPLICITY_NO_ERROR;
        case 7: *result = FEE; return SIMPLICITY_NO_ERROR;
        case 8: *result = OUTPUT_VALUE; return SIMPLICITY_NO_ERROR;
        case 9: *result = OUTPUT_SCRIPT_HASH; return SIMPLICITY_NO_ERROR;
        case 10: *result = TOTAL_OUTPUT_VALUE; return SIMPLICITY_NO_ERROR;
        case 11: *result = CURRENT_PREV_OUTPOINT; return SIMPLICITY_NO_ERROR;
        case 12: *result = CURRENT_VALUE; return SIMPLICITY_NO_ERROR;
        case 13: *result = CURRENT_SCRIPT_HASH; return SIMPLICITY_NO_ERROR;
        case 14: *result = CURRENT_SEQUENCE; return SIMPLICITY_NO_ERROR;
        case 15: *result = CURRENT_ANNEX_HASH; return SIMPLICITY_NO_ERROR;
        case 16: *result = CURRENT_SCRIPT_SIG_HASH; return SIMPLICITY_NO_ERROR;
        case 17: *result = INPUT_PREV_OUTPOINT; return SIMPLICITY_NO_ERROR;
        case 18: *result = INPUT_VALUE; return SIMPLICITY_NO_ERROR;
        case 19: *result = INPUT_SCRIPT_HASH; return SIMPLICITY_NO_ERROR;
        case 20: *result = INPUT_SEQUENCE; return SIMPLICITY_NO_ERROR;
        case 21: *result = INPUT_ANNEX_HASH; return SIMPLICITY_NO_ERROR;
        case 22: *result = INPUT_SCRIPT_SIG_HASH; return SIMPLICITY_NO_ERROR;
        case 23: *result = TOTAL_INPUT_VALUE; return SIMPLICITY_NO_ERROR;
        case 24: *result = TAPLEAF_VERSION; return SIMPLICITY_NO_ERROR;
        case 25: *result = TAPPATH; return SIMPLICITY_NO_ERROR;
        case 26: *result = VERSION; return SIMPLICITY_NO_ERROR;
        case 27: *result = TRANSACTION_ID; return SIMPLICITY_NO_ERROR;
      }
      break;
  }
}