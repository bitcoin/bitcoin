#include <omnicore/rpcrequirements.h>

#include <omnicore/dbspinfo.h>
#include <omnicore/dex.h>
#include <omnicore/omnicore.h>
#include <omnicore/sp.h>
#include <omnicore/nftdb.h>
#include <omnicore/utilsbitcoin.h>

#include <amount.h>
#include <validation.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <sync.h>
#include <tinyformat.h>

#include <stdint.h>
#include <string>

void RequireBalance(const std::string& address, uint32_t propertyId, int64_t amount)
{
    int64_t balance = GetTokenBalance(address, propertyId, BALANCE);
    if (balance < amount) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance");
    }
    int64_t balanceUnconfirmed = GetAvailableTokenBalance(address, propertyId);
    if (balanceUnconfirmed < amount) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance (due to pending transactions)");
    }
}

void RequirePrimaryToken(uint32_t propertyId)
{
    if (propertyId < 1 || 2 < propertyId) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier must be 1 (OMN) or 2 (TOMN)");
    }
}

void RequirePropertyName(const std::string& name)
{
    if (name.empty()) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Property name must not be empty");
    }
}

void RequireExistingProperty(uint32_t propertyId)
{
    LOCK(cs_tally);
    if (!mastercore::IsPropertyIdValid(propertyId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    }
}

void RequireExistingDelegate(uint32_t propertyId)
{
    LOCK(cs_tally);
    if (!mastercore::HasDelegate(propertyId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property does not have a delegate");
    }
}

void RequireEmptyDelegate(uint32_t propertyId)
{
    LOCK(cs_tally);
    if (mastercore::HasDelegate(propertyId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property already has a delegate " + mastercore::GetDelegate(propertyId));
    }
}

void RequireSenderDelegateBeforeIssuer(uint32_t propertyId, const std::string& address)
{
    LOCK(cs_tally);
    CMPSPInfo::Entry sp;
    if (!mastercore::pDbSpInfo->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to retrieve property");
    }
    if (sp.delegate.empty()) {
        if (address != sp.issuer) {
            throw JSONRPCError(RPC_TYPE_ERROR, "Sender does not match issuer");
        }
    } else {
        if (address != sp.delegate) {
            throw JSONRPCError(RPC_TYPE_ERROR, "Sender does not match delegate");
        }
    }
}

void RequireSenderDelegateOrIssuer(uint32_t propertyId, const std::string& address)
{
    LOCK(cs_tally);
    CMPSPInfo::Entry sp;
    if (!mastercore::pDbSpInfo->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to retrieve property");
    }
    if (address != sp.issuer && address != sp.delegate) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Sender does not match issuer or delegate");
    }
}

void RequireMatchingDelegate(uint32_t propertyId, const std::string& address)
{
    LOCK(cs_tally);
    CMPSPInfo::Entry sp;
    if (!mastercore::pDbSpInfo->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to retrieve property");
    }
    if (sp.delegate != address) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Delegate does not belong to property");
    }
}

void RequireSameEcosystem(uint32_t propertyId, uint32_t otherId)
{
    if (mastercore::isTestEcosystemProperty(propertyId) != mastercore::isTestEcosystemProperty(otherId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Properties must be in the same ecosystem");
    }
}

void RequireDifferentIds(uint32_t propertyId, uint32_t otherId)
{
    if (propertyId == otherId) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifiers must not be the same");
    }
}

void RequireCrowdsale(uint32_t propertyId)
{
    LOCK(cs_tally);
    CMPSPInfo::Entry sp;
    if (!mastercore::pDbSpInfo->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to retrieve property");
    }
    if (sp.fixed || sp.manual) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not refer to a crowdsale");
    }
}

void RequireActiveCrowdsale(uint32_t propertyId)
{
    LOCK(cs_tally);
    if (!mastercore::isCrowdsaleActive(propertyId)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Property identifier does not refer to an active crowdsale");
    }
}

