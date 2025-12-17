This directory contains a modified copy of the libsecp256k1 at `bdf39000b9c6a0818e7149ccb500873d079e6e85` from <https://github.com/bitcoin-core/secp256k1/tree/v0.3.0>.
In general, the files in this directory should be compared with the corresponding files in the `src` directory from libsecp256k1.
There are some exceptions however:

* `precompute_ecmult.h` should be compared with `src/precompute_ecmult.c`
* `secp256k1.h` should be compared with `include/secp256k1.h`.
* `secp256k1_impl.h` should be compared with `src/secp256k1.c`.
* `extrakeys.h` should be compared with `include/secp256k1_extrakeys.h`.
* `extrakeys_impl.h` should be compared with `src/modules/extrakeys/main_impl.h`.
* `schnorrsig.h` should be compared with `include/secp256k1_schnorrsig.h`.
* `schnorrsig_impl.h` should be compared with `src/modules/schnorrsig/main_impl.h`.
* `generator.h` should be compared with `include/secp256k1_generator.h` from <https://github.com/BlockstreamResearch/secp256k1-zkp/tree/d47e4d40ca487bb573e895723bd4b0659ae8eee5>.
* `generator_impl.h` should be compared with `src/modules/generator/main_impl.h` from <https://github.com/BlockstreamResearch/secp256k1-zkp/tree/d47e4d40ca487bb573e895723bd4b0659ae8eee5>; however see `shallue_van_de_woestijne` below.

Our use of libsecp256k1 for various jets requires access to the internal functions that are not exposed by their API, so we cannot use libsecp256k1's normal interface.
Furthermore, because Simplicity has no abstract data types, the specific details of the representation of field and group elements computed by jetted functions ends up being consensus critical.
Therefore, even if libsecp256k1's interface exposed the functionality we needed, we still wouldn't be able perform libsecp256k1 version upgrades because different versions of libsecp256k1 do not guarantee that their functions won't change the representation of computed field and group elements.
Even libsecp256k1's configuration options, including `ECMULT_WINDOW_SIZE`, all can affect the representation of the computed group elements.
Therefore we need to fix these options to specific values.

Simplicity computations are on public data and therefore we do not jet functions that manipulate private or secret data.
In particular, we only need to jet variable-time algorithms when there is a choice of variable-time or constant-time algorithms.

In incorporating the libsecp256k1 library into the Simplicity library, there is a tension between making minimal changes to the library versus removing configuration options and other code that, if they were activated, could cause consensus incompatible changes in functionality.
Because we will not be able to easily migrate to newer versions of libsecp256k1 anyways, we have taken a heavy-handed approach to trimming unused configuration options, dead code, functions specific to working with secret data, etc.
In some cases we have made minor code changes:

* `secp256k1_fe_sqrt` has been modified to call `secp256k1_fe_equal_var` (as `secp256k1_fe_equal` has been removed).  The function has been renamed to `secp256k1_fe_sqrt_var` and similar for other indirect callers.
* The implementation of `secp256k1_gej_eq_ge_var` is taken from <https://github.com/bitcoin-core/secp256k1/tree/a47cd97d51e37c38ecf036d04e48518f6b0063f7>.
* The use of secp256k1's `hash.h` for Schnorr signatures has been replaced with calls to Simplicity's internal `sha256.h` implementation.  This removes the duplication of functionality ~~and replaces the non-portable use of the `WORDS_BIGENDIAN` flag in `hash_impl.h` with our portable implementation~~.
* `checked_malloc` and `checked_realloc` have been removed along with any functions that called them.
* `ARG_CHECK` doesn't call the callback.
* Callbacks have been removed.
* `secp256k1_context` has been removed.
* `shallue_van_de_woestijne`'s implementation is taken from https://github.com/BlockstreamResearch/secp256k1-zkp/blob/03aecafe4c45f51736ce05b339d2e8bcc2e5da55/src/modules/generator/main_impl.h>, which fixes <https://github.com/BlockstreamResearch/secp256k1-zkp/issues/279>.

Additionally, some changes have been made to ensure that the `infinity` flag of `secp256k1_gej` always corresponds to whether or not the z-coordinate is zero or not.
Adjustments have been made in the following functions:

* `secp256k1_gej_set_ge`
* `secp256k1_gej_double_var`
* `secp256k1_gej_add_zinv_var`

Also, our jets are designed to operate on off-curve points.
However, the ecmult algorithms in libsecp256k1 are not designed to handle extremely low order, off-curve points[^1].
We have patched `secp256k1_gej_add_ge_var` to ensure `rzr` is set even when `a` is infinity.

Lastly, all active uses of normalize are replaced with the variable-time implementation.

[^1]: More specifically, the when a point has a very low and odd order, the `ai` values in the `secp256k1_ecmult_odd_multiples_table` can reach infinity, violating libsecp256k1's assumption that `secp256k1_gej_add_ge_var`'s `a` parameter is never infinity.
The value we set to the `rzr` in this case does not matter since it ends up only being multiplied with zero in `secp256k1_ge_table_set_globalz`.
It just needs to be set to some value to avoid reading uninitialized memory.
