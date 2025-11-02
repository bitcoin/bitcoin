// src/ecai/api.h
#pragma once
#include <vector>
#include <cstdint>
#include <primitives/transaction.h>  


class CTransaction;
class CTxMemPool;
class uint256;

namespace ecai {
  struct Policy {
	  bool loaded{false};
	  int c_pol{0};
	  int  id{0};
	  std::vector<unsigned char> bytes;
  };
  struct State  { Policy policy; };
  extern State g_state;

  struct Ctx { int height{0}; int64_t mediantime{0}; };

  enum class Verdict : unsigned char { REJECT=0, ACCEPT=1, QUEUE=2 };

  bool Enabled();

  std::vector<unsigned char> EncodeTLVFeatures(const CTransaction&, const CTxMemPool&);
  bool HashToCurve(const CTransaction&, const std::vector<unsigned char>&);
  bool HashToCurve(const std::vector<unsigned char>& feats,
                 const std::array<unsigned char, 33>& point);
  bool HashToCurve(const std::array<unsigned char, 33>& point,
                 const std::vector<unsigned char>& feats);

  inline bool HashToCurve(const std::vector<unsigned char>& f, const CTransaction& tx)
  { return HashToCurve(tx, f); }

  Verdict Evaluate(const std::vector<unsigned char>& policy_bytes,
                   const std::vector<unsigned char>& feats);

  struct Proof {};
  bool SignAndAssemble(const uint256& txid, Proof& out);
  bool SignAndAssemble(const Txid& txid, Proof& out);

  // NEW: match the call from validation.cpp
bool SignAndAssemble(const Txid& txid,
                     int policy_id,
                     unsigned char verdict,
                     int c_pol,
                     const std::array<unsigned char, 33>& p_tx,
                     const Ctx& ctx,
                     Proof& out);
  bool Publish(const Proof& p);
}

