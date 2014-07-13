// Copyright (c) 2014 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "names.h"

#include "leveldbwrapper.h"

#include <assert.h>

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
