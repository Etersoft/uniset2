#include <stdio.h>

int main()
{
    int a = 5;
    int b = -5 ;
    unsigned int c = 5;
    unsigned int d = -5 ;
    int* e = (int*)&d;

    if (a < b)
        printf("a<b\n");

    if ( c < d)
        printf("c<d\n");

    printf("%x\n", *e);
}
