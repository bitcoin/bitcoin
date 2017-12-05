#ifndef OMNICORE_ERRORS_H
#define OMNICORE_ERRORS_H

#include <string>

#include "omnicore/omnicore.h"

enum MPRPCErrorCode
{
    //INTERNAL_1packet
    MP_INSUF_FUNDS_BPENDI =         -1,     // balance before pending
    MP_INSUF_FUNDS_APENDI =         -2,     // balance after pending
    MP_INPUT_NOT_IN_RANGE =         -11,    // input value larger than supported

    //ClassAgnosticWalletTXBuilder(
    MP_INPUTS_INVALID =             -212,
    MP_ENCODING_ERROR =             -250,
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

      case PKT_ERROR -1:
          ec_str = "Attempt to execute logic in RPC mode";
          break;
      case PKT_ERROR -2:
          ec_str = "Failed to interpret transaction";
          break;
      case PKT_ERROR -3:
          ec_str = "Sender is frozen for the property";
          break;
      case PKT_ERROR -22:
          ec_str = "Transaction type or version not permitted";
          break;
      case PKT_ERROR -51:
          ec_str = "Sender is not authorized";
          break;
      case PKT_ERROR -54:
          ec_str = "Activation failed";
          break;

      case PKT_ERROR -100:
          ec_str = "Transaction is not a supported type";
          break;
      case PKT_ERROR -500:
          ec_str = "Transaction version is not supported";
          break;
      case PKT_ERROR -999:
          ec_str = "Failed to determine subaction";
          break;

      case PKT_ERROR_SEND -22:
          ec_str = "Transaction type or version not permitted";
          break;
      case PKT_ERROR_SEND -23:
          ec_str = "Value out of range or zero";
          break;
      case PKT_ERROR_SEND -24:
          ec_str = "Property does not exist";
          break;
      case PKT_ERROR_SEND -25:
          ec_str = "Sender has insufficient balance";
          break;

      case PKT_ERROR_STO -22:
          ec_str = "Transaction type or version not permitted";
          break;
      case PKT_ERROR_STO -23:
          ec_str = "Value out of range or zero";
          break;
      case PKT_ERROR_STO -24:
          ec_str = "Property does not exist";
          break;
      case PKT_ERROR_STO -25:
          ec_str = "Sender has insufficient balance";
          break;
      case PKT_ERROR_STO -26:
          ec_str = "No other owners of the property";
          break;
      case PKT_ERROR_STO -27:
          ec_str = "Sender has insufficient balance to pay for fee";
          break;
      case PKT_ERROR_STO -28:
          ec_str = "Sender has insufficient balance to pay for amount + fee";
          break;

      case PKT_ERROR_SEND_ALL -22:
          ec_str = "Transaction type or version not permitted";
          break;
      case PKT_ERROR_SEND_ALL -54:
          ec_str = "Sender has no tokens to send";
          break;
      case PKT_ERROR_SEND_ALL -55:
          ec_str = "Sender has no tokens to send";
          break;

      case PKT_ERROR_TRADEOFFER -22:
          ec_str = "Transaction type or version not permitted";
          break;
      case PKT_ERROR_TRADEOFFER -23:
          ec_str = "Value out of range or zero";
          break;
      case PKT_ERROR_TRADEOFFER -47:
          ec_str = "Property for sale must be OMNI or TOMNI";
          break;
      case PKT_ERROR_TRADEOFFER -49:
          ec_str = "Sender has no active sell offer for the property";
          break;
      case PKT_ERROR_TRADEOFFER -48:
          ec_str = "Sender already has an active sell offer for the property";
          break;

      case DEX_ERROR_SELLOFFER -101:
          ec_str = "Value out of range or zero";
          break;
      case DEX_ERROR_SELLOFFER -10:
          ec_str = "Sender already has an active sell offer for the property";
          break;
      case DEX_ERROR_SELLOFFER -25:
          ec_str = "Sender has insufficient balance";
          break;
      case DEX_ERROR_SELLOFFER -11:
          ec_str = "Sender has no active sell offer for the property";
          break;
      case DEX_ERROR_SELLOFFER -12:
          ec_str = "Sender has no active sell offer for the property";
          break;

      case DEX_ERROR_ACCEPT -15:
          ec_str = "No matching sell offer for accept order found";
          break;
      case DEX_ERROR_ACCEPT -20:
          ec_str = "Cannot locate accept to destroy";
          break;
      case DEX_ERROR_ACCEPT -22:
          ec_str = "Transaction type or version not permitted";
          break;
      case DEX_ERROR_ACCEPT -23:
          ec_str = "Value out of range or zero";
          break;
      case DEX_ERROR_ACCEPT -205:
          ec_str = "An accept from the sender to the recipient already exists";
          break;
      case DEX_ERROR_ACCEPT -105:
          ec_str = "Transaction fee too small";
          break;

