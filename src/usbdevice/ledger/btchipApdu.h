/*
*******************************************************************************
*   BTChip Bitcoin Hardware Wallet C test interface
*   (c) 2014 BTChip - 1BTChip7VfTnrPra5jqci7ejnMguuHogTn
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*   limitations under the License.
********************************************************************************/

#ifndef __APDU_H__

#define __APDU_H__

#define BTCHIP_CLA 0xE0
#define BTCHIP_ADM_CLA 0xD0

#define BTCHIP_INS_GET_COIN_VER              0x16
#define BTCHIP_INS_SETUP                     0x20
#define BTCHIP_INS_VERIFY_PIN                0x22
#define BTCHIP_INS_GET_OPERATION_MODE        0x24
#define BTCHIP_INS_SET_OPERATION_MODE        0x26
#define BTCHIP_INS_SET_KEYMAP		     	 0x28
#define BTCHIP_INS_SET_COMM_PROTOCOL         0x2A
#define BTCHIP_INS_GET_WALLET_PUBLIC_KEY     0x40
#define BTCHIP_INS_GET_TRUSTED_INPUT         0x42
#define BTCHIP_INS_HASH_INPUT_START          0x44
#define BTCHIP_INS_HASH_INPUT_FINALIZE       0x46
#define BTCHIP_INS_HASH_SIGN                 0x48
#define BTCHIP_INS_HASH_INPUT_FINALIZE_FULL  0x4A
#define BTCHIP_INS_GET_INTERNAL_CHAIN_INDEX  0x4C
#define BTCHIP_INS_SIGN_MESSAGE				 0x4E
#define BTCHIP_INS_GET_TRANSACTION_LIMIT     0xA0
#define BTCHIP_INS_SET_TRANSACTION_LIMIT     0xA2
#define BTCHIP_INS_IMPORT_PRIVATE_KEY        0xB0
#define BTCHIP_INS_GET_PUBLIC_KEY            0xB2
#define BTCHIP_INS_DERIVE_BIP32_KEY          0xB4
#define BTCHIP_INS_SIGNVERIFY_IMMEDIATE      0xB6
#define BTCHIP_INS_GET_RANDOM				 0xC0
#define BTCHIP_INS_GET_ATTESTATION			 0xC2
#define BTCHIP_INS_GET_FIRMWARE_VERSION		 0xC4
#define BTCHIP_INS_COMPOSE_MOFN_ADDRESS		 0xC6
#define BTCHIP_INS_DONGLE_AUTHENTICATE		 0xC8
#define BTCHIP_INS_GET_POS_SEED				 0xCA

#define BTCHIP_INS_ADM_INIT_KEYS			 0x20
#define BTCHIP_INS_ADM_INIT_ATTESTATION      0x22
#define BTCHIP_INS_ADM_GET_UPDATE_ID         0x24
#define BTCHIP_INS_ADM_FIRMWARE_UPDATE       0x42

#define SW_OK 0x9000
#define SW_UNKNOWN 0x6D00
#define SW_PIN_REMAINING_ATTEMPTS 0x63C0
#define SW_INCORRECT_LENGTH 0x6700
#define SW_COMMAND_INCOMPATIBLE_FILE_STRUCTURE 0x6981
#define SW_SECURITY_STATUS_NOT_SATISFIED 0x6982
#define SW_CONDITIONS_OF_USE_NOT_SATISFIED 0x6985
#define SW_INCORRECT_DATA 0x6A80
#define SW_NOT_ENOUGH_MEMORY_SPACE 0x6A84
#define SW_REFERENCED_DATA_NOT_FOUND 0x6A88
#define SW_FILE_ALREADY_EXISTS 0x6A89
#define SW_INCORRECT_P1_P2 0x6B00
#define SW_INS_NOT_SUPPORTED 0x6D00
#define SW_CLA_NOT_SUPPORTED 0x6E00
#define SW_TECHNICAL_PROBLEM 0x6F00
#define SW_MEMORY_PROBLEM 0x9240
#define SW_NO_EF_SELECTED 0x9400
#define SW_INVALID_OFFSET 0x9402
#define SW_FILE_NOT_FOUND 0x9404
#define SW_INCONSISTENT_FILE 0x9408
#define SW_ALGORITHM_NOT_SUPPORTED 0x9484
#define SW_INVALID_KCV 0x9485
#define SW_CODE_NOT_INITIALIZED 0x9802
#define SW_ACCESS_CONDITION_NOT_FULFILLED 0x9804
#define SW_CONTRADICTION_SECRET_CODE_STATUS 0x9808
#define SW_CONTRADICTION_INVALIDATION 0x9810
#define SW_CODE_BLOCKED 0x9840
#define SW_MAX_VALUE_REACHED 0x9850
#define SW_GP_AUTH_FAILED 0x6300
#define SW_LICENSING 0x6F42
#define SW_HALTED 0x6FAA


#define OFFSET_CDATA 0x04

#define TRUSTED_INPUT_SIZE 56

#define MAX_BIP32_PATH 10

enum btchip_modes_e {
    BTCHIP_MODE_ISSUER = 0x00,
    BTCHIP_MODE_SETUP_NEEDED = 0xff,
    BTCHIP_MODE_WALLET = 0x01,
    BTCHIP_MODE_RELAXED_WALLET = 0x02,
    BTCHIP_MODE_SERVER = 0x04,
    BTCHIP_MODE_DEVELOPER = 0x08,
};

#endif


