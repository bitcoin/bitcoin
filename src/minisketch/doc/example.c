/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Distributed under the MIT software license, see the accompanying   *
 * file LICENSE or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include "../include/minisketch.h"

int main(void) {

  minisketch *sketch_a = minisketch_create(12, 0, 4);

  for (int i = 3000; i < 3010; ++i) {
    minisketch_add_uint64(sketch_a, i);
  }

  size_t sersize = minisketch_serialized_size(sketch_a);
  assert(sersize == 12 * 4 / 8); // 4 12-bit values is 6 bytes.
  unsigned char *buffer_a = malloc(sersize);
  minisketch_serialize(sketch_a, buffer_a);
  minisketch_destroy(sketch_a);

  minisketch *sketch_b = minisketch_create(12, 0, 4); // Bob's own sketch
  for (int i = 3002; i < 3012; ++i) {
    minisketch_add_uint64(sketch_b, i);
  }

  sketch_a = minisketch_create(12, 0, 4);     // Alice's sketch
  minisketch_deserialize(sketch_a, buffer_a); // Load Alice's sketch
  free(buffer_a);

  // Merge the elements from sketch_a into sketch_b. The result is a sketch_b
  // which contains all elements that occurred in Alice's or Bob's sets, but not
  // in both.
  minisketch_merge(sketch_b, sketch_a);

  uint64_t differences[4];
  ssize_t num_differences = minisketch_decode(sketch_b, 4, differences);
  minisketch_destroy(sketch_a);
  minisketch_destroy(sketch_b);
  if (num_differences < 0) {
    printf("More than 4 differences!\n");
  } else {
    ssize_t i;
    for (i = 0; i < num_differences; ++i) {
      printf("%u is in only one of the two sets\n", (unsigned)differences[i]);
    }
  }
}
