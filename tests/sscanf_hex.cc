#include <stdio.h>

bool check(const char *t, int nres)
{
	int n;
	sscanf(t, "%i", &n);
	printf("res=%d\n", n);
	if ( n != nres)
		printf("FAILED\n");
	return n == nres;
}

int main()
{
	check("100",100);
	check("0x100",0x100);
	check("0XFF",0xff);
	// check(NULL,0); // SegFault
}
