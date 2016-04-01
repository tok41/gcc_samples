#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <math.h>

#include "picojson.h"

#include "NN_parameters.hpp"

/*
  NeuralNetworkのパラメータを整形する
  2016/04/01, Y.Tokita
 */

std::map<std::string, unsigned char> NN_parameters::AF_MAP;


int main()
{
	NN_parameters NN;

	// ファイルからJSON文字列を取得する
	std::string filepath = "./weight.json";
	std::ifstream ifs(filepath);
	picojson::value v;
	std::string err = picojson::parse(v, ifs);
	if (! err.empty()) {
		std::cout << "設定ファイルのパースに失敗しました。" << std::endl;
		return -1;
	}

	// そのままシリアライズしてJSONテキストで処理オブジェクトに送る
	std::string json_str = v.serialize();
	if(int ret = NN.setJsonString(json_str) < 0 ) {
		std::cout << "JSON文字列のparseに失敗 : " << ret << std::endl;
		return 0;
	}

	// パラメータの確認
	NN.dispNNParameterSUM();

	// バイト文字列に変換
	NN.convert();

	// NNのパラメータのバイト文字列を確認する
	std::cout << "params_byte["<<NN.byte_size<<"] : ";
	for (int i=0 ; i<NN.byte_size ; i++) {
		std::cout<<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)NN.params_byte[i]<<", ";
	}

	return 0;
}

void NN_parameters::set_type_param()
{
	AF_MAP["NONE"] = 0x00;
	AF_MAP["SIGMOID"] = 0x01;
	AF_MAP["SOFTMAX"] = 0x02;
	AF_MAP["ReLU"] = 0x03;
}

void NN_parameters::init_normalized_param()
{
	n_p = new normalize_param[n_ch];
	float base_np = 1.0;
	for(int i=0 ; i<n_ch ; i++) {
		int n = pow( 2, i );
		memcpy(&n_p[i].ch_id, &n, 1);
		n_p[i].parameter = base_np;
		//std::cout<<i<<" : ch_id:0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)n_p[i].ch_id
		//		 <<", n_param:"<<std::dec<<n_p[i].parameter
		//		 <<std::endl;
	}
}

NN_parameters::NN_parameters()
{
	set_type_param();
	// constructer
	nn_type = 0x01;
	n_ch = 4;
	input_ch = 0x0F;
	init_normalized_param();
	params_byte = NULL;
}
NN_parameters::NN_parameters(unsigned char typ, int n_c, unsigned char ch)
{
	set_type_param();
	// constructer
	nn_type = typ;
	n_ch = n_c;
	input_ch = ch;
	init_normalized_param();
	params_byte = NULL;
}

NN_parameters::~NN_parameters()
{
	// destructer
	for (int i=0 ; i<layers.size() ; i++) {
		delete[] layers[i].W;
		delete[] layers[i].b;
	}
	delete[] n_p;
	if ( params_byte != NULL ) delete params_byte;
}

void NN_parameters::dispNNParameterSUM()
{
	for(int i=0 ; i<layers.size() ; i++) {
		std::cout << i
			//<<": layer_id:"<<layers[i].layer_id
				  <<", layer_name:"<<layers[i].layer_name
				  <<", n_input:"<<layers[i].n_input
				  <<", n_output:"<<layers[i].n_output
				  <<", func_layer_name:"<<layers[i].func_lay_name
				  <<", func_name:"<<layers[i].func_typ_name
				  <<", activate_f:0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)layers[i].a_func <<std::endl;
		std::cout << std::dec;
	}
}

