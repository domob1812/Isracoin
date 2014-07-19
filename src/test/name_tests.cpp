// Copyright (c) 2014 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "core.h"
#include "main.h"
#include "names.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE (name_tests)

BOOST_AUTO_TEST_CASE (name_script_parsing)
{
  CNameData data;

  CBitcoinAddress addr ("i5qPw9kNW6Ce9T2jwMn3vWaRrPWDY8C4G9");
  BOOST_CHECK (addr.IsValid ());
  data.address.SetDestination (addr.Get ());
  const CName name = NameFromString ("my-cool-name");

  CScript script;
  ConstructNameRegistration (script, name, data);

  opcodetype op;
  CName name2;
  std::vector<vchType> args;
  bool error;

  BOOST_CHECK (DecodeNameScript (script, op, name2, args, error));
  BOOST_CHECK (!error);
  BOOST_CHECK (op == OP_NAME_REGISTER);
  BOOST_CHECK (name2 == name);
  BOOST_CHECK (args.size () == 1);
  BOOST_CHECK (args[0] == data.address);

  BOOST_CHECK (!DecodeNameScript (data.address, op, name2, args, error));
  BOOST_CHECK (!error);

  script = CScript ();
  script << OP_RETURN << OP_NAME_REGISTER;
  BOOST_CHECK (!DecodeNameScript (script, op, name2, args, error));
  BOOST_CHECK (error);

  script << name;
  BOOST_CHECK (!DecodeNameScript (script, op, name2, args, error));
  BOOST_CHECK (error);

  script << OP_NOP;
  BOOST_CHECK (!DecodeNameScript (script, op, name2, args, error));
  BOOST_CHECK (error);

  ConstructNameRegistration (script, name, data);
  script << OP_NOP;
  BOOST_CHECK (!DecodeNameScript (script, op, name2, args, error));
  BOOST_CHECK (error);
}

BOOST_AUTO_TEST_CASE (names_in_block)
{
  CBlock block;
  CValidationState state;

  CNameData data;
  CBitcoinAddress addr ("i5qPw9kNW6Ce9T2jwMn3vWaRrPWDY8C4G9");
  BOOST_CHECK (addr.IsValid ());
  data.address.SetDestination (addr.Get ());

  const CName name = NameFromString ("my-cool-name");
  CTxOut txo;
  txo.nValue = GetNameCost (name);
  ConstructNameRegistration (txo.scriptPubKey, name, data);

  CTransaction tx;
  tx.vout.push_back (txo);

  block.vtx.push_back (tx);
  BOOST_CHECK (CheckNamesInBlock (block, state));

  block.vtx.push_back (tx);
  BOOST_CHECK (!CheckNamesInBlock (block, state));
}

BOOST_AUTO_TEST_CASE (names_database)
{
  const CName name = NameFromString ("database-test-name");
  CNameData data, data2;
  CBitcoinAddress addr ("i5qPw9kNW6Ce9T2jwMn3vWaRrPWDY8C4G9");
  BOOST_CHECK (addr.IsValid ());
  data.address.SetDestination (addr.Get ());

  CCoinsViewCache& view = *pcoinsTip;

  BOOST_CHECK (!view.GetName (name, data2));
  BOOST_CHECK (view.SetName (name, data));
  BOOST_CHECK (view.GetName (name, data2));
  BOOST_CHECK (data == data2);
  BOOST_CHECK (view.Flush ());

  BOOST_CHECK (view.DeleteName (name));
  BOOST_CHECK (!view.GetName (name, data2));
  BOOST_CHECK (view.Flush ());
}

BOOST_AUTO_TEST_CASE (name_operations)
{
  CCoinsView dummy;
  CCoinsViewCache view(dummy);

  const CName name = NameFromString ("database-test-name");
  CNameData data, data2;
  CBitcoinAddress addr ("i5qPw9kNW6Ce9T2jwMn3vWaRrPWDY8C4G9");
  BOOST_CHECK (addr.IsValid ());
  data.address.SetDestination (addr.Get ());

  CValidationState state;
  CTxOut txo;
  ConstructNameRegistration (txo.scriptPubKey, name, data);

  txo.nValue = GetNameCost (name) - 1;
  BOOST_CHECK (!CheckNameOperation (txo, view, state));

  txo.nValue = GetNameCost (name);
  BOOST_CHECK (CheckNameOperation (txo, view, state));
  BOOST_CHECK (ApplyNameOperation (txo, view, state));
  BOOST_CHECK (view.GetName (name, data2));
  BOOST_CHECK (data == data2);
  BOOST_CHECK (!CheckNameOperation (txo, view, state));
}

BOOST_AUTO_TEST_SUITE_END ()