void RequireManagedProperty(uint32_t propertyId)
{
    LOCK(cs_tally);
    CMPSPInfo::Entry sp;
    if (!mastercore::pDbSpInfo->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to retrieve property");
    }
    if (sp.fixed || !sp.manual) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not refer to a managed property");
    }
}

void RequireNonFungibleProperty(uint32_t propertyId)
{
    LOCK(cs_tally);
    CMPSPInfo::Entry sp;
    if (!mastercore::pDbSpInfo->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to retrieve property");
    }
    if (!sp.unique) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not refer to a non-fungible property");
    }
}

void RequireTokenIssuer(const std::string& address, uint32_t propertyId)
{
    LOCK(cs_tally);
    CMPSPInfo::Entry sp;
    if (!mastercore::pDbSpInfo->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to retrieve property");
    }
    if (address != sp.issuer) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Sender is not authorized to manage the property");
    }
}

void RequireMatchingDExOffer(const std::string& address, uint32_t propertyId)
{
    LOCK(cs_tally);
    if (!mastercore::DEx_offerExists(address, propertyId)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "No matching sell offer on the distributed exchange");
    }
}

void RequireNoOtherDExOffer(const std::string& address)
{
    LOCK(cs_tally);
    if (mastercore::DEx_hasOffer(address)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Another active sell offer from the given address already exists on the distributed exchange");
    }
}

void RequireMatchingDExAccept(const std::string& sellerAddress, uint32_t propertyId, const std::string& buyerAddress)
{
    LOCK(cs_tally);
    if (!mastercore::DEx_acceptExists(sellerAddress, propertyId, buyerAddress)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "No matching accept order on the distributed exchange");
    }
}

void RequireSaneReferenceAmount(int64_t amount)
{
    if ((0.01 * COIN) < amount) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Reference amount higher is than 0.01 BTC");
    }
}

void RequireSaneDExPaymentWindow(const std::string& address, uint32_t propertyId)
{
    LOCK(cs_tally);
    const CMPOffer* poffer = mastercore::DEx_getOffer(address, propertyId);
    if (poffer == nullptr) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Unable to load sell offer from the distributed exchange");
    }
    if (poffer->getBlockTimeLimit() < 10) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Payment window is less than 10 blocks (use override = true to continue)");
    }
}

void RequireSaneDExFee(const std::string& address, uint32_t propertyId)
{
    LOCK(cs_tally);
    const CMPOffer* poffer = mastercore::DEx_getOffer(address, propertyId);
    if (poffer == nullptr) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Unable to load sell offer from the distributed exchange");
    }
    if (poffer->getMinFee() > 1000000) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Minimum accept fee is higher than 0.01 BTC (use override = true to continue)");
    }
}

void RequireSaneNonFungibleRange(int64_t tokenStart, int64_t tokenEnd)
{
    if (tokenStart <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unique range start value must not be zero or negative");
    }
    if (tokenEnd <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unique range end value must not be zero or negative");
    }
    if (tokenStart > tokenEnd) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unique range start value must be higher than non-fungible range end value");
    }
}

void RequireHeightInChain(int blockHeight)
{
    if (blockHeight < 0 || mastercore::GetHeight() < blockHeight) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height is out of range");
    }
}

void RequireNonFungibleTokenOwner(const std::string& address, uint32_t propertyId, int64_t tokenStart, int64_t tokenEnd)
{
    std::string rangeStartOwner = mastercore::pDbNFT->GetNonFungibleTokenOwner(propertyId, tokenStart);
    std::string rangeEndOwner = mastercore::pDbNFT->GetNonFungibleTokenOwner(propertyId, tokenEnd);
    bool contiguous = mastercore::pDbNFT->IsRangeContiguous(propertyId, tokenStart, tokenEnd);
    if (rangeStartOwner != address || rangeEndOwner != address || !contiguous) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Sender does not own the range");
    }
}
