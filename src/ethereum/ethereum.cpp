// Copyright (c) 2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <uint256.h>
#include <arith_uint256.h>
#include <ethereum/ethereum.h>
#include <ethereum/sha3.h>
#include <services/witnessaddress.h>
#include <logging.h>
#include <util/strencodings.h>


int nibblesToTraverse(const std::string &encodedPartialPath, const std::string &path, int pathPtr) {
  std::string partialPath;
  char pathPtrInt[2] = {encodedPartialPath[0], '\0'};
  int partialPathInt = strtol(pathPtrInt, NULL, 10);
  if(partialPathInt == 0 || partialPathInt == 2){
    partialPath = encodedPartialPath.substr(2);
  }else{
    partialPath = encodedPartialPath.substr(1);
  }
  if(partialPath == path.substr(pathPtr, partialPath.size())){
    return partialPath.size();
  }else{
    return -1;
  }
}
bool VerifyProof(dev::bytesConstRef path, const dev::RLP& value, const dev::RLP& parentNodes, const dev::RLP& root) {
    try{
        dev::RLP currentNode;
        const int len = parentNodes.itemCount();
        dev::RLP nodeKey = root;       
        int pathPtr = 0;

    	const std::string pathString = dev::toHex(path);
  
        int nibbles;
        char pathPtrInt[2];
        for (int i = 0 ; i < len ; i++) {
          currentNode = parentNodes[i];
          if(!nodeKey.payload().contentsEqual(sha3(currentNode.data()).ref().toVector())){
            return false;
          } 

          if(pathPtr > (int)pathString.size()){
            return false;
          }

          switch(currentNode.itemCount()){
            case 17://branch node
              if(pathPtr == (int)pathString.size()){
                if(currentNode[16].payload().contentsEqual(value.data().toVector())){
    
                  return true;
                }else{
                  return false;
                }
              }
          
              pathPtrInt[0] = pathString[pathPtr];
              pathPtrInt[1] = '\0';

              nodeKey = currentNode[strtol(pathPtrInt, NULL, 16)]; //must == sha3(rlp.encode(currentNode[path[pathptr]]))
              pathPtr += 1;
              break;
            case 2:
              nibbles = nibblesToTraverse(toHex(currentNode[0].payload()), pathString, pathPtr);

              if(nibbles <= -1)
                return false;
              pathPtr += nibbles;
      
              if(pathPtr == (int)pathString.size()) { //leaf node
                if(currentNode[1].payload().contentsEqual(value.data().toVector())){
         
                  return true;
                } else {
                  return false;
                }
              } else {//extension node
                nodeKey = currentNode[1];
              }
              break;
            default:
              return false;
          }
        }
    }
    catch(...){
        return false;
    }
  return false;
}

/**
 * Parse eth input string expected to contain smart contract method call data. If the method call is not what we
 * expected, or the length of the expected string is not what we expect then return false.
 *
 * @param vchInputExpectedMethodHash The expected method hash
 * @param vchInputData The input to parse
 * @param outputAmount The amount burned
 * @param nAsset The asset burned
 * @param witnessAddress The destination witness address for the minting
 * @return true if everything is valid
 */
bool parseEthMethodInputData(const std::vector<unsigned char>& vchInputExpectedMethodHash, const std::vector<unsigned char>& vchInputData, CAmount& nAmount, uint32_t& nAsset, CWitnessAddress& witnessAddress) {
    // total 5 or 6 fields are expected @ 32 bytes each field, 6 fields if witness > 32 bytes + 4 byte method hash
    if(vchInputData.size() < 164 || vchInputData.size() > 196) {
      return false;  
    }
    // method hash is 4 bytes
    std::vector<unsigned char>::const_iterator firstMethod = vchInputData.begin();
    std::vector<unsigned char>::const_iterator lastMethod = firstMethod + 4;
    const std::vector<unsigned char> vchMethodHash(firstMethod,lastMethod);
    // if the method hash doesn't match the expected method hash then return false
    if(vchMethodHash != vchInputExpectedMethodHash) {
      return false;
    }

    std::vector<unsigned char> vchAmount(lastMethod,lastMethod + 32);
    // reverse endian
    std::reverse(vchAmount.begin(), vchAmount.end());
    arith_uint256 outputAmountArith = UintToArith256(uint256(vchAmount));
    nAmount = outputAmountArith.GetLow64();
    
    // convert the vch into a uint32_t (nAsset)
    // should be in position 68 walking backwards
    nAsset = static_cast<uint32_t>(vchInputData[67]);
    nAsset |= static_cast<uint32_t>(vchInputData[66]) << 8;
    nAsset |= static_cast<uint32_t>(vchInputData[65]) << 16;
    nAsset |= static_cast<uint32_t>(vchInputData[64]) << 24;
    
    size_t dataPos = 130;
    // skip data field marker (32 bytes) + 31 bytes offset to the varint _byte
    const unsigned char &dataLength = vchInputData[dataPos++] - 1; // // - 1 to account for the version byte
    // witness programs can extend to 40 bytes, min length is 2 for min witness program
    if(dataLength > 40 || dataLength < 2){
      return false;
    }

    // witness address information starting at position dataPos till the end
    // get version proceeded by witness program bytes
    const unsigned char& nVersion = vchInputData[dataPos++];
    std::vector<unsigned char>::const_iterator firstWitness = vchInputData.begin()+dataPos;
    std::vector<unsigned char>::const_iterator lastWitness = firstWitness + dataLength; 
    witnessAddress = CWitnessAddress(nVersion, std::vector<unsigned char>(firstWitness,lastWitness));
    return witnessAddress.IsValid();
}