      case PKT_ERROR_METADEX -21:
          ec_str = "Ecosystem is invalid";
          break;
      case PKT_ERROR_METADEX -22:
          ec_str = "Transaction type or version not permitted";
          break;
      case PKT_ERROR_METADEX -25:
          ec_str = "Sender has insufficient balance";
          break;
      case PKT_ERROR_METADEX -29:
          ec_str = "Property for sale and desired property are not be equal";
          break;
      case PKT_ERROR_METADEX -30:
          ec_str = "Property for sale and desired property are not in the same ecosystem";
          break;
      case PKT_ERROR_METADEX -31:
          ec_str = "Property for sale does not exist";
          break;
      case PKT_ERROR_METADEX -32:
          ec_str = "Property desired does not exist";
          break;
      case PKT_ERROR_METADEX -33:
          ec_str = "Amount for sale out of range or zero";
          break;
      case PKT_ERROR_METADEX -34:
          ec_str = "Amount desired out of range or zero";
          break;
      case PKT_ERROR_METADEX -35:
          ec_str = "One side of the trade must be OMNI or TOMNI";
          break;

      case METADEX_ERROR -1:
          ec_str = "Unknown MetaDEx (Add) error";
          break;
      case METADEX_ERROR -20:
          ec_str = "Unknown MetaDEx (Cancel Price) error";
          break;
      case METADEX_ERROR -30:
          ec_str = "Unknown MetaDEx (Cancel Pair) error";
          break;
      case METADEX_ERROR -40:
          ec_str = "Unknown MetaDEx (Cancel Everything) error";
          break;
      case METADEX_ERROR -66:
          ec_str = "Trade has a unit price of zero";
          break;
      case METADEX_ERROR -70:
          ec_str = "Trade already exists";
          break;

      case PKT_ERROR_SP -20:
          ec_str = "Block is not in the active chain";
          break;
      case PKT_ERROR_SP -21:
          ec_str = "Ecosystem is invalid";
          break;
      case PKT_ERROR_SP -22:
          ec_str = "Transaction type or version not permitted";
          break;
      case PKT_ERROR_SP -23:
          ec_str = "Value out of range or zero";
          break;
      case PKT_ERROR_SP -24:
          ec_str = "Desired property does not exist";
          break;
      case PKT_ERROR_SP -36:
          ec_str = "Invalid property type";
          break;
      case PKT_ERROR_SP -37:
          ec_str = "Property name is empty";
          break;
      case PKT_ERROR_SP -38:
          ec_str = "Deadline is in the past";
          break;
      case PKT_ERROR_SP -39:
          ec_str = "Sender has an active crowdsale";
          break;
      case PKT_ERROR_SP -40:
          ec_str = "Sender has no active crowdsale";
          break;
      case PKT_ERROR_SP -41:
          ec_str = "Property identifier mismatch";
          break;
      case PKT_ERROR_SP -42:
          ec_str = "Property is not managed";
          break;
      case PKT_ERROR_SP -43:
          ec_str = "Sender is not the issuer of the property";
          break;
      case PKT_ERROR_SP -44:
          ec_str = "Attempt to grant more than the maximum number of tokens";
          break;
      case PKT_ERROR_SP -50:
          ec_str = "Tokens to issue and desired property are not in the same ecosystem";
          break;

      case PKT_ERROR_TOKENS -22:
          ec_str = "Transaction type or version not permitted";
          break;
      case PKT_ERROR_TOKENS -23:
          ec_str = "Value out of range or zero";
          break;
      case PKT_ERROR_TOKENS -24:
          ec_str = "Property does not exist";
          break;
      case PKT_ERROR_TOKENS -25:
          ec_str = "Sender has insufficient balance";
          break;
      case PKT_ERROR_TOKENS -39:
          ec_str = "Sender has an active crowdsale";
          break;
      case PKT_ERROR_TOKENS -43:
          ec_str = "Sender is not the issuer of the property";
          break;
      case PKT_ERROR_TOKENS -45:
          ec_str = "Receiver is empty";
          break;
      case PKT_ERROR_TOKENS -46:
          ec_str = "Receiver has an active crowdsale";
          break;
      case PKT_ERROR_TOKENS -47:
          ec_str = "Freezing is not enabled for the property";
          break;
      case PKT_ERROR_TOKENS -48:
          ec_str = "Address is not frozen";
          break;
      case PKT_ERROR_TOKENS -49:
          ec_str = "Freezing is already enabled for the property";
          break;
      case PKT_ERROR_TOKENS -50:
          ec_str = "Address is already frozen";
          break;

      default:
          ec_str = "Unknown error";
  }

  return ec_str;
}


#endif // OMNICORE_ERRORS_H
