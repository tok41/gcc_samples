#include <stdio.h>
#include "test.h"

void hoge(void){
	printf("hoge!\n");
}

extern "C" {
	void c_hoge(void){
		printf("hoge!\n");
	}
}
