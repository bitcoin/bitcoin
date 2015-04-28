// Master Protocol transaction code

#include "mastercore_tx.h"

#include "mastercore.h"
#include "mastercore_convert.h"
#include "mastercore_dex.h"
#include "mastercore_mdex.h"
#include "mastercore_log.h"
#include "mastercore_sp.h"

#include "alert.h"
#include "amount.h"
#include "main.h"
#include "utiltime.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <vector>

using boost::algorithm::token_compress_on;

using namespace mastercore;


// initial packet interpret step
int CMPTransaction::step1()
{
  if (PACKET_SIZE_CLASS_A > pkt_size)  // class A could be 19 bytes
  {
    file_log("%s() ERROR PACKET TOO SMALL; size = %d, line %d, file: %s\n", __FUNCTION__, pkt_size, __LINE__, __FILE__);
    return -(PKT_ERROR -1);
  }

  // collect version
  memcpy(&version, &pkt[0], 2);
  swapByteOrder16(version);

  // blank out version bytes in the packet
  pkt[0]=0; pkt[1]=0;
  
  memcpy(&type, &pkt[0], 4);

  swapByteOrder32(type);

  file_log("version: %d, Class %s\n", version, !multi ? "A":"B");
  file_log("\t            type: %u (%s)\n", type, c_strMasterProtocolTXType(type));

  return (type);
}

// extract alert info for alert packets
int CMPTransaction::step2_Alert(std::string *new_global_alert_message)
{
  const char *p = 4 + (char *)&pkt;
  std::vector<std::string>spstr;
  char alertString[SP_STRING_FIELD_LEN];

  // is sender authorized?
  bool authorized = false;
  if (
     // TESTNET
     (sender == "mpDex4kSX4iscrmiEQ8fBiPoyeTH55z23j") || // Michael
     (sender == "mCraigAddress") || // Craig
     (sender == "mpZATHupfCLqet5N1YL48ByCM1ZBfddbGJ") || // Zathras
     // MAINNET
     (sender == "1MicH2Vu4YVSvREvxW1zAx2XKo2GQomeXY") || // Michael
     (sender == "16Zwbujf1h3v1DotKcn9XXt1m7FZn2o4mj") || // Craig
     (sender == "1zAtHRASgdHvZDfHs6xJquMghga4eG7gy") || // Zathras
     (sender == "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P") // Exodus
     //(sender=="1Anyone2Else3Who4Should5Be6Here") // Who else?  JR? David? DexX?
     ) authorized = true;

  if(!authorized)
  {
      // not authorized, ignore alert
      file_log("\t      alert auth: false\n");
      return (PKT_ERROR -912);
  }
  else
  {
      // authorized, decode and make sure there are 4 tokens, then replace global_alert_message
      spstr.push_back(std::string(p));
      memcpy(alertString, spstr[0].c_str(), std::min(spstr[0].length(),sizeof(alertString)-1));

      std::vector<std::string> vstr;
      boost::split(vstr, alertString, boost::is_any_of(":"), token_compress_on);
      file_log("\t      alert auth: true\n");
      file_log("\t    alert sender: %s\n", sender);

      if (5 != vstr.size())
      {
          // there are not 5 tokens in the alert, badly formed alert and must discard
          file_log("\t    packet error: badly formed alert != 5 tokens\n");
          return (PKT_ERROR -911);
      }
      else
      {
          int32_t alertType;
          uint64_t expiryValue;
          uint32_t typeCheck;
          uint32_t verCheck;
          string alertMessage;
          try
          {
              alertType = boost::lexical_cast<int32_t>(vstr[0]);
              expiryValue = boost::lexical_cast<uint64_t>(vstr[1]);
              typeCheck = boost::lexical_cast<uint32_t>(vstr[2]);
              verCheck = boost::lexical_cast<uint32_t>(vstr[3]);
          } catch (const boost::bad_lexical_cast &e)
            {
                  file_log("DEBUG ALERT - error in converting values from global alert string\n");
                  return (PKT_ERROR -910); //(something went wrong)
            }
          alertMessage = vstr[4];
          file_log("\t    message type: %llu\n",alertType);
          file_log("\t    expiry value: %llu\n",expiryValue);
          file_log("\t      type check: %llu\n",typeCheck);
          file_log("\t       ver check: %llu\n",verCheck);
          file_log("\t   alert message: %s\n", alertMessage);
          // copy the alert string into the global_alert_message and return a 0 rc
          string message(alertString);
          *new_global_alert_message=message;
          // we have a new alert, fire a notify event if needed
          CAlert::Notify(alertMessage, true);
          return 0;
      }
  }
}

