// --------------------------------------------------------------------------
//!  \version $Id: mtrconv.cc,v 1.2 2009/01/22 02:11:23 vpashka Exp $
// --------------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include "UniSetTypes.h"
#include "MTR.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace MTR;
// --------------------------------------------------------------------------
static void print_help()
{
	printf("Usage: mtrconv TYPE[T1...T12] hex1 hex2\n");
}
// --------------------------------------------------------------------------
int main( int argc, char **argv )
{   
	unsigned short v1 = 0;
	unsigned short v2 = 0;
	char* type="";

	if( argc<2 )
	{
		print_help();
		return 1;
	} 

	type = argv[1];
	v1 = UniSetTypes::uni_atoi(argv[2]);
	
	if( argc>=4 )
	{
		v1 = UniSetTypes::uni_atoi(argv[3]);
		v2 = UniSetTypes::uni_atoi(argv[2]);
	}


	if( !strcmp(type,"T1") )
		cout << "(T1): v1=" << v1 << " --> (unsigned) " << v1 << endl;
	else if( !strcmp(type,"T2") )
		cout << "(T2): v1=" << v1 << " --> (signed) " << (signed short)v1 << endl;
	else if( !strcmp(type,"T3") )
	{
		T3 t(v1,v2);
		cout << "(T3): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
			<< " --> " << (long)t << endl;
	}
	else if( !strcmp(type,"T4") )
	{
		T4 t(v1);
		cout << "(T4): v1=" << t.raw
			<< " --> " << t.sval << endl;
	}
	else if( !strcmp(type,"T5") )
	{
		T5 t(v1,v2);
		cout << "(T5): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1] 
			<< " --> " << t.raw.u2.val << " * 10^" << (int)t.raw.u2.exp 
			<< " ===> " << t.val << endl;
	}
	else if( !strcmp(type,"T6") )
	{
		T6 t(v1,v2);
		cout << "(T6): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1] 
			<< " --> " << t.raw.u2.val << " * 10^" << (int)t.raw.u2.exp 
			<< " ===> " << t.val << endl;
	}
	else if( !strcmp(type,"T7") )
	{
		T7 t(v1,v2);
		cout << "(T7): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1] 
//			<< " --> " << T7.val << " * 10^-4" 
			<< " ===> " << t.val 
			<< " [" << ( t.raw.u2.ic == 0xFF ? "CAP" : "IND" ) << "|"
			<< ( t.raw.u2.ie == 0xFF ? "EXP" : "IMP" ) << "]"
			<< endl;
	}
	else if( !strcmp(type,"T8") )
	{
		T8 t(v1,v2);
		cout << "(T8): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1] 
			<< " ===> " << setfill('0') << hex
			<< setw(2) << t.hour() << ":" << setw(2) << t.min()
			<< " " << setw(2) << t.day() << "/" << setw(2) << t.mon()
			<< endl;
	}
	else if( !strcmp(type,"T9") )
	{
		T9 t(v1,v2);
		cout << "(T9): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
			<< " ===> " << setfill('0') << hex
			<< setw(2) << t.hour() << ":" << setw(2) << t.min()
			<< ":" << setw(2) << t.sec() << "." << setw(2) << t.ssec()
			<< endl;
	}
	else if( !strcmp(type,"F1") )
	{
		F1 f(v1,v2);
		cout << "(F1): v1=" << f.raw.v[0] << " v2=" << f.raw.v[1]
			<< " ===> " << f.raw.val << endl;
	}
	else 
	{
		cout << " Unknown type: " << type << endl;
	}

	return 0;
}
// --------------------------------------------------------------------------
