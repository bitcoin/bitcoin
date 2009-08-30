// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"










//
// Global state variables
//

//// later figure out how these are persisted
map<uint256, CProduct> mapMyProducts;




map<uint256, CProduct> mapProducts;
CCriticalSection cs_mapProducts;

bool AdvertInsert(const CProduct& product)
{
    uint256 hash = product.GetHash();
    bool fNew = false;
    bool fUpdated = false;

    CRITICAL_BLOCK(cs_mapProducts)
    {
        // Insert or find existing product
        pair<map<uint256, CProduct>::iterator, bool> item = mapProducts.insert(make_pair(hash, product));
        CProduct* pproduct = &(*(item.first)).second;
        fNew = item.second;

        // Update if newer
        if (product.nSequence > pproduct->nSequence)
        {
            *pproduct = product;
            fUpdated = true;
        }
    }

    //if (fNew)
    //    NotifyProductAdded(hash);
    //else if (fUpdated)
    //    NotifyProductUpdated(hash);

    return (fNew || fUpdated);
}

void AdvertErase(const CProduct& product)
{
    uint256 hash = product.GetHash();
    CRITICAL_BLOCK(cs_mapProducts)
        mapProducts.erase(hash);
    //NotifyProductDeleted(hash);
}



















template<typename T>
unsigned int Union(T& v1, T& v2)
{
    // v1 = v1 union v2
    // v1 and v2 must be sorted
    // returns the number of elements added to v1

    ///// need to check that this is equivalent, then delete this comment
    //vector<unsigned short> vUnion(v1.size() + v2.size());
    //vUnion.erase(set_union(v1.begin(), v1.end(),
    //                       v2.begin(), v2.end(),
    //                       vUnion.begin()),
    //             vUnion.end());

    T vUnion;
    vUnion.reserve(v1.size() + v2.size());
    set_union(v1.begin(), v1.end(),
              v2.begin(), v2.end(),
              back_inserter(vUnion));
    unsigned int nAdded = vUnion.size() - v1.size();
    if (nAdded > 0)
        v1 = vUnion;
    return nAdded;
}

void CUser::AddAtom(unsigned short nAtom, bool fOrigin)
{
    // Ignore duplicates
    if (binary_search(vAtomsIn.begin(), vAtomsIn.end(), nAtom) ||
        find(vAtomsNew.begin(), vAtomsNew.end(), nAtom) != vAtomsNew.end())
        return;

    //// instead of zero atom, should change to free atom that propagates,
    //// limited to lower than a certain value like 5 so conflicts quickly
    // The zero atom never propagates,
    // new atoms always propagate through the user that created them
    if (nAtom == 0 || fOrigin)
    {
        vector<unsigned short> vTmp(1, nAtom);
        Union(vAtomsIn, vTmp);
        if (fOrigin)
            vAtomsOut.push_back(nAtom);
        return;
    }

    vAtomsNew.push_back(nAtom);

    if (vAtomsNew.size() >= nFlowthroughRate || vAtomsOut.empty())
    {
        // Select atom to flow through to vAtomsOut
        vAtomsOut.push_back(vAtomsNew[GetRand(vAtomsNew.size())]);

        // Merge vAtomsNew into vAtomsIn
        sort(vAtomsNew.begin(), vAtomsNew.end());
        Union(vAtomsIn, vAtomsNew);
        vAtomsNew.clear();
    }
}

bool AddAtomsAndPropagate(uint256 hashUserStart, const vector<unsigned short>& vAtoms, bool fOrigin)
{
    CReviewDB reviewdb;
    map<uint256, vector<unsigned short> > pmapPropagate[2];
    pmapPropagate[0][hashUserStart] = vAtoms;

    for (int side = 0; !pmapPropagate[side].empty(); side = 1 - side)
    {
        map<uint256, vector<unsigned short> >& mapFrom = pmapPropagate[side];
        map<uint256, vector<unsigned short> >& mapTo = pmapPropagate[1 - side];

        for (map<uint256, vector<unsigned short> >::iterator mi = mapFrom.begin(); mi != mapFrom.end(); ++mi)
        {
            const uint256& hashUser = (*mi).first;
            const vector<unsigned short>& vReceived = (*mi).second;

            ///// this would be a lot easier on the database if it put the new atom at the beginning of the list,
            ///// so the change would be right next to the vector size.

            // Read user
            CUser user;
            reviewdb.ReadUser(hashUser, user);
            unsigned int nIn = user.vAtomsIn.size();
            unsigned int nNew = user.vAtomsNew.size();
            unsigned int nOut = user.vAtomsOut.size();

            // Add atoms received
            foreach(unsigned short nAtom, vReceived)
                user.AddAtom(nAtom, fOrigin);
            fOrigin = false;

            // Don't bother writing to disk if no changes
            if (user.vAtomsIn.size() == nIn && user.vAtomsNew.size() == nNew)
                continue;

            // Propagate
            if (user.vAtomsOut.size() > nOut)
                foreach(const uint256& hash, user.vLinksOut)
                    mapTo[hash].insert(mapTo[hash].end(), user.vAtomsOut.begin() + nOut, user.vAtomsOut.end());

            // Write back
            if (!reviewdb.WriteUser(hashUser, user))
                return false;
        }
        mapFrom.clear();
    }
    return true;
}






bool CReview::AcceptReview()
{
    // Timestamp
    nTime = GetTime();

    // Check signature
    if (!CKey::Verify(vchPubKeyFrom, GetSigHash(), vchSig))
        return false;

    CReviewDB reviewdb;

    // Add review text to recipient
    vector<CReview> vReviews;
    reviewdb.ReadReviews(hashTo, vReviews);
    vReviews.push_back(*this);
    if (!reviewdb.WriteReviews(hashTo, vReviews))
        return false;

    // Add link from sender
    CUser user;
    uint256 hashFrom = Hash(vchPubKeyFrom.begin(), vchPubKeyFrom.end());
    reviewdb.ReadUser(hashFrom, user);
    user.vLinksOut.push_back(hashTo);
    if (!reviewdb.WriteUser(hashFrom, user))
        return false;

    reviewdb.Close();

    // Propagate atoms to recipient
    vector<unsigned short> vZeroAtom(1, 0);
    if (!AddAtomsAndPropagate(hashTo, user.vAtomsOut.size() ? user.vAtomsOut : vZeroAtom, false))
        return false;

    return true;
}





bool CProduct::CheckSignature()
{
    return (CKey::Verify(vchPubKeyFrom, GetSigHash(), vchSig));
}

bool CProduct::CheckProduct()
{
    if (!CheckSignature())
        return false;

    // Make sure it's a summary product
    if (!mapDetails.empty() || !vOrderForm.empty())
        return false;

    // Look up seller's atom count
    CReviewDB reviewdb("r");
    CUser user;
    reviewdb.ReadUser(GetUserHash(), user);
    nAtoms = user.GetAtomCount();
    reviewdb.Close();

    ////// delme, this is now done by AdvertInsert
    //// Store to memory
    //CRITICAL_BLOCK(cs_mapProducts)
    //    mapProducts[GetHash()] = *this;

    return true;
}