// extract Value for certain types of packets
int CMPTransaction::step2_Value()
{
  memcpy(&nValue, &pkt[8], 8);
  swapByteOrder64(nValue);

  // here we are copying nValue into nNewValue to be stored into our leveldb later: MP_txlist
  nNewValue = nValue;

  memcpy(&property, &pkt[4], 4);
  swapByteOrder32(property);

  file_log("\t        property: %u (%s)\n", property, strMPProperty(property));
  file_log("\t           value: %s\n", FormatMP(property, nValue));

  if (MAX_INT_8_BYTES < nValue)
  {
    return (PKT_ERROR -801);  // out of range
  }

  return 0;
}

// overrun check, are we beyond the end of packet?
bool CMPTransaction::isOverrun(const char *p, unsigned int line)
{
int now = (char *)p - (char *)&pkt;
bool bRet = (now > pkt_size);

    if (bRet) file_log("%s(%sline=%u):now= %u, pkt_size= %u\n", __FUNCTION__, bRet ? "OVERRUN !!! ":"", line, now, pkt_size);

    return bRet;
}

// extract Smart Property data
// RETURNS: the pointer to the next piece to be parsed
// ERROR is returns NULL and/or sets the error_code
const char *CMPTransaction::step2_SmartProperty(int &error_code)
{
const char *p = 11 + (char *)&pkt;
std::vector<std::string>spstr;
unsigned int i;
unsigned int prop_id;

  error_code = 0;

  memcpy(&ecosystem, &pkt[4], 1);
  file_log("\t       Ecosystem: %u\n", ecosystem);

  // valid values are 1 & 2
  if ((OMNI_PROPERTY_MSC != ecosystem) && (OMNI_PROPERTY_TMSC != ecosystem))
  {
    error_code = (PKT_ERROR_SP -501);
    return NULL;
  }

  prop_id = _my_sps->peekNextSPID(ecosystem);

  memcpy(&prop_type, &pkt[5], 2);
  swapByteOrder16(prop_type);

  memcpy(&prev_prop_id, &pkt[7], 4);
  swapByteOrder32(prev_prop_id);

  file_log("\t     Property ID: %u (%s)\n", prop_id, strMPProperty(prop_id));
  file_log("\t   Property type: %u (%s)\n", prop_type, c_strPropertyType(prop_type));
  file_log("\tPrev Property ID: %u\n", prev_prop_id);

  // only 1 & 2 are valid right now
  if ((MSC_PROPERTY_TYPE_INDIVISIBLE != prop_type) && (MSC_PROPERTY_TYPE_DIVISIBLE != prop_type))
  {
    error_code = (PKT_ERROR_SP -502);
    return NULL;
  }

  for (i = 0; i<5; i++)
  {
    spstr.push_back(std::string(p));
    p += spstr.back().size() + 1;
  }

  i = 0;

  memcpy(category, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(category)-1)); i++;
  memcpy(subcategory, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(subcategory)-1)); i++;
  memcpy(name, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(name)-1)); i++;
  memcpy(url, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(url)-1)); i++;
  memcpy(data, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(data)-1)); i++;

  file_log("\t        Category: %s\n", category);
  file_log("\t     Subcategory: %s\n", subcategory);
  file_log("\t            Name: %s\n", name);
  file_log("\t             URL: %s\n", url);
  file_log("\t            Data: %s\n", data);

  if (!isTransactionTypeAllowed(block, prop_id, type, version))
  {
    error_code = (PKT_ERROR_SP -503);
    return NULL;
  }

  // name can not be NULL
  if ('\0' == name[0])
  {
    error_code = (PKT_ERROR_SP -505);
    return NULL;
  }

  if (!p) error_code = (PKT_ERROR_SP -510);

  if (isOverrun(p, __LINE__))
  {
    error_code = (PKT_ERROR_SP -800);
    return NULL;
  }

  return p;
}

