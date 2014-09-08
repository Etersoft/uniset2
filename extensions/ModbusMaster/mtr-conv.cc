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
    printf("Usage: mtrconv TYPE[T1...T12,T16,T17] hex1 hex2\n");
}
// --------------------------------------------------------------------------
int main( int argc, const char **argv )
{   
    unsigned short v1 = 0;
    unsigned short v2 = 0;
    const char* type="";

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
    else if( !strcmp(type,"T16") )
    {
        T16 t(v1);
        cout << "(T16): v1=" << t.val << " float=" << t.fval << endl;
    }
    else if( !strcmp(type,"T17") )
    {
        T17 t(v1);
        cout << "(T17): v1=" << t.val << " float=" << t.fval << endl;
    }
    else if( !strcmp(type,"T3") )
    {
        T3 t(v1,v2);
        cout << "(T3): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
            << " --> " << t << endl;
    }
    else if( !strcmp(type,"T4") )
    {
        T4 t(v1);
        cout << "(T4): v1=" << t.raw
            << " --> " << t << endl;
    }
    else if( !strcmp(type,"T5") )
    {
        T5 t(v1,v2);
        cout << "(T5): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
            << " --> " << t << endl;
    }
    else if( !strcmp(type,"T6") )
    {
        T6 t(v1,v2);
        cout << "(T6): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
            << " --> " << t << endl;
    }
    else if( !strcmp(type,"T7") )
    {
        T7 t(v1,v2);
        cout << "(T7): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
//            << " --> " << T7.val << " * 10^-4"
            << " ===> " << t << endl;
    }
    else if( !strcmp(type,"T8") )
    {
        T8 t(v1,v2);
        cout << "(T8): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
            << " ===> " << t << endl;
    }
    else if( !strcmp(type,"T9") )
    {
        T9 t(v1,v2);
        cout << "(T9): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
            << " ===> " << t << endl;
    }
    else if( !strcmp(type,"T10") )
    {
        T10 t(v1,v2);
        cout << "(T10): v1=" << t.raw.v[0] << " v2=" << t.raw.v[1]
            << " ===> " << t << endl;
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
        return 1;
    }

    return 0;
}
// --------------------------------------------------------------------------
