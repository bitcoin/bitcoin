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


#include "ravenscroll.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

// Macros used internally

#define RAVENSCROLL_UNSIGNED_BYTE_MAX 0xFF
#define RAVENSCROLL_UNSIGNED_2_BYTES_MAX 0xFFFF
#define RAVENSCROLL_UNSIGNED_3_BYTES_MAX 0xFFFFFF
#define RAVENSCROLL_UNSIGNED_4_BYTES_MAX 0xFFFFFFFF

#define RAVENSCROLL_METADATA_IDENTIFIER "SPK"
#define RAVENSCROLL_METADATA_IDENTIFIER_LEN 3
#define RAVENSCROLL_LENGTH_PREFIX_MAX 96
#define RAVENSCROLL_GENESIS_PREFIX 'g'
#define RAVENSCROLL_TRANSFERS_PREFIX 't'
#define RAVENSCROLL_PAYMENTREF_PREFIX 'r'
#define RAVENSCROLL_MESSAGE_PREFIX 'm'

#define RAVENSCROLL_FEE_BASIS_MAX_SATOSHIS 1000

#define RAVENSCROLL_GENESIS_QTY_FLAGS_LENGTH 2
#define RAVENSCROLL_GENESIS_QTY_MASK 0x3FFF
#define RAVENSCROLL_GENESIS_QTY_EXPONENT_MULTIPLE 1001
#define RAVENSCROLL_GENESIS_FLAG_CHARGE_FLAT 0x4000
#define RAVENSCROLL_GENESIS_FLAG_CHARGE_BPS 0x8000
#define RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MULTIPLE 101
#define RAVENSCROLL_GENESIS_CHARGE_FLAT_LENGTH 1
#define RAVENSCROLL_GENESIS_CHARGE_BPS_LENGTH 1

#define RAVENSCROLL_DOMAIN_PACKING_PREFIX_MASK 0xC0
#define RAVENSCROLL_DOMAIN_PACKING_PREFIX_SHIFT 6
#define RAVENSCROLL_DOMAIN_PACKING_SUFFIX_MASK 0x3F
#define RAVENSCROLL_DOMAIN_PACKING_SUFFIX_MAX 61
#define RAVENSCROLL_DOMAIN_PACKING_SUFFIX_IPv4_NO_PATH 62 // messages only
#define RAVENSCROLL_DOMAIN_PACKING_SUFFIX_IPv4 63
#define RAVENSCROLL_DOMAIN_PACKING_IPv4_HTTPS 0x40
#define RAVENSCROLL_DOMAIN_PACKING_IPv4_NO_PATH_PREFIX 0x80

#define RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE 40
#define RAVENSCROLL_DOMAIN_PATH_FALSE_END_CHAR '<'
#define RAVENSCROLL_DOMAIN_PATH_TRUE_END_CHAR '>'

#define RAVENSCROLL_PACKING_GENESIS_MASK 0xC0
#define RAVENSCROLL_PACKING_GENESIS_PREV 0x00
#define RAVENSCROLL_PACKING_GENESIS_3_3_BYTES 0x40 // 3 bytes for block index, 3 for txn offset
#define RAVENSCROLL_PACKING_GENESIS_3_4_BYTES 0x80 // 3 bytes for block index, 4 for txn offset
#define RAVENSCROLL_PACKING_GENESIS_4_4_BYTES 0xC0 // 4 bytes for block index, 4 for txn offset

#define RAVENSCROLL_PACKING_INDICES_MASK 0x38
#define RAVENSCROLL_PACKING_INDICES_0P_0P 0x00 // input 0 only or previous, output 0 only or previous
#define RAVENSCROLL_PACKING_INDICES_0P_1S 0x08 // input 0 only or previous, output 1 only or subsequent single
#define RAVENSCROLL_PACKING_INDICES_0P_ALL 0x10 // input 0 only or previous, all outputs
#define RAVENSCROLL_PACKING_INDICES_1S_0P 0x18 // input 1 only or subsequent single, output 0 only or previous
#define RAVENSCROLL_PACKING_INDICES_ALL_0P 0x20 // all inputs, output 0 only or previous
#define RAVENSCROLL_PACKING_INDICES_ALL_1S 0x28 // all inputs, output 1 only or subsequent single
#define RAVENSCROLL_PACKING_INDICES_ALL_ALL 0x30 // all inputs, all outputs
#define RAVENSCROLL_PACKING_INDICES_EXTEND 0x38 // use second byte for more extensive information

#define RAVENSCROLL_PACKING_EXTEND_INPUTS_SHIFT 3
#define RAVENSCROLL_PACKING_EXTEND_OUTPUTS_SHIFT 0

#define RAVENSCROLL_PACKING_EXTEND_MASK 0x07
#define RAVENSCROLL_PACKING_EXTEND_0P 0x00 // index 0 or previous (transfers only)
#define RAVENSCROLL_PACKING_EXTEND_PUBLIC 0x00 // this is public (messages only)
#define RAVENSCROLL_PACKING_EXTEND_1S 0x01 // index 1 or subsequent to previous (transfers only)
#define RAVENSCROLL_PACKING_EXTEND_0_1_BYTE 0x01 // starting at 0, 1 byte for count (messages only)
#define RAVENSCROLL_PACKING_EXTEND_1_0_BYTE 0x02 // 1 byte for single index, count is 1
#define RAVENSCROLL_PACKING_EXTEND_2_0_BYTES 0x03 // 2 bytes for single index, count is 1
#define RAVENSCROLL_PACKING_EXTEND_1_1_BYTES 0x04 // 1 byte for first index, 1 byte for count
#define RAVENSCROLL_PACKING_EXTEND_2_1_BYTES 0x05 // 2 bytes for first index, 1 byte for count
#define RAVENSCROLL_PACKING_EXTEND_2_2_BYTES 0x06 // 2 bytes for first index, 2 bytes for count
#define RAVENSCROLL_PACKING_EXTEND_ALL 0x07 // all inputs|outputs

#define RAVENSCROLL_PACKING_QUANTITY_MASK 0x07
#define RAVENSCROLL_PACKING_QUANTITY_1P 0x00 // quantity=1 or previous
#define RAVENSCROLL_PACKING_QUANTITY_1_BYTE 0x01
#define RAVENSCROLL_PACKING_QUANTITY_2_BYTES 0x02
#define RAVENSCROLL_PACKING_QUANTITY_3_BYTES 0x03
#define RAVENSCROLL_PACKING_QUANTITY_4_BYTES 0x04
#define RAVENSCROLL_PACKING_QUANTITY_6_BYTES 0x05
#define RAVENSCROLL_PACKING_QUANTITY_FLOAT 0x06
#define RAVENSCROLL_PACKING_QUANTITY_MAX 0x07 // transfer all quantity across

#define RAVENSCROLL_TRANSFER_QTY_FLOAT_LENGTH 2
#define RAVENSCROLL_TRANSFER_QTY_FLOAT_MANTISSA_MAX 1000
#define RAVENSCROLL_TRANSFER_QTY_FLOAT_EXPONENT_MAX 11
#define RAVENSCROLL_TRANSFER_QTY_FLOAT_MASK 0x3FFF
#define RAVENSCROLL_TRANSFER_QTY_FLOAT_EXPONENT_MULTIPLE 1001

#define RAVENSCROLL_OUTPUTS_MORE_FLAG 0x80
#define RAVENSCROLL_OUTPUTS_RESERVED_MASK 0x60
#define RAVENSCROLL_OUTPUTS_TYPE_MASK 0x18
#define RAVENSCROLL_OUTPUTS_TYPE_SINGLE 0x00 // one output index (0...7)
#define RAVENSCROLL_OUTPUTS_TYPE_FIRST 0x08 // first (0...7) outputs
#define RAVENSCROLL_OUTPUTS_TYPE_UNUSED 0x10 // for future use
#define RAVENSCROLL_OUTPUTS_TYPE_EXTEND 0x18 // "extend", including public/all
#define RAVENSCROLL_OUTPUTS_VALUE_MASK 0x07
#define RAVENSCROLL_OUTPUTS_VALUE_MAX 7

#define RAVENSCROLL_ADDRESS_PREFIX 's'
#define RAVENSCROLL_ADDRESS_FLAG_CHARS_MULTIPLE 10
#define RAVENSCROLL_ADDRESS_CHAR_INCREMENT 13

// Type definitions and constants used internally

typedef enum { // options to use in order of priority
    firstPackingType=0,
    _0P=0,
    _1S,
    _ALL,
    _1_0_BYTE,
    _0_1_BYTE,
    _2_0_BYTES,
    _1_1_BYTES,
    _2_1_BYTES,
    _2_2_BYTES,
    countPackingTypes
} PackingType;

static const char packingExtendMap[]={ // same order as above
    RAVENSCROLL_PACKING_EXTEND_0P,
    RAVENSCROLL_PACKING_EXTEND_1S,
    RAVENSCROLL_PACKING_EXTEND_ALL,
    RAVENSCROLL_PACKING_EXTEND_1_0_BYTE,
    RAVENSCROLL_PACKING_EXTEND_0_1_BYTE,
    RAVENSCROLL_PACKING_EXTEND_2_0_BYTES,
    RAVENSCROLL_PACKING_EXTEND_1_1_BYTES,
    RAVENSCROLL_PACKING_EXTEND_2_1_BYTES,
    RAVENSCROLL_PACKING_EXTEND_2_2_BYTES
};

static const char domainPathChars[RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE+1]="0123456789abcdefghijklmnopqrstuvwxyz-.<>";
    // last two characters are end markers, < means false, > means true

static const char integerToBase58[58]="123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static const int base58Minus49ToInteger[74]={
     0,  1,  2,  3,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1,
     9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1, 22,
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1, -1,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57
};

static const char *domainNamePrefixes[]={
    "",
    "www."
};

static const char *domainNameSuffixes[60]={ // leave space for 3 more in future
    "",
    
    // most common suffixes based on Alexa's top 1M sites as of 10 June 2014, with some manual adjustments

    ".at",
    ".au",
    ".be",
    ".biz",
    ".br",
    ".ca",
    ".ch",
    ".cn",
    ".co.jp",
    ".co.kr",
    ".co.uk",
    ".co.za",
    ".co",
    ".com.ar",
    ".com.au",
    ".com.br",
    ".com.cn",
    ".com.mx",
    ".com.tr",
    ".com.tw",
    ".com.ua",
    ".com",
    ".cz",
    ".de",
    ".dk",
    ".edu",
    ".es",
    ".eu",
    ".fr",
    ".gov",
    ".gr",
    ".hk",
    ".hu",
    ".il",
    ".in",
    ".info",
    ".ir",
    ".it",
    ".jp",
    ".kr",
    ".me",
    ".mx",
    ".net",
    ".nl",
    ".no",
    ".org",
    ".pl",
    ".ps",
    ".ro",
    ".ru",
    ".se",
    ".sg",
    ".tr",
    ".tv",
    ".tw",
    ".ua",
    ".uk",
    ".us",
    ".vn"
};

typedef bool PackingOptions[countPackingTypes];

typedef struct {
    size_t  blockNumBytes, txOffsetBytes, txIDPrefixBytes, firstInputBytes,
            countInputsBytes, firstOutputBytes, countOutputsBytes, quantityBytes;
} PackingByteCounts;

// Functions used internally

static bool WriteSmallEndianUnsigned(const long long value, char* dataBuffer, size_t bytes)
{
    size_t byte;
    long long valueLeft;
    
    if (value<0)
        return FALSE; // does not support negative values
    
    valueLeft=value;
    
    for (byte=bytes; byte>0; byte--) {
        *dataBuffer++=(char)(valueLeft&0xFF);
        valueLeft/=256;
    }
    
    return !valueLeft; // if still something left, we didn't have enough bytes for representation
}

static long long ReadSmallEndianUnsigned(const char* dataBuffer, size_t bytes)
{
    long long value;
    const char *dataBufferPtr;
    
    value=0;
    
    for (dataBufferPtr=dataBuffer+bytes-1; dataBufferPtr>=dataBuffer; dataBufferPtr--) {
        value*=256;
        value+=*(unsigned char*)dataBufferPtr;
    }
    
    return value&0x7FFFFFFFFFFFFFFF;
}

static void UInt8ToHexCharPair(u_int8_t value, char* hexChars)
{
    const char hexCharMap[]="0123456789ABCDEF";
    
    hexChars[0]=hexCharMap[value>>4];
    hexChars[1]=hexCharMap[value&15];
}

static char* UnsignedToSmallEndianHex(long long value, size_t bytes, char* string) // string must be at least 2*bytes+1 long
{
    size_t byte;
    
    for (byte=0; byte<bytes; byte++) {
        UInt8ToHexCharPair(value&0xFF, string+2*byte);
        value/=256;
    }
    
    string[2*bytes]=0x00; // null terminator
    
    return string;
}

static char* BinaryToHex(const void* binary, const size_t bytes, char* hexString)
{
    size_t byte;
    
    for (byte=0; byte<bytes; byte++)
        UInt8ToHexCharPair(((u_int8_t*)binary)[byte], hexString+2*byte);
    
    hexString[2*bytes]=0x00; // null terminator

    return hexString;
}

static bool HexCharToUInt8(const char hexChar, u_int8_t* value)
{
    if ((hexChar>='0') && (hexChar<='9'))
        *value=hexChar-'0';
    else if ((hexChar>='a') && (hexChar<='f'))
        *value=hexChar-'a'+10;
    else if ((hexChar>='A') && (hexChar<='F'))
        *value=hexChar-'A'+10;
    else
        return FALSE;
    
    return TRUE;
}

static bool HexToBinary(const char* hexString, void* binary, const size_t bytes)
{
    size_t byte;
    u_int8_t valueHigh, valueLow;
    
    for (byte=0; byte<bytes; byte++)
        if (HexCharToUInt8(hexString[2*byte], &valueHigh) && HexCharToUInt8(hexString[2*byte+1], &valueLow))
            ((u_int8_t*)binary)[byte]=(valueHigh<<4)|valueLow;
        else
            return FALSE;
    
    return TRUE;
}

static void sha256(unsigned char hval[], const unsigned char data[], unsigned long len);

void RavenScrollCalcSHA256Hash(const unsigned char* input, const size_t inputLen, unsigned char hash[32])
{
    sha256(hash, input, inputLen);
}

size_t AllocRawScript(const char* scriptPubKey, const size_t scriptPubKeyLen, const bool scriptIsHex, const char** _scriptPubKeyRaw)
{
    size_t scriptPubKeyRawLen;
    char* scriptPubKeyRaw;
    
    if (scriptIsHex) {
        if ((scriptPubKeyLen%2)==0) {
            scriptPubKeyRawLen=scriptPubKeyLen/2;
            scriptPubKeyRaw=malloc(scriptPubKeyRawLen);
            
            if (HexToBinary(scriptPubKey, scriptPubKeyRaw, scriptPubKeyRawLen)) {
                *_scriptPubKeyRaw=scriptPubKeyRaw;
                return scriptPubKeyRawLen;
                
            } else {
                free(scriptPubKeyRaw);
                *_scriptPubKeyRaw=NULL;
                *scriptPubKeyRaw=0;
            }
        }
        
    } else {
        *_scriptPubKeyRaw=scriptPubKey;
        return scriptPubKeyLen;
    }
    
    return 0;
}

void FreeRawScript(const bool scriptIsHex, const char* scriptPubKeyRaw)
{
    if (scriptIsHex && scriptPubKeyRaw)
        free((void*)scriptPubKeyRaw);
}

static int Base58ToInteger(const char base58Character) // returns -1 if invalid
{
    if ( (base58Character<49) || (base58Character>122) )
        return -1;
    
    return base58Minus49ToInteger[base58Character-49];
}

static long long MantissaExponentToQty(int mantissa, int exponent)
{
    long long quantity;
    
    quantity=mantissa;
    
    for (; exponent>0; exponent--)
        quantity*=10;
    
    return quantity;
}

static long long QtyToMantissaExponent(long long quantity, int rounding, int mantissaMax, int exponentMax, int* mantissa, int* exponent)
{
    long long roundOffset;
    
    if (rounding<0)
        roundOffset=0;
    else if (rounding>0)
        roundOffset=9;
    else
        roundOffset=4;
    
    *exponent=0;
    
    while (quantity>mantissaMax) {
        quantity=(quantity+roundOffset)/10;
        (*exponent)++;
    }
    
    *mantissa=(int)quantity;
    *exponent=RAVENSCROLL_MIN(*exponent, exponentMax);
    
    return MantissaExponentToQty(*mantissa, *exponent);
}

RavenScrollSatoshiQty GetMinFeeBasis(const RavenScrollSatoshiQty* outputsSatoshis, const bool* outputsRegular, const int countOutputs)
{
    RavenScrollSatoshiQty smallestOutputSatoshis;
    int outputIndex;
    
    smallestOutputSatoshis=RAVENSCROLL_SATOSHI_QTY_MAX;
    
    for (outputIndex=0; outputIndex<countOutputs; outputIndex++)
        if (outputsRegular[outputIndex])
            smallestOutputSatoshis=RAVENSCROLL_MIN(smallestOutputSatoshis, outputsSatoshis[outputIndex]);
    
    return RAVENSCROLL_MIN(RAVENSCROLL_FEE_BASIS_MAX_SATOSHIS, smallestOutputSatoshis);
}

static int GetLastRegularOutput(const bool* outputsRegular, const int countOutputs)
{
    int outputIndex;
    
    for (outputIndex=countOutputs-1; outputIndex>=0; outputIndex--)
        if (outputsRegular[outputIndex])
            return outputIndex;
    
    return countOutputs; // indicates no regular ones were found
}

static int CountNonLastRegularOutputs(const bool* outputsRegular, const int countOutputs)
{
    int countRegularOutputs, outputIndex;
    
    countRegularOutputs=0;
    
    for (outputIndex=0; outputIndex<countOutputs; outputIndex++)
        if (outputsRegular[outputIndex])
            countRegularOutputs++;
    
    return RAVENSCROLL_MAX(countRegularOutputs-1, 0);
}

static void GetDefaultRouteMap(const RavenScrollTransfer* transfers, const int countTransfers,
                               const int countInputs, const int countOutputs, const bool* outputsRegular, int* inputDefaultOutput)
{
    int lastRegularOutput, transferIndex, inputIndex, lastInputIndex, outputIndex;
    
//  Default to last output for all inputs
    
    lastRegularOutput=GetLastRegularOutput(outputsRegular, countOutputs); // can be countOutputs if no regular ones found
    for (inputIndex=0; inputIndex<countInputs; inputIndex++)
        inputDefaultOutput[inputIndex]=lastRegularOutput;

//  Apply any default route transfers in reverse order (since early ones take precedence)
    
    for (transferIndex=countTransfers-1; transferIndex>=0; transferIndex--)
        if (transfers[transferIndex].assetRef.blockNum==RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE) {
            outputIndex=transfers[transferIndex].outputs.first; // outputs.count is not relevant
            
            if ( (outputIndex>=0) && (outputIndex<countOutputs) ) {
                inputIndex=RAVENSCROLL_MAX(transfers[transferIndex].inputs.first, 0);
                lastInputIndex=RAVENSCROLL_MIN(inputIndex+transfers[transferIndex].inputs.count, countInputs)-1;
                
                for (; inputIndex<=lastInputIndex; inputIndex++)
                    inputDefaultOutput[inputIndex]=outputIndex;
            }
        }
}

static void GetPackingOptions(const RavenScrollIORange *previousRange, const RavenScrollIORange *range, const int countInputOutputs, PackingOptions packingOptions, bool forMessages)
{
    bool firstZero, firstByte, first2Bytes, countOne, countByte;
    
    firstZero=(range->first==0);
    firstByte=(range->first<=RAVENSCROLL_UNSIGNED_BYTE_MAX);
    first2Bytes=(range->first<=RAVENSCROLL_UNSIGNED_2_BYTES_MAX);
    countOne=(range->count==1);
    countByte=(range->count<=RAVENSCROLL_UNSIGNED_BYTE_MAX);
    
    if (forMessages) {
        packingOptions[_0P]=FALSE;
        packingOptions[_1S]=FALSE; // these two options not used for messages
        packingOptions[_0_1_BYTE]=firstZero && countByte;
        
    } else {
        if (previousRange) {
            packingOptions[_0P]=(range->first==previousRange->first) && (range->count==previousRange->count);
            packingOptions[_1S]=(range->first==(previousRange->first+previousRange->count)) && countOne;
            
        } else {
            packingOptions[_0P]=firstZero && countOne;
            packingOptions[_1S]=(range->first==1) && countOne;
        }
        
        packingOptions[_0_1_BYTE]=FALSE; // this option not used for transfers
    }
    
    packingOptions[_1_0_BYTE]=firstByte && countOne;
    packingOptions[_2_0_BYTES]=first2Bytes && countOne;
    packingOptions[_1_1_BYTES]=firstByte && countByte;
    packingOptions[_2_1_BYTES]=first2Bytes && countByte;
    packingOptions[_2_2_BYTES]=first2Bytes && (range->count<=RAVENSCROLL_UNSIGNED_2_BYTES_MAX);
    packingOptions[_ALL]=firstZero && (range->count>=countInputOutputs);
}

static void PackingTypeToValues(const PackingType packingType, const RavenScrollIORange *previousRange, const int countInputOutputs, RavenScrollIORange *range)
{
    switch (packingType)
    {
        case _0P:
            if (previousRange) {
                range->first=previousRange->first;
                range->count=previousRange->count;
            } else {
                range->first=0;
                range->count=1;
            }
            break;
            
        case _1S:
            if (previousRange)
                range->first=previousRange->first+previousRange->count;
            else
                range->first=1;
            
            range->count=1;
            break;
            
        case _0_1_BYTE:
            range->first=0;
            break;
            
        case _1_0_BYTE:
        case _2_0_BYTES:
            range->count=1;
            break;
            
        case _ALL:
            range->first=0;
            range->count=countInputOutputs;
            break;
            
        default: // to prevent compiler warning
            break;
    }
}

static void PackingExtendAddByteCounts(char packingExtend, size_t* firstBytes, size_t* countBytes, bool forMessages)
{
    switch (packingExtend) {
        case RAVENSCROLL_PACKING_EXTEND_0_1_BYTE:
            if (forMessages) // otherwise it's really RAVENSCROLL_PACKING_EXTEND_1S
                *countBytes=1;
            break;
            
        case RAVENSCROLL_PACKING_EXTEND_1_0_BYTE:
            *firstBytes=1;
            break;
            
        case RAVENSCROLL_PACKING_EXTEND_2_0_BYTES:
            *firstBytes=2;
            break;
            
        case RAVENSCROLL_PACKING_EXTEND_1_1_BYTES:
            *firstBytes=1;
            *countBytes=1;
            break;
            
        case RAVENSCROLL_PACKING_EXTEND_2_1_BYTES:
            *firstBytes=2;
            *countBytes=1;
            break;
            
        case RAVENSCROLL_PACKING_EXTEND_2_2_BYTES:
            *firstBytes=2;
            *countBytes=2;
            break;
    }
}

