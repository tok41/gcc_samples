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
	float x = 0.5;
	unsigned char b[4];
	memcpy(b, &x, 4);

	printf("num : %f\n", x);
	printf("byte): ");
	for(int i=0 ; i<sizeof(b) ; i++) {
		printf("0x%02x ", b[i]);
	}
	printf("\n");
	return 0;
}