int CMPTransaction::step3_sp_fixed(const char *p)
{
  if (!p) return (PKT_ERROR_SP -1);

  memcpy(&nValue, p, 8);
  swapByteOrder64(nValue);
  p += 8;

  // here we are copying nValue into nNewValue to be stored into our leveldb later: MP_txlist
  nNewValue = nValue;

  if (MSC_PROPERTY_TYPE_INDIVISIBLE == prop_type)
  {
    file_log("\t           value: %lu\n", nValue);
    if (0 == nValue) return (PKT_ERROR_SP -101);
  }
  else
  if (MSC_PROPERTY_TYPE_DIVISIBLE == prop_type)
  {
    file_log("\t           value: %lu.%08lu\n", nValue/COIN, nValue%COIN);
    if (0 == nValue) return (PKT_ERROR_SP -102);
  }

  if (MAX_INT_8_BYTES < nValue)
  {
    return (PKT_ERROR -802);  // out of range
  }

  if (isOverrun(p, __LINE__)) return (PKT_ERROR_SP -900);

  return 0;
}

int CMPTransaction::step3_sp_variable(const char *p)
{
  if (!p) return (PKT_ERROR_SP -1);

  memcpy(&property, p, 4);  // property desired
  swapByteOrder32(property);
  p += 4;

  file_log("\t        property: %u (%s)\n", property, strMPProperty(property));

  memcpy(&nValue, p, 8);
  swapByteOrder64(nValue);
  p += 8;

  // here we are copying nValue into nNewValue to be stored into our leveldb later: MP_txlist
  nNewValue = nValue;

  if (MSC_PROPERTY_TYPE_INDIVISIBLE == prop_type)
  {
    file_log("\t           value: %lu\n", nValue);
    if (0 == nValue) return (PKT_ERROR_SP -201);
  }
  else
  if (MSC_PROPERTY_TYPE_DIVISIBLE == prop_type)
  {
    file_log("\t           value: %lu.%08lu\n", nValue/COIN, nValue%COIN);
    if (0 == nValue) return (PKT_ERROR_SP -202);
  }

  if (MAX_INT_8_BYTES < nValue)
  {
    return (PKT_ERROR -803);  // out of range
  }

  memcpy(&deadline, p, 8);
  swapByteOrder64(deadline);
  p += 8;
  file_log("\t        Deadline: %s (%lX)\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", deadline), deadline);

  if (!deadline) return (PKT_ERROR_SP -203);  // deadline cannot be 0

  // deadline can not be smaller than the timestamp of the current block
  if (deadline < (uint64_t)blockTime) return (PKT_ERROR_SP -204);

  memcpy(&early_bird, p++, 1);
  file_log("\tEarly Bird Bonus: %u\n", early_bird);

  memcpy(&percentage, p++, 1);
  file_log("\t      Percentage: %u\n", percentage);

  if (isOverrun(p, __LINE__)) return (PKT_ERROR_SP -765);

  return 0;
}

void CMPTransaction::printInfo(FILE *fp)
{
  fprintf(fp, "BLOCK: %d txid: %s, Block Time: %s\n", block, txid.GetHex().c_str(), DateTimeStrFormat("%Y-%m-%d %H:%M:%S", blockTime).c_str());
  fprintf(fp, "sender: %s\n", sender.c_str());
}

