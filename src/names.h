// Copyright (c) 2014 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_NAMES_H
#define BITCOIN_NAMES_H

#include "core.h"
#include "serialize.h"

#include <map>
#include <set>
#include <string>
#include <vector>

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

/* Construct a name from a string.  */
CName NameFromString (const std::string& str);
/* Convert a name to a string.  */
std::string NameToString (const CName& name);

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

/* "Memory pool" for name operations.  This is used by CTxMemPool, and
   makes sure that for each name, only a single tx operating on it
   will ever be held in memory.  */
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

#endif
