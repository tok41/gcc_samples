#include <iostream>
#include <string>

#include "picojson.h"

int main()
{
	std::string json = "{ \"aaa\":100, \"bbb\":39, \"ccc\":50 }";
	picojson::value v;
	std::string err = picojson::parse(v, json);

	//picojson::value v;
	//// parse the input
	//std::cin >> v;
	//std::string err = picojson::get_last_error();
	//if (! err.empty()) {
	//	std::cerr << err << std::endl;
	//	exit(1);
	//}

	// check if the type of the value is "object"
	if (! v.is<picojson::object>()) {
		std::cerr << "JSON is not an object" << std::endl;
		exit(2);
	}

	// obtain a const reference to the map, and print the contents
	const picojson::value::object& obj = v.get<picojson::object>();
	for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
		std::cout << i->first << ": " << i->second.to_str() << std::endl;
	}

	std::cout << obj.begin()->first << std::endl;

	std::map<std::string, picojson::value> &data = v.get<picojson::object>();
	std::cout << data["aaa"].get<double>() << std::endl;
	std::cout << data["ccc"] << std::endl;

	return 0;
}
