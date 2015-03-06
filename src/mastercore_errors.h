#ifndef _MASTERCOIN_ERRORS
#define _MASTERCOIN_ERRORS 1

enum MPRPCErrorCode
{
    //INTERNAL_1packet
    MP_INSUF_FUNDS_BPENDI =         -1,     // balance before pending
    MP_INSUF_FUNDS_APENDI =         -2,     // balance after pending
    MP_INPUT_NOT_IN_RANGE =         -11,    // input value larger than supported
    
    //ClassB_send
    MP_INPUTS_INVALID =             -212,
    MP_REDEMP_ILLEGAL =             -233,
    MP_REDEMP_BAD_KEYID =           -220,
    MP_REDEMP_FETCH_ERR_PUBKEY =    -221,
    MP_REDEMP_INVALID_PUBKEY =      -222,
    MP_REDEMP_BAD_VALIDATION =      -223,
    MP_ERR_WALLET_ACCESS =          -205,
    MP_ERR_INPUTSELECT_FAIL =       -206,
    MP_ERR_CREATE_TX =              -211,
    MP_ERR_COMMIT_TX =              -213,
    
    //gettransaction_MP, listtransactions_MP
    MP_TX_NOT_FOUND =               -3331,  // No information available about transaction. (GetTransaction failed)
    MP_TX_UNCONFIRMED =             -3332,  // Unconfirmed transactions are not supported. (blockHash is 0)
    MP_BLOCK_NOT_IN_CHAIN =         -3333,  // Transaction not part of the active chain.   (pBlockIndex is NULL)
    MP_CROWDSALE_WITHOUT_PROPERTY = -3334,  // Potential database corruption: "Crowdsale Purchase" without valid property identifier.
    MP_INVALID_TX_IN_DB_FOUND     = -3335,  // Potential database corruption: Invalid transaction found.
    MP_TX_IS_NOT_MASTER_PROTOCOL  = -3336,  // Not a Master Protocol transaction.
};

inline std::string error_str(int ec) {
  std::string ec_str;
  
  switch (ec)
  {
      case MP_INSUF_FUNDS_BPENDI:
          ec_str = "Not enough funds in user address";
          break;
      case MP_INSUF_FUNDS_APENDI:
          ec_str = "Not enough funds in user address due to PENDING/UNCONFIRMED transactions";
          break;
      case MP_INPUT_NOT_IN_RANGE:
          ec_str = "Input value supplied larger than supported in Master Protocol";
          break;
      case MP_INPUTS_INVALID:
          ec_str = "Error choosing inputs for the send transaction";
          break;
      case MP_REDEMP_ILLEGAL:
          ec_str = "Error with redemption address";
          break;
      case MP_REDEMP_BAD_KEYID:
          ec_str = "Error with redemption address key ID";
          break;
      case MP_REDEMP_FETCH_ERR_PUBKEY:
          ec_str = "Error obtaining public key for redemption address";
          break;
      case MP_REDEMP_INVALID_PUBKEY:
          ec_str = "Error public key for redemption address is not valid";
          break;
      case MP_REDEMP_BAD_VALIDATION:
          ec_str = "Error validating redemption address";
          break;
      case MP_ERR_WALLET_ACCESS:
          ec_str = "Error with wallet object";
          break;
      case MP_ERR_INPUTSELECT_FAIL:
          ec_str = "Error with selected inputs for the send transaction";
          break;
      case MP_ERR_CREATE_TX:
          ec_str = "Error creating transaction (wallet may be locked or fees may not be sufficient)";
          break;
      case MP_ERR_COMMIT_TX:
          ec_str = "Error committing transaction";
          break;
      default:
          ec_str = "Unknown error";
  }

  return ec_str;
}

#endif // _MASTERCOIN_ERRORS
