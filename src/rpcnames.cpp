// Copyright (c) 2014 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "core.h"
#include "main.h"
#include "names.h"
#include "rpcserver.h"

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

#include <sstream>

json_spirit::Value
name_getaddress (const json_spirit::Array& params, bool fHelp)
{
  if (fHelp || params.size () != 1)
    throw std::runtime_error (
        "name_getaddress \"name\"\n"
        "Look up the address corresponding to the given name."
        "  It fails if the name doesn't exist or if its associated"
        " script cannot be parsed for an address.\n"
        "\nResult:\n"
        "\"xxxx\"                        (string) address of the name\n"
        "\nExamples:\n"
        + HelpExampleCli ("name_getaddress", "\"myname\"")
        + HelpExampleRpc ("name_getaddress", "\"myname\"")
      );

  CName name;
  CNameData data;

  name = NameFromString (params[0].get_str ());
  if (!pcoinsTip->GetName (name, data))
    {
      std::ostringstream msg;
      msg << "name not found: '" << NameToString (name) << "'";
      throw JSONRPCError (RPC_NAME_NOT_FOUND, msg.str ());
    }

  CTxDestination dest;
  CBitcoinAddress addr;
  if (!ExtractDestination (data.address, dest) || !addr.Set (dest))
    throw JSONRPCError (RPC_INVALID_ADDRESS_OR_KEY,
                        "destination address cannot be extracted");

  return addr.ToString ();
}
