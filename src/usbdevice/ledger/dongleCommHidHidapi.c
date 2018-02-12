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

#include "dongleCommHidHidapi.h"
#include "ledgerLayer.h"

#define BTCHIP_VID 0x2581
#define LEDGER_VID 0x2C97
#define BTCHIP_HID_PID 0x2b7c
#define BTCHIP_HID_PID_LEDGER 0x3b7c
#define BTCHIP_HID_PID_LEDGER_PROTON 0x4b7c
#define BTCHIP_HID_BOOTLOADER_PID 0x1807
#define BLUE_PID 0x0000
#define NANOS_PID 0x0001

#define TIMEOUT 60000
#define SW1_DATA 0x61
#define MAX_BLOCK 64

#ifdef HAVE_HIDAPI

int initHidHidapi() {
	return hid_init();
}

int exitHidHidapi() {
	return hid_exit();
}

int sendApduHidHidapi(hid_device *handle, const unsigned char ledger, const unsigned char *apdu, size_t apduLength, unsigned char *out, size_t outLength, int *sw) {
	unsigned char buffer[400];
	unsigned char paddingBuffer[MAX_BLOCK+1];
	int result;
	int length;
	int swOffset;
	int remaining = apduLength;
	int offset = 0;

	if (ledger) {
		result = wrapCommandAPDU(DEFAULT_LEDGER_CHANNEL, apdu, apduLength, LEDGER_HID_PACKET_SIZE, buffer, sizeof(buffer));
		if (result < 0) {
			return result;
		}
		remaining = result;
	}
	else {
		memcpy(buffer, apdu, apduLength);
		remaining = apduLength;
	}
	while (remaining > 0) {
		int blockSize = (remaining > MAX_BLOCK ? MAX_BLOCK : remaining);
		// First byte must be 0x00, report ID
		memset(paddingBuffer, 0, MAX_BLOCK+1);
		memcpy(paddingBuffer+1, buffer + offset, blockSize);
		result = hid_write(handle, paddingBuffer, blockSize+1);
		if (result < 0) {
			return result;
		}
		offset += blockSize;
		remaining -= blockSize;
	}
	result = hid_read_timeout(handle, buffer, MAX_BLOCK, TIMEOUT);
	if (result < 0) {
		return result;
	}
	offset = MAX_BLOCK;
	if (!ledger) {
		if (buffer[0] == SW1_DATA) {
			length = buffer[1];
			length += 2;
			if (length > (MAX_BLOCK - 2)) {
				remaining = length - (MAX_BLOCK - 2);
				while (remaining != 0) {
					int blockSize;
					if (remaining > MAX_BLOCK) {
						blockSize = MAX_BLOCK;
					}
					else {
						blockSize = remaining;
					}
					result = hid_read_timeout(handle, buffer + offset, MAX_BLOCK, TIMEOUT);
					if (result < 0) {
						return result;
					}
					offset += blockSize;
					remaining -= blockSize;
				}
			}
			length -= 2;
			memcpy(out, buffer + 2, length);
			swOffset = 2 + length;
		}
		else {
			length = 0;
			swOffset = 0;
		}
		if (sw != NULL) {
			*sw = (buffer[swOffset] << 8) | buffer[swOffset + 1];
		}
	}
	else {
		for (;;) {
			result = unwrapReponseAPDU(DEFAULT_LEDGER_CHANNEL, buffer, offset, LEDGER_HID_PACKET_SIZE, out, outLength);
			if (result < 0) {
				return result;
			}
			if (result != 0) {
				length = result - 2;
				swOffset = result - 2;
				break;
			}
			result = hid_read_timeout(handle, buffer + offset, MAX_BLOCK, TIMEOUT);
			if (result < 0) {
				return result;
			}
			offset += MAX_BLOCK;
		}
		if (sw != NULL) {
			*sw = (out[swOffset] << 8) | out[swOffset + 1];
		}
	}
	return length;
}

hid_device* getFirstDongleHidHidapi(unsigned char *ledger) {
	hid_device *result = hid_open(BTCHIP_VID, BTCHIP_HID_PID, NULL);
	if (result != NULL) {
		return result;
	}
	result = hid_open(BTCHIP_VID, BTCHIP_HID_PID_LEDGER, NULL);
	if (result != NULL) {
		*ledger = 1;
		return result;
	}
	result = hid_open(BTCHIP_VID, BTCHIP_HID_PID_LEDGER_PROTON, NULL);
	if (result != NULL) {
		*ledger = 1;
		return result;
	}
	result = hid_open(BTCHIP_VID, BTCHIP_HID_BOOTLOADER_PID, NULL);
	if (result != NULL) {
		return result;
	}
	result = hid_open(LEDGER_VID, BLUE_PID, NULL);
	if (result != NULL) {
		*ledger = 1;
		return result;
	}
	result = hid_open(LEDGER_VID, NANOS_PID, NULL);
	if (result != NULL) {
		*ledger = 1;
		return result;
	}
	return NULL;
}

void closeDongleHidHidapi(hid_device *handle) {
	hid_close(handle);
}

#endif
