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
	//std::cout << "bias : " << t["bias"] << std::endl;
	picojson::array t2 = t["bias"].get<picojson::array>();
	std::cout << "bias_size : " << t2.size() << std::endl;
	//for (int i=0 ; i<t2.size() ; i++) 
	//	std::cout << t2[i];
	//std::cout << std::endl;

	picojson::array tw = t["weight"].get<picojson::array>();
	picojson::array tw1 = tw[0].get<picojson::array>();
	picojson::array tw2 = tw1[0].get<picojson::array>();
	picojson::array tw3 = tw2[0].get<picojson::array>();
	std::cout << "weight_size : " << tw.size() <<"x"<<tw1.size()<<"x"<<tw2.size()<<"x"<<tw3.size() << std::endl;
	//for(int i=0 ; i<tw3.size() ; i++)
	//	std::cout << tw3[i] << ", ";
	//std::cout << std::endl;
	//for(int i=0 ; i<tw.size() ; i++){
	//	picojson::array tw_ = tw[i].get<picojson::array>();
	//	for(int j=0 ; j<tw_.size() ; j++) {
	//		picojson::array tw__ = tw_[j].get<picojson::array>();
	//		std::cout << tw__.size() << ", ";
	//	}
	//	std::cout << std::endl ;
	//}

	//std::cout << tw.size() << std::endl;
	//picojson::array tw0 = tw[0].get<picojson::array>();
	//std::cout << tw0.size() << std::endl;
	//std::cout << tw0[0] << std::endl;
	//picojson::array tw00 = tw0[0].get<picojson::array>();
	//std::cout << tw00.size() << std::endl;
	//std::cout << tw00[0] << std::endl;
	
	
	//std::cout << tw[0] << std::endl;
	
	return 0;
}
