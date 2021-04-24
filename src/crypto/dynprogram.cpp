#include "dynprogram.h"




uint256 CDynProgram::execute(unsigned char* blockHeader, uint256 prevBlockHash, uint256 merkleRoot) {

    //initial input is SHA256 of header data

    CSHA256 ctx;

    uint256 resultLE;
    ctx.Write(blockHeader, 80);
    ctx.Finalize(resultLE.begin());
    uint256 resultBE;
    for (int i = 0; i < 32; i++)
        resultBE.data()[i] = resultLE.data()[31 - i];

    arith_uint256 result = UintToArith256(resultBE);

    int line_ptr = 0;       //program execution line pointer
    int loop_counter = 0;   //counter for loop execution
    unsigned int memory_size = 0;    //size of current memory pool
    uint256* memPool = NULL;  //memory pool

    while (line_ptr < program.size()) {
        std::istringstream iss(program[line_ptr]);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};     //split line into tokens

        //simple ADD and XOR functions with one constant argument
        if (tokens[0] == "ADD") {
            arith_uint256 arg1;
            arg1.SetHex(tokens[1]);
            result += arg1;
        }

        else if (tokens[0] == "XOR") {
            arith_uint256 arg1;
            arg1.SetHex(tokens[1]);
            result ^= arg1;
        }

        //hash algo which can be optionally repeated several times
        else if (tokens[0] == "SHA2") {
            if (tokens.size() == 2) { //includes a loop count
                loop_counter = atoi(tokens[1].c_str());
                for (int i = 0; i < loop_counter; i++) {
                    if (tokens[0] == "SHA2") {
                        uint256 input = ArithToUint256(result);
                        ctx.Write(input.begin(), 32);
                        ctx.Finalize(resultLE.begin());
                        uint256 resultBE;
                        for (int i = 0; i < 32; i++)
                            resultBE.data()[i] = resultLE.data()[31 - i];
                        result = UintToArith256(resultBE);
                    }
                }
            }

            else {                         //just a single run
                if (tokens[0] == "SHA2") {
                    uint256 input = ArithToUint256(result);
                    ctx.Write(input.begin(), 32);
                    ctx.Finalize(resultLE.begin());
                    uint256 resultBE;
                    for (int i = 0; i < 32; i++)
                        resultBE.data()[i] = resultLE.data()[31 - i];
                    result = UintToArith256(resultBE);
                }
            }
        }

        //generate a block of memory based on a hashing algo
        else if (tokens[0] == "MEMGEN") {
            if (memPool != NULL)
                free(memPool);
            memory_size = atoi(tokens[2].c_str());
            memPool = (uint256*)malloc(memory_size * sizeof(uint256));
            for (int i = 0; i < memory_size; i++) {
                if (tokens[1] == "SHA2") {
                    uint256 input = ArithToUint256(result);
                    ctx.Write(input.begin(), 32);
                    ctx.Finalize(resultLE.begin());
                    uint256 resultBE;
                    for (int i = 0; i < 32; i++)
                        resultBE.data()[i] = resultLE.data()[31 - i];
                    result = UintToArith256(resultBE);
                    memPool[i] = ArithToUint256(result);
                }
            }
        }

        //add a constant to every value in the memory block
        else if (tokens[0] == "MEMADD") {
            if (memPool != NULL) {
                for (int i = 0; i < memory_size; i++) {
                    arith_uint256 arg1;
                    arg1.SetHex(tokens[1]);
                    memPool[i] = ArithToUint256(UintToArith256(memPool[i]) + arg1);
                }
            }
        }

        //xor a constant with every value in the memory block
        else if (tokens[0] == "MEMXOR") {
            if (memPool != NULL) {
                for (int i = 0; i < memory_size; i++) {
                    arith_uint256 arg1;
                    arg1.SetHex(tokens[1]);
                    memPool[i] = ArithToUint256(UintToArith256(memPool[i]) ^ arg1);
                }
            }
        }

        //read a value based on an index into the generated block of memory
        else if (tokens[0] == "READMEM") {
            if (memPool != NULL) {
                unsigned int index = 0;

                if (tokens[1] == "MERKLE") {
                    uint64_t radix = UintToArith256(merkleRoot).GetLow64();
                    index = radix % memory_size;
                    result = UintToArith256 (memPool[index]);
                }

                else if (tokens[1] == "HASHPREV") {
                    uint64_t radix = UintToArith256(prevBlockHash).GetLow64();
                    index = radix % memory_size;
                    result = UintToArith256(memPool[index]);
                }
            }
        }

        line_ptr++;


    }


    if (memPool != NULL)
        free(memPool);

    return ArithToUint256(result);
}

