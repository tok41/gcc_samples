#include <iostream>
#include <fstream>
#include <string>

#include "picojson.h"

int main()
{
	// ファイルを読む
	std::string filepath = "./weight.json";
	std::ifstream ifs(filepath);
	picojson::value v;
	std::string err = picojson::parse(v, ifs);
	// *** エラー処理を書く
	
	std::map<std::string, picojson::value> &data = v.get<picojson::object>();
	std::cout << data["layers"] << std::endl;
	std::cout << data.size() << std::endl;

	for (auto& x:data) {
		std::cout << x.first << std::endl;
	}

	std::cout << "*** conv1.keys ***" << std::endl;
	std::map<std::string, picojson::value> &t = data["conv1"].get<picojson::object>();
	for (auto& x:t) {
		std::cout << x.first << std::endl;
	}
	std::cout << "*** conv1.values ***" << std::endl;
	std::cout << "type : " << t["type"] << std::endl;
	//std::cout << "weight : " << t["weight"] << std::endl;
	std::cout << "bias : " << t["bias"] << std::endl;
	
	return 0;
}
