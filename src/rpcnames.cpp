// Copyright (c) 2014 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "core.h"
#include "init.h"
#include "main.h"
#include "names.h"
#include "rpcserver.h"

#ifdef ENABLE_WALLET
# include "wallet.h"
#endif /* ENABLE_WALLET?  */

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
      throw JSONRPCError(RPC_NAME_NOT_FOUND, msg.str ());
    }

  CTxDestination dest;
  CBitcoinAddress addr;
  if (!ExtractDestination (data.address, dest) || !addr.Set (dest))
    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                       "destination address cannot be extracted");

  return addr.ToString ();
}

#ifdef ENABLE_WALLET

json_spirit::Value
sendtoname (const json_spirit::Array& params, bool fHelp)
{
  if (fHelp || params.size () < 2 || params.size () > 4)
    throw std::runtime_error (
        "sendtoaddress \"name\" amount ( \"comment\" \"comment-to\" )\n"
        "\nSent an amount to the address of a given name.  The amount is a"
        " real and is rounded to the nearest 0.00000001.\n"
        + HelpRequiringPassphrase () +
        "\nArguments:\n"
        "1. \"name\"        (string, required) The name to send to.\n"
        "2. \"amount\"      (numeric, required) The amount in ISR to send. eg 100.01\n"
        "3. \"comment\"     (string, optional) A comment used to store what the transaction is for. \n"
        "                             This is not part of the transaction, just kept in your wallet.\n"
        "4. \"comment-to\"  (string, optional) A comment to store the name of the person or organization \n"
        "                             to which you're sending the transaction. This is not part of the \n"
        "                             transaction, just kept in your wallet.\n"
        "\nResult:\n"
        "\"transactionid\"  (string) The transaction id. (view at https://blockchain.info/tx/[transactionid])\n"
        "\nExamples:\n"
        + HelpExampleCli ("sendtoname", "\"myname\" 0.1")
        + HelpExampleCli ("sendtoname", "\"myname\" 0.1 \"donation\" \"seans outpost\"")
        + HelpExampleRpc ("sendtoname", "\"myname\", 0.1, \"donation\", \"seans outpost\"")
      );

  /* Extract destination script from name database.  */

  CName name;
  CNameData data;

  name = NameFromString (params[0].get_str ());
  if (!pcoinsTip->GetName (name, data))
    {
      std::ostringstream msg;
      msg << "name not found: '" << NameToString (name) << "'";
      throw JSONRPCError (RPC_NAME_NOT_FOUND, msg.str ());
    }

  /* Amount and wallet comments, just as in "sendtoaddress".  */

  const int64_t nAmount = AmountFromValue (params[1]);
  
  CWalletTx wtx;
  if (params.size () > 2 && params[2].type () != json_spirit::null_type
      && !params[2].get_str ().empty ())
    wtx.mapValue["comment"] = params[2].get_str ();
  if (params.size () > 3 && params[3].type () != json_spirit::null_type
      && !params[3].get_str ().empty ())
    wtx.mapValue["to"] = params[3].get_str ();

  /* Perform the send.  */

  EnsureWalletIsUnlocked ();

  std::string strError = pwalletMain->SendMoney (data.address, nAmount, wtx);
  if (strError != "")
    throw JSONRPCError(RPC_WALLET_ERROR, strError);

  return wtx.GetHash ().GetHex ();
}

#endif /* ENABLE_WALLET?  */
