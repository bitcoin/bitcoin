// Copyright (c) 2014-2015 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "names.h"

#include "leveldbwrapper.h"
#include "main.h"
#include "util.h"

#include <assert.h>

/* Construct a name from a string.  */
CName
NameFromString (const std::string& str)
{
  const unsigned char* strPtr;
  strPtr = reinterpret_cast<const unsigned char*> (str.c_str ());
  return CName(strPtr, strPtr + str.size ());
}

/* Convert a name to a string.  */
std::string
NameToString (const CName& name)
{
  return std::string (name.begin (), name.end ());
}

/* Return the required (minimum) cost of a name registration.  */
int64_t
GetNameCost (const CName& name)
{
  const vchType::size_type len = name.size ();

  /* The empty name is not allowed.  */
  if (len < 1 || len > 100)
    return -1;

  /* Cost decided by the Crowncoin team.  */
  if (len <= 3)
    return 500 * COIN;
  if (len <= 15)
    return 250 * COIN;
  if (len <= 30)
    return 100 * COIN;
  if (len <= 50)
    return 50 * COIN;
  if (len <= 100)
    return 25 * COIN;

  assert (false);
}

/* ************************************************************************** */
/* CNameCache.  */

/* Try to get a name's associated data.  This looks only
   in entries, and doesn't care about deleted data.  */
bool
CNameCache::Get (const CName& name, CNameData& data) const
{
  const std::map<CName, CNameData>::const_iterator i = entries.find (name);
  if (i == entries.end ())
    return false;

  data = i->second;
  return true;
}

/* Insert (or update) a name.  If it is marked as "deleted", this also
   removes the "deleted" mark.  */
void
CNameCache::Set (const CName& name, const CNameData& data)
{
  const std::set<CName>::iterator di = deleted.find (name);
  if (di != deleted.end ())
    deleted.erase (di);

  const std::map<CName, CNameData>::iterator ei = entries.find (name);
  if (ei != entries.end ())
    ei->second = data;
  else
    entries.insert (std::make_pair (name, data));
}

/* Delete a name.  If it is in the "entries" set also, remove it there.  */
void
CNameCache::Delete (const CName& name)
{
  const std::map<CName, CNameData>::iterator ei = entries.find (name);
  if (ei != entries.end ())
    entries.erase (ei);

  deleted.insert (name);
}

/* Apply all the changes in the passed-in record on top of this one.  */
void
CNameCache::Apply (const CNameCache& cache)
{
  for (std::map<CName, CNameData>::const_iterator i = cache.entries.begin ();
       i != cache.entries.end (); ++i)
    Set (i->first, i->second);

  for (std::set<CName>::const_iterator i = cache.deleted.begin ();
       i != cache.deleted.end (); ++i)
    Delete (*i);
}

/* Write all cached changes to a database batch update object.  */
void
CNameCache::WriteBatch (CLevelDBBatch& batch) const
{
  for (std::map<CName, CNameData>::const_iterator i = entries.begin ();
       i != entries.end (); ++i)
    batch.Write (std::make_pair ('n', i->first), i->second);

  for (std::set<CName>::const_iterator i = deleted.begin ();
       i != deleted.end (); ++i)
    batch.Erase (std::make_pair ('n', *i));
}

/* ************************************************************************** */
/* CNameUndo.  */

/* Undo everything in here on the given coins view.  */
bool
CNameUndo::applyUndo (CCoinsView& view) const
{
  for (std::set<CName>::const_iterator i = registrations.begin ();
       i != registrations.end (); ++i)
    if (!view.DeleteName (*i))
      return error ("CNameUndo::applyUndo: failed to delete name '%s'",
                    NameToString (*i).c_str ());

  return true;
}

/* ************************************************************************** */
/* CNameMemPool.  */

/* Check if a given new transaction conflicts with the names
   already in here.  */
bool
CNameMemPool::checkTransaction (const CTransaction& tx) const
{
  for (std::vector<CTxOut>::const_iterator out = tx.vout.begin ();
       out != tx.vout.end (); ++out)
    {
      CName name;
      if (!IsNameOperation (*out, name))
        continue;

      if (hasName (name))
        return error ("CNameMemPool: name '%s' has a pending operation",
                      NameToString (name).c_str ());
    }

  return true;
}

/* Add all names appearing in the given tx.  This should only be
   called after CheckTransaction has already been fine.  */
void
CNameMemPool::addTransaction (const CTransaction& tx)
{
  for (std::vector<CTxOut>::const_iterator out = tx.vout.begin ();
       out != tx.vout.end (); ++out)
    {
      CName name;
      if (!IsNameOperation (*out, name))
        continue;

      names.insert (name);
    }
}

/* Remove all entries for the given tx.  */
void
CNameMemPool::removeTransaction (const CTransaction& tx)
{
  for (std::vector<CTxOut>::const_iterator out = tx.vout.begin ();
       out != tx.vout.end (); ++out)
    {
      CName name;
      if (!IsNameOperation (*out, name))
        continue;

      names.erase (name);
    }
}

/* ************************************************************************** */

