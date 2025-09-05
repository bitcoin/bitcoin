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
 *  or spending Silent Payment outputs can be determined.
 *
 *  Note that this module is _not_ a full implementation of BIP352, as it
 *  inherently doesn't deal with higher-level concepts like addresses, output
 *  script types or transactions. The intent is to provide a module for
 *  abstracting away the elliptic-curve operations required for the protocol. For
 *  any wallet software already using libsecp256k1, this API should provide all
 *  the functions needed for a Silent Payments implementation without requiring
 *  any further elliptic-curve operations from the wallet.
 */

static const unsigned char secp256k1_silentpayments_prevouts_summary_magic[4] = { 0xa7, 0x1c, 0xd3, 0x5e };

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

/** Create Silent Payment outputs for recipient(s).
 *
 *  Given a list of n secret keys a_1...a_n (one for each silent payment
 *  eligible input to spend), a serialized outpoint, and a list of recipients,
 *  create the taproot outputs. Inputs with conditional branches or multiple
 *  public keys are excluded from silent payments eligible inputs; see BIP352
 *  for more information.
 *
 *  `outpoint_smallest36` refers to the smallest outpoint lexicographically
 *  from the transaction inputs (both silent payments eligible and non-eligible
 *  inputs). This value MUST be the smallest outpoint out of all of the
 *  transaction inputs, otherwise the recipient will be unable to find the
 *  payment. Determining the smallest outpoint from the list of transaction
 *  inputs is the responsibility of the caller. It is strongly recommended
 *  that implementations ensure they are doing this correctly by using the
 *  test vectors from BIP352.
 *
 *  Returns: 1 if creation of outputs was successful.
 *           0 if the input private keys sum to zero,
 *             or input_hash or hash(shared_secret || k) are invalid scalars
 *             (statistically improbable).
 *  Args:                ctx: pointer to a context object
 *  Out:   generated_outputs: pointer to an array of pointers to xonly pubkeys,
 *                            one per recipient.
 *                            The outputs are ordered to match the original
 *                            ordering of the recipient objects, i.e.,
 *                            `generated_outputs[0]` is the generated output
 *                            for the `_silentpayments_recipient` object with
 *                            index = 0.
 *  In:           recipients: pointer to an array of pointers to silent payment
 *                            recipients, where each recipient is a scan public
 *                            key, a spend public key, and an index indicating
 *                            its position in the original ordering. The
 *                            recipient array will be grouped by scan public key
 *                            in place (as specified in BIP0352), but generated
 *                            outputs are saved in the `generated_outputs` array
 *                            to match the original ordering (using the index
 *                            field). This ensures the caller is able to match
 *                            the generated outputs to the correct silent
 *                            payment addresses. The same recipient can be
 *                            passed multiple times to create multiple outputs
 *                            for the same recipient.
 *              n_recipients: the number of recipients. This is equal to the
 *                            total number of outputs to be generated as each
 *                            recipient may passed multiple times to generate
 *                            multiple outputs for the same recipient
 *       outpoint_smallest36: serialized (36-byte) smallest outpoint
 *                            (lexicographically) from the transaction inputs
 *           taproot_seckeys: pointer to an array of pointers to taproot
 *                            keypair inputs (can be NULL if no secret keys
 *                            of taproot inputs are used)
 *         n_taproot_seckeys: the number of sender's taproot input secret keys
 *             plain_seckeys: pointer to an array of pointers to 32-byte
 *                            secret keys of non-taproot inputs (can be NULL
 *                            if no secret keys of non-taproot inputs are
 *                            used)
 *           n_plain_seckeys: the number of sender's non-taproot input secret
 *                            keys
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

/** Create Silent Payment label tweak and label.
 *
 *  Given a recipient's 32 byte scan key and a label integer m, calculate the
 *  corresponding label tweak and label:
 *
 *      label_tweak = hash(recipient_scan_key || m)
 *            label = label_tweak * G
 *
 *  Returns: 1 if label tweak and label creation was successful.
 *           0 if label_tweak32 is an invalid scalar (statistically improbable).
 *  Args:                ctx: pointer to a context object
 *  Out:               label: pointer to the resulting label public key
 *             label_tweak32: pointer to the 32 byte label tweak
 *  In: recipient_scan_key32: pointer to the recipient's 32 byte scan key
 *                         m: label integer (0 is used for change outputs)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_create_label(
    const secp256k1_context *ctx,
    secp256k1_pubkey *label,
    unsigned char *label_tweak32,
    const unsigned char *recipient_scan_key32,
    const uint32_t m
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Create Silent Payment labeled spend public key.
 *
 *  Given a recipient's spend public key and a label, calculate the
 *  corresponding labeled spend public key:
 *
 *      labeled_spend_pubkey = recipient_spend_pubkey + label
 *
 *  The result is used by the recipient to create a Silent Payment address,
 *  consisting of the serialized and concatenated scan public key and
 *  (labeled) spend public key.
 *
 *  Returns: 1 if labeled spend public key creation was successful.
 *           0 if the input public keys are invalid,
 *             or the spend pubkey + label sum to zero (statistically improbable).
 *  Args:                    ctx: pointer to a context object
 *  Out:    labeled_spend_pubkey: pointer to the resulting labeled spend
 *                                public key
 *  In:   unlabeled_spend_pubkey: pointer to the recipient's unlabeled spend pubkey
 *                         label: pointer to the recipient's label public
 *                                key
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(
    const secp256k1_context *ctx,
    secp256k1_pubkey *labeled_spend_pubkey,
    const secp256k1_pubkey *unlabeled_spend_pubkey,
    const secp256k1_pubkey *label
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Opaque data structure that holds silent payments prevout summary data.
 *
 *  This structure does not contain secret data. Guaranteed to be 101 bytes in
 *  size. It can be safely copied/moved. Created with
 *  `secp256k1_silentpayments_recipient_prevouts_summary_create`. Can be serialized as a
 *  compressed public key using
 *  `secp256k1_silentpayments_recipient_prevouts_summary_serialize`. The serialization is
 *  intended for sending the prevout summary data to light clients. Light clients
 *  can use this serialization with
 *  `secp256k1_silentpayments_recipient_prevouts_summary_parse`.
 */
