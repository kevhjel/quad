// Copyright 2014-2015 Josh Pieper, jjp@pobox.com.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "property_tree_archive.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/test/auto_unit_test.hpp>

namespace {
using namespace mjmech::base;

struct TestData {
  int intval = 3;
  double doubleval = 9.1;

  template <typename Archive>
  void Serialize(Archive* a) {
    a->Visit(LT_NVP(intval));
    a->Visit(LT_NVP(doubleval));
  }
};

struct Container {
  TestData child;
  int stuff = 5;

  template <typename Archive>
  void Serialize(Archive* a) {
    a->Visit(LT_NVP(child));
    a->Visit(LT_NVP(stuff));
  }
};

namespace pt = boost::property_tree;
}

BOOST_AUTO_TEST_CASE(BasicPropertyTreeTest) {
  {
    TestData data;
    std::ostringstream ostr;
    pt::write_json(ostr,
                   PropertyTreeWriteArchive().Accept(&data).tree());
    std::string expected =
        "{\n"
        "    \"intval\": \"3\",\n"
        "    \"doubleval\": \"9.1\"\n"
        "}\n";
    BOOST_CHECK_EQUAL(ostr.str(), expected);
  }

  {
    Container container;
    std::ostringstream ostr;
    pt::write_json(ostr,
                   PropertyTreeWriteArchive().Accept(&container).tree());
    BOOST_CHECK_EQUAL(ostr.str(),
                      "{\n"
                      "    \"child\":\n"
                      "    {\n"
                      "        \"intval\": \"3\",\n"
                      "        \"doubleval\": \"9.1\"\n"
                      "    },\n"
                      "    \"stuff\": \"5\"\n"
                      "}\n");
  }
}

namespace {
struct VectorTest {
  std::vector<int> intvector;
  std::vector<TestData> structvector;

  template <typename Archive>
  void Serialize(Archive* a) {
    a->Visit(LT_NVP(intvector));
    a->Visit(LT_NVP(structvector));
  }
};
}

BOOST_AUTO_TEST_CASE(VectorPropertyTreeTest) {
  {
    VectorTest foo;
    std::ostringstream ostr;
    pt::write_json(ostr,
                   PropertyTreeWriteArchive().Accept(&foo).tree());
    std::string expected =
        "{\n"
        "    \"intvector\": \"\",\n"
        "    \"structvector\": \"\"\n"
        "}\n";
    BOOST_CHECK_EQUAL(ostr.str(), expected);
  }
  {
    VectorTest foo;
    foo.intvector = { 3, 5, 6 };
    foo.structvector.emplace_back(TestData());

    std::ostringstream ostr;
    pt::write_json(ostr,
                   PropertyTreeWriteArchive().Accept(&foo).tree());
    std::string expected =
        "{\n"
        "    \"intvector\":\n"
        "    [\n"
        "        \"3\",\n"
        "        \"5\",\n"
        "        \"6\"\n"
        "    ],\n"
        "    \"structvector\":\n"
        "    [\n"
        "        {\n"
        "            \"intval\": \"3\",\n"
        "            \"doubleval\": \"9.1\"\n"
        "        }\n"
        "    ]\n"
        "}\n";
    BOOST_CHECK_EQUAL(ostr.str(), expected);
  }
}