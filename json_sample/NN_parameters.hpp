/*
 */

#ifndef _INC_NN
#define _INC_NN

#include <string>
#include <vector>
#include "picojson.h"

typedef struct {
	unsigned short layer_id;
	std::string layer_name;
	int n_input;
	int n_output;
	float *W;
	float *b;
	unsigned char a_func;
	std::string func_typ_name;
	std::string func_lay_name;
} Layer;

typedef struct {
  unsigned char ch_id;
  float parameter;
} normalize_param;


class NN_parameters {
public:
	NN_parameters();
	NN_parameters(unsigned char typ, int n_c, unsigned char ch);
	~NN_parameters();

	int setJsonString(std::string str);
	void convert();
	void dispNNParameterSUM();

	// パラメータ
	unsigned char nn_type;
	unsigned int n_ch;
	unsigned char input_ch;
	normalize_param *n_p;
	unsigned int n_layer;
	std::vector<Layer> layers;

	static std::map<std::string, unsigned char> AF_MAP;

	// NNのパラメータのバイト文字列
	unsigned char *params_byte;
	unsigned int byte_size;

private:
	void set_type_param();
	void init_normalized_param();
	
	picojson::value jobj;
	
};

#endif
