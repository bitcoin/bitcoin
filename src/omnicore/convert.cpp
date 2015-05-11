#include "omnicore/convert.h"

#include <cmath>
#include <stdint.h>

namespace mastercore
{

// TODO: move to seperate file with checks
static bool isBigEndian()
{
  union
  {
    uint32_t i;
    char c[4];
  } bint = {0x01020304};

  return 1 == bint.c[0];
}

uint64_t rounduint64(long double ld)
{
    return static_cast<uint64_t>(roundl(fabsl(ld)));
}

void swapByteOrder16(uint16_t& us)
{
  if (isBigEndian()) return;

    us = (us >> 8) |
         (us << 8);
}

void swapByteOrder32(uint32_t& ui)
{
  if (isBigEndian()) return;

    ui = (ui >> 24) |
         ((ui << 8) & 0x00FF0000) |
         ((ui >> 8) & 0x0000FF00) |
         (ui << 24);
}

void swapByteOrder64(uint64_t& ull)
{
  if (isBigEndian()) return;

    ull = (ull >> 56) |
          ((ull << 40) & 0x00FF000000000000) |
          ((ull << 24) & 0x0000FF0000000000) |
          ((ull << 8)  & 0x000000FF00000000) |
          ((ull >> 8)  & 0x00000000FF000000) |
          ((ull >> 24) & 0x0000000000FF0000) |
          ((ull >> 40) & 0x000000000000FF00) |
          (ull << 56);
}

} // namespace mastercore
