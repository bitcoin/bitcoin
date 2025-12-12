#ifndef SECP256K1_SILENTPAYMENTS_H
#define SECP256K1_SILENTPAYMENTS_H

#include <stdint.h>
#include "secp256k1.h"
#include "secp256k1_extrakeys.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This module provides an implementation for Silent Payments, as specified in
 *  BIP352. This particularly involves the creation of input tweak data by
 *  summing up secret or public keys and the derivation of a shared secret using
 *  Elliptic Curve Diffie-Hellman. Combined are either:
 *    - spender's secret keys and recipient's public key (a * B, sender side)
 *    - spender's public keys and recipient's secret key (A * b, recipient side)
 *  With this result, the necessary key material for ultimately creating/scanning
 *  or spending Silent Payments outputs can be determined.
 *
 *  Note that this module is _not_ a full implementation of BIP352, as it
 *  inherently doesn't deal with higher-level concepts like addresses, output
 *  script types or transactions. The intent is to provide a module for
 *  abstracting away the elliptic-curve operations required for the protocol. For
 *  any wallet software already using libsecp256k1, this API should provide all
 *  the functions needed for a Silent Payments implementation without requiring
 *  any further elliptic-curve operations from the wallet.
 */

/** The data from a single recipient address
 *
 *  This struct serves as an input argument to `silentpayments_sender_create_outputs`.
 *
 *  `index` must be set to the position (starting with 0) of this recipient in the
 *  `recipients` array passed to `silentpayments_sender_create_outputs`. It is
 *  used to map the returned generated outputs back to the original recipient.
 *
 *  Note:
 *  The spend public key named `spend_pubkey` may have been optionally tweaked with
 *  a label by the recipient. Whether `spend_pubkey` has actually been tagged with
 *  a label is irrelevant for the sender. As a documentation convention in this API,
 *  `unlabeled_spend_pubkey` is used to indicate when the unlabeled spend public must
 *  be used.
 */
typedef struct secp256k1_silentpayments_recipient {
    secp256k1_pubkey scan_pubkey;
    secp256k1_pubkey spend_pubkey;
    size_t index;
} secp256k1_silentpayments_recipient;

