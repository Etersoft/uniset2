// --------------------------------------------------------------------------
//!  \version $Id: VType.cc,v 1.1 2008/12/14 21:57:50 vpashka Exp $
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

VType str2type( const std::string s )
{
	if( s == "Byte" || s == "byte" )
		return vtByte;
	if( s == "F2" || s == "f2" )
		return vtF2;
	if( s == "F4" || s == "f4" )
		return vtF4;
	if( s == "Unsigned" || s == "unsigned" )
		return vtUnsigned;
	if( s == "Signed" || s == "signed" )
		return vtSigned;

	return vtUnknown;
}
// -------------------------------------------------------------------------
string type2str( VType t )
{
	if( t == vtByte )
		return "Byte";
	if( t == vtF2 )
		return "F2";
	if( t == vtF4 )
		return "F4";
	if( t == vtUnsigned )
		return "Unsigned";
	if( t == vtSigned )
		return "Signed";

	return "vtUnknown";
}
// -------------------------------------------------------------------------
int wsize(  VType t )
{
	if( t == vtByte )
		return Byte::wsize();
	if( t == vtF2 )
		return F2::wsize();
	if( t == vtF4 )
		return F4::wsize();
	if( t == vtUnsigned )
		return Unsigned::wsize();
	if( t == vtSigned )
		return Signed::wsize();

	return 1;
}
// -----------------------------------------------------------------------------
} // end of namespace VTypes
// -----------------------------------------------------------------------------
