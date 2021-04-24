#include "dynhash.h"



void CDynHash::load(std::string program) {

    for (int i = 0; i < program.size(); i++)
        program[i] = toupper(program[i]);

    std::stringstream stream(program);
    std::string line;
    while (std::getline(stream, line)) {
        int32_t blockTime = atoi(line.c_str()); //TODO - year 2038 problem
        bool programDone = false;
        CDynProgram* program = new CDynProgram();
        while (!programDone) {
            std::getline(stream, line);
            if (line == "-END PROGRAM-")
                programDone = true;
            else {
                program->program.push_back(line);
            }
        }
        program->startingTime = blockTime;
        programs.push_back(program);
    }
}


std::string CDynHash::calcBlockHeaderHash(uint32_t blockTime, unsigned char* blockHeader, std::string prevBlockHash, std::string merkleRoot)
{

    bool found = false;
    int i = 0;
    while ((!found) && (i < programs.size()))
        if (blockTime < programs[i]->startingTime)
            i++;
        else
            found = true;
    if (found)
        return programs[i]->execute(blockHeader, prevBlockHash, merkleRoot);

}