static void TransferPackingToByteCounts(char packing, char packingExtend, PackingByteCounts* counts)
{
    
//  Set default values for bytes for all fields to zero
    
    counts->blockNumBytes=0;
    counts->txOffsetBytes=0;
    counts->txIDPrefixBytes=0;
    
    counts->firstInputBytes=0;
    counts->countInputsBytes=0;
    counts->firstOutputBytes=0;
    counts->countOutputsBytes=0;
    
    counts->quantityBytes=0;
    
//  Packing for genesis reference
    
    switch (packing & RAVENSCROLL_PACKING_GENESIS_MASK)
    {
        case RAVENSCROLL_PACKING_GENESIS_3_3_BYTES:
            counts->blockNumBytes=3;
            counts->txOffsetBytes=3;
            counts->txIDPrefixBytes=RAVENSCROLL_ASSETREF_TXID_PREFIX_LEN;
            break;
            
        case RAVENSCROLL_PACKING_GENESIS_3_4_BYTES:
            counts->blockNumBytes=3;
            counts->txOffsetBytes=4;
            counts->txIDPrefixBytes=RAVENSCROLL_ASSETREF_TXID_PREFIX_LEN;
            break;
            
        case RAVENSCROLL_PACKING_GENESIS_4_4_BYTES:
            counts->blockNumBytes=4;
            counts->txOffsetBytes=4;
            counts->txIDPrefixBytes=RAVENSCROLL_ASSETREF_TXID_PREFIX_LEN;
            break;
    }
    
//  Packing for input and output indices (relevant for extended indices only)
    
    if ((packing & RAVENSCROLL_PACKING_INDICES_MASK) == RAVENSCROLL_PACKING_INDICES_EXTEND) {
        PackingExtendAddByteCounts((packingExtend >> RAVENSCROLL_PACKING_EXTEND_INPUTS_SHIFT) & RAVENSCROLL_PACKING_EXTEND_MASK,
            &counts->firstInputBytes, &counts->countInputsBytes, FALSE);
        PackingExtendAddByteCounts((packingExtend >> RAVENSCROLL_PACKING_EXTEND_OUTPUTS_SHIFT) & RAVENSCROLL_PACKING_EXTEND_MASK,
            &counts->firstOutputBytes, &counts->countOutputsBytes, FALSE);
    }
    
//  Packing for quantity
    
    switch (packing & RAVENSCROLL_PACKING_QUANTITY_MASK)
    {
        case RAVENSCROLL_PACKING_QUANTITY_1_BYTE:
            counts->quantityBytes=1;
            break;
            
        case RAVENSCROLL_PACKING_QUANTITY_2_BYTES:
            counts->quantityBytes=2;
            break;
            
        case RAVENSCROLL_PACKING_QUANTITY_3_BYTES:
            counts->quantityBytes=3;
            break;
            
        case RAVENSCROLL_PACKING_QUANTITY_4_BYTES:
            counts->quantityBytes=4;
            break;
            
        case RAVENSCROLL_PACKING_QUANTITY_6_BYTES:
            counts->quantityBytes=6;
            break;
            
        case RAVENSCROLL_PACKING_QUANTITY_FLOAT:
            counts->quantityBytes=RAVENSCROLL_TRANSFER_QTY_FLOAT_LENGTH;
            break;
    }
}

static bool EncodePackingExtend(const PackingOptions packingOptions, char *packingExtend)
{
    PackingType option;
    
    for (option=firstPackingType; option<countPackingTypes; option++)
        if (packingOptions[option]) {
            *packingExtend=packingExtendMap[option];
            return TRUE;
        }
    
    return FALSE;
}

static bool DecodePackingExtend(const char packingExtend, PackingType *packingType, bool forMessages)
{
    PackingType option;
    
    for (option=firstPackingType; option<countPackingTypes; option++)
        if (packingExtend==packingExtendMap[option])
            if (option!=(forMessages ? _1S : _0_1_BYTE)) { // no _1S for messages, no _0_1_BYTE for transfers
                *packingType=option;
                return TRUE;
            }
    
    return FALSE;
}

static bool GetMessageOutputRangePacking(const RavenScrollIORange *outputRange, int countOutputs, char* packing, size_t *firstBytes, size_t *countBytes)
{
    PackingOptions packingOptions;
    char packingExtend;
    
    GetPackingOptions(NULL, outputRange, countOutputs, packingOptions, TRUE);
    
    *firstBytes=0;
    *countBytes=0;
    
    if (packingOptions[_1_0_BYTE] && (outputRange->first<=RAVENSCROLL_OUTPUTS_VALUE_MAX)) // inline single output
        *packing=RAVENSCROLL_OUTPUTS_TYPE_SINGLE | (outputRange->first & RAVENSCROLL_OUTPUTS_VALUE_MASK);
    
    else if (packingOptions[_0_1_BYTE] && (outputRange->count<=RAVENSCROLL_OUTPUTS_VALUE_MAX)) // inline first few outputs
        *packing=RAVENSCROLL_OUTPUTS_TYPE_FIRST | (outputRange->count & RAVENSCROLL_OUTPUTS_VALUE_MASK);
    
    else { // we'll be taking additional bytes
        if (!EncodePackingExtend(packingOptions, &packingExtend))
            return FALSE;
        
        PackingExtendAddByteCounts(packingExtend, firstBytes, countBytes, TRUE);
        
        *packing=RAVENSCROLL_OUTPUTS_TYPE_EXTEND | (packingExtend & RAVENSCROLL_OUTPUTS_VALUE_MASK);
    }
    
    return TRUE;
}

static bool LocateMetadataRange(const char** _metadataPtr, const char** _metadataEnd, char desiredPrefix)
{
    char foundPrefix;
    const char *metadataPtr, *metadataEnd;
    
    metadataPtr=*_metadataPtr;
    metadataEnd=*_metadataEnd;
    
    if ( (metadataPtr+RAVENSCROLL_METADATA_IDENTIFIER_LEN+1) > metadataEnd ) // check for 4 bytes at least
        return FALSE;
    
    if (memcmp(metadataPtr, RAVENSCROLL_METADATA_IDENTIFIER, RAVENSCROLL_METADATA_IDENTIFIER_LEN)) // check it starts 'SPK'
        return FALSE;
    
    metadataPtr+=RAVENSCROLL_METADATA_IDENTIFIER_LEN; // skip past 'SPK'
    
    while (metadataPtr<metadataEnd) {
        foundPrefix=*metadataPtr++; // read the next prefix
        
        if (desiredPrefix ? (foundPrefix==desiredPrefix) : (foundPrefix>RAVENSCROLL_LENGTH_PREFIX_MAX)) {
            // it's our data from here to the end (if desiredPrefix is 0, it matches the last one whichever it is)
            *_metadataPtr=metadataPtr;
            return TRUE;
        }
        
        if (foundPrefix>RAVENSCROLL_LENGTH_PREFIX_MAX) // it's some other type of data from here to end
            return FALSE;
        
        // if we get here it means we found a length byte
        
        if (foundPrefix>(metadataEnd-metadataPtr)) // something went wrong - length indicated is longer than that available
            return FALSE;
        
        if (metadataPtr>=metadataEnd) // something went wrong - that was the end of the input data
            return FALSE;
        
        if (*metadataPtr==desiredPrefix) { // it's the length of our part
            *_metadataPtr=metadataPtr+1;
            *_metadataEnd=metadataPtr+foundPrefix;
            return TRUE;
            
        } else
            metadataPtr+=foundPrefix; // skip over this many bytes
    }
    
    return FALSE;
}

static size_t ShrinkLowerDomainName(const char* fullDomainName, size_t fullDomainNameLen, char* shortDomainName, size_t shortDomainNameMaxLen, char* packing)
{
    char sourceDomainName[256];
    int charIndex, bestPrefixLen, prefixIndex, prefixLen, bestPrefixIndex, bestSuffixLen, suffixIndex, suffixLen, bestSuffixIndex;
    size_t sourceDomainLen;

//  Check we have things in range
    
    if (fullDomainNameLen>=sizeof(sourceDomainName)) // >= because of null terminator
        return 0;
    
    if (fullDomainNameLen<=0)
        return 0; // nothing there
    
//  Convert to lower case and C-terminated string
    
    sourceDomainLen=fullDomainNameLen;

    for (charIndex=0; charIndex<sourceDomainLen; charIndex++)
        sourceDomainName[charIndex]=tolower(fullDomainName[charIndex]);
    
    sourceDomainName[sourceDomainLen]=0;

//  Search for prefixes
    
    bestPrefixLen=-1;
    for (prefixIndex=0; prefixIndex<(sizeof(domainNamePrefixes)/sizeof(*domainNamePrefixes)); prefixIndex++) {
        prefixLen=(int)strlen(domainNamePrefixes[prefixIndex]);
        
        if ( (prefixLen>bestPrefixLen) && (strncmp(sourceDomainName, domainNamePrefixes[prefixIndex], prefixLen)==0) ) {
            bestPrefixIndex=prefixIndex;
            bestPrefixLen=prefixLen;
        }
    }
    
    sourceDomainLen-=bestPrefixLen;
    memmove(sourceDomainName, sourceDomainName+bestPrefixLen, sourceDomainLen+1); // includes null terminator
    
//  Search for suffixes
    
    bestSuffixLen=-1;
    for (suffixIndex=0; suffixIndex<(sizeof(domainNameSuffixes)/sizeof(*domainNameSuffixes)); suffixIndex++) {
        suffixLen=(int)strlen(domainNameSuffixes[suffixIndex]);
        
        if ( (suffixLen>bestSuffixLen) && (strncmp(sourceDomainName+sourceDomainLen-suffixLen, domainNameSuffixes[suffixIndex], suffixLen)==0) ) {
            bestSuffixIndex=suffixIndex;
            bestSuffixLen=suffixLen;
        }
    }
    
    sourceDomainLen-=bestSuffixLen;
    sourceDomainName[sourceDomainLen]=0x00;
    
//  Output and return
    
    if (sourceDomainLen>=shortDomainNameMaxLen)
        return 0;
    
    strcpy(shortDomainName, sourceDomainName);
    *packing=((bestPrefixIndex<<RAVENSCROLL_DOMAIN_PACKING_PREFIX_SHIFT)&RAVENSCROLL_DOMAIN_PACKING_PREFIX_MASK)|
        (bestSuffixIndex&RAVENSCROLL_DOMAIN_PACKING_SUFFIX_MASK);
 
    return sourceDomainLen;
}

static size_t ExpandDomainName(const char* shortDomainName, size_t shortDomainNameLen, const char packing, char* fullDomainName, size_t fullDomainNameMaxLen)
{
    char destDomainName[256];
    int prefixIndex, prefixLen, suffixIndex;
    size_t destDomainLen;
    
    if (shortDomainNameLen>=(sizeof(destDomainName)-12))
        return 0; // too long to safely expand

//  Convert to C-terminated string
    
    destDomainLen=shortDomainNameLen;
    memmove(destDomainName, shortDomainName, destDomainLen);
    destDomainName[destDomainLen]=0;
    
//  Prepend prefix
    
    prefixIndex=(packing&RAVENSCROLL_DOMAIN_PACKING_PREFIX_MASK)>>RAVENSCROLL_DOMAIN_PACKING_PREFIX_SHIFT;
    if (prefixIndex>=(sizeof(domainNamePrefixes)/sizeof(*domainNamePrefixes)))
        return 0; // out of range
    
    prefixLen=(int)strlen(domainNamePrefixes[prefixIndex]);
    memmove(destDomainName+prefixLen, destDomainName, destDomainLen+1);
    memcpy(destDomainName, domainNamePrefixes[prefixIndex], prefixLen);
    destDomainLen+=prefixLen;
    
//  Append suffix
    
    suffixIndex=packing&RAVENSCROLL_DOMAIN_PACKING_SUFFIX_MASK;
    if (suffixIndex>=(sizeof(domainNameSuffixes)/sizeof(*domainNameSuffixes)))
        return 0; // out of range

    strcpy(destDomainName+destDomainLen, domainNameSuffixes[suffixIndex]);
    destDomainLen+=strlen(domainNameSuffixes[suffixIndex]);
    
//  Output and return
    
    if (destDomainLen>fullDomainNameMaxLen)
        return 0;
    
    strcpy(fullDomainName, destDomainName);
    
    return destDomainLen;
}

static bool ReadIPv4Address(const char* string, u_int8_t* octets)
{
    int octetNum, octetValue;
    char stringChar;
    
    for (octetNum=0; octetNum<4; octetNum++) {
        octetValue=0;
        
        while (TRUE) {
            stringChar=*string++;
            
            if ((stringChar>='0') && (stringChar<='9')) {
                octetValue=octetValue*10+(stringChar-'0');
                if (octetValue>255)
                    return FALSE;
            
            } else if ((stringChar=='.') || (stringChar==0x00)) {
                break;
                
            } else
                return FALSE;
        }

        octets[octetNum]=octetValue;
        if (stringChar != ((octetNum==3) ? 0x00 : '.'))
            return FALSE;
    }
    
    return TRUE;
}

static size_t EncodeDomainPathTriplets(const char* string, const size_t stringLen, char* _metadataPtr, const char* metadataEnd)
{
    int stringPos, stringTriplet, encodeValue;
    char *metadataPtr, *foundCharPtr;
    
    metadataPtr=_metadataPtr;
    
    stringTriplet=0;
    for (stringPos=0; stringPos<stringLen; stringPos++) {
        foundCharPtr=strchr(domainPathChars, tolower(string[stringPos]));
        if (foundCharPtr==NULL)
            goto cannotEncodeTriplets; // invalid character found
        
        encodeValue=(int)(foundCharPtr-domainPathChars);
        
        switch (stringPos%3)
        {
            case 0:
                stringTriplet=encodeValue;
                break;
                
            case 1:
                stringTriplet+=encodeValue*RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE;
                break;
                
            case 2:
                stringTriplet+=encodeValue*RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE*RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE;
                break;
        }
        
        if ( ((stringPos%3)==2) || (stringPos==(stringLen-1)) ) { // write out 2 bytes if we've collected 3 chars, or if we're finishing
            if ((metadataPtr+2)<=metadataEnd) {
                if (!WriteSmallEndianUnsigned(stringTriplet, metadataPtr, 2))
                    goto cannotEncodeTriplets;
                
                metadataPtr+=2;
            } else
                goto cannotEncodeTriplets;
        }
    }
    
    return metadataPtr-_metadataPtr;
    
    cannotEncodeTriplets:
    return 0;
}

static size_t DecodeDomainPathTriplets(const char* _metadataPtr, const char* metadataEnd, char* string, size_t stringMaxLen, int parts)
{
    const char *metadataPtr;
    int stringPos, stringTriplet, decodeValue;
    char decodeChar;
    
    metadataPtr=_metadataPtr;
    stringPos=0;
    
    while (parts>0) {
        if ((stringPos+1)>=stringMaxLen)
            goto cannotDecodeTriplets; // ran out of buffer space
            
        if ((stringPos%3)==0) {
            if ((metadataPtr+2)<=metadataEnd) {
                stringTriplet=(int)ReadSmallEndianUnsigned(metadataPtr, 2);
                if (stringTriplet>=(RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE*RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE*RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE))
                    goto cannotDecodeTriplets; // invalid value
                
                metadataPtr+=2;
                
            } else
                goto cannotDecodeTriplets; // ran out of metadata
        }
        
        switch (stringPos%3)
        {
            case 0:
                decodeValue=stringTriplet%RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE;
                break;
                
            case 1:
                decodeValue=(stringTriplet/RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE)%RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE;
                break;
                
            case 2:
                decodeValue=stringTriplet/(RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE*RAVENSCROLL_DOMAIN_PATH_ENCODE_BASE);
                break;
        }
        
        decodeChar=domainPathChars[decodeValue];
        string[stringPos++]=decodeChar;
       
        if ((decodeChar==RAVENSCROLL_DOMAIN_PATH_TRUE_END_CHAR) || (decodeChar==RAVENSCROLL_DOMAIN_PATH_FALSE_END_CHAR))
            parts--;
    }
    
    string[stringPos]=0x00; // null terminator

    return metadataPtr-_metadataPtr;
    
    cannotDecodeTriplets:
    return 0;
}

static size_t EncodeDomainAndOrPath(const char* domainName, bool useHttps, const char* pagePath, bool usePrefix,
                                      char* _metadataPtr, const char* metadataEnd, bool forMessages)
{
    size_t encodeStringLen, pagePathLen, encodeLen;
    char *metadataPtr, packing, encodeString[256];
    u_int8_t octets[4];
    
    metadataPtr=_metadataPtr;
    encodeStringLen=0;

//  Domain name
    
    if (domainName) {
        if (ReadIPv4Address(domainName, octets)) { // special space-saving encoding for IPv4 addresses
            
            if ((metadataPtr+5)<=metadataEnd) {

                if (pagePath && forMessages && (*pagePath==0)) {
                    packing=RAVENSCROLL_DOMAIN_PACKING_SUFFIX_IPv4_NO_PATH;
                    if (usePrefix)
                        packing|=RAVENSCROLL_DOMAIN_PACKING_IPv4_NO_PATH_PREFIX;

                    pagePath=NULL; // skip encoding the empty page path

                } else
                    packing=RAVENSCROLL_DOMAIN_PACKING_SUFFIX_IPv4;
                
                if (useHttps)
                    packing|=RAVENSCROLL_DOMAIN_PACKING_IPv4_HTTPS;

                *metadataPtr++=packing;
                
                ((u_int8_t*)metadataPtr)[0]=octets[0];
                ((u_int8_t*)metadataPtr)[1]=octets[1];
                ((u_int8_t*)metadataPtr)[2]=octets[2];
                ((u_int8_t*)metadataPtr)[3]=octets[3];
                
                metadataPtr+=4;
                
            } else
                goto cannotEncodeDomainAndPath;
            
        } else { // otherwise shrink the domain name and prepare it for encoding
            
            encodeStringLen=ShrinkLowerDomainName(domainName, strlen(domainName), encodeString, sizeof(encodeString), &packing);
            if (!encodeStringLen)
                goto cannotEncodeDomainAndPath;
            
            if (metadataPtr<metadataEnd)
                *metadataPtr++=packing;
            else
                goto cannotEncodeDomainAndPath;
            
            encodeString[encodeStringLen++]=useHttps ? RAVENSCROLL_DOMAIN_PATH_TRUE_END_CHAR : RAVENSCROLL_DOMAIN_PATH_FALSE_END_CHAR;
        }
    }
    
//  Page path
    
    if (pagePath) {
        pagePathLen=strlen(pagePath);
        if ((encodeStringLen+pagePathLen+2)>sizeof(encodeString)) // check sufficient space in local buffer
            goto cannotEncodeDomainAndPath;
        
        memcpy(encodeString+encodeStringLen, pagePath, pagePathLen);
        encodeStringLen+=pagePathLen;
        encodeString[encodeStringLen++]=usePrefix ? RAVENSCROLL_DOMAIN_PATH_TRUE_END_CHAR : RAVENSCROLL_DOMAIN_PATH_FALSE_END_CHAR;
    }
    
//  Encode whatever is required as triplets
    
    if (encodeStringLen) {
        encodeLen=EncodeDomainPathTriplets(encodeString, encodeStringLen, metadataPtr, metadataEnd);
        if (!encodeLen)
            goto cannotEncodeDomainAndPath;
        
        metadataPtr+=encodeLen;
    }
    
    return metadataPtr-_metadataPtr;
    
    cannotEncodeDomainAndPath:
    return 0;
}

static size_t DecodeDomainAndOrPath(const char* _metadataPtr, const char* metadataEnd, char* domainName, size_t domainNameMaxLen, bool* useHttps, char* pagePath, size_t pagePathMaxLen, bool* usePrefix, bool forMessages)
{
    const char *metadataPtr;
    size_t decodedLen, ipAddressLen, metadataLen, decodePathLen, prevDecodedLen;
    bool isIpAddress;
    char packing, packingSuffix, ipAddress[16], decodeString[256], decodeChar;
    u_int8_t octets[4];
    int metadataParts;

    metadataPtr=_metadataPtr;
    metadataParts=0;

//  Domain name
    
    if (domainName) {
 
    //  Get packing byte
        
        if (metadataPtr<metadataEnd)
            packing=*metadataPtr++;
        else
            goto cannotDecodeDomainAndPath;

    //  Extract IP address if present
        
        packingSuffix=packing&RAVENSCROLL_DOMAIN_PACKING_SUFFIX_MASK;
        isIpAddress=(packingSuffix==RAVENSCROLL_DOMAIN_PACKING_SUFFIX_IPv4) ||
            (forMessages && (packingSuffix==RAVENSCROLL_DOMAIN_PACKING_SUFFIX_IPv4_NO_PATH));
        
        if (isIpAddress) {
            *useHttps=(packing&RAVENSCROLL_DOMAIN_PACKING_IPv4_HTTPS) ? TRUE : FALSE;
            
            if ((metadataPtr+4)<=metadataEnd) {
                octets[0]=((u_int8_t*)metadataPtr)[0];
                octets[1]=((u_int8_t*)metadataPtr)[1];
                octets[2]=((u_int8_t*)metadataPtr)[2];
                octets[3]=((u_int8_t*)metadataPtr)[3];
                
                metadataPtr+=4;
            
            } else
                goto cannotDecodeDomainAndPath;
            
            sprintf(ipAddress, "%u.%u.%u.%u", octets[0], octets[1], octets[2], octets[3]);
            ipAddressLen=strlen(ipAddress);
            
            if (ipAddressLen>=domainNameMaxLen) // allow for null terminator
                goto cannotDecodeDomainAndPath;
            
            strcpy(domainName, ipAddress);
            
            if (pagePath && forMessages && (packingSuffix==RAVENSCROLL_DOMAIN_PACKING_SUFFIX_IPv4_NO_PATH)) {
                *pagePath=0;
                *usePrefix=(packing&RAVENSCROLL_DOMAIN_PACKING_IPv4_NO_PATH_PREFIX) ? TRUE : FALSE;
                pagePath=NULL; // skip decoding the empty page path
            }
        
        } else
            metadataParts++;
    }
    
//  Convert remaining metadata to string
    
    if (pagePath)
        metadataParts++;
    
    if (metadataParts>0) {
        metadataLen=DecodeDomainPathTriplets(metadataPtr, metadataEnd, decodeString, sizeof(decodeString), metadataParts);
        if (!metadataLen)
            goto cannotDecodeDomainAndPath;

        metadataPtr+=metadataLen;
        decodedLen=0;
        
    //  Extract domain name if IP address was not present
        
        if (domainName && !isIpAddress)
            while (TRUE) {
                if (decodedLen>=sizeof(decodeString))
                    goto cannotDecodeDomainAndPath; // should never happen
                
                decodeChar=decodeString[decodedLen++];

                if ((decodeChar==RAVENSCROLL_DOMAIN_PATH_TRUE_END_CHAR) || (decodeChar==RAVENSCROLL_DOMAIN_PATH_FALSE_END_CHAR)) {
                    if (!ExpandDomainName(decodeString, decodedLen-1, packing, domainName, domainNameMaxLen))
                        goto cannotDecodeDomainAndPath;
                    
                    *useHttps=(decodeChar==RAVENSCROLL_DOMAIN_PATH_TRUE_END_CHAR);
                    break;
                }
            }
        
    //  Extract page path
        
        prevDecodedLen=decodedLen;
        
        if (pagePath)
            while (TRUE) {
                if (decodedLen>=sizeof(decodeString))
                    goto cannotDecodeDomainAndPath; // should never happen

                decodeChar=decodeString[decodedLen++];
                
                if ((decodeChar==RAVENSCROLL_DOMAIN_PATH_TRUE_END_CHAR) || (decodeChar==RAVENSCROLL_DOMAIN_PATH_FALSE_END_CHAR)) {
                    decodePathLen=decodedLen-1-prevDecodedLen;
                    if (decodePathLen>=pagePathMaxLen)
                        goto cannotDecodeDomainAndPath;
                    
                    memcpy(pagePath, decodeString+prevDecodedLen, decodePathLen);
                    pagePath[decodePathLen]=0x00;
                    *usePrefix=(decodeChar==RAVENSCROLL_DOMAIN_PATH_TRUE_END_CHAR);
                    break;
                }
            }
    }
    
//  Finish and return
    
    return metadataPtr-_metadataPtr;
    
    cannotDecodeDomainAndPath:
    return 0;
}

