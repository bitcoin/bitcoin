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

#ifndef __LEDGERLAYER_H__

#define __LEDGERLAYER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_LEDGER_CHANNEL 0x0101
#define LEDGER_HID_PACKET_SIZE 64

int wrapCommandAPDU(unsigned int channel, const unsigned char *command, size_t commandLength, unsigned int packetSize, unsigned char *out, size_t outLength);
int unwrapReponseAPDU(unsigned int channel, const unsigned char *data, size_t dataLength, unsigned int packetSize, unsigned char *out, size_t outLength);

#endif


