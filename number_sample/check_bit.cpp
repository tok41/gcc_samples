#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cmath>

#include <ctime>
#include <time.h>

#include <sstream>

bool check_bit(int x, int b)
{
	int cs = (int)std::pow(2, b-1);
	int y = x & cs;
	if(y > 0) return true;
	else return false;
}

int main()
{
	std::cout << "i : b1 : b2 : b3 : b4" <<std::endl;
	for (int i=1 ; i<=15 ; i++ ) {
		std::cout << i ;
		int y1 = check_bit(i, 1);
		std::cout << " : " << y1;
		int y2 = check_bit(i, 2);
		std::cout << " : " << y2;
		int y3 = check_bit(i, 3);
		std::cout << " : " << y3;
		int y4 = check_bit(i, 4);
		std::cout << " : " << y4 <<std::endl;
	}
	return 0;
}
