#include <iostream>
#include "Extensions.h"
#include "TDelay.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetExtensions;
// -------------------------------------------------------------------------
TDelay::TDelay( Element::ElementID id, int delayMS, int inCount):
    Element(id),
    myout(false),
    delay(delayMS)
{
    if( inCount!=0 )
    {
        // создаём заданное количество входов
        for( unsigned int i=1;i<=inCount;i++ )
            ins.push_front(InputInfo(i,false)); // addInput(i,st);
    }
}

TDelay::~TDelay()
{
}
// -------------------------------------------------------------------------
void TDelay::setIn( int num, bool state )
{
    bool prev = myout;
    // сбрасываем сразу
    if( !state )
    {
        pt.setTiming(0); // reset timer
        myout = false;
        dinfo << this << ": set " << myout << endl;
        if( prev != myout )
            Element::setChildOut();
        return;
    }

//    if( state )

    // выставляем без задержки
    if( delay <= 0 )
    {
        pt.setTiming(0); // reset timer
        myout = true;
        dinfo << this << ": set " << myout << endl;
        if( prev != myout )
            Element::setChildOut();
        return;
    }

    // засекаем, если ещё не установлен таймер
    if( !myout && !prev  ) // т.е. !myout && prev != myout
    {
        dinfo << this << ": set timer " << delay << " [msec]" << endl;
        pt.setTiming(delay);
    }
}
// -------------------------------------------------------------------------
void TDelay::tick()
{
    if( pt.getInterval()!=0 && pt.checkTime() )
    {
        myout = true;
        pt.setTiming(0); // reset timer
        dinfo << getType() << "(" << myid << "): TIMER!!!! myout=" << myout << endl;
        Element::setChildOut();
    }
}
// -------------------------------------------------------------------------
bool TDelay::getOut()
{ 
    return myout; 
}
// -------------------------------------------------------------------------
void TDelay::setDelay( int timeMS )
{
    delay = timeMS;
}
// -------------------------------------------------------------------------
