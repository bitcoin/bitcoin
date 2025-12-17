#include "rsort.h"

#include <string.h>

#include "simplicity_assert.h"
#include "simplicity_alloc.h"

static_assert(UCHAR_MAX < SIZE_MAX, "UCHAR_MAX >= SIZE_MAX");
#define CHAR_COUNT ((size_t)1 << CHAR_BIT)

/* Return the 'i'th char of the object representation of the midstate pointed to by a.
 *
 * In C, values are represented as 'unsigned char [sizeof(v)]' array.  However the exact
 * specification of how this representation works is implementation defined.
 *
 * For the 'uint32_t' values of 'sha256_midstate', the object representation of these values will differ
 * between big endian and little endian architectures.
 *
 * Precondition: NULL != a
 *               i < sizeof(a->s);
 */
static unsigned char readIndex(const sha256_midstate* a, unsigned int i) {
  return ((const unsigned char*)(a->s))[i];
}

/* Given an array of midstate pointers,
 * count the frequencies of the values of the 'j'th character of each midstate's internal representation.
 * Returns 'true' if the 'j'th character of every entry is the same, otherwise returns 'false'.
 *
 * The time complexity of 'freq' is O('len').
 *
 * Precondition: uint32_t result[CHAR_COUNT];
 *               for all 0 <= i < len, NULL != a[i];
 *               j < sizeof((*a)->s)
 */
static bool freq(uint32_t* result, const sha256_midstate * const * a, uint_fast32_t len, unsigned int j) {
  memset(result, 0, CHAR_COUNT * sizeof(uint32_t));

  if (0 == len) return true;

  for (size_t i = 0; i < len - 1; ++i) {
    result[readIndex(a[i],j)]++;
  }

  /* Check the final iteration to see if the frequency is equal to 'len'. */
  return len == ++result[readIndex(a[len-1],j)];
}

/* Given an array of bucket sizes, and an initial value 'bucketEdge[0]',
 * add subsequent bucket edges of the given bucket sizes.
 *
 * Precondition: uint32_t bucketEdge[CHAR_COUNT];
 *               const uint32_t sizes[CHAR_COUNT];
 */
static void cumulative(uint32_t* restrict bucketEdge, const uint32_t* restrict sizes) {
  uint32_t accumulator = bucketEdge[0] + sizes[0];
  for (unsigned int i = 1; i < CHAR_COUNT; ++i) {
    bucketEdge[i] = accumulator;
    accumulator += sizes[i];
  }
}

/* Exchange two pointers.
 * (a == b is acceptable.)
 *
 * Precondition: NULL != a;
 *               NULL != b;
 */
static void swap(const sha256_midstate** a, const sha256_midstate** b) {
  const sha256_midstate* tmp = *a;
  *a = *b;
  *b = tmp;
}

/* Sort the subarray a[begin:end) by their ix-th character,
 * where begin = bucketEdge[0] and end = bucketEdge[UMAX_CHAR] + bucketSize[UMAX_CHAR].
 *
 * We expect that the ix-length prefix of every entry in a[being:end) to be identical
 * so that the result is that the a[begin:end) subarray ends up sorted by their first 'ix + 1' characters.
 *
 * Precondition: For all begin <= i < end, NULL != a[i];
 *               for all i < CHAR_COUNT, bucketSize[i] is the number of of entries in the subentries in a[begin:end) whose ix-th character is 'i'.
 *               for all i < UMAX_CHAR, bucketEdge[i] + bucketSize[i] = bucketEdge[i+1];
 *               ix < sizeof((*a)->s);
 * Postcondition: The contents of bucketSize is not preserved!
 */
static void sort_buckets(const sha256_midstate** a, uint32_t* restrict bucketSize, const uint32_t* restrict bucketEdge, unsigned int ix) {
  /* The implementation works by finding the first non-empty bucket and then swapping the first element of that bucket into its position
     at the far end of the bucket where it belongs.

     After moving the element into that position the size of the target bucket is decreased by one,
     and thus the next element that will be swapped into that bucket will be placed behind it.

     This process continues until the first element of the first non-empty bucket gets swapped with itself
     and its own bucket is decremented from size 1 to size 0.

     At that point we search again for the next non-empty bucket and repeat this process until there are no more non-empty buckets.
   */
  for (unsigned int i = 0; i < CHAR_COUNT; ++i) {
    size_t start = bucketEdge[i];
    while (bucketSize[i]) {
      /* Each time through this while loop some bucketSize is decremented.
         Therefore this body is executed 'sum(i < CHAR_COUNT, bucketSize[i]) = end - begin' many times.
       */
      size_t bucket = readIndex(a[start], ix);
      simplicity_assert(bucketSize[bucket]);
      bucketSize[bucket]--;
      swap(a + start, a + bucketEdge[bucket] + bucketSize[bucket]);
    }
  }
}

