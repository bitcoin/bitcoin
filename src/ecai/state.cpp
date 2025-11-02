#include "ecai/api.h"
#include "ecai/state.h"  // your real state

namespace ecai {
  State g_state;

  bool Enabled() {
	  return true;
  }

  static void refresh_policy_bytes(Policy& p) {
    p.bytes = policy::Serialize();   // call your serializer if you have one
    p.loaded = policy::IsLoaded();   // or set true if always available
    p.c_pol  = policy::CurrentId();  // or compute/leave 0 if unused
  }

  // Call refresh_policy_bytes() from your init hook so it’s ready for validation.cpp
}

