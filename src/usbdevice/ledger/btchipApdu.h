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
#define SW_SECURITY_STATUS_NOT_SATISFIED 0x6982


#define OFFSET_CDATA 0x04

#define TRUSTED_INPUT_SIZE 56

#define MAX_BIP32_PATH 10

#endif


