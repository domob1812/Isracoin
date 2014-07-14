// Copyright (c) 2014 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "names.h"

#include "leveldbwrapper.h"

#include <assert.h>

/* Construct a name from a string.  */
CName
NameFromString (const std::string& str)
{
  const unsigned char* strPtr;
  strPtr = reinterpret_cast<const unsigned char*> (str.c_str ());
  return CName(strPtr, strPtr + str.size ());
}

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
