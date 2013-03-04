#ifndef UTypes_H_
#define UTypes_H_
// -------------------------------------------------------------------------- 
#include <UniSetTypes.h>
// -------------------------------------------------------------------------- 
namespace UTypes
{
	const long DefaultID = UniSetTypes::DefaultObjectId;
	
	struct Params
	{
		static const int max = 20;

		Params():argc(0){}

		bool add( char* s )
		{
			if( argc < Params::max )
			{
				argv[argc++] = s;
				return true;
			}
			return false;
		}

		int argc;
		char* argv[max];
		
		static Params inst(){ return Params(); }
	};
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
