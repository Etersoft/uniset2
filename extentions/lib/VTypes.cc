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
	if( s == "F2" )
		return vtF2;
	if( s == "F4" )
		return vtF4;

	return vtUnknown;
}
// -------------------------------------------------------------------------
string type2str( VType t )
{
	if( t == vtF2 )
		return "F2";
	if( t == vtF4 )
		return "F4";

	return "vtUnknown";
}
// -------------------------------------------------------------------------
int wsize(  VType t )
{
	if( t == vtF2 )
		return F2::wsize();
	if( t == vtF4 )
		return F4::wsize();

	return 0;
}
// -----------------------------------------------------------------------------
} // end of namespace VTypes
// -----------------------------------------------------------------------------
