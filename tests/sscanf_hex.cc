#include <stdio.h>

bool check(const char *t, int nres)
{
	int n = 105;
	sscanf(t, "%i", &n);
	printf("res=%d\n", n);
	if ( n != nres )
		printf("check %d   [FAILED]\n",nres);
	return n == nres;
}

int main()
{
	check("100",100);
	check("0x100",0x100);
	check("0XFF",0xff);
	check("010",010);
	check("-10",-10);
	check("-1000",-1000);
	check("",0);
	// check(NULL,0); // SegFault
	return 0;
}