int CMPTransaction::logicMath_TradeOffer(CMPOffer *obj_o)
{
int rc = PKT_ERROR_TRADEOFFER;
uint64_t amount_desired, min_fee;
unsigned char blocktimelimit, subaction = 0;
static const char * const subaction_name[] = { "empty", "new", "update", "cancel" };

      if ((OMNI_PROPERTY_TMSC != property) && (OMNI_PROPERTY_MSC != property))
      {
        file_log("No smart properties allowed on the DeX...\n");
        return PKT_ERROR_TRADEOFFER -72;
      }

      // block height checks, for instance DEX is only available on MSC starting with block 290630
      if (!isTransactionTypeAllowed(block, property, type, version)) return -88888;

      memcpy(&amount_desired, &pkt[16], 8);
      memcpy(&blocktimelimit, &pkt[24], 1);
      memcpy(&min_fee, &pkt[25], 8);
      memcpy(&subaction, &pkt[33], 1);

      swapByteOrder64(amount_desired);
      swapByteOrder64(min_fee);

    file_log("\t  amount desired: %lu.%08lu\n", amount_desired / COIN, amount_desired % COIN);
    file_log("\tblock time limit: %u\n", blocktimelimit);
    file_log("\t         min fee: %lu.%08lu\n", min_fee / COIN, min_fee % COIN);
    file_log("\t      sub-action: %u (%s)\n", subaction, subaction < sizeof(subaction_name)/sizeof(subaction_name[0]) ? subaction_name[subaction] : "");

      if (obj_o)
      {
        obj_o->Set(amount_desired, min_fee, blocktimelimit, subaction);
        return PKT_RETURNED_OBJECT;
      }

      // figure out which Action this is based on amount for sale, version & etc.
      switch (version)
      {
        case MP_TX_PKT_V0:
          if (0 != nValue)
          {
            if (!DEx_offerExists(sender, property))
            {
              rc = DEx_offerCreate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
            }
            else
            {
              rc = DEx_offerUpdate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
            }
          }
          else
          // what happens if nValue is 0 for V0 ?  ANSWER: check if exists and it does -- cancel, otherwise invalid
          {
            if (DEx_offerExists(sender, property))
            {
              rc = DEx_offerDestroy(sender, property);
            }
          }

          break;

        case MP_TX_PKT_V1:
        {
          if (DEx_offerExists(sender, property))
          {
            if ((CANCEL != subaction) && (UPDATE != subaction))
            {
              file_log("%s() INVALID SELL OFFER -- ONE ALREADY EXISTS\n", __FUNCTION__);
              rc = PKT_ERROR_TRADEOFFER -11;
              break;
            }
          }
          else
          {
            // Offer does not exist
            if ((NEW != subaction))
            {
              file_log("%s() INVALID SELL OFFER -- UPDATE OR CANCEL ACTION WHEN NONE IS POSSIBLE\n", __FUNCTION__);
              rc = PKT_ERROR_TRADEOFFER -12;
              break;
            }
          }
 
          switch (subaction)
          {
            case NEW:
              rc = DEx_offerCreate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
              break;

            case UPDATE:
              rc = DEx_offerUpdate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
              break;

            case CANCEL:
              rc = DEx_offerDestroy(sender, property);
              break;

            default:
              rc = (PKT_ERROR -999);
              break;
          }
          break;
        }

        default:
          rc = (PKT_ERROR -500);  // neither V0 nor V1
          break;
      };

    return rc;
}

int CMPTransaction::logicMath_AcceptOffer_BTC()
{
int rc = DEX_ERROR_ACCEPT;

    // the min fee spec requirement is checked in the following function
    rc = DEx_acceptCreate(sender, receiver, property, nValue, block, tx_fee_paid, &nNewValue);

    return rc;
}

