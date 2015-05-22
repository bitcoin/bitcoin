#ifndef OMNICORE_TX_H
#define OMNICORE_TX_H

class CMPOffer;
class CMPMetaDEx;

#include "omnicore/log.h"
#include "omnicore/omnicore.h"

class CTransaction;

#include "amount.h"
#include "uint256.h"
#include "utilstrencodings.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <utility>

using std::pair;
using std::string;

using mastercore::c_strMasterProtocolTXType;


// The class responsible for tx interpreting/parsing.
//
// It invokes other classes & methods: offers, accepts, tallies (balances).
class CMPTransaction
{
private:
  string sender;
  string receiver;
  uint256 txid;
  int block;
  unsigned int tx_idx;  // tx # within the block, 0-based
  int pkt_size;
  unsigned char pkt[1 + MAX_PACKETS * PACKET_SIZE];
  uint64_t nValue;
  int multi;  // Class A = 0, Class B = 1, Class C = 2
  uint64_t tx_fee_paid;
  unsigned int type;
  unsigned int property;
  unsigned short version; // = MP_TX_PKT_V0;
  uint64_t nNewValue;
  int64_t blockTime;  // internally nTime is still an "unsigned int"

  // SP additions, perhaps a new class or a union is needed
  unsigned char ecosystem;
  unsigned short prop_type;
  unsigned int prev_prop_id;

  char category[SP_STRING_FIELD_LEN];
  char subcategory[SP_STRING_FIELD_LEN];
  char name[SP_STRING_FIELD_LEN];
  char url[SP_STRING_FIELD_LEN];
  char data[SP_STRING_FIELD_LEN];

  uint64_t deadline;
  unsigned char early_bird;
  unsigned char percentage;

  // METADEX additions
  unsigned int desired_property;
  uint64_t desired_value;

  // TradeOffer
  uint64_t amount_desired;
  unsigned char blocktimelimit;
  uint64_t min_fee;
  unsigned char subaction;

  // MetaDEx
  unsigned char action;

  // Alert
  char alertString[SP_STRING_FIELD_LEN];


  class SendToOwners_compare
  {
  public:

    bool operator()(pair<int64_t, string> p1, pair<int64_t, string> p2) const
    {
      if (p1.first == p2.first) return p1.second > p2.second; // reverse check
      else return p1.first < p2.first;
    }
  };

public:
  enum ActionTypes
  {
    INVALID = 0,

    NEW = 1,

    UPDATE = 2,

    CANCEL = 3,

    ADD                 = 1,
    CANCEL_AT_PRICE     = 2,
    CANCEL_ALL_FOR_PAIR = 3,
    CANCEL_EVERYTHING   = 4,
  };

//  mutable CCriticalSection cs_msc;  // TODO: need to refactor first...

  unsigned int getType() const { return type; }
  const string getTypeString() const { return string(c_strMasterProtocolTXType(getType())); }
  unsigned int getProperty() const { return property; }
  unsigned short getVersion() const { return version; }
  unsigned short getPropertyType() const { return prop_type; }
  uint64_t getFeePaid() const { return tx_fee_paid; }

  const string & getSender() const { return sender; }
  const string & getReceiver() const { return receiver; }

  uint64_t getAmount() const { return nValue; }
  uint64_t getNewAmount() const { return nNewValue; }

  string getSPName() const { return string(name); }

  void printInfo(FILE *fp);

  void SetNull()
  {
    property = 0;
    type = 0;
    txid = 0;
    tx_idx = 0;  // tx # within the block, 0-based
    nValue = 0;
    nNewValue = 0;
    tx_fee_paid = 0;
    block = -1;
    pkt_size = 0;
    sender.erase();
    receiver.erase();

    blockTime = 0;

    ecosystem = 0;
    prop_type = 0;
    prev_prop_id = 0;

    memset(&pkt, 0, sizeof(pkt));

    memset(&category, 0, sizeof(category));
    memset(&subcategory, 0, sizeof(subcategory));
    memset(&name, 0, sizeof(name));
    memset(&url, 0, sizeof(url));
    memset(&data, 0, sizeof(data));
  }