/* Attempts to (partially) sort an array of pointers to 'sha256_midstate's in place in memcmp order.
 * If NULL == hasDuplicates then sorting is always run to completion.
 * Otherwise, if NULL != hasDuplicates and if duplicate entries are found, the sorting is aborted and
 * *hasDuplicates is set to one of pointers of a duplicate entry.
 * If NULL != hasDuplicates and no duplicate entries are found, then *hasDuplicates is set to NULL.
 *
 * The time complexity of rsort is O('len').
 *
 * Precondition: For all 0 <= i < len, NULL != a[i];
 *               uint32_t stack[(CHAR_COUNT - 1)*(sizeof(*a)->s) + 1];
 */
static void rsort_ex(const sha256_midstate** a, uint_fast32_t len, const sha256_midstate** hasDuplicates, uint32_t* stack) {
  unsigned int depth = 0;
  size_t bucketCount[sizeof((*a)->s) + 1];
  size_t totalBucketCount = 1;

  static_assert(sizeof((*a)->s) <= UINT_MAX, "UINT_MAX too small to hold depth.");
  stack[0]=0;
  bucketCount[depth] = 1;

  /* This implementation contains a 'stack' of 'totalBucketCount' many subarrays (called buckets)
     that have been partially sorted up to various prefix length.

     The buckets are disjoint and cover the entire array from 0 up to len.
     As sorting proceeds, the end of the array will be sorted first.
     We will decrease len as we go as we find out that items at the end of the array are in their proper, sorted position.

     The 'i'th bucket is the subarray a[stack[i]:stack[i+1]),
     except for the last bucket which is the subarray a[stack[totalBucketCount-1]:len).

     The depth to which various buckets are sorted increases the further down the stack you go.
     The 'bucketCount' stores how many buckets are sorted to various depths.

     * for all i <= depth, the subarray a[sum(j < i, bucketCount[i]):end) have their first i characters sorted.
     * for all i <= depth, the (sum(j < i, bucketCount[i]))th and later buckets have all the first i characters of their entries identical.

     So all buckets are sorted up to a prefix length of 0 many characters.
     Then, if 1 <= depth, all bucket except for the first bucketCount[0]-many buckets are sorted by their 1 character prefix
     And then, if 2 <= depth, all bucket except for the first (bucketCount[0] + bucket[1])-many buckets are sorted by their 2 character prefix
     And so on.

     It is always the case that 'totalBucketCount = sum(i <= depth, bucketCount[i])',
     and thus the last bucket in the stack is located at stack[totalBucketCount-1].
     'totalBucketCount' and 'bucketCount' items are always increased and decreased in tandum.

     The loop is initialized with a 'totalBucketCount' of 1 and this one bucket contains the whole array a[0:len).
     'depth' is initialized to 0, and all the entries are trivialy sorted by the first 0-many characters.

     As we go through the loop, the last bucket is "popped" off the stack and processed, which can go one of two ways.

     If the last bucket is size 2 or greater, we proceed to sort it by its 'depth' character and partition the bucket into 256 sub-buckets
     which are then pushed onto the stack (notice this causes a net increase of the stack size by 255 items, because one was popped off).

     If ever the depth is beyond the size of the data being sorted,
     we can immediately halt as we have found 2 or more item that are identical.

     Note: there is an added optimization where by if there is only one non-empty bucket found when attempting to sort,
     i.e. it happens that every bucket item already has identical 'depth' characters,
     we skip the subdivision and move onto the next depth immediately.
     (This is equivalent to pushing the one non-empty bucket onto the stack and immediately popping it back off.)

     If the last bucket is of size 0 or 1, it must be already be sorted.
     Since this bucket is at the end of the array we decrease 'len'.
   */
  while(totalBucketCount) {
    /* Find the correct "depth" of the last bucket. */
    while(0 == bucketCount[depth]) {
      simplicity_assert(depth);
      depth--;
    }

    /* "pop" last bucket off the stack. */
    bucketCount[depth]--; totalBucketCount--;

    if (2 <= len - stack[totalBucketCount]) {
      uint32_t bucketSize[CHAR_COUNT];
      uint32_t* bucketEdge = stack + totalBucketCount;
      uint32_t begin = bucketEdge[0];

      /* Set bucketSize[i] to the count of the number of items in the array whose 'depth' character is 'i'.
         The time complexity of 'freq' is O('len - begin').
         WARNING: the 'freq' function modifies the contents of 'bucketSize' but is only executed when depth < sizeof((*a)->s).
       */
      while (depth < sizeof((*a)->s) && freq(bucketSize, a + begin, len - begin, depth)) {
        /* Optimize the case where there is only one bucket. i.e. when the 'depth' character of the interval [begin, len) are all identical. */
        depth++;
        bucketCount[depth] = 0;
      };

      if (depth < sizeof((*a)->s)) {
        /* Using bucketSize, compute all then next set of bucket edges based on
           where items will end up when they are sorted by their 'depth' character.
         */
        cumulative(bucketEdge, bucketSize);
        simplicity_assert(len == bucketEdge[UCHAR_MAX] + bucketSize[UCHAR_MAX]);

        /* Sort this bucket by their depth character, placing them into their proper buckets based on their bucketEdges. */
        sort_buckets(a, bucketSize, bucketEdge, depth);

        depth++; bucketCount[depth] = CHAR_COUNT; totalBucketCount += CHAR_COUNT;
        continue;
      }
      if (hasDuplicates) {
        /* Early return if we are searching for duplicates and have found a bucket of size 2 or more
           whose "prefix" agree up to then entire length of the hash value, and hence are all identical.
         */
        *hasDuplicates = a[begin];
        return;
      }
    }
    /* len - stack[totalBucketCount] < 2 || sizeof((*a)->s) == depth */

    /* When the last bucket size is 0 or 1, or when the depth exceeds sizeof((*a)->s), there is no sorting to do within the bucket.
       It is already sorted, and since it is at the end we can decrease len.
    */
    len = stack[totalBucketCount];
  }
  simplicity_assert(0 == len);

  if (hasDuplicates) *hasDuplicates = NULL;
}