static int CompareAssetRefs(const RavenScrollAssetRef* assetRef1, const RavenScrollAssetRef* assetRef2)
{
    // -1 if assetRef1<assetRef2, 1 if assetRef2>assetRef1, 0 otherwise

    if (assetRef1->blockNum!=assetRef2->blockNum)
        return (assetRef1->blockNum<assetRef2->blockNum) ? -1 : 1;
    else if (assetRef1->blockNum==RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE) // in this case don't compare other fields
        return 0;
    else if (assetRef1->txOffset!=assetRef2->txOffset)
        return (assetRef1->txOffset<assetRef2->txOffset) ? -1 : 1;
    else
        return memcmp(assetRef1->txIDPrefix, assetRef2->txIDPrefix, RAVENSCROLL_ASSETREF_TXID_PREFIX_LEN);
}

static void TransfersGroupOrdering(const RavenScrollTransfer* transfers, int* ordering, int countTransfers)
{
    bool *transferUsed;
    int orderIndex, transferIndex, bestTransferIndex, transferScore, bestTransferScore;
    
    transferUsed=(bool*)malloc(countTransfers*sizeof(*transferUsed));
    
    for (transferIndex=0; transferIndex<countTransfers; transferIndex++)
        transferUsed[transferIndex]=FALSE;
    
    for (orderIndex=0; orderIndex<countTransfers; orderIndex++) {
        bestTransferScore=0;
        bestTransferIndex=-1;
        
        for (transferIndex=0; transferIndex<countTransfers; transferIndex++)
            if (!transferUsed[transferIndex]) {
                if (transfers[transferIndex].assetRef.blockNum==RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE)
                    transferScore=3; // top priority to default routes, which must be first in the encoded list
                else if ((orderIndex>0) && RavenScrollAssetRefMatch(&transfers[ordering[orderIndex-1]].assetRef, &transfers[transferIndex].assetRef))
                    transferScore=2; // then next best is one which has same asset reference as previous
                else
                    transferScore=1; // otherwise any will do
        
                if (transferScore>bestTransferScore) { // if it's clearly the best, take it
                    bestTransferScore=transferScore;
                    bestTransferIndex=transferIndex;
                
                } else if (transferScore==bestTransferScore) // otherwise give priority to "lower" asset references
                    if (CompareAssetRefs(&transfers[transferIndex].assetRef, &transfers[bestTransferIndex].assetRef)<0)
                        bestTransferIndex=transferIndex;
            }
        
        ordering[orderIndex]=bestTransferIndex;
        transferUsed[bestTransferIndex]=TRUE;
    }
    
    free(transferUsed);
}

static int NormalizeIORanges(const RavenScrollIORange* inRanges, RavenScrollIORange* outRanges, int countRanges)
{
    int rangeIndex, orderIndex, lowestRangeIndex, lowestRangeFirst, countRemoved, lastRangeEnd, thisRangeEnd;
    bool *rangeUsed;
    
    rangeUsed=(bool*)malloc(countRanges*sizeof(*rangeUsed));
    
    for (rangeIndex=0; rangeIndex<countRanges; rangeIndex++)
        rangeUsed[rangeIndex]=FALSE;
    
    countRemoved=0;
    
    for (orderIndex=0; orderIndex<countRanges; orderIndex++) {
        lowestRangeFirst=0;
        lowestRangeIndex=-1;
        
        for (rangeIndex=0; rangeIndex<countRanges; rangeIndex++)
            if (!rangeUsed[rangeIndex])
                if ( (lowestRangeIndex==-1) || (inRanges[rangeIndex].first<lowestRangeFirst) ) {
                    lowestRangeFirst=inRanges[rangeIndex].first;
                    lowestRangeIndex=rangeIndex;
                }
        
        if ((orderIndex>0) && (inRanges[lowestRangeIndex].first<=lastRangeEnd)) { // we can combine two adjacent ranges
            countRemoved++;
            thisRangeEnd=inRanges[lowestRangeIndex].first+inRanges[lowestRangeIndex].count;
            outRanges[orderIndex-countRemoved].count=RAVENSCROLL_MAX(lastRangeEnd, thisRangeEnd)-outRanges[orderIndex-countRemoved].first;
        
        } else
            outRanges[orderIndex-countRemoved]=inRanges[lowestRangeIndex];
            
        lastRangeEnd=outRanges[orderIndex-countRemoved].first+outRanges[orderIndex-countRemoved].count;
        rangeUsed[lowestRangeIndex]=TRUE;
    }

    free(rangeUsed);
    
    return countRanges-countRemoved;
}

// Public functions

size_t RavenScrollScriptToMetadata(const char* scriptPubKey, const size_t scriptPubKeyLen, const bool scriptIsHex,
                                 char* metadata, const size_t metadataMaxLen)
{
    size_t scriptPubKeyRawLen, metadataLen;
    const char *scriptPubKeyRaw;
    
    scriptPubKeyRawLen=AllocRawScript(scriptPubKey, scriptPubKeyLen, scriptIsHex, &scriptPubKeyRaw);
    metadataLen=scriptPubKeyRawLen-2;
    
    if ( (scriptPubKeyRawLen>2) && (scriptPubKeyRaw[0]==0x6a) && (scriptPubKeyRaw[1]>0) && (scriptPubKeyRaw[1]<=75) && (scriptPubKeyRaw[1]==metadataLen))
        memcpy(metadata, scriptPubKeyRaw+2, metadataLen);
    else
        metadataLen=0;
    
    FreeRawScript(scriptIsHex, scriptPubKeyRaw);
   
    return metadataLen;
}

size_t RavenScrollScriptsToMetadata(const char* scriptPubKeys[], const size_t scriptPubKeyLens[], const bool scriptsAreHex,
                                  const int countScripts, char* metadata, const size_t metadataMaxLen)
{
    int scriptIndex;
    
    for (scriptIndex=0; scriptIndex<countScripts; scriptIndex++)
        if (!RavenScrollScriptIsRegular(scriptPubKeys[scriptIndex], scriptPubKeyLens[scriptIndex], scriptsAreHex))
            return RavenScrollScriptToMetadata(scriptPubKeys[scriptIndex], scriptPubKeyLens[scriptIndex], scriptsAreHex, metadata, metadataMaxLen);
    
    return 0;
}

size_t RavenScrollMetadataToScript(const char* metadata, const size_t metadataLen,
                                 char* scriptPubKey, const size_t scriptPubKeyMaxLen, const bool toHexScript)
{
    size_t scriptRawLen;
    
    if (metadataLen<=75) {
        scriptRawLen=metadataLen+2;
        
        if (toHexScript) {
            if (scriptPubKeyMaxLen>=(2*scriptRawLen+1)) { // include null terminator for hex string
                UInt8ToHexCharPair(0x6a, scriptPubKey);
                UInt8ToHexCharPair(metadataLen, scriptPubKey+2);
                BinaryToHex(metadata, metadataLen, scriptPubKey+4);
                scriptPubKey[2*scriptRawLen]=0x00; // null terminator
                
                return 2*scriptRawLen;
            }
     
        } else {
            if (scriptPubKeyMaxLen>=scriptRawLen) {
                scriptPubKey[0]=0x6a;
                scriptPubKey[1]=metadataLen;
                memcpy(scriptPubKey+2, metadata, metadataLen);

                return scriptRawLen;
            }
        }
    }
    
    return 0;
}

size_t RavenScrollMetadataMaxAppendLen(char* metadata, const size_t metadataLen, const size_t metadataMaxLen)
{
    int appendLen;
    
    appendLen=(int)metadataMaxLen-((int)metadataLen+1-RAVENSCROLL_METADATA_IDENTIFIER_LEN);

    return RAVENSCROLL_MAX(appendLen, 0);
}

size_t RavenScrollMetadataAppend(char* metadata, const size_t metadataLen, const size_t metadataMaxLen, const char* appendMetadata, const size_t appendMetadataLen)
{
    size_t needLength, lastMetadataLen;
    char *lastMetadata, *lastMetadataEnd;
    
    lastMetadata=metadata;
    lastMetadataEnd=lastMetadata+metadataLen;
    
    if (!LocateMetadataRange((const char**)&lastMetadata, (const char**)&lastMetadataEnd, 0)) // check we can find last metadata
        return 0;
    
    if (appendMetadataLen<(RAVENSCROLL_METADATA_IDENTIFIER_LEN+1)) // check there is enough to check the prefix
        return 0;
    
    if (memcmp(appendMetadata, RAVENSCROLL_METADATA_IDENTIFIER, RAVENSCROLL_METADATA_IDENTIFIER_LEN)) // then check the prefix
        return 0;
    
    // we don't check the character after the prefix in appendMetadata because it could itself be composite
               
    needLength=metadataLen+appendMetadataLen-RAVENSCROLL_METADATA_IDENTIFIER_LEN+1; // check there is enough spae
    if (metadataMaxLen<needLength)
        return 0;
    
    lastMetadataLen=lastMetadataEnd-lastMetadata+1; // include prefix
    memmove(lastMetadata, lastMetadata-1, lastMetadataLen);
    lastMetadata[-1]=lastMetadataLen;
    
    memcpy(lastMetadata+lastMetadataLen, appendMetadata+RAVENSCROLL_METADATA_IDENTIFIER_LEN, appendMetadataLen-RAVENSCROLL_METADATA_IDENTIFIER_LEN);
    
    return needLength;
}

bool RavenScrollScriptIsRegular(const char* scriptPubKey, const size_t scriptPubKeyLen, const bool scriptIsHex)
{
    u_int8_t firstValue;
    
    if (scriptIsHex)
        return (scriptPubKeyLen<2) || (!HexToBinary(scriptPubKey, &firstValue, 1)) || (firstValue!=0x6a);
    else
        return (scriptPubKeyLen<1) || (scriptPubKey[0]!=0x6a);
}

void RavenScrollAddressClear(RavenScrollAddress* address)
{
    address->bitcoinAddress[0]=0x00;
    address->addressFlags=0;
    address->paymentRef=0;
}

bool RavenScrollAddressToString(const RavenScrollAddress* address, char* string, const size_t stringMaxLen)
{
    typedef struct {
        RavenScrollAddressFlags flag;
        char* string;
    } FlagToString;

    char buffer[1024], *bufferPtr;
    size_t bufferLength, copyLength;
    int flagIndex;
    bool flagOutput;

    FlagToString flagsToStrings[]={
        { RAVENSCROLL_ADDRESS_FLAG_ASSETS, "assets" },
        { RAVENSCROLL_ADDRESS_FLAG_PAYMENT_REFS, "payment references" },
        { RAVENSCROLL_ADDRESS_FLAG_TEXT_MESSAGES, "text messages" },
        { RAVENSCROLL_ADDRESS_FLAG_FILE_MESSAGES, "file messages" }
    };
    
    bufferPtr=buffer;
    
    bufferPtr+=sprintf(bufferPtr, "RAVENSCROLL ADDRESS\n");
    bufferPtr+=sprintf(bufferPtr, "  Bitcoin address: %s\n", address->bitcoinAddress);
    bufferPtr+=sprintf(bufferPtr, "    Address flags: %d", address->addressFlags);
    
    flagOutput=FALSE;

    for (flagIndex=0; flagIndex<(sizeof(flagsToStrings)/sizeof(*flagsToStrings)); flagIndex++)
        if (address->addressFlags & flagsToStrings[flagIndex].flag) {
            bufferPtr+=sprintf(bufferPtr, "%s%s", flagOutput ? ", " : " [", flagsToStrings[flagIndex].string);
            flagOutput=TRUE;
        }

    bufferPtr+=sprintf(bufferPtr, "%s\n", flagOutput ? "]" : "");
    
    bufferPtr+=sprintf(bufferPtr, "Payment reference: %lld\n", (long long)address->paymentRef);
    bufferPtr+=sprintf(bufferPtr, "END RAVENSCROLL ADDRESS\n\n");

    bufferLength=bufferPtr-buffer;
    copyLength=RAVENSCROLL_MIN(bufferLength, stringMaxLen-1);
    memcpy(string, buffer, copyLength);
    string[copyLength]=0x00;
    
    return (copyLength==bufferLength);
}

bool RavenScrollAddressIsValid(const RavenScrollAddress* address)
{
    if (strlen(address->bitcoinAddress)==0)
        return FALSE;

    if ((address->addressFlags&RAVENSCROLL_ADDRESS_FLAG_MASK) != address->addressFlags)
        return FALSE;
    
    return RavenScrollPaymentRefIsValid(address->paymentRef);
}

bool RavenScrollAddressMatch(const RavenScrollAddress* address1, const RavenScrollAddress* address2)
{
    return (!strcmp(address1->bitcoinAddress, address2->bitcoinAddress)) &&
    (address1->addressFlags==address2->addressFlags) && (address1->paymentRef==address2->paymentRef);
}

size_t RavenScrollAddressEncode(const RavenScrollAddress* address, char* string, const size_t stringMaxLen)
{
    size_t bitcoinAddressLen, stringLen, halfLength;
    int charIndex, charValue, addressFlagChars, paymentRefChars, extraDataChars;
    RavenScrollAddressFlags testAddressFlags;
    RavenScrollPaymentRef testPaymentRef;
    char stringBase58[1024];
    
    if (!RavenScrollAddressIsValid(address))
        goto cannotEncodeAddress;
    
//  Build up extra data for address flags
    
    addressFlagChars=0;
    testAddressFlags=address->addressFlags;
    
    while (testAddressFlags>0) {
        stringBase58[2+addressFlagChars]=testAddressFlags%58;
        testAddressFlags/=58; // keep as integer
        addressFlagChars++;
    }
    
//  Build up extra data for payment reference
    
    paymentRefChars=0;
    testPaymentRef=address->paymentRef;
    
    while (testPaymentRef>0) {
        stringBase58[2+addressFlagChars+paymentRefChars]=testPaymentRef%58;
        testPaymentRef/=58; // keep as integer
        paymentRefChars++;
    }

//  Calculate/encode extra length and total length required
    
    extraDataChars=addressFlagChars+paymentRefChars;
    bitcoinAddressLen=strlen(address->bitcoinAddress);
    stringLen=bitcoinAddressLen+2+extraDataChars;

    if (stringMaxLen<=stringLen) // use <= because we will also add 0x00 C terminator byte
        return 0;
    
    if (stringLen>sizeof(stringBase58))
        return 0;

    stringBase58[1]=addressFlagChars*RAVENSCROLL_ADDRESS_FLAG_CHARS_MULTIPLE+paymentRefChars;

//  Convert the bitcoin address
    
    for (charIndex=0; charIndex<bitcoinAddressLen; charIndex++) {
        charValue=Base58ToInteger(address->bitcoinAddress[charIndex]);
        if (charValue<0)
            return 0; // invalid base58 character

        charValue+=RAVENSCROLL_ADDRESS_CHAR_INCREMENT;
        
        if (extraDataChars>0)
            charValue+=stringBase58[2+charIndex%extraDataChars];
        
        stringBase58[2+extraDataChars+charIndex]=charValue%58;
    }
    
//  Obfuscate first half of address using second half to prevent common prefixes
    
    halfLength=(stringLen+1)/2;
    for (charIndex=1; charIndex<halfLength; charIndex++) // exclude first character
        stringBase58[charIndex]=(stringBase58[charIndex]+stringBase58[stringLen-charIndex])%58;
    
//  Convert to base 58 and add prefix and terminator
    
    string[0]=RAVENSCROLL_ADDRESS_PREFIX;
    for (charIndex=1; charIndex<stringLen; charIndex++)
        string[charIndex]=integerToBase58[stringBase58[charIndex]];
    string[stringLen]=0;
    
    return stringLen;
    
    cannotEncodeAddress:
    return 0;
}

bool RavenScrollAddressDecode(RavenScrollAddress* address, const char* string, const size_t stringLen)
{
    size_t bitcoinAddressLen, halfLength;
    int charIndex, charValue, addressFlagChars, paymentRefChars, extraDataChars;
    long long multiplier;
    char stringBase58[1024];
    
//  Check for basic validity
    
    if ( (stringLen<2) || (stringLen>sizeof(stringBase58)) )
        goto cannotDecodeAddress;
    
    if (string[0]!=RAVENSCROLL_ADDRESS_PREFIX)
        goto cannotDecodeAddress;

//  Convert from base 58
    
    for (charIndex=1; charIndex<stringLen; charIndex++) { // exclude first character
        charValue=Base58ToInteger(string[charIndex]);
        if (charValue<0)
            goto cannotDecodeAddress;
        stringBase58[charIndex]=charValue;
    }
    
//  De-obfuscate first half of address using second half
    
    halfLength=(stringLen+1)/2;
    for (charIndex=1; charIndex<halfLength; charIndex++) // exclude first character
        stringBase58[charIndex]=(stringBase58[charIndex]+58-stringBase58[stringLen-charIndex])%58;
    
//  Get length of extra data
    
    charValue=stringBase58[1];
    addressFlagChars=charValue/RAVENSCROLL_ADDRESS_FLAG_CHARS_MULTIPLE; // keep as integer
    paymentRefChars=charValue%RAVENSCROLL_ADDRESS_FLAG_CHARS_MULTIPLE;
    extraDataChars=addressFlagChars+paymentRefChars;
    
    if (stringLen<(2+extraDataChars))
        goto cannotDecodeAddress;
    
//  Check we have sufficient length for the decoded address
    
    bitcoinAddressLen=stringLen-2-extraDataChars;
    if (sizeof(address->bitcoinAddress)<=bitcoinAddressLen) // use <= because we will also add 0x00 C terminator byte
        goto cannotDecodeAddress;
    
//  Read the extra data for address flags
    
    address->addressFlags=0;
    multiplier=1;
    
    for (charIndex=0; charIndex<addressFlagChars; charIndex++) {
        charValue=stringBase58[2+charIndex];
        address->addressFlags+=charValue*multiplier;
        multiplier*=58;
    }
    
//  Read the extra data for payment reference
    
    address->paymentRef=0;
    multiplier=1;
    
    for (charIndex=0; charIndex<paymentRefChars; charIndex++) {
        charValue=stringBase58[2+addressFlagChars+charIndex];
        address->paymentRef+=charValue*multiplier;
        multiplier*=58;
    }
    
//  Convert the bitcoin address
    
    for (charIndex=0; charIndex<bitcoinAddressLen; charIndex++) {
        charValue=stringBase58[2+extraDataChars+charIndex];
        charValue+=58*2-RAVENSCROLL_ADDRESS_CHAR_INCREMENT; // avoid worrying about the result of modulo on negative numbers in any language
        
        if (extraDataChars>0)
            charValue-=stringBase58[2+charIndex%extraDataChars];
        
        address->bitcoinAddress[charIndex]=integerToBase58[charValue%58];
    }
    
    address->bitcoinAddress[bitcoinAddressLen]=0; // C terminator byte
    
    return RavenScrollAddressIsValid(address);
    
    cannotDecodeAddress:
    return FALSE;
}

void RavenScrollGenesisClear(RavenScrollGenesis *genesis)
{
    genesis->qtyMantissa=0;
    genesis->qtyExponent=0;
    genesis->chargeFlatMantissa=0;
    genesis->chargeFlatExponent=0;
    genesis->chargeBasisPoints=0;
    genesis->useHttps=FALSE;
    genesis->domainName[0]=0x00;
    genesis->usePrefix=TRUE;
    genesis->pagePath[0]=0x00;
    genesis->assetHashLen=0;
}

bool RavenScrollGenesisToString(const RavenScrollGenesis *genesis, char* string, const size_t stringMaxLen)
{
    char buffer[1024], hex[128], *bufferPtr;
    size_t bufferLength, copyLength, domainPathEncodeLen;
    int quantityEncoded, chargeFlatEncoded;
    RavenScrollAssetQty quantity, chargeFlat;
    char domainPathMetadata[64];
    
    bufferPtr=buffer;

    quantity=RavenScrollGenesisGetQty(genesis);
    quantityEncoded=(genesis->qtyExponent*RAVENSCROLL_GENESIS_QTY_EXPONENT_MULTIPLE+genesis->qtyMantissa)&RAVENSCROLL_GENESIS_QTY_MASK;
    chargeFlat=RavenScrollGenesisGetChargeFlat(genesis);
    chargeFlatEncoded=genesis->chargeFlatExponent*RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MULTIPLE+genesis->chargeFlatMantissa;
    domainPathEncodeLen=EncodeDomainAndOrPath(genesis->domainName, genesis->useHttps, genesis->pagePath, genesis->usePrefix,
                                            domainPathMetadata, domainPathMetadata+sizeof(domainPathMetadata), FALSE);
    
    bufferPtr+=sprintf(bufferPtr, "RAVENSCROLL GENESIS\n");
    bufferPtr+=sprintf(bufferPtr, "   Quantity mantissa: %d\n", genesis->qtyMantissa);
    bufferPtr+=sprintf(bufferPtr, "   Quantity exponent: %d\n", genesis->qtyExponent);
    bufferPtr+=sprintf(bufferPtr, "    Quantity encoded: %d (small endian hex %s)\n", quantityEncoded, UnsignedToSmallEndianHex(quantityEncoded, RAVENSCROLL_GENESIS_QTY_FLAGS_LENGTH, hex));
    bufferPtr+=sprintf(bufferPtr, "      Quantity value: %lld\n", (long long)quantity);
    bufferPtr+=sprintf(bufferPtr, "Flat charge mantissa: %d\n", genesis->chargeFlatMantissa);
    bufferPtr+=sprintf(bufferPtr, "Flat charge exponent: %d\n", genesis->chargeFlatExponent);
    bufferPtr+=sprintf(bufferPtr, " Flat charge encoded: %d (small endian hex %s)\n", chargeFlatEncoded, UnsignedToSmallEndianHex(chargeFlatEncoded, RAVENSCROLL_GENESIS_CHARGE_FLAT_LENGTH, hex));
    bufferPtr+=sprintf(bufferPtr, "   Flat charge value: %lld\n", (long long)chargeFlat);
    bufferPtr+=sprintf(bufferPtr, " Basis points charge: %d (hex %s)\n", genesis->chargeBasisPoints, UnsignedToSmallEndianHex(genesis->chargeBasisPoints, RAVENSCROLL_GENESIS_CHARGE_BPS_LENGTH, hex));
    bufferPtr+=sprintf(bufferPtr, "           Asset URL: %s://%s/%s%s/ (length %zd+%zd encoded %s length %zd)\n",
        genesis->useHttps ? "https" : "http", genesis->domainName,
        genesis->usePrefix ? "ravenscroll/" : "", genesis->pagePath[0] ? genesis->pagePath : "[spent-txid]",
        strlen(genesis->domainName), strlen(genesis->pagePath),
        BinaryToHex(domainPathMetadata, domainPathEncodeLen, hex), domainPathEncodeLen);
    bufferPtr+=sprintf(bufferPtr, "          Asset hash: %s (length %zd)\n", BinaryToHex(genesis->assetHash, genesis->assetHashLen, hex), genesis->assetHashLen);
    bufferPtr+=sprintf(bufferPtr, "END RAVENSCROLL GENESIS\n\n");
    
    bufferLength=bufferPtr-buffer;
    copyLength=RAVENSCROLL_MIN(bufferLength, stringMaxLen-1);
    memcpy(string, buffer, copyLength);
    string[copyLength]=0x00;
    
    return (copyLength==bufferLength);
}