/** Create Silent Payments outputs for recipient(s).
 *
 *  Given a list of n secret keys a_1...a_n (one for each Silent Payments
 *  eligible input to spend), a serialized outpoint, and a list of recipients,
 *  create the taproot outputs. Inputs with conditional branches or multiple
 *  public keys are excluded from Silent Payments eligible inputs; see BIP352
 *  for more information.
 *
 *  `outpoint_smallest36` refers to the smallest outpoint lexicographically
 *  from the transaction inputs (both Silent Payments eligible and non-eligible
 *  inputs). This value MUST be the smallest outpoint out of all of the
 *  transaction inputs, otherwise the recipient will be unable to find the
 *  payment. Determining the smallest outpoint from the list of transaction
 *  inputs is the responsibility of the caller. It is strongly recommended
 *  that implementations ensure they are doing this correctly by using the
 *  test vectors from BIP352.
 *
 *  When creating more than one generated output, all of the generated outputs
 *  MUST be included in the final transaction. Dropping any of the generated
 *  outputs from the final transaction may make all or some of the outputs
 *  unfindable by the recipient.
 *
 *  Returns: 1 if creation of outputs was successful.
 *           0 on failure. This is expected only with an adversarially chosen
 *           recipient spend key. Specifically, failure occurs when:
 *             - Input secret keys sum to 0 or the negation of a spend key
 *               (negligible probability if at least one of the input secret
 *               keys is uniformly random and independent of all other keys)
 *             - A hash output is not a valid scalar (negligible probability
 *               per hash evaluation)
 *
 *  Args:                ctx: pointer to a context object
 *                            (not secp256k1_context_static).
 *  Out:   generated_outputs: pointer to an array of pointers to xonly public keys,
 *                            one per recipient.
 *                            The outputs are ordered to match the original
 *                            ordering of the recipient objects, i.e.,
 *                            `generated_outputs[0]` is the generated output
 *                            for the `_silentpayments_recipient` object with
 *                            index = 0.
 *  In:           recipients: pointer to an array of pointers to Silent Payments
 *                            recipients, where each recipient is a scan public
 *                            key, a spend public key, and an index indicating
 *                            its position in the original ordering. The
 *                            recipient array will be grouped by scan public key
 *                            in place (as specified in BIP0352), but generated
 *                            outputs are saved in the `generated_outputs` array
 *                            to match the original ordering (using the index
 *                            field). This ensures the caller is able to match
 *                            the generated outputs to the correct Silent
 *                            Payments addresses. The same recipient can be
 *                            passed multiple times to create multiple outputs
 *                            for the same recipient.
 *              n_recipients: the size of the recipients array.
 *       outpoint_smallest36: serialized (36-byte) smallest outpoint
 *                            (lexicographically) from the transaction inputs
 *           taproot_seckeys: pointer to an array of pointers to taproot
 *                            keypair inputs (can be NULL if no secret keys
 *                            of taproot inputs are used)
 *         n_taproot_seckeys: the size of taproot_seckeys array.
 *             plain_seckeys: pointer to an array of pointers to 32-byte
 *                            secret keys of non-taproot inputs (can be NULL
 *                            if no secret keys of non-taproot inputs are
 *                            used)
 *           n_plain_seckeys: the size of the plain_seckeys array.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_sender_create_outputs(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey **generated_outputs,
    const secp256k1_silentpayments_recipient **recipients,
    size_t n_recipients,
    const unsigned char *outpoint_smallest36,
    const secp256k1_keypair * const *taproot_seckeys,
    size_t n_taproot_seckeys,
    const unsigned char * const *plain_seckeys,
    size_t n_plain_seckeys
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5);

/** Create Silent Payments label tweak and label.
 *
 *  Given a recipient's 32 byte scan key and a label integer m, calculate the
 *  corresponding label tweak and label:
 *
 *      label_tweak = hash(scan_key || m)
 *            label = label_tweak * G
 *
 *  Returns: 1 if label tweak and label creation was successful.
 *           0 if hash output label_tweak32 is not valid scalar (negligible
 *             probability per hash evaluation).
 *
 *  Args:                ctx: pointer to a context object
 *                            (not secp256k1_context_static)
 *  Out:               label: pointer to the resulting label public key
 *             label_tweak32: pointer to the 32 byte label tweak
 *  In:           scan_key32: pointer to the recipient's 32 byte scan key
 *                         m: integer for the m-th label (0 is used for change outputs)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_create_label(
    const secp256k1_context *ctx,
    secp256k1_pubkey *label,
    unsigned char *label_tweak32,
    const unsigned char *scan_key32,
    uint32_t m
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Create Silent Payments labeled spend public key.
 *
 *  Given a recipient's spend public key and a label, calculate the
 *  corresponding labeled spend public key:
 *
 *      labeled_spend_pubkey = unlabeled_spend_pubkey + label
 *
 *  The result is used by the recipient to create a Silent Payments address,
 *  consisting of the serialized and concatenated scan public key and
 *  (labeled) spend public key.
 *
 *  Returns: 1 if labeled spend public key creation was successful.
 *           0 if spend pubkey and label sum to zero (negligible probability for
 *             labels created according to BIP352).
 *
 *  Args:                    ctx: pointer to a context object
 *  Out:    labeled_spend_pubkey: pointer to the resulting labeled spend public key
 *  In:   unlabeled_spend_pubkey: pointer to the recipient's unlabeled spend public key
 *                         label: pointer to the recipient's label public key
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(
    const secp256k1_context *ctx,
    secp256k1_pubkey *labeled_spend_pubkey,
    const secp256k1_pubkey *unlabeled_spend_pubkey,
    const secp256k1_pubkey *label
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Opaque data structure that holds Silent Payments prevouts summary data.
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. It is
 *  however guaranteed to be 101 bytes in size, and can be safely copied/moved.
 *  This structure does not contain secret data. It can be created with
 *  `secp256k1_silentpayments_recipient_prevouts_summary_create`.
 */
typedef struct secp256k1_silentpayments_prevouts_summary {
    unsigned char data[101];
} secp256k1_silentpayments_prevouts_summary;

/** Compute Silent Payments prevouts summary from prevout public keys and transaction
 *  inputs.
 *
 *  Given a list of n public keys A_1...A_n (one for each Silent Payments
 *  eligible input to spend) and a serialized outpoint_smallest36, create a
 *  `prevouts_summary` object. This object summarizes the prevout data from the
 *  transaction inputs needed for scanning.
 *
 *  `outpoint_smallest36` refers to the smallest outpoint lexicographically
 *  from the transaction inputs (both Silent Payments eligible and non-eligible
 *  inputs). This value MUST be the smallest outpoint out of all of the
 *  transaction inputs, otherwise the recipient will be unable to find the
 *  payment.
 *
 *  The public keys have to be passed in via two different parameter pairs, one
 *  for regular and one for x-only public keys, in order to avoid the need of
 *  users converting to a common public key format before calling this function.
 *  The resulting data can be used for scanning on the recipient side.
 *
 *  Returns: 1 if prevouts summary creation was successful.
 *           0 if the transaction is not a Silent Payments transaction.
 *
 *  Args:                 ctx: pointer to a context object
 *  Out:     prevouts_summary: pointer to prevouts_summary object containing the
 *                             summed public key and input_hash.
 *  In:   outpoint_smallest36: serialized smallest outpoint (lexicographically)
 *                             from the transaction inputs
 *              xonly_pubkeys: pointer to an array of pointers to taproot
 *                             x-only public keys (can be NULL if no taproot
 *                             inputs are used)
 *            n_xonly_pubkeys: the size of the xonly_pubkeys array.
 *              plain_pubkeys: pointer to an array of pointers to non-taproot
 *                             public keys (can be NULL if no non-taproot
 *                             inputs are used)
 *            n_plain_pubkeys: the size of the plain_pubkeys array.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_prevouts_summary_create(
    const secp256k1_context *ctx,
    secp256k1_silentpayments_prevouts_summary *prevouts_summary,
    const unsigned char *outpoint_smallest36,
    const secp256k1_xonly_pubkey * const *xonly_pubkeys,
    size_t n_xonly_pubkeys,
    const secp256k1_pubkey * const *plain_pubkeys,
    size_t n_plain_pubkeys
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Callback function for label lookups
 *
 *  This function is implemented by the recipient to check if a value exists in
 *  the recipients label cache during scanning.
 *
 *  For creating the labels cache data,
 *  `secp256k1_silentpayments_recipient_create_label` can be used.
 *
 *  Returns: pointer to the 32-byte label tweak if there is a match.
 *           NULL pointer if there is no match.
 *
 *  In:         label: pointer to the label public key to check (computed during
 *                     scanning)
 *      label_context: pointer to the recipients label cache.
 */
