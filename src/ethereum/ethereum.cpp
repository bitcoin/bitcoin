// Copyright (c) 2017 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ethereum/ethereum.h>
#include <ethereum/sha3.h>
#include <services/witnessaddress.h>
using namespace dev;


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
bool VerifyProof(bytesConstRef path, const RLP& value, const RLP& parentNodes, const RLP& root) {
    try{
        dev::RLP currentNode;
        const int len = parentNodes.itemCount();
        dev::RLP nodeKey = root;       
        int pathPtr = 0;

    	const std::string pathString = toHex(path);
  
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
 * @param nAsset The asset burned or 0 for SYS 
 * @return true if everything is valid
 */
bool parseEthMethodInputData(const std::vector<unsigned char>& vchInputExpectedMethodHash, const std::vector<unsigned char>& vchInputData, CAmount& outputAmount, uint32_t& nAsset, CWitnessAddress& witnessAddress) {
    // 132 for the varint position + 1 for varint + 1 for version + 3 minimum for witness program bytes
    if(vchInputData.size() < 137) 
        return false;  
    // method hash is 4 bytes
    std::vector<unsigned char>::const_iterator first = vchInputData.begin();
    std::vector<unsigned char>::const_iterator last = first + 4;
    const std::vector<unsigned char> vchMethodHash(first,last);
    // if the method hash doesn't match the expected method hash then return false
    if(vchMethodHash != vchInputExpectedMethodHash) 
        return false;

    // get the first parameter and convert to CAmount and assign to output var
    // convert the vch into a int64_t (CAmount)
    // should be in position 36 walking backwards
    uint64_t result = static_cast<uint64_t>(vchInputData[35]);
    result |= static_cast<uint64_t>(vchInputData[34]) << 8;
    result |= static_cast<uint64_t>(vchInputData[33]) << 16;
    result |= static_cast<uint64_t>(vchInputData[32]) << 24;
    result |= static_cast<uint64_t>(vchInputData[31]) << 32;
    result |= static_cast<uint64_t>(vchInputData[30]) << 40;
    result |= static_cast<uint64_t>(vchInputData[29]) << 48;
    result |= static_cast<uint64_t>(vchInputData[28]) << 56;
    outputAmount = (CAmount)result;
    
    // convert the vch into a uin32_t (nAsset)
    // should be in position 68 walking backwards
    nAsset = static_cast<uint32_t>(vchInputData[67]);
    nAsset |= static_cast<uint32_t>(vchInputData[66]) << 8;
    nAsset |= static_cast<uint32_t>(vchInputData[65]) << 16;
    nAsset |= static_cast<uint32_t>(vchInputData[64]) << 24;
    
    // skip data position field (68 + 32) + 31 (offset to the varint _byte)
    int dataPos = 131;
    const unsigned char &dataLength = vchInputData[dataPos++];
    // witness programs can extend to 40 bytes, plus 1 for version, min length is 2 for min witness program + 1 for version
    if(dataLength > 41 || dataLength < 3)
        return false;
    // witness address information starting at position dataPos till the end
    // get version proceeded by witness program bytes
    const unsigned char& nVersion = vchInputData[dataPos++];
    std::vector<unsigned char>::const_iterator firstWitness = vchInputData.begin()+dataPos;
    std::vector<unsigned char>::const_iterator lastWitness = firstWitness + (dataLength-1);
    witnessAddress = CWitnessAddress(nVersion, std::vector<unsigned char>(firstWitness,lastWitness));
    return witnessAddress.IsValid();
}