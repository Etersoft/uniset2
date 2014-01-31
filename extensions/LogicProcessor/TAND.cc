#include <iostream>
#include "Element.h"
// -------------------------------------------------------------------------
using namespace std;

// -------------------------------------------------------------------------
TAND::TAND(ElementID id, int num, bool st):
	TOR(id,num,st)
{
}

TAND::~TAND()
{
}
// -------------------------------------------------------------------------
void TAND::setIn( int num, bool state )
{
//	cout << this << ": input " << num << " set " << state << endl;
	for( InputList::iterator it=ins.begin(); it!=ins.end(); ++it )
	{
		if( it->num == num )
		{
			if( it->state == state )
				return; // вход не менялся можно вообще прервать проверку
			
			it->state = state;	
			break;
		}
	}

	bool prev = myout;
	bool brk = false; // признак досрочного завершения проверки

	// проверяем изменился ли выход
	// для тригера 'AND' проверка до первого 0
	for( InputList::iterator it=ins.begin(); it!=ins.end(); ++it )
	{
		if( !it->state )
		{
			myout = false;
			brk = true;
			break;
		}
	}
	
	if( !brk )
		myout = true;

	cout << this << ": myout " << myout << endl;	
	if( prev != myout )
		Element::setChildOut();
}
// -------------------------------------------------------------------------