typedef const unsigned char* (*secp256k1_silentpayments_label_lookup)(const unsigned char* label33, const void* label_context);

/** Found outputs struct
 *
 *  Struct for holding a found output along with data needed to spend it later.
 *
 *            output: the x-only public key for the taproot output
 *             tweak: the 32-byte tweak needed to spend the output
 *  found_with_label: boolean value to indicate if the output was sent to a
 *                    labeled address. If true, label will be set with a valid
 *                    public key.
 *             label: public key representing the label used.
 *                    If found_with_label = false, this is set to an invalid
 *                    public key.
 */
typedef struct secp256k1_silentpayments_found_output {
    secp256k1_xonly_pubkey output;
    unsigned char tweak[32];
    int found_with_label;
    secp256k1_pubkey label;
} secp256k1_silentpayments_found_output;

/** Scan for Silent Payments transaction outputs.
 *
 *  Given a prevouts_summary object, a recipient's 32 byte scan key and spend public key,
 *  and the relevant transaction outputs, scan for outputs belonging to
 *  the recipient and return the tweak(s) needed for spending the output(s). An
 *  optional label_lookup callback function and label_context can be passed if
 *  the recipient uses labels. This allows for checking if a label exists in
 *  the recipients label cache and retrieving the label tweak during scanning.
 *
 *  If used, the `label_lookup` function must return a pointer to a 32-byte label
 *  tweak if the label is found, or NULL otherwise. The returned pointer must remain
 *  valid until the next call to `label_lookup` or until the function returns,
 *  whichever comes first. It is not retained beyond that.
 *
 *  For creating the labels cache, `secp256k1_silentpayments_recipient_create_label`
 *  can be used.
 *
 *  Returns: 1 if output scanning was successful.
 *           0 if the transaction is not a Silent Payments transaction,
 *             or if the arguments are invalid.
 *
 *  Args:                   ctx: pointer to a context object
 *  Out:          found_outputs: pointer to an array of pointers to found
 *                               output objects. The found outputs array MUST
 *                               have the same length as the tx_outputs array.
 *              n_found_outputs: pointer to an integer indicating the final
 *                               size of the found outputs array. This number
 *                               represents the number of outputs found while
 *                               scanning (0 if none are found).
 *  In:              tx_outputs: pointer to the transaction's x-only public key outputs
 *                 n_tx_outputs: the size of the tx_outputs array.
 *                   scan_key32: pointer to the recipient's 32 byte scan key. The scan
 *                               key is valid if it passes secp256k1_ec_seckey_verify
 *             prevouts_summary: pointer to the transaction prevouts summary data
 *                               (see `_recipient_prevouts_summary_create`).
 *       unlabeled_spend_pubkey: pointer to the recipient's unlabeled spend public key
 *                 label_lookup: pointer to a callback function for looking up
 *                               a label value. This function takes a label
 *                               public key as an argument and returns a pointer to
 *                               the label tweak if the label exists, otherwise
 *                               returns a NULL pointer (NULL if labels are not
 *                               used)
 *                label_context: pointer to a label context object (NULL if
 *                               labels are not used or context is not needed)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_scan_outputs(
    const secp256k1_context *ctx,
    secp256k1_silentpayments_found_output **found_outputs,
    size_t *n_found_outputs,
    const secp256k1_xonly_pubkey **tx_outputs,
    size_t n_tx_outputs,
    const unsigned char *scan_key32,
    const secp256k1_silentpayments_prevouts_summary *prevouts_summary,
    const secp256k1_pubkey *unlabeled_spend_pubkey,
    const secp256k1_silentpayments_label_lookup label_lookup,
    const void *label_context
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(6) SECP256K1_ARG_NONNULL(7) SECP256K1_ARG_NONNULL(8);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SILENTPAYMENTS_H */
