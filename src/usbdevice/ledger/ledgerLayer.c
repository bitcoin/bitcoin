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

#include "ledgerLayer.h"

#define TAG_APDU 0x05

int wrapCommandAPDU(unsigned int channel, const unsigned char *command, size_t commandLength, unsigned int packetSize, unsigned char *out, size_t outLength) {
	int sequenceIdx = 0;
	int offset = 0;
	int offsetOut = 0;
	int blockSize;
	if (packetSize < 3) {
		return -1;
	}
	if (outLength < 7) {
		return -1;
	}
	outLength -= 7;
	out[offsetOut++] = ((channel >> 8) & 0xff);
	out[offsetOut++] = (channel & 0xff);
	out[offsetOut++] = TAG_APDU;
	out[offsetOut++] = ((sequenceIdx >> 8) & 0xff);
	out[offsetOut++] = (sequenceIdx & 0xff);
	sequenceIdx++;
	out[offsetOut++] = ((commandLength >> 8) & 0xff);
	out[offsetOut++] = (commandLength & 0xff);
	blockSize = (commandLength > packetSize - 7 ? packetSize - 7 : commandLength);
	if (outLength < blockSize) {
		return -1;
	}
	outLength -= blockSize;
	memcpy(out + offsetOut, command + offset, blockSize);
	offsetOut += blockSize;
	offset += blockSize;
	while (offset != commandLength) {
		if (outLength < 5) {
			return -1;
		}
		outLength -= 5;
		out[offsetOut++] = ((channel >> 8) & 0xff);
		out[offsetOut++] = (channel & 0xff);
		out[offsetOut++] = TAG_APDU;
		out[offsetOut++] = ((sequenceIdx >> 8) & 0xff);
		out[offsetOut++] = (sequenceIdx & 0xff);
		sequenceIdx++;
		blockSize = ((commandLength - offset) > packetSize - 5 ? packetSize - 5 : commandLength - offset);
		if (outLength < blockSize) {
			return -1;
		}
		outLength -= blockSize;
		memcpy(out + offsetOut, command + offset, blockSize);
		offsetOut += blockSize;
		offset += blockSize;
	}
	while ((offsetOut % packetSize) != 0) {
		if (outLength < 1) {
			return -1;
		}
		outLength--;
		out[offsetOut++] = 0;
	}
	return offsetOut;
}

int unwrapReponseAPDU(unsigned int channel, const unsigned char *data, size_t dataLength, unsigned int packetSize, unsigned char *out, size_t outLength) {
	int sequenceIdx = 0;
	int offset = 0;
	int offsetOut = 0;
	int responseLength;
	int blockSize;
	if ((data == NULL) || (dataLength < 7 + 5)) { 
		return 0;
	}
	if (data[offset++] != ((channel >> 8) & 0xff)) {
		return -1;
	}
	if (data[offset++] != (channel & 0xff)) {
		return -1;
	}
	if (data[offset++] != TAG_APDU)	{
		return -1;
	}
	if (data[offset++] != ((sequenceIdx >> 8) & 0xff)) {
		return -1;
	}
	if (data[offset++] != (sequenceIdx & 0xff)) {
		return -1;
	}
	responseLength = (data[offset++] << 8);
	responseLength |= data[offset++];
	if (outLength < responseLength) {
		return -1;
	}
	if (dataLength < 7 + responseLength) {
		return 0;
	}
	blockSize = (responseLength > packetSize - 7 ? packetSize - 7 : responseLength);
	memcpy(out + offsetOut, data + offset, blockSize);
	offset += blockSize;
	offsetOut += blockSize;
	while (offsetOut != responseLength) {
		sequenceIdx++;
		if (offset == dataLength) {
			return 0;
		}
		if (data[offset++] != ((channel >> 8) & 0xff)) {
			return -1;
		}
		if (data[offset++] != (channel & 0xff)) {
			return -1;
		}
		if (data[offset++] != TAG_APDU)	{
			return -1;
		}
		if (data[offset++] != ((sequenceIdx >> 8) & 0xff)) {
			return -1;
		}
		if (data[offset++] != (sequenceIdx & 0xff)) {
			return -1;
		}
		blockSize = ((responseLength - offsetOut) > packetSize - 5 ? packetSize - 5 : responseLength - offsetOut);
		if (blockSize > dataLength - offset) {
			return 0;
		}
		memcpy(out + offsetOut, data + offset, blockSize);
		offset += blockSize;
		offsetOut += blockSize;
	}
	return offsetOut;
}

