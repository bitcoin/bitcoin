#ifndef OMNICORE_RPCREQUIREMENTS_H
#define OMNICORE_RPCREQUIREMENTS_H

#include <stdint.h>
#include <string>

void RequireBalance(const std::string& address, uint32_t propertyId, int64_t amount);
void RequirePrimaryToken(uint32_t propertyId);
void RequirePropertyName(const std::string& name);
void RequireExistingProperty(uint32_t propertyId);
void RequireSameEcosystem(uint32_t propertyId, uint32_t otherId);
void RequireDifferentIds(uint32_t propertyId, uint32_t otherId);
void RequireCrowdsale(uint32_t propertyId);
void RequireActiveCrowdsale(uint32_t propertyId);
void RequireManagedProperty(uint32_t propertyId);
void RequireTokenIssuer(const std::string& address, uint32_t propertyId);
void RequireMatchingDExOffer(const std::string& address, uint32_t propertyId);
void RequireNoOtherDExOffer(const std::string& address, uint32_t propertyId);
void RequireSaneReferenceAmount(int64_t amount);
void RequireSaneDExPaymentWindow(const std::string& address, uint32_t propertyId);
void RequireSaneDExFee(const std::string& address, uint32_t propertyId);
void RequireHeightInChain(int blockHeight);

// TODO:
// Checks for MetaDEx orders for cancel operations


#endif // OMNICORE_RPCREQUIREMENTS_H
