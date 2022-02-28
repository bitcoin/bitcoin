#include "dynprogram.h"
//

std::string CDynProgram::execute(unsigned char* blockHeader, std::string prevBlockHash, std::string merkleRoot) {

    //initial input is SHA256 of header data

    CSHA256 ctx;

    uint32_t iResult[8];


    /*
    memset(blockHeader, 0, 80);
    prevBlockHash = "0000000000000000000000000000000000000000000000000000000000000000";
    merkleRoot = "0000000000000000000000000000000000000000000000000000000000000000";
    */

    ctx.Write(blockHeader, 80);
    ctx.Finalize((unsigned char*) iResult);


    int line_ptr = 0;       //program execution line pointer
    int loop_counter = 0;   //counter for loop execution
    unsigned int memory_size = 0;    //size of current memory pool
    uint32_t* memPool = NULL;     //memory pool

    int loop_line_ptr = 0;      //to mark return OPCODE for LOOP command
    unsigned int loop_opcode_count = 0;  //number of times to run the LOOP

    uint32_t temp[8];

    uint32_t prevHashSHA[8];
    uint32_t iPrevHash[8];
    parseHex(prevBlockHash, (unsigned char*)iPrevHash);


    ///////////////memset(iPrevHash, 0, 32);


    ctx.Reset();
    ctx.Write((unsigned char*)iPrevHash, 32);
    ctx.Finalize((unsigned char*)prevHashSHA);

    ////////////int c = 0;

    while (line_ptr < program.size()) {
        std::istringstream iss(program[line_ptr]);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};     //split line into tokens


        /*
        printf("%s\n", program[line_ptr].c_str());
        printf("start %08X%08X%08X%08X%08X%08X%08X%08X\n",  iResult[0], iResult[1], iResult[2], iResult[3], iResult[4], iResult[5], iResult[6], iResult[7]);
        */
        

        //simple ADD and XOR functions with one constant argument
        if (tokens[0] == "ADD") {
            uint32_t arg1[8];
            parseHex(tokens[1], (unsigned char*) arg1);
            for (int i = 0; i < 8; i++)
                iResult[i] += arg1[i];
        }

        else if (tokens[0] == "XOR") {
            uint32_t arg1[8];
            parseHex(tokens[1], (unsigned char*)arg1);
            for (int i = 0; i < 8; i++)
                iResult[i] ^= arg1[i];
        }

        //hash algo which can be optionally repeated several times
        else if (tokens[0] == "SHA2") {
            if (tokens.size() == 2) { //includes a loop count
                loop_counter = atoi(tokens[1].c_str());
                for (int i = 0; i < loop_counter; i++) {
                    if (tokens[0] == "SHA2") {
                        unsigned char output[32];
                        ctx.Reset();
                        ctx.Write((unsigned char*)iResult, 32);
                        ctx.Finalize(output);
                        memcpy(iResult, output, 32);
                    }
                }
            }

            else {                         //just a single run
                if (tokens[0] == "SHA2") {
                    unsigned char output[32];
                    ctx.Reset();
                    ctx.Write((unsigned char*)iResult, 32);
                    ctx.Finalize(output);
                    memcpy(iResult, output, 32);
                }
            }
        }

        //generate a block of memory based on a hashing algo
        else if (tokens[0] == "MEMGEN") {
            if (memPool != NULL)
                free(memPool);
            memory_size = atoi(tokens[2].c_str());
            memPool = (uint32_t*)malloc(memory_size * 32);
            for (int i = 0; i < memory_size; i++) {
                if (tokens[1] == "SHA2") {
                    unsigned char output[32];
                    ctx.Reset();
                    ctx.Write((unsigned char*)iResult, 32);
                    ctx.Finalize(output);
                    memcpy(iResult, output, 32);
                    memcpy(memPool + i * 8, iResult, 32);
                }
            }
        }

        //add a constant to every value in the memory block
        else if (tokens[0] == "MEMADD") {
            if (memPool != NULL) {
                uint32_t arg1[8];
                parseHex(tokens[1], (unsigned char*)arg1);

                for (int i = 0; i < memory_size; i++) {
                    for (int j = 0; j < 8; j++)
                        memPool[i * 8 + j] += arg1[j];
                }
            }
        }

        //add the sha of the prev block hash with every value in the memory block
        else if (tokens[0] == "MEMADDHASHPREV") {
            if (memPool != NULL) {
                for (int i = 0; i < memory_size; i++) {
                    for (int j = 0; j < 8; j++) {
                        memPool[i * 8 + j] += iResult[j];
                        memPool[i * 8 + j] += prevHashSHA[j];
                    }
                }
            }
        }


        //xor a constant with every value in the memory block
        else if (tokens[0] == "MEMXOR") {
            if (memPool != NULL) {
                uint32_t arg1[8];
                parseHex(tokens[1], (unsigned char*)arg1);

                for (int i = 0; i < memory_size; i++) {
                    for (int j = 0; j < 8; j++)
                        memPool[i * 8 + j] ^= arg1[j];
                }
            }
        }

        //xor the sha of the prev block hash with every value in the memory block
        else if (tokens[0] == "MEMXORHASHPREV") {
            if (memPool != NULL) {

                for (int i = 0; i < memory_size; i++) {
                    for (int j = 0; j < 8; j++) {
                        memPool[i * 8 + j] += iResult[j];
                        memPool[i * 8 + j] ^= prevHashSHA[j];
                    }
                }
            }
        }

        //read a value based on an index into the generated block of memory
        else if (tokens[0] == "READMEM") {
            if (memPool != NULL) {
                unsigned int index = 0;

                if (tokens[1] == "MERKLE") {
                    uint32_t arg1[8];
                    parseHex(merkleRoot, (unsigned char*)arg1);
                    index = arg1[0] % memory_size;
                    memcpy(iResult, memPool + index * 8, 32);
                }

                else if (tokens[1] == "HASHPREV") {
                    uint32_t arg1[8];
                    parseHex(prevBlockHash, (unsigned char*)arg1);
                    index = arg1[0] % memory_size;
                    memcpy(iResult, memPool + index * 8, 32);
                }
            }
        }

        else if (tokens[0] == "READMEM2") {
            if (memPool != NULL) {
                unsigned int index = 0;

                if (tokens[1] == "XOR") {
                    if (tokens[2] == "HASHPREVSHA2") {
                        for (int i = 0; i < 8; i++)
                            iResult[i] ^= prevHashSHA[i];

                        for (int i = 0; i < 8; i++)
                            index += iResult[i];

                        index = index % memory_size;
                        memcpy(iResult, memPool + index * 8, 32);
                    }
                }

                else if (tokens[1] == "ADD") {
                    if (tokens[2] == "HASHPREVSHA2") {
                        for (int i = 0; i < 8; i++)
                            iResult[i] += prevHashSHA[i];

                        for (int i = 0; i < 8; i++)
                            index += iResult[i];

                        index = index % memory_size;
                        memcpy(iResult, memPool + index * 8, 32);
                    }
                }
            }
        }


        else if (tokens[0] == "LOOP") {
            loop_line_ptr = line_ptr;
            loop_opcode_count = 0;
            for (int i = 0; i < 8; i++)
                loop_opcode_count += iResult[i];
            loop_opcode_count = loop_opcode_count % atoi(tokens[1].c_str()) + 1;
        }

        else if (tokens[0] == "ENDLOOP") {
            loop_opcode_count--;
            if (loop_opcode_count > 0)
                line_ptr = loop_line_ptr;
        }

        else if (tokens[0] == "IF") {
            uint32_t sum = 0;
            for (int i = 0; i < 8; i++)
                sum += iResult[i];
            if ((sum % atoi(tokens[1].c_str())) == 0)
                line_ptr++;
        }

        else if (tokens[0] == "STORETEMP") {
            for (int i = 0; i < 8; i++)
                temp[i] = iResult[i];
        }

        else if (tokens[0] == "EXECOP") {
            uint32_t sum = 0;
            for (int i = 0; i < 8; i++)
                sum += iResult[i];


            if (sum % 3 == 0) {
                for (int i = 0; i < 8; i++)
                    iResult[i] += temp[i];
            }
            else if (sum % 3 == 1) {
                for (int i = 0; i < 8; i++)
                    iResult[i] ^= temp[i];
            }
            else if (sum % 3 == 2) {
                unsigned char output[32];
                ctx.Reset();
                ctx.Write((unsigned char*)iResult, 32);
                ctx.Finalize(output);
                memcpy(iResult, output, 32);
            }
        }

         else if (tokens[0] == "SUMBLOCK") {
            //this calc can be optimized, although the performance gain is minimal
            uint64_t row = (iResult[0] + iResult[1] + iResult[2] + iResult[3]) % 3072;
            uint64_t col = (iResult[4] + iResult[5] + iResult[6] + iResult[7]) % 32768;
            uint64_t index = row * 32768 + col;
            const uint64_t hashBlockSize = 1024ULL * 1024ULL * 3072ULL;
            for (int i = 0; i < 256; i++)
                iResult[i % 8] += g_hashBlock[(index + i) % hashBlockSize];
        }


        line_ptr++;


        
        ///printf("end   %08X%08X%08X%08X%08X%08X%08X%08X\n",  iResult[0], iResult[1], iResult[2], iResult[3], iResult[4], iResult[5], iResult[6], iResult[7] );

        

    }


    if (memPool != NULL)
        free(memPool);

    return makeHex((unsigned char*)iResult, 32);
}


std::string CDynProgram::getProgramString() {
    std::string result;

    for (int i = 0; i < program.size(); i++)
        result += program[i] + "\n";

    return result;
}


void CDynProgram::parseHex(std::string input, unsigned char* output) {

    for (int i = 0; i < input.length(); i += 2) {
        unsigned char value = decodeHex(input[i]) * 16 + decodeHex(input[i + 1]);
        output[i/2] = value;
    }
}

unsigned char CDynProgram::decodeHex(char in) {
    in = toupper(in);
    if ((in >= '0') && (in <= '9'))
        return in - '0';
    else if ((in >= 'A') && (in <= 'F'))
        return in - 'A' + 10;
    else
        return 0;       //todo raise error
}

std::string CDynProgram::makeHex(unsigned char* in, int len)
{
    std::string result;
    for (int i = 0; i < len; i++) {
        result += hexDigit[in[i] / 16];
        result += hexDigit[in[i] % 16];
    }
    return result;
}
