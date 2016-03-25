#include <iostream>
#include <string>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

using namespace boost::property_tree;

int main()
{
  boost::property_tree::ptree pt;
  read_json("test1.json", pt);
  write_json(std::cout, pt, false);

  if (boost::optional<int> value = pt.get_optional<int>("a")) {
	std::cout << "val : " << value.get() << std::endl;
  }
  else {
	std::cout << "nothing key" << std::endl;
  }

  std::stringstream s("{\"a\":100}");
  read_json(s, pt);
  write_json(std::cout, pt, false);
}
