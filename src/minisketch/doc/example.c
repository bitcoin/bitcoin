#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "../include/minisketch.h"

int main(void) {
  const int num_elements = 10;
  const int bit_width = 12;
  const int bytes_per_element = bit_width / 8;

  minisketch *sketch_a = minisketch_create(bit_width, 0, num_elements);
  for (int i = 3000; i < 3000 + num_elements; ++i) {
    minisketch_add_uint64(sketch_a, i);
  }

  size_t sersize = minisketch_serialized_size(sketch_a);
  assert(sersize == num_elements * bytes_per_element);

  unsigned char *buffer_a = malloc(sersize);
  if (buffer_a == NULL) {
    perror("Failed to allocate memory for buffer_a");
    return 1; // Return an error code.
  }

  minisketch_serialize(sketch_a, buffer_a);
  minisketch_clear(sketch_a);

  for (int i = 3002; i < 3002 + num_elements; ++i) {
    minisketch_add_uint64(sketch_a, i);
  }

  // Load Alice's sketch
  minisketch_deserialize(sketch_a, buffer_a);
  free(buffer_a);

  // Merge the elements from sketch_a into sketch_a itself.
  minisketch_merge(sketch_a, sketch_a);

  uint64_t differences[num_elements];
  ssize_t num_differences = minisketch_decode(sketch_a, num_elements, differences);

  minisketch_destroy(sketch_a);

  if (num_differences < 0) {
    printf("More than %d differences!\n", num_elements);
  } else {
    printf("Differences:\n");
    for (ssize_t i = 0; i < num_differences; ++i) {
      printf("%u is in only one of the two sets\n", (unsigned)differences[i]);
    }
  }

  return 0;
}
