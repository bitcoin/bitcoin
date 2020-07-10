// Copyright (c) 2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <uint256.h>
#include <arith_uint256.h>
#include <ethereum/ethereum.h>
#include <ethereum/sha3.h>
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
 * @param nERC20Precision The erc20 precision to know how to convert ethereum's uint256 to a uint64 with truncation of insignifficant bits
 * @param nLocalPrecision The local precision to know how to convert ethereum's uint256 to a uint64 with truncation of insignifficant bits
 * @param vchInputData The input to parse
 * @param outputAmount The amount burned
 * @param nAsset The asset burned
 * @param witnessAddress The destination witness address for the minting
 * @return true if everything is valid
 */
bool parseEthMethodInputData(const std::vector<unsigned char>& vchInputExpectedMethodHash, const uint8_t &nERC20Precision, const uint8_t& nLocalPrecision, const std::vector<unsigned char>& vchInputData, uint64_t& outputAmount, uint32_t& nAsset, CTxDestination& witnessAddress) {
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
    // local precision can range between 0 and 8 decimal places, so it should fit within a CAmount
    // we pad zero's if erc20's precision is less than ours so we can accurately get the whole value of the amount transferred
    if(nLocalPrecision > nERC20Precision){
      outputAmountArith *= pow(10, nLocalPrecision-nERC20Precision);
    // ensure we truncate decimals to fit within int64 if erc20's precision is more than our asset precision
    } else if(nLocalPrecision < nERC20Precision){
      outputAmountArith /= pow(10, nERC20Precision-nLocalPrecision);
    }
    outputAmount = outputAmountArith.GetLow64();
    
    // convert the vch into a uint32_t (nAsset)
    // should be in position 68 walking backwards
    nAsset = static_cast<uint32_t>(vchInputData[67]);
    nAsset |= static_cast<uint32_t>(vchInputData[66]) << 8;
    nAsset |= static_cast<uint32_t>(vchInputData[65]) << 16;
    nAsset |= static_cast<uint32_t>(vchInputData[64]) << 24;
    
    // skip data field marker (32 bytes) + 31 bytes offset to the varint _byte
    const unsigned char &dataLength = vchInputData[131];
    // bech32 addresses to 46 bytes (sys1 vs bc1), min length is 9 for min witness address https://en.bitcoin.it/wiki/BIP_0173
    if(dataLength > 46 || dataLength < 9) {
      return false;
    }

    // witness address information starting at position 132 till the end
    witnessAddress = DecodeDestination(std::to_string(vchInputData.begin()+132, firstWitness + dataLength));
    return true;
}