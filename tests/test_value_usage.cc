#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "libiqxmlrpc/value.h"

using namespace boost::unit_test_framework;
using namespace iqxmlrpc;

void nil_test()
{
  BOOST_MESSAGE("Nil test...");
  Value nil = Nil();
  BOOST_CHECK(nil.is_nil());
}

void scalar_test()
{
  BOOST_MESSAGE("Scalar types test...");

  Value vi = 10;
  BOOST_CHECK(vi.is_int());
  int i = vi;
  BOOST_CHECK_EQUAL(i, 10);
  BOOST_CHECK_EQUAL(vi.type_name(), "i4");

  Value vd = 0.33;
  BOOST_CHECK(vd.is_double());
  double d = vd;
  BOOST_CHECK_EQUAL(d, 0.33);
  BOOST_CHECK_EQUAL(vd.type_name(), "double");

  Value vs = "Test String";
  BOOST_CHECK(vs.is_string());
  std::string s = vs;
  BOOST_CHECK_EQUAL(s, "Test String");
  BOOST_CHECK_EQUAL(vs.type_name(), "string");
}

void array_test()
{
  BOOST_MESSAGE("Array test...");

  Array a;
  a.push_back("test string");
  std::fill_n(std::back_inserter(a), 9, 10);
  Value v = a;

  BOOST_CHECK(v.is_array());
  BOOST_CHECK_EQUAL(v.size(), (unsigned) 10);
  BOOST_CHECK_EQUAL(v.type_name(), "array");
  BOOST_CHECK(v[0].is_string());
  BOOST_CHECK(v[1].is_int());

  std::vector<std::string> vec;
  std::fill_n(std::back_inserter(vec), 10, "test");
  v.the_array().assign(vec.begin(), vec.end());

  Array& a1 = v.the_array();
  BOOST_CHECK_EQUAL(a1.size(), vec.size());
  for( unsigned i = 0; i < a1.size(); ++i )
    BOOST_CHECK_EQUAL(a1[i].get_string(), vec[i]);

  Array a2;
  a2 = a1;
  BOOST_CHECK_EQUAL(a2.size(), a1.size());
  for( unsigned i = 0; i < a2.size(); ++i )
    BOOST_CHECK_EQUAL(a2[i].get_string(), a1[i].get_string());

  {
    Array a;
    a.push_back(0);
    BOOST_CHECKPOINT("Suspicious Array cloning");
    std::auto_ptr<Array> a1(a.clone());
    BOOST_CHECK_EQUAL((*a1.get())[0].get_int(), 0);
  }

  {
    BOOST_CHECKPOINT("Using STL algorithms with Array iterators");

    Array a;
    std::fill_n(std::back_inserter(a), 10, 5);
    BOOST_CHECK_EQUAL(a.size(), 10);

    Array b;
    std::copy(a.begin(), a.end(), std::back_inserter(b));

    BOOST_CHECK_EQUAL(b.size(), 10);
    BOOST_CHECK_EQUAL(b[5].get_int(), 5);
  }
}

inline void check_struct_value(const Struct& s)
{
  BOOST_CHECK(s.has_field("author"));
  BOOST_CHECK(s.has_field("title"));
  BOOST_CHECK(s.has_field("pages"));
  BOOST_CHECK_EQUAL(s["author"].get_string(), "D.D.Salinger");
  BOOST_CHECK_EQUAL(s["pages"].get_int(), 250);
}

void struct_test()
{
  BOOST_MESSAGE("Struct test...");

  BOOST_CHECKPOINT("Filling the struct");
  Struct s;
  s.insert( "author", "D.D.Salinger" );
  s.insert( "title", "The catcher in the rye." );
  s.insert( "pages", 250 );

  check_struct_value(s);

  {
    BOOST_CHECKPOINT("Struct assigment");
    Struct s1;
    s1 = s;
    check_struct_value(s1);
  }

  {
    BOOST_CHECKPOINT("Struct copy ctor");
    Struct s1(s);
    check_struct_value(s1);
  }

  {
    BOOST_CHECKPOINT("Struct hand-copy");
    Struct s1;
    for (Struct::const_iterator i = s.begin(); i != s.end(); ++i)
      s1.insert(i->first, *i->second);

    check_struct_value(s1);
  }

  {
    Struct s;
    s.insert("pages", 0);

    BOOST_CHECKPOINT("Inserting 0 into struct");
    BOOST_CHECK(s["pages"].is_int());

    BOOST_CHECKPOINT("Suspicious Struct cloning");
    std::auto_ptr<Struct> s1(s.clone());
    BOOST_CHECK(s1->has_field("pages"));
  }

  Value v(s);
  BOOST_CHECK_EQUAL(v.type_name(), "struct");
}

void binary_test()
{
  BOOST_MESSAGE("Binary_data test...");
  BOOST_FAIL("TEST NOT IMPLEMENTED!");
}

void date_time_test()
{
  BOOST_MESSAGE("Date_time test...");
  BOOST_FAIL("TEST NOT IMPLEMENTED!");
}

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
  test_suite* test = BOOST_TEST_SUITE("Value types usage test");
  test->add( BOOST_TEST_CASE(&nil_test) );
  test->add( BOOST_TEST_CASE(&scalar_test) );
  test->add( BOOST_TEST_CASE(&array_test) );
  test->add( BOOST_TEST_CASE(&struct_test) );
//  test->add( BOOST_TEST_CASE(&binary_test) );
//  test->add( BOOST_TEST_CASE(&date_time_test) );

  return test;
}