  CMPTransaction()
  {
    SetNull();
  }

  bool interpret_Transaction();
  bool interpret_TransactionType();
  bool interpret_SimpleSend();                  //  1
  bool interpret_SendToOwners();                //  3
  bool interpret_TradeOffer();                  // 20
  bool interpret_MetaDEx();                     // 21
  bool interpret_AcceptOfferBTC();              // 22
  bool interpret_CreatePropertyFixed();         // 50
  bool interpret_CreatePropertyVariable();      // 51
  bool interpret_CloseCrowdsale();              // 53
  bool interpret_CreatePropertyMananged();      // 54
  bool interpret_GrantTokens();                 // 55
  bool interpret_RevokeTokens();                // 56
  bool interpret_ChangeIssuer();                // 70
  bool interpret_Alert();                       // 65535

  int logicMath_SimpleSend();                   //  1
  int logicMath_SendToOwners(FILE* fp = NULL);  //  3
  int logicMath_TradeOffer(CMPOffer*);          // 20
  int logicMath_MetaDEx(CMPMetaDEx*);           // 21
  int logicMath_AcceptOffer_BTC();              // 22
  int logicMath_CreatePropertyFixed();          // 50
  int logicMath_CreatePropertyVariable();       // 51
  int logicMath_CloseCrowdsale();               // 53
  int logicMath_CreatePropertyMananged();       // 54
  int logicMath_GrantTokens();                  // 55
  int logicMath_RevokeTokens();                 // 56
  int logicMath_ChangeIssuer();                 // 70
  int logicMath_Alert();                        // 65535
  int logicMath_SavingsMark(void);              // missing
  int logicMath_SavingsCompromised(void);       // missing

 int interpretPacket(CMPOffer *obj_o = NULL, CMPMetaDEx *mdex_o = NULL);
 int step1(void);
 int step2_Alert(std::string *new_global_alert_message);
 int step2_Value(void);
 bool isOverrun(const char *p, unsigned int line);
 const char *step2_SmartProperty(int &error_code);
 int step3_sp_fixed(const char *p);
 int step3_sp_variable(const char *p);

  void Set(const uint256 &t, int b, unsigned int idx, int64_t bt)
  {
    txid = t;
    block = b;
    tx_idx = idx;
    blockTime = bt;
  }

  void Set(string s, string r, uint64_t n, const uint256 &t, int b, unsigned int idx, unsigned char *p, unsigned int size, int fMultisig, uint64_t txf)
  {
    sender = s;
    receiver = r;
    txid = t;
    block = b;
    tx_idx = idx;
    pkt_size = size < sizeof(pkt) ? size : sizeof(pkt);
    nValue = n;
    nNewValue = n;
    multi= fMultisig;
    tx_fee_paid = txf;

    memcpy(&pkt, p, pkt_size);
  }

  bool operator<(const CMPTransaction& other) const
  {
    // sort by block # & additionally the tx index within the block
    if (block != other.block) return block > other.block;
    return tx_idx > other.tx_idx;
  }

  void print()
  {
    PrintToLog("BLOCK: %d =txid: %s =fee: %1.8lf\n", block, txid.GetHex(), (double)tx_fee_paid/(double)COIN);
    PrintToLog("sender: %s ; receiver: %s\n", sender, receiver);

    if (0<pkt_size)
    {
      PrintToLog("pkt: %s\n", HexStr(pkt, pkt_size + pkt, false));
    }
    else
    {
      // error ?
    }
  }
};

int ParseTransaction(const CTransaction& tx, int nBlock, unsigned int idx, CMPTransaction* pmptx, unsigned int nTime=0);


#endif // OMNICORE_TX_H