typedef struct secp256k1_silentpayments_recipient_prevouts_summary {
    unsigned char data[101];
} secp256k1_silentpayments_recipient_prevouts_summary;

/** Compute Silent Payment prevout summary from prevout public keys and transaction
 *  inputs.
 *
 *  Given a list of n public keys A_1...A_n (one for each silent payment
 *  eligible input to spend) and a serialized outpoint_smallest36, create a
 *  `prevouts_summary` object. This object summarizes the prevout data from the
 *  transaction inputs needed for scanning.
 *
 *  `outpoint_smallest36` refers to the smallest outpoint lexicographically
 *  from the transaction inputs (both silent payments eligible and non-eligible
 *  inputs). This value MUST be the smallest outpoint out of all of the
 *  transaction inputs, otherwise the recipient will be unable to find the
 *  payment.
 *
 *  The public keys have to be passed in via two different parameter pairs, one
 *  for regular and one for x-only public keys, in order to avoid the need of
 *  users converting to a common pubkey format before calling this function.
 *  The resulting data can be used for scanning on the recipient side, or
 *  stored in an index for later use (e.g., wallet rescanning, sending data to
 *  light clients).
 *
 *  If calling this function for simply aggregating the public transaction data
 *  for later use, the caller can save the result with
 *  `silentpayments_recipient_prevouts_summary_serialize`.
 *
 *  Returns: 1 if prevout summary creation was successful.
 *           0 if the input public keys sum to zero,
 *             or the input_hash is an invalid scalar (statistically improbable).
 *  Args:                 ctx: pointer to a context object
 *  Out:     prevouts_summary: pointer to prevouts_summary object containing the
 *                             summed public key and input_hash.
 *  In:   outpoint_smallest36: serialized smallest outpoint (lexicographically)
 *                             from the transaction inputs
 *              xonly_pubkeys: pointer to an array of pointers to taproot
 *                             x-only public keys (can be NULL if no taproot
 *                             inputs are used)
 *            n_xonly_pubkeys: the number of taproot input public keys
 *              plain_pubkeys: pointer to an array of pointers to non-taproot
 *                             public keys (can be NULL if no non-taproot
 *                             inputs are used)
 *            n_plain_pubkeys: the number of non-taproot input public keys
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_prevouts_summary_create(
    const secp256k1_context *ctx,
    secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary,
    const unsigned char *outpoint_smallest36,
    const secp256k1_xonly_pubkey * const *xonly_pubkeys,
    size_t n_xonly_pubkeys,
    const secp256k1_pubkey * const *plain_pubkeys,
    size_t n_plain_pubkeys
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a silentpayments_recipient_prevouts_summary object into a 33-byte sequence.
 *
 *  Serializing a prevouts_summary object created with `_recipent_prevouts_summary_create` will result in
 *  an EC multiplication. This allows for a more compact serialization.
 *
 *  Returns: 1 always.
 *
 *  Args:            ctx: pointer to a context object
 *  Out:        output33: pointer to a 33-byte array to place the serialized
 *                        `silentpayments_recipient_prevouts_summary` in
 *  In: prevouts_summary: pointer to an initialized silentpayments_recipient_prevouts_summary
 *                        object
 */
