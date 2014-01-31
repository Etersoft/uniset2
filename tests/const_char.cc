#include <string.h>

// http://www.cpptalk.net/invalid-conversion-from-char-to-const-char-vt13716.html

main(int argc, char **argv)
{
	char **t1;
	const char **t2;
	const char **t3 = new const char *[10]; // t3 - массив указателей на const char*
	char * const *t4 = new char*[10]; // t4 - константный массив указателей на char*
	const char * const * t5;
	t1 = argv;
	// t2 = argv; // error: invalid conversion from 'char**' to 'const char**'
	t2 = (const char**) argv;
	t4 = argv;
	t3[0] = argv[0];
	// t4[0] = argv[0]; // error: assignment of read-only location '* t4'
	t3[0] = new char[100];
	//strcpy(t3[0],"text"); // error: invalid conversion from 'const char*' to 'char*'
	strcpy(t4[0],"text"); // error: invalid conversion from 'const char*' to 'char*'
	t3[0] = "test";
	// t4[0] = "test"; // error: assignment of read-only location '* t4', warning: deprecated conversion from string constant to 'char*'
	t5 = argv;
	t5 = t3;
	t5 = t4;
}