int CMPTransaction::logicMath_MetaDEx(CMPMetaDEx *mdex_o)
{
    int rc = PKT_ERROR_METADEX -100;
    unsigned char action = 0;

    memcpy(&desired_property, &pkt[16], 4);
    swapByteOrder32(desired_property);

    memcpy(&desired_value, &pkt[20], 8);
    swapByteOrder64(desired_value);

    file_log("\tdesired property: %u (%s)\n", desired_property, strMPProperty(desired_property));
    file_log("\t   desired value: %s\n", FormatMP(desired_property, desired_value));

    memcpy(&action, &pkt[28], 1);

    file_log("\t          action: %u\n", action);

    if (mdex_o)
    {
      mdex_o->Set(sender, block, property, nValue, desired_property, desired_value, txid, tx_idx, action);
      return PKT_RETURNED_OBJECT;
    }

    switch (action)
    {
      case ADD:
      {
        if (!isTransactionTypeAllowed(block, property, type, version)) return (PKT_ERROR_METADEX -889);

        // ensure we are not trading same property for itself
        if (property == desired_property) return (PKT_ERROR_METADEX -5);

        // ensure no cross-over of properties from test to main ecosystem
        if (isTestEcosystemProperty(property) != isTestEcosystemProperty(desired_property)) return (PKT_ERROR_METADEX -4);

        // ensure the desired property exists in our universe
        if (!_my_sps->hasSP(desired_property)) return (PKT_ERROR_METADEX -30);

        // ensure offered and desired values are positive
        if (0 >= static_cast<int64_t>(nNewValue)) return (PKT_ERROR_METADEX -11);
        if (0 >= static_cast<int64_t>(desired_value)) return (PKT_ERROR_METADEX -12);

        // ensure sufficient balance is available to offer
        if (getMPbalance(sender, property, BALANCE) < static_cast<int64_t>(nNewValue)) return (PKT_ERROR_METADEX -567);

        rc = MetaDEx_ADD(sender, property, nNewValue, block, desired_property, desired_value, txid, tx_idx);
        break;
      }

      case CANCEL_AT_PRICE:
      {
        if (!isTransactionTypeAllowed(block, property, type, version)) return (PKT_ERROR_METADEX -890);

        // ensure we are not trading same property for itself
        if (property == desired_property) return (PKT_ERROR_METADEX -5);

        // ensure no cross-over of properties from test to main ecosystem
        if (isTestEcosystemProperty(property) != isTestEcosystemProperty(desired_property)) return (PKT_ERROR_METADEX -4);

        // ensure the desired property exists in our universe
        if (!_my_sps->hasSP(desired_property)) return (PKT_ERROR_METADEX -30);

        // ensure offered and desired values are positive
        if (0 >= static_cast<int64_t>(nNewValue)) return (PKT_ERROR_METADEX -11);
        if (0 >= static_cast<int64_t>(desired_value)) return (PKT_ERROR_METADEX -12);

        rc = MetaDEx_CANCEL_AT_PRICE(txid, block, sender, property, nNewValue, desired_property, desired_value);
        break;
      }

      case CANCEL_ALL_FOR_PAIR:
      {
        if (!isTransactionTypeAllowed(block, property, type, version)) return (PKT_ERROR_METADEX -891);

        // ensure we are not trading same property for itself
        if (property == desired_property) return (PKT_ERROR_METADEX -5);

        // ensure no cross-over of properties from test to main ecosystem
        if (isTestEcosystemProperty(property) != isTestEcosystemProperty(desired_property)) return (PKT_ERROR_METADEX -4);

        // ensure the desired property exists in our universe
        if (!_my_sps->hasSP(desired_property)) return (PKT_ERROR_METADEX -30);

        rc = MetaDEx_CANCEL_ALL_FOR_PAIR(txid, block, sender, property, desired_property);
        break;
      }

      case CANCEL_EVERYTHING:
      {
        // cancel all open orders, if offered and desired properties are within the same ecosystem,
        // otherwise cancel all open orders for all properties of both ecosystems
        unsigned char ecosystem = 0;
        if (isMainEcosystemProperty(property) && isMainEcosystemProperty(desired_property)) ecosystem = OMNI_PROPERTY_MSC;
        if (isTestEcosystemProperty(property) && isTestEcosystemProperty(desired_property)) ecosystem = OMNI_PROPERTY_TMSC;

        if (!isTransactionTypeAllowed(block, ecosystem, type, version, true)) return (PKT_ERROR_METADEX -892);

        rc = MetaDEx_CANCEL_EVERYTHING(txid, block, sender, ecosystem);
        break;
      }

      default:
        return (PKT_ERROR_METADEX -999);
    }

    return rc;
}