/* Sorts an array of pointers to 'sha256_midstate's in place in memcmp order.
 * If malloc fails, returns false.
 * Otherwise, returns true.
 *
 * The time complexity of rsort is O('len').
 *
 * We are sorting in memcmp order, which is the lexicographical order of the object representation, i.e. the order that one
 * gets when casting 'sha256_midstate' to a 'unsigned char[]'. This representation is implementation defined, and will differ
 * on big endian and little endian architectures.
 *
 * It is critical that the details of this order remain unobservable from the consensus rules.
 *
 * Precondition: For all 0 <= i < len, NULL != a[i];
 */
bool simplicity_rsort(const sha256_midstate** a, uint_fast32_t len) {
  uint32_t *stack = simplicity_malloc(((CHAR_COUNT - 1)*(sizeof((*a)->s)) + 1) * sizeof(uint32_t));
  if (!stack) return false;
  rsort_ex(a, len, NULL, stack);
  simplicity_free(stack);
  return true;
}

/* Searches for duplicates in an array of 'sha256_midstate's.
 * If malloc fails, returns -1.
 * If no duplicates are found, returns 0.
 * If duplicates are found, returns a positive value.
 *
 * Precondition: const sha256_midstate a[len];
 *               len <= DAG_LEN_MAX;
 */
int simplicity_hasDuplicates(const sha256_midstate* a, uint_fast32_t len) {
  if (len < 2) return 0;
  static_assert(sizeof(a->s) * CHAR_BIT == 256, "sha256_midstate.s has unnamed padding.");
  static_assert(DAG_LEN_MAX <= UINT32_MAX, "DAG_LEN_MAX does not fit in uint32_t.");
  static_assert(DAG_LEN_MAX <= SIZE_MAX / sizeof(const sha256_midstate*), "perm array too large.");
  simplicity_assert((size_t)len <= SIZE_MAX / sizeof(const sha256_midstate*));
  const sha256_midstate **perm = simplicity_malloc(len * sizeof(const sha256_midstate*));
  uint32_t *stack = simplicity_malloc(((CHAR_COUNT - 1)*(sizeof((*perm)->s)) + 1) * sizeof(uint32_t));
  int result = perm && stack ? 0 : -1;

  if (0 <= result) {
    for (uint_fast32_t i = 0; i < len; ++i) {
      perm[i] = a + i;
    }

    const sha256_midstate *duplicate;
    rsort_ex(perm, len, &duplicate, stack);
    result = NULL != duplicate;
  }

  simplicity_free(perm);
  simplicity_free(stack);
  return result;
}
