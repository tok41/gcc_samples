#include <iostream>
#include <string.h>
#include <fstream>
#include <stdio.h>

#include "lib_sample.h"


CSampleClass newCSampleClass()
{
	return static_cast<void*>(new SampleClass());
}

void delCSampleClass(CSampleClass csc)
{
	SampleClass *pc = static_cast<SampleClass*>(csc);
	delete pc;
}

void dispC(CSampleClass CS)
{
static_cast<SampleClass*>(CS)->disp();
}



SampleClass::SampleClass()
{
	// constructer
}

SampleClass::~SampleClass()
{
	// destructer
}

void SampleClass::disp()
{
	std::cout << "this is sample code!" << std::endl;
}
