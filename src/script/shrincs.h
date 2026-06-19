#ifndef BITCOIN_SCRIPT_SHRINCS_H
#define BITCOIN_SCRIPT_SHRINCS_H

#include <vector>
#include <cstdint>

#define SHRINCS_SIG_SIZE 580
#define SHRINCS_PUBKEY_SIZE 32

bool VerifySHRINCS(const std::vector<unsigned char>& sig,
                   const std::vector<unsigned char>& msg,
                   const std::vector<unsigned char>& pubkey);

#endif