int NN_parameters::setJsonString(std::string str)
{
	/*
	  JSON文字列を入力して、NNパラメータの整形をする
	 */
	std::string err = picojson::parse(jobj, str);
	if (! err.empty()) { return -1; }
	
	std::map<std::string, picojson::value> &data = jobj.get<picojson::object>();
	picojson::array &lays = data["layers"].get<picojson::array>();
	// 特定のLayerのパラメータだけを使う
	n_layer = 0;
	for (int i=0 ; i<lays.size() ; i++) {
		std::map<std::string, picojson::value> &t = data[lays[i].get<std::string>()].get<picojson::object>();
		//std::cout << i << ", layer=" << lays[i].get<std::string>() << ", type=" << t["type"] << std::endl;
		if ( t["type"].get<std::string>() == "FC" ) {
			Layer l;
			// Layerの名前やタイプをコピーする
			l.layer_id = n_layer;
			l.layer_name = lays[i].get<std::string>();
			if( i<lays.size()-1 ) {
				std::map<std::string, picojson::value> &af = data[lays[i+1].get<std::string>()].get<picojson::object>();
				l.func_lay_name = lays[i+1].get<std::string>();
				l.func_typ_name = af["type"].get<std::string>();
				if (AF_MAP.find(l.func_typ_name) == AF_MAP.end() ) {
					l.func_lay_name = "NONE";
					l.func_typ_name = "NONE";
					l.a_func = 0x00;
				} else {
					l.a_func = AF_MAP[l.func_typ_name];
				}
			} else {
				l.func_lay_name = "NONE";
				l.func_typ_name = "NONE";
				l.a_func = 0x00;
			}
			// Layerのノード数の設定
			picojson::array w_o = t["weight"].get<picojson::array>();
			picojson::array w_i = w_o[0].get<picojson::array>();
			l.n_input = w_i.size();
			l.n_output = w_o.size();
			// ##### ノード数を暫定的に小さく設定
			std::cout << "##### ノード数を暫定的に小さく設定" << std::endl;
			l.n_input = 2;
			l.n_output = 2;
			l.W = new float[l.n_input*l.n_output];
			l.b = new float[l.n_output];
			// 重み係数のコピー
			//for(int o=0 ; o<w_o.size() ; o++) { // こっちの方が良いと思う
			for(int o=0 ; o<l.n_output ; o++) {
				picojson::array w = w_o[o].get<picojson::array>();
				//for(int i=0 ; i<w.size() ; i++) { // こっちの方が良いと思う
				for(int i=0 ; i<l.n_input ; i++) {
					l.W[o*w_o.size()+i] = (float)w[i].get<double>();
				}
			}
			// バイアスのコピー
			picojson::array b = t["bias"].get<picojson::array>();
			//for(int o=0 ; o<b.size() ; o++) {
			for(int o=0 ; o<l.n_output ; o++) {
				l.b[o] = (float)b[o].get<double>();
			}
			// push
			layers.push_back(l);
			n_layer++;
		}
	}
	return 1;
}


void NN_parameters::convert( )
{
    // サイズを確定させるためにサイズの計算する
    byte_size = 1+1+ (1+4)*n_ch +1; // NN_type, input_ch, (ch_id, normalized_param)*n_ch, total_layer_num
    for(int i=0 ; i<n_layer ; i++)
		byte_size = byte_size + 1+2+2+ 4*(layers[i].n_input*layers[i].n_output) + 4*(layers[i].n_output) + 1;
    printf("byte_size:%d\n", byte_size);
    params_byte = new unsigned char[byte_size];
	// ネットワーク全体のパラメータ
	int pos = 0;
	memcpy(&params_byte[pos], &nn_type, 1); pos = pos + 1;
	memcpy(&params_byte[pos], &input_ch, 1); pos = pos + 1;
	// 入力CHごとの正規化定数
	for(unsigned int i=0 ; i<n_ch ; i++){
		memcpy(&params_byte[pos], &n_p[i].ch_id, 1); pos = pos + 1;
		memcpy(&params_byte[pos], &n_p[i].parameter, 4); pos = pos + 4;
	}
	memcpy(&params_byte[pos], &n_layer, 1); pos = pos + 1;
	// Layerごとのパラメータ
    for(int i=0 ; i<n_layer ; i++){
		memcpy(&params_byte[pos], &layers[i].layer_id, 1); pos = pos + 1;
		memcpy(&params_byte[pos], &layers[i].n_input, 2); pos = pos + 2;
		memcpy(&params_byte[pos], &layers[i].n_output, 2); pos = pos + 2;
		// Weight
		for(int m=0 ; m<layers[i].n_input ; m++) {
			for(int n=0 ; n<layers[i].n_output ; n++) {
				memcpy(&params_byte[pos], &layers[i].W[m*layers[i].n_output + n], 4);
				pos = pos + 4;
			}
		}
		// Bias
		for(int m=0 ; m<layers[i].n_output ; m++) {
			memcpy(&params_byte[pos], &layers[i].b[m], 4);
			pos = pos + 4;
		}
		// function
		memcpy(&params_byte[pos], &layers[i].a_func, 1);
		pos = pos + 1;
    }
}
