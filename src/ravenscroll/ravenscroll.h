/*
 * RavenScroll 2.1 - C library
 *
 * Copyright (c) Coin Sciences Ltd
 * Copyright (c) 2017 The Raven Core developers
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifndef _RAVENSCROLL_H_
#define _RAVENSCROLL_H_

#include <stdlib.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif
    

// RavenScroll constants and other macros
    
#define TRUE 1
#define FALSE 0
    
#define RAVENSCROLL_MIN(a,b) (((a)<(b)) ? (a) : (b))
#define RAVENSCROLL_MAX(a,b) (((a)>(b)) ? (a) : (b))
    
#define RAVENSCROLL_SATOSHI_QTY_MAX 2100000000000000
#define RAVENSCROLL_ASSET_QTY_MAX 100000000000000
#define RAVENSCROLL_PAYMENT_REF_MAX 0xFFFFFFFFFFFFF // 2^52-1
  
#define RAVENSCROLL_GENESIS_QTY_MANTISSA_MIN 1
#define RAVENSCROLL_GENESIS_QTY_MANTISSA_MAX 1000
#define RAVENSCROLL_GENESIS_QTY_EXPONENT_MIN 0
#define RAVENSCROLL_GENESIS_QTY_EXPONENT_MAX 11
#define RAVENSCROLL_GENESIS_CHARGE_FLAT_MAX 5000
#define RAVENSCROLL_GENESIS_CHARGE_FLAT_MANTISSA_MIN 0
#define RAVENSCROLL_GENESIS_CHARGE_FLAT_MANTISSA_MAX 100
#define RAVENSCROLL_GENESIS_CHARGE_FLAT_MANTISSA_MAX_IF_EXP_MAX 50
#define RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MIN 0
#define RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MAX 2
#define RAVENSCROLL_GENESIS_CHARGE_BASIS_POINTS_MIN 0
#define RAVENSCROLL_GENESIS_CHARGE_BASIS_POINTS_MAX 250
#define RAVENSCROLL_GENESIS_DOMAIN_NAME_MAX_LEN 32
#define RAVENSCROLL_GENESIS_PAGE_PATH_MAX_LEN 24
#define RAVENSCROLL_GENESIS_HASH_MIN_LEN 12
#define RAVENSCROLL_GENESIS_HASH_MAX_LEN 32

#define RAVENSCROLL_ASSETREF_BLOCK_NUM_MAX 4294967295
#define RAVENSCROLL_ASSETREF_TX_OFFSET_MAX 4294967295
#define RAVENSCROLL_ASSETREF_TXID_PREFIX_LEN 2

#define RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE -1 // magic number for a default route
    
#define RAVENSCROLL_MESSAGE_SERVER_HOST_MAX_LEN 32
#define RAVENSCROLL_MESSAGE_SERVER_PATH_MAX_LEN 24
#define RAVENSCROLL_MESSAGE_HASH_MIN_LEN 12
#define RAVENSCROLL_MESSAGE_HASH_MAX_LEN 32
#define RAVENSCROLL_MESSAGE_MAX_IO_RANGES 16

#define RAVENSCROLL_IO_INDEX_MAX 65535

#define RAVENSCROLL_ADDRESS_FLAG_ASSETS 1
#define RAVENSCROLL_ADDRESS_FLAG_PAYMENT_REFS 2
#define RAVENSCROLL_ADDRESS_FLAG_TEXT_MESSAGES 4
#define RAVENSCROLL_ADDRESS_FLAG_FILE_MESSAGES 8
#define RAVENSCROLL_ADDRESS_FLAG_MASK 0x7FFFFF // 23 bits are currently usable

// RavenScroll type definitions

#ifndef __cplusplus
    typedef char bool;
#endif
    
typedef int64_t RavenScrollSatoshiQty;
typedef int64_t RavenScrollAssetQty;
typedef int32_t RavenScrollIOIndex;
typedef int32_t RavenScrollAddressFlags;
typedef int64_t RavenScrollPaymentRef;
    
typedef struct {
    char bitcoinAddress[64];
    RavenScrollAddressFlags addressFlags;
    RavenScrollPaymentRef paymentRef;
} RavenScrollAddress;
    
typedef struct {
    int16_t qtyMantissa;
    int16_t qtyExponent;
    int16_t chargeFlatMantissa;
    int16_t chargeFlatExponent;
    int16_t chargeBasisPoints; // one hundredths of a percent
    bool useHttps;
    char domainName[RAVENSCROLL_GENESIS_DOMAIN_NAME_MAX_LEN+1]; // null terminated
    bool usePrefix; // prefix ravenscroll/ in asset web page URL path
    char pagePath[RAVENSCROLL_GENESIS_PAGE_PATH_MAX_LEN+1]; // null terminated
    int8_t assetHash[RAVENSCROLL_GENESIS_HASH_MAX_LEN];
    size_t assetHashLen; // number of bytes in assetHash that are valid for comparison/encoding
} RavenScrollGenesis;

typedef struct {
    int64_t blockNum; // block in which genesis transaction is confirmed
    int64_t txOffset; // byte offset within that block
    u_int8_t txIDPrefix[RAVENSCROLL_ASSETREF_TXID_PREFIX_LEN]; // first bytes of genesis transaction id
} RavenScrollAssetRef;

typedef struct {
    RavenScrollIOIndex first, count;
} RavenScrollIORange;

typedef struct {
    RavenScrollAssetRef assetRef;
    RavenScrollIORange inputs;
    RavenScrollIORange outputs;
    RavenScrollAssetQty qtyPerOutput;
} RavenScrollTransfer;
    
typedef struct {
    bool useHttps;
    char serverHost[RAVENSCROLL_MESSAGE_SERVER_HOST_MAX_LEN+1]; // null terminated
    bool usePrefix; // prefix ravenscroll/ in server path
    char serverPath[RAVENSCROLL_MESSAGE_SERVER_PATH_MAX_LEN+1]; // null terminated
    bool isPublic; // is the message publicly viewable
    int countOutputRanges; // number of elements in output range array
    RavenScrollIORange outputRanges[RAVENSCROLL_MESSAGE_MAX_IO_RANGES]; // array of output ranges
    int8_t hash[RAVENSCROLL_MESSAGE_HASH_MAX_LEN];
    size_t hashLen; // number of bytes in hash that are valid for comparison/encoding
} RavenScrollMessage;
    
    
// General functions for managing RavenScroll metadata and bitcoin transaction output scripts
    
size_t RavenScrollScriptToMetadata(const char* scriptPubKey, const size_t scriptPubKeyLen, const bool scriptIsHex,
                                 char* metadata, const size_t metadataMaxLen);
    // Checks whether the bitcoin tx output script in scriptPubKey (byte length scriptPubKeyLen) contains OP_RETURN metadata.
    // If scriptIsHex is true, scriptPubKey is interpreted as hex text, otherwise as raw binary data.
    // If so, copy it to metadata (size metadataMaxLen) and return the number of bytes used.
    // If no OP_RETURN data is found or metadataMaxLen is too small, returns 0.
    
size_t RavenScrollScriptsToMetadata(const char* scriptPubKeys[], const size_t scriptPubKeyLens[], const bool scriptsAreHex,
                                  const int countScripts, char* metadata, const size_t metadataMaxLen);
    // Extracts the first OP_RETURN metadata from the array of bitcoin transaction scripts in scriptPubKeys
    // (array length countScripts) where each script's byte length is in the respective element of scriptPubKeyLens.
    // If scriptsAreHex is true, scriptPubKeys are interpreted as hex text, otherwise as raw binary data.
    // Copies the metadata from the first OP_RETURN to metadata (size metadataMaxLen) and return the number of bytes used.
    // If no OP_RETURN data is found or metadataMaxLen is too small, returns 0.
    
size_t RavenScrollMetadataToScript(const char* metadata, const size_t metadataLen,
                                 char* scriptPubKey, const size_t scriptPubKeyMaxLen, const bool toHexScript);
    // Converts the data in metadata (length metadataLen) to an OP_RETURN bitcoin tx output in scriptPubKey
    // (maximum byte size scriptPubKeyMaxLen). If toHexScript is true, outputs hex text, otherwise binary data.
    // Returns the number of bytes used in scriptPubKey, or 0 if scriptPubKeyMaxLen was too small.
    
size_t RavenScrollMetadataMaxAppendLen(char* metadata, const size_t metadataLen, const size_t metadataMaxLen);
    // Calculates the maximum length of RavenScroll metadata that can be appended to some existing RavenScroll metadata
    // to fit into a total of metadataMaxLen bytes. The existing metadata (size metadataLen) can itself already be
    // a combination of more than one RavenScroll metadata element. The calculation is not simply metadataMaxLen-metadataLen
    // because some space is saved when combining pieces of RavenScroll metadata together.

size_t RavenScrollMetadataAppend(char* metadata, const size_t metadataLen, const size_t metadataMaxLen,
                               const char* appendMetadata, const size_t appendMetadataLen);
    // Appends the RavenScroll metadata in appendMetadata (size appendMetadataLen) to the existing RavenScroll metadata,
    // whose current size is metadataLen and whose maximum size is metadataMaxLen.
    // Returns the new total size of the metadata, or 0 if there was insufficient space or another error.

bool RavenScrollScriptIsRegular(const char* scriptPubKey, const size_t scriptPubKeyLen, const bool scriptIsHex);
    // Returns whether the bitcoin tx output script scriptPubKey (byte length scriptPubKeyLen) is 'regular', i.e. not an OP_RETURN.
    // This function will declare empty scripts or invalid hex scripts as 'regular' as well, since they are not OP_RETURNs.
    // If scriptIsHex is true, scriptPubKey is interpreted as hex text, otherwise as raw binary data.
    // Use this to build outputsRegular arrays which are used by various functions below.


// Functions for managing RavenScroll addresses
    
void RavenScrollAddressClear(RavenScrollAddress* address);
    // Set all fields in address to their default/zero values, which are not necessarily valid.
    
bool RavenScrollAddressToString(const RavenScrollAddress* address, char* string, const size_t stringMaxLen);
    // Outputs the address to a null-delimited string (size stringMaxLen) for debugging.
    // Returns true if there was enough space (1024 bytes recommended) or false otherwise.

bool RavenScrollAddressIsValid(const RavenScrollAddress* address);
    // Returns true if all values in the address are in their permitted ranges, false otherwise.
    
bool RavenScrollAddressMatch(const RavenScrollAddress* address1, const RavenScrollAddress* address2);
    // Returns true if the two RavenScrollAddress structures are identical.
    
size_t RavenScrollAddressEncode(const RavenScrollAddress* address, char* string, const size_t stringMaxLen);
    // Encodes the fields in address to a null-delimited RavenScroll address string (length stringMaxLen).
    // Returns the size of the formatted string if successful, otherwise 0.
    
bool RavenScrollAddressDecode(RavenScrollAddress* address, const char* string, const size_t stringLen);
    // Decodes the RavenScroll address string (length stringLen) into the fields in address.
    // Returns true if the address could be successfully read, otherwise false.
    
    
// Functions for managing asset genesis metadata

void RavenScrollGenesisClear(RavenScrollGenesis *genesis);
    // Set all fields in genesis to their default/zero values, which are not necessarily valid.
    
bool RavenScrollGenesisToString(const RavenScrollGenesis *genesis, char* string, const size_t stringMaxLen);
    // Outputs the genesis to a null-delimited string (size stringMaxLen) for debugging.
    // Returns true if there was enough space (1024 bytes recommended) or false otherwise.
    
bool RavenScrollGenesisIsValid(const RavenScrollGenesis *genesis);
    // Returns true if all values in the genesis are in their permitted ranges, false otherwise.
    
bool RavenScrollGenesisMatch(const RavenScrollGenesis* genesis1, const RavenScrollGenesis* genesis2, const bool strict);
    // Returns true if the two RavenScrollGenesis structures are the same. If strict is true then
    // the qtyMantissa, qtyExponent, chargeFlatMantissa and chargeFlatExponent fields must be identical.
    // If strict is false then it is enough if each pair just represents the same final quantity.

RavenScrollAssetQty RavenScrollGenesisGetQty(const RavenScrollGenesis *genesis);
    // Returns the number of units denoted by the genesis qtyMantissa and qtyExponent fields.
    
RavenScrollAssetQty RavenScrollGenesisSetQty(RavenScrollGenesis *genesis, const RavenScrollAssetQty desiredQty, const int rounding);
    // Sets the qtyMantissa and qtyExponent fields in genesis to be as close to desiredQty as possible.
    // Set rounding to [-1, 0, 1] for rounding [down, closest, up] respectively.
    // Returns the quantity that was actually encoded, via RavenScrollGenesisGetQty().
    
RavenScrollAssetQty RavenScrollGenesisGetChargeFlat(const RavenScrollGenesis *genesis);
    // Returns the number of units denoted by the genesis chargeFlatMantissa and chargeFlatExponent fields.
    
RavenScrollAssetQty RavenScrollGenesisSetChargeFlat(RavenScrollGenesis *genesis, RavenScrollAssetQty desiredChargeFlat, const int rounding);
    // Sets the chargeFlatMantissa and chargeFlatExponent fields in genesis to be as close to desiredChargeFlat as possible.
    // Set rounding to [-1, 0, 1] for rounding [down, closest, up] respectively.
    // Returns the quantity that was actually encoded, via RavenScrollGenesisGetChargeFlat().

RavenScrollAssetQty RavenScrollGenesisCalcCharge(const RavenScrollGenesis *genesis, RavenScrollAssetQty qtyGross);
    // Calculates the payment charge specified by genesis for sending the raw quantity qtyGross.
    
RavenScrollAssetQty RavenScrollGenesisCalcNet(const RavenScrollGenesis *genesis, RavenScrollAssetQty qtyGross);
    // Calculates the quantity that will be received after the payment charge specified by genesis is applied to qtyGross.

RavenScrollAssetQty RavenScrollGenesisCalcGross(const RavenScrollGenesis *genesis, RavenScrollAssetQty qtyNet);
    // Calculates the quantity that should be sent so that, after the payment charge specified by genesis
    // is applied, the recipient will receive qtyNet units.
    
size_t RavenScrollGenesisCalcHashLen(const RavenScrollGenesis *genesis, const size_t metadataMaxLen);
    // Calculates the appropriate asset hash length of genesis so that when encoded as metadata the genesis will
    // fit in metadataMaxLen bytes. For now, set metadataMaxLen to 40 (see Bitcoin's MAX_OP_RETURN_RELAY parameter).


// Functions for encoding, decoding and calculating minimum fees for RavenScroll genesis transactions
    
size_t RavenScrollGenesisEncode(const RavenScrollGenesis *genesis, char* metadata, const size_t metadataMaxLen);
    // Encodes genesis into metadata (size metadataMaxLen).
    // If the encoding was successful, returns the number of bytes used, otherwise 0.

bool RavenScrollGenesisDecode(RavenScrollGenesis* genesis, const char* metadata, const size_t metadataLen);
    // Decodes the data in metadata (length metadataLen) into genesis.
    // Return true if the decode was successful, false otherwise.
    
RavenScrollSatoshiQty RavenScrollGenesisCalcMinFee(const RavenScrollGenesis *genesis, const RavenScrollSatoshiQty* outputsSatoshis,
                                               const bool* outputsRegular, const int countOutputs);
    // Returns the minimum transaction fee (in bitcoin satoshis) required to make the genesis transaction valid.
    // Pass the number of bitcoin satoshis in each output in outputsSatoshis (array size countOutputs).
    // Use RavenScrollScriptIsRegular() to pass an array of bools in outputsRegular for whether each output script is regular.
    
    
// Functions for managing asset references
    
void RavenScrollAssetRefClear(RavenScrollAssetRef *assetRef);
    // Set all fields in assetRef to their default/zero values, which are not necessarily valid.
    
bool RavenScrollAssetRefToString(const RavenScrollAssetRef *assetRef, char* string, const size_t stringMaxLen);
    // Outputs the assetRef to a null-delimited string (size stringMaxLen) for debugging.
    // Returns true if there was enough space (1024 bytes recommended) or false otherwise.

bool RavenScrollAssetRefIsValid(const RavenScrollAssetRef *assetRef);
    // Returns true if all values in the asset reference are in their permitted ranges, false otherwise.    
    
bool RavenScrollAssetRefMatch(const RavenScrollAssetRef* assetRef1, const RavenScrollAssetRef* assetRef2);
    // Returns true if the two RavenScrollAssetRef structures are identical.

size_t RavenScrollAssetRefEncode(const RavenScrollAssetRef* assetRef, char* string, const size_t stringMaxLen);
    // Encodes the assetRef to a null-delimited RavenScroll asset reference string (length stringMaxLen).
    // Returns the size of the formatted string if successful, otherwise 0.
    
bool RavenScrollAssetRefDecode(RavenScrollAssetRef *assetRef, const char* string, const size_t stringLen);
    // Decodes the RavenScroll asset reference string (length stringLen) into assetRef.
    // Returns true if the asset reference could be successfully read, otherwise false.
    

    
// Functions for managing asset transfer metadata and arrays thereof
    
void RavenScrollTransferClear(RavenScrollTransfer* transfer);
    // Set all fields in transfer to their default/zero values, which are not necessarily valid.
    
bool RavenScrollTransferToString(const RavenScrollTransfer* transfer, char* string, const size_t stringMaxLen);
    // Outputs the transfer to a null-delimited string (size stringMaxLen) for debugging.
    // Returns true if there was enough space (1024 bytes recommended) or false otherwise.
    
bool RavenScrollTransfersToString(const RavenScrollTransfer* transfers, const int countTransfers, char* string, const size_t stringMaxLen);
    // Outputs the array of transfers to a null-delimited string (size stringMaxLen) for debugging.
    // Returns true if there was enough space (1024 * countTransfers bytes recommended) or false otherwise.
    
bool RavenScrollTransferMatch(const RavenScrollTransfer* transfer1, const RavenScrollTransfer* transfer2);
    // Returns true if the two RavenScrollTransfer structures are identical.
    
bool RavenScrollTransfersMatch(const RavenScrollTransfer* transfers1, const RavenScrollTransfer* transfers2, int countTransfers, bool strict);
    // Returns true if the two arrays of transfers in transfers1 and transfers2 are the same.
    // If strict is true then the ordering in the two arrays must be identical. If strict is false
    // then it is enough if each list is equivalent, i.e. the same transfers in the same order for each asset reference.

bool RavenScrollTransferIsValid(const RavenScrollTransfer* transfer);
    // Returns true is all values in the transfer are in their permitted ranges, false otherwise.
    
bool RavenScrollTransfersAreValid(const RavenScrollTransfer* transfers, const int countTransfers);
    // Returns true if all values in the array of transfers (size countTransfers) are valid.
    

// Functions for encoding, decoding and calculating minimum fees for RavenScroll transfer transactions

size_t RavenScrollTransfersEncode(const RavenScrollTransfer* transfers, const int countTransfers,
                                const int countInputs, const int countOutputs,
                                char* metadata, const size_t metadataMaxLen);
    // Encodes the array of transfers (length countTransfers) into metadata (whose size is metadataMaxLen).
    // Pass the number of transaction inputs and outputs in countInputs and countOutputs respectively.
    // If the encoding was successful, returns the number of bytes used, otherwise 0.
    
int RavenScrollTransfersDecodeCount(const char* metadata, const size_t metadataLen);
    // Returns the number of transfers encoded in metadata (length metadataLen).

int RavenScrollTransfersDecode(RavenScrollTransfer* transfers, const int maxTransfers,
                             const int countInputs, const int countOutputs,
                             const char* metadata, const size_t metadataLen);
    // Decodes up to maxTransfers transfers from metadata (length metadataLen) into the transfers array.
    // Pass the number of transaction inputs and outputs in countInputs and countOutputs respectively.
    // Returns total number of transfers encoded in metadata like RavenScrollTransfersDecodeCount() or 0 if an error.
    // If this return value is more than maxTransfers, try calling this again with a larger array.

RavenScrollSatoshiQty RavenScrollTransfersCalcMinFee(const RavenScrollTransfer* transfers, const int countTransfers,
                                                 const int countInputs, const int countOutputs,
                                                 const RavenScrollSatoshiQty* outputsSatoshis, const bool* outputsRegular);
    // Returns the minimum transaction fee (in bitcoin satoshis) required to make the set of transfers (array size countTransfers) valid.
    // Pass the number of transaction inputs and outputs in countInputs and countOutputs respectively.
    // Pass the number of bitcoin satoshis in each output in outputsSatoshis (array size countOutputs).
    // Use RavenScrollScriptIsRegular() to pass an array of bools in outputsRegular for whether each output script is regular.

    
// Functions for calculating asset quantities and change outputs
    
void RavenScrollGenesisApply(const RavenScrollGenesis* genesis, const bool* outputsRegular,
                           RavenScrollAssetQty* outputBalances, const int countOutputs);
    // For the asset specified by genesis, calculate the number of newly created asset units in each
    // output of the genesis transaction into the outputBalances array (size countOutputs).
    // Use RavenScrollScriptIsRegular() to pass an array of bools in outputsRegular for whether each output script is regular.
    // ** This is only relevant if the transaction DOES HAVE a sufficient fee to make the genesis valid **
    
void RavenScrollTransfersApply(const RavenScrollAssetRef* assetRef, const RavenScrollGenesis* genesis,
                             const RavenScrollTransfer* transfers, const int countTransfers,
                             const RavenScrollAssetQty* inputBalances, const int countInputs,
                             const bool* outputsRegular, RavenScrollAssetQty* outputBalances, const int countOutputs);
    // For the asset specified by assetRef and genesis, and list of transfers (size countTransfers), applies those transfers
    // to move units of that asset from inputBalances (size countInputs) to outputBalances (size countOutputs).
    // Only transfers whose assetRef matches the function's assetRef parameter will be applied (apart from default routes).
    // Use RavenScrollScriptIsRegular() to pass an array of bools in outputsRegular for whether each output script is regular.
    // ** Call this if the transaction DOES HAVE a sufficient fee to make the list of transfers valid **
    
void RavenScrollTransfersApplyNone(const RavenScrollAssetRef* assetRef, const RavenScrollGenesis* genesis,
                                 const RavenScrollAssetQty* inputBalances, const int countInputs,
                                 const bool* outputsRegular, RavenScrollAssetQty* outputBalances, const int countOutputs);
    // For the asset specified by assetRef and genesis, move units of that asset from inputBalances (size countInputs) to
    // outputBalances (size countOutputs), applying the default behavior only (all assets goes to last regular output).
    // This is equivalent to calling RavenScrollTransfersApply() with countTransfers=0.
    // ** Call this if the transaction DOES NOT HAVE a sufficient fee to make the list of transfers valid **

void RavenScrollTransfersDefaultOutputs(const RavenScrollTransfer* transfers, const int countTransfers, const int countInputs,
                                      const bool* outputsRegular, bool* outputsDefault, const int countOutputs);
    // For the list of transfers (size countTransfers) on a transaction with countInputs inputs, calculate
    // the array of bools in outputsDefault (size countOutputs) where each entry indicates whether that
    // output might receive some assets due to default routes. 
    
    
// Functions for managing payment references
    
bool RavenScrollPaymentRefToString(const RavenScrollPaymentRef paymentRef, char* string, const size_t stringMaxLen);
    // Outputs the paymentRef to a null-delimited string (size stringMaxLen) for debugging.
    // Returns true if there was enough space (1024 bytes recommended) or false otherwise.

bool RavenScrollPaymentRefIsValid(const RavenScrollPaymentRef paymentRef);
    // Returns true if paymentRef is in the permitted range, false otherwise.

RavenScrollPaymentRef RavenScrollPaymentRefRandom();
    // Returns a random payment reference that can be used for a RavenScroll address and embedded in a transaction
    
size_t RavenScrollPaymentRefEncode(const RavenScrollPaymentRef paymentRef, char* metadata, const size_t metadataMaxLen);
    // Encodes the paymentRef into metadata (whose size is metadataMaxLen);
    // If the encoding was successful, returns the number of bytes used, otherwise 0.
    
bool RavenScrollPaymentRefDecode(RavenScrollPaymentRef* paymentRef, const char* metadata, const size_t metadataLen);
    // Decodes the payment reference in metadata (length metadataLen) into paymentRef.
    // Return true if the decode was successful, false otherwise.
    
    
// Functions for managing messages

void RavenScrollMessageClear(RavenScrollMessage* message);
    // Set all fields in message to their default/zero values, which are not necessarily valid.
    
bool RavenScrollMessageToString(const RavenScrollMessage* message, char* string, const size_t stringMaxLen);
    // Outputs the message to a null-delimited string (size stringMaxLen) for debugging.
    // Returns true if there was enough space (4096 bytes recommended) or false otherwise.

bool RavenScrollMessageIsValid(const RavenScrollMessage* message);
    // Returns true if message is valid, false otherwise.

bool RavenScrollMessageMatch(const RavenScrollMessage* message1, const RavenScrollMessage* message2, const bool strict);
    // Returns true if the two RavenScrollMessage structures are the same. If strict is true then the outputRange
    // fields must be identical. If strict is false then they must only represent the same set of outputs.

size_t RavenScrollMessageEncode(const RavenScrollMessage* message, const int countOutputs, char* metadata, const size_t metadataMaxLen);
    // Encodes the message into metadata (whose size is metadataMaxLen)
    // Pass the number of transaction outputs in countOutputs respectively.
    // If the encoding was successful, returns the number of bytes used, otherwise 0.

bool RavenScrollMessageDecode(RavenScrollMessage* message, const int countOutputs, const char* metadata, const size_t metadataLen);
    // Decodes the message in metadata (length metadataLen) into the message variable.
    // Pass the number of transaction outputs in countOutputs respectively.
    // Returns true if the decode was successful, false otherwise.
    
bool RavenScrollMessageHasOutput(const RavenScrollMessage* message, int outputIndex);
    // Returns true if the message is intended for the given outputIndex.

size_t RavenScrollMessageCalcHashLen(const RavenScrollMessage *message, int countOutputs, const size_t metadataMaxLen);
    // Calculates the appropriate hash length for message so that when encoded as metadata the message will
    // fit in metadataMaxLen bytes. For now, set metadataMaxLen to 40 (see Bitcoin's MAX_OP_RETURN_RELAY parameter).

    
// Functions for calculating URLs and hashes
    
size_t RavenScrollGenesisCalcAssetURL(const RavenScrollGenesis *genesis, const char* firstSpentTxID, const int firstSpentVout,
                                    char* urlString, const size_t urlStringMaxLen);
    // Calculates the URL for the asset web page of genesis into urlString (length urlStringMaxLen).
    // In firstSpentTxID, pass the previous txid whose output was spent by the first input of the genesis.
    // In firstSpentVout, pass the output index of firstSpentTxID spent by the first input of the genesis.
    // Returns the length of the URL, or 0 if urlStringMaxLen (recommended 256 bytes) was not large enough.
    
size_t RavenScrollMessageCalcServerURL(const RavenScrollMessage* message, char* urlString, const size_t urlStringMaxLen);
    // Calculates the URL for the message server into urlString (length urlStringMaxLen).
    // Returns the length of the URL, or 0 if urlStringMaxLen (recommended 256 bytes) was not large enough.
    
void RavenScrollCalcAssetHash(const char* name, size_t nameLen,
                            const char* issuer, size_t issuerLen,
                            const char* description, size_t descriptionLen,
                            const char* units, size_t unitsLen,
                            const char* issueDate, size_t issueDateLen,
                            const char* expiryDate, size_t expiryDateLen,
                            const double* interestRate, const double* multiple,
                            const char* contractContent, const size_t contractContentLen,
                            unsigned char assetHash[32]);
    // Calculates the assetHash for the key information from a RavenScroll asset web page JSON specification.
    // All char* string parameters except contractContent must be passed using UTF-8 encoding.
    // You may pass NULL (and if appropriate, a length of zero) for any parameter which was not in the JSON.
    // Note that you need to pass in the contract *content* and length, not its URL.
    
typedef struct {
    char* mimeType;
    size_t mimeTypeLen;
    char* fileName; // can be NULL
    size_t fileNameLen;
    unsigned char* content;
    size_t contentLen;
} RavenScrollMessagePart;

void RavenScrollCalcMessageHash(const unsigned char* salt, size_t saltLen, const RavenScrollMessagePart* messageParts,
                              const int countParts, unsigned char messageHash[32]);
    // Calculates the messageHash for a RavenScroll message containing the given messageParts array (length countParts).
    // Pass in a random string in salt (length saltLen), that should be sent to the message server along with the content.

void RavenScrollCalcSHA256Hash(const unsigned char* input, const size_t inputLen, unsigned char hash[32]);
    // Calculates the SHA-256 hash of the raw data in input (size inputLen) and places it in the hash variable.

    
// Functions whose purpose is to lock down the legal definitions in RavenScroll contracts

char* RavenScrollGetGenesisWebPageURL(const char* scriptPubKeys[], const size_t scriptPubKeysLen[], const int countOutputs,
                                    u_int8_t firstSpentTxId[32], const int firstSpentVout);
    
RavenScrollAssetQty RavenScrollGetGenesisOutputQty(const char* scriptPubKeys[], const size_t scriptPubKeysLen[],
                                               const RavenScrollSatoshiQty* outputsSatoshis, const int countOutputs,
                                               const RavenScrollSatoshiQty transactionFee, const int getOutputIndex);

RavenScrollAssetQty RavenScrollGetTransferOutputQty(const char* genesisScriptPubKeys[], const size_t genesisScriptPubKeysLen[],
                                                const RavenScrollSatoshiQty* genesisOutputsSatoshis, const int genesisCountOutputs,
                                                const RavenScrollSatoshiQty genesisTransactionFee,
                                                int64_t genesisBlockNum, int64_t genesisTxOffset, u_int8_t genesisTxId[32],
                                                const RavenScrollAssetQty* thisInputBalances, const int thisCountInputs,
                                                const char* thisScriptPubKeys[], const size_t thisScriptPubKeysLen[],
                                                const RavenScrollSatoshiQty* thisOutputsSatoshis, const int thisCountOutputs,
                                                const RavenScrollSatoshiQty thisTransactionFee, const int getOutputIndex);
    

#ifdef __cplusplus
}
#endif

#endif // _RAVENSCROLL_H_
