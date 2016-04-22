#include <iostream>
#include <fstream>
#include <string>

#include "picojson.h"

int main()
{
	// ファイルを読む
	std::string filepath = "./weight_t.json";
	std::ifstream ifs(filepath);
	picojson::value v;
	std::string err = picojson::parse(v, ifs);
	if (! err.empty()) {
		std::cout << "JSONファイルのパースに失敗しました。" << std::endl;
	}
	
	std::map<std::string, picojson::value> &data = v.get<picojson::object>();
	std::cout << "threshold : " << data["threshold"] <<std::endl;
	std::cout << data["layers"] << ", size="<< sizeof(data["layers"]) << std::endl;

	// *** layers ***
	std::cout << "*** layers ***" << std::endl;
	for (auto& x:data) {
		if(x.first != "layers" && x.first != "threshold") {
			std::cout << "type["<< x.first << "] : " ;
			std::map<std::string, picojson::value> &t = data[x.first].get<picojson::object>();
			std::cout << t["type"] << std::endl;
		}
	}

	std::cout << "*** keys ***" << std::endl;
	for (auto& x:data) {
		if(x.first != "layers" && x.first != "threshold") {
			std::cout << x.first << " : " ;
			std::map<std::string, picojson::value> &t = data[x.first].get<picojson::object>();
			for (auto& x2:t) {
				std::cout << x2.first << ", " ;
			}
			std::cout << std::endl;
		}
	}

	//std::map<std::string, picojson::value> &t3 = data["ip1"].get<picojson::object>();
	//std::cout << "*** ip1 ***" << std::endl;
	//picojson::array tmp = t3["weight"].get<picojson::array>();
	//std::cout << "weight, size=" << tmp.size() << std::endl;
	//tmp = t3["bias"].get<picojson::array>();
	//std::cout << "bias, size=" << tmp.size() << std::endl;
	//picojson::array ip1_w = t3["weight"].get<picojson::array>();
	//std::cout <<"size weight : " << ip1_w.size()<< std::endl;
	//picojson::array ip1_w_0 = ip1_w[0].get<picojson::array>();
	//std::cout <<"size weight[0] : " << ip1_w_0.size()<< std::endl;
	//picojson::array ip1_w_1 = ip1_w[1].get<picojson::array>();
	//std::cout <<"size weight[1] : " << ip1_w_1.size()<< std::endl;
	//
	//std::map<std::string, picojson::value> &t4 = data["ipf"].get<picojson::object>();
	//std::cout << "*** ipf ***" << std::endl;
	//tmp = t4["weight"].get<picojson::array>();
	//std::cout << "weight, size=" << tmp.size() << std::endl;
	//tmp = t4["bias"].get<picojson::array>();
	//std::cout << "bias, size=" << tmp.size() <<", val="<<t4["bias"] << std::endl;
	//picojson::array ipf_w = t4["weight"].get<picojson::array>();
	//std::cout <<"size weight : " << ipf_w.size()<< std::endl;
	//picojson::array ipf_w_0 = ipf_w[0].get<picojson::array>();
	//std::cout <<"size weight[0] : " << ipf_w_0.size()<< std::endl;
	
	//std::map<std::string, picojson::value> &t = data["conv1"].get<picojson::object>();
	//std::cout << "*** conv1.values ***" << std::endl;
	//std::cout << "type : " << t["type"] << std::endl;
	//std::cout << "stride : " << t["stride"] << std::endl;
	//picojson::array t2 = t["bias"].get<picojson::array>();
	//std::cout << "bias_size : " << t2.size() << std::endl;
	//for (int i=0 ; i<t2.size() ; i++) 
	//	std::cout << t2[i];
	//std::cout << std::endl;

	//picojson::array tw = t["weight"].get<picojson::array>();
	//picojson::array tw1 = tw[0].get<picojson::array>();
	//picojson::array tw2 = tw1[0].get<picojson::array>();
	//picojson::array tw3 = tw2[0].get<picojson::array>();
	//std::cout << "weight_size : " << tw.size() <<"x"<<tw1.size()<<"x"<<tw2.size()<<"x"<<tw3.size() << std::endl;
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

	//std::cout << "**** serizlized ****" << std::endl;
	//std::cout << data["relu1"].serialize() << std::endl;
	
	return 0;
}