SECP256K1_API int secp256k1_silentpayments_recipient_prevouts_summary_serialize(
    const secp256k1_context *ctx,
    unsigned char *output33,
    const secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Parse a 33-byte sequence into a silentpayments_recipients_prevouts_summary object.
 *
 *  Returns: 1 if the data was able to be parsed.
 *           0 if the sequence is invalid (e.g., does not represent a valid
 *           public key).
 *
 *  Args:              ctx: pointer to a context object.
 *  Out:  prevouts_summary: pointer to a silentpayments_recipient_prevouts_summary object. If 1 is
 *                     returned, it is set to a parsed version of input33.
 *  In:       input33: pointer to a serialized silentpayments_recipient_prevouts_summary.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_prevouts_summary_parse(
    const secp256k1_context *ctx,
    secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary,
    const unsigned char *input33
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
 *  In:         label: pointer to the label pubkey to check (computed during
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

/** Scan for Silent Payment transaction outputs.
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
 *           0 if any combination of the shared secret, label and spend pubkey
 *             sum to zero (statistically improbable).
 *
 *  Args:                   ctx: pointer to a context object
 *  Out:          found_outputs: pointer to an array of pointers to found
 *                               output objects. The found outputs array MUST
 *                               be initialized to be the same length as the
 *                               tx_outputs array
 *              n_found_outputs: pointer to an integer indicating the final
 *                               size of the found outputs array. This number
 *                               represents the number of outputs found while
 *                               scanning (0 if none are found)
 *  In:              tx_outputs: pointer to the tx's x-only public key outputs
 *                 n_tx_outputs: the number of tx_outputs being scanned
 *         recipient_scan_key32: pointer to the recipient's 32 byte scan key
 *             prevouts_summary: pointer to the transaction prevouts summary data
 *                               (see `_recipient_prevouts_summary_create`).
 *       unlabeled_spend_pubkey: pointer to the recipient's unlabeled spend pubkey
 *                 label_lookup: pointer to a callback function for looking up
 *                               a label value. This function takes a label
 *                               pubkey as an argument and returns a pointer to
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
    const secp256k1_xonly_pubkey * const *tx_outputs,
    size_t n_tx_outputs,
    const unsigned char *recipient_scan_key32,
    const secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary,
    const secp256k1_pubkey *unlabeled_spend_pubkey,
    const secp256k1_silentpayments_label_lookup label_lookup,
    const void *label_context
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4)
    SECP256K1_ARG_NONNULL(6) SECP256K1_ARG_NONNULL(7) SECP256K1_ARG_NONNULL(8);

/** Create Silent Payment shared secret.
 *
 *  Given the public input data (secp256k1_silentpayments_recipient_prevouts_summary),
 *  and the recipient's 32 byte scan key, calculate the shared secret.
 *
 *  The resulting shared secret is needed as input for creating silent payments
 *  outputs belonging to the same recipient scan public key. This function is
 *  intended for light clients, i.e., scenarios where the caller does not have
 *  access to the full transaction. If the caller does have access to the full
 *  transaction, `secp256k1_silentpayments_recipient_scan_outputs` should be
 *  used instead.
 *
 *  Returns: 1 if shared secret creation was successful.
 *           0 if the recipient scan key is invalid.
 *  Args:                  ctx: pointer to a context object
 *  Out:       shared_secret33: pointer to the resulting 33-byte shared secret
 *  In:   recipient_scan_key32: pointer to the recipient's 32 byte scan key
 *            prevouts_summary: pointer to the prevouts_summary object, loaded using
 *                              `_recipient_prevouts_summary_parse`
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_create_shared_secret(
    const secp256k1_context *ctx,
    unsigned char *shared_secret33,
    const unsigned char *recipient_scan_key32,
    const secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Create Silent Payment output public key.
 *
 *  Given a shared_secret, a recipient spend public key, and an output counter k,
 *  create an output public key.
 *
 *  This function is used by the recipient when scanning for outputs without
 *  access to the transaction outputs (e.g., using BIP158 block filters). When
 *  scanning with this function, it is the scanners responsibility to determine
 *  if the generated output exists in a block before proceeding to the next
 *  value of `k`.
 *
 *  Returns: 1 if output creation was successful.
 *           0 if hash(shared secret || k) results in an invalid scalar
 *             (statistically improbable).
 *  Args:                   ctx: pointer to a context object
 *  Out:           output_xonly: pointer to the resulting output x-only pubkey
 *  In:         shared_secret33: shared secret, derived from either sender's
 *                               or recipient's perspective with routines from
 *                               above
 *                 spend_pubkey: pointer to the recipient's spend pubkey
 *                               (labeled or unlabeled)
 *                            k: output counter (initially set to 0, must be
 *                               incremented for each additional output created
 *                               or after each output found when scanning)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_silentpayments_recipient_create_output_pubkey(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey *output_xonly,
    const unsigned char *shared_secret33,
    const secp256k1_pubkey *spend_pubkey,
    const uint32_t k
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SILENTPAYMENTS_H */
