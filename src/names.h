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

/* Decode a tx output script and see if it is a name operation.  This also
   checks that the operation is well-formed.  If it looks like a name operation
   (OP_RETURN OP_NAME_*) but isn't well-formed, it isn't accepted at all
   (not just ignored).  In that case, fError is set to true.  */
bool DecodeNameScript (const CScript& script, opcodetype& op, CName& name,
                       std::vector<vchType>& args, bool& fError);

/* Construct a name registration script.  The passed-in script is
   overwritten with the constructed one.  */
void ConstructNameRegistration (CScript& out, const CName& name,
                                const CNameData& data);

#endif