bool RavenScrollGenesisIsValid(const RavenScrollGenesis *genesis)
{
    if ( (genesis->qtyMantissa<RAVENSCROLL_GENESIS_QTY_MANTISSA_MIN) || (genesis->qtyMantissa>RAVENSCROLL_GENESIS_QTY_MANTISSA_MAX) )
        goto genesisIsInvalid;
    
    if ( (genesis->qtyExponent<RAVENSCROLL_GENESIS_QTY_EXPONENT_MIN) || (genesis->qtyExponent>RAVENSCROLL_GENESIS_QTY_EXPONENT_MAX) )
        goto genesisIsInvalid;
    
    if ( (genesis->chargeFlatExponent<RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MIN) || (genesis->chargeFlatExponent>RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MAX) )
        goto genesisIsInvalid;
    
    if (genesis->chargeFlatMantissa<RAVENSCROLL_GENESIS_CHARGE_FLAT_MANTISSA_MIN)
        goto genesisIsInvalid;
    
    if (genesis->chargeFlatMantissa > ((genesis->chargeFlatExponent==RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MAX) ? RAVENSCROLL_GENESIS_CHARGE_FLAT_MANTISSA_MAX_IF_EXP_MAX : RAVENSCROLL_GENESIS_CHARGE_FLAT_MANTISSA_MAX))
        goto genesisIsInvalid;
    
    if ( (genesis->chargeBasisPoints<RAVENSCROLL_GENESIS_CHARGE_BASIS_POINTS_MIN) || (genesis->chargeBasisPoints>RAVENSCROLL_GENESIS_CHARGE_BASIS_POINTS_MAX) )
        goto genesisIsInvalid;
    
    if (strlen(genesis->domainName)>RAVENSCROLL_GENESIS_DOMAIN_NAME_MAX_LEN)
        goto genesisIsInvalid;
    
    if (strlen(genesis->pagePath)>RAVENSCROLL_GENESIS_PAGE_PATH_MAX_LEN)
        goto genesisIsInvalid;
    
    if ( (genesis->assetHashLen<RAVENSCROLL_GENESIS_HASH_MIN_LEN) || (genesis->assetHashLen>RAVENSCROLL_GENESIS_HASH_MAX_LEN) )
        goto genesisIsInvalid;
    
    return TRUE;
    
    genesisIsInvalid:
    return FALSE;
}

bool RavenScrollGenesisMatch(const RavenScrollGenesis* genesis1, const RavenScrollGenesis* genesis2, const bool strict)
{
    size_t hashCompareLen;
    bool floatQuantitiesMatch;
    
    hashCompareLen=RAVENSCROLL_MIN(genesis1->assetHashLen, genesis2->assetHashLen);
    hashCompareLen=RAVENSCROLL_MIN(hashCompareLen, RAVENSCROLL_GENESIS_HASH_MAX_LEN);
    
    if (strict)
        floatQuantitiesMatch=(genesis1->qtyMantissa==genesis2->qtyMantissa) && (genesis1->qtyExponent==genesis2->qtyExponent)
        && (genesis1->chargeFlatMantissa==genesis2->chargeFlatMantissa) && (genesis1->chargeFlatExponent==genesis2->chargeFlatExponent);
    else
        floatQuantitiesMatch=(RavenScrollGenesisGetQty(genesis1)==RavenScrollGenesisGetQty(genesis2)) &&
        (RavenScrollGenesisGetChargeFlat(genesis1)==RavenScrollGenesisGetChargeFlat(genesis2));
    
    return
        floatQuantitiesMatch && (genesis1->chargeBasisPoints==genesis2->chargeBasisPoints) &&
        (genesis1->useHttps==genesis2->useHttps) &&
        (!strcasecmp(genesis1->domainName, genesis2->domainName)) &&
        (genesis1->usePrefix==genesis2->usePrefix) &&
        (!strcasecmp(genesis1->pagePath, genesis2->pagePath)) &&
        (!memcmp(&genesis1->assetHash, &genesis2->assetHash, hashCompareLen));
}

RavenScrollAssetQty RavenScrollGenesisGetQty(const RavenScrollGenesis *genesis)
{
    return MantissaExponentToQty(genesis->qtyMantissa, genesis->qtyExponent);
}

RavenScrollAssetQty RavenScrollGenesisSetQty(RavenScrollGenesis *genesis, const RavenScrollAssetQty desiredQty, const int rounding)
{
    int qtyMantissa, qtyExponent;
    
    QtyToMantissaExponent(desiredQty, rounding, RAVENSCROLL_GENESIS_QTY_MANTISSA_MAX, RAVENSCROLL_GENESIS_QTY_EXPONENT_MAX,
                          &qtyMantissa, &qtyExponent);
    
    genesis->qtyMantissa=qtyMantissa;
    genesis->qtyExponent=qtyExponent;
    
    return RavenScrollGenesisGetQty(genesis);
}

RavenScrollAssetQty RavenScrollGenesisGetChargeFlat(const RavenScrollGenesis *genesis)
{
    return MantissaExponentToQty(genesis->chargeFlatMantissa, genesis->chargeFlatExponent);
}

RavenScrollAssetQty RavenScrollGenesisSetChargeFlat(RavenScrollGenesis *genesis, RavenScrollAssetQty desiredChargeFlat, const int rounding)
{
    int chargeFlatMantissa, chargeFlatExponent;
    
    QtyToMantissaExponent(desiredChargeFlat, rounding, RAVENSCROLL_GENESIS_CHARGE_FLAT_MANTISSA_MAX, RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MAX, &chargeFlatMantissa, &chargeFlatExponent);
    
    if (chargeFlatExponent==RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MAX)
        chargeFlatMantissa=RAVENSCROLL_MIN(chargeFlatMantissa, RAVENSCROLL_GENESIS_CHARGE_FLAT_MANTISSA_MAX_IF_EXP_MAX);
    
    genesis->chargeFlatMantissa=chargeFlatMantissa;
    genesis->chargeFlatExponent=chargeFlatExponent;
    
    return RavenScrollGenesisGetChargeFlat(genesis);
}

RavenScrollAssetQty RavenScrollGenesisCalcCharge(const RavenScrollGenesis *genesis, RavenScrollAssetQty qtyGross)
{
    RavenScrollAssetQty charge;
    
    charge=RavenScrollGenesisGetChargeFlat(genesis)+(qtyGross*genesis->chargeBasisPoints+5000)/10000; // rounds to nearest
    
    return RAVENSCROLL_MIN(qtyGross, charge); // can't charge more than the final amount
}

RavenScrollAssetQty RavenScrollGenesisCalcNet(const RavenScrollGenesis *genesis, RavenScrollAssetQty qtyGross)
{
    return qtyGross-RavenScrollGenesisCalcCharge(genesis, qtyGross);
}

RavenScrollAssetQty RavenScrollGenesisCalcGross(const RavenScrollGenesis *genesis, RavenScrollAssetQty qtyNet)
{
    RavenScrollAssetQty lowerGross;
    
    if (qtyNet<=0)
        return 0; // no point getting past charges if we end up with zero anyway
    
    lowerGross=((qtyNet+RavenScrollGenesisGetChargeFlat(genesis))*10000)/(10000-genesis->chargeBasisPoints); // divides rounding down
    
    return (RavenScrollGenesisCalcNet(genesis, lowerGross)>=qtyNet) ? lowerGross : (lowerGross+1);
}

size_t RavenScrollGenesisCalcHashLen(const RavenScrollGenesis *genesis, const size_t metadataMaxLen)
{
    size_t domainPathLen;
    int assetHashLen;
    u_int8_t octets[4];
    char domainName[256], packing;
    
    assetHashLen=(int)metadataMaxLen-RAVENSCROLL_METADATA_IDENTIFIER_LEN-1-RAVENSCROLL_GENESIS_QTY_FLAGS_LENGTH;
    
    if (genesis->chargeFlatMantissa>0)
        assetHashLen-=RAVENSCROLL_GENESIS_CHARGE_FLAT_LENGTH;
    
    if (genesis->chargeBasisPoints>0)
        assetHashLen-=RAVENSCROLL_GENESIS_CHARGE_BPS_LENGTH;
    
    domainPathLen=strlen(genesis->pagePath)+1;
    
    if (ReadIPv4Address(genesis->domainName, octets))
        assetHashLen-=5; // packing and IP octets
    else {
        assetHashLen-=1; // packing
        domainPathLen+=ShrinkLowerDomainName(genesis->domainName, strlen(genesis->domainName), domainName, sizeof(domainName), &packing)+1;
    }
    
    assetHashLen-=2*((domainPathLen+2)/3); // uses integer arithmetic

    return RAVENSCROLL_MIN(RAVENSCROLL_MAX(assetHashLen, 0), RAVENSCROLL_GENESIS_HASH_MAX_LEN);
}

size_t RavenScrollGenesisEncode(const RavenScrollGenesis *genesis, char* metadata, const size_t metadataMaxLen)
{
    char* metadataPtr, *metadataEnd;
    size_t encodeLen;
    int quantityEncoded, chargeEncoded;
    
    if (!RavenScrollGenesisIsValid(genesis))
        goto cannotEncodeGenesis;
    
    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataMaxLen;
    
//  4-character identifier
    
    if ((metadataPtr+RAVENSCROLL_METADATA_IDENTIFIER_LEN+1)<=metadataEnd) {
        memcpy(metadataPtr, RAVENSCROLL_METADATA_IDENTIFIER, RAVENSCROLL_METADATA_IDENTIFIER_LEN);
        metadataPtr+=RAVENSCROLL_METADATA_IDENTIFIER_LEN;
        *metadataPtr++=RAVENSCROLL_GENESIS_PREFIX;
    } else
        goto cannotEncodeGenesis;
    
//  Quantity mantissa and exponent
    
    quantityEncoded=(genesis->qtyExponent*RAVENSCROLL_GENESIS_QTY_EXPONENT_MULTIPLE+genesis->qtyMantissa)&RAVENSCROLL_GENESIS_QTY_MASK;
    if (genesis->chargeFlatMantissa>0)
        quantityEncoded|=RAVENSCROLL_GENESIS_FLAG_CHARGE_FLAT;
    if (genesis->chargeBasisPoints>0)
        quantityEncoded|=RAVENSCROLL_GENESIS_FLAG_CHARGE_BPS;
    
    if ((metadataPtr+RAVENSCROLL_GENESIS_QTY_FLAGS_LENGTH)<=metadataEnd) {
        if (!WriteSmallEndianUnsigned(quantityEncoded, metadataPtr, RAVENSCROLL_GENESIS_QTY_FLAGS_LENGTH))
            goto cannotEncodeGenesis;
        
        metadataPtr+=RAVENSCROLL_GENESIS_QTY_FLAGS_LENGTH;
    } else
        goto cannotEncodeGenesis;
    
//  Charges - flat and basis points
    
    if (quantityEncoded & RAVENSCROLL_GENESIS_FLAG_CHARGE_FLAT) {
        chargeEncoded=genesis->chargeFlatExponent*RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MULTIPLE+genesis->chargeFlatMantissa;
        
        if ((metadataPtr+RAVENSCROLL_GENESIS_CHARGE_FLAT_LENGTH)<=metadataEnd) {
            if (!WriteSmallEndianUnsigned(chargeEncoded, metadataPtr, RAVENSCROLL_GENESIS_CHARGE_FLAT_LENGTH))
                goto cannotEncodeGenesis;
            
            metadataPtr+=RAVENSCROLL_GENESIS_CHARGE_FLAT_LENGTH;
            
        } else
            goto cannotEncodeGenesis;
    }
    
    if (quantityEncoded & RAVENSCROLL_GENESIS_FLAG_CHARGE_BPS) {
        if ((metadataPtr+RAVENSCROLL_GENESIS_CHARGE_BPS_LENGTH)<=metadataEnd) {
            if (!WriteSmallEndianUnsigned(genesis->chargeBasisPoints, metadataPtr, RAVENSCROLL_GENESIS_CHARGE_BPS_LENGTH))
                goto cannotEncodeGenesis;
            
            metadataPtr+=RAVENSCROLL_GENESIS_CHARGE_BPS_LENGTH;
            
        } else
            goto cannotEncodeGenesis;
    }
    
//  Domain name and page path
    
    encodeLen=EncodeDomainAndOrPath(genesis->domainName, genesis->useHttps, genesis->pagePath, genesis->usePrefix, metadataPtr, metadataEnd, FALSE);
    if (!encodeLen)
        goto cannotEncodeGenesis;
    
    metadataPtr+=encodeLen;
        
//  Asset hash
    
    if ((metadataPtr+genesis->assetHashLen)<=metadataEnd) {
        memcpy(metadataPtr, genesis->assetHash, genesis->assetHashLen);
        metadataPtr+=genesis->assetHashLen;
    } else
        goto cannotEncodeGenesis;
    
//  Return the number of bytes used
    
    return metadataPtr-metadata;
    
    cannotEncodeGenesis:
    return 0;
}

bool RavenScrollGenesisDecode(RavenScrollGenesis* genesis, const char* metadata, const size_t metadataLen)
{
    const char *metadataPtr, *metadataEnd;
    int quantityEncoded, chargeEncoded;
    size_t decodeLen;
    
    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataLen;
    
    if (!LocateMetadataRange(&metadataPtr, &metadataEnd, RAVENSCROLL_GENESIS_PREFIX))
        goto cannotDecodeGenesis;
    
//  Quantity mantissa and exponent
    
    if ((metadataPtr+RAVENSCROLL_GENESIS_QTY_FLAGS_LENGTH)<=metadataEnd) {
        quantityEncoded=(int)ReadSmallEndianUnsigned(metadataPtr, RAVENSCROLL_GENESIS_QTY_FLAGS_LENGTH);
        metadataPtr+=RAVENSCROLL_GENESIS_QTY_FLAGS_LENGTH;
        
        genesis->qtyMantissa=(quantityEncoded&RAVENSCROLL_GENESIS_QTY_MASK)%RAVENSCROLL_GENESIS_QTY_EXPONENT_MULTIPLE;
        genesis->qtyExponent=(quantityEncoded&RAVENSCROLL_GENESIS_QTY_MASK)/RAVENSCROLL_GENESIS_QTY_EXPONENT_MULTIPLE;
        
    } else
        goto cannotDecodeGenesis;
    
//  Charges - flat and basis points
    
    if (quantityEncoded & RAVENSCROLL_GENESIS_FLAG_CHARGE_FLAT) {
        if ((metadataPtr+RAVENSCROLL_GENESIS_CHARGE_FLAT_LENGTH)<=metadataEnd) {
            chargeEncoded=(int)ReadSmallEndianUnsigned(metadataPtr, RAVENSCROLL_GENESIS_CHARGE_FLAT_LENGTH);
            metadataPtr+=RAVENSCROLL_GENESIS_CHARGE_FLAT_LENGTH;
            
            genesis->chargeFlatMantissa=chargeEncoded%RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MULTIPLE;
            genesis->chargeFlatExponent=chargeEncoded/RAVENSCROLL_GENESIS_CHARGE_FLAT_EXPONENT_MULTIPLE;
             
        } else
            goto cannotDecodeGenesis;
        
    } else {
        genesis->chargeFlatMantissa=0;
        genesis->chargeFlatExponent=0;
    }
    
    if (quantityEncoded & RAVENSCROLL_GENESIS_FLAG_CHARGE_BPS) {
        if ((metadataPtr+RAVENSCROLL_GENESIS_CHARGE_BPS_LENGTH)<=metadataEnd) {
            genesis->chargeBasisPoints=(int16_t)ReadSmallEndianUnsigned(metadataPtr, RAVENSCROLL_GENESIS_CHARGE_BPS_LENGTH);
            metadataPtr+=RAVENSCROLL_GENESIS_CHARGE_BPS_LENGTH;
            
        } else
            goto cannotDecodeGenesis;
        
    } else
        genesis->chargeBasisPoints=0;
    
//  Domain name and page path
    
    decodeLen=DecodeDomainAndOrPath(metadataPtr, metadataEnd, genesis->domainName, sizeof(genesis->domainName),
                                    &genesis->useHttps, genesis->pagePath, sizeof(genesis->pagePath), &genesis->usePrefix, FALSE);
    
    if (!decodeLen)
        goto cannotDecodeGenesis;
    
    metadataPtr+=decodeLen;
    
//  Asset hash
    
    genesis->assetHashLen=RAVENSCROLL_MIN(metadataEnd-metadataPtr, RAVENSCROLL_GENESIS_HASH_MAX_LEN); // apply maximum
    memcpy(genesis->assetHash, metadataPtr, genesis->assetHashLen);
    metadataPtr+=genesis->assetHashLen;
    
//  Return validity
    
    return RavenScrollGenesisIsValid(genesis);
    
    cannotDecodeGenesis:
    return FALSE;
}

RavenScrollSatoshiQty RavenScrollGenesisCalcMinFee(const RavenScrollGenesis *genesis, const RavenScrollSatoshiQty* outputsSatoshis,
                                               const bool* outputsRegular, const int countOutputs)
{
    return CountNonLastRegularOutputs(outputsRegular, countOutputs)*GetMinFeeBasis(outputsSatoshis, outputsRegular, countOutputs);
}

void RavenScrollAssetRefClear(RavenScrollAssetRef *assetRef)
{
    assetRef->blockNum=0;
    assetRef->txOffset=0;
    memset(assetRef->txIDPrefix, 0, RAVENSCROLL_ASSETREF_TXID_PREFIX_LEN);
}

static bool RavenScrollAssetRefToStringInner(const RavenScrollAssetRef *assetRef, char* string, const size_t stringMaxLen, bool headers)
{
    char buffer[1024], hex[17], *bufferPtr;
    size_t bufferLength, copyLength;
    
    bufferPtr=buffer;
    
    if (headers)
        bufferPtr+=sprintf(bufferPtr, "RAVENSCROLL ASSET REFERENCE\n");
    
    bufferPtr+=sprintf(bufferPtr, "Genesis block index: %lld (small endian hex %s)\n", (long long)assetRef->blockNum, UnsignedToSmallEndianHex(assetRef->blockNum, 4, hex));
    bufferPtr+=sprintf(bufferPtr, " Genesis txn offset: %lld (small endian hex %s)\n", (long long)assetRef->txOffset, UnsignedToSmallEndianHex(assetRef->txOffset, 4, hex));
    bufferPtr+=sprintf(bufferPtr, "Genesis txid prefix: %s\n", BinaryToHex(assetRef->txIDPrefix, sizeof(assetRef->txIDPrefix), hex));
    
    if (headers)
        bufferPtr+=sprintf(bufferPtr, "END RAVENSCROLL ASSET REFERENCE\n\n");

    bufferLength=bufferPtr-buffer;
    copyLength=RAVENSCROLL_MIN(bufferLength, stringMaxLen-1);
    memcpy(string, buffer, copyLength);
    string[copyLength]=0x00;
    
    return (copyLength==bufferLength);
}

bool RavenScrollAssetRefToString(const RavenScrollAssetRef *assetRef, char* string, const size_t stringMaxLen)
{
    return RavenScrollAssetRefToStringInner(assetRef, string, stringMaxLen, TRUE);
}

bool RavenScrollAssetRefIsValid(const RavenScrollAssetRef *assetRef)
{
    if (assetRef->blockNum!=RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE) {
        if ( (assetRef->blockNum<0) || (assetRef->blockNum>RAVENSCROLL_ASSETREF_BLOCK_NUM_MAX) )
            goto assetRefIsInvalid;
        
        if ( (assetRef->txOffset<0) || (assetRef->txOffset>RAVENSCROLL_ASSETREF_TX_OFFSET_MAX) )
            goto assetRefIsInvalid;
    }

    return TRUE;

    assetRefIsInvalid:
    return FALSE;
}

bool RavenScrollAssetRefMatch(const RavenScrollAssetRef* assetRef1, const RavenScrollAssetRef* assetRef2)
{
    return (!memcmp(assetRef1->txIDPrefix, assetRef2->txIDPrefix, RAVENSCROLL_ASSETREF_TXID_PREFIX_LEN)) &&
    (assetRef1->txOffset==assetRef2->txOffset) &&
    (assetRef1->blockNum==assetRef2->blockNum);
}

size_t RavenScrollAssetRefEncode(const RavenScrollAssetRef *assetRef, char* string, const size_t stringMaxLen)
{
    char buffer[1024];
    size_t bufferLength, copyLength;
    int txIDPrefixInteger;
        
    if (!RavenScrollAssetRefIsValid(assetRef))
        goto cannotEncodeAssetRef;

    txIDPrefixInteger=256*((unsigned char*)assetRef->txIDPrefix)[1]+((unsigned char*)assetRef->txIDPrefix)[0];
    
    bufferLength=sprintf(buffer, "%lld-%lld-%d", (long long)assetRef->blockNum, (long long)assetRef->txOffset, txIDPrefixInteger);
        
    copyLength=RAVENSCROLL_MIN(bufferLength, stringMaxLen-1);
    memcpy(string, buffer, copyLength);
    string[copyLength]=0x00;
    
    return copyLength;
    
    cannotEncodeAssetRef:
    return 0;
}

bool RavenScrollAssetRefDecode(RavenScrollAssetRef *assetRef, const char* string, const size_t stringLen)
{
    char buffer[1024];
    int txIDPrefixInteger;
    long long blockNum, txOffset;
    
    if (stringLen>=sizeof(buffer))
        return FALSE;
    
    memcpy(buffer, string, stringLen);
    buffer[stringLen]=0; // copy to our buffer and null terminate to allow scanf
    
    if (strchr(buffer, '+')) // special check for '+' character which would be accepted by sscanf() below
        return FALSE;
           
    if (sscanf(buffer, "%lld-%lld-%d", &blockNum, &txOffset, &txIDPrefixInteger)!=3)
        return FALSE;
    
    if ( (txIDPrefixInteger<0) || (txIDPrefixInteger>0xFFFF) )
        return FALSE;
    
    assetRef->blockNum=blockNum;
    assetRef->txOffset=txOffset;
    ((unsigned char*)assetRef->txIDPrefix)[0]=txIDPrefixInteger%256;
    ((unsigned char*)assetRef->txIDPrefix)[1]=txIDPrefixInteger/256;
    
    return RavenScrollAssetRefIsValid(assetRef);
}

