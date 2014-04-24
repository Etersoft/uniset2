// --------------------------------------------------------------------------
#include <cmath>
#include "VTypes.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
namespace VTypes
{

std::ostream& operator<<( std::ostream& os, const VType& vt )
{
	return os << type2str(vt);
}

VType str2type( const std::string& s )
{
	if( s == "Byte" || s == "byte" )
		return vtByte;
	if( s == "F2" || s == "f2" )
		return vtF2;
	if( s == "F2r" || s == "f2r" )
		return vtF2r;
	if( s == "F4" || s == "f4" )
		return vtF4;
	if( s == "Unsigned" || s == "unsigned" )
		return vtUnsigned;
	if( s == "Signed" || s == "signed" )
		return vtSigned;
	if( s == "I2" || s == "i2" )
		return vtI2;
	if( s == "I2r" || s == "i2r" )
		return vtI2r;
	if( s == "U2" || s == "u2" )
		return vtU2;
	if( s == "U2r" || s == "u2r" )
		return vtU2r;

	return vtUnknown;
}
// -------------------------------------------------------------------------
string type2str( VType t )
{
	if( t == vtByte )
		return "Byte";
	if( t == vtF2 )
		return "F2";
	if( t == vtF2r )
		return "F2r";
	if( t == vtF4 )
		return "F4";
	if( t == vtUnsigned )
		return "Unsigned";
	if( t == vtSigned )
		return "Signed";
	if( t == vtI2 )
		return "I2";
	if( t == vtI2r )
		return "I2r";
	if( t == vtU2 )
		return "U2";
	if( t == vtU2r )
		return "U2r";

	return "vtUnknown";
}
// -------------------------------------------------------------------------
int wsize(  VType t )
{
	if( t == vtByte )
		return Byte::wsize();
	if( t == vtF2 || t == vtF2r )
		return F2::wsize();
	if( t == vtF4 )
		return F4::wsize();
	if( t == vtUnsigned )
		return Unsigned::wsize();
	if( t == vtSigned )
		return Signed::wsize();
	if( t == vtI2 || t == vtI2r )
		return I2::wsize();
	if( t == vtU2 || t == vtU2r )
		return U2::wsize();

	return 1;
}
// -----------------------------------------------------------------------------
} // end of namespace VTypes
// -----------------------------------------------------------------------------