int CMPTransaction::logicMath_GrantTokens()
{
    int rc = PKT_ERROR_TOKENS - 1000;

    if (!isTransactionTypeAllowed(block, property, type, version)) {
      file_log("\tRejecting Grant: Transaction type not yet allowed\n");
      return (PKT_ERROR_TOKENS - 22);
    }

    if (sender.empty()) {
      file_log("\tRejecting Grant: Sender is empty\n");
      return (PKT_ERROR_TOKENS - 23);
    }

    // manual issuance check
    if (false == _my_sps->hasSP(property)) {
      file_log("\tRejecting Grant: SP id:%u does not exist\n", property);
      return (PKT_ERROR_TOKENS - 24);
    }

    CMPSPInfo::Entry sp;
    _my_sps->getSP(property, sp);

    if (false == sp.manual) {
      file_log("\tRejecting Grant: SP id:%u was not issued with a TX 54\n", property);
      return (PKT_ERROR_TOKENS - 25);
    }


    // issuer check
    if (false == boost::iequals(sender, sp.issuer)) {
      file_log("\tRejecting Grant: %s is not the issuer of SP id: %u\n", sender, property);
      return (PKT_ERROR_TOKENS - 26);
    }

    // overflow tokens check
    if (MAX_INT_8_BYTES - sp.num_tokens < nValue) {
      char prettyTokens[256];
      if (sp.isDivisible()) {
        snprintf(prettyTokens, 256, "%lu.%08lu", nValue / COIN, nValue % COIN);
      } else {
        snprintf(prettyTokens, 256, "%lu", nValue);
      }
      file_log("\tRejecting Grant: granting %s tokens on SP id:%u would overflow the maximum limit for tokens in a smart property\n", prettyTokens, property);
      return (PKT_ERROR_TOKENS - 27);
    }

    // grant the tokens
    update_tally_map(sender, property, nValue, BALANCE);

    // call the send logic
    rc = logicMath_SimpleSend();

    // record this grant
    std::vector<uint64_t> dataPt;
    dataPt.push_back(nValue);
    dataPt.push_back(0);
    string txidStr = txid.ToString();
    sp.historicalData.insert(std::make_pair(txidStr, dataPt));
    sp.update_block = chainActive[block]->GetBlockHash();
    _my_sps->updateSP(property, sp);

    return rc;
}

int CMPTransaction::logicMath_RevokeTokens()
{
    int rc = PKT_ERROR_TOKENS - 1000;

    if (!isTransactionTypeAllowed(block, property, type, version)) {
      file_log("\tRejecting Revoke: Transaction type not yet allowed\n");
      return (PKT_ERROR_TOKENS - 22);
    }

    if (sender.empty()) {
      file_log("\tRejecting Revoke: Sender is empty\n");
      return (PKT_ERROR_TOKENS - 23);
    }

    // manual issuance check
    if (false == _my_sps->hasSP(property)) {
      file_log("\tRejecting Revoke: SP id:%d does not exist\n", property);
      return (PKT_ERROR_TOKENS - 24);
    }

    CMPSPInfo::Entry sp;
    _my_sps->getSP(property, sp);

    if (false == sp.manual) {
      file_log("\tRejecting Revoke: SP id:%d was not issued with a TX 54\n", property);
      return (PKT_ERROR_TOKENS - 25);
    }

    // insufficient funds check and revoke
    if (false == update_tally_map(sender, property, -nValue, BALANCE)) {
      file_log("\tRejecting Revoke: insufficient funds\n");
      return (PKT_ERROR_TOKENS - 111);
    }

    // record this revoke
    std::vector<uint64_t> dataPt;
    dataPt.push_back(0);
    dataPt.push_back(nValue);
    string txidStr = txid.ToString();
    sp.historicalData.insert(std::make_pair(txidStr, dataPt));
    sp.update_block = chainActive[block]->GetBlockHash();
    _my_sps->updateSP(property, sp);

    rc = 0;
    return rc;
}

