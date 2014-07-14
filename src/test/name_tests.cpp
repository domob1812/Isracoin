// Copyright (c) 2014 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "core.h"
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

BOOST_AUTO_TEST_SUITE_END ()
