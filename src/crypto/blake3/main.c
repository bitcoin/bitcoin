/*
 * This main file is intended for testing via `make test`. It does not build in
 * other settings. See README.md in this directory for examples of how to build
 * C code.
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "blake3.h"
#include "blake3_impl.h"

#define HASH_MODE 0
#define KEYED_HASH_MODE 1
#define DERIVE_KEY_MODE 2

static void hex_char_value(uint8_t c, uint8_t *value, bool *valid) {
  if ('0' <= c && c <= '9') {
    *value = c - '0';
    *valid = true;
  } else if ('a' <= c && c <= 'f') {
    *value = 10 + c - 'a';
    *valid = true;
  } else {
    *valid = false;
  }
}

static int parse_key(char *hex_key, uint8_t out[BLAKE3_KEY_LEN]) {
  size_t hex_len = strlen(hex_key);
  if (hex_len != 64) {
    fprintf(stderr, "Expected a 64-char hexadecimal key, got %zu chars.\n",
            hex_len);
    return 1;
  }
  for (size_t i = 0; i < 64; i++) {
    uint8_t value;
    bool valid;
    hex_char_value(hex_key[i], &value, &valid);
    if (!valid) {
      fprintf(stderr, "Invalid hex char.\n");
      return 1;
    }
    if (i % 2 == 0) {
      out[i / 2] = 0;
      value <<= 4;
    }
    out[i / 2] += value;
  }
  return 0;
}

/* A little repetition here */
enum cpu_feature {
  SSE2 = 1 << 0,
  SSSE3 = 1 << 1,
  SSE41 = 1 << 2,
  AVX = 1 << 3,
  AVX2 = 1 << 4,
  AVX512F = 1 << 5,
  AVX512VL = 1 << 6,
  /* ... */
  UNDEFINED = 1 << 30
};

extern enum cpu_feature g_cpu_features;
enum cpu_feature get_cpu_features();

int main(int argc, char **argv) {
  size_t out_len = BLAKE3_OUT_LEN;
  uint8_t key[BLAKE3_KEY_LEN];
  char *context = "";
  uint8_t mode = HASH_MODE;
  while (argc > 1) {
    if (argc <= 2) {
      fprintf(stderr, "Odd number of arguments.\n");
      return 1;
    }
    if (strcmp("--length", argv[1]) == 0) {
      char *endptr = NULL;
      errno = 0;
      unsigned long long out_len_ll = strtoull(argv[2], &endptr, 10);
      if (errno != 0 || out_len > SIZE_MAX || endptr == argv[2] ||
          *endptr != 0) {
        fprintf(stderr, "Bad length argument.\n");
        return 1;
      }
      out_len = (size_t)out_len_ll;
    } else if (strcmp("--keyed", argv[1]) == 0) {
      mode = KEYED_HASH_MODE;
      int ret = parse_key(argv[2], key);
      if (ret != 0) {
        return ret;
      }
    } else if (strcmp("--derive-key", argv[1]) == 0) {
      mode = DERIVE_KEY_MODE;
      context = argv[2];
    } else {
      fprintf(stderr, "Unknown flag.\n");
      return 1;
    }
    argc -= 2;
    argv += 2;
  }

  /*
   * We're going to hash the input multiple times, so we need to buffer it all.
   * This is just for test cases, so go ahead and assume that the input is less
   * than 1 MiB.
   */
  size_t buf_capacity = 1 << 20;
  uint8_t *buf = malloc(buf_capacity);
  assert(buf != NULL);
  size_t buf_len = 0;
  while (1) {
    size_t n = fread(&buf[buf_len], 1, buf_capacity - buf_len, stdin);
    if (n == 0) {
      break;
    }
    buf_len += n;
    assert(buf_len < buf_capacity);
  }

  const int mask = get_cpu_features();
  int feature = 0;
  do {
    fprintf(stderr, "Testing 0x%08X\n", feature);
    g_cpu_features = feature;
    blake3_hasher hasher;
    switch (mode) {
    case HASH_MODE:
      blake3_hasher_init(&hasher);
      break;
    case KEYED_HASH_MODE:
      blake3_hasher_init_keyed(&hasher, key);
      break;
    case DERIVE_KEY_MODE:
      blake3_hasher_init_derive_key(&hasher, context);
      break;
    default:
      abort();
    }

    blake3_hasher_update(&hasher, buf, buf_len);

    /* TODO: An incremental output reader API to avoid this allocation. */
    uint8_t *out = malloc(out_len);
    if (out_len > 0 && out == NULL) {
      fprintf(stderr, "malloc() failed.\n");
      return 1;
    }
    blake3_hasher_finalize(&hasher, out, out_len);
    for (size_t i = 0; i < out_len; i++) {
      printf("%02x", out[i]);
    }
    printf("\n");
    free(out);
    feature = (feature - mask) & mask;
  } while (feature != 0);
  free(buf);
  return 0;
}
