#include <sstream>
#include <iostream>
#include "Element.h"
// -------------------------------------------------------------------------
using namespace std;

// -------------------------------------------------------------------------
TNOT::TNOT( ElementID id, bool out_default ):
	Element(id),
	myout(out_default)
{
	ins.push_front(InputInfo(1,!out_default));
}
// -------------------------------------------------------------------------
TNOT::~TNOT()
{
}
// -------------------------------------------------------------------------
void TNOT::setIn( int num, bool state )
{
	bool prev = myout;
	myout = !state;
	
	cout << this << ": myout " << myout << endl;	
	if( prev != myout )
		Element::setChildOut();
}
// -------------------------------------------------------------------------
