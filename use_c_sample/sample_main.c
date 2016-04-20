//#include <iostream>
//#include <string.h>
//#include <fstream>
//#include <stdio.h>

#include "lib_sample.h"

int main()
{
  //void *the_object;
  CSampleClass the_object;
  the_object = newCSampleClass();
  dispC(the_object);
  dispC(the_object);
  delCSampleClass(the_object);
  return 0;
}

