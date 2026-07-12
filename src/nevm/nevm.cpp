// Copyright (c) 2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <nevm/nevm.h>
#include <nevm/sha3.h>
#include <util/strencodings.h>

static bool MatchProofValue(dev::bytes node_value,
                            const dev::RLP& expected_value,
                            std::optional<uint8_t>* envelope_type)
{
  if(node_value.empty()) {
    return false;
  }

  std::optional<uint8_t> parsed_type;
  // EIP-2718 values are TransactionType || TransactionPayload, where the
  // TransactionType range is inclusive: [0x00, 0x7f]. The consensus parser
  // separately whitelists the authenticated type values it understands.
  if(node_value[0] <= 0x7f) {
    parsed_type = node_value[0];
    node_value.erase(node_value.begin());
  }
  if(node_value != expected_value.data().toBytes()) {
    return false;
  }
  if(envelope_type) {
    *envelope_type = parsed_type;
  }
  return true;
}

int nibblesToTraverse(const std::string &encodedPartialPath, const std::string &path, int pathPtr) {
  if(encodedPartialPath.empty()) {
    return -1;
  }
  std::string partialPath;
  // typecast as the character
  uint8_t partialPathInt; 
  char pathPtrInt[2] = {encodedPartialPath[0], '\0'};
  if(!ParseUInt8(pathPtrInt, &partialPathInt))
    return -1;
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
bool VerifyProof(dev::bytesConstRef path,
                 const dev::RLP& value,
                 const dev::RLP& parentNodes,
                 const dev::RLP& root,
                 std::optional<uint8_t>* envelope_type) {
  
  dev::RLP currentNode;
  const int len = parentNodes.itemCount();
  dev::RLP nodeKey = root;       
  int pathPtr = 0;

  const std::string pathString = dev::toHex(path);
  int nibbles;
  char pathPtrInt[2];
  uint8_t pathInt;
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
          // RLP-encoded transaction/receipt indexes are prefix-free, so
          // canonical Ethereum tries terminate these proofs at leaf nodes.
          // Keep generic MPT branch-value handling consistent with leaves.
          return MatchProofValue(currentNode[16].toBytes(), value, envelope_type);
        }
        pathPtrInt[0] = pathString[pathPtr];
        pathPtrInt[1] = '\0';
        if(!ParseUInt8FromHex(pathPtrInt, &pathInt)) {
          return false;
        }
        nodeKey = currentNode[pathInt]; //must == sha3(rlp.encode(currentNode[path[pathptr]]))
        pathPtr += 1;
        break;
      case 2:
        {
        const std::string encodedPartialPath = toHex(currentNode[0].payload());
        if(encodedPartialPath.empty()) {
          return false;
        }
        nibbles = nibblesToTraverse(encodedPartialPath, pathString, pathPtr);
        if(nibbles <= -1) {
          return false;
        }
        pathPtr += nibbles;
        if(pathPtr == (int)pathString.size()) { //leaf node
          dev::bytes nodeVec(currentNode[1].toBytes());
          return MatchProofValue(std::move(nodeVec), value, envelope_type);
        } else {//extension node
          nodeKey = currentNode[1];
        }
        }
        break;
      default:
        return false;
    }
  }
  
  return false;
}

