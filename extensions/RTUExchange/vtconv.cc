// --------------------------------------------------------------------------
//!  \version $Id: mtrconv.cc,v 1.2 2009/01/22 02:11:23 vpashka Exp $
// --------------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include "UniSetTypes.h"
#include "VTypes.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace VTypes;
// --------------------------------------------------------------------------
static void print_help()
{
	printf("Usage: vtconv TYPE[F2|F4] hex1 hex2 [hex3 hex4]\n");
}
// --------------------------------------------------------------------------
int main( int argc, const char **argv )
{   

	VTypes::F2 f2;
	f2.raw.val = 2.345;
	
	cout << "Example(F2): float=" << f2.raw.val
		<< " regs:"
		<< " v[0]=" << f2.raw.v[0]
		<< " v[1]=" << f2.raw.v[1]
		<< endl;

	VTypes::F4 f4;
	f4.raw.val = 2.345123123;
	
	cout << "Example(F4): float=" << f4.raw.val
		<< " regs:"
		<< " v[0]=" << f4.raw.v[0]
		<< " v[1]=" << f4.raw.v[1]
		<< " v[2]=" << f4.raw.v[2]
		<< " v[3]=" << f4.raw.v[3]
		<< endl;
	
	cout << "-------------" << endl << endl;

//	return 0;


	unsigned short v[4];
	memset(v,0,sizeof(v));

	const char* type="";

	if( argc<3 )
	{
		print_help();
		return 1;
	} 
	
	type = argv[1];
	v[0] = UniSetTypes::uni_atoi(argv[2]);
	
	if( argc>3 )
		v[1] = UniSetTypes::uni_atoi(argv[3]);

	if( argc>4 )
		v[2] = UniSetTypes::uni_atoi(argv[4]);

	if( argc>5 )
		v[3] = UniSetTypes::uni_atoi(argv[5]);

	if( !strcmp(type,"F2") )
	{
		VTypes::F2 f(v,sizeof(v));
		cout << "(F2): v[0]=" << v[0] 
			 << " v[1]=" << v[1]
			<< " --> (float) " << (float)f << endl;
	}
	else if( !strcmp(type,"F4") )
	{
		VTypes::F4 f(v,sizeof(v));
		cout << "(F4): v[0]=" << v[0] 
			 << " v[1]=" << v[1]
			 << " v[2]=" << v[2]
			 << " v[3]=" << v[3]
			<< " --> (float) " << (float)f << endl;
	}
	else 
	{
		cout << " Unknown type: " << type << endl;
	}

	return 0;
}
// --------------------------------------------------------------------------
