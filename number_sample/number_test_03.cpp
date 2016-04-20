#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ctime>
#include <time.h>

#include <sstream>

int main()
{
	unsigned char x = 0x80;
	printf("x_byte:0x%02x, x_int:%d\n", x, x);

	int y = 120;
	unsigned char y2 = (unsigned char)y;
	printf("x_byte:0x%02x, x_int:%d\n", y2, y2);
	return 0;
}