void RavenScrollTransferClear(RavenScrollTransfer* transfer)
{
    RavenScrollAssetRefClear(&transfer->assetRef);
    transfer->inputs.first=0;
    transfer->inputs.count=0;
    transfer->outputs.first=0;
    transfer->outputs.count=0;
    transfer->qtyPerOutput=0;
}

static bool RavenScrollTransferToStringInner(const RavenScrollTransfer* transfer, char* string, const size_t stringMaxLen, bool headers)
{
    char buffer[1024], assetRefString[256], hex1[17], hex2[17], *bufferPtr;
    size_t bufferLength, copyLength;
    int qtyMantissa, qtyExponent;
    RavenScrollAssetQty encodeQuantity;
    bool isDefaultRoute;
    
    bufferPtr=buffer;

    if (headers)
        bufferPtr+=sprintf(bufferPtr, "RAVENSCROLL TRANSFER\n");

    isDefaultRoute=(transfer->assetRef.blockNum==RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE);
    
    if (isDefaultRoute)
        bufferPtr+=sprintf(bufferPtr, "      Default route:\n");
    
    else {
        RavenScrollAssetRefToStringInner(&transfer->assetRef, bufferPtr, buffer+stringMaxLen-bufferPtr, FALSE);
        bufferPtr+=strlen(bufferPtr);
        
        RavenScrollAssetRefEncode(&transfer->assetRef, assetRefString, sizeof(assetRefString));
        bufferPtr+=sprintf(bufferPtr, "    Asset reference: %s\n", assetRefString);
    }
    
    if (transfer->inputs.count>0) {
        if (transfer->inputs.count>1)
            bufferPtr+=sprintf(bufferPtr, "             Inputs: %d - %d (count %d)", transfer->inputs.first,
                               transfer->inputs.first+transfer->inputs.count-1, transfer->inputs.count);
        else
            bufferPtr+=sprintf(bufferPtr, "              Input: %d", transfer->inputs.first);
    } else
        bufferPtr+=sprintf(bufferPtr, "             Inputs: none");
    
    bufferPtr+=sprintf(bufferPtr, " (small endian hex: first %s count %s)\n", UnsignedToSmallEndianHex(transfer->inputs.first, 2, hex1), UnsignedToSmallEndianHex(transfer->inputs.count, 2, hex2));
    
    if (transfer->outputs.count>0) {
        if ((transfer->outputs.count>1) && !isDefaultRoute)
            bufferPtr+=sprintf(bufferPtr, "            Outputs: %d - %d (count %d)", transfer->outputs.first,
                               transfer->outputs.first+transfer->outputs.count-1, transfer->outputs.count);
        else
            bufferPtr+=sprintf(bufferPtr, "             Output: %d", transfer->outputs.first);
    } else
        bufferPtr+=sprintf(bufferPtr, "            Outputs: none");
    
    bufferPtr+=sprintf(bufferPtr, " (small endian hex: first %s count %s)\n", UnsignedToSmallEndianHex(transfer->outputs.first, 2, hex1), UnsignedToSmallEndianHex(transfer->outputs.count, 2, hex2));
    
    if (!isDefaultRoute) {
        bufferPtr+=sprintf(bufferPtr, "     Qty per output: %lld (small endian hex %s", (long long)transfer->qtyPerOutput,     UnsignedToSmallEndianHex(transfer->qtyPerOutput, 8, hex1));
    
        if (QtyToMantissaExponent(transfer->qtyPerOutput, 0, RAVENSCROLL_TRANSFER_QTY_FLOAT_MANTISSA_MAX, RAVENSCROLL_TRANSFER_QTY_FLOAT_EXPONENT_MAX, &qtyMantissa, &qtyExponent)==transfer->qtyPerOutput) {
            encodeQuantity=(qtyExponent*RAVENSCROLL_TRANSFER_QTY_FLOAT_EXPONENT_MULTIPLE+qtyMantissa)&RAVENSCROLL_TRANSFER_QTY_FLOAT_MASK;
            
            bufferPtr+=sprintf(bufferPtr, ", as float %s", UnsignedToSmallEndianHex(encodeQuantity, RAVENSCROLL_TRANSFER_QTY_FLOAT_LENGTH, hex1));
        }
        
        bufferPtr+=sprintf(bufferPtr, ")\n");
    }
    
    if (headers)
        bufferPtr+=sprintf(bufferPtr, "END RAVENSCROLL TRANSFER\n\n");

    bufferLength=bufferPtr-buffer;
    copyLength=RAVENSCROLL_MIN(bufferLength, stringMaxLen-1);
    memcpy(string, buffer, copyLength);
    string[copyLength]=0x00;
    
    return (copyLength==bufferLength);
}

bool RavenScrollTransferToString(const RavenScrollTransfer* transfer, char* string, const size_t stringMaxLen)
{
    return RavenScrollTransferToStringInner(transfer, string, stringMaxLen, TRUE);
}

bool RavenScrollTransfersToString(const RavenScrollTransfer* transfers, const int countTransfers, char* string, const size_t stringMaxLen)
{
    char *buffer, *bufferPtr;
    int transferIndex;
    size_t bufferLength, copyLength, bufferCapacity;
    int ordering[1024];
    
    bufferCapacity=1024*(1+countTransfers);
    buffer=(char*)malloc(bufferCapacity);
    bufferPtr=buffer;
    
    if (countTransfers>(sizeof(ordering)/sizeof(*ordering)))
        return FALSE;
    
    TransfersGroupOrdering(transfers, ordering, countTransfers);
    
    bufferPtr+=sprintf(bufferPtr, "RAVENSCROLL TRANSFERS\n");
    
    for (transferIndex=0; transferIndex<countTransfers; transferIndex++) {
        if (transferIndex>0)
            bufferPtr+=sprintf(bufferPtr, "\n");
        
        RavenScrollTransferToStringInner(transfers+ordering[transferIndex], bufferPtr, buffer+bufferCapacity-bufferPtr, FALSE);
        bufferPtr+=strlen(bufferPtr);
    }
    
    bufferPtr+=sprintf(bufferPtr, "END RAVENSCROLL TRANSFERS\n\n");
    
    bufferLength=bufferPtr-buffer;
    copyLength=RAVENSCROLL_MIN(bufferLength, stringMaxLen-1);
    memcpy(string, buffer, copyLength);
    string[copyLength]=0x00;
    free(buffer);
    
    return (copyLength==bufferLength);
}

bool RavenScrollTransferMatch(const RavenScrollTransfer* transfer1, const RavenScrollTransfer* transfer2)
{
    bool partialMatch;
    
    partialMatch=(transfer1->inputs.first==transfer2->inputs.first) && (transfer1->inputs.count==transfer2->inputs.count) &&
    (transfer1->outputs.first==transfer2->outputs.first);
    
    if (transfer1->assetRef.blockNum==RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE)
        return (transfer2->assetRef.blockNum==RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE) && partialMatch;
        
    else
        return RavenScrollAssetRefMatch(&transfer1->assetRef, &transfer2->assetRef) && partialMatch &&
            (transfer1->outputs.count==transfer2->outputs.count) && (transfer1->qtyPerOutput==transfer2->qtyPerOutput);
}

bool RavenScrollTransfersMatch(const RavenScrollTransfer* transfers1, const RavenScrollTransfer* transfers2, int countTransfers, bool strict)
{
    int ordering1[1024], ordering2[1024];
    int transferIndex;
    
    if (strict) {
        for (transferIndex=0; transferIndex<countTransfers; transferIndex++)
            if (!RavenScrollTransferMatch(transfers1+transferIndex, transfers2+transferIndex))
                return FALSE;
        
    } else {
        if (countTransfers>(sizeof(ordering1)/sizeof(*ordering1)))
            return FALSE;
        
        TransfersGroupOrdering(transfers1, ordering1, countTransfers);
        TransfersGroupOrdering(transfers2, ordering2, countTransfers);
        
        for (transferIndex=0; transferIndex<countTransfers; transferIndex++)
            if (!RavenScrollTransferMatch(transfers1+ordering1[transferIndex], transfers2+ordering2[transferIndex]))
                return FALSE;
    }
    
    return TRUE;
}

bool RavenScrollTransferIsValid(const RavenScrollTransfer* transfer)
{
    if (!RavenScrollAssetRefIsValid(&transfer->assetRef))
        goto transferIsInvalid;
    
    if ( (transfer->inputs.first<0) || (transfer->inputs.first>RAVENSCROLL_IO_INDEX_MAX) )
        goto transferIsInvalid;
    
    if ( (transfer->inputs.count<0) || (transfer->inputs.count>RAVENSCROLL_IO_INDEX_MAX) )
        goto transferIsInvalid;
    
    if ( (transfer->outputs.first<0) || (transfer->outputs.first>RAVENSCROLL_IO_INDEX_MAX) )
        goto transferIsInvalid;
    
    if ( (transfer->outputs.count<0) || (transfer->outputs.count>RAVENSCROLL_IO_INDEX_MAX) )
        goto transferIsInvalid;
    
    if ( (transfer->qtyPerOutput<0) || (transfer->qtyPerOutput>RAVENSCROLL_ASSET_QTY_MAX) )
        goto transferIsInvalid;
    
    return TRUE;
    
    transferIsInvalid:
    return FALSE;
}

bool RavenScrollTransfersAreValid(const RavenScrollTransfer* transfers, const int countTransfers)
{
    int transferIndex;
    
    for (transferIndex=0; transferIndex<countTransfers; transferIndex++)
        if (!RavenScrollTransferIsValid(transfers+transferIndex))
            return FALSE;
            
    return TRUE;
}

static size_t RavenScrollTransferEncode(const RavenScrollTransfer* transfer, const RavenScrollTransfer* previousTransfer, char* metadata, const size_t metadataMaxLen, const int countInputs, const int countOutputs)
{
    char *metadataPtr, *metadataEnd;
    PackingOptions inputPackingOptions, outputPackingOptions;
    char packing, packingExtendInput, packingExtendOutput, packingExtend;
    PackingByteCounts counts;
    RavenScrollAssetQty encodeQuantity;
    int qtyMantissa, qtyExponent;
    bool isDefaultRoute;
    
    if (!RavenScrollTransferIsValid(transfer))
        goto cannotEncodeTransfer;
    
    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataMaxLen;
    packing=0;
    packingExtend=0;
    isDefaultRoute=(transfer->assetRef.blockNum==RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE);
    
//  Packing for genesis reference
    
    if (isDefaultRoute) {
        if (previousTransfer && (previousTransfer->assetRef.blockNum!=RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE))
            goto cannotEncodeTransfer; // default route transfers have to come at the start
        
        packing|=RAVENSCROLL_PACKING_GENESIS_PREV;
    
    } else {
        
        if (previousTransfer && RavenScrollAssetRefMatch(&transfer->assetRef, &previousTransfer->assetRef))
            packing|=RAVENSCROLL_PACKING_GENESIS_PREV;
     
        else if (transfer->assetRef.blockNum <= RAVENSCROLL_UNSIGNED_3_BYTES_MAX) {
            if (transfer->assetRef.txOffset <= RAVENSCROLL_UNSIGNED_3_BYTES_MAX)
                packing|=RAVENSCROLL_PACKING_GENESIS_3_3_BYTES;
            else if (transfer->assetRef.txOffset <= RAVENSCROLL_UNSIGNED_4_BYTES_MAX)
                packing|=RAVENSCROLL_PACKING_GENESIS_3_4_BYTES;
            else
                goto cannotEncodeTransfer;
        
        } else if ( (transfer->assetRef.blockNum <= RAVENSCROLL_UNSIGNED_4_BYTES_MAX) && (transfer->assetRef.txOffset <= RAVENSCROLL_UNSIGNED_4_BYTES_MAX) )
            packing|=RAVENSCROLL_PACKING_GENESIS_4_4_BYTES;
        
        else
            goto cannotEncodeTransfer;
    }
    
//  Packing for input and output indices
    
    GetPackingOptions(previousTransfer ? &previousTransfer->inputs : NULL, &transfer->inputs, countInputs, inputPackingOptions, FALSE);
    GetPackingOptions(previousTransfer ? &previousTransfer->outputs : NULL, &transfer->outputs, countOutputs, outputPackingOptions, FALSE);
    
    if (inputPackingOptions[_0P] && outputPackingOptions[_0P])
        packing|=RAVENSCROLL_PACKING_INDICES_0P_0P;
    else if (inputPackingOptions[_0P] && outputPackingOptions[_1S])
        packing|=RAVENSCROLL_PACKING_INDICES_0P_1S;
    else if (inputPackingOptions[_0P] && outputPackingOptions[_ALL])
        packing|=RAVENSCROLL_PACKING_INDICES_0P_ALL;
    else if (inputPackingOptions[_1S] && outputPackingOptions[_0P])
        packing|=RAVENSCROLL_PACKING_INDICES_1S_0P;
    else if (inputPackingOptions[_ALL] && outputPackingOptions[_0P])
        packing|=RAVENSCROLL_PACKING_INDICES_ALL_0P;
    else if (inputPackingOptions[_ALL] && outputPackingOptions[_1S])
        packing|=RAVENSCROLL_PACKING_INDICES_ALL_1S;
    else if (inputPackingOptions[_ALL] && outputPackingOptions[_ALL])
        packing|=RAVENSCROLL_PACKING_INDICES_ALL_ALL;
    
    else { // we need the second (extended) packing byte
        packing|=RAVENSCROLL_PACKING_INDICES_EXTEND;
        
        if (!EncodePackingExtend(inputPackingOptions, &packingExtendInput))
            goto cannotEncodeTransfer;
        
        if (!EncodePackingExtend(outputPackingOptions, &packingExtendOutput))
            goto cannotEncodeTransfer;
        
        packingExtend=(packingExtendInput<<RAVENSCROLL_PACKING_EXTEND_INPUTS_SHIFT) | (packingExtendOutput<<RAVENSCROLL_PACKING_EXTEND_OUTPUTS_SHIFT);
    }
    
//  Packing for quantity
    
    encodeQuantity=transfer->qtyPerOutput;
    
    if (transfer->qtyPerOutput==(previousTransfer ? previousTransfer->qtyPerOutput : 1))
        packing|=RAVENSCROLL_PACKING_QUANTITY_1P;
    else if (transfer->qtyPerOutput>=RAVENSCROLL_ASSET_QTY_MAX)
        packing|=RAVENSCROLL_PACKING_QUANTITY_MAX;
    else if (transfer->qtyPerOutput<=RAVENSCROLL_UNSIGNED_BYTE_MAX)
        packing|=RAVENSCROLL_PACKING_QUANTITY_1_BYTE;
    else if (transfer->qtyPerOutput<=RAVENSCROLL_UNSIGNED_2_BYTES_MAX)
        packing|=RAVENSCROLL_PACKING_QUANTITY_2_BYTES;
    else if (QtyToMantissaExponent(transfer->qtyPerOutput, 0, RAVENSCROLL_TRANSFER_QTY_FLOAT_MANTISSA_MAX,
            RAVENSCROLL_TRANSFER_QTY_FLOAT_EXPONENT_MAX, &qtyMantissa, &qtyExponent)==transfer->qtyPerOutput) {
        packing|=RAVENSCROLL_PACKING_QUANTITY_FLOAT;
        encodeQuantity=(qtyExponent*RAVENSCROLL_TRANSFER_QTY_FLOAT_EXPONENT_MULTIPLE+qtyMantissa)&RAVENSCROLL_TRANSFER_QTY_FLOAT_MASK;
    } else if (transfer->qtyPerOutput<=RAVENSCROLL_UNSIGNED_3_BYTES_MAX)
        packing|=RAVENSCROLL_PACKING_QUANTITY_3_BYTES;
    else if (transfer->qtyPerOutput<=RAVENSCROLL_UNSIGNED_4_BYTES_MAX)
        packing|=RAVENSCROLL_PACKING_QUANTITY_4_BYTES;
    else
        packing|=RAVENSCROLL_PACKING_QUANTITY_6_BYTES;

//  Write out the actual data
    
    #define WRITE_BYTE_FIELD(bytes, source) \
        if (bytes>0) { \
            if ( (metadataPtr+(bytes)) <= metadataEnd ) { \
                memcpy(metadataPtr, &source, bytes); \
                metadataPtr+=(bytes); \
            } else \
                goto cannotEncodeTransfer; \
        }
    
    #define WRITE_UNSIGNED_FIELD(bytes, source) \
        if (bytes>0) { \
            if ( (metadataPtr+(bytes)) <= metadataEnd ) { \
                if (!WriteSmallEndianUnsigned(source, metadataPtr, bytes)) \
                    goto cannotEncodeTransfer; \
                metadataPtr+=(bytes); \
            } else \
                goto cannotEncodeTransfer; \
        }
    
    TransferPackingToByteCounts(packing, packingExtend, &counts);
    
    WRITE_BYTE_FIELD(1, packing);
   
    if ((packing & RAVENSCROLL_PACKING_INDICES_MASK)==RAVENSCROLL_PACKING_INDICES_EXTEND)
        WRITE_BYTE_FIELD(1, packingExtend);
    
    WRITE_UNSIGNED_FIELD(counts.blockNumBytes, transfer->assetRef.blockNum);
    WRITE_UNSIGNED_FIELD(counts.txOffsetBytes, transfer->assetRef.txOffset);
    WRITE_BYTE_FIELD(counts.txIDPrefixBytes, transfer->assetRef.txIDPrefix);
    WRITE_UNSIGNED_FIELD(counts.firstInputBytes, transfer->inputs.first);
    WRITE_UNSIGNED_FIELD(counts.countInputsBytes, transfer->inputs.count);
    WRITE_UNSIGNED_FIELD(counts.firstOutputBytes, transfer->outputs.first);
    WRITE_UNSIGNED_FIELD(counts.countOutputsBytes, transfer->outputs.count);
    WRITE_UNSIGNED_FIELD(counts.quantityBytes, encodeQuantity);
    
//  Clear up and return
    
    return metadataPtr-metadata;
    
    cannotEncodeTransfer:
    return 0;
}

size_t RavenScrollTransfersEncode(const RavenScrollTransfer* transfers, const int countTransfers, const int countInputs, const int countOutputs, char* metadata, const size_t metadataMaxLen)
{
    char *metadataPtr, *metadataEnd;
    int transferIndex;
    size_t oneBytesUsed;
    const RavenScrollTransfer *previousTransfer;
    int ordering[1024];
    
    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataMaxLen;
    
    if (countTransfers>(sizeof(ordering)/sizeof(*ordering)))
        return 0; // too many for statically sized array
    
//  4-character identifier
    
    if ((metadataPtr+RAVENSCROLL_METADATA_IDENTIFIER_LEN+1)<=metadataEnd) {
        memcpy(metadataPtr, RAVENSCROLL_METADATA_IDENTIFIER, RAVENSCROLL_METADATA_IDENTIFIER_LEN);
        metadataPtr+=RAVENSCROLL_METADATA_IDENTIFIER_LEN;
        *metadataPtr++=RAVENSCROLL_TRANSFERS_PREFIX;
    } else
        return 0; // return straight away if no space for header

//  Encode each transfer, grouping by asset reference, but preserving original order otherwise
    
    TransfersGroupOrdering(transfers, ordering, countTransfers);
    
    previousTransfer=NULL;
    
    for (transferIndex=0; transferIndex<countTransfers; transferIndex++) {
        oneBytesUsed=RavenScrollTransferEncode(&transfers[ordering[transferIndex]], previousTransfer,
            metadataPtr, metadataEnd-metadataPtr, countInputs, countOutputs);
        
        previousTransfer=&transfers[ordering[transferIndex]];
        
        if (oneBytesUsed>0)
            metadataPtr+=oneBytesUsed;
        else
            return 0;
    }

//  Return number of bytes used
    
    return metadataPtr-metadata;
}

