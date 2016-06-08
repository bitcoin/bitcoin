// Copyright (c) 2014-2015 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef CROWNCOIN_NAMES_H
#define CROWNCOIN_NAMES_H

#include "core.h"
#include "serialize.h"

#include <stdint.h>

#include <map>
#include <set>
#include <string>
#include <vector>

class CCoinsView;
class CValidationState;

/* Format of name scripts:

OP_NAME_REGISTER:

  OP_RETURN OP_NAME_REGISTER <name> <script>

  <name> and <script> are vchType values, where <script> is the
  script corresponding to the name's desired address.

*/

class CLevelDBBatch;

/** Type representing a name internally.  */
typedef vchType CName;

/** Construct a name from a string.  */
CName NameFromString (const std::string& str);
/** Convert a name to a string.  */
std::string NameToString (const CName& name);

/**
 * Return the required (minimum) cost of a name registration.  It may return
 * -1 to signify that the name is not allowed to be registered at all.
 */
int64_t GetNameCost (const CName& name);

/**
 * Information stored internally for a name.  For now, this is just
 * the corresponding owner/recipient script.
 */
class CNameData
{

public:

  /** The name's ownership / recipient script.  */
  CScript address;

  IMPLEMENT_SERIALIZE
  (
    READWRITE (address);
  )

  /* Implement == and != operators.  */
  friend inline bool
  operator== (const CNameData& a, const CNameData& b)
  {
    return (a.address == b.address);
  }
  friend inline bool
  operator!= (const CNameData& a, const CNameData& b)
  {
    return !(a == b);
  }

};

/**
 * Cache / record of updates to the name database.  In addition to
 * new names (or updates to them), this also keeps track of deleted names
 * (when rolling back changes).
 */
class CNameCache
{

public:

  /** New or updated names.  */
  std::map<CName, CNameData> entries;
  /** Deleted names.  */
  std::set<CName> deleted;

  CNameCache ()
    : entries(), deleted()
  {}

  inline void
  Clear ()
  {
    entries.clear ();
    deleted.clear ();
  }

  /* See if the given name is marked as deleted.  */
  inline bool
  IsDeleted (const CName& name) const
  {
    return (deleted.count (name) > 0); 
  }

  /* Try to get a name's associated data.  This looks only
     in entries, and doesn't care about deleted data.  */
  bool Get (const CName& name, CNameData& data) const;

  /* Insert (or update) a name.  If it is marked as "deleted", this also
     removes the "deleted" mark.  */
  void Set (const CName& name, const CNameData& data);

  /* Delete a name.  If it is in the "entries" set also, remove it there.  */
  void Delete (const CName& name);

  /* Apply all the changes in the passed-in record on top of this one.  */
  void Apply (const CNameCache& cache);

  /* Write all cached changes to a database batch update object.  */
  void WriteBatch (CLevelDBBatch& batch) const;

};

/**
 * Undo object for name operations in a block.
 */
class CNameUndo
{

public:

  /**
   * Name registrations that have to be undone.  These names are simply
   * removed from the database when performing the undo.
   */
  std::set<CName> registrations;

  IMPLEMENT_SERIALIZE
  (
    /* For future extensions (changing of names), we store also
       a "format version".  */
    int nFormat = 1;
    READWRITE (nFormat);
    assert (nFormat == 1);

    READWRITE (registrations);
  )

  /* Check if this object is "empty".  This is enforced when writing
     the undo information before the hardfork point (so that we ensure
     that no information is lost by *not* actually writing the object
     before that).  */
  inline bool
  IsNull () const
  {
    return registrations.empty ();
  }

  /* Undo everything in here on the given coins view.  */
  bool applyUndo (CCoinsView& view) const;

};

/**
 * "Memory pool" for name operations.  This is used by CTxMemPool, and
 * makes sure that for each name, only a single tx operating on it
 * will ever be held in memory.
 */
class CNameMemPool
{

public:

  /* The names that have pending operations in the mempool.  */
  std::set<CName> names;

  inline CNameMemPool ()
    : names()
  {}

  /* See if a name has already a pending operation.  */
  inline bool
  hasName (const CName& name) const
  {
    return (names.count (name) != 0);
  }

  /* Check if a given new transaction conflicts with the names
     already in here.  */
  bool checkTransaction (const CTransaction& tx) const;

  /* Add all names appearing in the given tx.  This should only be
     called after CheckTransaction has already been fine.  */
  void addTransaction (const CTransaction& tx);

  /* Remove all entries for the given tx.  */
  void removeTransaction (const CTransaction& tx);

  /* Completely clear.  */
  inline void
  clear ()
  {
    names.clear ();
  }

  /* Return number of names in here.  This is used by the sanity checks
     of CTxMemPool.  */
  inline unsigned
  size () const
  {
    return names.size ();
  }

};

/* Decode a tx output script and see if it is a name operation.  This also
   checks that the operation is well-formed.  If it looks like a name operation
   (OP_RETURN OP_NAME_*) but isn't well-formed, it isn't accepted at all
   (not just ignored).  In that case, fError is set to true.  */
bool DecodeNameScript (const CScript& script, opcodetype& op, CName& name,
                       std::vector<vchType>& args, bool& fError);
/* See if a given tx output is a name operation.  */
bool IsNameOperation (const CTxOut& txo, CName& name);

/* Construct a name registration script.  The passed-in script is
   overwritten with the constructed one.  */
void ConstructNameRegistration (CScript& out, const CName& name,
                                const CNameData& data);

/* "Hook" for basic checking of a block.  This looks through all transactions
   in it, and verifies that each name is touched at most once by an operation
   in the block.  This is done as a preparatory step for block validation,
   before checking the transactions in detail.  */
bool CheckNamesInBlock (const CBlock& block, CValidationState& state);

/* Check a tx output from the name point-of-view.  If it looks like
   a name operation, verify that it is valid (taking also the
   chain state in coins into account).  */
bool CheckNameOperation (const CTxOut& txo, const CCoinsView& coins,
                         CValidationState& state);
/* If the tx output is a name operation, apply it to the coin view.  This also
   fills in the appropriate undo information.  */
bool ApplyNameOperation (const CTxOut& txo, CCoinsView& coins, CNameUndo& undo,
                         CValidationState& state);

#endif