int CMPTransaction::logicMath_ChangeIssuer()
{
  int rc = PKT_ERROR_TOKENS - 1000;

  if (!isTransactionTypeAllowed(block, property, type, version)) {
    file_log("\tRejecting Change of Issuer: Transaction type not yet allowed\n");
    return (PKT_ERROR_TOKENS - 22);
  }

  if (sender.empty()) {
    file_log("\tRejecting Change of Issuer: Sender is empty\n");
    return (PKT_ERROR_TOKENS - 23);
  }

  if (receiver.empty()) {
    file_log("\tRejecting Change of Issuer: Receiver is empty\n");
    return (PKT_ERROR_TOKENS - 23);
  }

  if (false == _my_sps->hasSP(property)) {
    file_log("\tRejecting Change of Issuer: SP id:%d does not exist\n", property);
    return (PKT_ERROR_TOKENS - 24);
  }

  CMPSPInfo::Entry sp;
  _my_sps->getSP(property, sp);

  // issuer check
  if (false == boost::iequals(sender, sp.issuer)) {
    file_log("\tRejecting Change of Issuer: %s is not the issuer of SP id:%d\n", sender, property);
    return (PKT_ERROR_TOKENS - 26);
  }

  // record this change of issuer
  sp.issuer = receiver;
  sp.update_block = chainActive[block]->GetBlockHash();
  _my_sps->updateSP(property, sp);

  rc = 0;
  return rc;
}

int CMPTransaction::logicMath_SavingsMark()
{
int rc = -12345;

  return rc;
}

int CMPTransaction::logicMath_SavingsCompromised()
{
int rc = -23456;

  return rc;
}

char *mastercore::c_strMasterProtocolTXType(int i)
{
  switch (i)
  {
    case MSC_TYPE_SIMPLE_SEND: return ((char *)"Simple Send");
    case MSC_TYPE_RESTRICTED_SEND: return ((char *)"Restricted Send");
    case MSC_TYPE_SEND_TO_OWNERS: return ((char *)"Send To Owners");
    case MSC_TYPE_SAVINGS_MARK: return ((char *)"Savings");
    case MSC_TYPE_SAVINGS_COMPROMISED: return ((char *)"Savings COMPROMISED");
    case MSC_TYPE_RATELIMITED_MARK: return ((char *)"Rate-Limiting");
    case MSC_TYPE_AUTOMATIC_DISPENSARY: return ((char *)"Automatic Dispensary");
    case MSC_TYPE_TRADE_OFFER: return ((char *)"DEx Sell Offer");
    case MSC_TYPE_METADEX: return ((char *)"MetaDEx token trade");
    case MSC_TYPE_ACCEPT_OFFER_BTC: return ((char *)"DEx Accept Offer");
    case MSC_TYPE_CREATE_PROPERTY_FIXED: return ((char *)"Create Property - Fixed");
    case MSC_TYPE_CREATE_PROPERTY_VARIABLE: return ((char *)"Create Property - Variable");
    case MSC_TYPE_PROMOTE_PROPERTY: return ((char *)"Promote Property");
    case MSC_TYPE_CLOSE_CROWDSALE: return ((char *)"Close Crowdsale");
    case MSC_TYPE_CREATE_PROPERTY_MANUAL: return ((char *)"Create Property - Manual");
    case MSC_TYPE_GRANT_PROPERTY_TOKENS: return ((char *)"Grant Property Tokens");
    case MSC_TYPE_REVOKE_PROPERTY_TOKENS: return ((char *)"Revoke Property Tokens");
    case MSC_TYPE_CHANGE_ISSUER_ADDRESS: return ((char *)"Change Issuer Address");
    case MSC_TYPE_NOTIFICATION: return ((char *)"Notification");
    case OMNICORE_MESSAGE_TYPE_ALERT: return ((char *)"ALERT");

    default: return ((char *)"* unknown type *");
  }
}