static size_t RavenScrollTransferDecode(const char* metadata, const size_t metadataLen, const RavenScrollTransfer* previousTransfer, RavenScrollTransfer* transfer, const int countInputs, const int countOutputs)
{
    const char *metadataPtr, *metadataEnd;
    char packing, packingExtend;
    PackingType inputPackingType, outputPackingType;
    PackingByteCounts counts;
    RavenScrollAssetQty decodeQuantity;

    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataLen;
    
//  Extract packing
    
    if (metadataPtr<metadataEnd)
        packing=*metadataPtr++;
    else
        goto cannotDecodeTransfer;

//  Packing for genesis reference
    
    switch (packing & RAVENSCROLL_PACKING_GENESIS_MASK)
    {
        case RAVENSCROLL_PACKING_GENESIS_PREV:
            if (previousTransfer)
                transfer->assetRef=previousTransfer->assetRef;

            else { // it's for a default route
                transfer->assetRef.blockNum=RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE;
                transfer->assetRef.txOffset=0;
                memset(&transfer->assetRef.txIDPrefix, 0, sizeof(transfer->assetRef.txIDPrefix));
            }
            break;
    }
    
//  Packing for input and output indices
    
    if ((packing & RAVENSCROLL_PACKING_INDICES_MASK) == RAVENSCROLL_PACKING_INDICES_EXTEND) { // we're using second packing metadata byte
        if (metadataPtr<metadataEnd)
            packingExtend=*metadataPtr++;
        else
            goto cannotDecodeTransfer;
    
        if (!DecodePackingExtend((packingExtend>>RAVENSCROLL_PACKING_EXTEND_INPUTS_SHIFT) & RAVENSCROLL_PACKING_EXTEND_MASK, &inputPackingType, FALSE))
            goto cannotDecodeTransfer;
        
        if (!DecodePackingExtend((packingExtend>>RAVENSCROLL_PACKING_EXTEND_OUTPUTS_SHIFT) & RAVENSCROLL_PACKING_EXTEND_MASK, &outputPackingType, FALSE))
            goto cannotDecodeTransfer;
        
    } else { // not using second packing metadata byte
        packingExtend=0;
        
        switch (packing & RAVENSCROLL_PACKING_INDICES_MASK) // input packing
        {
            case RAVENSCROLL_PACKING_INDICES_0P_0P:
            case RAVENSCROLL_PACKING_INDICES_0P_1S:
            case RAVENSCROLL_PACKING_INDICES_0P_ALL:
                inputPackingType=_0P;
                break;
                
            case RAVENSCROLL_PACKING_INDICES_1S_0P:
                inputPackingType=_1S;
                break;
            
            case RAVENSCROLL_PACKING_INDICES_ALL_0P:
            case RAVENSCROLL_PACKING_INDICES_ALL_1S:
            case RAVENSCROLL_PACKING_INDICES_ALL_ALL:
                inputPackingType=_ALL;
                break;
        }
        
        switch (packing & RAVENSCROLL_PACKING_INDICES_MASK) // output packing
        {
            case RAVENSCROLL_PACKING_INDICES_0P_0P:
            case RAVENSCROLL_PACKING_INDICES_1S_0P:
            case RAVENSCROLL_PACKING_INDICES_ALL_0P:
                outputPackingType=_0P;
                break;
                
            case RAVENSCROLL_PACKING_INDICES_0P_1S:
            case RAVENSCROLL_PACKING_INDICES_ALL_1S:
                outputPackingType=_1S;
                break;
                
            case RAVENSCROLL_PACKING_INDICES_0P_ALL:
            case RAVENSCROLL_PACKING_INDICES_ALL_ALL:
                outputPackingType=_ALL;
                break;
        }
    }
    
//  Final stage of packing for input and output indices
    
    PackingTypeToValues(inputPackingType, previousTransfer ? &previousTransfer->inputs : NULL, countInputs, &transfer->inputs);
    PackingTypeToValues(outputPackingType, previousTransfer ? &previousTransfer->outputs : NULL, countOutputs, &transfer->outputs);
    
//  Read in the fields as appropriate
    
    TransferPackingToByteCounts(packing, packingExtend, &counts);
    
    #define READ_BYTE_FIELD(bytes, destination) \
        if (bytes>0) { \
            if ( (metadataPtr+(bytes)) <= metadataEnd ) { \
                memcpy(&destination, metadataPtr, bytes); \
                metadataPtr+=(bytes); \
            } else \
                goto cannotDecodeTransfer; \
        }
    
    #define READ_UNSIGNED_FIELD(bytes, destination, cast) \
        if (bytes>0) { \
            if ( (metadataPtr+(bytes)) <= metadataEnd ) { \
                (destination)=(cast)ReadSmallEndianUnsigned(metadataPtr, bytes); \
                metadataPtr+=(bytes); \
            } else \
                goto cannotDecodeTransfer; \
        }
    
    READ_UNSIGNED_FIELD(counts.blockNumBytes, transfer->assetRef.blockNum, int64_t);
    READ_UNSIGNED_FIELD(counts.txOffsetBytes, transfer->assetRef.txOffset, int64_t);
    READ_BYTE_FIELD(counts.txIDPrefixBytes, transfer->assetRef.txIDPrefix);
    READ_UNSIGNED_FIELD(counts.firstInputBytes, transfer->inputs.first, RavenScrollIOIndex);
    READ_UNSIGNED_FIELD(counts.countInputsBytes, transfer->inputs.count, RavenScrollIOIndex);
    READ_UNSIGNED_FIELD(counts.firstOutputBytes, transfer->outputs.first, RavenScrollIOIndex);
    READ_UNSIGNED_FIELD(counts.countOutputsBytes, transfer->outputs.count, RavenScrollIOIndex);
    READ_UNSIGNED_FIELD(counts.quantityBytes, decodeQuantity, RavenScrollAssetQty);
    
//  Finish up reading in quantity
    
    switch (packing & RAVENSCROLL_PACKING_QUANTITY_MASK)
    {
        case RAVENSCROLL_PACKING_QUANTITY_1P:
            if (previousTransfer)
                transfer->qtyPerOutput=previousTransfer->qtyPerOutput;
            else
                transfer->qtyPerOutput=1;
            break;
            
        case RAVENSCROLL_PACKING_QUANTITY_MAX:
            transfer->qtyPerOutput=RAVENSCROLL_ASSET_QTY_MAX;
            break;
            
        case RAVENSCROLL_PACKING_QUANTITY_FLOAT:
            decodeQuantity&=RAVENSCROLL_TRANSFER_QTY_FLOAT_MASK;
            transfer->qtyPerOutput=MantissaExponentToQty(decodeQuantity%RAVENSCROLL_TRANSFER_QTY_FLOAT_EXPONENT_MULTIPLE,
                (int)(decodeQuantity/RAVENSCROLL_TRANSFER_QTY_FLOAT_EXPONENT_MULTIPLE));
            break;
        
        default:
            transfer->qtyPerOutput=decodeQuantity;
            break;
    }
    
//  Finish up and return
    
    if (!RavenScrollTransferIsValid(transfer))
        goto cannotDecodeTransfer;
    
    return metadataPtr-metadata;

    cannotDecodeTransfer:
    return 0;
}

int RavenScrollTransfersDecodeCount(const char* metadata, const size_t metadataLen)
{
    return RavenScrollTransfersDecode(NULL, 0, RAVENSCROLL_IO_INDEX_MAX, RAVENSCROLL_IO_INDEX_MAX, metadata, metadataLen);
}

int RavenScrollTransfersDecode(RavenScrollTransfer* transfers, const int maxTransfers, const int countInputs, const int countOutputs,
                             const char* metadata, const size_t metadataLen)
{
    const char *metadataPtr, *metadataEnd;
    RavenScrollTransfer transfer, previousTransfer;
    int countTransfers;
    size_t transferBytesUsed;
    
    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataLen;

    if (!LocateMetadataRange(&metadataPtr, &metadataEnd, RAVENSCROLL_TRANSFERS_PREFIX))
        return 0;

//  Iterate over list
    
    countTransfers=0;
    while (metadataPtr<metadataEnd) {
        transferBytesUsed=RavenScrollTransferDecode(metadataPtr, metadataEnd-metadataPtr, countTransfers ? &previousTransfer : NULL,
                                                  &transfer, countInputs, countOutputs);
        
        if (transferBytesUsed>0) {
            if (countTransfers<maxTransfers)
                transfers[countTransfers]=transfer; // copy across if still space
            
            countTransfers++;
            metadataPtr+=transferBytesUsed;
            previousTransfer=transfer;

        } else
            return 0; // something was invalid
    }
    
//  Return count
    
    return countTransfers;
}

RavenScrollSatoshiQty RavenScrollTransfersCalcMinFee(const RavenScrollTransfer* transfers, const int countTransfers,
                                                 const int countInputs, const int countOutputs,
                                                 const RavenScrollSatoshiQty* outputsSatoshis, const bool* outputsRegular)
{
    int transfersToCover, transferIndex, outputIndex, lastOutputIndex;
    
    transfersToCover=0;
    
    for (transferIndex=0; transferIndex<countTransfers; transferIndex++) {
        if (
            (transfers[transferIndex].assetRef.blockNum != RAVENSCROLL_TRANSFER_BLOCK_NUM_DEFAULT_ROUTE) && // don't count default routes
            (transfers[transferIndex].inputs.count>0) &&
            (transfers[transferIndex].inputs.first<countInputs) // only count if at least one valid input index
        ) {
            outputIndex=RAVENSCROLL_MAX(transfers[transferIndex].outputs.first, 0);
            lastOutputIndex=RAVENSCROLL_MIN(transfers[transferIndex].outputs.first+transfers[transferIndex].outputs.count, countOutputs)-1;
            
            for (; outputIndex<=lastOutputIndex; outputIndex++)
                if (outputsRegular[outputIndex])
                    transfersToCover++;
        }
    }
    
    return transfersToCover*GetMinFeeBasis(outputsSatoshis, outputsRegular, countOutputs);
}

void RavenScrollGenesisApply(const RavenScrollGenesis* genesis, const bool* outputsRegular,
                           RavenScrollAssetQty* outputBalances, const int countOutputs)
{
    int divideOutputs, outputIndex, lastRegularOutput;
    RavenScrollAssetQty genesisQty, qtyPerOutput, extraFirstOutput;
    
    lastRegularOutput=GetLastRegularOutput(outputsRegular, countOutputs);
    divideOutputs=CountNonLastRegularOutputs(outputsRegular, countOutputs);
    genesisQty=RavenScrollGenesisGetQty(genesis);
    
    if (divideOutputs==0)
        qtyPerOutput=0;
    else
        qtyPerOutput=genesisQty/divideOutputs; // rounds down
    
    extraFirstOutput=genesisQty-qtyPerOutput*divideOutputs;

    for (outputIndex=0; outputIndex<countOutputs; outputIndex++)
        if (outputsRegular[outputIndex] && (outputIndex!=lastRegularOutput)) {
            outputBalances[outputIndex]=qtyPerOutput+extraFirstOutput;
            extraFirstOutput=0; // so it will only contribute to the first
        } else
            outputBalances[outputIndex]=0;
}

void RavenScrollTransfersApply(const RavenScrollAssetRef* assetRef, const RavenScrollGenesis* genesis,
                             const RavenScrollTransfer* transfers, const int countTransfers,
                             const RavenScrollAssetQty* inputBalances, const int countInputs,
                             const bool* outputsRegular, RavenScrollAssetQty* outputBalances, const int countOutputs)
{
    int transferIndex, outputIndex, inputIndex, lastInputIndex, lastOutputIndex;
    int *inputDefaultOutput;
    RavenScrollAssetQty *inputsRemaining, transferRemaining, transferQuantity;

//  Copy all input quantities and zero output quantities
    
    inputsRemaining=malloc(countInputs*sizeof(*inputsRemaining));
    for (inputIndex=0; inputIndex<countInputs; inputIndex++)
        inputsRemaining[inputIndex]=inputBalances[inputIndex];
    
    for (outputIndex=0; outputIndex<countOutputs; outputIndex++)
        outputBalances[outputIndex]=0;

//  Perform explicit transfers (i.e. not default routes)
    
    for (transferIndex=0; transferIndex<countTransfers; transferIndex++) {
        if (RavenScrollAssetRefMatch(assetRef, &transfers[transferIndex].assetRef)) { // will exclude default route entries
            inputIndex=RAVENSCROLL_MAX(transfers[transferIndex].inputs.first, 0);
            outputIndex=RAVENSCROLL_MAX(transfers[transferIndex].outputs.first, 0);
            
            lastInputIndex=RAVENSCROLL_MIN(inputIndex+transfers[transferIndex].inputs.count, countInputs)-1;
            lastOutputIndex=RAVENSCROLL_MIN(outputIndex+transfers[transferIndex].outputs.count, countOutputs)-1;
            
            for (; outputIndex<=lastOutputIndex; outputIndex++)
                if (outputsRegular[outputIndex]) {
                    transferRemaining=transfers[transferIndex].qtyPerOutput;
                    
                    while (inputIndex<=lastInputIndex) {
                        transferQuantity=RAVENSCROLL_MIN(transferRemaining, inputsRemaining[inputIndex]);
                        
                        if (transferQuantity>0) { // skip all this if nothing is to be transferred (branch not really necessary)
                            inputsRemaining[inputIndex]-=transferQuantity;
                            transferRemaining-=transferQuantity;
                            outputBalances[outputIndex]+=transferQuantity;
                        }
                        
                        if (transferRemaining>0)
                            inputIndex++; // move to next input since this one is drained
                        else
                            break; // stop if we have nothing left to transfer
                    }
                }
        }
    }

//  Apply payment charges to all quantities not routed by default
    
    for (outputIndex=0; outputIndex<countOutputs; outputIndex++)
        if (outputsRegular[outputIndex])
            outputBalances[outputIndex]=RavenScrollGenesisCalcNet(genesis, outputBalances[outputIndex]);
    
//  Send remaining quantities to default outputs
    
    inputDefaultOutput=(int*)malloc(sizeof(*inputDefaultOutput)*countInputs);
    GetDefaultRouteMap(transfers, countTransfers, countInputs, countOutputs, outputsRegular, inputDefaultOutput);
    
    for (inputIndex=0; inputIndex<countInputs; inputIndex++) {
        outputIndex=inputDefaultOutput[inputIndex];
        
        if (outputIndex<countOutputs) // could be out of range if there are no regular outputs
            outputBalances[outputIndex]+=inputsRemaining[inputIndex];
    }
    
    free(inputDefaultOutput);
    free(inputsRemaining);
}

void RavenScrollTransfersApplyNone(const RavenScrollAssetRef* assetRef, const RavenScrollGenesis* genesis,
                              const RavenScrollAssetQty* inputBalances, const int countInputs,
                              const bool* outputsRegular, RavenScrollAssetQty* outputBalances, const int countOutputs)
{
    RavenScrollTransfersApply(assetRef, genesis, NULL, 0, inputBalances, countInputs, outputsRegular, outputBalances, countOutputs);
}

void RavenScrollTransfersDefaultOutputs(const RavenScrollTransfer* transfers, const int countTransfers, const int countInputs,
                                const bool* outputsRegular, bool* outputsDefault, const int countOutputs)
{
    int *inputDefaultOutput, outputIndex, inputIndex;

    for (outputIndex=0; outputIndex<countOutputs; outputIndex++)
        outputsDefault[outputIndex]=FALSE;
    
    inputDefaultOutput=(int*)malloc(sizeof(*inputDefaultOutput)*countInputs);
    GetDefaultRouteMap(transfers, countTransfers, countInputs, countOutputs, outputsRegular, inputDefaultOutput);
    
    for (inputIndex=0; inputIndex<countInputs; inputIndex++) {
        outputIndex=inputDefaultOutput[inputIndex];
        
        if (outputIndex<countOutputs)
            outputsDefault[outputIndex]=TRUE;
    }
    
    free(inputDefaultOutput);
}

bool RavenScrollPaymentRefToString(const RavenScrollPaymentRef paymentRef, char* string, const size_t stringMaxLen)
{
    char buffer[1024], hex[17], *bufferPtr;
    size_t bufferLength, copyLength;
    
    bufferPtr=buffer;

    bufferPtr+=sprintf(bufferPtr, "RAVENSCROLL PAYMENT REFERENCE\n");
    bufferPtr+=sprintf(bufferPtr, "%lld (small endian hex %s)\n", (long long)paymentRef, UnsignedToSmallEndianHex(paymentRef, 8, hex));
    bufferPtr+=sprintf(bufferPtr, "END RAVENSCROLL PAYMENT REFERENCE\n\n");
    
    bufferLength=bufferPtr-buffer;
    copyLength=RAVENSCROLL_MIN(bufferLength, stringMaxLen-1);
    memcpy(string, buffer, copyLength);
    string[copyLength]=0x00;
    
    return (copyLength==bufferLength);
}

bool RavenScrollPaymentRefIsValid(const RavenScrollPaymentRef paymentRef)
{
    return (paymentRef>=0) && (paymentRef<=RAVENSCROLL_PAYMENT_REF_MAX);
}

RavenScrollPaymentRef RavenScrollPaymentRefRandom()
{
    RavenScrollPaymentRef paymentRef;
    long long bitsRemaining;
    
    paymentRef=0;
    
    for (bitsRemaining=RAVENSCROLL_PAYMENT_REF_MAX; bitsRemaining>0; bitsRemaining>>=13) {
        paymentRef<<=13;
        paymentRef|=rand()&0x1FFF;
    }
    
    return paymentRef % (1+RAVENSCROLL_PAYMENT_REF_MAX);
}

size_t RavenScrollPaymentRefEncode(const RavenScrollPaymentRef paymentRef, char* metadata, const size_t metadataMaxLen)
{
    long long paymentLeft;
    size_t bytes;
    char *metadataPtr, *metadataEnd;

    if (!RavenScrollPaymentRefIsValid(paymentRef))
        goto cannotEncodePaymentRef;

    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataMaxLen;

//  4-character identifier
    
    if ((metadataPtr+RAVENSCROLL_METADATA_IDENTIFIER_LEN+1)<=metadataEnd) {
        memcpy(metadataPtr, RAVENSCROLL_METADATA_IDENTIFIER, RAVENSCROLL_METADATA_IDENTIFIER_LEN);
        metadataPtr+=RAVENSCROLL_METADATA_IDENTIFIER_LEN;
        *metadataPtr++=RAVENSCROLL_PAYMENTREF_PREFIX;

    } else
        goto cannotEncodePaymentRef;

//  The payment reference

    bytes=0;
    for (paymentLeft=paymentRef; paymentLeft>0; paymentLeft>>=8)
        bytes++;
    
    if ((metadataPtr+bytes)<=metadataEnd) {
        WriteSmallEndianUnsigned(paymentRef, metadataPtr, bytes);
        metadataPtr+=bytes;
        
    } else
        goto cannotEncodePaymentRef;
    
//  Return the number of bytes used
    
    return metadataPtr-metadata;
    
    cannotEncodePaymentRef:
    return 0;
}

bool RavenScrollPaymentRefDecode(RavenScrollPaymentRef* paymentRef, const char* metadata, const size_t metadataLen)
{
    const char *metadataPtr, *metadataEnd;
    size_t finalMetadataLen;
    
    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataLen;
    
    if (!LocateMetadataRange(&metadataPtr, &metadataEnd, RAVENSCROLL_PAYMENTREF_PREFIX))
        goto cannotDecodePaymentRef;

//  The payment reference
    
    finalMetadataLen=metadataEnd-metadataPtr;
    if (finalMetadataLen>8)
        goto cannotDecodePaymentRef;
    
    *paymentRef=ReadSmallEndianUnsigned(metadataPtr, finalMetadataLen);

//  Return validity
    
    return RavenScrollPaymentRefIsValid(*paymentRef);
    
    cannotDecodePaymentRef:
    return FALSE;
}

void RavenScrollMessageClear(RavenScrollMessage* message)
{
    message->useHttps=FALSE;
    message->serverHost[0]=0x00;
    message->usePrefix=FALSE;
    message->serverPath[0]=0x00;
    message->isPublic=FALSE;
    message->countOutputRanges=0;
    message->hashLen=0;
}

bool RavenScrollMessageToString(const RavenScrollMessage* message, char* string, const size_t stringMaxLen)
{
    char buffer[4096], urlString[256], hex1[128], hex2[128], *bufferPtr;
    size_t bufferLength, copyLength, hostPathEncodeLen;
    char hostPathMetadata[64];
    int outputRangeIndex;
    const RavenScrollIORange *outputRange;
    
    bufferPtr=buffer;
    
    hostPathEncodeLen=EncodeDomainAndOrPath(message->serverHost, message->useHttps, message->serverPath, message->usePrefix,
                                            hostPathMetadata, hostPathMetadata+sizeof(hostPathMetadata), TRUE);
    RavenScrollMessageCalcServerURL(message, urlString, sizeof(urlString));
    
    bufferPtr+=sprintf(bufferPtr, "RAVENSCROLL MESSAGE\n");
    bufferPtr+=sprintf(bufferPtr, "    Server URL: %s (length %zd+%zd encoded %s length %zd)\n",
        urlString, strlen(message->serverHost), strlen(message->serverPath),
        BinaryToHex(hostPathMetadata, hostPathEncodeLen, hex1), hostPathEncodeLen);
    bufferPtr+=sprintf(bufferPtr, "Public message: %s\n", message->isPublic ? "yes" : "no");
    
    for (outputRangeIndex=0; outputRangeIndex<message->countOutputRanges; outputRangeIndex++) {
        outputRange=message->outputRanges+outputRangeIndex;
        
    	if (outputRange->count>0) {
			if (outputRange->count>1)
				bufferPtr+=sprintf(bufferPtr, "       Outputs: %d - %d (count %d)", outputRange->first,
								   outputRange->first+outputRange->count-1, outputRange->count);
			else
				bufferPtr+=sprintf(bufferPtr, "        Output: %d", outputRange->first);
		} else
			bufferPtr+=sprintf(bufferPtr, "       Outputs: none");
        
        bufferPtr+=sprintf(bufferPtr, " (small endian hex: first %s count %s)\n", UnsignedToSmallEndianHex(outputRange->first, 2, hex1), UnsignedToSmallEndianHex(outputRange->count, 2, hex2));
    }
    
    bufferPtr+=sprintf(bufferPtr, "  Message hash: %s (length %zd)\n", BinaryToHex(message->hash, message->hashLen, hex1), message->hashLen);
    bufferPtr+=sprintf(bufferPtr, "END RAVENSCROLL MESSAGE\n\n");

    bufferLength=bufferPtr-buffer;
    copyLength=RAVENSCROLL_MIN(bufferLength, stringMaxLen-1);
    memcpy(string, buffer, copyLength);
    string[copyLength]=0x00;
    
    return (copyLength==bufferLength);
}

bool RavenScrollMessageIsValid(const RavenScrollMessage* message)
{
    int outputRangeIndex;
    const RavenScrollIORange *outputRange;
    
    if (strlen(message->serverHost)>RAVENSCROLL_MESSAGE_SERVER_HOST_MAX_LEN)
        goto messageIsInvalid;
    
    if (strlen(message->serverPath)>RAVENSCROLL_MESSAGE_SERVER_PATH_MAX_LEN)
        goto messageIsInvalid;
    
    if ( (message->hashLen<RAVENSCROLL_MESSAGE_HASH_MIN_LEN) || (message->hashLen>RAVENSCROLL_MESSAGE_HASH_MAX_LEN) )
        goto messageIsInvalid;

    if ( (!message->isPublic) && (message->countOutputRanges<1) ) // public or aimed at some outputs at least
        goto messageIsInvalid;
    
    if ( (message->countOutputRanges<0) || (message->countOutputRanges>RAVENSCROLL_MESSAGE_MAX_IO_RANGES) )
        goto messageIsInvalid;
    
    for (outputRangeIndex=0; outputRangeIndex<message->countOutputRanges; outputRangeIndex++) {
        outputRange=message->outputRanges+outputRangeIndex;
        
        if ( (outputRange->first<0) || (outputRange->first>RAVENSCROLL_IO_INDEX_MAX) )
            goto messageIsInvalid;
        
        if ( (outputRange->count<0) || (outputRange->count>RAVENSCROLL_IO_INDEX_MAX) )
            goto messageIsInvalid;
    }
    
    return TRUE;
    
    messageIsInvalid:
    return FALSE;
}

bool RavenScrollMessageMatch(const RavenScrollMessage* message1, const RavenScrollMessage* message2, const bool strict)
{
    size_t hashCompareLen;
    int compareRangeIndex, countCompareRanges1, countCompareRanges2;
    RavenScrollIORange *normalizedRanges1, *normalizedRanges2;
    const RavenScrollIORange *compareRanges1, *compareRanges2;
    bool outputRangesMatch;

    hashCompareLen=RAVENSCROLL_MIN(message1->hashLen, message2->hashLen);
    hashCompareLen=RAVENSCROLL_MIN(hashCompareLen, RAVENSCROLL_MESSAGE_HASH_MAX_LEN);
    
    if (strict) {
        compareRanges1=message1->outputRanges;
        compareRanges2=message2->outputRanges;
        
        countCompareRanges1=message1->countOutputRanges;
        countCompareRanges2=message2->countOutputRanges;
        
    } else {
        compareRanges1=normalizedRanges1=(RavenScrollIORange*)malloc(message1->countOutputRanges*sizeof(*normalizedRanges1));
        compareRanges2=normalizedRanges2=(RavenScrollIORange*)malloc(message2->countOutputRanges*sizeof(*normalizedRanges2));
        
        countCompareRanges1=NormalizeIORanges(message1->outputRanges, normalizedRanges1, message1->countOutputRanges);
        countCompareRanges2=NormalizeIORanges(message2->outputRanges, normalizedRanges2, message2->countOutputRanges);
    }
    
    outputRangesMatch=(countCompareRanges1==countCompareRanges2);
    
    if (outputRangesMatch)
        for (compareRangeIndex=0; compareRangeIndex<countCompareRanges1; compareRangeIndex++)
            if ((compareRanges1[compareRangeIndex].first!=compareRanges2[compareRangeIndex].first) ||
                (compareRanges1[compareRangeIndex].count!=compareRanges2[compareRangeIndex].count)) {
                outputRangesMatch=FALSE;
                break;
            }
    
    if (!strict) {
        free(normalizedRanges1);
        free(normalizedRanges2);
    }

    return outputRangesMatch && (message1->useHttps==message2->useHttps) &&
        (!strcasecmp(message1->serverHost, message2->serverHost)) &&
        (message1->usePrefix==message2->usePrefix) &&
        (!strcasecmp(message1->serverPath, message2->serverPath)) &&
        (message1->isPublic==message2->isPublic) &&
        (!memcmp(&message1->hash, &message2->hash, hashCompareLen));
    
}

