#include <vbk/test/integration/utils.hpp>

#include <base58.h>
#include <hash.h>
#include <secp256k1.h>
#include <serialize.h>
#include <streams.h>
#include <util/strencodings.h>
#include <version.h>

#include <vbk/service_locator.hpp>

namespace VeriBlockTest {

static const std::string prefix = "3056301006072A8648CE3D020106052B8104000A034200";

//Hex value of the block that you shoud put into alt-service config file as bootstrapblock "00000000000200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000027100101000000000000"
//block hash "E461382359B037993181835EAB58FEF3D0A800CFD0D24BF9"
static VeriBlock::VeriBlockBlock genesisVeriBlockBlock;

//Hex value of the block that you shoud put into alt-service config file as bootstrapblock "010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010270000FFFF7F2000000000"
//block hash "3633A2742138DB93C08DAC755CE72FECB73E196FA9D1BF36425CBD0B287B9110"
static VeriBlock::BitcoinBlock genesisBitcoinBlock;

void setUpGenesisBlocks()
{
    std::vector<uint8_t> bytes;
    genesisVeriBlockBlock.set_height(0);
    genesisVeriBlockBlock.set_nonce(0);
    genesisVeriBlockBlock.set_difficulty(16842752);
    genesisVeriBlockBlock.set_timestamp(10000);
    genesisVeriBlockBlock.set_version(2);
    bytes = ParseHex("000000000000000000000000000000000000000000000000");
    genesisVeriBlockBlock.set_previousblock(bytes.data(), bytes.size());
    genesisVeriBlockBlock.set_previouskeystone(bytes.data(), bytes.size());
    genesisVeriBlockBlock.set_secondpreviouskeystone(bytes.data(), bytes.size());
    genesisVeriBlockBlock.set_merkleroot(bytes.data(), bytes.size());

    bytes = ParseHex("0000000000000000000000000000000000000000000000000000000000000000");
    genesisBitcoinBlock.set_timestamp(10000);
    genesisBitcoinBlock.set_bits(545259519);
    genesisBitcoinBlock.set_nonce(0);
    genesisBitcoinBlock.set_previousblock(bytes.data(), bytes.size());
    genesisBitcoinBlock.set_merkleroot(bytes.data(), bytes.size());
    genesisBitcoinBlock.set_version(1);
}

std::vector<uint8_t> trimmedByteArrayFromLong(uint64_t value)
{
    int x = 8;
    do {
        if ((value >> ((x - 1) * 8)) != 0) {
            break;
        }
        x--;
    } while (x > 1);

    std::vector<uint8_t> trimmedByteArray(x);
    for (int i = 0; i < x; i++) {
        trimmedByteArray[x - i - 1] = static_cast<char>(value);
        value >>= 8;
    }

    return trimmedByteArray;
}

void mineVeriBlockBlocks(VeriBlock::VeriBlockBlock& block)
{
    auto& integrationService = VeriBlock::getService<VeriBlock::GrpcIntegrationService>();

    while (!integrationService.verifyVeriBlockBlock(block)) {
        block.set_nonce(block.nonce() + 1);
    }
}

void mineBitcoinBlock(VeriBlock::BitcoinBlock& block)
{
    auto& integrationService = VeriBlock::getService<VeriBlock::GrpcIntegrationService>();

    while (!integrationService.verifyBitcoinBlock(block)) {
        block.set_nonce(block.nonce() + 1);
    }
}

static std::vector<uint8_t> generatePubkeyFromPrivatekey(unsigned char private_key[32])
{
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    secp256k1_pubkey pubkey;
    secp256k1_ec_pubkey_create(ctx, &pubkey, private_key);
    size_t size = 65;
    unsigned char pubkey_serialized[65];
    secp256k1_ec_pubkey_serialize(ctx, pubkey_serialized, &size, &pubkey, SECP256K1_EC_UNCOMPRESSED);
    secp256k1_context_destroy(ctx);

    std::vector<uint8_t> result(pubkey_serialized, pubkey_serialized + size);
    std::vector<uint8_t> prefixBytes = ParseHex(prefix);
    result.insert(result.begin(), prefixBytes.begin(), prefixBytes.end());

    return result;
}

static std::vector<uint8_t> generateSignature(uint256& txId, unsigned char private_key[32])
{
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    uint256 txIdHash;
    CSHA256 hash_writer;
    hash_writer.Write(txId.begin(), txId.size());
    hash_writer.Finalize((uint8_t*)&txIdHash);
    hash_writer.Reset();

    std::vector<uint8_t> txIdHashBytes(txIdHash.begin(), txIdHash.end());

    secp256k1_ecdsa_signature new_signature;
    secp256k1_ecdsa_sign(ctx, &new_signature, txIdHashBytes.data(), private_key, NULL, NULL);
    secp256k1_ecdsa_signature normolize_sig;
    secp256k1_ecdsa_signature_normalize(ctx, &normolize_sig, &new_signature);
    size_t size = 64;
    unsigned char* new_signature_der = new unsigned char[size];
    if (!secp256k1_ecdsa_signature_serialize_der(ctx, new_signature_der, &size, &normolize_sig)) {
        delete[] new_signature_der;
        new_signature_der = new unsigned char[size];
        secp256k1_ecdsa_signature_serialize_der(ctx, new_signature_der, &size, &normolize_sig);
    }
    std::vector<unsigned char> result(new_signature_der, new_signature_der + size);

    delete[] new_signature_der;
    secp256k1_context_destroy(ctx);

    return result;
}

static void generateVeriBlockAdressFromPubkey(std::vector<uint8_t> pubkey, VeriBlock::Address* address)
{
    uint256 pubkeyHash;
    CSHA256 hash_writer;
    hash_writer.Write(pubkey.data(), pubkey.size());
    hash_writer.Finalize((uint8_t*)&pubkeyHash);
    hash_writer.Reset();

    std::string data = "V" + EncodeBase58(pubkeyHash.begin(), pubkeyHash.end()).substr(0, 24);


    std::vector<unsigned char> v;
    std::copy(data.begin(), data.end(), std::back_inserter(v));
    uint256 hash;
    hash_writer.Write(v.data(), v.size());
    hash_writer.Finalize((uint8_t*)&hash);
    hash_writer.Reset();

    std::string checksum = EncodeBase58(hash.begin(), hash.end());
    checksum = checksum.substr(0, 4 + 1);
    std::string new_address = data + checksum;

    address->set_address(new_address);
}

static uint256 signVeriBlockTransaction(VeriBlock::VeriBlockTransaction* tx, unsigned char private_key[32])
{
    auto& integrationService = VeriBlock::getService<VeriBlock::GrpcIntegrationService>();

    std::vector<uint8_t> transactionEffects;
    transactionEffects.push_back(tx->type());

    std::vector<uint8_t> addressBytes = integrationService.serializeAddress(tx->sourceaddress());
    transactionEffects.insert(transactionEffects.end(), addressBytes.begin(), addressBytes.end());

    std::vector<uint8_t> sourceAmountBytes = trimmedByteArrayFromLong(tx->sourceamount().atomicunits());
    transactionEffects.push_back(sourceAmountBytes.size());
    transactionEffects.insert(transactionEffects.end(), sourceAmountBytes.begin(), sourceAmountBytes.end());

    transactionEffects.push_back(tx->outputs_size());

    std::vector<uint8_t> signatureIndexBytes = trimmedByteArrayFromLong(tx->signatureindex());
    transactionEffects.push_back(signatureIndexBytes.size());
    transactionEffects.insert(transactionEffects.end(), signatureIndexBytes.begin(), signatureIndexBytes.end());

    std::vector<uint8_t> publicationDataBytes = integrationService.serializePublicationData(tx->publicationdata());
    std::vector<uint8_t> sizeBytes = trimmedByteArrayFromLong(publicationDataBytes.size());
    transactionEffects.push_back(sizeBytes.size());
    transactionEffects.insert(transactionEffects.end(), sizeBytes.begin(), sizeBytes.end());
    transactionEffects.insert(transactionEffects.end(), publicationDataBytes.begin(), publicationDataBytes.end());

    uint256 txId;
    CSHA256 hash_writer;
    hash_writer.Write(transactionEffects.data(), transactionEffects.size());
    hash_writer.Finalize((uint8_t*)&txId);
    hash_writer.Reset();

    std::vector<uint8_t> signature = generateSignature(txId, private_key);
    tx->set_signature(signature.data(), signature.size());

    return txId;
}

VeriBlock::AltPublication generateSignedAltPublication(const CBlock& endorsedBlock, const uint32_t& identifier, const CScript& payoutInfo)
{
    static unsigned char private_key[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    std::vector<uint8_t> publicKey = generatePubkeyFromPrivatekey(private_key);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << endorsedBlock.GetBlockHeader();

    VeriBlock::PublicationData* publicationData = new VeriBlock::PublicationData();
    publicationData->set_header(stream.data(), stream.size());
    publicationData->set_identifier(identifier);
    publicationData->set_payoutinfo(payoutInfo.data(), payoutInfo.size());

    VeriBlock::NetworkByte* networkByte = new VeriBlock::NetworkByte();
    networkByte->set_byteexists(false);
    networkByte->set_networkbyte(0);

    VeriBlock::Coin* coin = new VeriBlock::Coin();
    coin->set_atomicunits(1000);

    VeriBlock::Address* address = new VeriBlock::Address();
    generateVeriBlockAdressFromPubkey(publicKey, address);

    VeriBlock::VeriBlockTransaction* transaction = new VeriBlock::VeriBlockTransaction();
    transaction->set_signatureindex(7);
    transaction->set_type(1);
    transaction->set_allocated_sourceamount(coin);
    transaction->set_allocated_sourceaddress(address);
    transaction->set_publickey(publicKey.data(), publicKey.size());
    transaction->set_allocated_publicationdata(publicationData);

    uint256 txId = signVeriBlockTransaction(transaction, private_key);
    std::string merklePath = HexStr(txId.begin(), txId.end());
    uint256 merkleRoot;
    CSHA256 hash_writer;
    hash_writer.Write(txId.begin(), txId.size());
    hash_writer.Write(txId.begin(), txId.size());
    hash_writer.Finalize((uint8_t*)&merkleRoot);
    hash_writer.Reset();

    VeriBlock::VeriBlockBlock* containingBlock = new VeriBlock::VeriBlockBlock();
    containingBlock->set_difficulty(genesisVeriBlockBlock.difficulty());
    containingBlock->set_height(genesisVeriBlockBlock.height() + 1);
    containingBlock->set_merkleroot(merkleRoot.begin(), merkleRoot.size());
    std::vector<uint8_t> previousBlockBytes = ParseHex("E461382359B037993181835EAB58FEF3D0A800CFD0D24BF9");
    containingBlock->set_previousblock(previousBlockBytes.data(), previousBlockBytes.size());
    containingBlock->set_previouskeystone(genesisVeriBlockBlock.previouskeystone());
    containingBlock->set_secondpreviouskeystone(genesisVeriBlockBlock.secondpreviouskeystone());
    containingBlock->set_nonce(0);
    containingBlock->set_timestamp(endorsedBlock.nTime + 100);

    mineVeriBlockBlocks(*containingBlock);

    VeriBlock::AltPublication publication;
    publication.set_allocated_transaction(transaction);
    publication.set_merklepath("1:13:" + merklePath + ":" + merklePath);
    publication.set_allocated_containingblock(containingBlock);

    return publication;
}
} // namespace VeriBlockTest
