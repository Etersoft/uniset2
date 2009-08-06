#include <sstream>
#include <iostream>
#include "Element.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
const Element::ElementID Element::DefaultElementID="?id?";
// -------------------------------------------------------------------------

void Element::addChildOut( Element* el, int num )
{
	if( el == this )
	{
		ostringstream msg;
		msg << "(" << myid << "): ðïðôëá óäåìáôØ óóùìëõ îá óáíïçï óåâñ!!!";
		throw LogicException(msg.str());
	}


	for( OutputList::iterator it=outs.begin(); it!=outs.end(); ++it )
	{
		if( it->el == el )
		{
			ostringstream msg;
			msg << "(" << myid << "):" << el->getId() << " ÕÖÅ ÅÓÔØ × ÓÐÉÓËÅ ÄÏÞÅÒÎÉÈ(ÔÁËÏÅ ÓÏÅÄÉÎÅÎÉÅ ÕÖÅ ÅÓÔØ)...";
			throw LogicException(msg.str());
		}
	}

	// ÐÒÏ×ÅÒËÁ ÎÁ ÃÉËÌÉÞÅÓËÕÀ ÚÁ×ÉÓÉÍÏÓÔØ
	// el ÎÅ ÄÏÌÖÅÎ ÓÏÄÅÒÖÁÔØ × Ó×ÏÉÈ ÐÏÔÏÍËÁÈ myid
	if( el->find(myid) != NULL )
	{
		ostringstream msg;
		msg << "(" << myid << "):  ðïðùôëá óïúäáôø ãéëìéþåëõà úá÷éóéíïóôø!!!\n";
		msg << " id" << el->getId() << " ÉÍÅÅÔ × Ó×ÏÉÈ 'ÐÏÔÏÍËÁÈ' Element id=" << myid << endl;
		throw LogicException(msg.str());
	}
	
	outs.push_front(ChildInfo(el,num));
}
// -------------------------------------------------------------------------
void Element::delChildOut( Element* el )
{
	for( OutputList::iterator it=outs.begin(); it!=outs.end(); ++it )
	{
		if( it->el == el )
		{	
			outs.erase(it);
			return;
		}
	}
}

// -------------------------------------------------------------------------
void Element::setChildOut()
{
	bool _myout(getOut());
	for( OutputList::iterator it=outs.begin(); it!=outs.end(); ++it )
	{
//		try
//		{
			it->el->setIn(it->num,_myout);
//		}
//		catch(...){}
	}	
}
// -------------------------------------------------------------------------
Element* Element::find( ElementID id )
{
	for( OutputList::iterator it=outs.begin(); it!=outs.end(); ++it )
	{
		if( it->el->getId() == id )
			return it->el;
		
		Element* el( it->el->find(id) );
		if( el != NULL )
			return el;
	}
	
	return 0;
}
// -------------------------------------------------------------------------
void Element::addInput(int num, bool state)
{
	for( InputList::iterator it=ins.begin(); it!=ins.end(); ++it )
	{
		if( it->num == num )
		{
			ostringstream msg;
			msg << "(" << myid << "): ÐÏÐÙÔËÁ ×ÔÏÒÏÊ ÒÁÚ ÄÏÂÁ×ÉÔØ input N" << num;
			throw LogicException(msg.str());
		}
	}
	
	ins.push_front(InputInfo(num,state));
}
// -------------------------------------------------------------------------
void Element::delInput( int num )
{
	for( InputList::iterator it=ins.begin(); it!=ins.end(); ++it )
	{
		if( it->num == num )
		{
			ins.erase(it);
			return;
		}
	}
}
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