size_t RavenScrollMessageEncode(const RavenScrollMessage* message, const int countOutputs, char* metadata, const size_t metadataMaxLen)
{
    char* metadataPtr, *metadataEnd;
    size_t encodeLen, firstBytes, countBytes;
    char packing;
    int outputRangeIndex;
    const RavenScrollIORange *outputRange;
    
    if (!RavenScrollMessageIsValid(message))
        goto cannotEncodeMessage;
    
    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataMaxLen;

//  4-character identifier
    
    if ((metadataPtr+RAVENSCROLL_METADATA_IDENTIFIER_LEN+1)<=metadataEnd) {
        memcpy(metadataPtr, RAVENSCROLL_METADATA_IDENTIFIER, RAVENSCROLL_METADATA_IDENTIFIER_LEN);
        metadataPtr+=RAVENSCROLL_METADATA_IDENTIFIER_LEN;
        *metadataPtr++=RAVENSCROLL_MESSAGE_PREFIX;
    } else
        goto cannotEncodeMessage;

//  Server host and path
    
    encodeLen=EncodeDomainAndOrPath(message->serverHost, message->useHttps, message->serverPath, message->usePrefix, metadataPtr, metadataEnd, TRUE);
    if (!encodeLen)
        goto cannotEncodeMessage;
    
    metadataPtr+=encodeLen;
    
//  Output ranges
    
    if (message->isPublic) { // add public indicator first
        packing=((message->countOutputRanges>0) ? RAVENSCROLL_OUTPUTS_MORE_FLAG : 0)|RAVENSCROLL_OUTPUTS_TYPE_EXTEND|RAVENSCROLL_PACKING_EXTEND_PUBLIC;
        
        if (metadataPtr<metadataEnd)
            *metadataPtr++=packing;
        else
            goto cannotEncodeMessage;
    }
    
    for (outputRangeIndex=0; outputRangeIndex<message->countOutputRanges; outputRangeIndex++) { // other output ranges
        outputRange=message->outputRanges+outputRangeIndex;
        
        if (!GetMessageOutputRangePacking(outputRange, countOutputs, &packing, &firstBytes, &countBytes))
            goto cannotEncodeMessage;
        
    //  The packing byte
        
        if ((outputRangeIndex+1)<message->countOutputRanges)
            packing|=RAVENSCROLL_OUTPUTS_MORE_FLAG;

        if (metadataPtr<metadataEnd)
            *metadataPtr++=packing;
        else
            goto cannotEncodeMessage;
        
    //  The index of the first output, if necessary
        
        if (firstBytes>0) {
            if ((metadataPtr+firstBytes)<=metadataEnd) {
                if (!WriteSmallEndianUnsigned(outputRange->first, metadataPtr, firstBytes))
                    goto cannotEncodeMessage;
                metadataPtr+=firstBytes;
            } else
                goto cannotEncodeMessage;
        }
        
    //  The number of outputs, if necessary
        
        if (countBytes>0) {
            if ((metadataPtr+countBytes)<=metadataEnd) {
                if (!WriteSmallEndianUnsigned(outputRange->count, metadataPtr, countBytes))
                    goto cannotEncodeMessage;
                metadataPtr+=countBytes;
            } else
                goto cannotEncodeMessage;
        }
    }

//  Message hash

    if ((metadataPtr+message->hashLen)<=metadataEnd) {
        memcpy(metadataPtr, message->hash, message->hashLen);
        metadataPtr+=message->hashLen;
    } else
        goto cannotEncodeMessage;
 
//  Return the number of bytes used
    
    return metadataPtr-metadata;
    
    cannotEncodeMessage:
    return 0;
}

bool RavenScrollMessageDecode(RavenScrollMessage* message, const int countOutputs, const char* metadata, const size_t metadataLen)
{
    const char *metadataPtr, *metadataEnd;
    size_t decodeLen, firstBytes, countBytes;
    char packing, packingType, packingValue;
    RavenScrollIORange *outputRange;
    PackingType extendPackingType;
    
    metadataPtr=metadata;
    metadataEnd=metadataPtr+metadataLen;
    
    if (!LocateMetadataRange(&metadataPtr, &metadataEnd, RAVENSCROLL_MESSAGE_PREFIX))
        goto cannotDecodeMessage;

//  Server host and path
    
    decodeLen=DecodeDomainAndOrPath(metadataPtr, metadataEnd, message->serverHost, sizeof(message->serverHost),
                                    &message->useHttps, message->serverPath, sizeof(message->serverPath), &message->usePrefix, TRUE);
    
    if (!decodeLen)
        goto cannotDecodeMessage;
    
    metadataPtr+=decodeLen;
    
//  Output ranges
    
    message->isPublic=FALSE;
    message->countOutputRanges=0;
    
    do {
        
    //  Read the next packing byte and check reserved bits are zero
        
        if (metadataPtr<metadataEnd)
            packing=*metadataPtr++;
        else
            goto cannotDecodeMessage;
     
        if (packing&RAVENSCROLL_OUTPUTS_RESERVED_MASK)
            goto cannotDecodeMessage;
        
        packingType=packing & RAVENSCROLL_OUTPUTS_TYPE_MASK;
        packingValue=packing & RAVENSCROLL_OUTPUTS_VALUE_MASK;
        
        if ((packingType==RAVENSCROLL_OUTPUTS_TYPE_EXTEND) && (packingValue==RAVENSCROLL_PACKING_EXTEND_PUBLIC))
            message->isPublic=TRUE; // special case for public messages
        
        else {
        
        //  Create a new output range
            
            if (message->countOutputRanges>=RAVENSCROLL_MESSAGE_MAX_IO_RANGES) // too many output ranges
                goto cannotDecodeMessage;
            
            outputRange=message->outputRanges+message->countOutputRanges;
            message->countOutputRanges++;
            
            firstBytes=0;
            countBytes=0;
            
        //  Decode packing byte
            
            if (packingType==RAVENSCROLL_OUTPUTS_TYPE_SINGLE) { // inline single input
                outputRange->first=packingValue;
                outputRange->count=1;
           
            } else if (packingType==RAVENSCROLL_OUTPUTS_TYPE_FIRST) { // inline first few outputs
                outputRange->first=0;
                outputRange->count=packingValue;
                
            } else if (packingType==RAVENSCROLL_OUTPUTS_TYPE_EXTEND) { // we'll be taking additional bytes
                if (!DecodePackingExtend(packingValue, &extendPackingType, TRUE))
                    goto cannotDecodeMessage;
                
                PackingTypeToValues(extendPackingType, NULL, countOutputs, outputRange);
                PackingExtendAddByteCounts(packingValue, &firstBytes, &countBytes, TRUE);
                
            } else
                goto cannotDecodeMessage; // will be RAVENSCROLL_OUTPUTS_TYPE_UNUSED
            
        //  The index of the first output, if necessary
            
            if (firstBytes>0) {
                if ((metadataPtr+firstBytes)<=metadataEnd) {
                    outputRange->first=(RavenScrollIOIndex)ReadSmallEndianUnsigned(metadataPtr, firstBytes);
                    metadataPtr+=firstBytes;
                } else
                    goto cannotDecodeMessage;
            }
            
        //  The number of outputs, if necessary
            
            if (countBytes>0) {
                if ((metadataPtr+countBytes)<=metadataEnd) {
                    outputRange->count=(RavenScrollIOIndex)ReadSmallEndianUnsigned(metadataPtr, countBytes);
                    metadataPtr+=countBytes;
                } else
                    goto cannotDecodeMessage;
            }
        }
        
    } while
        (packing&RAVENSCROLL_OUTPUTS_MORE_FLAG);
    
//  Message hash
    
    message->hashLen=RAVENSCROLL_MIN(metadataEnd-metadataPtr, RAVENSCROLL_MESSAGE_HASH_MAX_LEN); // apply maximum
    memcpy(message->hash, metadataPtr, message->hashLen);
    metadataPtr+=message->hashLen;

//  Return validity
    
    return RavenScrollMessageIsValid(message);
    
    cannotDecodeMessage:
    return FALSE;
}

bool RavenScrollMessageHasOutput(const RavenScrollMessage* message, int outputIndex)
{
    int outputRangeIndex;
    const RavenScrollIORange *outputRange;
    
    for (outputRangeIndex=0; outputRangeIndex<message->countOutputRanges; outputRangeIndex++) {
        outputRange=message->outputRanges+outputRangeIndex;
        
        if ( (outputIndex>=outputRange->first) && (outputIndex<(outputRange->first+outputRange->count)) )
            return TRUE;
    }

    return FALSE;
}

size_t RavenScrollMessageCalcHashLen(const RavenScrollMessage *message, int countOutputs, const size_t metadataMaxLen)
{
    size_t hostPathLen, firstBytes, countBytes;
    int hashLen, outputRangeIndex;
    u_int8_t octets[4];
    char hostName[256], packing;
    
    hashLen=(int)metadataMaxLen-RAVENSCROLL_METADATA_IDENTIFIER_LEN-1;
    
    hostPathLen=strlen(message->serverPath)+1;
    
    if (ReadIPv4Address(message->serverHost, octets)) {
        hashLen-=5; // packing and IP octets
        if (hostPathLen==1)
            hostPathLen=0; // will skip server path in this case
    } else {
        hashLen-=1; // packing
        hostPathLen+=ShrinkLowerDomainName(message->serverHost, strlen(message->serverHost), hostName, sizeof(hostName), &packing)+1;
    }
    
    hashLen-=2*((hostPathLen+2)/3); // uses integer arithmetic
    
    if (message->isPublic)
        hashLen--;
    
    for (outputRangeIndex=0; outputRangeIndex<message->countOutputRanges; outputRangeIndex++)// other output ranges
        if (GetMessageOutputRangePacking(message->outputRanges+outputRangeIndex, countOutputs, &packing, &firstBytes, &countBytes))
            hashLen-=(1+firstBytes+countBytes);
    
    return RAVENSCROLL_MIN(RAVENSCROLL_MAX(hashLen, 0), RAVENSCROLL_MESSAGE_HASH_MAX_LEN);
}

size_t RavenScrollGenesisCalcAssetURL(const RavenScrollGenesis *genesis, const char* firstSpentTxID, const int firstSpentVout, char* urlString, const size_t urlStringMaxLen)
{
    char firstSpentTxIdPart[17], fullURL[256];
    int charIndex;
    size_t fullURLLen;
    
    for (charIndex=0; charIndex<16; charIndex++)
        firstSpentTxIdPart[charIndex]=firstSpentTxID[(charIndex+firstSpentVout)%64];
    firstSpentTxIdPart[16]=0; // C null string terminator
    
    sprintf(fullURL, "%s://%s/%s%s/", genesis->useHttps ? "https" : "http", genesis->domainName, genesis->usePrefix ? "ravenscroll/" : "",
            genesis->pagePath[0] ? genesis->pagePath : firstSpentTxIdPart);
    fullURLLen=strlen(fullURL);
    
    if (fullURLLen<urlStringMaxLen) { // allow for C null terminator
        for (charIndex=0; charIndex<=fullURLLen; charIndex++)
            urlString[charIndex]=tolower(fullURL[charIndex]);

        return fullURLLen;

    } else
        return 0;
}

size_t RavenScrollMessageCalcServerURL(const RavenScrollMessage* message, char* urlString, const size_t urlStringMaxLen)
{
    char fullURL[256];
    int charIndex;
    size_t fullURLLen;
    
    sprintf(fullURL, "%s://%s/%s%s%s", message->useHttps ? "https" : "http", message->serverHost, message->usePrefix ? "ravenscroll/" : "",
            message->serverPath, message->serverPath[0] ? "/" : "");
    fullURLLen=strlen(fullURL);

    if (fullURLLen<urlStringMaxLen) { // allow for C null terminator
        for (charIndex=0; charIndex<=fullURLLen; charIndex++)
            urlString[charIndex]=tolower(fullURL[charIndex]);
        
        return fullURLLen;
        
    } else
        return 0;
}

#define ADD_HASH_BUFFER_STRING(bufferPtr, string, stringLen) \
    if ((string) && (stringLen)) { \
        memcpy(bufferPtr, string, stringLen); \
        (bufferPtr)+=(stringLen); \
    } \
    *(bufferPtr)++=0x00;
    
void RavenScrollCalcAssetHash(const char* name, size_t nameLen,
                            const char* issuer, size_t issuerLen,
                            const char* description, size_t descriptionLen,
                            const char* units, size_t unitsLen,
                            const char* issueDate, size_t issueDateLen,
                            const char* expiryDate, size_t expiryDateLen,
                            const double* interestRate, const double* multiple,
                            const char* contractContent, const size_t contractContentLen,
                            unsigned char assetHash[32])
{
    char *buffer, *bufferPtr;
    long long interestRateToHash, multipleToHash;
    bool keepTrimming;
    
    buffer=malloc(nameLen+issuerLen+descriptionLen+issueDateLen+expiryDateLen+contractContentLen+1024);
    bufferPtr=buffer;
    
    #define TRIM_STRING(string, stringLen) \
        for (keepTrimming=TRUE; keepTrimming && (stringLen>0); ) \
            switch (string[0]) { \
                case 0x09: case 0x0A: case 0x0D: case 0x20: \
                    string++; \
                    stringLen--; \
                    break; \
                default: \
                    keepTrimming=FALSE; \
                    break; \
            } \
        for (keepTrimming=TRUE; keepTrimming && (stringLen>0); ) \
            switch (string[stringLen-1]) { \
                case 0x09: case 0x0A: case 0x0D: case 0x20: \
                    stringLen--; \
                    break; \
                default: \
                    keepTrimming=FALSE; \
                    break; \
            }

    TRIM_STRING(name, nameLen);
    TRIM_STRING(issuer, issuerLen);
    TRIM_STRING(description, descriptionLen);
    TRIM_STRING(units, unitsLen);
    TRIM_STRING(issueDate, issueDateLen);
    TRIM_STRING(expiryDate, expiryDateLen);

    ADD_HASH_BUFFER_STRING(bufferPtr, name, nameLen);
    ADD_HASH_BUFFER_STRING(bufferPtr, issuer, issuerLen);
    ADD_HASH_BUFFER_STRING(bufferPtr, description, descriptionLen);
    ADD_HASH_BUFFER_STRING(bufferPtr, units, unitsLen);
    ADD_HASH_BUFFER_STRING(bufferPtr, issueDate, issueDateLen);
    ADD_HASH_BUFFER_STRING(bufferPtr, expiryDate, expiryDateLen);
    
    interestRateToHash=(long long)((interestRate ? *interestRate : 0)*1000000.0+0.5);
    multipleToHash=(long long)((multiple ? *multiple : 1)*1000000.0+0.5);

    bufferPtr+=1+sprintf(bufferPtr, "%lld", (long long)interestRateToHash);
    bufferPtr+=1+sprintf(bufferPtr, "%lld", (long long)multipleToHash);
    
    ADD_HASH_BUFFER_STRING(bufferPtr, contractContent, contractContentLen);
    
    RavenScrollCalcSHA256Hash((unsigned char*)buffer, bufferPtr-buffer, assetHash);
    
    free(buffer);
}

void RavenScrollCalcMessageHash(const unsigned char* salt, size_t saltLen, const RavenScrollMessagePart* messageParts,
                              const int countParts, unsigned char messageHash[32])
{
    size_t bufferLen;
    int partIndex;
    char *buffer, *bufferPtr;
    
    bufferLen=saltLen+16;
    for (partIndex=0; partIndex<countParts; partIndex++)
        bufferLen+=messageParts[partIndex].mimeTypeLen+messageParts[partIndex].contentLen+
            (messageParts[partIndex].fileName ? messageParts[partIndex].fileNameLen : 0)+16;
    
    buffer=malloc(bufferLen);
    bufferPtr=buffer;
    
    ADD_HASH_BUFFER_STRING(bufferPtr, salt, saltLen);
    
    for (partIndex=0; partIndex<countParts; partIndex++) {
        ADD_HASH_BUFFER_STRING(bufferPtr, messageParts[partIndex].mimeType, messageParts[partIndex].mimeTypeLen);
        ADD_HASH_BUFFER_STRING(bufferPtr, messageParts[partIndex].fileName, messageParts[partIndex].fileNameLen);
        ADD_HASH_BUFFER_STRING(bufferPtr,messageParts[partIndex].content, messageParts[partIndex].contentLen);
    }
    
    RavenScrollCalcSHA256Hash((unsigned char*)buffer, bufferPtr-buffer, messageHash);
    
    free(buffer);
}

char* RavenScrollGetGenesisWebPageURL(const char* scriptPubKeys[], const size_t scriptPubKeysLen[], const int countOutputs, u_int8_t firstSpentTxId[32], const int firstSpentVout)
{
    char firstSpentTxIdString[65], metadata[400];
    size_t metadataLen;
    char* webPageURL;
    RavenScrollGenesis genesis;
    
    webPageURL=NULL;
   
    BinaryToHex(firstSpentTxId, 32, firstSpentTxIdString);
    
    metadataLen=RavenScrollScriptsToMetadata(scriptPubKeys, scriptPubKeysLen, FALSE, countOutputs, metadata, sizeof(metadata));
    if (!metadataLen)
        goto returnWebPageURL;
    
    if (!RavenScrollGenesisDecode(&genesis, metadata, metadataLen))
        goto returnWebPageURL;
    
    webPageURL=malloc(1024);
    RavenScrollGenesisCalcAssetURL(&genesis, firstSpentTxIdString, firstSpentVout, webPageURL, 1024);
    
    returnWebPageURL:
    return webPageURL; // called must call free() on the result but anyway this is for contract only
}


RavenScrollAssetQty RavenScrollGetGenesisOutputQty(const char* scriptPubKeys[], const size_t scriptPubKeysLen[],
                                               const RavenScrollSatoshiQty* outputsSatoshis, const int countOutputs,
                                               const RavenScrollSatoshiQty transactionFee, const int getOutputIndex)
{
    RavenScrollAssetQty outputQty, *outputBalances;
    char metadata[400];
    size_t metadataLen;
    RavenScrollGenesis genesis;
    RavenScrollSatoshiQty minValidFee;
    int outputIndex;
    bool *outputsRegular;
   
//  Default values for the end of this function
    
    outputQty=0;
    outputsRegular=NULL;
    outputBalances=NULL;

//  Decode the metadata

    metadataLen=RavenScrollScriptsToMetadata(scriptPubKeys, scriptPubKeysLen, FALSE, countOutputs, metadata, sizeof(metadata));
    if (!metadataLen)
        goto returnOutputQty;

    if (!RavenScrollGenesisDecode(&genesis, metadata, metadataLen))
        goto returnOutputQty;
    
//  Calculate outputsRegular flags

    outputsRegular=malloc(countOutputs*sizeof(*outputsRegular));
    
    for (outputIndex=0; outputIndex<countOutputs; outputIndex++)
        outputsRegular[outputIndex]=RavenScrollScriptIsRegular(scriptPubKeys[outputIndex], scriptPubKeysLen[outputIndex], FALSE);

//  Check the transaction fee is sufficient
    
    minValidFee=RavenScrollGenesisCalcMinFee(&genesis, outputsSatoshis, outputsRegular, countOutputs);

    if (transactionFee<minValidFee)
        goto returnOutputQty;

//  Perform the genesis calculation and extract the relevant output for the calculation

    outputBalances=malloc(countOutputs*sizeof(*outputBalances));
    
    RavenScrollGenesisApply(&genesis, outputsRegular, outputBalances, countOutputs);
    
    outputQty=outputBalances[getOutputIndex];

//  Free memory and return result
    
    returnOutputQty:
        
    if (outputsRegular)
        free(outputsRegular);
    
    if (outputBalances)
        free(outputBalances);

    return outputQty;
}