/* Decode a tx output script and see if it is a name operation.  This also
   checks that the operation is well-formed.  If it looks like a name operation
   (OP_RETURN OP_NAME_*) but isn't well-formed, it isn't accepted at all
   (not just ignored).  In that case, fError is set to true.  */
bool
DecodeNameScript (const CScript& script, opcodetype& op, CName& name,
                  std::vector<vchType>& args, bool& fError)
{
  CScript::const_iterator pc = script.begin ();

  opcodetype cur;
  if (!script.GetOp (pc, cur) || cur != OP_RETURN)
    {
      fError = false;
      return false;
    }
  if (!script.GetOp (pc, op) || op != OP_NAME_REGISTER)
    {
      fError = false;
      return false;
    }

  /* Get remaining data as arguments.  The name itself is also taken care of
     as the first argument.  */
  bool haveName = false;
  args.clear ();
  while (pc != script.end ())
    {
      vchType arg;
      if (!script.GetOp (pc, cur, arg) || cur < 0 || cur > OP_PUSHDATA4)
        {
          fError = true;
          return error ("fetching name script arguments failed");
        }

      if (haveName)
        args.push_back (arg);
      else
        {
          name = arg;
          haveName = true;
        }
    }

  if (!haveName)
    {
      fError = true;
      return error ("no name found in name script");
    }

  /* For now, only OP_NAME_REGISTER is implemented.  Thus verify that the
     arguments match what they should be.  */
  if (args.size () != 1)
    {
      fError = true;
      return error ("wrong argument count for OP_NAME_REGISTER");
    }

  fError = false;
  return true;
}

/* See if a given tx output is a name operation.  */
bool
IsNameOperation (const CTxOut& txo, CName& name)
{
  opcodetype op;
  std::vector<vchType> args;
  bool fError;

  return DecodeNameScript (txo.scriptPubKey, op, name, args, fError);
}

/* Construct a name registration script.  The passed-in script is
   overwritten with the constructed one.  */
void
ConstructNameRegistration (CScript& out, const CName& name,
                           const CNameData& data)
{
  out = CScript();
  out << OP_RETURN << OP_NAME_REGISTER << name
      << static_cast<const vchType&> (data.address);
}

/* "Hook" for basic checking of a block.  This looks through all transactions
   in it, and verifies that each name is touched at most once by an operation
   in the block.  This is done as a preparatory step for block validation,
   before checking the transactions in detail.  */
bool
CheckNamesInBlock (const CBlock& block, CValidationState& state)
{
  std::set<CName> names;
  for (std::vector<CTransaction>::const_iterator tx = block.vtx.begin ();
       tx != block.vtx.end (); ++tx)
    for (std::vector<CTxOut>::const_iterator out = tx->vout.begin ();
         out != tx->vout.end (); ++out)
      {
        CName name;

        /* Note: Actual checking of the transaction is not done here.  So
           we don't care about fError, and we don't do anything except keeping
           track of the names that appear.  */

        if (!IsNameOperation (*out, name))
          continue;
          
        if (names.count (name) != 0)
          return state.Invalid (error ("CheckNamesInBlock: duplicate name '%s'",
                                       NameToString (name).c_str ()));
        names.insert (name);
      }

  return true;
}

/* Check a tx output from the name point-of-view.  If it looks like
   a name operation, verify that it is valid (taking also the
   chain state in coins into account).  */
bool
CheckNameOperation (const CTxOut& txo, const CCoinsView& coins,
                    CValidationState& state)
{
  opcodetype op;
  CName name;
  std::vector<vchType> args;
  bool fError;
  if (!DecodeNameScript (txo.scriptPubKey, op, name, args, fError))
    {
      if (fError)
        return state.Invalid (error ("CheckNameOperation: decoding of name"
                                     " script returned an error flag"));
      return true;
    }

  /* Currently, only OP_NAME_REGISTER is implemented.  */
  assert (op == OP_NAME_REGISTER && args.size () == 1);

  CNameData data;
  if (coins.GetName (name, data))
    return state.Invalid (error ("CheckNameOperation: name '%s' exists already",
                                 NameToString (name).c_str ()));

  const int64_t cost = GetNameCost (name);
  if (cost == -1)
    return state.Invalid (error ("CheckNameOperation: name '%s' is invalid",
                                 NameToString (name).c_str ()));

  assert (cost >= 0);
  if (txo.nValue < cost)
    return state.Invalid (error ("CheckNameOperation: not enough coins paid"
                                 " for '%s'",
                                 NameToString (name).c_str ()));

  return true;
}

/* If the tx output is a name operation, apply it to the coin view.  */
bool
ApplyNameOperation (const CTxOut& txo, CCoinsView& coins, CNameUndo& undo,
                    CValidationState& state)
{
  opcodetype op;
  CName name;
  std::vector<vchType> args;
  bool fError;
  if (!DecodeNameScript (txo.scriptPubKey, op, name, args, fError))
    return true;

  /* Currently, only OP_NAME_REGISTER is implemented.  */
  assert (op == OP_NAME_REGISTER && args.size () == 1);

  CNameData data;
  data.address = CScript(args[0].begin (), args[0].end ());
  if (!coins.SetName (name, data))
    return state.Abort ("ApplyNameOperation: failed to write name");

  undo.registrations.insert (name);

  return true;
}