RavenScrollAssetQty RavenScrollGetTransferOutputQty(const char* genesisScriptPubKeys[], const size_t genesisScriptPubKeysLen[],
                                                const RavenScrollSatoshiQty* genesisOutputsSatoshis, const int genesisCountOutputs,
                                                const RavenScrollSatoshiQty genesisTransactionFee,
                                                int64_t genesisBlockNum, int64_t genesisTxOffset, u_int8_t genesisTxId[32],
                                                const RavenScrollAssetQty* thisInputBalances, const int thisCountInputs,
                                                const char* thisScriptPubKeys[], const size_t thisScriptPubKeysLen[],
                                                const RavenScrollSatoshiQty* thisOutputsSatoshis, const int thisCountOutputs,
                                                const RavenScrollSatoshiQty thisTransactionFee, const int getOutputIndex)
{
    RavenScrollAssetQty outputQty, *thisOutputBalances;
    char genesisMetadata[400], thisMetadata[400];
    size_t genesisMetadataLen, thisMetadataLen;
    RavenScrollSatoshiQty genesisMinValidFee, thisMinValidFee;
    RavenScrollGenesis genesis;
    RavenScrollAssetRef assetRef;
    RavenScrollTransfer *transfers;
    int countTransfers, decodedTransfers, outputIndex;
    bool *genesisOutputsRegular, *thisOutputsRegular;

//  Default values for the end of this function
    
    outputQty=0;
    genesisOutputsRegular=NULL;
    transfers=NULL;
    thisOutputsRegular=NULL;
    thisOutputBalances=NULL;
    
//  Decode the metadata in the genesis transaction
    
    genesisMetadataLen=RavenScrollScriptsToMetadata(genesisScriptPubKeys, genesisScriptPubKeysLen, FALSE,
                                                  genesisCountOutputs, genesisMetadata, sizeof(genesisMetadata));
    
    if (!genesisMetadataLen)
        goto returnOutputQty;
    
    if (!RavenScrollGenesisDecode(&genesis, genesisMetadata, genesisMetadataLen))
        goto returnOutputQty;
    
//  Calculate outputsRegular flags for the genesis transaction
    
    genesisOutputsRegular=malloc(genesisCountOutputs*sizeof(*genesisOutputsRegular));
    
    for (outputIndex=0; outputIndex<genesisCountOutputs; outputIndex++)
        genesisOutputsRegular[outputIndex]=RavenScrollScriptIsRegular(genesisScriptPubKeys[outputIndex], genesisScriptPubKeysLen[outputIndex], FALSE);
    
//  Check the genesis transaction fee is sufficient
    
    genesisMinValidFee=RavenScrollGenesisCalcMinFee(&genesis, genesisOutputsSatoshis, genesisOutputsRegular, genesisCountOutputs);

    if (genesisTransactionFee<genesisMinValidFee)
        goto returnOutputQty;
    
//  Decode the metadata in this transaction
    
    thisMetadataLen=RavenScrollScriptsToMetadata(thisScriptPubKeys, thisScriptPubKeysLen, FALSE, thisCountOutputs, thisMetadata, sizeof(thisMetadata));
    
    if (thisMetadataLen)
        countTransfers=RavenScrollTransfersDecodeCount(thisMetadata, thisMetadataLen);
    else
        countTransfers=0;
    
    if (countTransfers>0) {
        transfers=malloc(countTransfers*sizeof(*transfers));
        decodedTransfers=RavenScrollTransfersDecode(transfers, countTransfers, thisCountInputs, thisCountOutputs, thisMetadata, thisMetadataLen);

        if (decodedTransfers!=countTransfers)
            goto returnOutputQty;
    }
    
//  Calculate outputsRegular flags for this transaction
    
    thisOutputsRegular=malloc(thisCountOutputs*sizeof(*thisOutputsRegular));
    
    for (outputIndex=0; outputIndex<thisCountOutputs; outputIndex++)
        thisOutputsRegular[outputIndex]=RavenScrollScriptIsRegular(thisScriptPubKeys[outputIndex], thisScriptPubKeysLen[outputIndex], FALSE);
    
//  Calculate the minimum transaction fee for the transfer list to be valid
    
    thisMinValidFee=RavenScrollTransfersCalcMinFee(transfers, countTransfers, thisCountInputs, thisCountOutputs, thisOutputsSatoshis, thisOutputsRegular);
    
//  Built the asset reference
    
    assetRef.blockNum=genesisBlockNum;
    assetRef.txOffset=genesisTxOffset;
    memcpy(assetRef.txIDPrefix, genesisTxId, RAVENSCROLL_ASSETREF_TXID_PREFIX_LEN);
    
//  Perform the transfer calculation

    thisOutputBalances=malloc(thisCountOutputs*sizeof(*thisOutputBalances));
    
    if (thisTransactionFee>=thisMinValidFee)
        RavenScrollTransfersApply(&assetRef, &genesis, transfers, countTransfers, thisInputBalances, thisCountInputs,
                                thisOutputsRegular, thisOutputBalances, thisCountOutputs);
    else
        RavenScrollTransfersApplyNone(&assetRef, &genesis, thisInputBalances, thisCountInputs, thisOutputsRegular, thisOutputBalances, thisCountOutputs);
    
//  Extract the relevant output for the calculation
    
    outputQty=thisOutputBalances[getOutputIndex];
    
//  Free memory and return result
    
    returnOutputQty:
        
    if (genesisOutputsRegular)
        free(genesisOutputsRegular);
    
    if (transfers)
        free(transfers);
    
    if (thisOutputsRegular)
        free(thisOutputsRegular);
    
    if (thisOutputBalances)
        free(thisOutputBalances);
    
    return outputQty;
}

//	Code below is adapted from Dr Brian Gladman's library via http://www.gladman.me.uk/

/*
 ---------------------------------------------------------------------------
 Copyright (c) 2002, Dr Brian Gladman, Worcester, UK.   All rights reserved.
 
 LICENSE TERMS
 
 The free distribution and use of this software in both source and binary
 form is allowed (with or without changes) provided that:
 
 1. distributions of this source code include the above copyright
 notice, this list of conditions and the following disclaimer;
 
 2. distributions in binary form include the above copyright
 notice, this list of conditions and the following disclaimer
 in the documentation and/or other associated materials;
 
 3. the copyright holder's name is not used to endorse products
 built using this software without specific written permission.
 
 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL),
 in which case the provisions of the GPL apply INSTEAD OF those given above.
 
 DISCLAIMER
 
 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 01/08/2005
 */

/******************** brg_endian.h ********************/

#define IS_BIG_ENDIAN      4321 /* byte 0 is most significant (mc68k) */
#define IS_LITTLE_ENDIAN   1234 /* byte 0 is least significant (i386) */

/* Include files where endian defines and byteswap functions may reside */
#if defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || defined( __NetBSD__ )
#  include <sys/endian.h>
#elif defined( BSD ) && ( BSD >= 199103 ) || defined( __APPLE__ ) || \
defined( __CYGWIN32__ ) || defined( __DJGPP__ ) || defined( __osf__ )
#  include <machine/endian.h>
#elif defined( __linux__ ) || defined( __GNUC__ ) || defined( __GNU_LIBRARY__ )
#  if !defined( __MINGW32__ )
#    include <endian.h>
#    if !defined( __BEOS__ )
#      include <byteswap.h>
#    endif
#  endif
#endif

/* Now attempt to set the define for platform byte order using any  */
/* of the four forms SYMBOL, _SYMBOL, __SYMBOL & __SYMBOL__, which  */
/* seem to encompass most endian symbol definitions                 */

#if defined( BIG_ENDIAN ) && defined( LITTLE_ENDIAN )
#  if defined( BYTE_ORDER ) && BYTE_ORDER == BIG_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#  elif defined( BYTE_ORDER ) && BYTE_ORDER == LITTLE_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#  endif
#elif defined( BIG_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#elif defined( LITTLE_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif

#if defined( _BIG_ENDIAN ) && defined( _LITTLE_ENDIAN )
#  if defined( _BYTE_ORDER ) && _BYTE_ORDER == _BIG_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#  elif defined( _BYTE_ORDER ) && _BYTE_ORDER == _LITTLE_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#  endif
#elif defined( _BIG_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#elif defined( _LITTLE_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif

#if defined( __BIG_ENDIAN ) && defined( __LITTLE_ENDIAN )
#  if defined( __BYTE_ORDER ) && __BYTE_ORDER == __BIG_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#  elif defined( __BYTE_ORDER ) && __BYTE_ORDER == __LITTLE_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#  endif
#elif defined( __BIG_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#elif defined( __LITTLE_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif

#if defined( __BIG_ENDIAN__ ) && defined( __LITTLE_ENDIAN__ )
#  if defined( __BYTE_ORDER__ ) && __BYTE_ORDER__ == __BIG_ENDIAN__
#    define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#  elif defined( __BYTE_ORDER__ ) && __BYTE_ORDER__ == __LITTLE_ENDIAN__
#    define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#  endif
#elif defined( __BIG_ENDIAN__ )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#elif defined( __LITTLE_ENDIAN__ )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif

/*  if the platform byte order could not be determined, then try to */
/*  set this define using common machine defines                    */
#if !defined(PLATFORM_BYTE_ORDER)

#if   defined( __alpha__ ) || defined( __alpha ) || defined( i386 )       || \
defined( __i386__ )  || defined( _M_I86 )  || defined( _M_IX86 )    || \
defined( __OS2__ )   || defined( sun386 )  || defined( __TURBOC__ ) || \
defined( vax )       || defined( vms )     || defined( VMS )        || \
defined( __VMS )     || defined( _M_X64 )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN

#elif defined( AMIGA )   || defined( applec )    || defined( __AS400__ )  || \
defined( _CRAY )   || defined( __hppa )    || defined( __hp9000 )   || \
defined( ibm370 )  || defined( mc68000 )   || defined( m68k )       || \
defined( __MRC__ ) || defined( __MVS__ )   || defined( __MWERKS__ ) || \
defined( sparc )   || defined( __sparc)    || defined( SYMANTEC_C ) || \
defined( __VOS__ ) || defined( __TIGCC__ ) || defined( __TANDEM )   || \
defined( THINK_C ) || defined( __VMCMS__ )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN

#elif 0     /* **** EDIT HERE IF NECESSARY **** */
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#elif 0     /* **** EDIT HERE IF NECESSARY **** */
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#else
#  error Please edit lines 126 or 128 in brg_endian.h to set the platform byte order
#endif

#endif

/******************** brg_types.h ********************/

#include <limits.h>
    
#ifndef BRG_UI8
#  define BRG_UI8
#  if UCHAR_MAX == 255u
    typedef unsigned char uint_8t;
#  else
#    error Please define uint_8t as an 8-bit unsigned integer type in brg_types.h
#  endif
#endif
    
#ifndef BRG_UI16
#  define BRG_UI16
#  if USHRT_MAX == 65535u
    typedef unsigned short uint_16t;
#  else
#    error Please define uint_16t as a 16-bit unsigned short type in brg_types.h
#  endif
#endif
    
#ifndef BRG_UI32
#  define BRG_UI32
#  if UINT_MAX == 4294967295u
#    define li_32(h) 0x##h##u
    typedef unsigned int uint_32t;
#  elif ULONG_MAX == 4294967295u
#    define li_32(h) 0x##h##ul
    typedef unsigned long uint_32t;
#  elif defined( _CRAY )
#    error This code needs 32-bit data types, which Cray machines do not provide
#  else
#    error Please define uint_32t as a 32-bit unsigned integer type in brg_types.h
#  endif
#endif
    
#ifndef BRG_UI64
#  if defined( __BORLANDC__ ) && !defined( __MSDOS__ )
#    define BRG_UI64
#    define li_64(h) 0x##h##ull
    typedef unsigned __int64 uint_64t;
#  elif defined( _MSC_VER ) && ( _MSC_VER < 1300 )    /* 1300 == VC++ 7.0 */
#    define BRG_UI64
#    define li_64(h) 0x##h##ui64
    typedef unsigned __int64 uint_64t;
#  elif defined( __sun ) && defined(ULONG_MAX) && ULONG_MAX == 0xfffffffful
#    define BRG_UI64
#    define li_64(h) 0x##h##ull
    typedef unsigned long long uint_64t;
#  elif defined( UINT_MAX ) && UINT_MAX > 4294967295u
#    if UINT_MAX == 18446744073709551615u
#      define BRG_UI64
#      define li_64(h) 0x##h##u
    typedef unsigned int uint_64t;
#    endif
#  elif defined( ULONG_MAX ) && ULONG_MAX > 4294967295u
#    if ULONG_MAX == 18446744073709551615ul
#      define BRG_UI64
#      define li_64(h) 0x##h##ul
    typedef unsigned long uint_64t;
#    endif
#  elif defined( ULLONG_MAX ) && ULLONG_MAX > 4294967295u
#    if ULLONG_MAX == 18446744073709551615ull
#      define BRG_UI64
#      define li_64(h) 0x##h##ull
    typedef unsigned long long uint_64t;
#    endif
#  elif defined( ULONG_LONG_MAX ) && ULONG_LONG_MAX > 4294967295u
#    if ULONG_LONG_MAX == 18446744073709551615ull
#      define BRG_UI64
#      define li_64(h) 0x##h##ull
    typedef unsigned long long uint_64t;
#    endif
#  endif
#endif
    
#if defined( NEED_UINT_64T ) && !defined( BRG_UI64 )
#  error Please define uint_64t as an unsigned 64 bit type in brg_types.h
#endif
    
#ifndef RETURN_VALUES
#  define RETURN_VALUES
#  if defined( DLL_EXPORT )
#    if defined( _MSC_VER ) || defined ( __INTEL_COMPILER )
#      define VOID_RETURN    __declspec( dllexport ) void __stdcall
#      define INT_RETURN     __declspec( dllexport ) int  __stdcall
#    elif defined( __GNUC__ )
#      define VOID_RETURN    __declspec( __dllexport__ ) void
#      define INT_RETURN     __declspec( __dllexport__ ) int
#    else
#      error Use of the DLL is only available on the Microsoft, Intel and GCC compilers
#    endif
#  elif defined( DLL_IMPORT )
#    if defined( _MSC_VER ) || defined ( __INTEL_COMPILER )
#      define VOID_RETURN    __declspec( dllimport ) void __stdcall
#      define INT_RETURN     __declspec( dllimport ) int  __stdcall
#    elif defined( __GNUC__ )
#      define VOID_RETURN    __declspec( __dllimport__ ) void
#      define INT_RETURN     __declspec( __dllimport__ ) int
#    else
#      error Use of the DLL is only available on the Microsoft, Intel and GCC compilers
#    endif
#  elif defined( __WATCOMC__ )
#    define VOID_RETURN  void __cdecl
#    define INT_RETURN   int  __cdecl
#  else
#    define VOID_RETURN  void
#    define INT_RETURN   int
#  endif
#endif
    
#define ui_type(size)               uint_##size##t
#define dec_unit_type(size,x)       typedef ui_type(size) x
#define dec_bufr_type(size,bsize,x) typedef ui_type(size) x[bsize / (size >> 3)]
#define ptr_cast(x,size)            ((ui_type(size)*)(x))

/******************** sha2.h ********************/

#define SHA_256

#define SHA256_DIGEST_SIZE  32
#define SHA256_BLOCK_SIZE   64
    
    typedef struct
    {   uint_32t count[2];
        uint_32t hash[8];
        uint_32t wbuf[16];
    } sha256_ctx;
    
    static VOID_RETURN sha256_compile(sha256_ctx ctx[1]);
    
    static VOID_RETURN sha256_begin(sha256_ctx ctx[1]);
    static VOID_RETURN sha256_hash(const unsigned char data[], unsigned long len, sha256_ctx ctx[1]);
    static VOID_RETURN sha256(unsigned char hval[], const unsigned char data[], unsigned long len);

/******************** sha2.c ********************/

#if defined( _MSC_VER ) && ( _MSC_VER > 800 )
#pragma intrinsic(memcpy)
#endif
    
#define rotl32(x,n)   (((x) << n) | ((x) >> (32 - n)))
#define rotr32(x,n)   (((x) >> n) | ((x) << (32 - n)))

#if !defined(bswap_32)
#define bswap_32(x) ((rotr32((x), 24) & 0x00ff00ff) | (rotr32((x), 8) & 0xff00ff00))
#endif
    
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
#define SWAP_BYTES
#else
#undef  SWAP_BYTES
#endif
    
#define ch(x,y,z)       ((z) ^ ((x) & ((y) ^ (z))))
#define maj(x,y,z)      (((x) & (y)) | ((z) & ((x) ^ (y))))
    
    /* round transforms for SHA256 and SHA512 compression functions */
    
#define vf(n,i) v[(n - i) & 7]
    
#define hf(i) (p[i & 15] += \
g_1(p[(i + 14) & 15]) + p[(i + 9) & 15] + g_0(p[(i + 1) & 15]))
    
#define v_cycle(i,j)                                \
vf(7,i) += (j ? hf(i) : p[i]) + k_0[i+j]        \
+ s_1(vf(4,i)) + ch(vf(4,i),vf(5,i),vf(6,i));   \
vf(3,i) += vf(7,i);                             \
vf(7,i) += s_0(vf(0,i))+ maj(vf(0,i),vf(1,i),vf(2,i))
    
#if defined(SHA_224) || defined(SHA_256)
    
#define SHA256_MASK (SHA256_BLOCK_SIZE - 1)
    
#if defined(SWAP_BYTES)
#define bsw_32(p,n) \
{ int _i = (n); while(_i--) ((uint_32t*)p)[_i] = bswap_32(((uint_32t*)p)[_i]); }
#else
#define bsw_32(p,n)
#endif
    
#define s_0(x)  (rotr32((x),  2) ^ rotr32((x), 13) ^ rotr32((x), 22))
#define s_1(x)  (rotr32((x),  6) ^ rotr32((x), 11) ^ rotr32((x), 25))
#define g_0(x)  (rotr32((x),  7) ^ rotr32((x), 18) ^ ((x) >>  3))
#define g_1(x)  (rotr32((x), 17) ^ rotr32((x), 19) ^ ((x) >> 10))
#define k_0     k256
    
    /* rotated SHA256 round definition. Rather than swapping variables as in    */
    /* FIPS-180, different variables are 'rotated' on each round, returning     */
    /* to their starting positions every eight rounds                           */
    
#define q(n)  v##n
    
#define one_cycle(a,b,c,d,e,f,g,h,k,w)  \
q(h) += s_1(q(e)) + ch(q(e), q(f), q(g)) + k + w; \
q(d) += q(h); q(h) += s_0(q(a)) + maj(q(a), q(b), q(c))
    
    /* SHA256 mixing data   */
    
    static const uint_32t k256[64] =
    {   0x428a2f98ul, 0x71374491ul, 0xb5c0fbcful, 0xe9b5dba5ul,
        0x3956c25bul, 0x59f111f1ul, 0x923f82a4ul, 0xab1c5ed5ul,
        0xd807aa98ul, 0x12835b01ul, 0x243185beul, 0x550c7dc3ul,
        0x72be5d74ul, 0x80deb1feul, 0x9bdc06a7ul, 0xc19bf174ul,
        0xe49b69c1ul, 0xefbe4786ul, 0x0fc19dc6ul, 0x240ca1ccul,
        0x2de92c6ful, 0x4a7484aaul, 0x5cb0a9dcul, 0x76f988daul,
        0x983e5152ul, 0xa831c66dul, 0xb00327c8ul, 0xbf597fc7ul,
        0xc6e00bf3ul, 0xd5a79147ul, 0x06ca6351ul, 0x14292967ul,
        0x27b70a85ul, 0x2e1b2138ul, 0x4d2c6dfcul, 0x53380d13ul,
        0x650a7354ul, 0x766a0abbul, 0x81c2c92eul, 0x92722c85ul,
        0xa2bfe8a1ul, 0xa81a664bul, 0xc24b8b70ul, 0xc76c51a3ul,
        0xd192e819ul, 0xd6990624ul, 0xf40e3585ul, 0x106aa070ul,
        0x19a4c116ul, 0x1e376c08ul, 0x2748774cul, 0x34b0bcb5ul,
        0x391c0cb3ul, 0x4ed8aa4aul, 0x5b9cca4ful, 0x682e6ff3ul,
        0x748f82eeul, 0x78a5636ful, 0x84c87814ul, 0x8cc70208ul,
        0x90befffaul, 0xa4506cebul, 0xbef9a3f7ul, 0xc67178f2ul,
    };
    
    /* Compile 64 bytes of hash data into SHA256 digest value   */
    /* NOTE: this routine assumes that the byte order in the    */
    /* ctx->wbuf[] at this point is such that low address bytes */
    /* in the ORIGINAL byte stream will go into the high end of */
    /* words on BOTH big and little endian systems              */
    
    static VOID_RETURN sha256_compile(sha256_ctx ctx[1])
    {
        uint_32t j, *p = ctx->wbuf, v[8];
        
        memcpy(v, ctx->hash, 8 * sizeof(uint_32t));
        
        for(j = 0; j < 64; j += 16)
        {
            v_cycle( 0, j); v_cycle( 1, j);
            v_cycle( 2, j); v_cycle( 3, j);
            v_cycle( 4, j); v_cycle( 5, j);
            v_cycle( 6, j); v_cycle( 7, j);
            v_cycle( 8, j); v_cycle( 9, j);
            v_cycle(10, j); v_cycle(11, j);
            v_cycle(12, j); v_cycle(13, j);
            v_cycle(14, j); v_cycle(15, j);
        }
        
        ctx->hash[0] += v[0]; ctx->hash[1] += v[1];
        ctx->hash[2] += v[2]; ctx->hash[3] += v[3];
        ctx->hash[4] += v[4]; ctx->hash[5] += v[5];
        ctx->hash[6] += v[6]; ctx->hash[7] += v[7];
    }
    
    /* SHA256 hash data in an array of bytes into hash buffer   */
    /* and call the hash_compile function as required.          */
    
    static VOID_RETURN sha256_hash(const unsigned char data[], unsigned long len, sha256_ctx ctx[1])
    {   uint_32t pos = (uint_32t)(ctx->count[0] & SHA256_MASK),
        space = SHA256_BLOCK_SIZE - pos;
        const unsigned char *sp = data;
        
        if((ctx->count[0] += len) < len)
            ++(ctx->count[1]);
        
        while(len >= space)     /* tranfer whole blocks while possible  */
        {
            memcpy(((unsigned char*)ctx->wbuf) + pos, sp, space);
            sp += space; len -= space; space = SHA256_BLOCK_SIZE; pos = 0;
            bsw_32(ctx->wbuf, SHA256_BLOCK_SIZE >> 2)
            sha256_compile(ctx);
        }
        
        memcpy(((unsigned char*)ctx->wbuf) + pos, sp, len);
    }
    
    /* SHA256 Final padding and digest calculation  */
    
    static void sha_end1(unsigned char hval[], sha256_ctx ctx[1], const unsigned int hlen)
    {   uint_32t    i = (uint_32t)(ctx->count[0] & SHA256_MASK);
        
        /* put bytes in the buffer in an order in which references to   */
        /* 32-bit words will put bytes with lower addresses into the    */
        /* top of 32 bit words on BOTH big and little endian machines   */
        bsw_32(ctx->wbuf, (i + 3) >> 2)
        
        /* we now need to mask valid bytes and add the padding which is */
        /* a single 1 bit and as many zero bits as necessary. Note that */
        /* we can always add the first padding byte here because the    */
        /* buffer always has at least one empty slot                    */
        ctx->wbuf[i >> 2] &= 0xffffff80 << 8 * (~i & 3);
        ctx->wbuf[i >> 2] |= 0x00000080 << 8 * (~i & 3);
        
        /* we need 9 or more empty positions, one for the padding byte  */
        /* (above) and eight for the length count.  If there is not     */
        /* enough space pad and empty the buffer                        */
        if(i > SHA256_BLOCK_SIZE - 9)
        {
            if(i < 60) ctx->wbuf[15] = 0;
            sha256_compile(ctx);
            i = 0;
        }
        else    /* compute a word index for the empty buffer positions  */
            i = (i >> 2) + 1;
        
        while(i < 14) /* and zero pad all but last two positions        */
            ctx->wbuf[i++] = 0;
        
        /* the following 32-bit length fields are assembled in the      */
        /* wrong byte order on little endian machines but this is       */
        /* corrected later since they are only ever used as 32-bit      */
        /* word values.                                                 */
        ctx->wbuf[14] = (ctx->count[1] << 3) | (ctx->count[0] >> 29);
        ctx->wbuf[15] = ctx->count[0] << 3;
        sha256_compile(ctx);
        
        /* extract the hash value as bytes in case the hash buffer is   */
        /* mislaigned for 32-bit words                                  */
        for(i = 0; i < hlen; ++i)
            hval[i] = (unsigned char)(ctx->hash[i >> 2] >> (8 * (~i & 3)));
    }
    
#endif
    
#if defined(SHA_256)
    
    static const uint_32t i256[8] =
    {
        0x6a09e667ul, 0xbb67ae85ul, 0x3c6ef372ul, 0xa54ff53aul,
        0x510e527ful, 0x9b05688cul, 0x1f83d9abul, 0x5be0cd19ul
    };
    
    static VOID_RETURN sha256_begin(sha256_ctx ctx[1])
    {
        ctx->count[0] = ctx->count[1] = 0;
        memcpy(ctx->hash, i256, 8 * sizeof(uint_32t));
    }
    
    static VOID_RETURN sha256(unsigned char hval[], const unsigned char data[], unsigned long len)
    {   sha256_ctx  cx[1];
        
        sha256_begin(cx);
        sha256_hash(data, len, cx);
        sha_end1(hval, cx, SHA256_DIGEST_SIZE);
    }
    
#endif
